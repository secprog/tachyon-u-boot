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
#include <smem.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <soc/qcom/pmic_glink.h>
#include <time.h>

#define QPG_SMEM_XPRT_DESCRIPTOR		478
#define QPG_SMEM_XPRT_FIFO_0			479
#define QPG_SMEM_XPRT_FIFO_1			480

#define QPG_FIFO_FULL_RESERVE			8
#define QPG_TX_BLOCKED_CMD_RESERVE		8
#define QPG_RX_INTENT_SIZE			512
#define QPG_CHANNEL_NAME			"PMIC_RTR_ADSP_APPS"
#define QPG_ALTMODE_TIMEOUT_MS			5000

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

#define PMIC_GLINK_OWNER_USB_TYPE_C		32779
#define PMIC_GLINK_OWNER_USBC_PAN		32780
#define PMIC_GLINK_REQ_RESP			1

#define UCSI_READ_BUFFER_REQ			0x11
#define UCSI_NOTIFY_IND			0x13

#define USBC_SC8180X_NOTIFY_IND			0x13
#define USBC_CMD_WRITE_REQ			0x15
#define USBC_NOTIFY_IND				0x16

#define ALTMODE_PAN_EN				0x10
#define ALTMODE_PAN_ACK				0x11

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

struct qpg_ucsi_read_buffer_resp {
	struct qpg_pmic_hdr hdr;
	u8 read_buffer[48];
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
	bool altmode_notify_seen;
	bool altmode_no_dp;
	bool ucsi_read_acked;
	struct qpg_notify_debug notify;
};

static struct qpg qpg_session;
static bool qpg_session_ready;
static struct qcom_pmic_glink_altmode qpg_cached_altmode;
static bool qpg_cached_altmode_valid;
static int qpg_last_adsp_boot_ret;

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

static bool qpg_env_bool(const char *name)
{
	const char *value = env_get(name);

	return value && (!strcmp(value, "1") ||
			 !strcmp(value, "true") ||
			 !strcmp(value, "yes") ||
			 !strcmp(value, "on"));
}

static void qpg_program_sbu_mux(enum qcom_pmic_glink_orientation orientation,
				bool dp_active)
{
	bool invert_select = qpg_env_bool("tachyon_dp_invert_sbu_select");
	bool invert_enable = qpg_env_bool("tachyon_dp_invert_sbu_enable");
	struct gpio_desc sbu_enable = {};
	struct gpio_desc sbu_select = {};
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
		    dp_active ? "dp" : "safe", orientation,
		    dm_gpio_get_value(&sbu_enable),
		    dm_gpio_get_value(&sbu_select));

	dm_gpio_free(NULL, &sbu_enable);
	dm_gpio_free(NULL, &sbu_select);
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

	log_warning("pmic-glink: TX hlen=%zu dlen=%zu aligned=%zu head=%u next=%u avail=%zu\n",
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

	log_warning("pmic-glink: send RX_DONE cid=%u liid=%u\n", cid, liid);

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_wait_riid(struct qpg *pg);
static int qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg);
static int qpg_send_ucsi_read(struct qpg *pg);
static int qpg_drain_until(struct qpg *pg,
			   struct qcom_pmic_glink_altmode *altmode,
			   bool (*done)(struct qpg *,
					struct qcom_pmic_glink_altmode *),
			   u32 timeout_ms);
static bool qpg_done_pan_ack(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode);
static bool qpg_done_ucsi_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode);

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

	log_warning("pmic-glink: send data begin lcid=%u len=%zu riid=%u\n",
		    lcid, len, riid);
	ret = qpg_tx(pg, &hdr, sizeof(hdr), data, len);
	log_warning("pmic-glink: send data end ret=%d\n", ret);

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

static bool qpg_parse_sc8280xp_notify(struct qpg *pg,
				      struct qcom_pmic_glink_altmode *altmode,
				      const void *data, size_t len,
				      u32 *portp)
{
	const struct qpg_usbc_notify *notify = data;
	enum qcom_pmic_glink_orientation orientation;
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
	pg->notify.hpd = !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK);
	pg->notify.hpd_irq = !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK);

	if (svid != USB_TYPEC_DP_SID)
		return true;

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->hpd = pg->notify.hpd;
	altmode->hpd_irq = pg->notify.hpd_irq;
	pg->altmode_notify_seen = true;
	mode = pg->notify.dpam;
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    notify->payload[1], orientation);
	if (mode < DPAM_HPD_A) {
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_program_sbu_mux(orientation, false);
		log_warning("pmic-glink: DP notify safe/no-DP mux=%u dpam=%u\n",
			    notify->payload[2], mode);
		return true;
	}

	log_warning("pmic-glink: DPAM raw=%u pin_assignment=%u\n",
		    mode, mode - DPAM_HPD_A);

	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->dp = true;
	pg->altmode_no_dp = false;
	qpg_program_sbu_mux(orientation, true);

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
	pg->notify.svid = USB_TYPEC_DP_SID;
	pg->notify.dpam = mode;
	pg->notify.hpd = !!(notification & SC8180X_HPD_STATE_MASK);
	pg->notify.hpd_irq = !!(notification & SC8180X_HPD_IRQ_MASK);

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->hpd = pg->notify.hpd;
	altmode->hpd_irq = pg->notify.hpd_irq;
	pg->altmode_notify_seen = true;
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    raw_orientation, orientation);
	if (mux != 2 || mode < DPAM_HPD_A) {
		altmode->dp = false;
		altmode->pin_assignment = 0;
		pg->altmode_no_dp = true;
		qpg_program_sbu_mux(orientation, false);
		log_warning("pmic-glink: SC8180X notify safe/no-DP mux=%u mode=%u\n",
			    mux, mode);
		return true;
	}

	log_warning("pmic-glink: DPAM raw=%u pin_assignment=%u\n",
		    mode, mode - DPAM_HPD_A);

	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->dp = true;
	pg->altmode_no_dp = false;
	qpg_program_sbu_mux(orientation, true);

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

	return ret;
}

static void qpg_parse_pmic(struct qpg *pg,
			   struct qcom_pmic_glink_altmode *altmode,
			   const void *data, size_t len)
{
	const struct qpg_pmic_hdr *hdr = data;
	u32 owner, type, raw_opcode;
	bool ack_notify = false;
	u32 port = 0;
	int ret;
	u16 opcode;
	u16 svid;

	if (len < sizeof(*hdr)) {
		log_warning("pmic-glink: PMIC msg too short len=%zu\n", len);
		return;
	}

	owner = le32_to_cpu(hdr->owner);
	type = le32_to_cpu(hdr->type);
	raw_opcode = le32_to_cpu(hdr->opcode);
	opcode = raw_opcode & 0xff;
	svid = raw_opcode >> 16;

	log_warning("pmic-glink: PMIC msg owner=%u type=%u opcode=%02x raw_opcode=%08x svid=%04x len=%zu\n",
		    owner, type, opcode, raw_opcode, svid, len);

	if (owner == PMIC_GLINK_OWNER_USB_TYPE_C) {
		const struct qpg_ucsi_read_buffer_resp *resp = data;

		switch (opcode) {
		case UCSI_READ_BUFFER_REQ:
			pg->ucsi_read_acked = true;
			if (len >= sizeof(*resp)) {
				const u8 *buf = resp->read_buffer;

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
		case UCSI_NOTIFY_IND:
			log_warning("pmic-glink: UCSI notify len=%zu\n", len);
			break;
		default:
			log_warning("pmic-glink: USB Type-C owner opcode=%02x len=%zu\n",
				    opcode, len);
			break;
		}

		return;
	}

	if (owner != PMIC_GLINK_OWNER_USBC_PAN) {
		log_warning("pmic-glink: unsupported PMIC owner=%u opcode=%02x len=%zu\n",
			    owner, opcode, len);
		return;
	}

	switch (opcode) {
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
		ret = qpg_send_notify_pan_ack(pg, altmode, port);
		if (ret)
			log_warning("pmic-glink: ALTMODE_PAN_ACK failed ret=%d port=%u\n",
				    ret, port);
	}
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
	u16 cid;
	int ret = 0;

	if (avail < sizeof(hdr))
		return -EAGAIN;

	qpg_rx_peek(pg, &hdr, 0, sizeof(hdr));
	cid = le16_to_cpu(hdr.msg.param1);
	chunk_size = le32_to_cpu(hdr.chunk_size);
	liid = le32_to_cpu(hdr.msg.param2);
	log_warning("pmic-glink: RX data header cmd=%u lcid=%u liid=%u chunk=%u left=%u avail=%zu payload_len=%u\n",
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
		qpg_parse_pmic(pg, altmode, payload, chunk_size);
		rx_done_cid = pg->lcid;
	} else {
		log_warning("pmic-glink: RX data on unknown cid=%u liid=%u len=%u\n",
			    cid, liid, chunk_size);
	}

	if (rx_done_cid)
		ret = qpg_send_rx_done_for(pg, rx_done_cid, liid);

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

	avail = qpg_rx_avail(pg);
	if (avail < sizeof(msg))
		return -EAGAIN;

	tail = le32_to_cpu(*pg->rx_tail);
	raw_len = min_t(size_t, avail, sizeof(raw));
	qpg_rx_peek(pg, raw, 0, raw_len);
	if (avail >= 8)
		log_warning("pmic-glink: RX raw off=%u avail=%zu h0=%08x h1=%08x\n",
				tail, avail,
				le32_to_cpu(raw[0]),
				le32_to_cpu(raw[1]));
	else
		log_warning("pmic-glink: RX raw off=%u avail=%zu too short\n",
				tail, avail);

	qpg_rx_peek(pg, &msg, 0, sizeof(msg));
	cmd = le16_to_cpu(msg.cmd);
	param1 = le16_to_cpu(msg.param1);
	param2 = le32_to_cpu(msg.param2);

	ret = qpg_rx_packet_len(pg, avail, cmd, param2, &header_len,
				&payload_len);
	if (ret)
		return ret;

	log_warning("pmic-glink: RX cmd=%u param1=%u param2=%u avail=%zu\n",
		    cmd, param1, param2, avail);
	log_warning("pmic-glink: RX header cmd=%u param1=%u param2=%u header_len=%zu payload_len=%zu avail=%zu\n",
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
		log_warning("pmic-glink: RX handled cmd=%u rcid=%u done\n",
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

static bool qpg_done_altmode(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
{
	/*
	 * Accept DP Alt Mode entry regardless of HPD state.
	 * HPD can assert later; the display driver polls DPCD
	 * until the sink responds.
	 */
	return altmode->dp &&
	       altmode->orientation != QCOM_PMIC_GLINK_ORIENTATION_NONE;
}

static int qpg_wait_riid(struct qpg *pg)
{
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	log_warning("pmic-glink: wait RIID begin riid_avail=%d riid=%u riid_size=%u\n",
		    pg->riid_avail, pg->riid, pg->riid_size);

	ret = qpg_drain_until(pg, &altmode, qpg_done_riid, 500);

	log_warning("pmic-glink: wait RIID end ret=%d riid_avail=%d riid=%u riid_size=%u\n",
		    ret, pg->riid_avail, pg->riid, pg->riid_size);

	return ret;
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

static int qpg_send_ucsi_read(struct qpg *pg)
{
	struct qpg_ucsi_read_buffer_resp req = {
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
		if (adsp_boot_retp)
			*adsp_boot_retp = 0;
		if (glink_open_retp)
			*glink_open_retp = 0;
		if (pan_en_retp)
			*pan_en_retp = 0;
		return 0;
	}

	memset(&qpg_session, 0, sizeof(qpg_session));

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

	ret = qpg_send_ucsi_read(&qpg_session);
	if (ret)
		log_warning("pmic-glink: UCSI read poke ignored ret=%d\n", ret);

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

	qpg_session_ready = true;

	return 0;
}

int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	int ret;

	if (!altmode)
		return -EINVAL;

	memset(altmode, 0, sizeof(*altmode));

	log_warning("pmic-glink: get_altmode start\n");

	ret = qpg_open_session(altmode, NULL, NULL, NULL);
	if (ret)
		return ret;

	ret = qpg_drain_until(&qpg_session, altmode, qpg_done_altmode,
			      QPG_ALTMODE_TIMEOUT_MS);
	if (ret == -ETIMEDOUT && qpg_session.altmode_notify_seen &&
	    qpg_session.altmode_no_dp) {
		log_warning("pmic-glink: no DP sink active after valid notification\n");
		ret = -ENODEV;
	}

	log_warning("pmic-glink: wait altmode ret=%d dp=%d orientation=%u pin=%u hpd=%d irq=%d\n",
		    ret, altmode->dp, altmode->orientation, altmode->pin_assignment,
		    altmode->hpd, altmode->hpd_irq);

	if (!ret) {
		qpg_cached_altmode = *altmode;
		qpg_cached_altmode_valid = true;
		return 0;
	}

	if (ret == -ENODEV) {
		/*
		 * Save every fresh no-DP notification so orientation stays
		 * current across cable re-plug events.  On session-reuse
		 * calls altmode is still zeroed (no re-parse), so guard
		 * with an orientation check to avoid overwriting the cache.
		 */
		if (altmode->orientation != QCOM_PMIC_GLINK_ORIENTATION_NONE) {
			qpg_cached_altmode = *altmode;
			qpg_cached_altmode_valid = true;
		}
		if (qpg_cached_altmode_valid)
			*altmode = qpg_cached_altmode;
		return ret;
	}

	if (qpg_cached_altmode_valid) {
		*altmode = qpg_cached_altmode;
		log_warning("pmic-glink: using cached altmode dp=%d orientation=%u pin=%u hpd=%d irq=%d\n",
			    altmode->dp, altmode->orientation,
			    altmode->pin_assignment, altmode->hpd,
			    altmode->hpd_irq);
		return 0;
	}

	return ret;
}

static int do_qpg_altmode(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	struct qcom_pmic_glink_altmode altmode = {};
	int adsp_ret = 0;
	int open_ret = 0;
	int pan_ret = 0;
	int notify_ret;
	int ret;

	ret = qpg_open_session(&altmode, &adsp_ret, &open_ret, &pan_ret);
	if (!ret) {
		memset(&altmode, 0, sizeof(altmode));
		qpg_session.notify.seen = false;
		qpg_session.altmode_notify_seen = false;
		qpg_session.altmode_no_dp = false;
		qpg_session.pan_acked = false;
		pan_ret = qpg_send_altmode_req(&qpg_session, ALTMODE_PAN_EN, 0);
		if (!pan_ret)
			pan_ret = qpg_drain_until(&qpg_session, &altmode,
						  qpg_done_pan_ack, 1000);
		if (pan_ret)
			ret = pan_ret;
	}

	printf("ADSP boot: ret=%d\n", adsp_ret);
	printf("GLINK open: ret=%d\n", open_ret);
	printf("PAN_EN: ret=%d\n", pan_ret);

	if (!ret) {
		notify_ret = qpg_drain_until(&qpg_session, &altmode,
					     qpg_done_altmode,
					     QPG_ALTMODE_TIMEOUT_MS);
		if (notify_ret)
			ret = notify_ret;
	}

	if (qpg_session.notify.seen) {
		const struct qpg_notify_debug *notify = &qpg_session.notify;

		printf("notify: port=%u orientation=%u/%u mux=%u svid=%04x dpam=%02x hpd=%u irq=%u\n",
		       notify->port, notify->raw_orientation,
		       notify->orientation, notify->mux, notify->svid,
		       notify->dpam, notify->hpd, notify->hpd_irq);
		printf("altmode: ret=%d dp=%u orientation=%u pin=%u hpd=%u irq=%u\n",
		       ret, altmode.dp, altmode.orientation,
		       altmode.pin_assignment, altmode.hpd,
		       altmode.hpd_irq);
	} else {
		printf("notify: ret=%d\n", ret);
	}

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_qpg(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	if (argc == 2 && !strcmp(argv[1], "altmode"))
		return do_qpg_altmode(cmdtp, flag, argc - 1, argv + 1);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	qpg, 2, 1, do_qpg,
	"Qualcomm PMIC-GLINK diagnostics",
	"altmode - boot ADSP, open PMIC-GLINK, enable PAN, print Type-C altmode notification"
);
