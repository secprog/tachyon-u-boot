// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm PMIC-GLINK UCSI client.
 *
 * Sends UCSI/PPM commands over the GLINK transport owned by the pmic-glink
 * core (PMIC_RTR_ADSP_APPS channel, USB_TYPE_C owner). Split out of the
 * monolithic pmic_glink.c; operates on the shared struct qpg session.
 */

#define LOG_CATEGORY UCLASS_MISC

#include <dm.h>
#include <log.h>
#include <time.h>
#include <errno.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include "pmic_glink_internal.h"

static int qpg_send_ucsi_read(struct qpg *pg)
{
	struct qpg_ucsi_read_buffer_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USB_TYPE_C),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(UCSI_READ_BUFFER_REQ),
	};
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	pg->ucsi_read_acked = false;

	log_debug("pmic-glink: owner=%u channel=%s UCSI_READ_BUFFER_REQ\n",
		    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME);

	ret = qpg_send_data(pg, &req, sizeof(req));
	log_debug("pmic-glink: send UCSI_READ_BUFFER_REQ ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, &altmode, qpg_done_ucsi_read, 1000);
	log_debug("pmic-glink: wait UCSI_READ_BUFFER ret=%d ack=%d\n",
		    ret, pg->ucsi_read_acked);

	return ret;
}

static void qpg_log_ucsi_raw(struct qpg *pg, const char *label)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 cci = get_unaligned_le32(buf + 4);
	u8 len = (cci & UCSI_CCI_DATA_LENGTH_MASK) >> UCSI_CCI_DATA_LENGTH_SHIFT;

	log_debug("pmic-glink: UCSI %s ret=%u cci=%08x len=%u raw=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    label, pg->ucsi_read_return_code, cci, len,
		    buf[16], buf[17], buf[18], buf[19],
		    buf[20], buf[21], buf[22], buf[23],
		    buf[24], buf[25], buf[26], buf[27],
		    buf[28], buf[29], buf[30], buf[31]);
}

static void qpg_log_ucsi_connector_status(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 cci = get_unaligned_le32(buf + 4);
	u32 status = get_unaligned_le32(buf + 16);
	u32 rdo = get_unaligned_le32(buf + 20);
	u8 power_opmode = (status >> 16) & 0x7;
	bool connected = status & BIT(19);
	bool power_direction = status & BIT(20);
	bool partner_usb = status & BIT(21);
	bool partner_altmode = status & BIT(22);
	u8 partner_type = (status >> 29) & 0x7;

	log_debug("pmic-glink: UCSI connector%u ret=%u cci=%08x status=%08x connected=%u pwr_dir=%u usb=%u altmode=%u partner=%u opmode=%u rdo=%08x\n",
		    port + 1, pg->ucsi_read_return_code, cci, status,
		    connected, power_direction, partner_usb,
		    partner_altmode, partner_type, power_opmode, rdo);
	qpg_log_ucsi_raw(pg, "CONNECTOR_STATUS");
}

static void qpg_log_ucsi_capability(struct qpg *pg)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 attr = get_unaligned_le32(buf + 16);
	u64 cap = get_unaligned_le64(buf + 16);
	u8 connectors = (cap >> 32) & 0x7f;
	u32 optional = (cap >> 40) & 0xffffff;
	u8 altmodes = buf[24];

	log_debug("pmic-glink: UCSI CAPABILITY attr=%08x connectors=%u optional=%06x altmodes=%u\n",
		    attr, connectors, optional, altmodes);
	qpg_log_ucsi_raw(pg, "CAPABILITY");
}

static void qpg_log_ucsi_connector_capability(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u8 opmode = buf[16];
	bool provider = buf[17] & BIT(0);
	bool consumer = buf[17] & BIT(1);

	log_debug("pmic-glink: UCSI connector%u CAP opmode=%02x altmode=%u usb3=%u usb2=%u drp=%u provider=%u consumer=%u\n",
		    port + 1, opmode, !!(opmode & BIT(7)), !!(opmode & BIT(6)),
		    !!(opmode & BIT(5)), !!(opmode & BIT(2)), provider,
		    consumer);
	qpg_log_ucsi_raw(pg, "CONNECTOR_CAP");
}

static int qpg_send_ucsi_write(struct qpg *pg, const u8 *write_buffer)
{
	struct qpg_ucsi_write_buffer_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USB_TYPE_C),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(UCSI_WRITE_BUFFER_REQ),
	};
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	memcpy(req.write_buffer, write_buffer, UCSI_BUFFER_SIZE);
	pg->ucsi_write_acked = false;
	pg->ucsi_write_return_code = 0xffffffff;

	if (req.write_buffer[8] == UCSI_CMD_SET_NOTIFICATION_ENABLE) {
		log_debug("pmic-glink: owner=%u channel=%s UCSI_WRITE_BUFFER_REQ cmd=%u notify_mask=%04x\n",
			    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME,
			    req.write_buffer[8],
			    get_unaligned_le16(req.write_buffer + 10));
	} else {
		log_debug("pmic-glink: owner=%u channel=%s UCSI_WRITE_BUFFER_REQ cmd=%u b10=%u b11=%u b12=%u b13=%u\n",
			    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME,
			    req.write_buffer[8], req.write_buffer[10],
			    req.write_buffer[11], req.write_buffer[12],
			    req.write_buffer[13]);
	}

	ret = qpg_send_data(pg, &req, sizeof(req));
	log_debug("pmic-glink: send UCSI_WRITE_BUFFER_REQ ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, &altmode, qpg_done_ucsi_write, 1000);
	log_debug("pmic-glink: wait UCSI_WRITE_BUFFER ret=%d ack=%d code=%u\n",
		    ret, pg->ucsi_write_acked, pg->ucsi_write_return_code);

	return ret;
}

static u32 qpg_ucsi_cci(struct qpg *pg)
{
	return get_unaligned_le32(pg->ucsi_read_buffer + 4);
}

static u16 qpg_ucsi_version(struct qpg *pg)
{
	return get_unaligned_le16(pg->ucsi_read_buffer + 0);
}

static int qpg_ucsi_send_control(struct qpg *pg, u64 control)
{
	u8 write_buffer[UCSI_BUFFER_SIZE] = {};

	put_unaligned_le64(control, write_buffer + 8);
	return qpg_send_ucsi_write(pg, write_buffer);
}

static u8 qpg_ucsi_connector_change(struct qpg *pg)
{
	return (qpg_ucsi_cci(pg) & UCSI_CCI_CONNECTOR_CHANGE_MASK) >>
	       UCSI_CCI_CONNECTOR_CHANGE_SHIFT;
}

static int qpg_wait_ucsi_cci(struct qpg *pg, u32 old_cci, u32 timeout_ms)
{
	ulong start = get_timer(0);
	u32 cci;
	int ret;

	do {
		ret = qpg_send_ucsi_read(pg);
		if (ret)
			return ret;

		cci = qpg_ucsi_cci(pg);
		if (cci != old_cci &&
		    (cci & (UCSI_CCI_COMMAND_COMPLETE |
			    UCSI_CCI_ERROR |
			    UCSI_CCI_NOT_SUPPORTED |
			    UCSI_CCI_RESET_COMPLETE))) {
			log_debug("pmic-glink: UCSI command CCI ready cci=%08x notify_seen=%d notify=%08x\n",
				    cci, pg->ucsi_notify_seen,
				    pg->ucsi_notification);
			return 0;
		}

		mdelay(20);
	} while (get_timer(start) < timeout_ms);

	cci = qpg_ucsi_cci(pg);
	log_warning("pmic-glink: UCSI command CCI timeout cci=%08x notify_seen=%d notify=%08x\n",
		    cci, pg->ucsi_notify_seen, pg->ucsi_notification);

	return -ETIMEDOUT;
}

static int qpg_send_ucsi_ack_cc_ci(struct qpg *pg, bool connector_change,
				   bool command_complete)
{
	u16 d2 = 0;
	ulong start;
	u32 cci;
	int ret;

	if (connector_change)
		d2 |= UCSI_ACK_CC_CI_CONNECTOR_CHANGE;
	if (command_complete)
		d2 |= UCSI_ACK_CC_CI_COMMAND_COMPLETE;

	ret = qpg_ucsi_send_control(pg,
		UCSI_CTRL_D2(UCSI_CMD_ACK_CC_CI, d2));
	log_debug("pmic-glink: UCSI ACK_CC_CI ret=%d connector=%d command=%d\n",
		    ret, connector_change, command_complete);
	if (ret)
		return ret;

	/*
	 * Linux parity: block until ACK_COMPLETE appears in CCI.
	 * ucsi_acknowledge() in Linux uses sync_control which waits
	 * for command completion.  Use 5000 ms.
	 */
	start = get_timer(0);
	do {
		mdelay(20);
		if (get_timer(start) >= 5000)
			break;
		ret = qpg_send_ucsi_read(pg);
		if (ret)
			return ret;
		cci = qpg_ucsi_cci(pg);
		if (cci & UCSI_CCI_ACK_COMPLETE) {
			log_debug("pmic-glink: UCSI ACK_CC_CI complete cci=%08x\n",
				    cci);
			return 0;
		}
	} while (1);

	log_warning("pmic-glink: UCSI ACK_CC_CI timeout cci=%08x\n",
		    qpg_ucsi_cci(pg));
	return -ETIMEDOUT;
}

static int qpg_send_ucsi_command(struct qpg *pg, u8 command, u8 port,
				 u16 arg16, bool ack)
{
	u64 control = UCSI_CTRL_CMD(command);
	u32 old_cci = qpg_ucsi_cci(pg);
	int ret;

	if (command == UCSI_CMD_GET_CONNECTOR_CAPABILITY ||
	    command == UCSI_CMD_GET_CONNECTOR_STATUS)
		control |= (u64)(port + 1) << 16;
	else if (command == UCSI_CMD_GET_CAM_SUPPORTED ||
		 command == UCSI_CMD_GET_CURRENT_CAM)
		control |= (u64)(port + 1) << 16;
	else if (command == UCSI_CMD_SET_NOTIFICATION_ENABLE)
		control |= (u64)arg16 << 16;

	pg->ucsi_notify_seen = false;
	pg->ucsi_notification = 0;

	ret = qpg_ucsi_send_control(pg, control);
	if (ret)
		return ret;

	ret = qpg_wait_ucsi_cci(pg, old_cci, 5000);
	if (ret)
		return ret;

	if (ack) {
		ret = qpg_send_ucsi_ack_cc_ci(pg,
					      qpg_ucsi_connector_change(pg),
					      true);
		if (ret)
			return ret;
	}

	return ret;
}

static int qpg_send_ucsi_ppm_reset(struct qpg *pg)
{
	ulong start;
	u32 cci;
	int ret;

	/*
	 * Linux parity: if RESET_COMPLETE is already set, clear stale
	 * state by sending SET_NOTIFICATION_ENABLE with no mask before
	 * issuing a fresh reset (ucsi_reset_ppm() in Linux).
	 */
	ret = qpg_send_ucsi_read(pg);
	if (ret)
		return ret;
	cci = qpg_ucsi_cci(pg);
	if (cci & UCSI_CCI_RESET_COMPLETE) {
		ret = qpg_ucsi_send_control(pg,
			UCSI_CTRL_CMD(UCSI_CMD_SET_NOTIFICATION_ENABLE));
		if (ret)
			return ret;

		start = get_timer(0);
		do {
			mdelay(20);
			ret = qpg_send_ucsi_read(pg);
			if (ret)
				return ret;
			cci = qpg_ucsi_cci(pg);
			if (cci & UCSI_CCI_COMMAND_COMPLETE)
				break;
		} while (get_timer(start) < 10000);

		if (!(cci & UCSI_CCI_COMMAND_COMPLETE)) {
			log_warning("pmic-glink: UCSI stale-reset-clear timeout cci=%08x\n",
				    cci);
			return -ETIMEDOUT;
		}
		/* ACK the completion, then proceed to new reset */
		ret = qpg_ucsi_send_control(pg,
			UCSI_CTRL_D2(UCSI_CMD_ACK_CC_CI,
				     UCSI_ACK_CC_CI_COMMAND_COMPLETE));
		if (ret)
			return ret;
	}

	/* Issue PPM_RESET */
	ret = qpg_ucsi_send_control(pg, UCSI_CTRL_CMD(UCSI_CMD_PPM_RESET));
	if (ret)
		return ret;

	/*
	 * Linux-aligned: 10000 ms timeout, 20 ms poll interval.
	 * Does NOT require cci != old_cci — only waits for RESET_COMPLETE.
	 * If CCI has non-reset bits while pending, reissue PPM_RESET
	 * (ucsi_reset_ppm() in Linux reissues on spurious CCI).
	 */
	start = get_timer(0);
	for (;;) {
		mdelay(20);
		ret = qpg_send_ucsi_read(pg);
		if (ret)
			return ret;
		cci = qpg_ucsi_cci(pg);

		if (cci & UCSI_CCI_RESET_COMPLETE) {
			log_debug("pmic-glink: UCSI PPM_RESET complete cci=%08x\n",
				    cci);
			return 0;
		}

		/* CCI has non-reset bits — reissue PPM_RESET (Linux parity) */
		if (cci & ~UCSI_CCI_RESET_COMPLETE) {
			log_warning("pmic-glink: UCSI PPM_RESET reissue cci=%08x\n",
				    cci);
			ret = qpg_ucsi_send_control(pg,
				UCSI_CTRL_CMD(UCSI_CMD_PPM_RESET));
			if (ret)
				return ret;
		}

		if (get_timer(start) >= 10000) {
			log_warning("pmic-glink: UCSI PPM_RESET timeout cci=%08x\n",
				    cci);
			return -ETIMEDOUT;
		}
	}
}

static int qpg_send_ucsi_get_capability(struct qpg *pg)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CAPABILITY, 0, 0, true);
	if (!ret)
		qpg_log_ucsi_capability(pg);

	return ret;
}

static int qpg_send_ucsi_get_connector_capability(struct qpg *pg, u8 port)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CONNECTOR_CAPABILITY,
				    port, 0, true);
	if (!ret)
		qpg_log_ucsi_connector_capability(pg, port);

	return ret;
}

static int qpg_send_ucsi_get_connector_status(struct qpg *pg, u8 port)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CONNECTOR_STATUS, port,
				    0, true);
	if (!ret)
		qpg_log_ucsi_connector_status(pg, port);

	return ret;
}

static int qpg_enable_ucsi_notifications_phase2(struct qpg *pg)
{
	log_debug("pmic-glink: UCSI SET_NOTIFICATION_ENABLE phase2 mask=%04x\n",
		    QPG_UCSI_NTFY_ALL);
	return qpg_send_ucsi_command(pg, UCSI_CMD_SET_NOTIFICATION_ENABLE, 0,
				     QPG_UCSI_NTFY_ALL, true);
}

static int qpg_enable_ucsi_notifications(struct qpg *pg)
{
	u16 phase1_mask;

	phase1_mask = QPG_UCSI_NTFY_CMD_COMPLETE | QPG_UCSI_NTFY_ERROR;
	log_debug("pmic-glink: UCSI SET_NOTIFICATION_ENABLE phase1 mask=%04x\n",
		    phase1_mask);
	return qpg_send_ucsi_command(pg, UCSI_CMD_SET_NOTIFICATION_ENABLE, 0,
				     phase1_mask, true);
}

int qpg_ucsi_prewarm(struct qpg *pg)
{
	u16 version;
	int ret;

	if (pg->ucsi_prewarmed)
		return 0;

	if (qpg_env_bool("qpg_skip_ucsi_prewarm")) {
		log_debug("pmic-glink: UCSI prewarm skipped by env\n");
		return 0;
	}

	log_debug("pmic-glink: UCSI prewarm begin\n");

	/* Read UCSI version first (Linux parity: ucsi_register does this) */
	ret = qpg_send_ucsi_read(pg);
	if (!ret) {
		version = qpg_ucsi_version(pg);
		log_debug("pmic-glink: UCSI version = 0x%04x\n", version);
	}

	ret = qpg_send_ucsi_ppm_reset(pg);
	log_debug("pmic-glink: UCSI prewarm PPM_RESET ret=%d\n", ret);
	if (ret) {
		/*
		 * Non-fatal during bring-up: continue with diagnostic UCSI
		 * init so we can see whether later commands succeed after
		 * a reset timeout.
		 */
		log_warning("pmic-glink: PPM_RESET failed, continuing diagnostic UCSI init\n");
	}

	ret = qpg_enable_ucsi_notifications(pg);
	log_debug("pmic-glink: UCSI prewarm notifications phase1 ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_capability(pg);
	log_debug("pmic-glink: UCSI prewarm capability ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_capability(pg, 0);
	log_debug("pmic-glink: UCSI prewarm connector capability ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_status(pg, 0);
	log_debug("pmic-glink: UCSI prewarm connector status ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_enable_ucsi_notifications_phase2(pg);
	log_debug("pmic-glink: UCSI prewarm notifications phase2 ret=%d\n",
		    ret);
	if (ret)
		return ret;

	pg->ucsi_prewarmed = true;
	log_debug("pmic-glink: UCSI prewarm complete\n");

	return 0;
}

static void qpg_log_ucsi_cam_supported(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 bitmap = get_unaligned_le32(buf + 16);

	log_debug("pmic-glink: UCSI connector%u CAM_SUPPORTED bitmap=%08x\n",
		    port + 1, bitmap);
	qpg_log_ucsi_raw(pg, "CAM_SUPPORTED");
}

static void qpg_log_ucsi_current_cam(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;

	log_debug("pmic-glink: UCSI connector%u CURRENT_CAM=%02x\n",
		    port + 1, buf[16]);
	qpg_log_ucsi_raw(pg, "CURRENT_CAM");
}

static void qpg_log_ucsi_alternate_mode(struct qpg *pg, u8 port, u8 offset)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 cci = get_unaligned_le32(buf + 4);
	u8 len = (cci & UCSI_CCI_DATA_LENGTH_MASK) >> UCSI_CCI_DATA_LENGTH_SHIFT;
	u16 svid0 = get_unaligned_le16(buf + 16);
	u32 mid0 = get_unaligned_le32(buf + 18);
	u16 svid1 = get_unaligned_le16(buf + 22);
	u32 mid1 = get_unaligned_le32(buf + 24);

	log_debug("pmic-glink: UCSI connector%u ALT_MODE off=%u len=%u mode0=svid:%04x mid:%08x mode1=svid:%04x mid:%08x\n",
		    port + 1, offset, len, svid0, mid0, svid1, mid1);
	qpg_log_ucsi_raw(pg, "ALT_MODE");
}

static int qpg_send_ucsi_get_cam_supported(struct qpg *pg, u8 port)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CAM_SUPPORTED, port,
				    0, true);
	if (!ret)
		qpg_log_ucsi_cam_supported(pg, port);

	return ret;
}

static int qpg_send_ucsi_get_current_cam(struct qpg *pg, u8 port)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CURRENT_CAM, port,
				    0, true);
	if (!ret)
		qpg_log_ucsi_current_cam(pg, port);

	return ret;
}

static int qpg_send_ucsi_get_alternate_mode(struct qpg *pg, u8 port,
					    u8 offset, u8 count)
{
	/*
	 * Byte layout matches the original ad-hoc write:
	 *   byte 10 = recipient (connector=0)
	 *   byte 11 = connector number (port+1)
	 *   byte 12 = alternate mode offset
	 *   byte 13 = number of alternate modes - 1
	 * d2 bits 16-23 = byte 10, bits 24-31 = byte 11
	 * d4 bits 32-39 = byte 12, bits 40-47 = byte 13
	 */
	u16 d2 = (u16)(port + 1) << 8;
	u32 d4 = (u32)offset | ((u32)(count ? count - 1 : 0) << 8);
	u64 control = UCSI_CTRL_D2_D4(UCSI_CMD_GET_ALTERNATE_MODE, d2, d4);
	u32 old_cci = qpg_ucsi_cci(pg);
	int ret;

	pg->ucsi_notify_seen = false;
	pg->ucsi_notification = 0;

	ret = qpg_ucsi_send_control(pg, control);
	if (ret)
		return ret;

	ret = qpg_wait_ucsi_cci(pg, old_cci, 5000);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_ack_cc_ci(pg, qpg_ucsi_connector_change(pg), true);
	if (ret)
		return ret;

	qpg_log_ucsi_alternate_mode(pg, port, offset);

	return 0;
}

/*
 * Second, post-PAN UCSI sequence — the exact sequence the manual `qpg ucsi`
 * command ran AFTER qpg_open_session() (i.e. after prewarm + PAN_EN). Running
 * this once is what actually drives the ADSP to enter DisplayPort alt mode;
 * the pre-PAN prewarm alone is not enough. Mirrors the old do_qpg_ucsi:
 * PPM_RESET + notifications + capability + connector reads + CAM/alt-mode
 * reads, then notifications phase 2. Best-effort — a hiccup here does not tear
 * the session down (the caller logs the return and keeps the open session).
 */
int qpg_ucsi_discover(struct qpg *pg)
{
	int ret;

	log_debug("pmic-glink: UCSI discover begin (post-PAN)\n");

	ret = qpg_send_ucsi_ppm_reset(pg);
	log_debug("pmic-glink: UCSI discover PPM_RESET ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_enable_ucsi_notifications(pg);
	log_debug("pmic-glink: UCSI discover notifications ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_capability(pg);
	log_debug("pmic-glink: UCSI discover capability ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_capability(pg, 0);
	log_debug("pmic-glink: UCSI discover connector capability ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_status(pg, 0);
	log_debug("pmic-glink: UCSI discover connector status ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_cam_supported(pg, 0);
	log_debug("pmic-glink: UCSI discover CAM_SUPPORTED ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_current_cam(pg, 0);
	log_debug("pmic-glink: UCSI discover CURRENT_CAM ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_alternate_mode(pg, 0, 0, 2);
	log_debug("pmic-glink: UCSI discover ALT_MODE ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_enable_ucsi_notifications_phase2(pg);
	log_debug("pmic-glink: UCSI discover notifications phase2 ret=%d\n",
		    ret);

	return ret;
}

/* UCSI client child device (bound by name from the pmic-glink core). */
U_BOOT_DRIVER(qcom_pmic_glink_ucsi) = {
	.name	= "qcom_pmic_glink_ucsi",
	.id	= UCLASS_MISC,
};
