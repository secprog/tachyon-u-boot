// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm PM8xxx/PMK8350 RTC driver
 *
 * Copyright (c) 2026
 */

#define LOG_CATEGORY UCLASS_RTC

#include <dm.h>
#include <errno.h>
#include <log.h>
#include <power/pmic.h>
#include <rtc.h>
#include <linux/bitops.h>
#include <linux/kernel.h>

#define PM8XXX_RTC_ENABLE	BIT(7)
#define PM8XXX_RTC_REGS		4

struct qcom_pm8xxx_rtc_regs {
	u32 ctrl;
	u32 write;
	u32 read;
	u32 alarm_ctrl;
	u32 alarm_en;
};

struct qcom_pm8xxx_rtc_priv {
	const struct qcom_pm8xxx_rtc_regs *regs;
	struct udevice *pmic;
};

static const struct qcom_pm8xxx_rtc_regs pm8941_regs = {
	.ctrl = 0x6046,
	.write = 0x6040,
	.read = 0x6048,
	.alarm_ctrl = 0x6146,
	.alarm_en = BIT(7),
};

static const struct qcom_pm8xxx_rtc_regs pmk8350_regs = {
	.ctrl = 0x6146,
	.write = 0x6140,
	.read = 0x6148,
	.alarm_ctrl = 0x6246,
	.alarm_en = BIT(7),
};

static int qcom_pm8xxx_rtc_read(struct qcom_pm8xxx_rtc_priv *priv,
				uint reg, u8 *buf, int len)
{
	int i, val;

	for (i = 0; i < len; i++) {
		val = pmic_reg_read(priv->pmic, reg + i);
		if (val < 0)
			return val;

		buf[i] = val;
	}

	return 0;
}

static int qcom_pm8xxx_rtc_write(struct qcom_pm8xxx_rtc_priv *priv,
				 uint reg, const u8 *buf, int len)
{
	int i, ret;

	for (i = 0; i < len; i++) {
		ret = pmic_reg_write(priv->pmic, reg + i, buf[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int qcom_pm8xxx_rtc_read_raw(struct qcom_pm8xxx_rtc_priv *priv,
				    u32 *secs)
{
	const struct qcom_pm8xxx_rtc_regs *regs = priv->regs;
	u8 value[PM8XXX_RTC_REGS];
	int ret, lsb;

	ret = qcom_pm8xxx_rtc_read(priv, regs->read, value, sizeof(value));
	if (ret)
		return ret;

	lsb = pmic_reg_read(priv->pmic, regs->read);
	if (lsb < 0)
		return lsb;

	if (lsb < value[0]) {
		ret = qcom_pm8xxx_rtc_read(priv, regs->read, value,
					   sizeof(value));
		if (ret)
			return ret;
	}

	*secs = value[0] | (u32)value[1] << 8 | (u32)value[2] << 16 |
		(u32)value[3] << 24;

	return 0;
}

static int qcom_pm8xxx_rtc_get(struct udevice *dev, struct rtc_time *tm)
{
	struct qcom_pm8xxx_rtc_priv *priv = dev_get_priv(dev);
	u32 secs;
	int ret;

	if (!tm)
		return -EINVAL;

	ret = qcom_pm8xxx_rtc_read_raw(priv, &secs);
	if (ret)
		return ret;

	rtc_to_tm(secs, tm);

	return 0;
}

static int qcom_pm8xxx_rtc_set(struct udevice *dev,
			       const struct rtc_time *tm)
{
	struct qcom_pm8xxx_rtc_priv *priv = dev_get_priv(dev);
	const struct qcom_pm8xxx_rtc_regs *regs = priv->regs;
	u8 value[PM8XXX_RTC_REGS];
	time64_t time;
	u32 secs;
	bool alarm_enabled = false;
	int ret, ret2, alarm_ctrl;

	if (!tm)
		return -EINVAL;

	if (tm->tm_year < 1970)
		return -EINVAL;

	time = rtc_mktime(tm);
	if (time < 0 || time > U32_MAX)
		return -ERANGE;
	secs = time;

	value[0] = secs;
	value[1] = secs >> 8;
	value[2] = secs >> 16;
	value[3] = secs >> 24;

	alarm_ctrl = pmic_reg_read(priv->pmic, regs->alarm_ctrl);
	if (alarm_ctrl < 0)
		return alarm_ctrl;

	alarm_enabled = alarm_ctrl & regs->alarm_en;
	ret = pmic_clrsetbits(priv->pmic, regs->alarm_ctrl, regs->alarm_en, 0);
	if (ret)
		return ret;

	ret = pmic_clrsetbits(priv->pmic, regs->ctrl, PM8XXX_RTC_ENABLE, 0);
	if (ret)
		goto restore_alarm;

	ret = pmic_reg_write(priv->pmic, regs->write, 0);
	if (ret)
		goto enable_rtc;

	ret = qcom_pm8xxx_rtc_write(priv, regs->write + 1, &value[1],
				    sizeof(value) - 1);
	if (ret)
		goto enable_rtc;

	ret = pmic_reg_write(priv->pmic, regs->write, value[0]);

enable_rtc:
	ret2 = pmic_clrsetbits(priv->pmic, regs->ctrl, 0, PM8XXX_RTC_ENABLE);
	if (ret2 && !ret)
		ret = ret2;

restore_alarm:
	if (alarm_enabled) {
		ret2 = pmic_clrsetbits(priv->pmic, regs->alarm_ctrl, 0,
				       regs->alarm_en);
		if (ret2 && !ret)
			ret = ret2;
	}

	return ret;
}

static int qcom_pm8xxx_rtc_reset(struct udevice *dev)
{
	struct rtc_time tm = {
		.tm_year = 1970,
		.tm_mon = 1,
		.tm_mday = 1,
	};

	return qcom_pm8xxx_rtc_set(dev, &tm);
}

static int qcom_pm8xxx_rtc_read8(struct udevice *dev, unsigned int reg)
{
	struct qcom_pm8xxx_rtc_priv *priv = dev_get_priv(dev);

	return pmic_reg_read(priv->pmic, reg);
}

static int qcom_pm8xxx_rtc_write8(struct udevice *dev, unsigned int reg,
				  int val)
{
	struct qcom_pm8xxx_rtc_priv *priv = dev_get_priv(dev);

	return pmic_reg_write(priv->pmic, reg, val);
}

static int qcom_pm8xxx_rtc_probe(struct udevice *dev)
{
	struct qcom_pm8xxx_rtc_priv *priv = dev_get_priv(dev);

	priv->regs = (const struct qcom_pm8xxx_rtc_regs *)
		dev_get_driver_data(dev);
	if (!priv->regs)
		return -EINVAL;

	priv->pmic = dev_get_parent(dev);
	if (!priv->pmic)
		return -ENODEV;

	return pmic_clrsetbits(priv->pmic, priv->regs->ctrl, 0,
			       PM8XXX_RTC_ENABLE);
}

static const struct rtc_ops qcom_pm8xxx_rtc_ops = {
	.get = qcom_pm8xxx_rtc_get,
	.set = qcom_pm8xxx_rtc_set,
	.reset = qcom_pm8xxx_rtc_reset,
	.read8 = qcom_pm8xxx_rtc_read8,
	.write8 = qcom_pm8xxx_rtc_write8,
};

static const struct udevice_id qcom_pm8xxx_rtc_ids[] = {
	{ .compatible = "qcom,pm8941-rtc", .data = (ulong)&pm8941_regs },
	{ .compatible = "qcom,pmk8350-rtc", .data = (ulong)&pmk8350_regs },
	{ }
};

U_BOOT_DRIVER(qcom_pm8xxx_rtc) = {
	.name = "qcom_pm8xxx_rtc",
	.id = UCLASS_RTC,
	.of_match = qcom_pm8xxx_rtc_ids,
	.probe = qcom_pm8xxx_rtc_probe,
	.ops = &qcom_pm8xxx_rtc_ops,
	.priv_auto = sizeof(struct qcom_pm8xxx_rtc_priv),
};
