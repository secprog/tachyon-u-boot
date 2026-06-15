// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal Qualcomm MSM DisplayPort bring-up for Particle Tachyon.
 *
 * This is intentionally board-scoped: it consumes the Tachyon USB-C/DP graph
 * and the SC7280 DP register layout used by QCM6490, then exposes the selected
 * framebuffer through U-Boot's video uclass so EFI GOP uses the same buffer.
 */

#define LOG_CATEGORY UCLASS_VIDEO

#include <asm/gpio.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device.h>
#include <dm/read.h>
#include <dm/ofnode.h>
#include <cpu_func.h>
#include <env.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <lmb.h>
#include <log.h>
#include <mapmem.h>
#include <soc/qcom/pmic_glink.h>
#include <video.h>

#include "qcom_tachyon_dp.h"

DECLARE_GLOBAL_DATA_PTR;


static const struct tachyon_qmp_offsets tachyon_qmp_sc7280_offsets = {
	.com		= 0x0000,
	.dp_serdes	= QMP_OFF_DP_SERDES,
	.dp_tx0		= QMP_OFF_DP_TX0,
	.dp_tx1		= QMP_OFF_DP_TX1,
	.dp_phy		= QMP_OFF_DP_PHY,
};


static int tachyon_dp_request_core_clocks(struct udevice *dev,
					  struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DP_CORE_CLK_COUNT] = {
		"core_iface", "core_aux", "ctrl_link", "ctrl_link_iface",
	};
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = clk_get_by_name(dev, names[i], &priv->dp_clks[i]);
		if (ret) {
			log_warning("DP clock %s unavailable: %d\n", names[i],
				    ret);
			continue;
		}
		priv->dp_clk_valid[i] = true;
	}

	return 0;
}

static int tachyon_dp_enable_core_clocks(struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DP_CORE_CLK_COUNT] = {
		"core_iface", "core_aux", "ctrl_link", "ctrl_link_iface",
	};
	int i, ret;

	if (priv->dp_core_clocks_enabled)
		return 0;

	for (i = 0; i < TACHYON_DP_CORE_CLK_COUNT; i++) {
		if (!priv->dp_clk_valid[i])
			continue;
		ret = clk_enable(&priv->dp_clks[i]);
		if (ret && ret != -ENOSYS) {
			log_warning("Failed to enable DP core clock %s: %d\n",
				    names[i], ret);
			return ret;
		}
		if (!ret)
			priv->dp_clk_enabled[i] = true;
		log_debug("DP clk %s enable ret=%d\n", names[i], ret);
	}

	priv->dp_core_clocks_enabled = true;

	return 0;
}

static ofnode tachyon_dp_find_endpoint(ofnode node, u32 port_id)
{
	ofnode ports, port, ep;

	ports = ofnode_find_subnode(node, "ports");
	if (!ofnode_valid(ports))
		ports = node;

	ofnode_for_each_subnode(port, ports) {
		u32 reg;
		int ret;

		ret = ofnode_read_u32(port, "reg", &reg);
		if (ret) {
			/*
			 * DT binding allows omitting reg when there is only
			 * one port. Treat an unnumbered port as port 0.
			 */
			reg = 0;
		}

		if (reg != port_id)
			continue;

		ofnode_for_each_subnode(ep, port) {
			if (!strncmp(ofnode_get_name(ep), "endpoint", 8))
				return ep;
		}
	}

	/*
	 * Some device-trees (e.g. gpio-sbu-mux) place the endpoint directly
	 * under the node without a port wrapper.
	 */
	if (port_id == 0) {
		ofnode_for_each_subnode(ep, ports) {
			if (!strncmp(ofnode_get_name(ep), "endpoint", 8))
				return ep;
		}
	}

	return ofnode_null();
}

static void tachyon_dp_parse_graph(struct udevice *dev,
				   struct tachyon_dp_priv *priv)
{
	ofnode dp = dev_ofnode(dev);
	ofnode out_ep, in_ep, remote;
	u32 lane, count = 0, lane_map = 0;

	priv->max_lanes = 4;
	priv->graph_lanes = 4;
	priv->lane_map = 0xe4;

	out_ep = tachyon_dp_find_endpoint(dp, 1);
	if (ofnode_valid(out_ep)) {
		while (count < 4 &&
		       !ofnode_read_u32_index(out_ep, "data-lanes", count,
					      &lane)) {
			lane_map |= (lane & 0x3) << (count * 2);
			count++;
		}
		if (count) {
			priv->max_lanes = count;
			priv->graph_lanes = count;
			priv->lane_map = lane_map;
		}

		remote = ofnode_parse_phandle(out_ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("DP output endpoint has no remote endpoint\n");
	} else {
		log_warning("DP output endpoint missing; using default lane map\n");
	}

	in_ep = tachyon_dp_find_endpoint(dp, 0);
	if (ofnode_valid(in_ep)) {
		remote = ofnode_parse_phandle(in_ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("DP input endpoint has no DPU remote endpoint\n");
	} else {
		log_warning("DP input endpoint missing; DPU path is not described\n");
	}

	log_info("DP graph lane map=0x%x max_lanes=%u\n",
		 priv->lane_map, priv->max_lanes);
}

int tachyon_dp_pin_assignment_lanes(struct tachyon_dp_priv *priv)
{
	switch (priv->pin_assignment) {
	case 2: /* DP pin assignment C */
	case 4: /* DP pin assignment E */
		return 4;
	case 3: /* DP pin assignment D */
	case 5: /* DP pin assignment F */
		return 2;
	default:
		return 0;
	}
}

static bool tachyon_dp_valid_orientation(enum tachyon_dp_orientation orientation)
{
	return orientation == TACHYON_DP_ORIENTATION_NORMAL ||
	       orientation == TACHYON_DP_ORIENTATION_REVERSE;
}

static bool tachyon_dp_valid_pin_assignment(u8 pin)
{
	switch (pin) {
	case 2: /* DP pin assignment C: DP-only, 4 lanes */
	case 3: /* DP pin assignment D: USB3 + DP, 2 lanes */
	case 4: /* DP pin assignment E: DP-only, 4 lanes */
	case 5: /* DP pin assignment F: USB3 + DP, 2 lanes */
		return true;
	default:
		return false;
	}
}

bool tachyon_dp_typec_state_valid(struct tachyon_dp_priv *priv)
{
	return priv->typec_valid &&
	       priv->typec_source == TACHYON_DP_TYPEC_SOURCE_ALTMODE &&
	       tachyon_dp_valid_orientation(priv->orientation) &&
	       tachyon_dp_valid_pin_assignment(priv->pin_assignment);
}

void tachyon_dp_log_typec_resolved(struct tachyon_dp_priv *priv)
{
	u8 pin_lanes = tachyon_dp_pin_assignment_lanes(priv);

	log_debug("DP TYPEC RESOLVED: source=altmode orientation=%u pin=%u pin_lanes=%u graph_lanes=%u lane_map=%02x\n",
		    priv->orientation, priv->pin_assignment, pin_lanes,
		    priv->graph_lanes, priv->lane_map & 0xff);

	if (pin_lanes > priv->graph_lanes)
		log_warning("DP lane mismatch: Type-C pin assignment %u wants %u lanes but graph has %u; clamping to graph\n",
			    priv->pin_assignment, pin_lanes, priv->graph_lanes);
}

u32 tachyon_dp_env_u32(const char *name, u32 fallback)
{
	const char *val = env_get(name);
	char *end;
	ulong parsed;

	if (!val || !*val)
		return fallback;

	parsed = simple_strtoul(val, &end, 0);
	if (end == val)
		return fallback;

	return parsed;
}

bool tachyon_dp_env_has_u32(const char *name)
{
	const char *val = env_get(name);

	return val && *val;
}

bool tachyon_dp_env_bool(const char *name)
{
	const char *val = env_get(name);

	return val && (!strcmp(val, "1") ||
		       !strcmp(val, "true") ||
		       !strcmp(val, "yes"));
}

static void tachyon_dp_program_sbu_mux(struct tachyon_dp_priv *priv);

static int tachyon_dp_read_altmode(struct tachyon_dp_priv *priv)
{
	struct qcom_pmic_glink_altmode glink_altmode;
	int ret;

	ret = qcom_pmic_glink_get_altmode(&glink_altmode);
	log_debug("DP PMIC-GLINK raw altmode: ret=%d dp=%d port=%u orientation=%u pin=%u hpd=%d hpd_irq=%d\n",
		    ret, glink_altmode.dp, glink_altmode.port,
		    glink_altmode.orientation, glink_altmode.pin_assignment,
		    glink_altmode.hpd, glink_altmode.hpd_irq);
	if (ret)
		return ret;

	/*
	 * Accept DP mode even without HPD. HPD can assert later; we rely on
	 * tachyon_dp_wait_sink() to retry DPCD reads until the sink responds.
	 * The previous logic required dp && hpd, which blocked DP at boot.
	 */
	if (!ret && glink_altmode.dp) {
		switch (glink_altmode.orientation) {
		case QCOM_PMIC_GLINK_ORIENTATION_NORMAL:
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;
			break;
		case QCOM_PMIC_GLINK_ORIENTATION_REVERSE:
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
			break;
		default:
			log_warning("DP Alt-Mode invalid PMIC-GLINK orientation=%u\n",
				    glink_altmode.orientation);
			return -EINVAL;
		}

		priv->pin_assignment = glink_altmode.pin_assignment;

		log_info("DP Alt-Mode confirmed via PMIC-GLINK: orientation=%u pin=%u hpd=%d\n",
			 priv->orientation, priv->pin_assignment,
			 glink_altmode.hpd);
		return 1;
	}

	return 0;
}

static bool tachyon_dp_altmode_ready(struct tachyon_dp_priv *priv)
{
	int ret = tachyon_dp_read_altmode(priv);

	if (ret <= 0) {
		priv->typec_valid = false;
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
		return false;
	}

	priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
	priv->typec_valid = true;

	if (!tachyon_dp_typec_state_valid(priv)) {
		log_warning("DP Type-C Alt Mode invalid: orientation=%u pin=%u\n",
			    priv->orientation, priv->pin_assignment);
		priv->typec_valid = false;
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
		return false;
	}

	return true;
}

static int tachyon_dp_refresh_altmode(struct tachyon_dp_priv *priv,
				      bool *changed)
{
	enum tachyon_dp_orientation old_orientation = priv->orientation;
	u8 old_pin = priv->pin_assignment;

	if (!tachyon_dp_altmode_ready(priv))
		return -ENODEV;

	*changed = old_orientation != priv->orientation ||
		   old_pin != priv->pin_assignment;
	if (*changed)
		tachyon_dp_program_sbu_mux(priv);

	return 0;
}

static int tachyon_dp_request_sbu_mux(struct tachyon_dp_priv *priv)
{
	ofnode mux, ep, remote;
	int ret;

	for (mux = ofnode_by_compatible(ofnode_null(), "gpio-sbu-mux");
	     ofnode_valid(mux);
	     mux = ofnode_by_compatible(mux, "gpio-sbu-mux")) {
		if (!ofnode_is_enabled(mux))
			continue;
		if (ofnode_read_bool(mux, "orientation-switch") &&
		    ofnode_read_bool(mux, "mode-switch"))
			break;
	}
	if (!ofnode_valid(mux))
		mux = ofnode_path("/usb1-sbu-mux");
	if (!ofnode_valid(mux))
		return -ENOENT;

	log_debug("SBU mux request start: node=%s\n",
		    ofnode_get_name(mux));

	/* Debug: dump SBU mux node structure */
	{
		ofnode ports = ofnode_find_subnode(mux, "ports");
		log_debug("SBU ports valid=%d name=%s\n",
			    ofnode_valid(ports),
			    ofnode_valid(ports) ? ofnode_get_name(ports) : "<none>");
		if (ofnode_valid(ports)) {
			ofnode port0 = ofnode_find_subnode(ports, "port@0");
			log_debug("SBU port@0 valid=%d\n",
				    ofnode_valid(port0));
		}
	}

	ep = tachyon_dp_find_endpoint(mux, 0);
	if (ofnode_valid(ep)) {
		remote = ofnode_parse_phandle(ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("SBU mux endpoint has no PMIC-GLINK remote endpoint\n");
	} else {
		log_warning("SBU mux has no graph endpoint\n");
	}

	ret = gpio_request_by_name_nodev(mux, "enable-gpios", 0,
					 &priv->sbu_enable, GPIOD_IS_OUT);
	log_debug("SBU enable GPIO request ret=%d\n", ret);
	if (ret)
		return ret;

	ret = gpio_request_by_name_nodev(mux, "select-gpios", 0,
					 &priv->sbu_select, GPIOD_IS_OUT);
	log_debug("SBU select GPIO request ret=%d\n", ret);
	if (ret)
		return ret;

	log_debug("SBU mux request done\n");

	return 0;
}

static void tachyon_dp_program_sbu_mux(struct tachyon_dp_priv *priv)
{
	bool invert_select = tachyon_dp_env_bool("tachyon_dp_invert_sbu_select");
	bool invert_enable = tachyon_dp_env_bool("tachyon_dp_invert_sbu_enable");
	int select;
	int enable;

	log_debug("SBU mux program start orientation=%u pin=%u invert_select=%d invert_enable=%d\n",
		    priv->orientation, priv->pin_assignment,
		    invert_select ? 1 : 0,
		    invert_enable ? 1 : 0);

	select = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE;
	if (invert_select)
		select = !select;

	enable = 1;
	if (invert_enable)
		enable = 0;

	if (dm_gpio_is_valid(&priv->sbu_select))
		dm_gpio_set_value(&priv->sbu_select, select);

	udelay(1000);

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, enable);

	log_debug("SBU mux program done: enable=%d select=%d\n",
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1);
}

/*
 * Release SBU mux GPIOs so repeated probe attempts succeed.
 * Without this, a failed probe leaves GPIOs held and the next
 * probe gets -EBUSY (-16).
 */
static void tachyon_dp_release_sbu_mux(struct tachyon_dp_priv *priv)
{
	if (dm_gpio_is_valid(&priv->sbu_enable)) {
		dm_gpio_free(NULL, &priv->sbu_enable);
		memset(&priv->sbu_enable, 0, sizeof(priv->sbu_enable));
	}

	if (dm_gpio_is_valid(&priv->sbu_select)) {
		dm_gpio_free(NULL, &priv->sbu_select);
		memset(&priv->sbu_select, 0, sizeof(priv->sbu_select));
	}
}

/*
 * Program QMP combo PHY Type-C select and DP mode before AUX init.
 * On the SC7280/QCM6490, the QMP USB3-DP combo PHY must be told:
 *   - DP mode (not USB3)
 *   - Type-C orientation (which lanes map to which AUX/SBU pins)
 * If this is not done, AUX transactions may never reach the sink.
 *
 * IMPORTANT: These QMP registers use byte-style access (only the low byte
 * is meaningful).  Using setbits/clrbits/clrsetbits_le32() read-modify-write
 * on them produces byte-replicated garbage (0x02020202 etc.) because the
 * read path returns replicated bytes.  Use direct writel() with the
 * fully-computed value instead.
 */

static int tachyon_dp_find_phy(struct udevice *dev, struct tachyon_dp_priv *priv)
{
	const struct tachyon_qmp_offsets *offs = &tachyon_qmp_sc7280_offsets;
	struct ofnode_phandle_args args;
	fdt_addr_t addr;
	fdt_size_t size;
	int ret;

	ret = dev_read_phandle_with_args(dev, "phys", "#phy-cells", 0, 0,
					 &args);
	if (ret)
		return ret;

	addr = ofnode_get_addr_size(args.node, "reg", &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->phy = map_sysmem(addr, size);
	priv->qmp_com = (void __iomem *)((u8 __iomem *)priv->phy + offs->com);
	priv->qmp_dp_serdes = (void __iomem *)((u8 __iomem *)priv->phy +
					       offs->dp_serdes);
	priv->qmp_dp_tx0 = (void __iomem *)((u8 __iomem *)priv->phy +
					    offs->dp_tx0);
	priv->qmp_dp_tx1 = (void __iomem *)((u8 __iomem *)priv->phy +
					    offs->dp_tx1);
	priv->phy_dp = (void __iomem *)((u8 __iomem *)priv->phy +
					offs->dp_phy);

	log_debug("QMP offsets: base=%p com=%p dp_serdes=%p tx0=%p tx1=%p dp_phy=%p\n",
		    priv->phy, priv->qmp_com, priv->qmp_dp_serdes,
		    priv->qmp_dp_tx0, priv->qmp_dp_tx1, priv->phy_dp);

	/*
	 * Let the QMP combo PHY provider perform the common Linux-style
	 * clock/reset/regulator/COM bring-up before this board driver directly
	 * programs DP AUX/link registers.
	 */
	ret = generic_phy_get_by_index(dev, 0, &priv->qmp_phy);
	if (ret) {
		log_warning("QMP generic PHY get failed: %d\n", ret);
	} else {
		priv->has_qmp_phy = true;

		ret = generic_phy_init(&priv->qmp_phy);
		log_debug("QMP generic PHY init ret=%d id=%lu\n",
			    ret, priv->qmp_phy.id);
		if (ret)
			return ret;
	}

	return 0;
}

static int tachyon_dp_apply_pmic_typec_state(struct tachyon_dp_priv *priv,
		                 const struct qcom_pmic_glink_altmode_state *state)
{
	bool force_aux = tachyon_dp_env_bool("tachyon_dp_force_aux_without_hpd");

	if (!state || !state->notify_seen)
		return -EAGAIN;

	log_info("DP PMIC state: typec=%u hpd=%u hpd_irq=%u orientation=%u pin=%u mux=%u dpam=%02x\n",
		state->typec_state, state->hpd, state->hpd_irq,
		state->orientation, state->pin_assignment,
		state->mux, state->dpam_raw);

	switch (state->typec_state) {
	case QPG_TYPEC_STATE_SAFE:
		priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;
		priv->aux_xfers_enabled = false;
		break;
	case QPG_TYPEC_STATE_DP:
		if (state->orientation == QCOM_PMIC_GLINK_ORIENTATION_REVERSE)
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
		else
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;

		if (tachyon_dp_valid_pin_assignment(state->pin_assignment))
			priv->pin_assignment = state->pin_assignment;

		tachyon_dp_qmp_com_orientation_update(priv);
		tachyon_dp_program_sbu_mux(priv);
		tachyon_dp_qmp_force_aux_on(priv);

		if (state->hpd)
			priv->hpd_state = TACHYON_DP_HPD_CONNECTED;
		else if (priv->hpd_state == TACHYON_DP_HPD_UNKNOWN)
			priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;

		priv->aux_xfers_enabled = state->hpd || force_aux;
		break;
	case QPG_TYPEC_STATE_USB:
		priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;
		priv->aux_xfers_enabled = false;
		break;
	default:
		priv->aux_xfers_enabled = force_aux;
		break;
	}

	return 0;
}

/*
 * Block until the dock asserts DisplayPort HPD (best-effort, bounded).
 *
 * The sink answers AUX (DPCD/EDID) as soon as the SBU mux + orientation are
 * programmed, but its main-link receiver only comes up when it raises HPD.
 * Clock recovery started while HPD is still low fails with lane status 00 at
 * every rate -- the source transmits but the sink receiver is off.  This made
 * boot DP intermittent: success depended purely on whether the entry/auto-DFP
 * poll happened to outlast HPD (HW-confirmed: identical normal-orientation
 * boots trained when hpd=1 arrived first, failed CR when training raced ahead
 * of it).  The PMIC/ADSP altmode HPD is the dependable signal -- the hpd=1 that
 * immediately preceded every successful train; the DP controller's own
 * HPD_INT_STATUS reads "connected" too early to trust.  Keep the altmode state
 * machine pumping so the notify is processed, and return as soon as HPD is
 * seen; on timeout the caller trains anyway so a dock that never surfaces HPD
 * still gets a chance.
 */
bool tachyon_dp_wait_pmic_hpd(struct tachyon_dp_priv *priv,
			      uint timeout_ms)
{
	struct qcom_pmic_glink_altmode_state state;
	ulong start = get_timer(0);
	int ret;

	do {
		ret = qcom_pmic_glink_altmode_poll(&state, 100);
		if (!ret) {
			tachyon_dp_apply_pmic_typec_state(priv, &state);
			if (state.hpd)
				return true;
		}
	} while (get_timer(start) < timeout_ms);

	return false;
}

/* Fill the framebuffer with vertical colour bars (XRGB8888). */
static void tachyon_dp_fill_test_pattern(struct video_uc_plat *plat,
					 struct video_priv *uc_priv)
{
	static const u32 bars[8] = {
		0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00,
		0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00303030,
	};
	u32 *fb = (u32 *)plat->base;
	u32 w = uc_priv->xsize, h = uc_priv->ysize;
	u32 stride = uc_priv->line_length / 4;
	u32 x, y;

	if (!fb || !w || !h)
		return;

	for (y = 0; y < h; y++) {
		u32 *line = fb + (u64)y * stride;

		for (x = 0; x < w; x++)
			line[x] = bars[(x * 8) / w];
	}
	flush_dcache_range((ulong)plat->base,
			   (ulong)plat->base + (ulong)stride * 4 * h);
}

/*
 * Dump the DP controller MSA/video registers and the DPU INTF timing-engine
 * state so we can tell, when the sink shows "No Signal" despite a trained link,
 * whether the DPU is actually clocking real framebuffer pixels into the DP
 * interface (frame/line counters advancing) and whether the MSA timing the DP
 * controller is sending matches the selected mode.
 */
static void tachyon_dp_dump_video_state(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	void __iomem *intf;
	u32 frame0 = 0, line0 = 0, frame1 = 0, line1 = 0;

	log_debug("DP MSA regs: TOTAL=%08x ACTIVE=%08x SYNC_START=%08x WIDTH_POL=%08x MISC0=%08x MVID=%08x NVID=%08x\n",
		    readl(priv->link + REG_DP_TOTAL_HOR_VER),
		    readl(priv->link + REG_DP_ACTIVE_HOR_VER),
		    readl(priv->link + REG_DP_START_HOR_VER_FROM_SYNC),
		    readl(priv->link + REG_DP_HSYNC_VSYNC_WIDTH_POLARITY),
		    readl(priv->link + REG_DP_MISC1_MISC0),
		    readl(priv->link + REG_DP_SOFTWARE_MVID),
		    readl(priv->link + REG_DP_SOFTWARE_NVID));
	log_debug("DP mode expect: %ux%u htotal=%u vtotal=%u pclk=%u STATE_CTRL=%08x READY=%08x\n",
		    t->hactive.typ, t->vactive.typ, tachyon_dp_htotal(t),
		    tachyon_dp_vtotal(t), t->pixelclock.typ,
		    readl(priv->link + REG_DP_STATE_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_READY));

	if (priv->dpu) {
		intf = priv->dpu + DPU_INTF_0_BASE;
		frame0 = readl(intf + DPU_INTF_FRAME_COUNT);
		line0 = readl(intf + DPU_INTF_LINE_COUNT);
		mdelay(50);
		frame1 = readl(intf + DPU_INTF_FRAME_COUNT);
		line1 = readl(intf + DPU_INTF_LINE_COUNT);
		log_debug("DPU INTF: TE_EN=%08x frame %u->%u line %u->%u (advancing=%d)\n",
			    readl(intf + DPU_INTF_TIMING_ENGINE_EN),
			    frame0, frame1, line0, line1,
			    (frame1 != frame0) || (line1 != line0));
	}

	tachyon_dp_dump_dpu_state(priv);

	/*
	 * Re-assert D0 and read the sink/branch link status AFTER video has been
	 * sent.  If the branch dropped symbol lock once real video started (TU /
	 * MVID/NVID mismatch) the lane-status bytes here will show it; if the
	 * branch sees no downstream HDMI display, SINK_COUNT will be 0.
	 */
	{
		u8 l01 = 0, l23 = 0, align = 0;

		tachyon_dp_sink_power_on(priv);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE0_1_STATUS,
				     &l01, 1);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE2_3_STATUS,
				     &l23, 1);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE_ALIGN_STATUS,
				     &align, 1);
		log_debug("DP post-video link status: LANE0_1=0x%02x LANE2_3=0x%02x ALIGN=0x%02x\n",
			    l01, l23, align);
	}
}

static int tachyon_dp_program_mainlink(struct tachyon_dp_priv *priv)
{
	tachyon_dp_log_lanes(priv, "mainlink-entry (post-train)");

	writel(DP_SW_RESET, priv->ctrl + REG_DP_SW_RESET);
	udelay(1000);
	writel(0, priv->ctrl + REG_DP_SW_RESET);
	tachyon_dp_log_lanes(priv, "after SW_RESET");

	tachyon_dp_program_video_timing(priv);
	tachyon_dp_configure_source_link(priv);
	/*
	 * Do NOT pulse DP_MAINLINK_CTRL_RESET here: the link is already trained
	 * at this point (we run after link training, not before like msm),
	 * and asserting the mainlink reset — especially holding it for 1ms —
	 * resets the mainlink and makes the sink lose clock-recovery (lane status
	 * collapses 0x77 -> 0x00 right here, which was the "No Signal" cause).
	 * Just (re)enable the mainlink so the freshly programmed MSA is latched.
	 */
	writel(DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL |
	       DP_MAINLINK_CTRL_FLUSH_MODE,
	       priv->link + REG_DP_MAINLINK_CTRL);
	tachyon_dp_log_lanes(priv, "after MAINLINK enable");


	/*
	 * Start the video stream with this sequence of DP_STATE_CTRL commands to avoid a "No Signal" sink state:
	 *   1. PUSH_IDLE  — emit idle patterns first to avoid an underflow when
	 *      the stream switches on (STATE_CTRL must be cleared before each
	 *      command), then wait for IDLE_PATTERNS_SENT.
	 *   2. SEND_VIDEO — switch the mainlink to the active video stream.
	 *   3. Enable MDP->DP backpressure (OVERRIDE_ACK=0) so the DP TX actually
	 *      pulls pixel data from the MDP.  Without this the controller reports
	 *      READY_FOR_VIDEO but stays in SEND_IDLE_PATTERN (MAINLINK_READY had
	 *      bit10 set) and the sink shows "No Signal".
	 */
	writel(0, priv->link + REG_DP_STATE_CTRL);
	writel(DP_STATE_CTRL_PUSH_IDLE, priv->link + REG_DP_STATE_CTRL);
	tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
			     DP_MAINLINK_READY_IDLE_PATTERNS_SENT,
			     DP_MAINLINK_READY_IDLE_PATTERNS_SENT, 2000);

	writel(0, priv->link + REG_DP_STATE_CTRL);
	writel(DP_STATE_CTRL_SEND_VIDEO, priv->link + REG_DP_STATE_CTRL);

	/* Enable MDP->DP backpressure so pixel data flows (clears OVERRIDE_ACK). */
	if (priv->p0) {
		u32 dto = readl(priv->p0 + MMSS_DP_P0CLK_DSC_DTO);

		dto &= ~(DP_P0CLK_DSC_DTO_OVERRIDE_ACK |
			 DP_P0CLK_DSC_DTO_OVERRIDE_ACK_VALUE);
		writel(dto, priv->p0 + MMSS_DP_P0CLK_DSC_DTO);
		log_debug("DP backpressure enabled: DSC_DTO=%08x\n",
			    readl(priv->p0 + MMSS_DP_P0CLK_DSC_DTO));
	}

	/*
	 * Start the pixel source LAST.  Only now that the DP controller is armed
	 * (SEND_VIDEO + backpressure) do we enable the DPU INTF timing engine, so
	 * the DP TX latches a clean blanking->active transition from a
	 * freshly-started timing engine — what the DP->HDMI bridge needs to lock.
	 * (Previously the INTF engine was turned on back in program_intf/scanout,
	 * BEFORE SEND_VIDEO, so pixels free-ran into an un-armed/SW-reset DP
	 * controller -> sink "No Signal" despite a trained, ready link.)  This
	 * mirrors Linux dpu_encoder_phys_vid handle_post_kickoff (INTF enabled
	 * after the DP stream-on + CTL flush).  The pending CTL flush from
	 * tachyon_dpu_program_ctl (issued with the engine off) is consumed at this
	 * first vsync.
	 */
	if (priv->dpu && !tachyon_dp_env_bool("tachyon_dp_tpg")) {
		writel(1, priv->dpu + DPU_INTF_0_BASE +
			  DPU_INTF_TIMING_ENGINE_EN);
		log_debug("DP INTF timing engine ON (after SEND_VIDEO)\n");
	} else if (priv->dpu) {
		/*
		 * TPG mode: leave the DPU INTF timing engine OFF so the DP
		 * controller's internal p0 BIST is the sole pixel/timing source
		 * (no DPU vs p0 timing-engine conflict).  TPG shares the same DP
		 * main link + MSA + TU as normal video, so this isolates the
		 * DPU pixel path from the DP stream.
		 */
		log_debug("DP TPG mode: DPU INTF timing engine left OFF\n");
	}

	{
		int rdy = tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
					       DP_MAINLINK_READY_FOR_VIDEO,
					       DP_MAINLINK_READY_FOR_VIDEO, 5000);
		log_debug("DP mainlink ready-for-video ret=%d MAINLINK_READY=%08x MAINLINK_CTRL=%08x\n",
			    rdy, readl(priv->link + REG_DP_MAINLINK_READY),
			    readl(priv->link + REG_DP_MAINLINK_CTRL));
		tachyon_dp_dump_video_state(priv);
		return rdy;
	}
}

static void tachyon_dp_disable_clocks(struct tachyon_dp_priv *priv)
{
	int i, ret;

	if (priv->has_pixel_clk && priv->pixel_clk_enabled) {
		ret = clk_disable(&priv->pixel_clk);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DP pixel clock: %d\n",
				    ret);
		else
			priv->pixel_clk_enabled = false;
	}

	for (i = 0; i < TACHYON_DP_CORE_CLK_COUNT; i++) {
		if (!priv->dp_clk_enabled[i])
			continue;
		ret = clk_disable(&priv->dp_clks[i]);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DP clock %d: %d\n",
				    i, ret);
		else
			priv->dp_clk_enabled[i] = false;
	}
	priv->dp_core_clocks_enabled = false;

	for (i = 0; i < TACHYON_DPU_CLK_COUNT; i++) {
		if (!priv->dpu_clk_enabled[i])
			continue;
		ret = clk_disable(&priv->dpu_clks[i]);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DPU clock %d: %d\n",
				    i, ret);
		else
			priv->dpu_clk_enabled[i] = false;
	}
	priv->dpu_clocks_enabled = false;
}

static void tachyon_dp_quiesce(struct tachyon_dp_priv *priv)
{
	int ret;

	tachyon_dpu_quiesce(priv);
	log_debug("DP quiesce: step dpu done\n");
	tachyon_dp_controller_quiesce(priv);
	log_debug("DP quiesce: step controller done\n");

	if (priv->phy_dp && (priv->qmp_dp_touched ||
			     priv->qmp_dp_serdes_programmed ||
			     priv->qmp_dp_phy_started)) {
		tachyon_dp_qmp_power_down(priv);
		log_debug("DP quiesce: step qmp_power_down done\n");
	}

	if (priv->has_qmp_phy) {
		ret = generic_phy_power_off(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY power_off failed: %d\n", ret);
		log_debug("DP quiesce: step phy_power_off done\n");
		ret = generic_phy_exit(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY exit failed: %d\n", ret);
		log_debug("DP quiesce: step phy_exit done\n");
		priv->has_qmp_phy = false;
	}

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, 0);
	tachyon_dp_release_sbu_mux(priv);

	tachyon_dp_disable_clocks(priv);
}

/*
 * No .remove handler: DP is intentionally NOT torn down at ExitBootServices.
 * The link is left trained and the framebuffer scanning out so Windows (which
 * has no native Qualcomm DP/DPU driver) keeps a live GOP display across the
 * handoff.  This intentionally breaks the Linux clean handoff (msm_dpu fails
 * modeset "Cannot find any crtc or sizes", dock black on Linux boot) -- accepted.
 * The err_quiesce path in the bringup code still quiesces on a failed train.
 */

static int tachyon_dp_wait_sink(struct tachyon_dp_priv *priv)
{
	int ret, i;
	struct qcom_pmic_glink_altmode_state altmode_state;
	int altmode_ret;

	log_debug("DP wait sink: %d tries, %d us interval\n",
		    TACHYON_DP_AUX_DEBOUNCE_TRIES, 20000);

	for (i = 0; i < TACHYON_DP_AUX_DEBOUNCE_TRIES; i++) {
		/*
		 * Keep the PMIC altmode state machine polling while we wait for
		 * HPD and the sink's DPCD registers to become available.
		 */
		altmode_ret = qcom_pmic_glink_altmode_poll(&altmode_state, 100);
		if (!altmode_ret)
			tachyon_dp_apply_pmic_typec_state(priv, &altmode_state);
		else if (altmode_ret != -ENOSYS && altmode_ret != -ETIMEDOUT)
			log_debug("DP PMIC altmode poll failed: %d\n", altmode_ret);

		if (tachyon_dp_hw_hpd_connected(priv)) {
			priv->hpd_state = TACHYON_DP_HPD_CONNECTED;
			priv->aux_xfers_enabled = true;
		}

		if (priv->phy_dp && !tachyon_dp_qmp_phy_ready(priv)) {
			u8 qmp_status = tachyon_dp_qmp_status_low(priv);

			log_debug("DP wait sink try %d/%d: QMP PHY not ready (%02x); waiting for PHY_READY before DPCD\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES, qmp_status);
			udelay(20000);
			continue;
		}

		if (priv->hpd_state == TACHYON_DP_HPD_DISCONNECTED &&
		    !tachyon_dp_env_bool("tachyon_dp_force_aux_without_hpd")) {
			log_debug("DP wait sink try %d/%d: no HPD yet, skipping DPCD read\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES);
			udelay(20000);
			continue;
		}

		/*
		 * Hard-reset the AUX controller before every DPCD retry.
		 * The GO bit was observed stuck at 0x200 after timeouts;
		 * a full AUX reset ensures a clean transaction state each
		 * attempt.
		 */
		tachyon_dp_aux_hw_init(priv);

		/*
		 * Try a 1-byte native DPCD_REV read first.  If a 1-byte
		 * read succeeds but 16-byte fails, the AUX length handling
		 * is wrong.  If 1-byte also times out, the problem is still
		 * physical AUX/QMP routing.
		 */
		{
			u8 dpcd_rev;

			ret = tachyon_dp_aux_retry(priv, false, true,
						   DP_DPCD_REV, &dpcd_rev, 1);
			log_debug("DPCD_REV 1-byte read ret=%d val=%02x\n",
				    ret, dpcd_rev);
		}

		if (!ret) {
			/* 1-byte succeeded — now do the full 16-byte caps */
			ret = tachyon_dp_read_dpcd_caps(priv);
		}

		if (!ret) {
			log_info("DP sink DPCD read OK on try %d/%d\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES);
			return 0;
		}

		log_debug("DP sink DPCD try %d/%d failed ret=%d\n",
			    i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES, ret);

		/*
		 * Docks often expose AUX a little after orientation/mode-switch
		 * callbacks. Retry long enough to absorb plug and cable-flip
		 * bounce before giving up.
		 */
		udelay(20000);
	}

	log_warning("DP sink DPCD read failed after AUX debounce: %d\n", ret);
	return ret;
}

/*
 * Reset QMP Type-C select, reprogram SBU mux GPIOs, reinitialize QMP AUX
 * settings, force AUX out of powerdown/clamp, and reset the DP AUX
 * controller — all for a single orientation choice.
 */
static void tachyon_dp_prepare_aux_for_orientation(
		struct tachyon_dp_priv *priv,
		enum tachyon_dp_orientation orientation)
{
	u32 pd_low;

	priv->orientation = orientation;

	log_debug("DP prepare AUX orientation=%u pin=%u\n",
		    priv->orientation, priv->pin_assignment);

	/* Provider owns COM reset; the DP driver only updates orientation/mode. */
	tachyon_dp_qmp_com_orientation_update(priv);

	/* Reprogram the external SBU mux */
	tachyon_dp_program_sbu_mux(priv);

	/* Keep the AUX path powered, then reinitialize QMP AUX CFG. */
	tachyon_dp_qmp_force_aux_on(priv);
	tachyon_dp_qmp_aux_init(priv);

	/* Reset the DP AUX controller */
	tachyon_dp_aux_hw_init(priv);
	writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
	udelay(100);

	pd_low = readl(priv->phy_dp + QMP_DP_PHY_PD_CTL) & 0xff;

	log_debug("DP AUX orientation state: TYPEC=%02x MODE=%02x PD=%02x STATUS=%02x SBU_EN=%d SBU_SEL=%d AUX_CTRL=%08x AUX_STATUS=%08x AUX_TRANS=%08x\n",
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    pd_low,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_STATUS) & 0xff,
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1,
		    readl(priv->aux + REG_DP_AUX_CTRL),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL));
}

static int tachyon_dp_probe(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	int ret, altmode_ret;
	bool has_sbu_mux;
	bool forced_typec = false;

	log_debug("DP probe start\n");

	priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
	priv->typec_valid = false;
	priv->pin_assignment = 0;

	priv->ctrl = dev_remap_addr_index(dev, 0);
	priv->aux = dev_remap_addr_index(dev, 1);
	priv->link = dev_remap_addr_index(dev, 2);
	priv->p0 = dev_remap_addr_name(dev, "p0");
	if (!priv->p0)
		priv->p0 = dev_remap_addr_index(dev, 3);
	if (!priv->ctrl || !priv->aux || !priv->link) {
		log_warning("DP register remap failed ctrl=%p aux=%p link=%p\n",
			    priv->ctrl, priv->aux, priv->link);
		return -EINVAL;
	}

	tachyon_dp_parse_graph(dev, priv);
	tachyon_dp_request_core_clocks(dev, priv);
	ret = tachyon_dp_enable_core_clocks(priv);
	if (ret)
		goto err_quiesce;

	ret = clk_get_by_name(dev, "stream_pixel", &priv->pixel_clk);
	if (!ret) {
		priv->has_pixel_clk = true;
	} else {
		ret = clk_get_by_name(dev, "pixel", &priv->pixel_clk);
		priv->has_pixel_clk = !ret;
	}

	ret = tachyon_dpu_init(priv);
	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_find_phy(dev, priv);
	if (ret)
		goto err_quiesce;

	/* Print QMP base addresses for offset verification */
	log_debug("DP QMP base: phy=%p phy_dp=%p\n",
		    priv->phy, priv->phy_dp);
	log_debug("DP QMP offsets: TYPEC_CTRL=%x PHY_MODE_CTRL=%x DP_PD_CTL=%x DP_STATUS=%x\n",
		    (u32)QMP_V3_DP_COM_TYPEC_CTRL,
		    (u32)QMP_V3_DP_COM_PHY_MODE_CTRL,
		    (u32)(QMP_OFF_DP_PHY + QMP_DP_PHY_PD_CTL),
		    (u32)(QMP_OFF_DP_PHY + QMP_V4_DP_PHY_STATUS));

	/* Print DP core clock validity before any AUX/PHY work */
	{
		int ci;

		for (ci = 0; ci < TACHYON_DP_CORE_CLK_COUNT; ci++)
			log_debug("DP clk[%d] valid=%d\n",
				    ci, priv->dp_clk_valid[ci]);
	}

	/*
	 * Start the PMIC-GLINK altmode service early, but do not require the
	 * first notification to be DP-active. Qualcomm policy firmware can
	 * legitimately report SAFE before later reporting DP.
	 */
	ret = qcom_pmic_glink_altmode_start();
	if (ret) {
		log_warning("DP PMIC-GLINK altmode service start failed: %d\n",
			    ret);
		goto err_quiesce;
	}

	qcom_pmic_glink_altmode_poll(NULL, 100);
	altmode_ret = tachyon_dp_read_altmode(priv);
	if (altmode_ret > 0) {
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
		priv->typec_valid = true;
		if (!tachyon_dp_typec_state_valid(priv)) {
			log_warning("DP Type-C Alt Mode invalid: orientation=%u pin=%u\n",
				    priv->orientation, priv->pin_assignment);
			priv->typec_valid = false;
			priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
			ret = -EINVAL;
			goto err_quiesce;
		}
		tachyon_dp_log_typec_resolved(priv);
	} else {
		log_debug("DP Type-C Alt Mode not active yet: ret=%d; continuing DP init while PMIC service polls\n",
			    altmode_ret);

		/*
		 * The dock has not entered DP yet.  At boot it is a cold Type-C
		 * power-sink (UCSI opmode=5, no PD contract, partner=0) and only
		 * self-completes its PD/DP negotiation ~20-30s after power-on
		 * (which is why a manual run from the prompt always worked: the
		 * dock had settled by then).  PAN_EN is armed, so poll for the
		 * ADSP DP notify (mux=3 / dp_seen) and exit the instant it
		 * arrives; the long window just lets a cold dock settle.  A warm
		 * boot (dock already in DP) never reaches here.  Disable with
		 * tachyon_dp_no_auto_dfp=1; tune the window with
		 * tachyon_dp_auto_dfp_ms.
		 */
		if (!tachyon_dp_env_bool("tachyon_dp_no_auto_dfp")) {
			int dfp = qcom_pmic_glink_request_dfp(
				tachyon_dp_env_u32("tachyon_dp_auto_dfp_ms",
						   20000));

			log_debug("DP auto-DFP ret=%d; re-reading altmode\n",
				    dfp);
			altmode_ret = tachyon_dp_read_altmode(priv);
			if (altmode_ret > 0) {
				priv->typec_source =
					TACHYON_DP_TYPEC_SOURCE_ALTMODE;
				priv->typec_valid = true;
				if (tachyon_dp_typec_state_valid(priv)) {
					tachyon_dp_log_typec_resolved(priv);
				} else {
					priv->typec_valid = false;
					priv->typec_source =
						TACHYON_DP_TYPEC_SOURCE_NONE;
				}
			}
		}
	}

	ret = tachyon_dp_request_sbu_mux(priv);
	if (ret)
		log_warning("SBU mux unavailable: %d\n", ret);

	has_sbu_mux = dm_gpio_is_valid(&priv->sbu_enable) &&
		      dm_gpio_is_valid(&priv->sbu_select);
	log_debug("DP SBU mux usable=%d\n", has_sbu_mux ? 1 : 0);

	if (!priv->typec_valid) {
		const struct qcom_pmic_glink_altmode_state *state;

		state = qcom_pmic_glink_altmode_get_state();

		log_debug("DP Type-C Alt Mode not active before AUX; continuing with PMIC state=%u svid=%04x orient_raw=%u mux=%u dpam=%02x hpd=%u\n",
			    state ? state->typec_state : 0,
			    state ? state->svid : 0,
			    state ? state->orientation_raw : 0xff,
			    state ? state->mux : 0xff,
			    state ? state->dpam_raw : 0xff,
			    state ? state->hpd : 0);

		/*
		 * Diagnostic fallback:
		 * Linux proves this connector ultimately works through
		 * aux_hpd_bridge. Do not abort before AUX. Pick a forced
		 * orientation for testing.
		 */
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
		priv->typec_valid = true;
		forced_typec = true;

		if (tachyon_dp_env_bool("tachyon_dp_force_reverse"))
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
		else
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;

		/*
		 * Forced/diagnostic path: there was no ADSP alt-mode notify, so
		 * the negotiated Type-C pin is unknown. The Tachyon's target
		 * sinks are USB-C docks/hubs, which negotiate pin D = 2-lane DP +
		 * USB3 (combo MODE=0x03) — so default to pin D. Override with
		 * tachyon_dp_force_pin only for experiments (C=2 / E=4 are 4-lane
		 * DP-only MODE=0x02; F=5 is the reversed 2-lane combo). The pin
		 * choice drives both the link lane budget (reset_link_policy) and
		 * the QMP combo split (tachyon_dp_qmp_phy_mode).
		 */
		{
			u8 forced_pin = tachyon_dp_env_u32("tachyon_dp_force_pin",
							   3);

			if (!tachyon_dp_valid_pin_assignment(forced_pin)) {
				log_warning("DP invalid tachyon_dp_force_pin=%u; using pin D (3)\n",
					    forced_pin);
				forced_pin = 3;
			}
			priv->pin_assignment = forced_pin;
		}
		log_debug("DP forced Type-C: orientation=%u pin=%u pin_lanes=%u (override: tachyon_dp_force_pin / _reverse / _lanes / _max_rate)\n",
			    priv->orientation, priv->pin_assignment,
			    tachyon_dp_pin_assignment_lanes(priv));
		tachyon_dp_log_typec_resolved(priv);
	}

	/*
	 * Resolve the (orientation, pin) actually used and read the sink's DPCD
	 * link caps.  Each attempt first brings the DP PHY to PHY_READY
	 * (DP_STATUS) and forces AUX on for that orientation — without a locked
	 * PLL, AUX reads fail with DP_INTR_TIMEOUT and DP_STATUS=00.
	 *
	 * With a real ADSP alt-mode notify we already know orientation+pin, so
	 * there is a single candidate.  On the forced/diagnostic path the hub
	 * only reveals its pin once it has entered DP, so instead of forcing one
	 * mode we SWEEP the viable 2-lane dock assignments — pin D (normal) then
	 * pin F (D reversed), both 2-lane DP+USB3 / combo MODE=0x03 — and keep
	 * whichever the sink answers DPCD on.  AUX timing out (-110) just means
	 * "no sink on this orientation"; move to the next candidate.  An explicit
	 * tachyon_dp_force_pin / tachyon_dp_force_reverse pins a single attempt
	 * (use pin C=2 / E=4 there for a 4-lane DP-only sink).
	 */
	{
		struct { enum tachyon_dp_orientation orient; u8 pin; } cand[4];
		bool explicit_pin =
			tachyon_dp_env_has_u32("tachyon_dp_force_pin") ||
			tachyon_dp_env_bool("tachyon_dp_force_reverse");
		int ncand = 0, ci;

		if (!forced_typec || explicit_pin) {
			cand[ncand].orient = priv->orientation;
			cand[ncand].pin = priv->pin_assignment;
			ncand++;
		} else {
			cand[ncand].orient = TACHYON_DP_ORIENTATION_NORMAL;
			cand[ncand].pin = 3;	/* pin D: 2-lane DP + USB3 */
			ncand++;
			cand[ncand].orient = TACHYON_DP_ORIENTATION_REVERSE;
			cand[ncand].pin = 3;	/* pin F: same, reversed */
			ncand++;
		}

		ret = -EIO;
		for (ci = 0; ci < ncand; ci++) {
			priv->orientation = cand[ci].orient;
			priv->pin_assignment = cand[ci].pin;
			priv->rate = DP_LINK_RATE_HBR;
			priv->lanes = 2;

			log_debug("DP probe %d/%d: orientation=%u pin=%u pin_lanes=%u\n",
				    ci + 1, ncand, priv->orientation,
				    priv->pin_assignment,
				    tachyon_dp_pin_assignment_lanes(priv));

			tachyon_dp_program_sbu_mux(priv);
			tachyon_dp_prepare_aux_for_orientation(priv,
							       priv->orientation);

			ret = tachyon_dp_qmp_program_dp_phy(priv);
			log_debug("DP probe PHY bring-up ret=%d DP_STATUS=%02x\n",
				    ret, tachyon_dp_qmp_status_low(priv));

			ret = tachyon_dp_read_dpcd_caps(priv);
			log_debug("DP probe DPCD ret=%d caps.lanes=%u caps.max_rate=%u\n",
				    ret, priv->caps.lanes, priv->caps.max_rate);

			if (!ret && priv->caps.lanes && priv->caps.max_rate) {
				log_debug("DP probe LOCKED orientation=%u pin=%u after %d/%d\n",
					    priv->orientation,
					    priv->pin_assignment, ci + 1, ncand);
				break;
			}
		}
	}
	if (ret || !priv->caps.lanes || !priv->caps.max_rate) {
		priv->caps.lanes = priv->caps.lanes ? priv->caps.lanes : 2;
		priv->caps.max_rate = priv->caps.max_rate ?
				      priv->caps.max_rate : DP_LINK_RATE_HBR;
	}
	if (!priv->max_rate)
		priv->max_rate = priv->caps.max_rate;
	if (!priv->max_lanes)
		priv->max_lanes = priv->caps.lanes;
	if (!priv->rate)
		priv->rate = priv->max_rate;
	if (!priv->lanes)
		priv->lanes = priv->max_lanes;
	log_debug("DP link budget: max_rate=%u max_lanes=%u rate=%u lanes=%u caps.lanes=%u\n",
		    priv->max_rate, priv->max_lanes, priv->rate, priv->lanes,
		    priv->caps.lanes);

	log_debug("DP wait sink start with derived Type-C orientation=%u\n",
		    priv->orientation);

	tachyon_dp_prepare_aux_for_orientation(priv, priv->orientation);
	ret = tachyon_dp_wait_sink(priv);

	log_debug("DP wait sink done ret=%d orientation=%u\n",
		    ret, priv->orientation);

	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_read_edid_modes(priv);
	if (ret) {
		/*
		 * No EDID => no reachable DP sink (e.g. a USB-C hub that hasn't
		 * entered DP Alt Mode, so its mux never routed DP/AUX to the
		 * panel). Abort gracefully and cleanly release the PHY/SBU/clocks
		 * rather than crashing into link-training with zero lanes.
		 */
		log_warning("DP: no EDID / no DP sink reachable (%d) - aborting cleanly\n",
			    ret);
		goto err_quiesce;
	}


	ret = tachyon_dp_link_train(priv);
	if (ret)
		goto err_quiesce;
	priv->dp_link_up = true;

	/*
	 * Re-filter EDID modes against actual trained rate/lanes, which may
	 * be lower than the policy ceiling after link-training fallback.
	 */
	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);
	tachyon_dp_select_mode(priv, &width, &height);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;

	/*
	 * bind() leaves plat->size = 0 in manual mode (no boot-time FB
	 * reservation -> clean Linux handoff).  Set it now that we're actually
	 * bringing DP up.
	 */
	if (!plat->size)
		plat->size = TACHYON_DP_MAX_XRES * TACHYON_DP_MAX_YRES * 4;

	/*
	 * Allocate the framebuffer below 4 GB.  The DPU SSPP source-address
	 * register is 32-bit and there's no IOMMU in U-Boot, so a >4 GB buffer
	 * (the video uclass would reserve it at the top of this 8 GB+ board's RAM)
	 * has its high bits dropped by the SSPP -> DPU fetches the wrong memory ->
	 * "No Signal".  Reserve a region below 4 GB via lmb the DPU can address.
	 */
	if (!plat->base || (u64)plat->base + plat->size > 0x100000000ULL) {
		phys_addr_t low = lmb_alloc_base(plat->size, plat->align,
						 0x100000000ULL, LMB_NOOVERWRITE);

		if (low) {
			log_debug("DP relocating FB %lx -> %llx (<4GB for DPU SSPP)\n",
				    (ulong)plat->base, (u64)low);
			plat->base = (ulong)low;
		} else {
			log_warning("DP: lmb <4GB FB alloc failed; DPU may fetch wrong address\n");
		}
	}

	video_set_flush_dcache(dev, true);
	/*
	 * Initialise the framebuffer the DPU scans out.  Default to a clean
	 * black background so the real U-Boot console renders on it: with
	 * CONFIG_NO_FB_CLEAR=y the video uclass does NOT clear this freshly
	 * lmb-allocated buffer, after which video_post_probe draws the logo on
	 * the dock.  Set the env var
	 * "tachyon_dp_test_pattern" to instead paint vertical colour bars — a
	 * fetch-vs-no-fetch diagnostic: if the dock shows bars the DPU is
	 * genuinely fetching the framebuffer.
	 */
	if (tachyon_dp_env_bool("tachyon_dp_test_pattern")) {
		tachyon_dp_fill_test_pattern(plat, uc_priv);
	} else {
		memset((void *)plat->base, 0,
		       (size_t)uc_priv->line_length * uc_priv->ysize);
		flush_dcache_range((ulong)plat->base,
				   (ulong)plat->base +
				   (ulong)uc_priv->line_length * uc_priv->ysize);
	}

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		goto err_quiesce;

	log_debug("DP ready: %ux%u fb=%lx size=%lx aux timeouts=%u nacks=%u retries=%u\n",
		    width, height, (ulong)plat->base, (ulong)plat->size,
		    priv->aux_timeouts, priv->aux_nacks, priv->aux_retries);

	log_info("DP framebuffer base=%lx size=%lx aux timeouts=%u nacks=%u retries=%u\n",
		 (ulong)plat->base, (ulong)plat->size, priv->aux_timeouts,
		 priv->aux_nacks, priv->aux_retries);

	return 0;

err_quiesce:
	tachyon_dp_quiesce(priv);
	return ret;
}

static int tachyon_dp_video_sync(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	bool alt_changed = false;
	bool mode_changed;
	int ret;

	/*
	 * Once the DP link is trained and scanning out, do NOT keep re-poking
	 * the ADSP altmode service or re-training on every video_sync.  Doing so
	 * knocks the live DP link back to "safe" (the TV loses signal) and floods
	 * the console with altmode polls.  Keep the link stable and just let the
	 * video uclass flush the framebuffer.
	 */
	if (priv->dp_link_up)
		return 0;

	ret = tachyon_dp_refresh_altmode(priv, &alt_changed);
	if (ret)
		return ret;


	if (alt_changed) {
		tachyon_dp_prepare_aux_for_orientation(priv, priv->orientation);
		ret = tachyon_dp_wait_sink(priv);
		if (ret)
			return ret;
		ret = tachyon_dp_read_edid_modes(priv);
		if (ret)
			log_warning("Failed to refresh DP EDID modes: %d\n",
				    ret);
	}

	tachyon_dp_env_mode(&width, &height);
	mode_changed = width != priv->timing.hactive.typ ||
		       height != priv->timing.vactive.typ;
	if (!mode_changed && !alt_changed)
		return 0;

	if (!alt_changed)
		tachyon_dp_reset_link_policy(priv);

	ret = tachyon_dp_link_train(priv);
	if (ret)
		return ret;

	/*
	 * Re-filter modes against actual trained parameters in case link
	 * training fell back to a lower rate or lane count.
	 */
	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);
	tachyon_dp_select_mode(priv, &width, &height);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;
	if (uc_priv->fb_size > plat->size)
		return -ENOSPC;

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		return ret;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		return ret;

	return 0;
}

static int tachyon_dp_get_mode_count(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	return priv->mode_count;
}

static int tachyon_dp_get_mode_info(struct udevice *dev, u32 mode_number,
				    u32 *width, u32 *height,
				    enum video_format *format,
				    enum video_log2_bpp *bpix)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	if (mode_number >= (u32)priv->mode_count)
		return -ENOENT;

	*width = priv->modes[mode_number].width;
	*height = priv->modes[mode_number].height;
	*format = VIDEO_X8R8G8B8;
	*bpix = VIDEO_BPP32;
	return 0;
}

static int tachyon_dp_set_mode(struct udevice *dev, u32 mode_number)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	int ret;

	if (priv->mode_count < 1)
		return -EINVAL;
	if (mode_number >= (u32)priv->mode_count)
		return -EINVAL;

	width = priv->modes[mode_number].width;
	height = priv->modes[mode_number].height;

	/*
	 * SetMode() must respect the caller-chosen mode index, not
	 * tachyon_dp_env_mode().  Resolve timing directly from the
	 * requested mode, then train link + reprogram DPU/DP.
	 */
	if (!tachyon_dp_resolve_mode_timing(priv, mode_number))
		return -EINVAL;

	tachyon_dp_reset_link_policy(priv);
	memset(priv->swing, 0, sizeof(priv->swing));
	memset(priv->pre, 0, sizeof(priv->pre));

	ret = tachyon_dp_link_train(priv);
	if (ret)
		return ret;

	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	/*
	 * After link fallback, the originally requested mode may no longer
	 * fit within the trained rate/lanes.  Re-check bandwidth here so
	 * we don't blindly program a mode that exceeds link capacity (which
	 * would result in a black screen or unstable link).
	 */
	if (!tachyon_dp_mode_fits_link(priv, &priv->timing))
		return -ENOSPC;

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;
	if (uc_priv->fb_size > plat->size)
		return -ENOSPC;

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		return ret;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		return ret;

	return 0;
}


static const struct video_ops tachyon_dp_ops = {
	.video_sync = tachyon_dp_video_sync,
	.video_get_mode_count = tachyon_dp_get_mode_count,
	.video_get_mode_info = tachyon_dp_get_mode_info,
	.video_set_mode = tachyon_dp_set_mode,
};

static int tachyon_dp_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->align = TACHYON_DP_FB_ALIGN;

	plat->size = TACHYON_DP_MAX_XRES * TACHYON_DP_MAX_YRES * 4;

	return 0;
}


static const struct udevice_id tachyon_dp_ids[] = {
	{ .compatible = "qcom,sc7280-dp" },
	{ }
};

U_BOOT_DRIVER(tachyon_dp) = {
	.name		= "tachyon_dp",
	.id		= UCLASS_VIDEO,
	.of_match	= tachyon_dp_ids,
	.bind		= tachyon_dp_bind,
	.probe		= tachyon_dp_probe,
	.ops		= &tachyon_dp_ops,
	.priv_auto	= sizeof(struct tachyon_dp_priv),
	.plat_auto	= sizeof(struct video_uc_plat),
	.flags		= DM_FLAG_PRE_RELOC |
			  DM_FLAG_DEFAULT_PD_CTRL_OFF |
			  DM_FLAG_DEFAULT_CLKS_OFF,
};
