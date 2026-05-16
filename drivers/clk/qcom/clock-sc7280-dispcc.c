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
		clk_rcg_set_rate_mnd(priv->base, 0x1140, 1, 0, 0, 2 << 8, 8);
		break;
	case DISP_CC_MDSS_MDP_CLK:
		clk_rcg_set_rate_mnd(priv->base, 0x1090, 1, 0, 0, 0, 8);
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

static struct msm_clk_data sc7280_dispcc_data = {
	.clks = sc7280_dispcc_clks,
	.num_clks = ARRAY_SIZE(sc7280_dispcc_clks),
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
	.id		= UCLASS_CLK,
	.of_match	= dispcc_sc7280_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC | DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
