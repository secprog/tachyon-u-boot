// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal Qualcomm PMIC-GLINK transport for boot-time USB-C DP policy.
 *
 * This is intentionally small and synchronous. It brings up the GLINK SMEM
 * edge to LPASS/ADSP, opens PMIC_RTR_ADSP_APPS, enables altmode
 * notifications, and returns the first DP notification needed by display
 * bring-up.
 */

#define LOG_CATEGORY UCLASS_MISC

#include <asm/gpio.h>
#include <command.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/uclass.h>
#include <env.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <log.h>
#include <malloc.h>
#include <mailbox.h>
#include <mailbox-uclass.h>
#include <mapmem.h>
#include <phy/qcom_qmp_combo.h>
#include <smem.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <soc/qcom/pmic_glink.h>
#include <asm/unaligned.h>
#include <time.h>

#define QPG_SMEM_XPRT_DESCRIPTOR		478
#define QPG_SMEM_XPRT_FIFO_0			479
#define QPG_SMEM_XPRT_FIFO_1			480

#define QPG_FIFO_FULL_RESERVE			8
#define QPG_TX_BLOCKED_CMD_RESERVE		8
#define QPG_RX_INTENT_SIZE			512
#define QPG_CHANNEL_NAME			"PMIC_RTR_ADSP_APPS"
#define QPG_ALTMODE_TIMEOUT_MS			5000

/*
 * IPCRTR is the qrtr/QMI transport GLINK channel the ADSP opens at boot; the
 * SERVREG_LOC protection-domain service runs over it. The ADSP's charger_pd
 * appears to need this transport (servreg registration) up before it will run
 * the DisplayPort alt-mode VDM. We bring the channel up as a second GLINK
 * channel and (Step 1) log the qrtr packets the ADSP sends so we can implement
 * a minimal qrtr/servreg responder.
 */
#define QPG_IPCRTR_NAME				"IPCRTR"
#define QPG_IPCRTR_LCID				2
#define QPG_IPCRTR_INTENT_SIZE			4096

/* qrtr (QIPCRTR) protocol — see Linux include/uapi/linux/qrtr.h */
#define QRTR_PROTO_VER_1			1
#define QRTR_TYPE_DATA				1
#define QRTR_TYPE_HELLO				2
#define QRTR_TYPE_BYE				3
#define QRTR_TYPE_NEW_SERVER			4
#define QRTR_TYPE_DEL_SERVER			5
#define QRTR_TYPE_NEW_LOOKUP			10
#define QRTR_TYPE_DEL_LOOKUP			11
#define QRTR_NODE_BCAST				0xffffffffu
#define QRTR_PORT_CTRL				0xfffffffeu
/* Local (apps) qrtr node id we present to the ADSP. */
#define QPG_QRTR_LOCAL_NODE			1

struct qpg_qrtr_hdr {
	__le32 version;
	__le32 type;
	__le32 src_node;
	__le32 src_port;
	__le32 confirm_rx;
	__le32 size;
	__le32 dst_node;
	__le32 dst_port;
} __packed;

struct qpg_qrtr_ctrl {
	__le32 cmd;
	__le32 service;
	__le32 instance;
	__le32 node;
	__le32 port;
} __packed;

/*
 * servreg (protection-domain) over QMI — see Linux drivers/soc/qcom/pdr*.
 * The ADSP charger_pd hosts a NOTIFIER service (QMI service 0x42). To bring it
 * fully up (and let it run the DP alt-mode VDM) the AP must REGISTER_LISTENER
 * for "msm/adsp/charger_pd" with that notifier, then SET_ACK its state
 * indication — exactly what the Linux kernel pdr client does.
 */
#define QMI_TYPE_REQUEST			0
#define QMI_TYPE_RESPONSE			2
#define QMI_TYPE_INDICATION			4
#define SERVREG_NOTIFIER_QMI_SVC		0x42
#define SERVREG_REGISTER_LISTENER_REQ		0x20
#define SERVREG_STATE_UPDATED_IND		0x22
#define SERVREG_SET_ACK_REQ			0x23
#define QPG_SERVREG_PORT			0x14
#define QPG_CHARGER_PD_PATH			"msm/adsp/charger_pd"

struct qpg_qmi_hdr {
	u8 type;
	__le16 txn;
	__le16 msg_id;
	__le16 msg_len;
} __packed;

#define GLINK_VERSION_1				1
#define GLINK_FEATURE_INTENT_REUSE		BIT(0)

#define GLINK_CMD_VERSION			0
#define GLINK_CMD_VERSION_ACK			1
#define GLINK_CMD_OPEN				2
#define GLINK_CMD_OPEN_ACK			4
#define GLINK_CMD_INTENT			5
#define GLINK_CMD_RX_DONE			6
#define GLINK_CMD_RX_INTENT_REQ			7
#define GLINK_CMD_RX_INTENT_REQ_ACK		8
#define GLINK_CMD_TX_DATA			9
#define GLINK_CMD_TX_DATA_CONT			12
#define GLINK_CMD_READ_NOTIF			13
#define GLINK_CMD_RX_DONE_W_REUSE		14

#define PMIC_GLINK_OWNER_CHARGER		32778
#define PMIC_GLINK_OWNER_USB_TYPE_C		32779
#define PMIC_GLINK_OWNER_USBC_PAN		32780
#define PMIC_GLINK_REQ_RESP			1

/* PMIC_GLINK_OWNER_CHARGER (battmgr) opcodes */
#define BATT_MNGR_GET_CHARGER_STATUS_REQ	0x0001
#define BATT_MNGR_SET_OPERATIONAL_MODE_REQ	0x0003
#define BATT_MNGR_SET_NOTIFICATION_CRITERIA_REQ	0x0004
#define BATT_MNGR_NOTIFY_IND			0x0007

#define UCSI_READ_BUFFER_REQ			0x11
#define UCSI_WRITE_BUFFER_REQ			0x12
#define UCSI_NOTIFY_IND			0x13

#define USBC_CMD_READ_REQ			0x14
#define USBC_SC8180X_NOTIFY_IND			0x13
#define USBC_CMD_WRITE_REQ			0x15
#define USBC_NOTIFY_IND				0x16

#define ALTMODE_PAN_EN				0x10
#define ALTMODE_PAN_ACK				0x11
#define ALTMODE_READ_SEL			0x12

#define UCSI_BUFFER_SIZE			48
#define USBC_READ_BUFFER_SIZE			32
#define UCSI_CMD_PPM_RESET			1
#define UCSI_CMD_CONNECTOR_RESET		3
#define UCSI_CMD_ACK_CC_CI			4
#define UCSI_CMD_SET_UOR			9
#define UCSI_CMD_SET_NOTIFICATION_ENABLE	5
#define UCSI_CMD_GET_CAPABILITY			6
#define UCSI_CMD_GET_CONNECTOR_CAPABILITY	7
#define UCSI_CMD_GET_ALTERNATE_MODE		12
#define UCSI_CMD_GET_CAM_SUPPORTED		13
#define UCSI_CMD_GET_CURRENT_CAM		14
#define UCSI_CMD_GET_CONNECTOR_STATUS		18
#define QPG_UCSI_NTFY_CMD_COMPLETE	BIT(0)	/* Linux BIT(16) >> 16 */
#define QPG_UCSI_NTFY_ERROR		BIT(15)	/* Linux BIT(31) >> 16 */
#define QPG_UCSI_NTFY_ALL		0xdbe7	/* Linux 0xdbe70000 >> 16 */
#define UCSI_CCI_NOT_SUPPORTED			BIT(25)
#define UCSI_CCI_CANCEL_COMPLETE		BIT(26)
#define UCSI_CCI_RESET_COMPLETE			BIT(27)
#define UCSI_CCI_BUSY				BIT(28)
#define UCSI_CCI_ACK_COMPLETE			BIT(29)
#define UCSI_CCI_ERROR				BIT(30)
#define UCSI_CCI_COMMAND_COMPLETE		BIT(31)
#define UCSI_CCI_CONNECTOR_CHANGE_MASK		GENMASK(7, 1)
#define UCSI_CCI_CONNECTOR_CHANGE_SHIFT		1
#define UCSI_CCI_DATA_LENGTH_MASK		GENMASK(15, 8)
#define UCSI_CCI_DATA_LENGTH_SHIFT		8
#define UCSI_ACK_CC_CI_CONNECTOR_CHANGE		BIT(0)
#define UCSI_ACK_CC_CI_COMMAND_COMPLETE		BIT(1)

/*
 * UCSI u64 control value macros — serialize commands into bytes 8-15
 * of the UCSI write buffer via put_unaligned_le64().
 *
 * Byte layout (LE u64 value):
 *   bits  0- 7 (byte 8):  command
 *   bits  8-15 (byte 9):  data length
 *   bits 16-31 (bytes 10-11): command-specific word (2 bytes)
 *   bits 32-63 (bytes 12-15): command-specific dword (4 bytes)
 */
#define UCSI_CTRL_CMD(cmd)			((u64)(cmd))
#define UCSI_CTRL_D2(cmd, d2)			\
	((u64)(cmd) | ((u64)(d2) << 16))
#define UCSI_CTRL_D2_D4(cmd, d2, d4)		\
	((u64)(cmd) | ((u64)(d2) << 16) | ((u64)(d4) << 32))

#define USBC_READ_SEL_PIN_ASSIGNMENT		1
#define USBC_READ_DATA_PIN_ASSIGNMENT		1

#define USB_TYPEC_DP_SID			0xff01
#define DPAM_HPD_A				1

#define SC8180X_PORT_MASK			0x000000ff
#define SC8180X_ORIENTATION_MASK		0x0000ff00
#define SC8180X_MUX_MASK			0x00ff0000
#define SC8180X_MODE_MASK			0x3f000000
#define SC8180X_HPD_STATE_MASK			BIT(30)
#define SC8180X_HPD_IRQ_MASK			BIT(31)

#define SC8280XP_DPAM_MASK			0x3f
#define SC8280XP_HPD_STATE_MASK			BIT(6)
#define SC8280XP_HPD_IRQ_MASK			BIT(7)

enum qpg_typec_state {
	QPG_TYPEC_SAFE,
	QPG_TYPEC_USB,
	QPG_TYPEC_DP,
};

struct qpg_msg {
	__le16 cmd;
	__le16 param1;
	__le32 param2;
	u8 data[];
} __packed;

struct qpg_pmic_hdr {
	__le32 owner;
	__le32 type;
	__le32 opcode;
} __packed;

struct qpg_usbc_write_req {
	struct qpg_pmic_hdr hdr;
	__le32 cmd;
	__le32 arg;
	__le32 reserved;
} __packed;

struct qpg_battmgr_opmode_req {
	struct qpg_pmic_hdr hdr;
	__le32 operational_mode;
} __packed;

struct qpg_battmgr_notify_crit_req {
	struct qpg_pmic_hdr hdr;
	__le32 battery_id;
	__le32 power_state;
	__le32 low_capacity;
	__le32 high_capacity;
} __packed;

struct qpg_ucsi_read_buffer_req {
	struct qpg_pmic_hdr hdr;
} __packed;

struct qpg_ucsi_read_buffer_resp {
	struct qpg_pmic_hdr hdr;
	u8 read_buffer[UCSI_BUFFER_SIZE];
	__le32 return_code;
} __packed;

struct qpg_ucsi_write_buffer_req {
	struct qpg_pmic_hdr hdr;
	u8 write_buffer[UCSI_BUFFER_SIZE];
	__le32 reserved;
} __packed;

struct qpg_ucsi_write_buffer_resp {
	struct qpg_pmic_hdr hdr;
	__le32 return_code;
} __packed;

struct qpg_ucsi_notify {
	struct qpg_pmic_hdr hdr;
	__le32 notification;
	__le32 receiver;
	__le32 reserved;
} __packed;

struct qpg_usbc_read_req {
	struct qpg_pmic_hdr hdr;
	__le32 reserved;
} __packed;

struct qpg_usbc_read_resp {
	struct qpg_pmic_hdr hdr;
	u8 read_buffer[USBC_READ_BUFFER_SIZE];
	__le32 return_code;
} __packed;

struct qpg_usbc_notify {
	struct qpg_pmic_hdr hdr;
	u8 payload[16];
	__le32 reserved;
} __packed;

struct qpg_usbc_sc8180x_notify {
	struct qpg_pmic_hdr hdr;
	__le32 notification;
	__le32 reserved[2];
} __packed;

struct qpg_intent_pair {
	__le32 size;
	__le32 iid;
} __packed;

struct qpg_notify_debug {
	bool seen;
	u8 port;
	u8 raw_orientation;
	enum qcom_pmic_glink_orientation orientation;
	u8 mux;
	u16 svid;
	u8 dpam;
	u8 linux_mux_mode;
	u8 dp_pin_assignment;
	bool hpd;
	bool hpd_irq;
};

struct qpg {
	struct udevice *smem;
	struct mbox_chan mbox_chan;
	u32 remote_pid;
	__le32 *tx_tail;
	__le32 *tx_head;
	__le32 *rx_tail;
	__le32 *rx_head;
	u8 *tx_fifo;
	u8 *rx_fifo;
	size_t tx_len;
	size_t rx_len;
	u16 lcid;
	u16 rcid;
	u32 next_liid;
	u32 riid;
	u32 riid_size;
	bool riid_avail;
	bool version_acked;
	bool local_open_sent;
	bool open_acked;
	bool remote_opened;
	bool remote_open_ack_pending;
	bool remote_open_acked;
	bool pan_acked;
	bool battmgr_acked;
	/* IPCRTR (qrtr/servreg) second GLINK channel state */
	u16 ipcrtr_rcid;
	u32 ipcrtr_liid;
	bool ipcrtr_seen;
	bool ipcrtr_open_ack_pending;
	bool ipcrtr_local_open_sent;
	bool ipcrtr_open_acked;
	bool ipcrtr_intent_posted;
	/* queue of remote (ADSP) RX intents for sending qrtr packets back */
	u32 ipcrtr_riid_q[32];
	u8 ipcrtr_riid_head;
	u8 ipcrtr_riid_tail;
	bool ipcrtr_hello_pending;
	bool ipcrtr_hello_sent;
	u32 ipcrtr_node;
	/* servreg listener-client state for the ADSP charger_pd notifier */
	u32 servreg_node;
	u32 servreg_port;
	bool servreg_notifier_seen;
	bool servreg_register_pending;
	bool servreg_registered;
	bool servreg_ack_pending;
	u16 servreg_txn;
	u16 servreg_ack_txn;
	u32 servreg_last_state;
	bool altmode_notify_seen;
	bool altmode_no_dp;
	bool ucsi_read_acked;
	bool ucsi_write_acked;
	bool usbc_read_acked;
	bool ucsi_notify_seen;
	bool ucsi_prewarmed;
	u32 ucsi_notification;
	u32 ucsi_read_return_code;
	u32 ucsi_write_return_code;
	u32 usbc_read_return_code;
	u8 ucsi_read_buffer[UCSI_BUFFER_SIZE];
	u8 usbc_read_buffer[USBC_READ_BUFFER_SIZE];
	struct qpg_notify_debug notify;
};

static struct qpg qpg_session;
static bool qpg_session_ready;
static struct qcom_pmic_glink_altmode qpg_cached_altmode;
static bool qpg_cached_altmode_valid;
static struct qcom_pmic_glink_altmode_state qpg_cached_state;
static int qpg_last_adsp_boot_ret;
static int qpg_last_ucsi_prewarm_ret;

/**
 * qpg_mbox_from_glink() - Get IPCC mailbox channel from glink-edge DT node.
 *
 * The glink-edge subnode of remoteproc_adsp has:
 *   mboxes = <&ipcc IPCC_CLIENT_LPASS IPCC_MPROC_SIGNAL_GLINK_QMP>;
 *
 * Since pmic-glink is not a DM device bound to the glink-edge node,
 * we manually parse the phandle, get the IPCC device, and call the
 * mailbox provider's .of_xlate() and .request() to construct the
 * channel properly (matching what mbox_get_by_index() would do for a
 * bound device).
 */
static int qpg_mbox_from_glink(ofnode glink, struct mbox_chan *chan)
{
	struct ofnode_phandle_args args;
	const struct mbox_ops *ops;
	struct udevice *ipcc_dev;
	int ret;

	ret = ofnode_parse_phandle_with_args(glink, "mboxes",
					     "#mbox-cells", 0, 0, &args);
	if (ret) {
		log_warning("pmic-glink: glink mboxes parse ret=%d\n", ret);
		return ret;
	}

	if (args.args_count != 2 || !ofnode_valid(args.node)) {
		log_warning("pmic-glink: bad mboxes args_count=%d\n",
			    args.args_count);
		return -EINVAL;
	}

	/* Get the IPCC mailbox device by its DT node */
	ret = uclass_get_device_by_ofnode(UCLASS_MAILBOX, args.node,
					  &ipcc_dev);
	if (ret) {
		log_warning("pmic-glink: IPCC dev lookup ret=%d\n", ret);
		return ret;
	}

	ops = ipcc_dev->driver->ops;
	chan->dev = ipcc_dev;
	chan->con_priv = NULL;

	/* Use the provider's .of_xlate() to construct chan->id */
	if (ops->of_xlate)
		ret = ops->of_xlate(chan, &args);
	else
		chan->id = args.args[0];
	if (ret) {
		log_warning("pmic-glink: IPCC of_xlate failed ret=%d\n", ret);
		return ret;
	}

	/* Call provider's .request() to complete channel setup */
	if (ops->request)
		ret = ops->request(chan);
	if (ret) {
		log_warning("pmic-glink: IPCC request failed ret=%d\n", ret);
		return ret;
	}

	log_warning("pmic-glink: IPCC mbox chan dev=%s id=%08lx\n",
		    ipcc_dev->name, chan->id);

	return 0;
}

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

	qpg_cached_altmode = *altmode;
	qpg_cached_altmode_valid = true;

	qpg_cached_state.service_started = qpg_session_ready;
	qpg_cached_state.notify_seen = true;
	qpg_cached_state.dp_seen |= state == QPG_TYPEC_DP;
	qpg_cached_state.hpd = altmode->hpd;
	qpg_cached_state.hpd_irq = altmode->hpd_irq;
	qpg_cached_state.port = altmode->port;
	qpg_cached_state.orientation_raw = pg->notify.raw_orientation;
	qpg_cached_state.orientation = altmode->orientation;
	qpg_cached_state.mux = pg->notify.mux;
	qpg_cached_state.dpam_raw = pg->notify.dpam;
	qpg_cached_state.linux_mux_mode = pg->notify.linux_mux_mode;
	qpg_cached_state.dp_pin_assignment = pg->notify.dp_pin_assignment;
	qpg_cached_state.pin_assignment = altmode->pin_assignment;
	qpg_cached_state.svid = pg->notify.svid;
	qpg_cached_state.typec_state = qpg_public_typec_state(state);
	qpg_cached_state.last_notify_ms = get_timer(0);

	log_warning("qpg: service started=%u pan=%u notify_seen=%u dp_seen=%u hpd=%u raw_dpam=%u linux_mode=%u dp_pin=%u age_ms=%lu\n",
		    qpg_cached_state.service_started,
		    qpg_cached_state.pan_enabled,
		    qpg_cached_state.notify_seen,
		    qpg_cached_state.dp_seen,
		    qpg_cached_state.hpd,
		    qpg_cached_state.dpam_raw,
		    qpg_cached_state.linux_mux_mode,
		    qpg_cached_state.dp_pin_assignment,
		    get_timer(qpg_cached_state.last_notify_ms));
}

static bool qpg_env_bool(const char *name)
{
	const char *value = env_get(name);

	return value && (!strcmp(value, "1") ||
			 !strcmp(value, "true") ||
			 !strcmp(value, "yes") ||
			 !strcmp(value, "on"));
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

static size_t qpg_rx_avail(struct qpg *pg)
{
	u32 head = le32_to_cpu(*pg->rx_head);
	u32 tail = le32_to_cpu(*pg->rx_tail);

	if (head < tail)
		return pg->rx_len - tail + head;

	return head - tail;
}

static size_t qpg_tx_avail(struct qpg *pg)
{
	u32 head = le32_to_cpu(*pg->tx_head);
	u32 tail = le32_to_cpu(*pg->tx_tail);
	u32 avail;

	if (tail <= head)
		avail = pg->tx_len - head + tail;
	else
		avail = tail - head;

	if (avail < QPG_FIFO_FULL_RESERVE + QPG_TX_BLOCKED_CMD_RESERVE)
		return 0;

	return avail - QPG_FIFO_FULL_RESERVE - QPG_TX_BLOCKED_CMD_RESERVE;
}

static void qpg_rx_peek(struct qpg *pg, void *data, size_t offset, size_t len)
{
	u8 *buf = data;
	u32 tail = le32_to_cpu(*pg->rx_tail);
	size_t first;

	tail += offset;
	if (tail >= pg->rx_len)
		tail -= pg->rx_len;

	first = min_t(size_t, len, pg->rx_len - tail);
	memcpy(buf, pg->rx_fifo + tail, first);
	if (first != len)
		memcpy(buf + first, pg->rx_fifo, len - first);
}

static void qpg_rx_advance(struct qpg *pg, size_t len)
{
	u32 tail = le32_to_cpu(*pg->rx_tail);

	tail += len;
	if (tail >= pg->rx_len)
		tail %= pg->rx_len;

	*pg->rx_tail = cpu_to_le32(tail);
}

static u32 qpg_tx_write_one(struct qpg *pg, u32 head, const void *data,
			    size_t len)
{
	const u8 *buf = data;
	size_t first;

	first = min_t(size_t, len, pg->tx_len - head);
	memcpy(pg->tx_fifo + head, buf, first);
	if (first != len)
		memcpy(pg->tx_fifo, buf + first, len - first);

	head += len;
	if (head >= pg->tx_len)
		head -= pg->tx_len;

	return head;
}

static void qpg_kick(struct qpg *pg)
{
	int ret;

	ret = mbox_send(&pg->mbox_chan, NULL);
	if (ret)
		log_warning("pmic-glink: IPCC kick failed ret=%d\n", ret);
}

static int qpg_tx(struct qpg *pg, const void *hdr, size_t hlen,
		  const void *data, size_t dlen)
{
	size_t len = ALIGN(hlen + dlen, 8);
	u32 head, next;
	size_t avail;

	avail = qpg_tx_avail(pg);
	head = le32_to_cpu(*pg->tx_head);
	next = head + len;
	if (next >= pg->tx_len)
		next %= pg->tx_len;

	log_debug("pmic-glink: TX hlen=%zu dlen=%zu aligned=%zu head=%u next=%u avail=%zu\n",
		    hlen, dlen, len, head, next, avail);

	if (len > pg->tx_len || avail < len)
		return -EAGAIN;

	head = qpg_tx_write_one(pg, head, hdr, hlen);
	if (dlen)
		head = qpg_tx_write_one(pg, head, data, dlen);

	wmb();
	*pg->tx_head = cpu_to_le32(next);
	qpg_kick(pg);

	return 0;
}

static int qpg_send_simple(struct qpg *pg, u16 cmd, u16 param1, u32 param2)
{
	struct qpg_msg msg = {
		.cmd = cpu_to_le16(cmd),
		.param1 = cpu_to_le16(param1),
		.param2 = cpu_to_le32(param2),
	};
	int ret;

	if (cmd == GLINK_CMD_OPEN_ACK)
		log_warning("pmic-glink: send OPEN_ACK begin\n");

	ret = qpg_tx(pg, &msg, sizeof(msg), NULL, 0);

	if (cmd == GLINK_CMD_OPEN_ACK)
		log_warning("pmic-glink: send OPEN_ACK end ret=%d\n", ret);

	return ret;
}

static int qpg_send_open_ack(struct qpg *pg, u16 rcid, const char *name)
{
	log_warning("pmic-glink: send OPEN_ACK channel='%s' rcid=%u\n",
		    name ? name : "<unknown>", rcid);

	return qpg_send_simple(pg, GLINK_CMD_OPEN_ACK, rcid, 0);
}

static int qpg_send_version(struct qpg *pg)
{
	return qpg_send_simple(pg, GLINK_CMD_VERSION, GLINK_VERSION_1,
			       GLINK_FEATURE_INTENT_REUSE);
}

static int qpg_send_open_for(struct qpg *pg, u16 lcid, const char *name)
{
	struct {
		struct qpg_msg msg;
		char name[32];
	} __packed req = {};
	size_t name_len = strlen(name) + 1;

	if (name_len > sizeof(req.name))
		return -EINVAL;

	req.msg.cmd = cpu_to_le16(GLINK_CMD_OPEN);
	req.msg.param1 = cpu_to_le16(lcid);
	req.msg.param2 = cpu_to_le32(name_len);
	strcpy(req.name, name);

	log_warning("pmic-glink: send OPEN channel='%s' lcid=%u\n",
		    name, lcid);

	return qpg_tx(pg, &req, ALIGN(sizeof(req.msg) + name_len, 8), NULL, 0);
}

static int qpg_send_open(struct qpg *pg)
{
	return qpg_send_open_for(pg, pg->lcid, QPG_CHANNEL_NAME);
}

static u32 qpg_alloc_liid(struct qpg *pg)
{
	if (!pg->next_liid)
		pg->next_liid = 1;

	return pg->next_liid++;
}

static int qpg_send_rx_intent_for_size(struct qpg *pg, u16 cid, u32 liid,
				       u32 size)
{
	struct {
		__le16 cmd;
		__le16 lcid;
		__le32 count;
		__le32 size;
		__le32 liid;
	} __packed msg = {
		.cmd = cpu_to_le16(GLINK_CMD_INTENT),
		.lcid = cpu_to_le16(cid),
		.count = cpu_to_le32(1),
		.size = cpu_to_le32(size),
		.liid = cpu_to_le32(liid),
	};

	log_warning("pmic-glink: send RX_INTENT cid=%u liid=%u size=%u\n",
		    cid, liid, size);

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_send_rx_intent_for(struct qpg *pg, u16 cid, u32 liid)
{
	return qpg_send_rx_intent_for_size(pg, cid, liid,
					   QPG_RX_INTENT_SIZE);
}

static int qpg_send_rx_intent_req_ack_for(struct qpg *pg, u16 cid,
					  bool granted)
{
	log_warning("pmic-glink: send RX_INTENT_REQ_ACK cid=%u granted=%d\n",
		    cid, granted);

	return qpg_send_simple(pg, GLINK_CMD_RX_INTENT_REQ_ACK, cid,
			       granted);
}

static int qpg_send_rx_done_for(struct qpg *pg, u16 cid, u32 liid)
{
	struct {
		__le16 cmd;
		__le16 lcid;
		__le32 liid;
	} __packed msg = {
		.cmd = cpu_to_le16(GLINK_CMD_RX_DONE_W_REUSE),
		.lcid = cpu_to_le16(cid),
		.liid = cpu_to_le32(liid),
	};

	log_debug("pmic-glink: send RX_DONE cid=%u liid=%u\n", cid, liid);

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_wait_riid(struct qpg *pg);
static int qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg);
static int qpg_send_ucsi_read(struct qpg *pg);
static int qpg_send_usbc_read(struct qpg *pg,
			      struct qcom_pmic_glink_altmode *altmode);
static int qpg_send_ucsi_get_connector_status(struct qpg *pg, u8 port);
static int qpg_drain_until(struct qpg *pg,
			   struct qcom_pmic_glink_altmode *altmode,
			   bool (*done)(struct qpg *,
					struct qcom_pmic_glink_altmode *),
			   u32 timeout_ms);
static bool qpg_done_pan_ack(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode);
static bool qpg_done_ucsi_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode);
static bool qpg_done_ucsi_write(struct qpg *pg,
				struct qcom_pmic_glink_altmode *altmode);
static bool qpg_done_usbc_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode);
static int qpg_service_ipcrtr(struct qpg *pg);

static int qpg_send_data_for(struct qpg *pg, u16 lcid, u32 riid,
			     const void *data, size_t len)
{
	struct {
		struct qpg_msg msg;
		__le32 chunk_size;
		__le32 left_size;
	} __packed hdr;
	int ret;

	hdr.msg.cmd = cpu_to_le16(GLINK_CMD_TX_DATA);
	hdr.msg.param1 = cpu_to_le16(lcid);
	hdr.msg.param2 = cpu_to_le32(riid);
	hdr.chunk_size = cpu_to_le32(len);
	hdr.left_size = 0;

	log_debug("pmic-glink: send data begin lcid=%u len=%zu riid=%u\n",
		    lcid, len, riid);
	ret = qpg_tx(pg, &hdr, sizeof(hdr), data, len);
	log_debug("pmic-glink: send data end ret=%d\n", ret);

	return ret;
}

static int qpg_send_data(struct qpg *pg, const void *data, size_t len)
{
	int ret;

	ret = qpg_wait_riid(pg);
	if (ret)
		return ret;

	pg->riid_avail = false;

	return qpg_send_data_for(pg, pg->lcid, pg->riid, data, len);
}

/*
 * Send a qrtr packet (header + body) into the ADSP's IPCRTR RX intent.
 * type is a QRTR_TYPE_*; for control packets src/dst port = QRTR_PORT_CTRL.
 */
static bool qpg_ipcrtr_riid_ready(struct qpg *pg)
{
	return pg->ipcrtr_riid_head != pg->ipcrtr_riid_tail;
}

static bool qpg_ipcrtr_pop_riid(struct qpg *pg, u32 *riid)
{
	if (pg->ipcrtr_riid_head == pg->ipcrtr_riid_tail)
		return false;
	*riid = pg->ipcrtr_riid_q[pg->ipcrtr_riid_tail];
	pg->ipcrtr_riid_tail = (pg->ipcrtr_riid_tail + 1) % 32;
	return true;
}

static int qpg_qrtr_send(struct qpg *pg, u32 type, u32 src_port,
			 u32 dst_node, u32 dst_port,
			 const void *body, size_t body_len)
{
	u8 buf[256];
	struct qpg_qrtr_hdr *hdr = (void *)buf;
	u32 riid;

	if (sizeof(*hdr) + body_len > sizeof(buf))
		return -EINVAL;
	if (!qpg_ipcrtr_pop_riid(pg, &riid))
		return -EAGAIN;

	memset(hdr, 0, sizeof(*hdr));
	hdr->version = cpu_to_le32(QRTR_PROTO_VER_1);
	hdr->type = cpu_to_le32(type);
	hdr->src_node = cpu_to_le32(QPG_QRTR_LOCAL_NODE);
	hdr->src_port = cpu_to_le32(src_port);
	hdr->size = cpu_to_le32(body_len);
	hdr->dst_node = cpu_to_le32(dst_node);
	hdr->dst_port = cpu_to_le32(dst_port);
	if (body_len)
		memcpy(buf + sizeof(*hdr), body, body_len);

	return qpg_send_data_for(pg, QPG_IPCRTR_LCID, riid,
				 buf, sizeof(*hdr) + body_len);
}

/* Reply to the ADSP's qrtr HELLO so its qrtr/servreg stack proceeds. */
static int qpg_qrtr_send_hello(struct qpg *pg)
{
	struct qpg_qrtr_ctrl ctrl = {};
	int ret;

	ctrl.cmd = cpu_to_le32(QRTR_TYPE_HELLO);
	ret = qpg_qrtr_send(pg, QRTR_TYPE_HELLO, QRTR_PORT_CTRL,
			    pg->ipcrtr_node, QRTR_PORT_CTRL, &ctrl, sizeof(ctrl));
	log_warning("pmic-glink: IPCRTR sent HELLO -> node=%u ret=%d\n",
		    pg->ipcrtr_node, ret);
	return ret;
}

/* servreg REGISTER_LISTENER(enable=1, "msm/adsp/charger_pd") to the ADSP notifier. */
static int qpg_servreg_register(struct qpg *pg)
{
	u8 msg[96];
	struct qpg_qmi_hdr *qh = (void *)msg;
	u8 *p = msg + sizeof(*qh);
	size_t pathlen = strlen(QPG_CHARGER_PD_PATH);
	u16 msg_len;
	int ret;

	/* TLV 0x01: enable (u8) = 1 */
	*p++ = 0x01; *p++ = 0x01; *p++ = 0x00; *p++ = 0x01;
	/* TLV 0x02: service_path (string, no NUL on wire) */
	*p++ = 0x02; *p++ = pathlen & 0xff; *p++ = (pathlen >> 8) & 0xff;
	memcpy(p, QPG_CHARGER_PD_PATH, pathlen);
	p += pathlen;

	msg_len = (u16)(p - msg - sizeof(*qh));
	qh->type = QMI_TYPE_REQUEST;
	qh->txn = cpu_to_le16(++pg->servreg_txn);
	qh->msg_id = cpu_to_le16(SERVREG_REGISTER_LISTENER_REQ);
	qh->msg_len = cpu_to_le16(msg_len);

	ret = qpg_qrtr_send(pg, QRTR_TYPE_DATA, QPG_SERVREG_PORT,
			    pg->servreg_node, pg->servreg_port, msg, p - msg);
	log_warning("pmic-glink: servreg REGISTER_LISTENER -> %u:%u path=%s ret=%d\n",
		    pg->servreg_node, pg->servreg_port, QPG_CHARGER_PD_PATH, ret);
	return ret;
}

/* servreg SET_ACK(service_path, transaction_id) acking a state indication. */
static int qpg_servreg_send_ack(struct qpg *pg, u16 ind_txn)
{
	u8 msg[96];
	struct qpg_qmi_hdr *qh = (void *)msg;
	u8 *p = msg + sizeof(*qh);
	size_t pathlen = strlen(QPG_CHARGER_PD_PATH);
	u16 msg_len;
	int ret;

	/* TLV 0x01: service_path (string) */
	*p++ = 0x01; *p++ = pathlen & 0xff; *p++ = (pathlen >> 8) & 0xff;
	memcpy(p, QPG_CHARGER_PD_PATH, pathlen);
	p += pathlen;
	/* TLV 0x02: transaction_id (u16) */
	*p++ = 0x02; *p++ = 0x02; *p++ = 0x00;
	*p++ = ind_txn & 0xff; *p++ = (ind_txn >> 8) & 0xff;

	msg_len = (u16)(p - msg - sizeof(*qh));
	qh->type = QMI_TYPE_REQUEST;
	qh->txn = cpu_to_le16(++pg->servreg_txn);
	qh->msg_id = cpu_to_le16(SERVREG_SET_ACK_REQ);
	qh->msg_len = cpu_to_le16(msg_len);

	ret = qpg_qrtr_send(pg, QRTR_TYPE_DATA, QPG_SERVREG_PORT,
			    pg->servreg_node, pg->servreg_port, msg, p - msg);
	log_warning("pmic-glink: servreg SET_ACK ind_txn=%u ret=%d\n",
		    ind_txn, ret);
	return ret;
}

/* Find TLV @id in a QMI payload [p,len); return value ptr + length, or NULL. */
static const u8 *qpg_qmi_find_tlv(const u8 *p, size_t len, u8 id, u16 *vlen)
{
	size_t i = 0;

	while (i + 3 <= len) {
		u8 t = p[i];
		u16 l = p[i + 1] | (p[i + 2] << 8);

		if (i + 3 + l > len)
			break;
		if (t == id) {
			*vlen = l;
			return p + i + 3;
		}
		i += 3 + l;
	}
	return NULL;
}

/*
 * Handle a qrtr packet from the ADSP on IPCRTR: reply to HELLO, latch the
 * charger_pd servreg notifier from NEW_SERVER, and ack servreg state
 * indications. This is the AP/servreg-client behaviour the ADSP needs to bring
 * charger_pd fully up so it will run the DisplayPort alt-mode VDM.
 */
static void qpg_qrtr_rx(struct qpg *pg, const u8 *data, size_t len)
{
	const struct qpg_qrtr_hdr *hdr = (const void *)data;
	char hex[3 * 64 + 1];
	size_t n, i;
	u32 type, src_node, src_port, dst_port, size;

	n = min_t(size_t, len, 64);
	for (i = 0; i < n; i++)
		snprintf(hex + i * 3, 4, "%02x ", data[i]);
	hex[n ? n * 3 - 1 : 0] = '\0';

	if (len < sizeof(*hdr)) {
		log_warning("pmic-glink: IPCRTR qrtr SHORT len=%zu bytes=[%s]\n",
			    len, hex);
		return;
	}

	type = le32_to_cpu(hdr->type);
	src_node = le32_to_cpu(hdr->src_node);
	src_port = le32_to_cpu(hdr->src_port);
	dst_port = le32_to_cpu(hdr->dst_port);
	size = le32_to_cpu(hdr->size);

	log_warning("pmic-glink: IPCRTR qrtr type=%u src=%u:%08x dst=%u:%08x size=%u len=%zu bytes=[%s]\n",
		    type, src_node, src_port,
		    le32_to_cpu(hdr->dst_node), dst_port, size, len, hex);

	if (type == QRTR_TYPE_HELLO) {
		pg->ipcrtr_node = src_node;
		pg->ipcrtr_hello_pending = true;
		return;
	}

	if (type == QRTR_TYPE_NEW_SERVER && len >= sizeof(*hdr) + sizeof(struct qpg_qrtr_ctrl)) {
		const struct qpg_qrtr_ctrl *c = (const void *)(data + sizeof(*hdr));
		u32 svc = le32_to_cpu(c->service);

		if (svc == SERVREG_NOTIFIER_QMI_SVC && !pg->servreg_notifier_seen) {
			pg->servreg_node = le32_to_cpu(c->node);
			pg->servreg_port = le32_to_cpu(c->port);
			pg->servreg_notifier_seen = true;
			pg->servreg_register_pending = true;
			log_warning("pmic-glink: servreg NOTIFIER found svc=0x%x inst=0x%x @ %u:%u\n",
				    svc, le32_to_cpu(c->instance),
				    pg->servreg_node, pg->servreg_port);
		}
		return;
	}

	/* QMI DATA addressed to our servreg client port */
	if (type == QRTR_TYPE_DATA && dst_port == QPG_SERVREG_PORT &&
	    len >= sizeof(*hdr) + sizeof(struct qpg_qmi_hdr)) {
		const struct qpg_qmi_hdr *qh = (const void *)(data + sizeof(*hdr));
		const u8 *tlv = data + sizeof(*hdr) + sizeof(*qh);
		size_t tlv_len = len - sizeof(*hdr) - sizeof(*qh);
		u16 msg_id = le16_to_cpu(qh->msg_id);
		u16 vlen = 0;
		const u8 *v;

		log_warning("pmic-glink: servreg QMI type=%u msg_id=0x%x len=%u\n",
			    qh->type, msg_id, le16_to_cpu(qh->msg_len));

		if (qh->type == QMI_TYPE_RESPONSE &&
		    msg_id == SERVREG_REGISTER_LISTENER_REQ) {
			pg->servreg_registered = true;
			/* current state may be in TLV 0x10 (curr_state) */
			v = qpg_qmi_find_tlv(tlv, tlv_len, 0x10, &vlen);
			if (v && vlen >= 4)
				pg->servreg_last_state = v[0] | (v[1] << 8) |
					(v[2] << 16) | (v[3] << 24);
			log_warning("pmic-glink: servreg REGISTER ack state=%u\n",
				    pg->servreg_last_state);
		} else if (qh->type == QMI_TYPE_INDICATION &&
			   msg_id == SERVREG_STATE_UPDATED_IND) {
			u16 itxn = 0;

			v = qpg_qmi_find_tlv(tlv, tlv_len, 0x01, &vlen);
			if (v && vlen >= 4)
				pg->servreg_last_state = v[0] | (v[1] << 8) |
					(v[2] << 16) | (v[3] << 24);
			v = qpg_qmi_find_tlv(tlv, tlv_len, 0x03, &vlen);
			if (v && vlen >= 2)
				itxn = v[0] | (v[1] << 8);
			pg->servreg_ack_txn = itxn;
			pg->servreg_ack_pending = true;
			log_warning("pmic-glink: servreg STATE_UPDATED state=%u txn=%u -> ack\n",
				    pg->servreg_last_state, itxn);
		}
		return;
	}
}

static bool qpg_parse_sc8280xp_notify(struct qpg *pg,
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

static bool qpg_parse_sc8180x_notify(struct qpg *pg,
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

static int qpg_send_notify_pan_ack(struct qpg *pg,
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
		    qpg_public_typec_state_name(qpg_cached_state.typec_state),
		    ret);

	return ret;
}

static bool qpg_parse_pmic(struct qpg *pg,
			   struct qcom_pmic_glink_altmode *altmode,
			   const void *data, size_t len, u32 *pan_ack_port)
{
	const struct qpg_pmic_hdr *hdr = data;
	u32 owner, type, raw_opcode;
	bool ack_notify = false;
	u32 port = 0;
	u16 opcode;
	u16 svid;

	if (len < sizeof(*hdr)) {
		log_warning("pmic-glink: PMIC msg too short len=%zu\n", len);
		return false;
	}

	owner = le32_to_cpu(hdr->owner);
	type = le32_to_cpu(hdr->type);
	raw_opcode = le32_to_cpu(hdr->opcode);
	opcode = raw_opcode & 0xff;
	svid = raw_opcode >> 16;

	log_debug("pmic-glink: PMIC msg owner=%u type=%u opcode=%02x raw_opcode=%08x svid=%04x len=%zu\n",
		    owner, type, opcode, raw_opcode, svid, len);

	if (owner == PMIC_GLINK_OWNER_USB_TYPE_C) {
		const struct qpg_ucsi_read_buffer_resp *resp = data;
		const struct qpg_ucsi_write_buffer_resp *write_resp = data;
		const struct qpg_ucsi_notify *notify = data;

		switch (opcode) {
		case UCSI_READ_BUFFER_REQ:
			pg->ucsi_read_acked = true;
			if (len >= sizeof(*resp)) {
				const u8 *buf = resp->read_buffer;

				pg->ucsi_read_return_code =
					le32_to_cpu(resp->return_code);
				memcpy(pg->ucsi_read_buffer, resp->read_buffer,
				       sizeof(pg->ucsi_read_buffer));
				log_warning("pmic-glink: UCSI READ_BUFFER ret=%u buf=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					    le32_to_cpu(resp->return_code),
					    buf[0], buf[1], buf[2], buf[3],
					    buf[4], buf[5], buf[6], buf[7],
					    buf[8], buf[9], buf[10], buf[11],
					    buf[12], buf[13], buf[14], buf[15]);
			} else {
				log_warning("pmic-glink: UCSI READ_BUFFER short len=%zu expected=%zu\n",
					    len, sizeof(*resp));
			}
			break;
		case UCSI_WRITE_BUFFER_REQ:
			pg->ucsi_write_acked = true;
			if (len >= sizeof(*write_resp)) {
				pg->ucsi_write_return_code =
					le32_to_cpu(write_resp->return_code);
				log_warning("pmic-glink: UCSI WRITE_BUFFER ret=%u\n",
					    pg->ucsi_write_return_code);
			} else {
				log_warning("pmic-glink: UCSI WRITE_BUFFER short len=%zu expected=%zu\n",
					    len, sizeof(*write_resp));
			}
			break;
		case UCSI_NOTIFY_IND:
			pg->ucsi_notify_seen = true;
			if (len >= sizeof(*notify)) {
				pg->ucsi_notification =
					le32_to_cpu(notify->notification);
				log_warning("pmic-glink: UCSI notify cci=%08x receiver=%u len=%zu\n",
					    pg->ucsi_notification,
					    le32_to_cpu(notify->receiver), len);
			} else {
				log_warning("pmic-glink: UCSI notify short len=%zu expected=%zu\n",
					    len, sizeof(*notify));
			}
			break;
		default:
			log_warning("pmic-glink: USB Type-C owner opcode=%02x len=%zu\n",
				    opcode, len);
			break;
		}

		return false;
	}

	if (owner == PMIC_GLINK_OWNER_CHARGER) {
		pg->battmgr_acked = true;
		log_warning("pmic-glink: BATTMGR msg type=%u opcode=%02x len=%zu\n",
			    type, opcode, len);
		return false;
	}

	if (owner != PMIC_GLINK_OWNER_USBC_PAN) {
		log_warning("pmic-glink: unsupported PMIC owner=%u opcode=%02x len=%zu\n",
			    owner, opcode, len);
		return false;
	}

	switch (opcode) {
	case USBC_CMD_READ_REQ: {
		const struct qpg_usbc_read_resp *resp = data;

		pg->usbc_read_acked = true;
		if (len >= sizeof(*resp)) {
			const u8 *buf = resp->read_buffer;

			pg->usbc_read_return_code =
				le32_to_cpu(resp->return_code);
			memcpy(pg->usbc_read_buffer, resp->read_buffer,
			       sizeof(pg->usbc_read_buffer));
			log_warning("pmic-glink: USBC READ ret=%u buf=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				    pg->usbc_read_return_code,
				    buf[0], buf[1], buf[2], buf[3],
				    buf[4], buf[5], buf[6], buf[7],
				    buf[8], buf[9], buf[10], buf[11],
				    buf[12], buf[13], buf[14], buf[15]);
		} else {
			log_warning("pmic-glink: USBC READ short len=%zu expected=%zu\n",
				    len, sizeof(*resp));
		}
		break;
	}
	case USBC_CMD_WRITE_REQ:
		pg->pan_acked = true;
		log_warning("pmic-glink: PAN ACK received\n");
		break;
	case USBC_NOTIFY_IND:
		ack_notify = qpg_parse_sc8280xp_notify(pg, altmode, data, len,
						       &port);
		break;
	case USBC_SC8180X_NOTIFY_IND:
		ack_notify = qpg_parse_sc8180x_notify(pg, altmode, data, len,
						      &port);
		break;
	}

	if (ack_notify) {
		*pan_ack_port = port;
		return true;
	}

	return false;
}

static int qpg_rx_data(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
		       size_t avail)
{
	struct {
		struct qpg_msg msg;
		__le32 chunk_size;
		__le32 left_size;
	} __packed hdr;
	u8 *payload;
	u32 chunk_size, liid;
	u16 rx_done_cid = 0;
	bool pan_ack_pending = false;
	u32 pan_ack_port = 0;
	u16 cid;
	int ret = 0;

	if (avail < sizeof(hdr))
		return -EAGAIN;

	qpg_rx_peek(pg, &hdr, 0, sizeof(hdr));
	cid = le16_to_cpu(hdr.msg.param1);
	chunk_size = le32_to_cpu(hdr.chunk_size);
	liid = le32_to_cpu(hdr.msg.param2);
	log_debug("pmic-glink: RX data header cmd=%u lcid=%u liid=%u chunk=%u left=%u avail=%zu payload_len=%u\n",
		    le16_to_cpu(hdr.msg.cmd),
		    cid,
		    liid, chunk_size, le32_to_cpu(hdr.left_size),
		    avail, chunk_size);

	if (!chunk_size || avail < sizeof(hdr) + chunk_size)
		return -EAGAIN;

	payload = malloc(chunk_size);
	if (!payload)
		return -ENOMEM;

	qpg_rx_peek(pg, payload, sizeof(hdr), chunk_size);
	qpg_rx_advance(pg, ALIGN(sizeof(hdr) + chunk_size, 8));

	if (pg->remote_opened && cid == pg->rcid) {
		pan_ack_pending = qpg_parse_pmic(pg, altmode, payload,
						 chunk_size, &pan_ack_port);
		rx_done_cid = pg->lcid;
	} else if (pg->ipcrtr_seen && cid == pg->ipcrtr_rcid) {
		/* qrtr/QMI packet from the ADSP (servreg etc.) */
		qpg_qrtr_rx(pg, payload, chunk_size);
		rx_done_cid = QPG_IPCRTR_LCID;
		/* consume the intent; service re-posts a fresh one */
		pg->ipcrtr_intent_posted = false;
	} else {
		log_warning("pmic-glink: RX data on unknown cid=%u liid=%u len=%u\n",
			    cid, liid, chunk_size);
	}

	if (rx_done_cid)
		ret = qpg_send_rx_done_for(pg, rx_done_cid, liid);

	if (!ret && pan_ack_pending) {
		ret = qpg_send_notify_pan_ack(pg, altmode, pan_ack_port);
		if (ret)
			log_warning("pmic-glink: ALTMODE_PAN_ACK failed ret=%d port=%u\n",
				    ret, pan_ack_port);
	}

	free(payload);

	return ret;
}

static int qpg_handle_intent_req(struct qpg *pg, u16 cid, u32 size)
{
	bool granted = false;
	u32 liid = 0;
	int ack_ret;
	int ret = 0;

	log_warning("pmic-glink: RX_INTENT_REQ rcid=%u size=%u\n", cid, size);

	/* IPCRTR (qrtr) channel: grant an intent on our IPCRTR lcid so the
	 * ADSP can push qrtr/servreg packets to us. */
	if (pg->ipcrtr_seen && cid == pg->ipcrtr_rcid) {
		if (size) {
			liid = qpg_alloc_liid(pg);
			ret = qpg_send_rx_intent_for_size(pg, QPG_IPCRTR_LCID,
							  liid, size);
			if (!ret) {
				pg->ipcrtr_liid = liid;
				pg->ipcrtr_intent_posted = true;
				granted = true;
			}
		}
		ack_ret = qpg_send_rx_intent_req_ack_for(pg, QPG_IPCRTR_LCID,
							 granted);
		return ret ? ret : ack_ret;
	}

	if (!pg->remote_opened || cid != pg->rcid) {
		log_warning("pmic-glink: RX_INTENT_REQ unknown rcid=%u expected=%u\n",
			    cid, pg->rcid);
		goto ack;
	}

	if (!size) {
		log_warning("pmic-glink: RX_INTENT_REQ invalid size=0\n");
		goto ack;
	}

	liid = qpg_alloc_liid(pg);
	ret = qpg_send_rx_intent_for_size(pg, pg->lcid, liid, size);
	if (ret) {
		log_warning("pmic-glink: RX_INTENT_REQ advertise failed ret=%d\n",
			    ret);
		goto ack;
	}

	granted = true;

ack:
	ack_ret = qpg_send_rx_intent_req_ack_for(pg, pg->lcid, granted);
	if (ack_ret)
		return ack_ret;

	log_warning("pmic-glink: RX_INTENT_REQ done granted=%d liid=%u ret=%d\n",
		    granted, liid, ret);

	return 0;
}

static int qpg_handle_intent(struct qpg *pg, u16 cid, u32 count,
			     const struct qpg_intent_pair *intent)
{
	if (!count)
		return -EINVAL;

	if (pg->remote_opened && cid == pg->rcid) {
		pg->riid_size = le32_to_cpu(intent->size);
		pg->riid = le32_to_cpu(intent->iid);
		pg->riid_avail = pg->riid_size > 0;
		log_warning("pmic-glink: RIID channel=raw rcid=%u riid=%u size=%u avail=%d\n",
			    cid, pg->riid, pg->riid_size, pg->riid_avail);
	} else if (pg->ipcrtr_seen && cid == pg->ipcrtr_rcid) {
		/* remote intent on IPCRTR: queue it so we can send qrtr packets */
		u8 nh = (pg->ipcrtr_riid_head + 1) % 32;

		if (le32_to_cpu(intent->size) && nh != pg->ipcrtr_riid_tail) {
			pg->ipcrtr_riid_q[pg->ipcrtr_riid_head] =
				le32_to_cpu(intent->iid);
			pg->ipcrtr_riid_head = nh;
		}
		log_debug("pmic-glink: IPCRTR RIID riid=%u size=%u\n",
			  le32_to_cpu(intent->iid), le32_to_cpu(intent->size));
	} else {
		log_warning("pmic-glink: RIID unknown cid=%u size=%u iid=%u count=%u\n",
			    cid, le32_to_cpu(intent->size),
			    le32_to_cpu(intent->iid), count);
	}

	return 0;
}

static int qpg_handle_open(struct qpg *pg, u16 rcid, const char *name,
			   u32 name_len)
{
	log_warning("pmic-glink: RX remote OPEN rcid=%u name_len=%u name='%s'%s\n",
		    rcid, name_len, name,
		    name_len >= 32 ? " truncated" : "");

	if (!strcmp(name, QPG_CHANNEL_NAME)) {
		pg->rcid = rcid;
		pg->remote_opened = true;
		pg->remote_open_ack_pending = !pg->remote_open_acked;
		log_warning("pmic-glink: PMIC remote OPEN recorded rcid=%u lcid=%u\n",
			    pg->rcid, pg->lcid);
	} else if (!strcmp(name, QPG_IPCRTR_NAME)) {
		pg->ipcrtr_rcid = rcid;
		pg->ipcrtr_seen = true;
		pg->ipcrtr_open_ack_pending = true;
		log_warning("pmic-glink: IPCRTR remote OPEN recorded rcid=%u lcid=%u\n",
			    rcid, QPG_IPCRTR_LCID);
	}

	return 0;
}

static size_t qpg_rx_payload_len(u16 cmd, u32 param2)
{
	switch (cmd) {
	case GLINK_CMD_OPEN:
		return param2;
	case GLINK_CMD_INTENT:
		return param2 * sizeof(struct qpg_intent_pair);
	case GLINK_CMD_VERSION:
	case GLINK_CMD_VERSION_ACK:
	case GLINK_CMD_OPEN_ACK:
	case GLINK_CMD_RX_DONE:
	case GLINK_CMD_RX_INTENT_REQ:
	case GLINK_CMD_RX_INTENT_REQ_ACK:
	case GLINK_CMD_RX_DONE_W_REUSE:
	case GLINK_CMD_READ_NOTIF:
		return 0;
	default:
		return 0;
	}
}

static int qpg_rx_packet_len(struct qpg *pg, size_t avail, u16 cmd,
			     u32 param2, size_t *header_lenp,
			     size_t *payload_lenp)
{
	struct {
		struct qpg_msg msg;
		__le32 chunk_size;
		__le32 left_size;
	} __packed data_hdr;
	size_t header_len = sizeof(struct qpg_msg);
	size_t payload_len = qpg_rx_payload_len(cmd, param2);
	size_t aligned_len;

	if (cmd == GLINK_CMD_TX_DATA || cmd == GLINK_CMD_TX_DATA_CONT) {
		header_len = sizeof(data_hdr);
		if (avail < header_len) {
			log_warning("pmic-glink: short data header avail=%zu header_len=%zu cmd=%u\n",
				    avail, header_len, cmd);
			return -EAGAIN;
		}

		qpg_rx_peek(pg, &data_hdr, 0, sizeof(data_hdr));
		payload_len = le32_to_cpu(data_hdr.chunk_size);
	}

	if (payload_len > avail - header_len) {
		log_warning("pmic-glink: bad payload_len=%zu avail=%zu cmd=%u\n",
			    payload_len, avail, cmd);
		return -EINVAL;
	}

	aligned_len = ALIGN(header_len + payload_len, 8);
	if (aligned_len > avail) {
		log_warning("pmic-glink: bad aligned_len=%zu payload_len=%zu avail=%zu cmd=%u\n",
			    aligned_len, payload_len, avail, cmd);
		return -EINVAL;
	}

	*header_lenp = header_len;
	*payload_lenp = payload_len;
	return 0;
}

static int qpg_poll(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg_msg msg;
	__le32 raw[4] = {};
	size_t avail;
	size_t header_len;
	size_t payload_len;
	size_t raw_len;
	u32 tail;
	u16 cmd, param1;
	u32 param2;
	int ret = 0;

	/* Progress the IPCRTR (qrtr/servreg) channel handshake opportunistically. */
	qpg_service_ipcrtr(pg);

	avail = qpg_rx_avail(pg);
	if (avail < sizeof(msg))
		return -EAGAIN;

	tail = le32_to_cpu(*pg->rx_tail);
	raw_len = min_t(size_t, avail, sizeof(raw));
	qpg_rx_peek(pg, raw, 0, raw_len);
	if (avail >= 8)
		log_debug("pmic-glink: RX raw off=%u avail=%zu h0=%08x h1=%08x\n",
				tail, avail,
				le32_to_cpu(raw[0]),
				le32_to_cpu(raw[1]));
	else
		log_debug("pmic-glink: RX raw off=%u avail=%zu too short\n",
				tail, avail);

	qpg_rx_peek(pg, &msg, 0, sizeof(msg));
	cmd = le16_to_cpu(msg.cmd);
	param1 = le16_to_cpu(msg.param1);
	param2 = le32_to_cpu(msg.param2);

	ret = qpg_rx_packet_len(pg, avail, cmd, param2, &header_len,
				&payload_len);
	if (ret)
		return ret;

	log_debug("pmic-glink: RX cmd=%u param1=%u param2=%u avail=%zu\n",
		    cmd, param1, param2, avail);
	log_debug("pmic-glink: RX header cmd=%u param1=%u param2=%u header_len=%zu payload_len=%zu avail=%zu\n",
		    cmd, param1, param2, header_len, payload_len, avail);

	switch (cmd) {
	case GLINK_CMD_VERSION:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		qpg_send_simple(pg, GLINK_CMD_VERSION_ACK, GLINK_VERSION_1,
				param2 & GLINK_FEATURE_INTENT_REUSE);
		break;
	case GLINK_CMD_VERSION_ACK:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		pg->version_acked = true;
		break;
	case GLINK_CMD_OPEN:
	{
		char name[32] = {};
		size_t copy_len;

		copy_len = min_t(size_t, payload_len, sizeof(name) - 1);
		qpg_rx_peek(pg, name, header_len, copy_len);
		qpg_rx_advance(pg, ALIGN(header_len + payload_len, 8));
		ret = qpg_handle_open(pg, param1, name, param2);
		break;
	}
	case GLINK_CMD_OPEN_ACK:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		if (param1 == pg->lcid) {
			pg->open_acked = true;
			log_warning("pmic-glink: raw OPEN_ACK complete lcid=%u\n",
				    pg->lcid);
		} else if (param1 == QPG_IPCRTR_LCID) {
			pg->ipcrtr_open_acked = true;
			log_warning("pmic-glink: IPCRTR OPEN_ACK complete lcid=%u\n",
				    QPG_IPCRTR_LCID);
		}
		break;
	case GLINK_CMD_INTENT:
	{
		struct qpg_intent_pair intent = {};

		if (!param2) {
			ret = -EINVAL;
			break;
		}

		qpg_rx_peek(pg, &intent, header_len, sizeof(intent));
		qpg_rx_advance(pg, ALIGN(header_len + payload_len, 8));
		ret = qpg_handle_intent(pg, param1, param2, &intent);
		break;
	}
	case GLINK_CMD_TX_DATA:
	case GLINK_CMD_TX_DATA_CONT:
		ret = qpg_rx_data(pg, altmode, avail);
		break;
	case GLINK_CMD_RX_DONE:
	case GLINK_CMD_RX_DONE_W_REUSE:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		if (pg->remote_opened && param1 == pg->rcid &&
		    param2 == pg->riid)
			pg->riid_avail = cmd == GLINK_CMD_RX_DONE_W_REUSE;
		break;
	case GLINK_CMD_RX_INTENT_REQ:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		ret = qpg_handle_intent_req(pg, param1, param2);
		break;
	case GLINK_CMD_RX_INTENT_REQ_ACK:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		log_warning("pmic-glink: RX_INTENT_REQ_ACK cid=%u granted=%u\n",
			    param1, param2);
		break;
	case GLINK_CMD_READ_NOTIF:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		qpg_kick(pg);
		break;
	default:
		log_warning("pmic-glink: unhandled RX cmd=%u param1=%u param2=%u\n",
			    cmd, param1, param2);
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		break;
	}

	if (!ret)
		log_debug("pmic-glink: RX handled cmd=%u rcid=%u done\n",
			    cmd, pg->rcid);

	return ret;
}

static int qpg_service_pmic_open(struct qpg *pg)
{
	int ret;

	if (!pg->remote_opened)
		return 0;

	if (pg->remote_open_ack_pending) {
		log_warning("pmic-glink: service PMIC OPEN_ACK begin rcid=%u\n",
			    pg->rcid);
		ret = qpg_send_open_ack(pg, pg->rcid, QPG_CHANNEL_NAME);
		log_warning("pmic-glink: service PMIC OPEN_ACK end ret=%d\n",
			    ret);
		if (ret)
			return ret;

		pg->remote_open_ack_pending = false;
		pg->remote_open_acked = true;
	}

	if (!pg->local_open_sent) {
		log_warning("pmic-glink: service PMIC local OPEN begin lcid=%u\n",
			    pg->lcid);
		ret = qpg_send_open(pg);
		log_warning("pmic-glink: service PMIC local OPEN end ret=%d\n",
			    ret);
		if (ret)
			return ret;

		pg->local_open_sent = true;
	}

	return 0;
}

/*
 * Progress the IPCRTR (qrtr/servreg) channel handshake: OPEN_ACK the ADSP's
 * open, send our local OPEN, and once the ADSP ACKs it post an RX intent so the
 * ADSP can push qrtr packets to us. Called every poll cycle; cheap no-op until
 * the ADSP advertises IPCRTR.
 */
static int qpg_service_ipcrtr(struct qpg *pg)
{
	int ret;

	if (!pg->ipcrtr_seen)
		return 0;

	if (pg->ipcrtr_open_ack_pending) {
		ret = qpg_send_open_ack(pg, pg->ipcrtr_rcid, QPG_IPCRTR_NAME);
		log_warning("pmic-glink: IPCRTR OPEN_ACK sent rcid=%u ret=%d\n",
			    pg->ipcrtr_rcid, ret);
		if (ret)
			return ret;
		pg->ipcrtr_open_ack_pending = false;
	}

	if (!pg->ipcrtr_local_open_sent) {
		ret = qpg_send_open_for(pg, QPG_IPCRTR_LCID, QPG_IPCRTR_NAME);
		log_warning("pmic-glink: IPCRTR local OPEN sent lcid=%u ret=%d\n",
			    QPG_IPCRTR_LCID, ret);
		if (ret)
			return ret;
		pg->ipcrtr_local_open_sent = true;
	}

	if (pg->ipcrtr_open_acked && !pg->ipcrtr_intent_posted) {
		pg->ipcrtr_liid = qpg_alloc_liid(pg);
		ret = qpg_send_rx_intent_for_size(pg, QPG_IPCRTR_LCID,
						  pg->ipcrtr_liid,
						  QPG_IPCRTR_INTENT_SIZE);
		log_warning("pmic-glink: IPCRTR post RX intent lcid=%u liid=%u ret=%d\n",
			    QPG_IPCRTR_LCID, pg->ipcrtr_liid, ret);
		if (ret)
			return ret;
		pg->ipcrtr_intent_posted = true;
	}

	/* Reply to the ADSP's qrtr HELLO once we have a remote intent to send into. */
	if (pg->ipcrtr_hello_pending && !pg->ipcrtr_hello_sent &&
	    qpg_ipcrtr_riid_ready(pg)) {
		ret = qpg_qrtr_send_hello(pg);
		if (!ret) {
			pg->ipcrtr_hello_sent = true;
			pg->ipcrtr_hello_pending = false;
		}
	}

	/* Register as a servreg listener for charger_pd (kernel-pdr behaviour). */
	if (pg->servreg_register_pending && !pg->servreg_registered &&
	    pg->ipcrtr_hello_sent && qpg_ipcrtr_riid_ready(pg)) {
		ret = qpg_servreg_register(pg);
		if (!ret)
			pg->servreg_register_pending = false;
	}

	/* Ack any servreg state indication. */
	if (pg->servreg_ack_pending && qpg_ipcrtr_riid_ready(pg)) {
		ret = qpg_servreg_send_ack(pg, pg->servreg_ack_txn);
		if (!ret)
			pg->servreg_ack_pending = false;
	}

	return 0;
}

static int qpg_drain_until(struct qpg *pg,
			   struct qcom_pmic_glink_altmode *altmode,
			   bool (*done)(struct qpg *,
					struct qcom_pmic_glink_altmode *),
			   u32 timeout_ms)
{
	u32 i;

	for (i = 0; i < timeout_ms; i++) {
		while (qpg_poll(pg, altmode) == 0) {
			if (done(pg, altmode))
				return 0;
		}

		if (done(pg, altmode))
			return 0;

		udelay(1000);
	}

	return -ETIMEDOUT;
}

static bool qpg_done_version(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode)
{
	return pg->version_acked;
}

static bool qpg_done_remote_opened(struct qpg *pg,
				  struct qcom_pmic_glink_altmode *altmode)
{
	return pg->remote_opened;
}

static bool qpg_done_open(struct qpg *pg,
			  struct qcom_pmic_glink_altmode *altmode)
{
	return pg->local_open_sent &&
	       pg->open_acked;
}

static bool qpg_done_riid(struct qpg *pg,
			  struct qcom_pmic_glink_altmode *altmode)
{
	return pg->riid_avail;
}

static bool qpg_done_pan_ack(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode)
{
	return pg->pan_acked;
}

static bool qpg_done_ucsi_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ucsi_read_acked;
}

static bool qpg_done_ucsi_write(struct qpg *pg,
				struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ucsi_write_acked;
}

static bool qpg_done_usbc_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode)
{
	return pg->usbc_read_acked;
}

static int qpg_wait_riid(struct qpg *pg)
{
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	log_debug("pmic-glink: wait RIID begin riid_avail=%d riid=%u riid_size=%u\n",
		    pg->riid_avail, pg->riid, pg->riid_size);

	ret = qpg_drain_until(pg, &altmode, qpg_done_riid, 500);

	log_debug("pmic-glink: wait RIID end ret=%d riid_avail=%d riid=%u riid_size=%u\n",
		    ret, pg->riid_avail, pg->riid, pg->riid_size);

	return ret;
}

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
static int qpg_register_battmgr(struct qpg *pg)
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

	log_warning("pmic-glink: BATTMGR register: SET_OPERATIONAL_MODE\n");
	pg->battmgr_acked = false;
	ret = qpg_send_data(pg, &opmode, sizeof(opmode));
	if (ret) {
		log_warning("pmic-glink: BATTMGR opmode send ret=%d\n", ret);
		return ret;
	}
	ret = qpg_drain_until(pg, &altmode, qpg_done_battmgr, 1000);
	log_warning("pmic-glink: BATTMGR opmode ack ret=%d acked=%d\n",
		    ret, pg->battmgr_acked);

	log_warning("pmic-glink: BATTMGR register: SET_NOTIFICATION_CRITERIA\n");
	pg->battmgr_acked = false;
	ret = qpg_send_data(pg, &crit, sizeof(crit));
	if (ret) {
		log_warning("pmic-glink: BATTMGR crit send ret=%d\n", ret);
		return ret;
	}
	ret = qpg_drain_until(pg, &altmode, qpg_done_battmgr, 1000);
	log_warning("pmic-glink: BATTMGR crit ack ret=%d acked=%d\n",
		    ret, pg->battmgr_acked);

	return 0;
}

static int qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg)
{
	struct qpg_usbc_write_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USBC_PAN),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(USBC_CMD_WRITE_REQ),
		.cmd = cpu_to_le32(cmd),
		.arg = cpu_to_le32(arg),
	};

	log_warning("pmic-glink: owner=%u channel=%s altmode_cmd=%u arg=%u\n",
		    PMIC_GLINK_OWNER_USBC_PAN, QPG_CHANNEL_NAME, cmd, arg);

	return qpg_send_data(pg, &req, sizeof(req));
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

	log_warning("pmic-glink: owner=%u channel=%s UCSI_READ_BUFFER_REQ\n",
		    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME);

	ret = qpg_send_data(pg, &req, sizeof(req));
	log_warning("pmic-glink: send UCSI_READ_BUFFER_REQ ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, &altmode, qpg_done_ucsi_read, 1000);
	log_warning("pmic-glink: wait UCSI_READ_BUFFER ret=%d ack=%d\n",
		    ret, pg->ucsi_read_acked);

	return ret;
}

static void qpg_log_ucsi_raw(struct qpg *pg, const char *label)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 cci = get_unaligned_le32(buf + 4);
	u8 len = (cci & UCSI_CCI_DATA_LENGTH_MASK) >> UCSI_CCI_DATA_LENGTH_SHIFT;

	log_warning("pmic-glink: UCSI %s ret=%u cci=%08x len=%u raw=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
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

	log_warning("pmic-glink: UCSI connector%u ret=%u cci=%08x status=%08x connected=%u pwr_dir=%u usb=%u altmode=%u partner=%u opmode=%u rdo=%08x\n",
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

	log_warning("pmic-glink: UCSI CAPABILITY attr=%08x connectors=%u optional=%06x altmodes=%u\n",
		    attr, connectors, optional, altmodes);
	qpg_log_ucsi_raw(pg, "CAPABILITY");
}

static void qpg_log_ucsi_connector_capability(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u8 opmode = buf[16];
	bool provider = buf[17] & BIT(0);
	bool consumer = buf[17] & BIT(1);

	log_warning("pmic-glink: UCSI connector%u CAP opmode=%02x altmode=%u usb3=%u usb2=%u drp=%u provider=%u consumer=%u\n",
		    port + 1, opmode, !!(opmode & BIT(7)), !!(opmode & BIT(6)),
		    !!(opmode & BIT(5)), !!(opmode & BIT(2)), provider,
		    consumer);
	qpg_log_ucsi_raw(pg, "CONNECTOR_CAP");
}

static void qpg_log_ucsi_cam_supported(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;
	u32 bitmap = get_unaligned_le32(buf + 16);

	log_warning("pmic-glink: UCSI connector%u CAM_SUPPORTED bitmap=%08x\n",
		    port + 1, bitmap);
	qpg_log_ucsi_raw(pg, "CAM_SUPPORTED");
}

static void qpg_log_ucsi_current_cam(struct qpg *pg, u8 port)
{
	const u8 *buf = pg->ucsi_read_buffer;

	log_warning("pmic-glink: UCSI connector%u CURRENT_CAM=%02x\n",
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

	log_warning("pmic-glink: UCSI connector%u ALT_MODE off=%u len=%u mode0=svid:%04x mid:%08x mode1=svid:%04x mid:%08x\n",
		    port + 1, offset, len, svid0, mid0, svid1, mid1);
	qpg_log_ucsi_raw(pg, "ALT_MODE");
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
		log_warning("pmic-glink: owner=%u channel=%s UCSI_WRITE_BUFFER_REQ cmd=%u notify_mask=%04x\n",
			    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME,
			    req.write_buffer[8],
			    get_unaligned_le16(req.write_buffer + 10));
	} else {
		log_warning("pmic-glink: owner=%u channel=%s UCSI_WRITE_BUFFER_REQ cmd=%u b10=%u b11=%u b12=%u b13=%u\n",
			    PMIC_GLINK_OWNER_USB_TYPE_C, QPG_CHANNEL_NAME,
			    req.write_buffer[8], req.write_buffer[10],
			    req.write_buffer[11], req.write_buffer[12],
			    req.write_buffer[13]);
	}

	ret = qpg_send_data(pg, &req, sizeof(req));
	log_warning("pmic-glink: send UCSI_WRITE_BUFFER_REQ ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(pg, &altmode, qpg_done_ucsi_write, 1000);
	log_warning("pmic-glink: wait UCSI_WRITE_BUFFER ret=%d ack=%d code=%u\n",
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
			log_warning("pmic-glink: UCSI command CCI ready cci=%08x notify_seen=%d notify=%08x\n",
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
	log_warning("pmic-glink: UCSI ACK_CC_CI ret=%d connector=%d command=%d\n",
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
			log_warning("pmic-glink: UCSI ACK_CC_CI complete cci=%08x\n",
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
			log_warning("pmic-glink: UCSI PPM_RESET complete cci=%08x\n",
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

static int qpg_send_ucsi_get_connector_status(struct qpg *pg, u8 port)
{
	int ret;

	ret = qpg_send_ucsi_command(pg, UCSI_CMD_GET_CONNECTOR_STATUS, port,
				    0, true);
	if (!ret)
		qpg_log_ucsi_connector_status(pg, port);

	return ret;
}

/*
 * UCSI CONNECTOR_RESET (command 0x03).  The control word byte 2 holds the
 * connector number in bits[0:6] and the Hard Reset flag in bit[7], i.e.
 * d2 (bits 16-31 of the LE control u64) = (connector | hard<<7).
 *
 * On this platform the Type-C/PD policy engine and the DisplayPort Enter_Mode
 * VDM run autonomously on the ADSP; there is no AP "enter DP" opcode.  A
 * connector reset is the one AP-driven lever that forces the partner to
 * re-attach so the ADSP re-runs PD negotiation + alt-mode discovery + DP
 * Enter_Mode, this time with PAN notifications enabled and ACKed.
 */
static int qpg_send_ucsi_connector_reset(struct qpg *pg, u8 port, bool hard)
{
	u16 d2 = (u16)((port + 1) & 0x7f) | (hard ? 0x80 : 0);
	u64 control = UCSI_CTRL_D2(UCSI_CMD_CONNECTOR_RESET, d2);
	u32 old_cci = qpg_ucsi_cci(pg);
	int ret;

	log_warning("pmic-glink: UCSI CONNECTOR_RESET connector=%u hard=%d\n",
		    port + 1, hard);

	pg->ucsi_notify_seen = false;
	pg->ucsi_notification = 0;

	ret = qpg_ucsi_send_control(pg, control);
	if (ret)
		return ret;

	ret = qpg_wait_ucsi_cci(pg, old_cci, 5000);
	log_warning("pmic-glink: UCSI CONNECTOR_RESET cci ret=%d cci=%08x\n",
		    ret, qpg_ucsi_cci(pg));
	if (ret)
		return ret;

	return qpg_send_ucsi_ack_cc_ci(pg, qpg_ucsi_connector_change(pg), true);
}

/*
 * UCSI SET_UOR (command 0x09) — set USB Operation Role.  Control byte layout
 * (per the ADSP ucsi.h): ConnectorNumber bits[22:16], USBOpRoleDFP bit[23],
 * USBOpRoleUFP bit[24], USBOpRoleDualRole bit[25].  So d2 (bits 16-31 of the
 * LE control u64) = connector | (DFP?0x80:UFP?0x100).
 *
 * Why: the connector status reports partner_type=1 (a DFP is attached), i.e.
 * the Tachyon is operating as the UFP/device.  A UFP never initiates the
 * DisplayPort Enter_Mode VDM — only a DFP (host) drives DP to a sink.  This
 * requests a data-role swap to DFP so the ADSP DPM will run DP alt-mode.
 */
static int qpg_send_ucsi_set_uor(struct qpg *pg, u8 port, bool dfp)
{
	u16 d2 = (u16)((port + 1) & 0x7f) | (dfp ? 0x80 : 0x100);
	u64 control = UCSI_CTRL_D2(UCSI_CMD_SET_UOR, d2);
	u32 old_cci = qpg_ucsi_cci(pg);
	int ret;

	log_warning("pmic-glink: UCSI SET_UOR connector=%u role=%s\n",
		    port + 1, dfp ? "DFP" : "UFP");

	pg->ucsi_notify_seen = false;
	pg->ucsi_notification = 0;

	ret = qpg_ucsi_send_control(pg, control);
	if (ret)
		return ret;

	ret = qpg_wait_ucsi_cci(pg, old_cci, 5000);
	log_warning("pmic-glink: UCSI SET_UOR cci ret=%d cci=%08x\n",
		    ret, qpg_ucsi_cci(pg));
	if (ret)
		return ret;

	return qpg_send_ucsi_ack_cc_ci(pg, qpg_ucsi_connector_change(pg), true);
}

/*
 * Service a pending UCSI connector-change notification by reading connector
 * status (which ACKs the change via ACK_CC_CI).  If the AP never drains and
 * ACKs connector-changes the PPM stays busy with the change pending and the
 * ADSP can stop emitting further alt-mode (DP) updates, so this must run
 * whenever we are waiting for DP to come up.  Returns true if serviced.
 */
static bool qpg_service_ucsi_change(struct qpg *pg)
{
	if (!pg->ucsi_notify_seen)
		return false;

	pg->ucsi_notify_seen = false;
	log_warning("pmic-glink: UCSI connector-change cci=%08x; reading status\n",
		    pg->ucsi_notification);
	qpg_send_ucsi_get_connector_status(pg, 0);

	return true;
}

static int qpg_enable_ucsi_notifications_phase2(struct qpg *pg)
{
	log_warning("pmic-glink: UCSI SET_NOTIFICATION_ENABLE phase2 mask=%04x\n",
		    QPG_UCSI_NTFY_ALL);
	return qpg_send_ucsi_command(pg, UCSI_CMD_SET_NOTIFICATION_ENABLE, 0,
				     QPG_UCSI_NTFY_ALL, true);
}

static int qpg_enable_ucsi_notifications(struct qpg *pg)
{
	u16 phase1_mask;

	phase1_mask = QPG_UCSI_NTFY_CMD_COMPLETE | QPG_UCSI_NTFY_ERROR;
	log_warning("pmic-glink: UCSI SET_NOTIFICATION_ENABLE phase1 mask=%04x\n",
		    phase1_mask);
	return qpg_send_ucsi_command(pg, UCSI_CMD_SET_NOTIFICATION_ENABLE, 0,
				     phase1_mask, true);
}

static int qpg_ucsi_prewarm(struct qpg *pg)
{
	u16 version;
	int ret;

	if (pg->ucsi_prewarmed)
		return 0;

	if (qpg_env_bool("qpg_skip_ucsi_prewarm")) {
		log_warning("pmic-glink: UCSI prewarm skipped by env\n");
		return 0;
	}

	log_warning("pmic-glink: UCSI prewarm begin\n");

	/* Read UCSI version first (Linux parity: ucsi_register does this) */
	ret = qpg_send_ucsi_read(pg);
	if (!ret) {
		version = qpg_ucsi_version(pg);
		log_warning("pmic-glink: UCSI version = 0x%04x\n", version);
	}

	ret = qpg_send_ucsi_ppm_reset(pg);
	log_warning("pmic-glink: UCSI prewarm PPM_RESET ret=%d\n", ret);
	if (ret) {
		/*
		 * Non-fatal during bring-up: continue with diagnostic UCSI
		 * init so we can see whether later commands succeed after
		 * a reset timeout.
		 */
		log_warning("pmic-glink: PPM_RESET failed, continuing diagnostic UCSI init\n");
	}

	ret = qpg_enable_ucsi_notifications(pg);
	log_warning("pmic-glink: UCSI prewarm notifications phase1 ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_capability(pg);
	log_warning("pmic-glink: UCSI prewarm capability ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_capability(pg, 0);
	log_warning("pmic-glink: UCSI prewarm connector capability ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_send_ucsi_get_connector_status(pg, 0);
	log_warning("pmic-glink: UCSI prewarm connector status ret=%d\n",
		    ret);
	if (ret)
		return ret;

	ret = qpg_enable_ucsi_notifications_phase2(pg);
	log_warning("pmic-glink: UCSI prewarm notifications phase2 ret=%d\n",
		    ret);
	if (ret)
		return ret;

	pg->ucsi_prewarmed = true;
	log_warning("pmic-glink: UCSI prewarm complete\n");

	return 0;
}

static int qpg_init(struct qpg *pg)
{
	ofnode adsp;
	ofnode glink = ofnode_null();
	size_t size;
	__le32 *descs;
	ulong start;
	bool desc_exists;
	bool tx_exists;
	int ret;

	qpg_last_adsp_boot_ret = -EINPROGRESS;

	ret = uclass_first_device_err(UCLASS_SMEM, &pg->smem);
	log_warning("pmic-glink: smem lookup ret=%d smem=%p\n",
		    ret, pg->smem);
	if (ret)
		return ret;

	ret = qcom_adsp_pas_boot();
	qpg_last_adsp_boot_ret = ret;
	log_warning("pmic-glink: ADSP PAS boot ret=%d\n", ret);
	if (ret)
		return ret;

	adsp = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-adsp-pas");
	if (ofnode_valid(adsp))
		ofnode_for_each_subnode(glink, adsp) {
			const char *label = ofnode_read_string(glink, "label");

			if (label && (!strcmp(label, "adsp") ||
				      !strcmp(label, "lpass")))
				break;
		}
	log_warning("pmic-glink: adsp node valid=%d glink node valid=%d\n",
		    ofnode_valid(adsp), ofnode_valid(glink));
	if (!ofnode_valid(adsp)) {
		log_warning("pmic-glink: missing qcom,sc7280-adsp-pas node\n");
		return -ENOENT;
	}

	if (!ofnode_valid(glink)) {
		log_warning("pmic-glink: missing ADSP GLINK edge node\n");
		return -ENOENT;
	}

	ret = ofnode_read_u32(glink, "qcom,remote-pid", &pg->remote_pid);
	if (ret) {
		log_warning("pmic-glink: missing qcom,remote-pid ret=%d\n", ret);
		return ret;
	}

	/*
	 * Get IPCC mailbox channel via DT lookup.
	 * glink-edge DT node: mboxes = <&ipcc IPCC_CLIENT_LPASS IPCC_MPROC_SIGNAL_GLINK_QMP>;
	 * Routes through the qcom-ipcc mailbox driver (no more hardcoded MMIO).
	 */
	ret = qpg_mbox_from_glink(glink, &pg->mbox_chan);
	if (ret) {
		log_warning("pmic-glink: failed to get IPCC mbox ret=%d\n", ret);
		return ret;
	}

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 32);
	desc_exists = ret == -EEXIST;
	log_warning("pmic-glink: smem_alloc desc ret=%d\n", ret);
	if (ret && ret != -EEXIST)
		return ret;

	descs = smem_get(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 &size);
	log_warning("pmic-glink: descs=%p size=%zu\n", descs, size);
	if (!descs || size != 32)
		return -EINVAL;

	pg->tx_tail = &descs[0];
	pg->tx_head = &descs[1];
	pg->rx_tail = &descs[2];
	pg->rx_head = &descs[3];

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_FIFO_0,
			 SZ_16K);
	tx_exists = ret == -EEXIST;
	log_warning("pmic-glink: smem_alloc tx fifo ret=%d\n", ret);
	if (ret && ret != -EEXIST)
		return ret;

	pg->tx_fifo = smem_get(pg->smem, pg->remote_pid,
			       QPG_SMEM_XPRT_FIFO_0, &pg->tx_len);
	start = get_timer(0);
	do {
		pg->rx_fifo = smem_get(pg->smem, pg->remote_pid,
				       QPG_SMEM_XPRT_FIFO_1, &pg->rx_len);
		if (!IS_ERR_OR_NULL(pg->rx_fifo))
			break;
		mdelay(20);
	} while (get_timer(start) < 2000);
	log_warning("pmic-glink: tx_fifo=%p tx_len=%zu rx_fifo=%p rx_len=%zu\n",
		    pg->tx_fifo, pg->tx_len, pg->rx_fifo, pg->rx_len);
	if (IS_ERR_OR_NULL(pg->tx_fifo) || IS_ERR_OR_NULL(pg->rx_fifo)) {
		log_warning("pmic-glink: missing GLINK FIFO remote_pid=%u tx_ok=%d rx_ok=%d\n",
			    pg->remote_pid, !IS_ERR_OR_NULL(pg->tx_fifo),
			    !IS_ERR_OR_NULL(pg->rx_fifo));
		return -ENOENT;
	}

	if (!desc_exists && !tx_exists) {
		*pg->rx_tail = 0;
		*pg->tx_head = 0;
	} else {
		log_warning("pmic-glink: preserving existing fifo ptrs\n");
	}

	pg->lcid = 1;
	log_warning("pmic-glink: fifo ptrs tx_tail=%08x tx_head=%08x rx_tail=%08x rx_head=%08x\n",
		    le32_to_cpu(*pg->tx_tail), le32_to_cpu(*pg->tx_head),
		    le32_to_cpu(*pg->rx_tail), le32_to_cpu(*pg->rx_head));

	return 0;
}

static int qpg_open_session(struct qcom_pmic_glink_altmode *altmode,
			    int *adsp_boot_retp, int *glink_open_retp,
			    int *pan_en_retp)
{
	u32 liid;
	int ret;

	if (adsp_boot_retp)
		*adsp_boot_retp = -EINPROGRESS;
	if (glink_open_retp)
		*glink_open_retp = -EINPROGRESS;
	if (pan_en_retp)
		*pan_en_retp = -EINPROGRESS;

	if (qpg_session_ready) {
		log_warning("pmic-glink: reusing session lcid=%u rcid=%u rx_tail=%08x rx_head=%08x\n",
			    qpg_session.lcid, qpg_session.rcid,
			    le32_to_cpu(*qpg_session.rx_tail),
			    le32_to_cpu(*qpg_session.rx_head));
		qpg_cached_state.service_started = true;
		qpg_cached_state.pan_enabled = true;
		if (adsp_boot_retp)
			*adsp_boot_retp = 0;
		if (glink_open_retp)
			*glink_open_retp = 0;
		if (pan_en_retp)
			*pan_en_retp = 0;
		return 0;
	}

	memset(&qpg_session, 0, sizeof(qpg_session));
	memset(&qpg_cached_state, 0, sizeof(qpg_cached_state));
	qpg_last_ucsi_prewarm_ret = -EINPROGRESS;

	ret = qpg_init(&qpg_session);
	log_warning("pmic-glink: qpg_init ret=%d\n", ret);
	if (adsp_boot_retp)
		*adsp_boot_retp = qpg_last_adsp_boot_ret;
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_send_version(&qpg_session);
	log_warning("pmic-glink: send VERSION ret=%d\n", ret);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(&qpg_session, altmode, qpg_done_version, 1000);
	log_warning("pmic-glink: wait VERSION_ACK ret=%d version_acked=%d\n",
		    ret, qpg_session.version_acked);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	/*
	 * Linux-aligned channel open (two-phase):
	 * 1. Wait for remote (ADSP) to advertise PMIC_RTR_ADSP_APPS via
	 *    GLINK_CMD_OPEN.
	 * 2. Once seen, send OPEN_ACK + local OPEN in a single servicing
	 *    step.
	 * 3. Wait for local OPEN_ACK from the remote.
	 *
	 * We never send local OPEN before the remote has advertised the
	 * channel.
	 */
	ret = qpg_drain_until(&qpg_session, altmode, qpg_done_remote_opened,
			      2000);
	log_warning("pmic-glink: wait remote OPEN ret=%d remote_opened=%d rcid=%u\n",
		    ret, qpg_session.remote_opened, qpg_session.rcid);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_service_pmic_open(&qpg_session);
	log_warning("pmic-glink: service PMIC OPEN ret=%d local_sent=%d remote_acked=%d\n",
		    ret, qpg_session.local_open_sent,
		    qpg_session.remote_open_acked);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(&qpg_session, altmode, qpg_done_open, 2000);
	log_warning("pmic-glink: wait local OPEN_ACK ret=%d open_acked=%d\n",
		    ret, qpg_session.open_acked);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	liid = qpg_alloc_liid(&qpg_session);
	ret = qpg_send_rx_intent_for(&qpg_session, qpg_session.lcid, liid);
	log_warning("pmic-glink: send RX_INTENT ret=%d lcid=%u liid=%u\n",
		    ret, qpg_session.lcid, liid);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}
	if (glink_open_retp)
		*glink_open_retp = 0;

	ret = qpg_ucsi_prewarm(&qpg_session);
	qpg_last_ucsi_prewarm_ret = ret;
	if (ret)
		log_warning("pmic-glink: UCSI prewarm failed ret=%d; continuing PAN\n",
			    ret);

	qpg_session.pan_acked = false;
	ret = qpg_send_altmode_req(&qpg_session, ALTMODE_PAN_EN, 0);
	log_warning("pmic-glink: send PAN_EN ret=%d\n", ret);
	if (ret) {
		if (pan_en_retp)
			*pan_en_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(&qpg_session, altmode, qpg_done_pan_ack, 1000);
	log_warning("pmic-glink: wait PAN_ACK ret=%d pan_acked=%d\n",
		    ret, qpg_session.pan_acked);
	if (pan_en_retp)
		*pan_en_retp = ret;
	if (ret)
		return ret;

	/*
	 * Best-effort: register the battmgr/charger client so the ADSP
	 * "battman" firmware (which also hosts the Type-C/PD/alt-mode stack)
	 * sees the full set of host clients Linux brings up. Experiment to
	 * see whether this unblocks DP alt-mode entry.
	 */
	qpg_register_battmgr(&qpg_session);

	qpg_session_ready = true;
	qpg_cached_state.service_started = true;
	qpg_cached_state.pan_enabled = true;

	return 0;
}

int qcom_pmic_glink_altmode_start(void)
{
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	log_warning("qpg: service start\n");

	ret = qpg_open_session(&altmode, NULL, NULL, NULL);
	log_warning("qpg: service start ret=%d\n", ret);
	if (ret)
		return ret;

	qpg_cached_state.service_started = true;
	qpg_cached_state.pan_enabled = true;

	return 0;
}

int qcom_pmic_glink_altmode_poll(struct qcom_pmic_glink_altmode_state *state,
				 uint timeout_ms)
{
	struct qcom_pmic_glink_altmode altmode = {};
	ulong start;
	bool progressed = false;
	int ret;

	if (!qpg_session_ready)
		return -ENODEV;

	start = get_timer(0);
	for (;;) {
		ret = qpg_poll(&qpg_session, &altmode);
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
				*state = qpg_cached_state;
			return qpg_cached_state.notify_seen ? 0 : -ETIMEDOUT;
		}

		udelay(1000);
	}

	qpg_cached_state.service_started = qpg_session_ready;
	if (state)
		*state = qpg_cached_state;

	return 0;
}

const struct qcom_pmic_glink_altmode_state *
qcom_pmic_glink_altmode_get_state(void)
{
	qpg_cached_state.service_started = qpg_session_ready;

	return &qpg_cached_state;
}

bool qcom_pmic_glink_altmode_hpd_asserted(void)
{
	return qpg_cached_state.hpd;
}

int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	int refresh_ret;
	int ret;

	if (!altmode)
		return -EINVAL;

	memset(altmode, 0, sizeof(*altmode));

	log_warning("pmic-glink: get_altmode start\n");

	ret = qcom_pmic_glink_altmode_start();
	if (ret)
		return ret;

	ret = qcom_pmic_glink_altmode_poll(NULL, 0);
	if (ret && ret != -ETIMEDOUT)
		return ret;

	if (!qpg_cached_altmode_valid) {
		refresh_ret = qpg_refresh_usbc_pin_assignment(&qpg_session,
							      altmode);
		log_warning("pmic-glink: USBC pin refresh ret=%d dp=%d orientation=%u pin=%u hpd=%d irq=%d\n",
			    refresh_ret, altmode->dp, altmode->orientation,
			    altmode->pin_assignment, altmode->hpd,
			    altmode->hpd_irq);
		if (refresh_ret)
			return refresh_ret;
	}

	if (qpg_cached_altmode_valid)
		*altmode = qpg_cached_altmode;

	log_warning("pmic-glink: get_altmode ret=0 dp=%d orientation=%u pin=%u hpd=%d irq=%d state=%s\n",
		    altmode->dp, altmode->orientation,
		    altmode->pin_assignment, altmode->hpd,
		    altmode->hpd_irq,
		    qpg_public_typec_state_name(qpg_cached_state.typec_state));

	return qpg_cached_altmode_valid ? 0 : -EAGAIN;
}

static int do_qpg_service(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	struct qcom_pmic_glink_altmode_state state = {};
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	ulong start;
	ulong last_notify_ms = 0;
	u32 timeout_ms = QPG_ALTMODE_TIMEOUT_MS;
	int ret;

	if (argc > 2)
		return CMD_RET_USAGE;
	if (argc == 2)
		timeout_ms = simple_strtoul(argv[1], NULL, 0);
	if (!timeout_ms)
		timeout_ms = QPG_ALTMODE_TIMEOUT_MS;

	printf("qpg: service start\n");
	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);
	qpg_cached_state.service_started = !ret;
	qpg_cached_state.pan_enabled = !ret;

	printf("qpg: ADSP boot ret=%d\n", adsp_ret);
	printf("qpg: GLINK open ret=%d\n", open_ret);
	printf("qpg: UCSI prewarm ret=%d\n", qpg_last_ucsi_prewarm_ret);
	printf("qpg: PAN_EN ret=%d\n", pan_ret);
	if (ret)
		return CMD_RET_FAILURE;

	start = get_timer(0);
	while (get_timer(start) < timeout_ms) {
		ret = qcom_pmic_glink_altmode_poll(&state, 20);
		if (ret && ret != -ETIMEDOUT)
			break;

		/* Keep the PPM unstuck so the ADSP keeps emitting updates. */
		qpg_service_ucsi_change(&qpg_session);

		if (state.notify_seen && state.last_notify_ms != last_notify_ms) {
			last_notify_ms = state.last_notify_ms;
			printf("t=%05lu notify %s svid=%04x orient_raw=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
			       get_timer(start),
			       qpg_public_typec_state_name(state.typec_state),
			       state.svid, state.orientation_raw, state.mux,
			       state.dpam_raw, state.linux_mux_mode,
			       state.dp_pin_assignment, state.hpd,
			       state.hpd_irq);
			printf("t=%05lu state port=%u orient=%u pin=%u dp_seen=%u\n",
			       get_timer(start), state.port, state.orientation,
			       state.pin_assignment, state.dp_seen);
		}
	}

	if (ret && ret != -ETIMEDOUT)
		printf("qpg: service fatal ret=%d\n", ret);
	else
		printf("qpg: service timeout after %u ms\n", timeout_ms);

	return ret && ret != -ETIMEDOUT ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_qpg_reset(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	struct qcom_pmic_glink_altmode_state state = {};
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	ulong start;
	ulong last_notify_ms = 0;
	u32 timeout_ms = QPG_ALTMODE_TIMEOUT_MS;
	bool hard = false;
	int ret;

	if (argc > 2)
		return CMD_RET_USAGE;
	if (argc == 2) {
		if (!strcmp(argv[1], "hard"))
			hard = true;
		else
			return CMD_RET_USAGE;
	}

	printf("qpg: reset start hard=%d\n", hard);
	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);
	qpg_cached_state.service_started = !ret;
	qpg_cached_state.pan_enabled = !ret;
	printf("qpg: ADSP boot ret=%d\n", adsp_ret);
	printf("qpg: GLINK open ret=%d\n", open_ret);
	printf("qpg: UCSI prewarm ret=%d\n", qpg_last_ucsi_prewarm_ret);
	printf("qpg: PAN_EN ret=%d\n", pan_ret);
	if (ret)
		return CMD_RET_FAILURE;

	/* Snapshot connector state before the reset. */
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	ret = qpg_send_ucsi_connector_reset(&qpg_session, 0, hard);
	printf("qpg: CONNECTOR_RESET ret=%d hard=%d\n", ret, hard);

	/*
	 * After the reset the partner re-attaches and the ADSP re-runs PD
	 * negotiation + alt-mode discovery + DP Enter_Mode.  Drain and ACK
	 * altmode notifications while servicing UCSI connector-changes, and
	 * watch for a DP notify (mux=DP/dpam=pin/hpd=1).
	 */
	start = get_timer(0);
	while (get_timer(start) < timeout_ms) {
		ret = qcom_pmic_glink_altmode_poll(&state, 20);
		if (ret && ret != -ETIMEDOUT)
			break;

		qpg_service_ucsi_change(&qpg_session);

		if (state.notify_seen && state.last_notify_ms != last_notify_ms) {
			last_notify_ms = state.last_notify_ms;
			printf("t=%05lu notify %s svid=%04x orient_raw=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
			       get_timer(start),
			       qpg_public_typec_state_name(state.typec_state),
			       state.svid, state.orientation_raw, state.mux,
			       state.dpam_raw, state.linux_mux_mode,
			       state.dp_pin_assignment, state.hpd,
			       state.hpd_irq);
			printf("t=%05lu state port=%u orient=%u pin=%u dp_seen=%u\n",
			       get_timer(start), state.port, state.orientation,
			       state.pin_assignment, state.dp_seen);
		}
	}

	/* Final connector + alt-mode snapshot. */
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);
	qpg_send_ucsi_get_current_cam(&qpg_session, 0);

	printf("qpg: reset done dp_seen=%u hpd=%u mux=%u dpam=%02x\n",
	       state.dp_seen, state.hpd, state.mux, state.dpam_raw);

	return CMD_RET_SUCCESS;
}

static int do_qpg_dfp(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	struct qcom_pmic_glink_altmode_state state = {};
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	ulong start;
	ulong last_notify_ms = 0;
	u32 timeout_ms = QPG_ALTMODE_TIMEOUT_MS;
	int ret;

	if (argc > 2)
		return CMD_RET_USAGE;
	if (argc == 2)
		timeout_ms = simple_strtoul(argv[1], NULL, 0);
	if (!timeout_ms)
		timeout_ms = QPG_ALTMODE_TIMEOUT_MS;

	printf("qpg: dfp start (request data-role swap to DFP/host)\n");
	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);
	qpg_cached_state.service_started = !ret;
	qpg_cached_state.pan_enabled = !ret;
	printf("qpg: ADSP boot ret=%d\n", adsp_ret);
	printf("qpg: GLINK open ret=%d\n", open_ret);
	printf("qpg: UCSI prewarm ret=%d\n", qpg_last_ucsi_prewarm_ret);
	printf("qpg: PAN_EN ret=%d\n", pan_ret);
	if (ret)
		return CMD_RET_FAILURE;

	/* Snapshot connector role before the swap. */
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	ret = qpg_send_ucsi_set_uor(&qpg_session, 0, true);
	printf("qpg: SET_UOR(DFP) ret=%d\n", ret);

	/* Confirm the swap took effect (partner should now read as UFP). */
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	/*
	 * The DPM runs DisplayPort VDM discovery at attach-as-DFP, not after a
	 * mid-session role swap.  With the DFP preference now set, force a
	 * fresh re-attach so the ADSP re-runs PD + alt-mode discovery as the
	 * DFP/host and (hopefully) enters DP.
	 */
	ret = qpg_send_ucsi_connector_reset(&qpg_session, 0, false);
	printf("qpg: CONNECTOR_RESET-after-DFP ret=%d\n", ret);
	/* Re-assert DFP preference in case the re-attach reverted it. */
	qpg_send_ucsi_set_uor(&qpg_session, 0, true);
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	start = get_timer(0);
	while (get_timer(start) < timeout_ms) {
		ret = qcom_pmic_glink_altmode_poll(&state, 20);
		if (ret && ret != -ETIMEDOUT)
			break;

		qpg_service_ucsi_change(&qpg_session);

		if (state.notify_seen && state.last_notify_ms != last_notify_ms) {
			last_notify_ms = state.last_notify_ms;
			printf("t=%05lu notify %s svid=%04x orient_raw=%u mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u irq=%u\n",
			       get_timer(start),
			       qpg_public_typec_state_name(state.typec_state),
			       state.svid, state.orientation_raw, state.mux,
			       state.dpam_raw, state.linux_mux_mode,
			       state.dp_pin_assignment, state.hpd,
			       state.hpd_irq);
			printf("t=%05lu state port=%u orient=%u pin=%u dp_seen=%u\n",
			       get_timer(start), state.port, state.orientation,
			       state.pin_assignment, state.dp_seen);
		}
	}

	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	printf("qpg: dfp done dp_seen=%u hpd=%u mux=%u dpam=%02x\n",
	       state.dp_seen, state.hpd, state.mux, state.dpam_raw);

	return CMD_RET_SUCCESS;
}

static int do_qpg_ucsi(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	int ret;

	if (argc != 1)
		return CMD_RET_USAGE;

	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);

	printf("ADSP boot: ret=%d\n", adsp_ret);
	printf("GLINK open: ret=%d\n", open_ret);
	printf("UCSI prewarm: ret=%d\n", qpg_last_ucsi_prewarm_ret);
	printf("PAN_EN: ret=%d\n", pan_ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_ppm_reset(&qpg_session);
	printf("UCSI PPM_RESET: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_enable_ucsi_notifications(&qpg_session);
	printf("UCSI notifications phase1: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_capability(&qpg_session);
	printf("UCSI capability: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_connector_capability(&qpg_session, 0);
	printf("UCSI connector capability: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_connector_status(&qpg_session, 0);
	printf("UCSI connector status: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_cam_supported(&qpg_session, 0);
	printf("UCSI CAM_SUPPORTED: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_current_cam(&qpg_session, 0);
	printf("UCSI CURRENT_CAM: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_send_ucsi_get_alternate_mode(&qpg_session, 0, 0, 2);
	printf("UCSI ALT_MODE: ret=%d\n", ret);
	if (ret)
		return CMD_RET_FAILURE;

	ret = qpg_enable_ucsi_notifications_phase2(&qpg_session);
	printf("UCSI notifications phase2: ret=%d\n", ret);

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

/*
 * Settle helper: drain + ACK altmode notifications (and keep the UCSI PPM
 * unstuck) for ms milliseconds, printing any new notify.  Mirrors the passive
 * "sleep" windows in the userspace dp-renegotiate script.
 */
static void qpg_bounce_settle(struct qcom_pmic_glink_altmode_state *state,
			      ulong *last_notify_ms, u32 ms)
{
	ulong start = get_timer(0);

	while (get_timer(start) < ms) {
		int ret = qcom_pmic_glink_altmode_poll(state, 20);

		if (ret && ret != -ETIMEDOUT)
			break;

		qpg_service_ucsi_change(&qpg_session);

		if (state->notify_seen && state->last_notify_ms != *last_notify_ms) {
			*last_notify_ms = state->last_notify_ms;
			printf("t=%05lu notify %s svid=%04x mux=%u dpam=%02x linux_mode=%u dp_pin=%u hpd=%u dp_seen=%u\n",
			       get_timer(start),
			       qpg_public_typec_state_name(state->typec_state),
			       state->svid, state->mux, state->dpam_raw,
			       state->linux_mux_mode, state->dp_pin_assignment,
			       state->hpd, state->dp_seen);
		}
	}
}

/*
 * do_qpg_bounce: faithful replica of the working Linux userspace
 * dp-renegotiate "data-role bounce".  Empirically (journal + kprobe trace) this
 * is what makes the ADSP actually enter DisplayPort alt mode: the ADSP does NOT
 * enter DP autonomously even under Linux; a UCSI SET_UOR bounce UFP -> (settle)
 * -> DFP kicks the DPM into running the DP VDM.  Unlike "qpg dfp" this does NOT
 * issue a CONNECTOR_RESET (which reverts the role to UFP and defeats the bounce).
 */
static int do_qpg_bounce(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	struct qcom_pmic_glink_altmode_state state = {};
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	ulong last_notify_ms = 0;
	u32 settle_ms = 6000;
	int ret;

	if (argc > 2)
		return CMD_RET_USAGE;
	if (argc == 2)
		settle_ms = simple_strtoul(argv[1], NULL, 0);
	if (!settle_ms)
		settle_ms = 6000;

	printf("qpg: bounce start (data-role UFP->DFP, mirrors dp-renegotiate)\n");
	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);
	qpg_cached_state.service_started = !ret;
	qpg_cached_state.pan_enabled = !ret;
	printf("qpg: ADSP boot ret=%d\n", adsp_ret);
	printf("qpg: GLINK open ret=%d\n", open_ret);
	printf("qpg: UCSI prewarm ret=%d\n", qpg_last_ucsi_prewarm_ret);
	printf("qpg: PAN_EN ret=%d\n", pan_ret);
	if (ret)
		return CMD_RET_FAILURE;

	/*
	 * Let the IPCRTR/qrtr HELLO + servreg REGISTER_LISTENER handshake for
	 * charger_pd complete BEFORE the data-role bounce (mirrors Linux: servreg
	 * is up at boot, dp-renegotiate bounces later).
	 */
	qpg_bounce_settle(&state, &last_notify_ms, 4000);
	printf("qpg: servreg notifier_seen=%d registered=%d state=%u\n",
	       qpg_session.servreg_notifier_seen, qpg_session.servreg_registered,
	       qpg_session.servreg_last_state);

	/* Snapshot connector role before the bounce. */
	qpg_send_ucsi_get_connector_status(&qpg_session, 0);

	/* echo device > data_role  : SET_UOR(UFP) */
	ret = qpg_send_ucsi_set_uor(&qpg_session, 0, false);
	printf("qpg: SET_UOR(UFP) ret=%d\n", ret);
	qpg_bounce_settle(&state, &last_notify_ms, 1200);

	/* echo host > data_role  : SET_UOR(DFP), then wait for DP to come up */
	ret = qpg_send_ucsi_set_uor(&qpg_session, 0, true);
	printf("qpg: SET_UOR(DFP) ret=%d\n", ret);
	qpg_bounce_settle(&state, &last_notify_ms, settle_ms);

	qpg_send_ucsi_get_connector_status(&qpg_session, 0);
	printf("qpg: bounce done dp_seen=%u hpd=%u mux=%u dpam=%02x\n",
	       state.dp_seen, state.hpd, state.mux, state.dpam_raw);

	return CMD_RET_SUCCESS;
}

static int do_qpg(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	if (argc >= 2 && argc <= 3 && !strcmp(argv[1], "service"))
		return do_qpg_service(cmdtp, flag, argc - 1, argv + 1);
	if (argc >= 2 && argc <= 3 && !strcmp(argv[1], "reset"))
		return do_qpg_reset(cmdtp, flag, argc - 1, argv + 1);
	if (argc >= 2 && argc <= 3 && !strcmp(argv[1], "dfp"))
		return do_qpg_dfp(cmdtp, flag, argc - 1, argv + 1);
	if (argc >= 2 && argc <= 3 && !strcmp(argv[1], "bounce"))
		return do_qpg_bounce(cmdtp, flag, argc - 1, argv + 1);
	if (argc == 2 && !strcmp(argv[1], "ucsi"))
		return do_qpg_ucsi(cmdtp, flag, argc - 1, argv + 1);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	qpg, 3, 1, do_qpg,
	"Qualcomm PMIC-GLINK diagnostics",
	"service [timeout_ms] - keep PMIC-GLINK altmode service alive and print notifications\n"
	"reset [hard] - UCSI connector reset to force re-attach + DP alt-mode re-entry\n"
	"dfp [timeout_ms] - UCSI SET_UOR data-role swap to DFP/host, then watch for DP\n"
	"bounce [settle_ms] - data-role bounce UFP->DFP (mirrors dp-renegotiate) to enter DP\n"
	"ucsi - run UCSI reset/discovery diagnostics"
);
