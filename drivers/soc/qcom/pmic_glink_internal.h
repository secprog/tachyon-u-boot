/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Private shared declarations for the Qualcomm PMIC-GLINK driver family:
 * the core GLINK transport plus the altmode / UCSI / battmgr client
 * drivers. Not a UAPI/installed header -- internal to drivers/soc/qcom/.
 */
#ifndef __SOC_QCOM_PMIC_GLINK_INTERNAL_H__
#define __SOC_QCOM_PMIC_GLINK_INTERNAL_H__

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/sizes.h>
#include <linux/compiler.h>
#include <mailbox.h>
#include <dm/device.h>
#include <soc/qcom/pmic_glink.h>

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

	/*
	 * Session control + cached results (formerly file-scope statics in
	 * pmic_glink.c). Kept in the session so the core and the client drivers
	 * can share them through the core device priv.
	 */
	bool session_ready;
	struct qcom_pmic_glink_altmode cached_altmode;
	bool cached_altmode_valid;
	struct qcom_pmic_glink_altmode_state cached_state;
	int last_adsp_boot_ret;
	int last_ucsi_prewarm_ret;
};

/*
 * Cross-file interface between the pmic-glink core and its client drivers.
 * The core owns the GLINK transport, the rx parse loop and the session; the
 * client drivers (UCSI, altmode, battmgr) call the core helpers below and
 * export a few entry points the core invokes in return.
 */

/* Core transport / orchestration / utilities / session (defined in the core). */
struct qpg *qpg_session_get(void);
int qpg_send_data(struct qpg *pg, const void *data, size_t len);
bool qpg_env_bool(const char *name);
int qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg);
int qpg_poll(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode);
int qpg_open_session(struct qcom_pmic_glink_altmode *altmode,
		     int *adsp_boot_retp, int *glink_open_retp, int *pan_en_retp);
int qpg_drain_until(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
		    bool (*done)(struct qpg *, struct qcom_pmic_glink_altmode *),
		    u32 timeout_ms);
bool qpg_done_ucsi_read(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode);
bool qpg_done_ucsi_write(struct qpg *pg,
			 struct qcom_pmic_glink_altmode *altmode);
bool qpg_done_pan_ack(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode);
bool qpg_done_usbc_read(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode);

/* Alt-mode / DP client (pmic_glink_altmode.c) entry points used by the core. */
bool qpg_parse_sc8280xp_notify(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode,
			       const void *data, size_t len, u32 *portp);
bool qpg_parse_sc8180x_notify(struct qpg *pg,
			      struct qcom_pmic_glink_altmode *altmode,
			      const void *data, size_t len, u32 *portp);
int qpg_send_notify_pan_ack(struct qpg *pg,
			    struct qcom_pmic_glink_altmode *altmode, u32 port);

/* UCSI client (pmic_glink_ucsi.c) entry points used by the core. */
int qpg_ucsi_prewarm(struct qpg *pg);
int qpg_ucsi_discover(struct qpg *pg);

/*
 * Battery-manager client (pmic_glink_battmgr.c) entry point, optional. When the
 * client is compiled out, the core's call becomes a no-op stub.
 */
#if IS_ENABLED(CONFIG_QCOM_PMIC_GLINK_BATTMGR)
int qpg_register_battmgr(struct qpg *pg);
#else
static inline int qpg_register_battmgr(struct qpg *pg)
{
	return 0;
}
#endif

#endif /* __SOC_QCOM_PMIC_GLINK_INTERNAL_H__ */
