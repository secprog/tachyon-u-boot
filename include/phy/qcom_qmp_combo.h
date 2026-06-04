/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm QMP USB3/DP combo PHY helpers.
 */

#ifndef __PHY_QCOM_QMP_COMBO_H__
#define __PHY_QCOM_QMP_COMBO_H__

#include <errno.h>
#include <linux/kconfig.h>
#include <linux/types.h>

#if IS_ENABLED(CONFIG_PHY_QCOM_QMP_COMBO)
int qcom_qmp_combo_typec_set(bool reverse, bool dp_svid, u8 pin_assignment);
#else
static inline int qcom_qmp_combo_typec_set(bool reverse, bool dp_svid,
					   u8 pin_assignment)
{
	return -ENOSYS;
}
#endif

#endif
