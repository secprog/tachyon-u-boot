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
#include <dm.h>
#include <dm/lists.h>
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

#include "pmic_glink_internal.h"

/*
 * The GLINK session lives in the pmic-glink core device's private data.
 * qpg_session_get() resolves (and probes) that core device and returns its
 * priv; all transport/session functions operate on this single instance.
 */
struct qpg *qpg_session_get(void)
{
	struct udevice *dev;

	if (uclass_first_device_err(UCLASS_PMIC_GLINK, &dev))
		return NULL;

	return dev_get_priv(dev);
}

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

	log_debug("pmic-glink: IPCC mbox chan dev=%s id=%08lx\n",
		    ipcc_dev->name, chan->id);

	return 0;
}

bool qpg_env_bool(const char *name)
{
	const char *value = env_get(name);

	return value && (!strcmp(value, "1") ||
			 !strcmp(value, "true") ||
			 !strcmp(value, "yes") ||
			 !strcmp(value, "on"));
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
		log_debug("pmic-glink: send OPEN_ACK begin\n");

	ret = qpg_tx(pg, &msg, sizeof(msg), NULL, 0);

	if (cmd == GLINK_CMD_OPEN_ACK)
		log_debug("pmic-glink: send OPEN_ACK end ret=%d\n", ret);

	return ret;
}

static int qpg_send_open_ack(struct qpg *pg, u16 rcid, const char *name)
{
	log_debug("pmic-glink: send OPEN_ACK channel='%s' rcid=%u\n",
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

	log_debug("pmic-glink: send OPEN channel='%s' lcid=%u\n",
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

	log_debug("pmic-glink: send RX_INTENT cid=%u liid=%u size=%u\n",
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
	log_debug("pmic-glink: send RX_INTENT_REQ_ACK cid=%u granted=%d\n",
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

int qpg_send_data(struct qpg *pg, const void *data, size_t len)
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
	log_debug("pmic-glink: IPCRTR sent HELLO -> node=%u ret=%d\n",
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
	log_debug("pmic-glink: servreg REGISTER_LISTENER -> %u:%u path=%s ret=%d\n",
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
	log_debug("pmic-glink: servreg SET_ACK ind_txn=%u ret=%d\n",
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

	log_debug("pmic-glink: IPCRTR qrtr type=%u src=%u:%08x dst=%u:%08x size=%u len=%zu bytes=[%s]\n",
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
			log_debug("pmic-glink: servreg NOTIFIER found svc=0x%x inst=0x%x @ %u:%u\n",
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

		log_debug("pmic-glink: servreg QMI type=%u msg_id=0x%x len=%u\n",
			    qh->type, msg_id, le16_to_cpu(qh->msg_len));

		if (qh->type == QMI_TYPE_RESPONSE &&
		    msg_id == SERVREG_REGISTER_LISTENER_REQ) {
			pg->servreg_registered = true;
			/* current state may be in TLV 0x10 (curr_state) */
			v = qpg_qmi_find_tlv(tlv, tlv_len, 0x10, &vlen);
			if (v && vlen >= 4)
				pg->servreg_last_state = v[0] | (v[1] << 8) |
					(v[2] << 16) | (v[3] << 24);
			log_debug("pmic-glink: servreg REGISTER ack state=%u\n",
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
			log_debug("pmic-glink: servreg STATE_UPDATED state=%u txn=%u -> ack\n",
				    pg->servreg_last_state, itxn);
		}
		return;
	}
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
				log_debug("pmic-glink: UCSI READ_BUFFER ret=%u buf=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
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
				log_debug("pmic-glink: UCSI WRITE_BUFFER ret=%u\n",
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
				log_debug("pmic-glink: UCSI notify cci=%08x receiver=%u len=%zu\n",
					    pg->ucsi_notification,
					    le32_to_cpu(notify->receiver), len);
			} else {
				log_warning("pmic-glink: UCSI notify short len=%zu expected=%zu\n",
					    len, sizeof(*notify));
			}
			break;
		default:
			log_debug("pmic-glink: USB Type-C owner opcode=%02x len=%zu\n",
				    opcode, len);
			break;
		}

		return false;
	}

	if (owner == PMIC_GLINK_OWNER_CHARGER) {
		pg->battmgr_acked = true;
		log_debug("pmic-glink: BATTMGR msg type=%u opcode=%02x len=%zu\n",
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
			log_debug("pmic-glink: USBC READ ret=%u buf=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
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
		log_debug("pmic-glink: PAN ACK received\n");
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

	log_debug("pmic-glink: RX_INTENT_REQ rcid=%u size=%u\n", cid, size);

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

	log_debug("pmic-glink: RX_INTENT_REQ done granted=%d liid=%u ret=%d\n",
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
		log_debug("pmic-glink: RIID channel=raw rcid=%u riid=%u size=%u avail=%d\n",
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
	log_debug("pmic-glink: RX remote OPEN rcid=%u name_len=%u name='%s'%s\n",
		    rcid, name_len, name,
		    name_len >= 32 ? " truncated" : "");

	if (!strcmp(name, QPG_CHANNEL_NAME)) {
		pg->rcid = rcid;
		pg->remote_opened = true;
		pg->remote_open_ack_pending = !pg->remote_open_acked;
		log_debug("pmic-glink: PMIC remote OPEN recorded rcid=%u lcid=%u\n",
			    pg->rcid, pg->lcid);
	} else if (!strcmp(name, QPG_IPCRTR_NAME)) {
		pg->ipcrtr_rcid = rcid;
		pg->ipcrtr_seen = true;
		pg->ipcrtr_open_ack_pending = true;
		log_debug("pmic-glink: IPCRTR remote OPEN recorded rcid=%u lcid=%u\n",
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

int qpg_poll(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
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
	if (qpg_env_bool("qpg_enable_ipcrtr_servreg"))
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
			log_debug("pmic-glink: raw OPEN_ACK complete lcid=%u\n",
				    pg->lcid);
		} else if (param1 == QPG_IPCRTR_LCID) {
			pg->ipcrtr_open_acked = true;
			log_debug("pmic-glink: IPCRTR OPEN_ACK complete lcid=%u\n",
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
		log_debug("pmic-glink: RX_INTENT_REQ_ACK cid=%u granted=%u\n",
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
		log_debug("pmic-glink: service PMIC OPEN_ACK begin rcid=%u\n",
			    pg->rcid);
		ret = qpg_send_open_ack(pg, pg->rcid, QPG_CHANNEL_NAME);
		log_debug("pmic-glink: service PMIC OPEN_ACK end ret=%d\n",
			    ret);
		if (ret)
			return ret;

		pg->remote_open_ack_pending = false;
		pg->remote_open_acked = true;
	}

	if (!pg->local_open_sent) {
		log_debug("pmic-glink: service PMIC local OPEN begin lcid=%u\n",
			    pg->lcid);
		ret = qpg_send_open(pg);
		log_debug("pmic-glink: service PMIC local OPEN end ret=%d\n",
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
		log_debug("pmic-glink: IPCRTR OPEN_ACK sent rcid=%u ret=%d\n",
			    pg->ipcrtr_rcid, ret);
		if (ret)
			return ret;
		pg->ipcrtr_open_ack_pending = false;
	}

	if (!pg->ipcrtr_local_open_sent) {
		ret = qpg_send_open_for(pg, QPG_IPCRTR_LCID, QPG_IPCRTR_NAME);
		log_debug("pmic-glink: IPCRTR local OPEN sent lcid=%u ret=%d\n",
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
		log_debug("pmic-glink: IPCRTR post RX intent lcid=%u liid=%u ret=%d\n",
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

int qpg_drain_until(struct qpg *pg,
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

bool qpg_done_pan_ack(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode)
{
	return pg->pan_acked;
}

bool qpg_done_ucsi_read(struct qpg *pg,
			       struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ucsi_read_acked;
}

bool qpg_done_ucsi_write(struct qpg *pg,
				struct qcom_pmic_glink_altmode *altmode)
{
	return pg->ucsi_write_acked;
}

bool qpg_done_usbc_read(struct qpg *pg,
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

int qpg_send_altmode_req(struct qpg *pg, u32 cmd, u32 arg)
{
	struct qpg_usbc_write_req req = {
		.hdr.owner = cpu_to_le32(PMIC_GLINK_OWNER_USBC_PAN),
		.hdr.type = cpu_to_le32(PMIC_GLINK_REQ_RESP),
		.hdr.opcode = cpu_to_le32(USBC_CMD_WRITE_REQ),
		.cmd = cpu_to_le32(cmd),
		.arg = cpu_to_le32(arg),
	};

	log_debug("pmic-glink: owner=%u channel=%s altmode_cmd=%u arg=%u\n",
		    PMIC_GLINK_OWNER_USBC_PAN, QPG_CHANNEL_NAME, cmd, arg);

	return qpg_send_data(pg, &req, sizeof(req));
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

	/*
	 * Hard dependency: the GLINK transport must never open without the ADSP
	 * remoteproc running.  The board boots the ADSP independently at boot
	 * (board_late_init() -> qcom_adsp_pas_boot()); this call *ensures* it is
	 * up before we touch GLINK and refuses to proceed otherwise.
	 * qcom_adsp_pas_boot() is idempotent (returns 0 immediately when the ADSP
	 * is already booted), so in the normal flow this is just the dependency
	 * guard, but it also covers the case where the early boot was skipped or
	 * failed.
	 */
	pg->last_adsp_boot_ret = -EINPROGRESS;

	ret = uclass_first_device_err(UCLASS_SMEM, &pg->smem);
	log_debug("pmic-glink: smem lookup ret=%d smem=%p\n",
		    ret, pg->smem);
	if (ret)
		return ret;

	ret = qcom_adsp_pas_boot();
	pg->last_adsp_boot_ret = ret;
	log_debug("pmic-glink: ADSP dependency ensure ret=%d\n", ret);
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
	log_debug("pmic-glink: adsp node valid=%d glink node valid=%d\n",
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
	log_debug("pmic-glink: smem_alloc desc ret=%d\n", ret);
	if (ret && ret != -EEXIST)
		return ret;

	descs = smem_get(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 &size);
	log_debug("pmic-glink: descs=%p size=%zu\n", descs, size);
	if (!descs || size != 32)
		return -EINVAL;

	pg->tx_tail = &descs[0];
	pg->tx_head = &descs[1];
	pg->rx_tail = &descs[2];
	pg->rx_head = &descs[3];

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_FIFO_0,
			 SZ_16K);
	tx_exists = ret == -EEXIST;
	log_debug("pmic-glink: smem_alloc tx fifo ret=%d\n", ret);
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
	log_debug("pmic-glink: tx_fifo=%p tx_len=%zu rx_fifo=%p rx_len=%zu\n",
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
		log_debug("pmic-glink: preserving existing fifo ptrs\n");
	}

	pg->lcid = 1;
	log_debug("pmic-glink: fifo ptrs tx_tail=%08x tx_head=%08x rx_tail=%08x rx_head=%08x\n",
		    le32_to_cpu(*pg->tx_tail), le32_to_cpu(*pg->tx_head),
		    le32_to_cpu(*pg->rx_tail), le32_to_cpu(*pg->rx_head));

	return 0;
}

int qpg_open_session(struct qcom_pmic_glink_altmode *altmode,
			    int *adsp_boot_retp, int *glink_open_retp,
			    int *pan_en_retp)
{
	u32 liid;
	int ret;
	struct qpg *pg = qpg_session_get();

	if (!pg)
		return -ENODEV;

	if (adsp_boot_retp)
		*adsp_boot_retp = -EINPROGRESS;
	if (glink_open_retp)
		*glink_open_retp = -EINPROGRESS;
	if (pan_en_retp)
		*pan_en_retp = -EINPROGRESS;

	if (pg->session_ready) {
		log_debug("pmic-glink: reusing session lcid=%u rcid=%u rx_tail=%08x rx_head=%08x\n",
			    pg->lcid, pg->rcid,
			    le32_to_cpu(*pg->rx_tail),
			    le32_to_cpu(*pg->rx_head));
		pg->cached_state.service_started = true;
		pg->cached_state.pan_enabled = true;
		if (adsp_boot_retp)
			*adsp_boot_retp = 0;
		if (glink_open_retp)
			*glink_open_retp = 0;
		if (pan_en_retp)
			*pan_en_retp = 0;
		return 0;
	}

	/*
	 * Reset the transport/session state for a fresh open, but PRESERVE the
	 * cached alt-mode result across the (re)open. Before the session statics
	 * were folded into struct qpg, qpg_cached_altmode/_valid were file-scope
	 * statics that the open path never cleared, so a DP notify parsed during
	 * a previous (possibly failed) open survived into a retry. The whole-
	 * struct memset would otherwise discard it -- save and restore to keep
	 * the original behavior on the failed-open-then-retry path.
	 */
	{
		struct qcom_pmic_glink_altmode saved_altmode = pg->cached_altmode;
		bool saved_altmode_valid = pg->cached_altmode_valid;

		memset(pg, 0, sizeof(*pg));
		pg->cached_altmode = saved_altmode;
		pg->cached_altmode_valid = saved_altmode_valid;
	}
	memset(&pg->cached_state, 0, sizeof(pg->cached_state));
	pg->last_ucsi_prewarm_ret = -EINPROGRESS;

	ret = qpg_init(pg);
	log_debug("pmic-glink: qpg_init ret=%d\n", ret);
	if (adsp_boot_retp)
		*adsp_boot_retp = pg->last_adsp_boot_ret;
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_send_version(pg);
	log_debug("pmic-glink: send VERSION ret=%d\n", ret);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(pg, altmode, qpg_done_version, 1000);
	log_debug("pmic-glink: wait VERSION_ACK ret=%d version_acked=%d\n",
		    ret, pg->version_acked);
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
	ret = qpg_drain_until(pg, altmode, qpg_done_remote_opened,
			      2000);
	log_debug("pmic-glink: wait remote OPEN ret=%d remote_opened=%d rcid=%u\n",
		    ret, pg->remote_opened, pg->rcid);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_service_pmic_open(pg);
	log_debug("pmic-glink: service PMIC OPEN ret=%d local_sent=%d remote_acked=%d\n",
		    ret, pg->local_open_sent,
		    pg->remote_open_acked);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(pg, altmode, qpg_done_open, 2000);
	log_debug("pmic-glink: wait local OPEN_ACK ret=%d open_acked=%d\n",
		    ret, pg->open_acked);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}

	liid = qpg_alloc_liid(pg);
	ret = qpg_send_rx_intent_for(pg, pg->lcid, liid);
	log_debug("pmic-glink: send RX_INTENT ret=%d lcid=%u liid=%u\n",
		    ret, pg->lcid, liid);
	if (ret) {
		if (glink_open_retp)
			*glink_open_retp = ret;
		return ret;
	}
	if (glink_open_retp)
		*glink_open_retp = 0;

	/*
	 * Run the UCSI prewarm (PPM_RESET + SET_NOTIFICATION_ENABLE) by default.
	 * This is the UCSI *initialisation* Linux also performs (ucsi_init); the
	 * SET_UOR data-role swap and GET_CONNECTOR_STATUS that drive DP entry
	 * REQUIRE it — without it SET_UOR returns -110 (cci=0) and connector
	 * status reads connected=0, so the dock never reaches mux=3.  (Confirmed
	 * on HW: skipping it broke entry; running it then bounce reached mux=3.)
	 * The "passive" lesson from the Linux altmode driver is to avoid REPEATED
	 * reset/bounce churn, not to skip the one-time UCSI init.  Escape hatch:
	 * qpg_no_prewarm=1 skips it for experiments.
	 */
	if (!qpg_env_bool("qpg_no_prewarm")) {
		ret = qpg_ucsi_prewarm(pg);
		pg->last_ucsi_prewarm_ret = ret;
		if (ret)
			log_warning("pmic-glink: UCSI prewarm failed ret=%d; continuing PAN\n",
				    ret);
	} else {
		pg->last_ucsi_prewarm_ret = -ENOENT;
		log_warning("pmic-glink: UCSI prewarm SKIPPED (qpg_no_prewarm set)\n");
	}

	pg->pan_acked = false;
	ret = qpg_send_altmode_req(pg, ALTMODE_PAN_EN, 0);
	log_debug("pmic-glink: send PAN_EN ret=%d\n", ret);
	if (ret) {
		if (pan_en_retp)
			*pan_en_retp = ret;
		return ret;
	}

	ret = qpg_drain_until(pg, altmode, qpg_done_pan_ack, 1000);
	log_debug("pmic-glink: wait PAN_ACK ret=%d pan_acked=%d\n",
		    ret, pg->pan_acked);
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
	if (qpg_env_bool("qpg_enable_battmgr"))
		qpg_register_battmgr(pg);

	/*
	 * Second, post-PAN UCSI sequence — the same one the manual `qpg ucsi`
	 * command ran after opening the session. Prewarm (above, before PAN_EN)
	 * is required but not sufficient: this post-PAN pass is what actually
	 * drives the ADSP to enter DisplayPort alt mode. Best-effort, so a
	 * hiccup here does not tear down an otherwise-open session. Disable with
	 * qpg_no_ucsi_discover=1 for experiments.
	 */
	if (!qpg_env_bool("qpg_no_ucsi_discover")) {
		int disc = qpg_ucsi_discover(pg);

		log_debug("pmic-glink: UCSI discover ret=%d\n", disc);
	}

	pg->session_ready = true;
	pg->cached_state.service_started = true;
	pg->cached_state.pan_enabled = true;

	return 0;
}

static int qcom_pmic_glink_probe(struct udevice *dev)
{
	return 0;
}

static int qcom_pmic_glink_bind(struct udevice *dev)
{
	struct udevice *child;
	int ret;

	if (IS_ENABLED(CONFIG_QCOM_PMIC_GLINK_ALTMODE)) {
		ret = device_bind_driver(dev, "qcom_pmic_glink_altmode",
					 "pmic_glink_altmode", &child);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_QCOM_PMIC_GLINK_UCSI)) {
		ret = device_bind_driver(dev, "qcom_pmic_glink_ucsi",
					 "pmic_glink_ucsi", &child);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_QCOM_PMIC_GLINK_BATTMGR)) {
		ret = device_bind_driver(dev, "qcom_pmic_glink_battmgr",
					 "pmic_glink_battmgr", &child);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct udevice_id qcom_pmic_glink_ids[] = {
	{ .compatible = "qcom,pmic-glink" },
	{ }
};

U_BOOT_DRIVER(qcom_pmic_glink) = {
	.name		= "qcom_pmic_glink",
	.id		= UCLASS_PMIC_GLINK,
	.of_match	= qcom_pmic_glink_ids,
	.bind		= qcom_pmic_glink_bind,
	.probe		= qcom_pmic_glink_probe,
	.priv_auto	= sizeof(struct qpg),
};

UCLASS_DRIVER(pmic_glink) = {
	.name		= "pmic_glink",
	.id		= UCLASS_PMIC_GLINK,
};

/*
 * The client child drivers (altmode / UCSI / battmgr, UCLASS_MISC) live in
 * pmic_glink_altmode.c / pmic_glink_ucsi.c / pmic_glink_battmgr.c. They are
 * bound by name from qcom_pmic_glink_bind() above and reach the GLINK
 * transport through this core via the shared struct qpg session.
 */
