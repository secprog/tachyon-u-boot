// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm PMIC-GLINK USB-C alt-mode / DisplayPort client.
 *
 * Parses USBC_PAN alt-mode notifications from the ADSP, programs the SBU mux
 * and QMP combo-PHY Type-C state, and exposes the public altmode API used by
 * the Tachyon DisplayPort driver. Split out of the monolithic pmic_glink.c;
 * operates on the shared struct qpg session owned by the pmic-glink core.
 */

#define LOG_CATEGORY UCLASS_MISC

#include <dm.h>
#include <env.h>
#include <log.h>
#include <time.h>
#include <errno.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <phy/qcom_qmp_combo.h>
#include "pmic_glink_internal.h"

static enum qcom_pmic_glink_orientation qpg_orientation(u8 orientation)
{
	/*
	 * ADSP PAN orientation is USBPD_PIN_ASSIGNMENT_ORIENTATION_*:
	 * 0 = normal/CC1, 1 = flip/CC2, 2 = invalid/open.
	 */
	if (orientation == 0)
		return QCOM_PMIC_GLINK_ORIENTATION_NORMAL;
	if (orientation == 1)
		return QCOM_PMIC_GLINK_ORIENTATION_REVERSE;

	return QCOM_PMIC_GLINK_ORIENTATION_NONE;
}

static const char *qpg_typec_state_name(enum qpg_typec_state state)
{
	switch (state) {
	case QPG_TYPEC_SAFE:
		return "safe";
	case QPG_TYPEC_USB:
		return "usb";
	case QPG_TYPEC_DP:
		return "dp";
	default:
		return "unknown";
	}
}

static enum qcom_pmic_glink_typec_state
qpg_public_typec_state(enum qpg_typec_state state)
{
	switch (state) {
	case QPG_TYPEC_SAFE:
		return QPG_TYPEC_STATE_SAFE;
	case QPG_TYPEC_USB:
		return QPG_TYPEC_STATE_USB;
	case QPG_TYPEC_DP:
		return QPG_TYPEC_STATE_DP;
	default:
		return QPG_TYPEC_STATE_UNKNOWN;
	}
}

static const char *
qpg_public_typec_state_name(enum qcom_pmic_glink_typec_state state)
{
	switch (state) {
	case QPG_TYPEC_STATE_SAFE:
		return "safe";
	case QPG_TYPEC_STATE_USB:
		return "usb";
	case QPG_TYPEC_STATE_DP:
		return "dp";
	default:
		return "unknown";
	}
}

static void qpg_update_cached_state(struct qpg *pg,
				    struct qcom_pmic_glink_altmode *altmode,
				    enum qpg_typec_state state)
{
	if (!altmode)
		return;

	pg->cached_altmode = *altmode;
	pg->cached_altmode_valid = true;

	pg->cached_state.service_started = pg->session_ready;
	pg->cached_state.notify_seen = true;
	pg->cached_state.dp_seen |= state == QPG_TYPEC_DP;
	pg->cached_state.hpd = altmode->hpd;
	pg->cached_state.hpd_irq = altmode->hpd_irq;
	pg->cached_state.port = altmode->port;
	pg->cached_state.orientation_raw = pg->notify.raw_orientation;
	pg->cached_state.orientation = altmode->orientation;
	pg->cached_state.mux = pg->notify.mux;
	pg->cached_state.dpam_raw = pg->notify.dpam;
	pg->cached_state.linux_mux_mode = pg->notify.linux_mux_mode;
	pg->cached_state.dp_pin_assignment = pg->notify.dp_pin_assignment;
	pg->cached_state.pin_assignment = altmode->pin_assignment;
	pg->cached_state.svid = pg->notify.svid;
	pg->cached_state.typec_state = qpg_public_typec_state(state);
	pg->cached_state.last_notify_ms = get_timer(0);

	log_warning("qpg: service started=%u pan=%u notify_seen=%u dp_seen=%u hpd=%u raw_dpam=%u linux_mode=%u dp_pin=%u age_ms=%lu\n",
		    pg->cached_state.service_started,
		    pg->cached_state.pan_enabled,
		    pg->cached_state.notify_seen,
		    pg->cached_state.dp_seen,
		    pg->cached_state.hpd,
		    pg->cached_state.dpam_raw,
		    pg->cached_state.linux_mux_mode,
		    pg->cached_state.dp_pin_assignment,
		    get_timer(pg->cached_state.last_notify_ms));
}

static void qpg_program_sbu_mux(enum qcom_pmic_glink_orientation orientation,
				enum qpg_typec_state state)
{
	bool invert_select = qpg_env_bool("tachyon_dp_invert_sbu_select");
	bool invert_enable = qpg_env_bool("tachyon_dp_invert_sbu_enable");
	struct gpio_desc sbu_enable = {};
	struct gpio_desc sbu_select = {};
	bool dp_active = state == QPG_TYPEC_DP;
	ofnode mux;
	int select;
	int enable;
	int ret;

	mux = ofnode_path("/usb1-sbu-mux");
	if (!ofnode_valid(mux))
		return;

	ret = gpio_request_by_name_nodev(mux, "select-gpios", 0,
					 &sbu_select, GPIOD_IS_OUT);
	if (ret) {
		log_warning("pmic-glink: SBU select request ret=%d\n", ret);
		return;
	}

	ret = gpio_request_by_name_nodev(mux, "enable-gpios", 0,
					 &sbu_enable, GPIOD_IS_OUT);
	if (ret) {
		log_warning("pmic-glink: SBU enable request ret=%d\n", ret);
		dm_gpio_free(NULL, &sbu_select);
		return;
	}

	select = orientation == QCOM_PMIC_GLINK_ORIENTATION_REVERSE;
	if (invert_select)
		select = !select;

	enable = dp_active ? 1 : 0;
	if (invert_enable)
		enable = !enable;

	dm_gpio_set_value(&sbu_select, select);
	udelay(1000);
	dm_gpio_set_value(&sbu_enable, enable);

	log_warning("pmic-glink: SBU mux %s orientation=%u enable=%d select=%d\n",
		    qpg_typec_state_name(state), orientation,
		    dm_gpio_get_value(&sbu_enable),
		    dm_gpio_get_value(&sbu_select));

	dm_gpio_free(NULL, &sbu_enable);
	dm_gpio_free(NULL, &sbu_select);
}

static void qpg_program_qmp_typec(enum qcom_pmic_glink_orientation orientation,
				  enum qpg_typec_state state,
				  u8 pin_assignment)
{
	bool dp_svid = state == QPG_TYPEC_DP;
	bool reverse;
	int ret;

	if (orientation == QCOM_PMIC_GLINK_ORIENTATION_NONE)
		return;

	reverse = orientation == QCOM_PMIC_GLINK_ORIENTATION_REVERSE;
	ret = qcom_qmp_combo_typec_set(reverse, dp_svid, pin_assignment);
	log_warning("pmic-glink: QMP Type-C provider ret=%d state=%s dp_svid=%d orientation=%u pin=%u\n",
		    ret, qpg_typec_state_name(state), dp_svid,
		    orientation, pin_assignment);
}

static void qpg_apply_typec_state(enum qcom_pmic_glink_orientation orientation,
				  enum qpg_typec_state state,
				  u8 pin_assignment)
{
	bool reverse = orientation == QCOM_PMIC_GLINK_ORIENTATION_REVERSE;
	bool sbu_en;
	bool sbu_sel = reverse;

	if (state == QPG_TYPEC_DP && !pin_assignment)
		state = QPG_TYPEC_SAFE;

	qpg_program_sbu_mux(orientation, state);
	qpg_program_qmp_typec(orientation, state, pin_assignment);

	sbu_en = state == QPG_TYPEC_DP;
	log_warning("qpg: apply state=%s sbu_en=%u sbu_sel=%u qmp_mode=%s reverse=%u pin=%u ret=%d\n",
		    qpg_typec_state_name(state), sbu_en, sbu_sel,
		    qpg_typec_state_name(state), reverse, pin_assignment, 0);
}

bool qpg_parse_sc8280xp_notify(struct qpg *pg,
				      struct qcom_pmic_glink_altmode *altmode,
				      const void *data, size_t len,
				      u32 *portp)
{
	const struct qpg_usbc_notify *notify = data;
	enum qcom_pmic_glink_orientation orientation;
	u8 linux_mode;
	u8 mode;
	u8 port;
	u16 svid;

	log_warning("pmic-glink: SC8280XP notify len=%zu expected=%zu\n",
		    len, sizeof(*notify));

	if (len != sizeof(*notify))
		return false;

	port = notify->payload[0];
	*portp = port;
	svid = le32_to_cpu(notify->hdr.opcode) >> 16;
	log_warning("pmic-glink: SC8280XP port=%u orientation=%u mux=%u svid=%04x dpam=%02x hpd=%u irq=%u\n",
		    port, notify->payload[1],
		    notify->payload[2], svid,
		    notify->payload[8] & SC8280XP_DPAM_MASK,
		    !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK),
		    !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK));

	orientation = qpg_orientation(notify->payload[1]);
	pg->notify.seen = true;
	pg->notify.port = port;
	pg->notify.raw_orientation = notify->payload[1];
	pg->notify.orientation = orientation;
	pg->notify.mux = notify->payload[2];
	pg->notify.svid = svid;
	pg->notify.dpam = notify->payload[8] & SC8280XP_DPAM_MASK;
	pg->notify.linux_mux_mode = pg->notify.dpam - DPAM_HPD_A;
	pg->notify.dp_pin_assignment = 0;
	pg->notify.hpd = !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK);
	pg->notify.hpd_irq = !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK);
	log_warning("qpg: notify raw_opcode=%08x svid=%04x port=%u orient_raw=%u orient=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
		    le32_to_cpu(notify->hdr.opcode), svid, port,
		    pg->notify.raw_orientation, orientation, pg->notify.mux,
		    pg->notify.dpam, pg->notify.linux_mux_mode,
		    pg->notify.dp_pin_assignment, pg->notify.hpd,
		    pg->notify.hpd_irq);

	if (svid != USB_TYPEC_DP_SID) {
		altmode->port = port;
		altmode->orientation = orientation;
		altmode->hpd = false;
		altmode->hpd_irq = false;
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_notify_seen = true;
		qpg_apply_typec_state(orientation, QPG_TYPEC_USB, 0);
		qpg_update_cached_state(pg, altmode, QPG_TYPEC_USB);
		return true;
	}

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->hpd = pg->notify.hpd;
	altmode->hpd_irq = pg->notify.hpd_irq;
	pg->altmode_notify_seen = true;
	mode = pg->notify.dpam;
	linux_mode = pg->notify.linux_mux_mode;
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    notify->payload[1], orientation);
	if (linux_mode == 0xff) {
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_apply_typec_state(orientation, QPG_TYPEC_SAFE, 0);
		qpg_update_cached_state(pg, altmode, QPG_TYPEC_SAFE);
		log_warning("pmic-glink: DP notify safe/no-DP mux=%u raw_dpam=%u linux_mode=%u\n",
			    notify->payload[2], mode, linux_mode);
		return true;
	}

	pg->notify.dp_pin_assignment = linux_mode;
	log_warning("pmic-glink: DPAM raw=%u linux_mode=%u dp_pin_assignment=%u\n",
		    mode, linux_mode, pg->notify.dp_pin_assignment);

	altmode->pin_assignment = pg->notify.dp_pin_assignment;
	altmode->dp = true;
	pg->altmode_no_dp = false;
	qpg_apply_typec_state(orientation, QPG_TYPEC_DP,
			      altmode->pin_assignment);
	qpg_update_cached_state(pg, altmode, QPG_TYPEC_DP);

	return true;
}

bool qpg_parse_sc8180x_notify(struct qpg *pg,
				     struct qcom_pmic_glink_altmode *altmode,
				     const void *data, size_t len,
				     u32 *portp)
{
	const struct qpg_usbc_sc8180x_notify *msg = data;
	enum qcom_pmic_glink_orientation orientation;
	u32 notification;
	u8 mode;
	u8 mux;
	u8 raw_orientation;
	u8 port;
	u16 svid;

	log_warning("pmic-glink: SC8180X notify len=%zu expected=%zu\n",
		    len, sizeof(*msg));

	if (len != sizeof(*msg))
		return false;

	notification = le32_to_cpu(msg->notification);
	port = notification & SC8180X_PORT_MASK;
	*portp = port;
	raw_orientation = (notification & SC8180X_ORIENTATION_MASK) >> 8;
	mux = (notification & SC8180X_MUX_MASK) >> 16;
	mode = (notification & SC8180X_MODE_MASK) >> 24;
	svid = mux == 2 ? USB_TYPEC_DP_SID : 0;
	log_warning("pmic-glink: SC8180X notification=%08x port=%u orientation=%u mux=%u mode=%u hpd=%u irq=%u\n",
		    notification, port, raw_orientation, mux, mode,
		    !!(notification & SC8180X_HPD_STATE_MASK),
		    !!(notification & SC8180X_HPD_IRQ_MASK));
	orientation = qpg_orientation(raw_orientation);
	pg->notify.seen = true;
	pg->notify.port = port;
	pg->notify.raw_orientation = raw_orientation;
	pg->notify.orientation = orientation;
	pg->notify.mux = mux;
	pg->notify.svid = svid;
	pg->notify.dpam = mode;
	pg->notify.linux_mux_mode = mode;
	pg->notify.dp_pin_assignment = 0;
	pg->notify.hpd = !!(notification & SC8180X_HPD_STATE_MASK);
	pg->notify.hpd_irq = !!(notification & SC8180X_HPD_IRQ_MASK);
	log_warning("qpg: notify raw_opcode=%08x svid=%04x port=%u orient_raw=%u orient=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
		    le32_to_cpu(msg->hdr.opcode), svid, port,
		    pg->notify.raw_orientation, orientation, pg->notify.mux,
		    pg->notify.dpam, pg->notify.linux_mux_mode,
		    pg->notify.dp_pin_assignment, pg->notify.hpd,
		    pg->notify.hpd_irq);

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->hpd = pg->notify.hpd;
	altmode->hpd_irq = pg->notify.hpd_irq;
	pg->altmode_notify_seen = true;
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    raw_orientation, orientation);
	if (svid != USB_TYPEC_DP_SID) {
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_apply_typec_state(orientation, QPG_TYPEC_USB, 0);
		qpg_update_cached_state(pg, altmode, QPG_TYPEC_USB);
		log_warning("pmic-glink: SC8180X notify USB/no-DP mux=%u mode=%u\n",
			    mux, mode);
		return true;
	}

	if (mode == 0xff) {
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_apply_typec_state(orientation, QPG_TYPEC_SAFE, 0);
		qpg_update_cached_state(pg, altmode, QPG_TYPEC_SAFE);
		log_warning("pmic-glink: SC8180X notify safe/no-DP mux=%u mode=%u\n",
			    mux, mode);
		return true;
	}

	pg->notify.dp_pin_assignment = mode;
	log_warning("pmic-glink: SC8180X DP active linux_mode=%u dp_pin_assignment=%u\n",
		    mode, pg->notify.dp_pin_assignment);

	altmode->pin_assignment = pg->notify.dp_pin_assignment;
	altmode->dp = true;
	pg->altmode_no_dp = false;
	qpg_apply_typec_state(orientation, QPG_TYPEC_DP,
			      altmode->pin_assignment);
	qpg_update_cached_state(pg, altmode, QPG_TYPEC_DP);

	return true;
}

int qpg_send_notify_pan_ack(struct qpg *pg,
				   struct qcom_pmic_glink_altmode *altmode,
				   u32 port)
{
	int ret;

	log_warning("pmic-glink: send ALTMODE_PAN_ACK port=%u\n", port);

	pg->pan_acked = false;
	ret = qpg_send_altmode_req(pg, ALTMODE_PAN_ACK, port);
	log_warning("pmic-glink: send ALTMODE_PAN_ACK ret=%d port=%u\n",
		    ret, port);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, altmode, qpg_done_pan_ack, 1000);
	log_warning("pmic-glink: wait PAN_ACK ret=%d pan_acked=%d\n",
		    ret, pg->pan_acked);
	log_warning("qpg: PAN_ACK state=%s ret=%d\n",
		    qpg_public_typec_state_name(pg->cached_state.typec_state),
		    ret);

	return ret;
}

static void qpg_apply_usbc_pin_assignment(struct qpg *pg,
					  struct qcom_pmic_glink_altmode *altmode,
					  const u8 *pin, const char *source)
{
	enum qcom_pmic_glink_orientation orientation;
	u8 raw_orientation = pin[1];
	u8 mux = pin[2];
	u16 vid = get_unaligned_le16(pin + 4);
	u16 svid_le = get_unaligned_le16(pin + 6);
	u16 svid_be = ((u16)pin[6] << 8) | pin[7];
	u8 mode = pin[8] & SC8280XP_DPAM_MASK;
	u8 linux_mode = mode - DPAM_HPD_A;
	bool hpd = !!(pin[8] & SC8280XP_HPD_STATE_MASK);
	bool hpd_irq = !!(pin[8] & SC8280XP_HPD_IRQ_MASK);
	bool dp_svid = svid_le == USB_TYPEC_DP_SID ||
		       svid_be == USB_TYPEC_DP_SID;
	bool dp_active = mode >= DPAM_HPD_A &&
			 (dp_svid || mux == 2 || mux == 3);
	u8 port = pin[0];

	orientation = qpg_orientation(raw_orientation);
	log_warning("pmic-glink: %s pin port=%u orientation=%u/%u mux=%u vid=%04x svid_le=%04x svid_be=%04x svid_raw=%02x%02x dpam=%02x hpd=%u irq=%u\n",
		    source, port, raw_orientation, orientation, mux, vid,
		    svid_le, svid_be, pin[6], pin[7], mode, hpd, hpd_irq);
	log_warning("qpg: notify raw_opcode=%08x svid=%04x port=%u orient_raw=%u orient=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
		    (u32)(dp_svid ? USB_TYPEC_DP_SID : svid_le) << 16,
		    dp_svid ? USB_TYPEC_DP_SID : svid_le, port,
		    raw_orientation, orientation, mux, mode, linux_mode, 0,
		    hpd, hpd_irq);

	pg->notify.seen = true;
	pg->notify.port = port;
	pg->notify.raw_orientation = raw_orientation;
	pg->notify.orientation = orientation;
	pg->notify.mux = mux;
	pg->notify.svid = dp_svid ? USB_TYPEC_DP_SID : svid_le;
	pg->notify.dpam = mode;
	pg->notify.linux_mux_mode = linux_mode;
	pg->notify.dp_pin_assignment = 0;
	pg->notify.hpd = hpd;
	pg->notify.hpd_irq = hpd_irq;

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->hpd = hpd;
	altmode->hpd_irq = hpd_irq;
	pg->altmode_notify_seen = true;

	if (!dp_active) {
		enum qpg_typec_state state = dp_svid ? QPG_TYPEC_SAFE :
						    QPG_TYPEC_USB;

		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_apply_typec_state(orientation, state, 0);
		qpg_update_cached_state(pg, altmode, state);
		log_warning("pmic-glink: %s %s/no-DP mux=%u dpam=%u dp_svid=%u\n",
			    source, qpg_typec_state_name(state), mux, mode,
			    dp_svid);
		return;
	}

	pg->notify.dp_pin_assignment = linux_mode;
	altmode->pin_assignment = pg->notify.dp_pin_assignment;
	altmode->dp = true;
	pg->altmode_no_dp = false;
	qpg_apply_typec_state(orientation, QPG_TYPEC_DP,
			      altmode->pin_assignment);
	qpg_update_cached_state(pg, altmode, QPG_TYPEC_DP);
	log_warning("pmic-glink: %s DP active raw_dpam=%u linux_mode=%u dp_pin_assignment=%u\n",
		    source, mode, linux_mode, altmode->pin_assignment);
}

static void qpg_log_usbc_read(struct qpg *pg,
			      struct qcom_pmic_glink_altmode *altmode)
{
	const u8 *buf = pg->usbc_read_buffer;
	u32 data_type = get_unaligned_le32(buf);

	log_warning("pmic-glink: USBC READ decoded ret=%u data_type=%u raw=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    pg->usbc_read_return_code, data_type,
		    buf[0], buf[1], buf[2], buf[3],
		    buf[4], buf[5], buf[6], buf[7],
		    buf[8], buf[9], buf[10], buf[11],
		    buf[12], buf[13], buf[14], buf[15]);

	if (data_type == USBC_READ_DATA_PIN_ASSIGNMENT)
		qpg_apply_usbc_pin_assignment(pg, altmode, buf + 4,
					      "USBC READ");
}

static int qpg_send_usbc_read(struct qpg *pg,
			      struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg_usbc_read_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USBC_PAN),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(USBC_CMD_READ_REQ),
	};
	int ret;

	pg->usbc_read_acked = false;
	pg->usbc_read_return_code = 0xffffffff;

	log_warning("pmic-glink: owner=%u channel=%s USBC_READ_REQ\n",
		    PMIC_GLINK_OWNER_USBC_PAN, QPG_CHANNEL_NAME);

	ret = qpg_send_data(pg, &req, sizeof(req));
	log_warning("pmic-glink: send USBC_READ_REQ ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, altmode, qpg_done_usbc_read, 1000);
	log_warning("pmic-glink: wait USBC_READ ret=%d ack=%d\n",
		    ret, pg->usbc_read_acked);
	if (!ret)
		qpg_log_usbc_read(pg, altmode);

	return ret;
}

static int qpg_send_usbc_read_select(struct qpg *pg, u32 read_sel)
{
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	pg->pan_acked = false;
	ret = qpg_send_altmode_req(pg, ALTMODE_READ_SEL, read_sel);
	log_warning("pmic-glink: send READ_SEL ret=%d sel=%u\n", ret, read_sel);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, &altmode, qpg_done_pan_ack, 1000);
	log_warning("pmic-glink: wait READ_SEL_ACK ret=%d pan_acked=%d\n",
		    ret, pg->pan_acked);

	return ret;
}

static int qpg_refresh_usbc_pin_assignment(struct qpg *pg,
					   struct qcom_pmic_glink_altmode *altmode)
{
	int ret;

	ret = qpg_send_usbc_read_select(pg, USBC_READ_SEL_PIN_ASSIGNMENT);
	if (ret)
		return ret;

	return qpg_send_usbc_read(pg, altmode);
}

int qcom_pmic_glink_altmode_start(void)
{
	struct qcom_pmic_glink_altmode altmode = {};
	struct qpg *pg = qpg_session_get();
	int ret;

	if (!pg)
		return -ENODEV;

	log_warning("qpg: service start\n");

	ret = qpg_open_session(&altmode, NULL, NULL, NULL);
	log_warning("qpg: service start ret=%d\n", ret);
	if (ret)
		return ret;

	pg->cached_state.service_started = true;
	pg->cached_state.pan_enabled = true;

	return 0;
}

int qcom_pmic_glink_altmode_poll(struct qcom_pmic_glink_altmode_state *state,
				 uint timeout_ms)
{
	struct qcom_pmic_glink_altmode altmode = {};
	struct qpg *pg = qpg_session_get();
	ulong start;
	bool progressed = false;
	int ret;

	if (!pg || !pg->session_ready)
		return -ENODEV;

	start = get_timer(0);
	for (;;) {
		ret = qpg_poll(pg, &altmode);
		if (!ret) {
			progressed = true;
			continue;
		}
		if (ret != -EAGAIN)
			return ret;

		if (progressed || !timeout_ms)
			break;

		if (get_timer(start) >= timeout_ms) {
			if (state)
				*state = pg->cached_state;
			return pg->cached_state.notify_seen ? 0 : -ETIMEDOUT;
		}

		udelay(1000);
	}

	pg->cached_state.service_started = pg->session_ready;
	if (state)
		*state = pg->cached_state;

	return 0;
}

const struct qcom_pmic_glink_altmode_state *
qcom_pmic_glink_altmode_get_state(void)
{
	static const struct qcom_pmic_glink_altmode_state empty;
	struct qpg *pg = qpg_session_get();

	if (!pg)
		return &empty;

	pg->cached_state.service_started = pg->session_ready;

	return &pg->cached_state;
}

bool qcom_pmic_glink_altmode_hpd_asserted(void)
{
	struct qpg *pg = qpg_session_get();

	if (!pg)
		return false;

	return pg->cached_state.hpd;
}

int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg *pg = qpg_session_get();
	int refresh_ret;
	int ret;

	if (!altmode)
		return -EINVAL;

	if (!pg)
		return -ENODEV;

	memset(altmode, 0, sizeof(*altmode));

	log_warning("pmic-glink: get_altmode start\n");

	ret = qcom_pmic_glink_altmode_start();
	if (ret)
		return ret;

	ret = qcom_pmic_glink_altmode_poll(NULL, 0);
	if (ret && ret != -ETIMEDOUT)
		return ret;

	if (!pg->cached_altmode_valid) {
		refresh_ret = qpg_refresh_usbc_pin_assignment(pg,
							      altmode);
		log_warning("pmic-glink: USBC pin refresh ret=%d dp=%d orientation=%u pin=%u hpd=%d irq=%d\n",
			    refresh_ret, altmode->dp, altmode->orientation,
			    altmode->pin_assignment, altmode->hpd,
			    altmode->hpd_irq);
		if (refresh_ret)
			return refresh_ret;
	}

	if (pg->cached_altmode_valid)
		*altmode = pg->cached_altmode;

	log_warning("pmic-glink: get_altmode ret=0 dp=%d orientation=%u pin=%u hpd=%d irq=%d state=%s\n",
		    altmode->dp, altmode->orientation,
		    altmode->pin_assignment, altmode->hpd,
		    altmode->hpd_irq,
		    qpg_public_typec_state_name(pg->cached_state.typec_state));

	return pg->cached_altmode_valid ? 0 : -EAGAIN;
}

/*
 * E1 (audit): autonomous data-role swap to DFP for the cold DP path.
 *
 * Linux's sink path reaches DFP because the dock (a UFP/hub partner) drives the
 * DR_Swap and we accept it (now that SET_UOR carries ACCEPT_ROLE_SWAPS); if the
 * dock does NOT self-initiate, the host must request DFP itself.  The ADSP only
 * starts DP VDM discovery once DataRole==DFP.  So: if a DFP
 * partner is attached (we are UFP), issue ONE SET_UOR(DFP) and watch for the DP
 * notify — no PPM_RESET / connector-reset churn.  Returns 1 if DP entered.
 *
 * Called from the autonomous DP probe so `tachyon dp start` no longer requires a
 * manual `qpg bounce`.  Gate with tachyon_dp_auto_dfp=0 to keep it pure-passive.
 */
int qcom_pmic_glink_request_dfp(u32 settle_ms)
{
	struct qcom_pmic_glink_altmode altmode = {};
	struct qcom_pmic_glink_altmode_state state = {};
	struct qpg *pg = qpg_session_get();
	ulong start, last_notify_ms = 0;
	int ret;

	if (!pg)
		return -ENODEV;

	ret = qpg_open_session(&altmode, NULL, NULL, NULL);
	if (ret)
		return ret;

	if (!settle_ms)
		settle_ms = 4000;

	/*
	 * Wait for the DisplayPort alt-mode notify. After ALTMODE_PAN_EN (done
	 * in qpg_open_session) the ADSP runs PD + the DisplayPort Enter_Mode VDM
	 * for the attached dock and signals DP via an altmode notify (mux=3 /
	 * dp_seen). That notify takes a few seconds; the old working flow waited
	 * by virtue of the human running `tachyon dp start` a moment after
	 * `qpg ucsi`. So we just poll for it here -- no data-role bounce.
	 *
	 * Deliberately NOT gated on UCSI CAM_SUPPORTED / GET_ALTERNATE_MODE: this
	 * platform's ADSP firmware hardwires CAM_SUPPORTED=0 (its alt-mode table
	 * has the DP entry compiled out), so those reads are always 0 and the
	 * PAN notify is the only real DP-entry signal.
	 */
	start = get_timer(0);
	while (get_timer(start) < settle_ms) {
		ret = qcom_pmic_glink_altmode_poll(&state, 20);
		if (ret && ret != -ETIMEDOUT)
			break;
		if (state.notify_seen && state.last_notify_ms != last_notify_ms) {
			last_notify_ms = state.last_notify_ms;
			log_warning("qpg: DP wait notify mux=%u dpam=%02x hpd=%u dp_seen=%u svid=%04x\n",
				    state.mux, state.dpam_raw, state.hpd,
				    state.dp_seen, state.svid);
		}
		if (state.dp_seen || state.mux == 3) {
			log_warning("qpg: DP entered (mux=%u hpd=%u)\n",
				    state.mux, state.hpd);
			return 1;
		}
	}

	log_warning("qpg: DP wait timed out dp_seen=%u mux=%u\n",
		    state.dp_seen, state.mux);
	return 0;
}

/* ------------------------------------------------------------------------- *
 * Driver model: PMIC-GLINK core + client child devices.
 *
 * The core driver owns the GLINK transport and binds to the "qcom,pmic-glink"
 * DT node. Its client children (altmode / ucsi / battmgr) are created in code
 * here -- mirroring Linux's pmic_glink auxiliary devices -- rather than from
 * DT subnodes; each reaches the transport through this parent. Probe is kept
 * cheap: the heavy GLINK/ADSP bringup stays lazy (driven by the Type-C/DP path
 * via the public API), so binding at boot does not perturb boot timing.
 * ------------------------------------------------------------------------- */


/* Alt-mode / DisplayPort client child device (bound by name from the core). */
U_BOOT_DRIVER(qcom_pmic_glink_altmode) = {
	.name	= "qcom_pmic_glink_altmode",
	.id	= UCLASS_MISC,
};
