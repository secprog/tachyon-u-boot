// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm PMIC-GLINK battery-manager client (optional).
 *
 * Registers the CHARGER (owner 32778) client over the GLINK transport owned
 * by the pmic-glink core, mirroring Linux qcom_battmgr. Not required for the
 * USB-C/DisplayPort path; gated by CONFIG_QCOM_PMIC_GLINK_BATTMGR.
 */

#define LOG_CATEGORY UCLASS_MISC

#include <dm.h>
#include <log.h>
#include <errno.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include "pmic_glink_internal.h"

static bool qpg_done_battmgr(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode)
{
	return pg->battmgr_acked;
}

/*
 * Register the battery-manager (CHARGER, owner 32778) client over the same
 * GLINK channel: SET_OPERATIONAL_MODE(normal) then SET_NOTIFICATION_CRITERIA.
 * On this platform the ADSP "battman" firmware hosts both the charger AND the
 * USB Type-C/PD/alt-mode stack; this mirrors what Linux's qcom_battmgr does on
 * glink-up. Best-effort: failures here must not break the altmode session.
 */
int qpg_register_battmgr(struct qpg *pg)
{
	struct qcom_pmic_glink_altmode altmode = {};
	struct qpg_battmgr_opmode_req opmode = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_CHARGER),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(BATT_MNGR_SET_OPERATIONAL_MODE_REQ),
		.operational_mode = cpu_to_le32(1), /* normal mode */
	};
	struct qpg_battmgr_notify_crit_req crit = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_CHARGER),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(BATT_MNGR_SET_NOTIFICATION_CRITERIA_REQ),
		.battery_id = 0,
		.power_state = cpu_to_le32(0xf),
		.low_capacity = 0,
		.high_capacity = cpu_to_le32(100),
	};
	int ret;

	log_debug("pmic-glink: BATTMGR register: SET_OPERATIONAL_MODE\n");
	pg->battmgr_acked = false;
	ret = qpg_send_data(pg, &opmode, sizeof(opmode));
	if (ret) {
		log_warning("pmic-glink: BATTMGR opmode send ret=%d\n", ret);
		return ret;
	}
	ret = qpg_drain_until(pg, &altmode, qpg_done_battmgr, 1000);
	log_debug("pmic-glink: BATTMGR opmode ack ret=%d acked=%d\n",
		    ret, pg->battmgr_acked);

	log_debug("pmic-glink: BATTMGR register: SET_NOTIFICATION_CRITERIA\n");
	pg->battmgr_acked = false;
	ret = qpg_send_data(pg, &crit, sizeof(crit));
	if (ret) {
		log_warning("pmic-glink: BATTMGR crit send ret=%d\n", ret);
		return ret;
	}
	ret = qpg_drain_until(pg, &altmode, qpg_done_battmgr, 1000);
	log_debug("pmic-glink: BATTMGR crit ack ret=%d acked=%d\n",
		    ret, pg->battmgr_acked);

	return 0;
}

/* Battery-manager client child device (bound by name from the core). */
U_BOOT_DRIVER(qcom_pmic_glink_battmgr) = {
	.name	= "qcom_pmic_glink_battmgr",
	.id	= UCLASS_MISC,
};
