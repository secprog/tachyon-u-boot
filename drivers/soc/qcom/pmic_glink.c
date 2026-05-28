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

#include <asm/io.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/uclass.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <log.h>
#include <mapmem.h>
#include <smem.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <soc/qcom/pmic_glink.h>
#include <time.h>

#define QPG_SMEM_XPRT_DESCRIPTOR		478
#define QPG_SMEM_XPRT_FIFO_0			479
#define QPG_SMEM_XPRT_FIFO_1			480

#define QPG_IPCC_REG_SEND_ID			0x0c
#define QPG_FIFO_FULL_RESERVE			8
#define QPG_TX_BLOCKED_CMD_RESERVE		8
#define QPG_RX_INTENT_SIZE			512
#define QPG_QRTR_MAX_SERVERS			32
#define QPG_CHANNEL_NAME			"PMIC_RTR_ADSP_APPS"
#define QPG_IPCRTR_CHANNEL_NAME			"IPCRTR"
#define QPG_IPCRTR_DRAIN_MS			1000

#define QRTR_NODE_BCAST			0xffffffff
#define QRTR_PORT_CTRL				0xfffffffe
#define QRTR_PORT_CTRL_LEGACY			0x0000ffff
#define QRTR_LOCAL_NODE			1

#define QRTR_VERSION_1				1
#define QRTR_VERSION_2				3
#define QRTR_TYPE_DATA				1
#define QRTR_TYPE_HELLO				2
#define QRTR_TYPE_BYE				3
#define QRTR_TYPE_NEW_SERVER			4
#define QRTR_TYPE_DEL_SERVER			5
#define QRTR_TYPE_DEL_CLIENT			6
#define QRTR_TYPE_RESUME_TX			7
#define QRTR_TYPE_NEW_LOOKUP			10

#define GLINK_VERSION_1				1
#define GLINK_FEATURE_INTENT_REUSE		BIT(0)

#define GLINK_CMD_VERSION			0
#define GLINK_CMD_VERSION_ACK			1
#define GLINK_CMD_OPEN				2
#define GLINK_CMD_OPEN_ACK			4
#define GLINK_CMD_INTENT			5
#define GLINK_CMD_RX_DONE			6
#define GLINK_CMD_TX_DATA			9
#define GLINK_CMD_TX_DATA_CONT			12
#define GLINK_CMD_READ_NOTIF			13
#define GLINK_CMD_RX_DONE_W_REUSE		14

#define PMIC_GLINK_OWNER_USBC_PAN		32780
#define PMIC_GLINK_REQ_RESP			1

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

struct qpg_qrtr_hdr_v1 {
	__le32 version;
	__le32 type;
	__le32 src_node_id;
	__le32 src_port_id;
	__le32 confirm_rx;
	__le32 size;
	__le32 dst_node_id;
	__le32 dst_port_id;
} __packed;

struct qpg_qrtr_hdr_v2 {
	u8 version;
	u8 type;
	u8 flags;
	u8 optlen;
	__le32 size;
	__le16 src_node_id;
	__le16 src_port_id;
	__le16 dst_node_id;
	__le16 dst_port_id;
} __packed;

struct qpg_qrtr_ctrl_pkt {
	__le32 cmd;
	union {
		struct {
			__le32 service;
			__le32 instance;
			__le32 node;
			__le32 port;
		} server;
		struct {
			__le32 node;
			__le32 port;
		} client;
	} u;
} __packed;

struct qpg_qrtr_server {
	u32 service;
	u32 instance;
	u32 node;
	u32 port;
	bool valid;
};

struct qpg {
	struct udevice *smem;
	void __iomem *ipcc;
	u32 remote_pid;
	u16 ipcc_client;
	u16 ipcc_signal;
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
	u32 riid;
	u32 riid_size;
	u32 ipcrtr_riid;
	u32 ipcrtr_riid_size;
	u32 ipcrtr_liid;
	bool riid_avail;
	bool ipcrtr_riid_avail;
	bool version_acked;
	bool open_acked;
	bool remote_opened;
	bool ipcrtr_opened;
	bool ipcrtr_open_acked;
	bool ipcrtr_seen_data;
	bool pan_acked;
	u16 ipcrtr_rcid;
	u16 ipcrtr_lcid;
	u16 next_lcid;
	u32 next_liid;
	struct qpg_qrtr_server servers[QPG_QRTR_MAX_SERVERS];
};

static u32 qpg_hwirq(u16 client, u16 signal)
{
	return ((u32)client << 16) | signal;
}

static enum qcom_pmic_glink_orientation qpg_orientation(u8 orientation)
{
	if (orientation == 0)
		return QCOM_PMIC_GLINK_ORIENTATION_NORMAL;
	if (orientation == 1)
		return QCOM_PMIC_GLINK_ORIENTATION_REVERSE;

	return QCOM_PMIC_GLINK_ORIENTATION_NONE;
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
	log_warning("pmic-glink: kick hwirq=%08x client=%u signal=%u\n",
		    qpg_hwirq(pg->ipcc_client, pg->ipcc_signal),
		    pg->ipcc_client, pg->ipcc_signal);

	writel(qpg_hwirq(pg->ipcc_client, pg->ipcc_signal),
	       pg->ipcc + QPG_IPCC_REG_SEND_ID);
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

static int qpg_send_rx_intent_for(struct qpg *pg, u16 cid, u32 liid)
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
		.size = cpu_to_le32(QPG_RX_INTENT_SIZE),
		.liid = cpu_to_le32(liid),
	};

	log_warning("pmic-glink: send RX_INTENT cid=%u liid=%u size=%u\n",
		    cid, liid, QPG_RX_INTENT_SIZE);

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
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
static int qpg_wait_ipcrtr_riid(struct qpg *pg);

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

static int qpg_send_ipcrtr_data(struct qpg *pg, const void *data, size_t len)
{
	int ret;

	ret = qpg_wait_ipcrtr_riid(pg);
	if (ret)
		return ret;

	pg->ipcrtr_riid_avail = false;

	return qpg_send_data_for(pg, pg->ipcrtr_lcid, pg->ipcrtr_riid,
				 data, len);
}

static int qpg_send_qrtr_ctrl(struct qpg *pg, u32 type, u32 service,
			      u32 instance)
{
	struct {
		struct qpg_qrtr_hdr_v1 hdr;
		struct qpg_qrtr_ctrl_pkt ctrl;
	} __packed pkt = {};
	int ret;

	pkt.hdr.version = cpu_to_le32(QRTR_VERSION_1);
	pkt.hdr.type = cpu_to_le32(type);
	pkt.hdr.src_node_id = cpu_to_le32(QRTR_LOCAL_NODE);
	pkt.hdr.src_port_id = cpu_to_le32(QRTR_PORT_CTRL);
	pkt.hdr.size = cpu_to_le32(sizeof(pkt.ctrl));
	pkt.hdr.dst_node_id = cpu_to_le32(QRTR_NODE_BCAST);
	pkt.hdr.dst_port_id = cpu_to_le32(QRTR_PORT_CTRL);
	pkt.ctrl.cmd = cpu_to_le32(type);

	if (type == QRTR_TYPE_NEW_LOOKUP) {
		pkt.ctrl.u.server.service = cpu_to_le32(service);
		pkt.ctrl.u.server.instance = cpu_to_le32(instance);
	}

	log_warning("pmic-glink: QRTR send ctrl begin type=%u service=%u instance=%u lcid=%u riid_avail=%d riid=%u\n",
		    type, service, instance, pg->ipcrtr_lcid,
		    pg->ipcrtr_riid_avail, pg->ipcrtr_riid);
	ret = qpg_send_ipcrtr_data(pg, &pkt, sizeof(pkt));
	log_warning("pmic-glink: QRTR send ctrl end type=%u ret=%d\n",
		    type, ret);

	return ret;
}

static void qpg_parse_sc8280xp_notify(struct qcom_pmic_glink_altmode *altmode,
				      const void *data, size_t len)
{
	const struct qpg_usbc_notify *notify = data;
	enum qcom_pmic_glink_orientation orientation;
	u8 mode;
	u16 svid;

	log_warning("pmic-glink: SC8280XP notify len=%zu expected=%zu\n",
		    len, sizeof(*notify));

	if (len != sizeof(*notify))
		return;

	svid = le32_to_cpu(notify->hdr.opcode) >> 16;
	log_warning("pmic-glink: SC8280XP port=%u orientation=%u mux=%u svid=%04x dpam=%02x hpd=%u irq=%u\n",
		    notify->payload[0], notify->payload[1],
		    notify->payload[2], svid,
		    notify->payload[8] & SC8280XP_DPAM_MASK,
		    !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK),
		    !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK));
	if (svid != USB_TYPEC_DP_SID)
		return;

	mode = notify->payload[8] & SC8280XP_DPAM_MASK;
	if (mode < DPAM_HPD_A)
		return;

	orientation = qpg_orientation(notify->payload[1]);
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    notify->payload[1], orientation);
	log_warning("pmic-glink: DPAM raw=%u pin_assignment=%u\n",
		    mode, mode - DPAM_HPD_A);

	altmode->port = notify->payload[0];
	altmode->orientation = orientation;
	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->hpd = !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK);
	altmode->hpd_irq = !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK);
	altmode->dp = true;
}

static void qpg_parse_sc8180x_notify(struct qcom_pmic_glink_altmode *altmode,
				     const void *data, size_t len)
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
		return;

	notification = le32_to_cpu(msg->notification);
	port = notification & SC8180X_PORT_MASK;
	raw_orientation = (notification & SC8180X_ORIENTATION_MASK) >> 8;
	mux = (notification & SC8180X_MUX_MASK) >> 16;
	mode = (notification & SC8180X_MODE_MASK) >> 24;
	log_warning("pmic-glink: SC8180X notification=%08x port=%u orientation=%u mux=%u mode=%u hpd=%u irq=%u\n",
		    notification, port, raw_orientation, mux, mode,
		    !!(notification & SC8180X_HPD_STATE_MASK),
		    !!(notification & SC8180X_HPD_IRQ_MASK));
	if (mux != 2)
		return;

	if (mode < DPAM_HPD_A)
		return;

	orientation = qpg_orientation(raw_orientation);
	log_warning("pmic-glink: orientation raw=%u mapped=%u\n",
		    raw_orientation, orientation);
	log_warning("pmic-glink: DPAM raw=%u pin_assignment=%u\n",
		    mode, mode - DPAM_HPD_A);

	altmode->port = port;
	altmode->orientation = orientation;
	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->hpd = !!(notification & SC8180X_HPD_STATE_MASK);
	altmode->hpd_irq = !!(notification & SC8180X_HPD_IRQ_MASK);
	altmode->dp = true;
}

static void __maybe_unused
qpg_parse_pmic(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
	       const void *data, size_t len)
{
	const struct qpg_pmic_hdr *hdr = data;
	u32 owner, type, raw_opcode;
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

	switch (opcode) {
	case USBC_CMD_WRITE_REQ:
		pg->pan_acked = true;
		break;
	case USBC_NOTIFY_IND:
		qpg_parse_sc8280xp_notify(altmode, data, len);
		break;
	case USBC_SC8180X_NOTIFY_IND:
		qpg_parse_sc8180x_notify(altmode, data, len);
		break;
	}
}

static void qpg_qrtr_store_server(struct qpg *pg, u32 service, u32 instance,
				  u32 node, u32 port, bool valid)
{
	struct qpg_qrtr_server *free = NULL;
	int i;

	for (i = 0; i < QPG_QRTR_MAX_SERVERS; i++) {
		struct qpg_qrtr_server *srv = &pg->servers[i];

		if (!srv->valid) {
			if (!free)
				free = srv;
			continue;
		}

		if (srv->service == service && srv->instance == instance &&
		    srv->node == node && srv->port == port) {
			srv->valid = valid;
			log_warning("pmic-glink: QRTR server %s service=%u instance=%u node=%u port=%u\n",
				    valid ? "update" : "delete",
				    service, instance, node, port);
			return;
		}
	}

	if (!valid)
		return;

	if (!free) {
		log_warning("pmic-glink: QRTR server table full, dropping service=%u instance=%u node=%u port=%u\n",
			    service, instance, node, port);
		return;
	}

	free->service = service;
	free->instance = instance;
	free->node = node;
	free->port = port;
	free->valid = true;
	log_warning("pmic-glink: QRTR server add service=%u instance=%u node=%u port=%u\n",
		    service, instance, node, port);
}

static void qpg_qrtr_parse_ctrl(struct qpg *pg, u32 type, u32 src_node,
				u32 src_port, const void *payload, size_t len)
{
	const struct qpg_qrtr_ctrl_pkt *ctrl = payload;
	u32 cmd, service, instance, node, port;

	if (len < sizeof(*ctrl)) {
		log_warning("pmic-glink: QRTR ctrl short type=%u len=%zu need=%zu\n",
			    type, len, sizeof(*ctrl));
		return;
	}

	cmd = le32_to_cpu(ctrl->cmd);
	log_warning("pmic-glink: QRTR ctrl type=%u cmd=%u from=%u:%u\n",
		    type, cmd, src_node, src_port);

	switch (type) {
	case QRTR_TYPE_NEW_SERVER:
	case QRTR_TYPE_DEL_SERVER:
		service = le32_to_cpu(ctrl->u.server.service);
		instance = le32_to_cpu(ctrl->u.server.instance);
		node = le32_to_cpu(ctrl->u.server.node);
		port = le32_to_cpu(ctrl->u.server.port);
		log_warning("pmic-glink: QRTR server cmd=%u service=%u instance=%u node=%u port=%u\n",
			    cmd, service, instance, node, port);
		qpg_qrtr_store_server(pg, service, instance, node, port,
				      type == QRTR_TYPE_NEW_SERVER);
		break;
	case QRTR_TYPE_DEL_CLIENT:
	case QRTR_TYPE_RESUME_TX:
		node = le32_to_cpu(ctrl->u.client.node);
		port = le32_to_cpu(ctrl->u.client.port);
		log_warning("pmic-glink: QRTR client cmd=%u node=%u port=%u\n",
			    cmd, node, port);
		break;
	case QRTR_TYPE_HELLO:
	case QRTR_TYPE_BYE:
	case QRTR_TYPE_NEW_LOOKUP:
		break;
	default:
		break;
	}
}

static void qpg_log_ipcrtr(struct qpg *pg, const void *data, size_t len)
{
	const struct qpg_qrtr_hdr_v1 *v1 = data;
	const struct qpg_qrtr_hdr_v2 *v2 = data;
	const u8 *payload = data;
	size_t hdr_len, payload_avail;
	u32 version, type, size, src_node, src_port, dst_node, dst_port;

	if (len < 4) {
		log_warning("pmic-glink: IPCRTR short frame len=%zu\n", len);
		return;
	}

	version = le32_to_cpu(*(__le32 *)data);
	if (version == QRTR_VERSION_1) {
		if (len < sizeof(*v1)) {
			log_warning("pmic-glink: QRTRv1 short frame len=%zu need=%zu\n",
				    len, sizeof(*v1));
			return;
		}

		hdr_len = sizeof(*v1);
		type = le32_to_cpu(v1->type);
		size = le32_to_cpu(v1->size);
		src_node = le32_to_cpu(v1->src_node_id);
		src_port = le32_to_cpu(v1->src_port_id);
		dst_node = le32_to_cpu(v1->dst_node_id);
		dst_port = le32_to_cpu(v1->dst_port_id);
		log_warning("pmic-glink: QRTRv1 type=%u size=%u src=%u:%u dst=%u:%u confirm=%u frame_len=%zu\n",
			    type, size, src_node, src_port, dst_node, dst_port,
			    le32_to_cpu(v1->confirm_rx), len);
	} else if (*(const u8 *)data == QRTR_VERSION_2) {
		if (len < sizeof(*v2)) {
			log_warning("pmic-glink: QRTRv2 short frame len=%zu need=%zu\n",
				    len, sizeof(*v2));
			return;
		}

		hdr_len = sizeof(*v2) + v2->optlen;
		type = v2->type;
		size = le32_to_cpu(v2->size);
		src_node = le16_to_cpu(v2->src_node_id);
		src_port = le16_to_cpu(v2->src_port_id);
		dst_node = le16_to_cpu(v2->dst_node_id);
		dst_port = le16_to_cpu(v2->dst_port_id);
		log_warning("pmic-glink: QRTRv2 type=%u flags=%02x optlen=%u size=%u src=%u:%u dst=%u:%u frame_len=%zu\n",
			    type, v2->flags, v2->optlen, size,
			    src_node, src_port, dst_node, dst_port, len);
	} else {
		const __le32 *words = data;
		u32 w0 = 0, w1 = 0, w2 = 0, w3 = 0;

		if (len >= 4)
			w0 = le32_to_cpu(words[0]);
		if (len >= 8)
			w1 = le32_to_cpu(words[1]);
		if (len >= 12)
			w2 = le32_to_cpu(words[2]);
		if (len >= 16)
			w3 = le32_to_cpu(words[3]);

		log_warning("pmic-glink: IPCRTR unknown frame len=%zu w0=%08x w1=%08x w2=%08x w3=%08x\n",
			    len, w0, w1, w2, w3);
		return;
	}

	if (hdr_len > len) {
		log_warning("pmic-glink: QRTR bad header hdr_len=%zu len=%zu\n",
			    hdr_len, len);
		return;
	}

	payload_avail = len - hdr_len;
	if (size > payload_avail) {
		log_warning("pmic-glink: QRTR bad size=%u payload_avail=%zu type=%u\n",
			    size, payload_avail, type);
		return;
	}

	if (type != QRTR_TYPE_DATA)
		qpg_qrtr_parse_ctrl(pg, type, src_node, src_port,
				    payload + hdr_len, size);
}

static int qpg_rx_data(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
		       size_t avail)
{
	struct {
		struct qpg_msg msg;
		__le32 chunk_size;
		__le32 left_size;
	} __packed hdr;
	u8 payload[QPG_RX_INTENT_SIZE];
	u32 chunk_size, liid;
	u16 rx_done_cid = 0;
	u16 cid;

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

	if (chunk_size > sizeof(payload) || avail < sizeof(hdr) + chunk_size)
		return -EAGAIN;

	qpg_rx_peek(pg, payload, sizeof(hdr), chunk_size);
	qpg_rx_advance(pg, ALIGN(sizeof(hdr) + chunk_size, 8));

	if ((cid == pg->ipcrtr_lcid || cid == pg->ipcrtr_rcid) &&
	    liid == pg->ipcrtr_liid) {
		pg->ipcrtr_seen_data = true;
		qpg_log_ipcrtr(pg, payload, chunk_size);
		rx_done_cid = cid;
	} else if (pg->remote_opened && cid == pg->rcid && liid == 1) {
		log_warning("pmic-glink: raw PMIC frame ignored len=%u\n",
			    chunk_size);
		rx_done_cid = cid;
	} else {
		log_warning("pmic-glink: RX data on unknown cid=%u liid=%u len=%u\n",
			    cid, liid, chunk_size);
	}

	if (rx_done_cid)
		qpg_send_rx_done_for(pg, rx_done_cid, liid);

	return 0;
}

static int qpg_handle_intent(struct qpg *pg, u16 cid, u32 count,
			     const struct qpg_intent_pair *intent)
{
	if (!count)
		return -EINVAL;

	if (cid == pg->lcid) {
		pg->riid_size = le32_to_cpu(intent->size);
		pg->riid = le32_to_cpu(intent->iid);
		pg->riid_avail = pg->riid_size > 0;
		log_warning("pmic-glink: RIID channel=raw lcid=%u riid=%u size=%u avail=%d\n",
			    cid, pg->riid, pg->riid_size, pg->riid_avail);
	} else if (pg->ipcrtr_lcid && cid == pg->ipcrtr_lcid) {
		pg->ipcrtr_riid_size = le32_to_cpu(intent->size);
		pg->ipcrtr_riid = le32_to_cpu(intent->iid);
		pg->ipcrtr_riid_avail = pg->ipcrtr_riid_size > 0;
		log_warning("pmic-glink: RIID channel=IPCRTR lcid=%u riid=%u size=%u avail=%d\n",
			    cid, pg->ipcrtr_riid, pg->ipcrtr_riid_size,
			    pg->ipcrtr_riid_avail);
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
	} else if (!strcmp(name, QPG_IPCRTR_CHANNEL_NAME)) {
		pg->ipcrtr_rcid = rcid;
		if (!pg->ipcrtr_lcid)
			pg->ipcrtr_lcid = pg->next_lcid++;
		if (!pg->ipcrtr_liid)
			pg->ipcrtr_liid = pg->next_liid++;
		pg->ipcrtr_opened = true;
		log_warning("pmic-glink: IPCRTR ids confirmed rcid=%u allocated lcid=%u liid=%u\n",
			    pg->ipcrtr_rcid, pg->ipcrtr_lcid,
			    pg->ipcrtr_liid);
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
		if (ret)
			break;

		if (!strcmp(name, QPG_IPCRTR_CHANNEL_NAME)) {
			ret = qpg_send_open_ack(pg, param1, name);
			if (ret)
				break;

			pg->ipcrtr_open_acked = false;
			ret = qpg_send_open_for(pg, pg->ipcrtr_lcid,
						QPG_IPCRTR_CHANNEL_NAME);
		}
		break;
	}
	case GLINK_CMD_OPEN_ACK:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		if (param1 == pg->lcid) {
			pg->open_acked = true;
		} else if (pg->ipcrtr_opened && param1 == pg->ipcrtr_lcid) {
			pg->ipcrtr_open_acked = true;
			log_warning("pmic-glink: IPCRTR local OPEN_ACK lcid=%u rcid=%u\n",
				    pg->ipcrtr_lcid, pg->ipcrtr_rcid);
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
		if (param1 == pg->lcid && param2 == pg->riid)
			pg->riid_avail = cmd == GLINK_CMD_RX_DONE_W_REUSE;
		else if (param1 == pg->ipcrtr_lcid && param2 == pg->ipcrtr_riid)
			pg->ipcrtr_riid_avail =
				cmd == GLINK_CMD_RX_DONE_W_REUSE;
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

static int qpg_drain_until(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
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

static bool qpg_done_open(struct qpg *pg,
			  struct qcom_pmic_glink_altmode *altmode)
{
	return pg->open_acked;
}

static bool qpg_done_ipcrtr_open(struct qpg *pg,
				 struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ipcrtr_opened && pg->ipcrtr_open_acked;
}

static bool qpg_done_ipcrtr_data(struct qpg *pg,
				 struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ipcrtr_seen_data;
}

static bool qpg_done_riid(struct qpg *pg,
			  struct qcom_pmic_glink_altmode *altmode)
{
	return pg->riid_avail;
}

static bool qpg_done_ipcrtr_riid(struct qpg *pg,
				 struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ipcrtr_riid_avail;
}

static bool __maybe_unused
qpg_done_pan_ack(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
{
	return pg->pan_acked;
}

static bool __maybe_unused
qpg_done_altmode(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
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

static int qpg_wait_ipcrtr_riid(struct qpg *pg)
{
	struct qcom_pmic_glink_altmode altmode = {};
	int ret;

	log_warning("pmic-glink: wait IPCRTR RIID begin avail=%d riid=%u size=%u lcid=%u\n",
		    pg->ipcrtr_riid_avail, pg->ipcrtr_riid,
		    pg->ipcrtr_riid_size, pg->ipcrtr_lcid);

	ret = qpg_drain_until(pg, &altmode, qpg_done_ipcrtr_riid, 500);

	log_warning("pmic-glink: wait IPCRTR RIID end ret=%d avail=%d riid=%u size=%u\n",
		    ret, pg->ipcrtr_riid_avail, pg->ipcrtr_riid,
		    pg->ipcrtr_riid_size);

	return ret;
}

static int __maybe_unused qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg)
{
	struct qpg_usbc_write_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USBC_PAN),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(USBC_CMD_WRITE_REQ),
		.cmd = cpu_to_le32(cmd),
		.arg = cpu_to_le32(arg),
	};

	pg->pan_acked = false;

	log_warning("pmic-glink: owner=%u channel=%s altmode_cmd=%u arg=%u\n",
		    PMIC_GLINK_OWNER_USBC_PAN, QPG_CHANNEL_NAME, cmd, arg);

	return qpg_send_data(pg, &req, sizeof(req));
}

static int qpg_init(struct qpg *pg)
{
	struct ofnode_phandle_args args;
	ofnode ipcc = ofnode_null();
	ofnode adsp;
	ofnode glink = ofnode_null();
	fdt_addr_t addr;
	size_t size;
	__le32 *descs;
	ulong start;
	int ret;

	ret = uclass_first_device_err(UCLASS_SMEM, &pg->smem);
	log_warning("pmic-glink: smem lookup ret=%d smem=%p\n",
		    ret, pg->smem);
	if (ret)
		return ret;

	ret = qcom_adsp_pas_boot();
	log_warning("pmic-glink: ADSP PAS boot ret=%d\n", ret);
	if (ret)
		return ret;
	mdelay(100);

	adsp = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-adsp-pas");
	if (ofnode_valid(adsp))
		ofnode_for_each_subnode(glink, adsp) {
			const char *label = ofnode_read_string(glink, "label");

			if (label && !strcmp(label, "lpass"))
				break;
		}
	log_warning("pmic-glink: adsp node valid=%d glink node valid=%d\n",
		    ofnode_valid(adsp), ofnode_valid(glink));
	if (!ofnode_valid(adsp)) {
		log_warning("pmic-glink: missing qcom,sc7280-adsp-pas node\n");
		return -ENOENT;
	}

	if (!ofnode_valid(glink)) {
		log_warning("pmic-glink: missing lpass GLINK edge node\n");
		return -ENOENT;
	}

	ret = ofnode_read_u32(glink, "qcom,remote-pid", &pg->remote_pid);
	if (ret) {
		log_warning("pmic-glink: missing qcom,remote-pid ret=%d\n", ret);
		return ret;
	}

	ret = ofnode_parse_phandle_with_args(glink, "mboxes",
					     "#mbox-cells", 0, 0,
					     &args);
	if (ret) {
		log_warning("pmic-glink: missing/invalid mboxes ret=%d\n", ret);
		return ret;
	}

	if (args.args_count < 2 || !ofnode_valid(args.node)) {
		log_warning("pmic-glink: invalid mboxes args_count=%d node_valid=%d\n",
			    args.args_count, ofnode_valid(args.node));
		return -EINVAL;
	}

	ipcc = args.node;
	pg->ipcc_client = args.args[0];
	pg->ipcc_signal = args.args[1];
	log_warning("pmic-glink: DT remote_pid=%u ipcc_client=%u ipcc_signal=%u\n",
		    pg->remote_pid, pg->ipcc_client, pg->ipcc_signal);

	log_warning("pmic-glink: ipcc node valid=%d name=%s\n",
		    ofnode_valid(ipcc),
		    ofnode_valid(ipcc) ? ofnode_get_name(ipcc) : "<none>");

	addr = ofnode_get_addr(ipcc);
	log_warning("pmic-glink: ipcc addr=%llx\n",
		    (unsigned long long)addr);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pg->ipcc = map_sysmem(addr, 0x1000);
	log_warning("pmic-glink: ipcc mapped=%p\n", pg->ipcc);

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 32);
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

	*pg->rx_tail = 0;
	*pg->tx_head = 0;
	pg->lcid = 1;
	pg->next_lcid = pg->lcid + 1;
	pg->next_liid = 1;
	log_warning("pmic-glink: fifo ptrs tx_tail=%08x tx_head=%08x rx_tail=%08x rx_head=%08x\n",
		    le32_to_cpu(*pg->tx_tail), le32_to_cpu(*pg->tx_head),
		    le32_to_cpu(*pg->rx_tail), le32_to_cpu(*pg->rx_head));

	return 0;
}

int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg pg = {};
	int ret;

	if (!altmode)
		return -EINVAL;

	memset(altmode, 0, sizeof(*altmode));

	log_warning("pmic-glink: get_altmode start\n");

	ret = qpg_init(&pg);
	log_warning("pmic-glink: qpg_init ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_send_version(&pg);
	log_warning("pmic-glink: send VERSION ret=%d\n", ret);
	if (ret)
		return ret;

	ret = qpg_drain_until(&pg, altmode, qpg_done_version, 1000);
	log_warning("pmic-glink: wait VERSION_ACK ret=%d version_acked=%d\n",
		    ret, pg.version_acked);
	if (ret)
		return ret;

	ret = qpg_send_open(&pg);
	log_warning("pmic-glink: send OPEN ret=%d lcid=%u\n", ret, pg.lcid);
	if (ret)
		return ret;

	ret = qpg_drain_until(&pg, altmode, qpg_done_open, 1000);
	log_warning("pmic-glink: wait OPEN ret=%d open_acked=%d remote_opened=%d rcid=%u\n",
		    ret, pg.open_acked, pg.remote_opened, pg.rcid);
	if (ret)
		return ret;

	if (!pg.remote_opened) {
		log_warning("pmic-glink: direct channel %s not advertised; ipcrtr_opened=%d ipcrtr_rcid=%u ipcrtr_lcid=%u ipcrtr_ack=%d\n",
			    QPG_CHANNEL_NAME, pg.ipcrtr_opened, pg.ipcrtr_rcid,
			    pg.ipcrtr_lcid, pg.ipcrtr_open_acked);

		if (!pg.ipcrtr_opened) {
			ret = qpg_drain_until(&pg, altmode, qpg_done_ipcrtr_open,
					      1000);
			log_warning("pmic-glink: wait IPCRTR OPEN ret=%d opened=%d ack=%d lcid=%u rcid=%u\n",
				    ret, pg.ipcrtr_opened, pg.ipcrtr_open_acked,
				    pg.ipcrtr_lcid, pg.ipcrtr_rcid);
			if (ret)
				return -ENODEV;
		}

		if (!pg.ipcrtr_open_acked) {
			ret = qpg_drain_until(&pg, altmode, qpg_done_ipcrtr_open,
					      1000);
			log_warning("pmic-glink: wait IPCRTR OPEN_ACK ret=%d opened=%d ack=%d lcid=%u rcid=%u\n",
				    ret, pg.ipcrtr_opened, pg.ipcrtr_open_acked,
				    pg.ipcrtr_lcid, pg.ipcrtr_rcid);
			if (ret)
				return -ENODEV;
		}

		ret = qpg_send_rx_intent_for(&pg, pg.ipcrtr_lcid, pg.ipcrtr_liid);
		log_warning("pmic-glink: send IPCRTR RX_INTENT ret=%d lcid=%u liid=%u\n",
			    ret, pg.ipcrtr_lcid, pg.ipcrtr_liid);
		if (ret)
			return -ENODEV;

		ret = qpg_send_qrtr_ctrl(&pg, QRTR_TYPE_HELLO, 0, 0);
		if (ret)
			return -ENODEV;

		ret = qpg_send_qrtr_ctrl(&pg, QRTR_TYPE_NEW_LOOKUP, 0, 0);
		if (ret)
			return -ENODEV;

		pg.ipcrtr_seen_data = false;
		ret = qpg_drain_until(&pg, altmode, qpg_done_ipcrtr_data,
				      QPG_IPCRTR_DRAIN_MS);
		log_warning("pmic-glink: wait IPCRTR/QRTR data ret=%d seen=%d\n",
			    ret, pg.ipcrtr_seen_data);

		log_warning("pmic-glink: QRTR/IPCRTR discovery complete enough for service logging; PMIC altmode send remains disabled\n");
		return -ENODEV;
	}

	log_warning("pmic-glink: direct PMIC_RTR_ADSP_APPS channel is advertised; raw path still disabled in this debug build\n");
	return -ENODEV;
}
