/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Minimal Qualcomm PMIC-GLINK helpers for boot-time Type-C policy.
 */

#ifndef __SOC_QCOM_PMIC_GLINK_H__
#define __SOC_QCOM_PMIC_GLINK_H__

#include <linux/types.h>
#include <linux/kconfig.h>

enum qcom_pmic_glink_orientation {
	QCOM_PMIC_GLINK_ORIENTATION_NONE,
	QCOM_PMIC_GLINK_ORIENTATION_NORMAL,
	QCOM_PMIC_GLINK_ORIENTATION_REVERSE,
};

struct qcom_pmic_glink_altmode {
	bool dp;
	bool hpd;
	bool hpd_irq;
	enum qcom_pmic_glink_orientation orientation;
	u8 port;
	u8 pin_assignment;
};

#if IS_ENABLED(CONFIG_QCOM_PMIC_GLINK)
int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode);
#else
static inline int
qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	return -ENOSYS;
}
#endif

#endif
