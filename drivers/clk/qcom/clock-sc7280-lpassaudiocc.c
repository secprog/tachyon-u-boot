// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal SC7280/QCM6490 LPASS Audio Clock Controller support.
 *
 * This intentionally models only the clocks Tachyon marks as protected. On
 * QCM6490 the ADSP owns the LPASS audio PLL path, represented in DT with
 * qcom,adsp-skip-pll, so PLL and RCG clocks are accepted but not programmed.
 */

#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <dt-bindings/clock/qcom,lpassaudiocc-sc7280.h>

#define CBCR_BRANCH_ENABLE	BIT(0)
#define CBCR_BRANCH_OFF	BIT(31)
#define LPASS_AUDIOCC_POLL_US	1500

struct sc7280_lpass_audio_cc_priv {
	phys_addr_t base;
	bool adsp_skip_pll;
};

struct sc7280_lpass_audio_cc_clk {
	u32 reg;
	const char *name;
	bool adsp_owned;
};

static const struct sc7280_lpass_audio_cc_clk lpass_audio_cc_clks[] = {
	[LPASS_AUDIO_CC_PLL] = {
		.name = "lpass_audio_cc_pll",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_PLL_OUT_AUX2] = {
		.name = "lpass_audio_cc_pll_out_aux2",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_PLL_OUT_AUX2_DIV_CLK_SRC] = {
		.name = "lpass_audio_cc_pll_out_aux2_div_clk_src",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_PLL_OUT_MAIN_DIV_CLK_SRC] = {
		.name = "lpass_audio_cc_pll_out_main_div_clk_src",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_CDIV_RX_MCLK_DIV_CLK_SRC] = {
		.name = "lpass_audio_cc_cdiv_rx_mclk_div_clk_src",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_CODEC_MEM0_CLK] = {
		.reg = 0x1e004,
		.name = "lpass_audio_cc_codec_mem0_clk",
	},
	[LPASS_AUDIO_CC_CODEC_MEM1_CLK] = {
		.reg = 0x1e008,
		.name = "lpass_audio_cc_codec_mem1_clk",
	},
	[LPASS_AUDIO_CC_CODEC_MEM2_CLK] = {
		.reg = 0x1e00c,
		.name = "lpass_audio_cc_codec_mem2_clk",
	},
	[LPASS_AUDIO_CC_CODEC_MEM_CLK] = {
		.reg = 0x1e000,
		.name = "lpass_audio_cc_codec_mem_clk",
	},
	[LPASS_AUDIO_CC_EXT_MCLK0_CLK] = {
		.reg = 0x20018,
		.name = "lpass_audio_cc_ext_mclk0_clk",
	},
	[LPASS_AUDIO_CC_EXT_MCLK0_CLK_SRC] = {
		.name = "lpass_audio_cc_ext_mclk0_clk_src",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_EXT_MCLK1_CLK] = {
		.reg = 0x21018,
		.name = "lpass_audio_cc_ext_mclk1_clk",
	},
	[LPASS_AUDIO_CC_EXT_MCLK1_CLK_SRC] = {
		.name = "lpass_audio_cc_ext_mclk1_clk_src",
		.adsp_owned = true,
	},
	[LPASS_AUDIO_CC_RX_MCLK_2X_CLK] = {
		.reg = 0x240cc,
		.name = "lpass_audio_cc_rx_mclk_2x_clk",
	},
	[LPASS_AUDIO_CC_RX_MCLK_CLK] = {
		.reg = 0x240d4,
		.name = "lpass_audio_cc_rx_mclk_clk",
	},
	[LPASS_AUDIO_CC_RX_MCLK_CLK_SRC] = {
		.name = "lpass_audio_cc_rx_mclk_clk_src",
		.adsp_owned = true,
	},
};

static int sc7280_lpass_audio_cc_enable(struct clk *clk)
{
	struct sc7280_lpass_audio_cc_priv *priv = dev_get_priv(clk->dev);
	const struct sc7280_lpass_audio_cc_clk *map;
	u32 val;
	int ret;

	if (clk->id >= ARRAY_SIZE(lpass_audio_cc_clks))
		return -ENOENT;

	map = &lpass_audio_cc_clks[clk->id];
	if (!map->reg) {
		if (map->adsp_owned && priv->adsp_skip_pll)
			return 0;

		return -ENOENT;
	}

	setbits_le32(priv->base + map->reg, CBCR_BRANCH_ENABLE);

	ret = readl_poll_timeout(priv->base + map->reg, val,
				 !(val & CBCR_BRANCH_OFF),
				 LPASS_AUDIOCC_POLL_US);
	if (ret)
		printf("WARNING: %s stuck while enabling\n",
		       map->name ? map->name : "lpass_audio_cc");

	return ret;
}

static int sc7280_lpass_audio_cc_disable(struct clk *clk)
{
	return 0;
}

static ulong sc7280_lpass_audio_cc_get_rate(struct clk *clk)
{
	return 0;
}

static const struct clk_ops sc7280_lpass_audio_cc_ops = {
	.enable = sc7280_lpass_audio_cc_enable,
	.disable = sc7280_lpass_audio_cc_disable,
	.get_rate = sc7280_lpass_audio_cc_get_rate,
};

static int sc7280_lpass_audio_cc_probe(struct udevice *dev)
{
	struct sc7280_lpass_audio_cc_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->adsp_skip_pll = dev_read_bool(dev, "qcom,adsp-skip-pll");

	return 0;
}

static const struct udevice_id sc7280_lpass_audio_cc_ids[] = {
	{ .compatible = "qcom,sc7280-lpassaudiocc" },
	{ .compatible = "qcom,qcm6490-lpassaudiocc" },
	{ }
};

U_BOOT_DRIVER(sc7280_lpass_audio_cc) = {
	.name = "sc7280_lpass_audio_cc",
	.id = UCLASS_CLK,
	.of_match = sc7280_lpass_audio_cc_ids,
	.ops = &sc7280_lpass_audio_cc_ops,
	.probe = sc7280_lpass_audio_cc_probe,
	.priv_auto = sizeof(struct sc7280_lpass_audio_cc_priv),
	.flags = DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
