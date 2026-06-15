// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort DPU (display processing unit) pipe: SSPP / layer
 * mixer / INTF / CTL programming, scanout bring-up, and VBIF/BCM bandwidth votes.
 * Split out of qcom_tachyon_dp.c; operates on the shared struct tachyon_dp_priv
 * passed by the dp_display parent.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <clk.h>
#include <asm/io.h>
#include <cpu_func.h>
#include <mapmem.h>
#include <video.h>
#include <dm/ofnode.h>
#include <dm/read.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/tcs.h>
#include "qcom_tachyon_dp.h"

/* Dump the DPU pixel-fetch path (CTL/SSPP/LM/INTF) to see if real pixels flow. */
void tachyon_dp_dump_dpu_state(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl, *sspp, *lm, *intf;

	if (!priv->dpu)
		return;

	ctl = priv->dpu + DPU_CTL_0_BASE;
	sspp = priv->dpu + DPU_SSPP_DMA0_BASE;
	lm = priv->dpu + DPU_LM_0_BASE;
	intf = priv->dpu + DPU_INTF_0_BASE;

	log_debug("DPU CTL: FLUSH=%08x INTF_ACTIVE=%08x FETCH_PIPE=%08x LAYER0=%08x TOP=%08x\n",
		    readl(ctl + DPU_CTL_FLUSH), readl(ctl + DPU_CTL_INTF_ACTIVE),
		    readl(ctl + DPU_CTL_FETCH_PIPE_ACTIVE),
		    readl(ctl + DPU_CTL_LAYER_0), readl(ctl + DPU_CTL_TOP));
	log_debug("DPU SSPP: SRC0_ADDR=%08x SRC_SIZE=%08x OUT_SIZE=%08x FORMAT=%08x YSTRIDE=%08x CLK=%08x\n",
		    readl(sspp + DPU_SSPP_SRC0_ADDR),
		    readl(sspp + DPU_SSPP_SRC_SIZE),
		    readl(sspp + DPU_SSPP_OUT_SIZE),
		    readl(sspp + DPU_SSPP_SRC_FORMAT),
		    readl(sspp + DPU_SSPP_SRC_YSTRIDE0),
		    readl(sspp + DPU_SSPP_CLK_CTRL));
	log_debug("DPU LM_OUT=%08x INTF_STATUS=%08x INTF_MUX=%08x INTF_UNDERFLOW_COLOR=%08x\n",
		    readl(lm + DPU_LM_OUT_SIZE), readl(intf + DPU_INTF_STATUS),
		    readl(intf + DPU_INTF_MUX),
		    readl(intf + DPU_INTF_UNDERFLOW_COLOR));

	/* Confirm the LM-composite + SSPP op-mode config actually latched. */
	log_debug("DPU LM_OP_MODE=%08x BLEND0_OP=%08x SSPP_OP_MODE=%08x MULTIRECT=%08x\n",
		    readl(lm + DPU_LM_OP_MODE), readl(lm + DPU_LM_BLEND0_OP),
		    readl(sspp + DPU_SSPP_SRC_OP_MODE),
		    readl(sspp + DPU_SSPP_MULTIRECT_OPMODE));

	/*
	 * Sample the framebuffer the SSPP is pointed at (SRC0_ADDR) so the
	 * bars-vs-black question is answered IN the dp-start log (post-PD the
	 * serial floods and md is unusable).  Invalidate first so we read what
	 * the non-coherent DPU master would see in DRAM, not a stale CPU line.
	 * Expect the 8 colour bars 00ffffff/00ffff00/0000ffff/.../00303030 at
	 * x = k*(w/8).  All-zero => the fill never landed at this address.
	 */
	{
		u32 fb_pa = readl(sspp + DPU_SSPP_SRC0_ADDR);

		if (fb_pa) {
			u32 *fb = (u32 *)(ulong)fb_pa;

			invalidate_dcache_range((ulong)fb, (ulong)fb + 0x2000);
			log_debug("DPU FB@%08x: x0=%08x x240=%08x x480=%08x x960=%08x x1680=%08x\n",
				    fb_pa, fb[0], fb[240], fb[480], fb[960], fb[1680]);
		}
	}
}

int tachyon_dpu_init(struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DPU_CLK_COUNT] = {
		"bus", "nrt_bus", "iface", "lut", "core", "vsync",
	};
	ofnode node;
	fdt_addr_t addr;
	fdt_size_t size;
	int i, ret;

	for (node = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-dpu");
	     ofnode_valid(node);
	     node = ofnode_by_compatible(node, "qcom,sc7280-dpu")) {
		if (ofnode_is_enabled(node))
			break;
	}
	if (!ofnode_valid(node))
		return -ENODEV;

	addr = ofnode_get_addr_size_index(node, 0, &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;
	priv->dpu = map_sysmem(addr, size);

	addr = ofnode_get_addr_size_index(node, 1, &size);
	if (addr != FDT_ADDR_T_NONE)
		priv->vbif = map_sysmem(addr, size);

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = clk_get_by_name_nodev(node, names[i], &priv->dpu_clks[i]);
		if (ret) {
			log_warning("DPU clock %s unavailable: %d\n", names[i],
				    ret);
			continue;
		}
		priv->dpu_clk_valid[i] = true;
	}


	return 0;
}

static int tachyon_dpu_enable_clocks(struct tachyon_dp_priv *priv)
{
	ulong core_rate;
	long rate_ret;
	int i, ret;

	core_rate = max_t(ulong, 200000000,
			  (ulong)priv->timing.pixelclock.typ * 2);
	if (priv->dpu_clk_valid[4]) {
		rate_ret = clk_set_rate(&priv->dpu_clks[4], core_rate);
		if (rate_ret < 0)
			log_warning("Failed to set DPU core clock %lu Hz: %d\n",
				    core_rate, (int)rate_ret);
	}
	if (priv->dpu_clk_valid[5])
		clk_set_rate(&priv->dpu_clks[5], 19200000);

	if (priv->dpu_clocks_enabled)
		return 0;

	for (i = 0; i < TACHYON_DPU_CLK_COUNT; i++) {
		if (!priv->dpu_clk_valid[i])
			continue;
		ret = clk_enable(&priv->dpu_clks[i]);
		if (ret && ret != -ENOSYS) {
			log_warning("Failed to enable DPU clock %d: %d\n", i,
				    ret);
			return ret;
		}
		if (!ret)
			priv->dpu_clk_enabled[i] = true;
	}

	priv->dpu_clocks_enabled = true;

	return 0;
}

static void tachyon_dpu_program_sspp(struct tachyon_dp_priv *priv,
				     struct video_uc_plat *plat,
				     struct video_priv *uc_priv)
{
	void __iomem *sspp = priv->dpu + DPU_SSPP_DMA0_BASE;
	u32 width = uc_priv->xsize;
	u32 height = uc_priv->ysize;
	u32 size = (height << 16) | width;

	writel(size, sspp + DPU_SSPP_SRC_SIZE);
	writel(0, sspp + DPU_SSPP_SRC_XY);
	writel(size, sspp + DPU_SSPP_OUT_SIZE);
	writel(0, sspp + DPU_SSPP_OUT_XY);
	writel((u32)(ulong)plat->base, sspp + DPU_SSPP_SRC0_ADDR);
	writel(0, sspp + DPU_SSPP_SRC1_ADDR);
	writel(0, sspp + DPU_SSPP_SRC2_ADDR);
	writel(0, sspp + DPU_SSPP_SRC3_ADDR);
	writel(uc_priv->line_length, sspp + DPU_SSPP_SRC_YSTRIDE0);
	writel(0, sspp + DPU_SSPP_SRC_YSTRIDE1);
	writel(DPU_FORMAT_XRGB8888, sspp + DPU_SSPP_SRC_FORMAT);
	writel(DPU_UNPACK_XRGB8888, sspp + DPU_SSPP_SRC_UNPACK_PATTERN);
	/*
	 * ROOT-CAUSE FIX: PE_OVERRIDE makes the SSPP use the SW pixel-extension
	 * REQ_PIXELS for the per-line fetch count.  Without programming it the pipe
	 * fetched 0 pixels -> staged-but-black.  Program no extension (LR/TB=0) and
	 * REQ_PIXELS = full image (height<<16 | width), then assert PE_OVERRIDE --
	 * matches Linux dpu_hw_sspp_setup_pe_config.
	 */
	writel(0, sspp + DPU_SSPP_SW_PIX_EXT_C0_LR);
	writel(0, sspp + DPU_SSPP_SW_PIX_EXT_C0_TB);
	writel((height << 16) | width, sspp + DPU_SSPP_SW_PIX_EXT_C0_REQ_PIXELS);
	writel(DPU_SSPP_PE_OVERRIDE, sspp + DPU_SSPP_SRC_OP_MODE);
	/* Force RECT_SOLO so a warm-path multirect leftover can't suppress RECT0. */
	writel(0, sspp + DPU_SSPP_MULTIRECT_OPMODE);
	writel(0x87, sspp + DPU_SSPP_FETCH_CONFIG);
	writel(0xffff, sspp + DPU_SSPP_DANGER_LUT);
	writel(0xff00, sspp + DPU_SSPP_SAFE_LUT);
	writel(0, sspp + DPU_SSPP_CREQ_LUT);
	/* Program the real 8-level CREQ QoS LUT (0x74/0x78) so the pipe isn't credit-starved. */
	writel(0x22335777, sspp + DPU_SSPP_CREQ_LUT_0);
	writel(0x00112222, sspp + DPU_SSPP_CREQ_LUT_1);
	writel(1, sspp + DPU_SSPP_QOS_CTRL);
	writel(1, sspp + DPU_SSPP_CLK_CTRL);
}

static void tachyon_dpu_program_lm(struct tachyon_dp_priv *priv,
				   struct video_priv *uc_priv)
{
	void __iomem *lm = priv->dpu + DPU_LM_0_BASE;

	writel((uc_priv->ysize << 16) | uc_priv->xsize, lm + DPU_LM_OUT_SIZE);

	/*
	 * Program the LM stage-0 blend so the SSPP layer (staged at DPU_STAGE_0
	 * in the CTL) is actually composited into the mixer output.  WITHOUT
	 * this the LM blend mux is at reset -> the mixer emits border/background
	 * only, the staged pixels are never mixed, the INTF timing engine
	 * free-runs over blanking, and the DP TX (in SEND_VIDEO) carries no
	 * active video -> the dock bridge reports "No Signal" (not a black
	 * picture).  Mirrors Linux dpu_hw_lm_setup_blend_config_combined_alpha +
	 * setup_color3 for one opaque plane on SC7280.
	 */
	writel(DPU_LM_BLEND0_OP_VAL, lm + DPU_LM_BLEND0_OP);
	writel(DPU_LM_BLEND0_CONST_ALPHA_VAL, lm + DPU_LM_BLEND0_CONST_ALPHA);
	/*
	 * LM_OP_MODE (BLEND_COLOR_OUT) selects which stage's foreground is
	 * composited into the mixer output.  Linux dpu_crtc.c:500-501 sets
	 * mixer_op_mode |= 1 << pstate->stage, and dpu_hw_lm_setup_color3
	 * (dpu_hw_lm.c:191-203) writes it into LM_OP_MODE.  For one fullscreen
	 * plane at DPU_STAGE_0 (=1) the final value is BIT(1) (STAGE0_FG_ALPHA).
	 * Writing 0 leaves the BLEND0 block (0x20/0x24) configured but NEVER
	 * composited -> mixer emits its border only = black framebuffer with a
	 * valid signal.  Must agree with CTL_LAYER mix = (2<<18).  (An earlier
	 * change from BIT(1) to 0 was this exact regression.)
	 */
	writel(DPU_LM_OP_MODE_STAGE0, lm + DPU_LM_OP_MODE);

	/*
	 * DIAGNOSTIC: paint the mixer BORDER red.  BORDER_OUT is set in the CTL,
	 * so any region not covered by the composited SSPP layer shows red.
	 * Screen RED  => mixer->INTF->DP path works, pipe NOT composited.
	 * Screen WHITE=> pipe composited (FB shows) - fix worked.
	 * Screen BLACK=> mixer output never reaches the INTF (datapath/underflow).
	 */
	writel(DPU_LM_BORDER_RED_0, lm + DPU_LM_BORDER_COLOR_0);
	writel(DPU_LM_BORDER_RED_1, lm + DPU_LM_BORDER_COLOR_1);
}

static void tachyon_dpu_program_intf(struct tachyon_dp_priv *priv,
				     struct video_priv *uc_priv)
{
	const struct display_timing *t = &priv->timing;
	void __iomem *intf = priv->dpu + DPU_INTF_0_BASE;
	u32 hsync = t->hsync_len.typ;
	u32 hbp = t->hback_porch.typ;
	u32 hfp = t->hfront_porch.typ;
	u32 vsync = t->vsync_len.typ;
	u32 vbp = t->vback_porch.typ;
	u32 vfp = t->vfront_porch.typ;
	u32 htotal = tachyon_dp_htotal(t);
	u32 vtotal = tachyon_dp_vtotal(t);
	u32 hstart = hsync + hbp;
	u32 hend = htotal - hfp - 1;
	u32 display_v_start;
	u32 display_v_end;
	u32 active_v_start;
	u32 active_v_end;

	display_v_start = ((vsync + vbp) * htotal) + hstart;
	display_v_end = ((vtotal - vfp) * htotal) - hfp - 1;
	active_v_start = display_v_start;
	active_v_end = active_v_start + uc_priv->ysize * htotal - 1;

	writel(0, intf + DPU_INTF_TIMING_ENGINE_EN);
	writel(DPU_INTF_CFG_ACTIVE_H_EN | DPU_INTF_CFG_ACTIVE_V_EN,
	       intf + DPU_INTF_CONFIG);
	writel((htotal << 16) | hsync, intf + DPU_INTF_HSYNC_CTL);
	writel(vtotal * htotal, intf + DPU_INTF_VSYNC_PERIOD_F0);
	writel(vsync * htotal, intf + DPU_INTF_VSYNC_PULSE_WIDTH_F0);
	writel(display_v_start, intf + DPU_INTF_DISPLAY_V_START_F0);
	writel(display_v_end, intf + DPU_INTF_DISPLAY_V_END_F0);
	writel(active_v_start, intf + DPU_INTF_ACTIVE_V_START_F0);
	writel(active_v_end, intf + DPU_INTF_ACTIVE_V_END_F0);
	writel((hend << 16) | hstart, intf + DPU_INTF_DISPLAY_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_ACTIVE_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_DISPLAY_DATA_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_ACTIVE_DATA_HCTL);
	writel(0, intf + DPU_INTF_BORDER_COLOR);
	/* Underflow colour = black (a transient underrun shouldn't flash). */
	writel(0, intf + DPU_INTF_UNDERFLOW_COLOR);
	writel(0, intf + DPU_INTF_HSYNC_SKEW);
	writel(0, intf + DPU_INTF_POLARITY_CTL);
	writel(DPU_INTF_CONFIG2_DATA_HCTL_EN, intf + DPU_INTF_CONFIG2);
	writel(DPU_INTF_FORMAT_XRGB8888, intf + DPU_INTF_PANEL_FORMAT);
	writel(1, intf + DPU_INTF_FRAME_LINE_COUNT_EN);
	writel(0, intf + DPU_INTF_MUX);
	/*
	 * Do NOT enable the timing engine here.  The DP transmitter must be armed
	 * (SEND_VIDEO) BEFORE the pixel source starts, otherwise the DP TX never
	 * sees a clean blanking->active transition and the DP->HDMI bridge's DP RX
	 * won't lock to the main-video stream (trained link, READY_FOR_VIDEO,
	 * frame counter advancing, yet sink "No Signal").  The engine is enabled
	 * as the FINAL step in tachyon_dp_program_mainlink, after SEND_VIDEO —
	 * mirroring Linux's post-kickoff INTF enable and the rule "Video mode must
	 * flush CTL before enabling the timing engine" (the CTL flush in
	 * tachyon_dpu_program_ctl runs with the engine off; the pending flush is
	 * consumed at the first vsync once we enable it below).
	 */
}

static int tachyon_dpu_program_ctl(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl = priv->dpu + DPU_CTL_0_BASE;
	int ret;

	writel(1, ctl + DPU_CTL_SW_RESET);
	ret = tachyon_dp_read_poll(ctl, DPU_CTL_SW_RESET, BIT(0), 0, 1000);
	if (ret)
		return ret;

	/*
	 * DIAGNOSTIC: tachyon_dp_border_only=1 stages NO pipe (border-out only) so
	 * the mixer outputs the (red) border across the whole frame.  Screen RED =>
	 * LM->PP->INTF->DP datapath works and the bug is the SSPP pipe (fetch/SMMU
	 * or staging); screen BLACK => the mixer output never reaches the INTF.
	 */
	if (tachyon_dp_env_bool("tachyon_dp_border_only"))
		writel(DPU_CTL_LAYER_BORDER_OUT, ctl + DPU_CTL_LAYER_0);
	else
		writel(DPU_CTL_LAYER_BORDER_OUT | DPU_CTL_LAYER_DMA0_STAGE0,
		       ctl + DPU_CTL_LAYER_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT2_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT3_0);
	writel(0xf0000000, ctl + DPU_CTL_TOP);
	writel(BIT(0), ctl + DPU_CTL_INTF_ACTIVE);
	writel(BIT(0), ctl + DPU_CTL_FETCH_PIPE_ACTIVE);
	writel(BIT(0), ctl + DPU_CTL_INTF_FLUSH);
	writel(BIT(0), ctl + DPU_CTL_PERIPH_FLUSH);
	writel(DPU_CTL_FLUSH_DMA0 | DPU_CTL_FLUSH_LM0 | DPU_CTL_FLUSH_CTL |
	       DPU_CTL_FLUSH_INTF | DPU_CTL_FLUSH_PERIPH,
	       ctl + DPU_CTL_FLUSH);
	writel(1, ctl + DPU_CTL_START);

	/* Poll for flush commit – HW clears DMA0 bit when pipeline goes active */
	ret = tachyon_dp_read_poll(ctl, DPU_CTL_FLUSH, DPU_CTL_FLUSH_DMA0, 0,
				   5000);
	if (ret)
		log_warning("DPU CTL flush did not commit: %d\n", ret);

	return 0;
}

/*
 * The DPU framebuffer read goes MASTER_MDP0 -> mmss_noc -> DDR.  That AXI path
 * is bandwidth-gated by an RPMh BCM vote that U-Boot otherwise never makes, so
 * the SSPP fetch is starved to zero and the pipe underruns (solid INTF
 * underflow colour) even with every DPU register correct and the FB full of
 * pixels.  Linux msm_mdss_enable() votes a mandatory MIN_IB_BW=400MB/s floor on
 * the mdp0-mem path before the AXI clocks can move data.  Replicate that by
 * voting MM1 (the one non-keepalive BCM on the path; it carries qxm_mdp0) using
 * U-Boot's already-compiled RPMh-RSC + cmd-db transport.  MM0/SH0/MC0 are
 * keepalive (held by XBL/RPMh, DRAM+LLCC already up) and only re-asserted as
 * cheap insurance.  rpmh_rsc_send_data() is the same tested path the rpmh clock
 * driver uses for its xo.lvl vote, so no raw TCS poking.
 */
struct rsc_drv;
int rpmh_rsc_send_data(struct rsc_drv *drv, const struct tcs_request *msg);

/*
 * Secure-monitor MMIO accessors (implemented in drivers/soc/qcom/qcom_adsp_pas.c).
 * Used to reach the XPU/secure-owned MMNOC MMU-TBU GDSC bank that bare EL2-NS
 * writes can't touch.
 */
int qcom_scm_io_readl(phys_addr_t addr, u32 *val);
int qcom_scm_io_writel(phys_addr_t addr, u32 val);

/* sc7280 cmd-db BCM aux record (Linux icc-rpmh.c struct bcm_db). */
struct tachyon_bcm_aux {
	__le32 unit;
	__le16 width;
	u8 vcd;
	u8 reserved;
};

/* bcm_div(): a non-zero sub-unit vote must not collapse to 0 (bcm-voter.c). */
static u64 tachyon_bcm_div(u64 num, u32 base)
{
	if (num && num < base)
		return 1;
	return base ? num / base : 0;
}

/*
 * Compute the BCM vote fields, replicating bcm_aggregate() (bcm-voter.c:99-116).
 * CRUCIAL: avg (vote_x) divides by buswidth*channels; peak (vote_y) divides by
 * buswidth ONLY.  unit/width are firmware values read from cmd-db at runtime.
 */
static u32 tachyon_bcm_vote_x(const char *name, u32 buswidth, u32 channels,
			      u32 ab_kbps)
{
	const struct tachyon_bcm_aux *aux;
	size_t len = 0;
	u64 unit, width, t;

	aux = cmd_db_read_aux_data(name, &len);
	if (IS_ERR_OR_NULL(aux) || len < sizeof(*aux))
		return 0;
	unit  = le32_to_cpu(aux->unit);
	width = le16_to_cpu(aux->width);
	if (!unit || !width || !buswidth || !channels)
		return 0;

	t = tachyon_bcm_div((u64)ab_kbps * width, (u64)buswidth * channels);
	t = tachyon_bcm_div(t * 1000, (u32)unit);
	return t > BCM_TCS_CMD_VOTE_MASK ? BCM_TCS_CMD_VOTE_MASK : (u32)t;
}

static u32 tachyon_bcm_vote_y(const char *name, u32 buswidth, u32 ib_kbps)
{
	const struct tachyon_bcm_aux *aux;
	size_t len = 0;
	u64 unit, width, t;

	aux = cmd_db_read_aux_data(name, &len);
	if (IS_ERR_OR_NULL(aux) || len < sizeof(*aux))
		return 0;
	unit  = le32_to_cpu(aux->unit);
	width = le16_to_cpu(aux->width);
	if (!unit || !width || !buswidth)
		return 0;

	t = tachyon_bcm_div((u64)ib_kbps * width, buswidth);
	t = tachyon_bcm_div(t * 1000, (u32)unit);
	return t > BCM_TCS_CMD_VOTE_MASK ? BCM_TCS_CMD_VOTE_MASK : (u32)t;
}

static int tachyon_bcm_send(struct rsc_drv *drv, u32 addr, u32 data)
{
	struct tcs_cmd cmd = { .addr = addr, .data = data, .wait = 1 };
	struct tcs_request msg = {
		.state = RPMH_ACTIVE_ONLY_STATE,
		.wait_for_compl = 1,
		.is_read = false,
		.num_cmds = 1,
		.cmds = &cmd,
	};

	if (!addr)
		return -ENODEV;
	return rpmh_rsc_send_data(drv, &msg);
}

/*
 * Linux raises the SC7280_CX rpmhpd voltage corner to SVS (RPMH level 128)
 * atomically with the 300MHz DISP_CC_MDSS_MDP_CLK via the OPP framework
 * (sc7280.dtsi power-domains=<&rpmhpd SC7280_CX>; mdp_opp_table opp-300000000
 * required-opps=<&rpmhpd_opp_svs>).  U-Boot only sets the core clock, leaving
 * the DPU datapath at the idle CX voltage -> the SSPP DRAM fetch starves even
 * at 300MHz with bandwidth granted.  Replicate the ARC corner vote: .data is
 * the INDEX into the firmware "cx.lvl" level table for the first level >= 128
 * (rpmhpd.c rpmhpd_send_corner), sent active-only over the same RSC.
 */
static void tachyon_dp_vote_cx_corner(struct rsc_drv *drv)
{
	const __le16 *lvl;
	size_t len = 0, n, i;
	u32 addr;
	int idx = -1;

	addr = cmd_db_read_addr("cx.lvl");
	if (!addr) {
		log_warning("DP: no cx.lvl ARC addr; skipping CX corner vote\n");
		return;
	}
	lvl = cmd_db_read_aux_data("cx.lvl", &len);
	if (IS_ERR_OR_NULL(lvl) || len < sizeof(*lvl)) {
		log_warning("DP: no cx.lvl aux data; skipping CX corner vote\n");
		return;
	}
	n = len / sizeof(*lvl);
	for (i = 0; i < n; i++) {
		u16 v = le16_to_cpu(lvl[i]);

		if (i > 0 && v == 0)		/* zero-padded tail */
			break;
		if (v >= 128) {			/* RPMH_REGULATOR_LEVEL_SVS */
			idx = (int)i;
			break;
		}
	}
	if (idx < 0)
		idx = (int)n - 1;		/* clamp to max corner */

	{
		struct tcs_cmd cmd = { .addr = addr, .data = (u32)idx, .wait = 1 };
		struct tcs_request msg = {
			.state = RPMH_ACTIVE_ONLY_STATE, .wait_for_compl = 1,
			.is_read = false, .num_cmds = 1, .cmds = &cmd,
		};
		int ret = rpmh_rsc_send_data(drv, &msg);

		log_debug("DP: CX corner vote addr=%#x idx=%d ret=%d\n",
			    addr, idx, ret);
	}
}

static void tachyon_dp_grant_mdp0_bandwidth(void)
{
	/*
	 * Grant the SSPP its DRAM read throughput, replicating what Linux does
	 * over the same apps_rsc/RPMh transport:
	 *  (1) Vote the REAL 1080p60 bandwidth (AB~622MB/s, IB=1.6GB/s) on EVERY
	 *      BCM of the MASTER_MDP0->EBI path (icc_set_bw fans a path vote out to
	 *      all nodes): MM1(qxm_mdp0) MM0(qns_mem_noc_hf) SH0(qns_llcc) MC0(ebi),
	 *      each with its OWN buswidth+channels, + the ACV mask BCM.  Voting only
	 *      MM1 leaves the DDR/LLCC bus clocks unraised -> still starved.
	 *  (2) Raise the SC7280_CX corner the 300MHz core clock requires.
	 */
	const u32 ab_kbps = 622080;	/* 1080p60 plane avg, Bps/1000 */
	const u32 ib_kbps = 1600000;	/* peak 1.6 GB/s = min_dram_ib */
	static const struct { const char *n; u32 bw; u32 ch; } path[] = {
		{ "MM1", 32, 1 },	/* qxm_mdp0       */
		{ "MM0", 32, 2 },	/* qns_mem_noc_hf */
		{ "SH0", 16, 2 },	/* qns_llcc       */
		{ "MC0",  4, 2 },	/* ebi            */
	};
	struct udevice *rsc;
	struct rsc_drv *drv;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(qcom_rpmh_rsc), &rsc);
	if (ret) {
		log_warning("DP: no apps_rsc for MDP0 bw vote: %d\n", ret);
		return;
	}
	drv = dev_get_priv(rsc);

	for (i = 0; i < ARRAY_SIZE(path); i++) {
		u32 a  = cmd_db_read_addr(path[i].n);
		u32 vx = tachyon_bcm_vote_x(path[i].n, path[i].bw, path[i].ch,
					    ab_kbps);
		u32 vy = tachyon_bcm_vote_y(path[i].n, path[i].bw, ib_kbps);

		ret = tachyon_bcm_send(drv, a, BCM_TCS_CMD(1, 1, vx, vy));
		log_debug("DP: %s bw vote addr=%#x vx=%u vy=%u ret=%d\n",
			    path[i].n, a, vx, vy, ret);
	}

	/* ACV mask BCM (enable_mask=BIT(3)): mark the DDR channel active. */
	tachyon_bcm_send(drv, cmd_db_read_addr("ACV"), BCM_TCS_CMD(1, 1, 0, BIT(3)));

	/* Raise the CX voltage corner the 300MHz datapath needs. */
	tachyon_dp_vote_cx_corner(drv);
}

/*
 * Fix the MDP stream's SMR mask.  The DT spec is iommus=<&apps_smmu 0x900
 * 0x402>: SID 0x900, MASK 0x402 -- so the stream must match the MDP's masked
 * sub-SIDs (0x900/0x902/0xd00/0xd02, bits 1 and 10 wildcarded).  But the live
 * SMR[3] reads 0x80000900 (VALID|ID, MASK=0) -- it matches ONLY exactly
 * 0x900.  If the DMA0 SSPP fetch emits a masked variant it is an UNMATCHED
 * stream -> SMMU fault -> (on this platform, with secure fault handling) a
 * reset.  Now that we know the TBU is powered (PWR_STATUS!=0) and the SMMU
 * region is U-Boot-writable (CB3 writes stuck) + SVC_IO-serviced, widen the
 * SMR to the DT mask.  Write both NS and via SVC_IO and read back to see
 * which sticks.  Target SMR value = VALID|MASK(0x402<<16)|ID = 0x84020900.
 */
static void tachyon_dp_fix_mdp_smr(void)
{
	void __iomem *smmu = map_sysmem(0x15000000, 0x100000);
	int n;

	if (!smmu)
		return;

	for (n = 0; n < 128; n++) {
		u32 smr = readl(smmu + 0x800 + 4 * n);

		if (!(smr & BIT(31)) || (smr & 0xffff) != 0x0900)
			continue;
		{
			phys_addr_t pa = 0x15000000UL + 0x800 + 4 * n;
			u32 want = BIT(31) | (0x402u << 16) | 0x0900; /* 0x84020900 */
			u32 sv = 0;

			writel(want, smmu + 0x800 + 4 * n);	/* NS write   */
			qcom_scm_io_writel(pa, want);		/* secure write */
			qcom_scm_io_readl(pa, &sv);
			log_debug("MDP SMR[%d] mask-fix: was=%08x want=%08x ns_rb=%08x scm_rb=%08x\n",
				    n, smr, want, readl(smmu + 0x800 + 4 * n), sv);
		}
		break;
	}

	unmap_sysmem(smmu);
}

int tachyon_dpu_program_scanout(struct tachyon_dp_priv *priv,
				struct video_uc_plat *plat,
				struct video_priv *uc_priv)
{
	int ret;

	if (!priv->dpu)
		return -ENODEV;

	ret = tachyon_dpu_enable_clocks(priv);
	if (ret)
		return ret;

	setbits_le32(priv->dpu + DPU_TOP_BASE + DPU_CLK_CTRL,
		     DPU_CLK_CTRL_DMA0);

	/*
	 * Make the DMA0 SSPP AXI read client (xin_id=1) fetch-ready at the VBIF.
	 * THIS is why the SSPP fetched zero (solid INTF underflow) despite correct
	 * config + FB + clock + voltage + BCM bandwidth: at cold handoff the VBIF
	 * OUT_AXI_AMEMTYPE for the MDP read client is 0, so the SMMU/NoC drops
	 * every MDP read.  XBL sets AMEMTYPE=0x33333333/0x00333333 "to work with
	 * the new SMMU" (HALMDSS hal_mdp_vbif.c) and Linux re-runs
	 * dpu_vbif_init_memtypes() every runtime resume; U-Boot did neither (its
	 * old 0xd00/0xd20 writes were the wrong registers entirely).  Also un-halt
	 * the xin and clear stale AXI error latches in case XBL teardown left them.
	 */
	if (priv->vbif) {
		u32 pnd = readl(priv->vbif + VBIF_XIN_PND_ERR);
		u32 src = readl(priv->vbif + VBIF_XIN_SRC_ERR);
		u32 halt1_before = readl(priv->vbif + VBIF_XIN_HALT_CTRL1);

		/* Un-halt DMA0 (BIT(1) of HALT_CTRL0). */
		clrbits_le32(priv->vbif + VBIF_XIN_HALT_CTRL0,
			     BIT(VBIF_DMA0_XIN_ID));
		/* Clear any stale pending/source AXI error latches. */
		if (pnd | src)
			writel(pnd | src, priv->vbif + VBIF_XIN_CLR_ERR);
		/* AMEMTYPE = 3 (cacheable normal) for every xin, like XBL/Linux. */
		writel(0x33333333, priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF0);
		writel(0x00333333, priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF1);

		log_debug("VBIF xin1 init: HALT1_before=%08x PND=%08x SRC=%08x AMEM0=%08x\n",
			    halt1_before, pnd, src,
			    readl(priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF0));
	}

	/*
	 * Grant the MDP0->DDR read bandwidth (RPMh BCM vote) BEFORE the SSPP
	 * fetch is set up.  Without this the AXI read is starved and the pipe
	 * underruns (solid INTF underflow colour) despite correct config.
	 */
	tachyon_dp_grant_mdp0_bandwidth();

	tachyon_dpu_program_sspp(priv, plat, uc_priv);
	tachyon_dpu_program_lm(priv, uc_priv);
	tachyon_dpu_program_intf(priv, uc_priv);

	/*
	 * Widen the MDP stream's apps_smmu SMR to the DT mask (0x402) so the
	 * DMA0 fetch's masked sub-SIDs match SMR[3]->S2CR[3]->CB3 instead of
	 * faulting as an unmatched stream.  This is the step that lets the MDP
	 * AXI read reach DRAM (the M=0 passthrough CB is sufficient).
	 */
	tachyon_dp_fix_mdp_smr();

	return tachyon_dpu_program_ctl(priv);
}

void tachyon_dpu_quiesce(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl;
	void __iomem *intf;
	void __iomem *sspp;
	int ret;

	if (!priv->dpu || !priv->dpu_clocks_enabled)
		return;

	ctl = priv->dpu + DPU_CTL_0_BASE;
	intf = priv->dpu + DPU_INTF_0_BASE;
	sspp = priv->dpu + DPU_SSPP_DMA0_BASE;

	log_debug("DPU quiesce before OS handoff\n");

	writel(0, intf + DPU_INTF_TIMING_ENGINE_EN);
	writel(0, intf + DPU_INTF_FRAME_LINE_COUNT_EN);
	if (priv->p0)
		writel(0, priv->p0 + MMSS_DP_TIMING_ENGINE_EN);

	writel(0, ctl + DPU_CTL_FETCH_PIPE_ACTIVE);
	writel(0, ctl + DPU_CTL_INTF_ACTIVE);
	writel(0, ctl + DPU_CTL_LAYER_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT2_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT3_0);
	writel(DPU_CTL_FLUSH_DMA0 | DPU_CTL_FLUSH_LM0 | DPU_CTL_FLUSH_CTL |
	       DPU_CTL_FLUSH_INTF | DPU_CTL_FLUSH_PERIPH,
	       ctl + DPU_CTL_FLUSH);
	writel(1, ctl + DPU_CTL_START);

	ret = tachyon_dp_read_poll(ctl, DPU_CTL_FLUSH, DPU_CTL_FLUSH_DMA0, 0,
				   5000);
	if (ret)
		log_warning("DPU quiesce flush did not commit: %d\n", ret);

	writel(0, sspp + DPU_SSPP_SRC0_ADDR);
	writel(0, sspp + DPU_SSPP_SRC1_ADDR);
	writel(0, sspp + DPU_SSPP_SRC2_ADDR);
	writel(0, sspp + DPU_SSPP_SRC3_ADDR);
	writel(0, sspp + DPU_SSPP_SRC_OP_MODE);
	writel(0, sspp + DPU_SSPP_CLK_CTRL);
	clrbits_le32(priv->dpu + DPU_TOP_BASE + DPU_CLK_CTRL,
		     DPU_CLK_CTRL_DMA0);
}
