// SPDX-License-Identifier: GPL-2.0
/*
 * Clock drivers for Qualcomm sc7280
 *
 * (C) Copyright 2024 Linaro Ltd.
 */

#include <linux/types.h>
#include <clk-uclass.h>
#include <dm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/bitops.h>
#include <dt-bindings/clock/qcom,gcc-sc7280.h>

#include "clock-qcom.h"

#define USB30_PRIM_MOCK_UTMI_CLK_CMD_RCGR 0xf038
#define USB30_PRIM_MASTER_CLK_CMD_RCGR 0xf020
#define QUPV3_WRAP0_S5_CLK_CMD_RCGR 0x17600

static const struct freq_tbl ftbl_gcc_qupv3_wrap0_s5_clk_src[] = {
	F(7372800, CFG_CLK_SRC_GPLL0_EVEN, 1, 384, 15625),
	F(14745600, CFG_CLK_SRC_GPLL0_EVEN, 1, 768, 15625),
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	F(29491200, CFG_CLK_SRC_GPLL0_EVEN, 1, 1536, 15625),
	F(32000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 75),
	F(48000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 25),
	F(64000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 16, 75),
	F(75000000, CFG_CLK_SRC_GPLL0_EVEN, 4, 0, 0),
	F(80000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 15),
	F(96000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 25),
	F(100000000, CFG_CLK_SRC_GPLL0, 6, 0, 0),
	{ }
};

static ulong sc7280_set_rate(struct clk *clk, ulong rate)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);
	const struct freq_tbl *freq;

	if (clk->id < priv->data->num_clks)
		debug("%s: %s, requested rate=%ld\n", __func__, priv->data->clks[clk->id].name, rate);

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S5_CLK:
		freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s5_clk_src, rate);
		if (!freq || !freq->freq)
			return 0;
		clk_rcg_set_rate_mnd(priv->base, QUPV3_WRAP0_S5_CLK_CMD_RCGR,
				     freq->pre_div, freq->m, freq->n,
				     freq->src, 16);
		return freq->freq;
	case GCC_USB30_PRIM_MOCK_UTMI_CLK:
		WARN(rate != 19200000, "Unexpected rate for USB30_PRIM_MOCK_UTMI_CLK: %lu\n", rate);
		clk_rcg_set_rate(priv->base, USB30_PRIM_MASTER_CLK_CMD_RCGR, 0, CFG_CLK_SRC_CXO);
		return rate;
	case GCC_USB30_PRIM_MASTER_CLK:
		WARN(rate != 200000000, "Unexpected rate for USB30_PRIM_MASTER_CLK: %lu\n", rate);
		clk_rcg_set_rate_mnd(priv->base, USB30_PRIM_MASTER_CLK_CMD_RCGR,
				     1, 0, 0, CFG_CLK_SRC_GPLL0_ODD, 8);
		clk_rcg_set_rate(priv->base, 0xf064, 0, 0);
		return rate;
	default:
		return 0;
	}
}

static const struct gate_clk sc7280_clks[] = {
	GATE_CLK(GCC_CFG_NOC_USB3_PRIM_AXI_CLK, 0xf07c, 1),
	GATE_CLK(GCC_USB30_PRIM_MASTER_CLK, 0xf010, 1),
	GATE_CLK(GCC_AGGRE_USB3_PRIM_AXI_CLK, 0xf080, 1),
	GATE_CLK(GCC_QUPV3_WRAP0_CORE_2X_CLK, 0x52008, BIT(9)),
	GATE_CLK(GCC_QUPV3_WRAP0_CORE_CLK, 0x52008, BIT(8)),
	GATE_CLK(GCC_QUPV3_WRAP0_S5_CLK, 0x52008, BIT(15)),
	GATE_CLK(GCC_QUPV3_WRAP_0_M_AHB_CLK, 0x52008, BIT(6)),
	GATE_CLK(GCC_QUPV3_WRAP_0_S_AHB_CLK, 0x52008, BIT(7)),
	GATE_CLK(GCC_USB30_PRIM_SLEEP_CLK, 0xf018, 1),
	GATE_CLK(GCC_USB30_PRIM_MOCK_UTMI_CLK, 0xf01c, 1),
	GATE_CLK(GCC_USB3_PRIM_PHY_AUX_CLK, 0xf054, 1),
	GATE_CLK(GCC_USB3_PRIM_PHY_COM_AUX_CLK, 0xf058, 1),
	GATE_CLK(GCC_DISP_AHB_CLK, 0x27004, 0x00000001),
	GATE_CLK(GCC_DISP_HF_AXI_CLK, 0x2700c, 0x00000001),
	GATE_CLK(GCC_DISP_SF_AXI_CLK, 0x27014, 0x00000001),
	GATE_CLK(GCC_DISP_XO_CLK, 0x2701c, 0x00000001),
};

static int sc7280_enable(struct clk *clk)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);

	if (clk->id >= priv->data->num_clks) {
		debug("%s: unknown clk id %lu\n", __func__, clk->id);
		return 0;
	}

	debug("%s: clk %ld: %s\n", __func__, clk->id, sc7280_clks[clk->id].name);

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S5_CLK:
		qcom_gate_clk_en(priv, GCC_QUPV3_WRAP_0_M_AHB_CLK);
		qcom_gate_clk_en(priv, GCC_QUPV3_WRAP_0_S_AHB_CLK);
		qcom_gate_clk_en(priv, GCC_QUPV3_WRAP0_CORE_CLK);
		qcom_gate_clk_en(priv, GCC_QUPV3_WRAP0_CORE_2X_CLK);
		break;
	case GCC_AGGRE_USB3_PRIM_AXI_CLK:
		qcom_gate_clk_en(priv, GCC_USB30_PRIM_MASTER_CLK);
		fallthrough;
	case GCC_USB30_PRIM_MASTER_CLK:
		qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_AUX_CLK);
		qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_COM_AUX_CLK);
		break;
	case GCC_DISP_HF_AXI_CLK:
		qcom_gate_clk_en(priv, GCC_DISP_AHB_CLK);
		qcom_gate_clk_en(priv, GCC_DISP_XO_CLK);
		break;
	}

	qcom_gate_clk_en(priv, clk->id);

	return 0;
}

static const struct qcom_reset_map sc7280_gcc_resets[] = {
	[GCC_PCIE_0_BCR] = { 0x6b000 },
	[GCC_PCIE_0_PHY_BCR] = { 0x6c01c },
	[GCC_PCIE_1_BCR] = { 0x8d000 },
	[GCC_PCIE_1_PHY_BCR] = { 0x8e01c },
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x12000 },
	[GCC_QUSB2PHY_SEC_BCR] = { 0x12004 },
	[GCC_SDCC1_BCR] = { 0x75000 },
	[GCC_SDCC2_BCR] = { 0x14000 },
	[GCC_SDCC4_BCR] = { 0x16000 },
	[GCC_UFS_PHY_BCR] = { 0x77000 },
	[GCC_USB30_PRIM_BCR] = { 0xf000 },
	[GCC_USB30_SEC_BCR] = { 0x9e000 },
	[GCC_USB3_DP_PHY_PRIM_BCR] = { 0x50008 },
	[GCC_USB3_PHY_PRIM_BCR] = { 0x50000 },
	[GCC_USB3PHY_PHY_PRIM_BCR] = { 0x50004 },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x6a000 },
};

static const struct qcom_power_map sc7280_gdscs[] = {
	[GCC_UFS_PHY_GDSC] = { 0x77004 },
	[GCC_USB30_PRIM_GDSC] = { 0xf004 },
};

static struct msm_clk_data qcs404_gcc_data = {
	.resets = sc7280_gcc_resets,
	.num_resets = ARRAY_SIZE(sc7280_gcc_resets),
	.clks = sc7280_clks,
	.num_clks = ARRAY_SIZE(sc7280_clks),

	.power_domains = sc7280_gdscs,
	.num_power_domains = ARRAY_SIZE(sc7280_gdscs),

	.enable = sc7280_enable,
	.set_rate = sc7280_set_rate,
};

static const struct udevice_id gcc_sc7280_of_match[] = {
	{
		.compatible = "qcom,gcc-sc7280",
		.data = (ulong)&qcs404_gcc_data,
	},
	{ }
};

U_BOOT_DRIVER(gcc_sc7280) = {
	.name		= "gcc_sc7280",
	.id		= UCLASS_NOP,
	.of_match	= gcc_sc7280_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC | DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
