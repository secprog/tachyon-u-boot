// SPDX-License-Identifier: GPL-2.0+
/*
 * Display Clock drivers for Qualcomm sc7280
 */

#include <clk-uclass.h>
#include <dm.h>
#include <dt-bindings/clock/qcom,dispcc-sc7280.h>
#include "clock-qcom.h"

static const struct gate_clk sc7280_dispcc_clks[] = {
	GATE_CLK(DISP_CC_MDSS_AHB_CLK, 0x1050, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_BYTE0_CLK, 0x1030, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_BYTE0_INTF_CLK, 0x1034, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_DP_AUX_CLK, 0x104c, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_DP_CRYPTO_CLK, 0x1044, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_DP_LINK_CLK, 0x103c, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_DP_LINK_INTF_CLK, 0x1040, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_DP_PIXEL_CLK, 0x1048, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_EDP_AUX_CLK, 0x1060, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_EDP_LINK_CLK, 0x1058, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_EDP_LINK_INTF_CLK, 0x105c, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_EDP_PIXEL_CLK, 0x1054, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_ESC0_CLK, 0x1038, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_MDP_CLK, 0x1014, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_MDP_LUT_CLK, 0x1024, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_NON_GDSC_AHB_CLK, 0x2004, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_PCLK0_CLK, 0x1010, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_ROT_CLK, 0x101c, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_RSCC_AHB_CLK, 0x200c, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_RSCC_VSYNC_CLK, 0x2008, 0x00000001),
	GATE_CLK(DISP_CC_MDSS_VSYNC_CLK, 0x102c, 0x00000001),
	GATE_CLK(DISP_CC_SLEEP_CLK, 0x5004, 0x00000001),
	GATE_CLK(DISP_CC_XO_CLK, 0x5008, 0x00000001),
};

/*
 * The DP pixel clock (DISP_CC_MDSS_DP_PIXEL_CLK_SRC at 0x1140) is sourced from
 * the DP PHY PLL VCO_DIV output and must be divided down to the mode's pixel
 * clock by an M/N divider.  The required M/N depends on the link rate AND the
 * pixel rate, which only the DP driver knows, so it computes them and stashes
 * them here before enabling the clock.  If unset (0), we fall back to the old
 * pass-through (M=0/N=0), which runs the pixel clock far too fast.
 */
static u32 sc7280_dp_pixel_m;
static u32 sc7280_dp_pixel_n;

void sc7280_dispcc_set_dp_pixel_mn(u32 m, u32 n)
{
	sc7280_dp_pixel_m = m;
	sc7280_dp_pixel_n = n;
}

static int sc7280_dispcc_enable(struct clk *clk)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case DISP_CC_MDSS_AHB_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x1170, 1, 0, 0, 0, 8);
		break;
	case DISP_CC_MDSS_DP_AUX_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x1158, 1, 0, 0, 0, 8);
		break;
	case DISP_CC_MDSS_DP_CRYPTO_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x1128, 1, 0, 0, 1 << 8, 8);
		break;
	case DISP_CC_MDSS_DP_LINK_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x110c, 1, 0, 0, 1 << 8, 8);
		break;
	case DISP_CC_MDSS_DP_PIXEL_CLK:
		/*
		 * DP pixel RCG has 16-bit M/N/D registers (Linux mnd_width=16).
		 * Passing 8 here truncates ~(N-M) to 8 bits, yielding a wildly
		 * wrong divider (clock ~227 kHz) that starves the p0 region's
		 * clock and faults the bus.  Must be 16.
		 */
		clk_rcg_set_rate_mnd(priv->base, 0x1140, 1, sc7280_dp_pixel_m,
				     sc7280_dp_pixel_n, 2 << 8, 16);
		break;
	case DISP_CC_MDSS_MDP_CLK:
		/*
		 * MDP core clock.  Was TCXO/1 = 19.2 MHz (src=0, div=1) which is
		 * far too slow to sustain a 1080p60 SSPP fetch: the DPU pipe
		 * underruns and the INTF emits its underflow colour (black) on
		 * every active line while still frame-locking off the DP pixel
		 * clock (FB has the bars, all config correct, yet black).
		 * Source GPLL0 (mux value 4 on disp_cc_parent_map_4, Linux
		 * dispcc-sc7280.c:113) and divide by 2 (raw CFG_SRC_DIV code 3 =
		 * 2*2-1) -> 600/2 = 300 MHz, matching Linux
		 * ftbl_disp_cc_mdss_mdp_clk_src F(300000000, GPLL0, 2, 0, 0).
		 * NOTE: clk_rcg_set_rate_mnd writes div RAW (no 2*div-1) and ORs
		 * source straight into CFG_SRC_SEL[10:8], so source must be
		 * pre-shifted (4 << 8); a bare 4 masks to 0 = TCXO.
		 */
		clk_rcg_set_rate_mnd(priv->base, 0x1090, 3, 0, 0, 4 << 8, 8);
		/* Confirm the rate took (prompt can't read dispcc when idle). */
		printf("dispcc MDP_CLK: CMD=%08x CFG=%08x (want CFG src[10:8]=4 div[4:0]=3)\n",
		       readl(priv->base + 0x1090), readl(priv->base + 0x1094));
		break;
	case DISP_CC_MDSS_PCLK0_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x1078, 1, 0, 0, 1 << 8, 8);
		break;
	case DISP_CC_MDSS_ROT_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x10a8, 1, 0, 0, 0, 8);
		break;
	case DISP_CC_MDSS_VSYNC_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x10c0, 1, 0, 0, 0, 8);
		break;
	}

	qcom_gate_clk_en(priv, clk->id);

	return 0;
}

static const struct qcom_power_map sc7280_dispcc_gdscs[] = {
	[DISP_CC_MDSS_CORE_GDSC] = { 0x1004 },
};

static struct msm_clk_data sc7280_dispcc_data = {
	.clks = sc7280_dispcc_clks,
	.num_clks = ARRAY_SIZE(sc7280_dispcc_clks),
	.power_domains = sc7280_dispcc_gdscs,
	.num_power_domains = ARRAY_SIZE(sc7280_dispcc_gdscs),
	.enable = sc7280_dispcc_enable,
};

static const struct udevice_id dispcc_sc7280_of_match[] = {
	{
		.compatible = "qcom,sc7280-dispcc",
		.data = (ulong)&sc7280_dispcc_data,
	},
	{ }
};

U_BOOT_DRIVER(dispcc_sc7280) = {
	.name		= "dispcc_sc7280",
	.id		= UCLASS_NOP,
	.of_match	= dispcc_sc7280_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC | DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
