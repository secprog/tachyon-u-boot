// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal Qualcomm RPMh clock support.
 *
 * Linux clk-rpmh votes RPMH_CXO_CLK through the "xo.lvl" ARC resource with
 * enable value 0x3.  U-Boot only needs that path today for Q6V5/PAS proxy
 * resources such as ADSP.
 */

#include <clk-uclass.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <log.h>
#include <linux/err.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/rpmh.h>

#include <dt-bindings/clock/qcom,rpmh.h>

#define RPMH_XO_ON_VAL		0x3

struct qcom_rpmh_clk_priv {
	struct udevice *dev;
	u32 xo_addr;
	bool xo_on;
};

static int qcom_rpmh_clk_send(struct qcom_rpmh_clk_priv *priv, u32 data)
{
	struct tcs_cmd cmd = {
		.addr = priv->xo_addr,
		.data = data,
	};

	return rpmh_write(priv->dev, RPMH_ACTIVE_ONLY_STATE, &cmd, 1);
}

static int qcom_rpmh_clk_enable(struct clk *clk)
{
	struct qcom_rpmh_clk_priv *priv = dev_get_priv(clk->dev);
	int ret;

	if (clk->id != RPMH_CXO_CLK)
		return -ENOTSUPP;

	if (priv->xo_on)
		return 0;

	ret = qcom_rpmh_clk_send(priv, RPMH_XO_ON_VAL);
	log_warning("qcom-rpmh-clk: enable xo.lvl addr=%#x data=%#x ret=%d\n",
		    priv->xo_addr, RPMH_XO_ON_VAL, ret);
	if (!ret)
		priv->xo_on = true;

	return ret;
}

static int qcom_rpmh_clk_disable(struct clk *clk)
{
	struct qcom_rpmh_clk_priv *priv = dev_get_priv(clk->dev);
	int ret;

	if (clk->id != RPMH_CXO_CLK)
		return -ENOTSUPP;

	if (!priv->xo_on)
		return 0;

	ret = qcom_rpmh_clk_send(priv, 0);
	log_warning("qcom-rpmh-clk: disable xo.lvl addr=%#x ret=%d\n",
		    priv->xo_addr, ret);
	if (!ret)
		priv->xo_on = false;

	return ret;
}

static ulong qcom_rpmh_clk_get_rate(struct clk *clk)
{
	if (clk->id != RPMH_CXO_CLK)
		return 0;

	return 19200000;
}

static int qcom_rpmh_clk_probe(struct udevice *dev)
{
	struct qcom_rpmh_clk_priv *priv = dev_get_priv(dev);
	int ret;

	priv->dev = dev;
	priv->xo_addr = cmd_db_read_addr("xo.lvl");
	if (!priv->xo_addr) {
		dev_err(dev, "qcom-rpmh-clk: missing xo.lvl cmd-db address\n");
		return -ENODEV;
	}

	ret = cmd_db_read_slave_id("xo.lvl");
	if (ret != CMD_DB_HW_ARC) {
		dev_err(dev, "qcom-rpmh-clk: xo.lvl slave mismatch ret=%d\n",
			ret);
		return -EINVAL;
	}

	log_warning("qcom-rpmh-clk: xo.lvl addr=%#x\n", priv->xo_addr);

	return 0;
}

static const struct clk_ops qcom_rpmh_clk_ops = {
	.enable = qcom_rpmh_clk_enable,
	.disable = qcom_rpmh_clk_disable,
	.get_rate = qcom_rpmh_clk_get_rate,
};

static const struct udevice_id qcom_rpmh_clk_ids[] = {
	{ .compatible = "qcom,sc7280-rpmh-clk" },
	{ }
};

U_BOOT_DRIVER(qcom_rpmh_clk) = {
	.name = "qcom_rpmh_clk",
	.id = UCLASS_CLK,
	.of_match = qcom_rpmh_clk_ids,
	.ops = &qcom_rpmh_clk_ops,
	.probe = qcom_rpmh_clk_probe,
	.priv_auto = sizeof(struct qcom_rpmh_clk_priv),
};
