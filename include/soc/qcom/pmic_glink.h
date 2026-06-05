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

enum qcom_pmic_glink_typec_state {
	QPG_TYPEC_STATE_UNKNOWN = 0,
	QPG_TYPEC_STATE_SAFE,
	QPG_TYPEC_STATE_USB,
	QPG_TYPEC_STATE_DP,
};

struct qcom_pmic_glink_altmode_state {
	bool service_started;
	bool pan_enabled;
	bool notify_seen;
	bool dp_seen;
	bool hpd;
	bool hpd_irq;

	u8 port;
	u8 orientation_raw;
	u8 orientation;
	u8 mux;
	u8 dpam_raw;
	u8 linux_mux_mode;
	u8 dp_pin_assignment;
	u8 pin_assignment;
	u16 svid;

	enum qcom_pmic_glink_typec_state typec_state;

	ulong last_notify_ms;
};

#if IS_ENABLED(CONFIG_QCOM_PMIC_GLINK)
int qcom_pmic_glink_altmode_start(void);
int qcom_pmic_glink_altmode_poll(struct qcom_pmic_glink_altmode_state *state,
				 uint timeout_ms);
const struct qcom_pmic_glink_altmode_state *
qcom_pmic_glink_altmode_get_state(void);
bool qcom_pmic_glink_altmode_hpd_asserted(void);
int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode);
#else
static inline int qcom_pmic_glink_altmode_start(void)
{
	return -ENOSYS;
}

static inline int
qcom_pmic_glink_altmode_poll(struct qcom_pmic_glink_altmode_state *state,
			     uint timeout_ms)
{
	return -ENOSYS;
}

static inline const struct qcom_pmic_glink_altmode_state *
qcom_pmic_glink_altmode_get_state(void)
{
	return NULL;
}

static inline bool qcom_pmic_glink_altmode_hpd_asserted(void)
{
	return false;
}

static inline int
qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	return -ENOSYS;
}
#endif

#endif
