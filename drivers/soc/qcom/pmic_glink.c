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
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <log.h>
#include <mapmem.h>
#include <smem.h>
#include <soc/qcom/pmic_glink.h>

#define QPG_IPCC_CLIENT_LPASS			3
#define QPG_IPCC_SIGNAL_GLINK_QMP		0

#define QPG_SMEM_XPRT_DESCRIPTOR		478
#define QPG_SMEM_XPRT_FIFO_0			479
#define QPG_SMEM_XPRT_FIFO_1			480

#define QPG_IPCC_REG_SEND_ID			0x0c
#define QPG_FIFO_FULL_RESERVE			8
#define QPG_TX_BLOCKED_CMD_RESERVE		8
#define QPG_RX_INTENT_SIZE			512
#define QPG_CHANNEL_NAME			"PMIC_RTR_ADSP_APPS"

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
	bool riid_avail;
	bool version_acked;
	bool open_acked;
	bool remote_opened;
	bool pan_acked;
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
	writel(qpg_hwirq(pg->ipcc_client, pg->ipcc_signal),
	       pg->ipcc + QPG_IPCC_REG_SEND_ID);
}

static int qpg_tx(struct qpg *pg, const void *hdr, size_t hlen,
		  const void *data, size_t dlen)
{
	size_t len = ALIGN(hlen + dlen, 8);
	u32 head, next;

	if (len > pg->tx_len || qpg_tx_avail(pg) < len)
		return -EAGAIN;

	head = le32_to_cpu(*pg->tx_head);
	next = head + len;

	head = qpg_tx_write_one(pg, head, hdr, hlen);
	if (dlen)
		head = qpg_tx_write_one(pg, head, data, dlen);

	if (next >= pg->tx_len)
		next %= pg->tx_len;

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

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_send_version(struct qpg *pg)
{
	return qpg_send_simple(pg, GLINK_CMD_VERSION, GLINK_VERSION_1,
			       GLINK_FEATURE_INTENT_REUSE);
}

static int qpg_send_open(struct qpg *pg)
{
	struct {
		struct qpg_msg msg;
		char name[32];
	} __packed req = {};
	size_t name_len = strlen(QPG_CHANNEL_NAME) + 1;

	req.msg.cmd = cpu_to_le16(GLINK_CMD_OPEN);
	req.msg.param1 = cpu_to_le16(pg->lcid);
	req.msg.param2 = cpu_to_le32(name_len);
	strcpy(req.name, QPG_CHANNEL_NAME);

	return qpg_tx(pg, &req, ALIGN(sizeof(req.msg) + name_len, 8), NULL, 0);
}

static int qpg_send_rx_intent(struct qpg *pg)
{
	struct {
		__le16 cmd;
		__le16 lcid;
		__le32 count;
		__le32 size;
		__le32 liid;
	} __packed msg = {
		.cmd = cpu_to_le16(GLINK_CMD_INTENT),
		.lcid = cpu_to_le16(pg->lcid),
		.count = cpu_to_le32(1),
		.size = cpu_to_le32(QPG_RX_INTENT_SIZE),
		.liid = cpu_to_le32(1),
	};

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_send_rx_done(struct qpg *pg, u32 liid)
{
	struct {
		__le16 cmd;
		__le16 lcid;
		__le32 liid;
	} __packed msg = {
		.cmd = cpu_to_le16(GLINK_CMD_RX_DONE_W_REUSE),
		.lcid = cpu_to_le16(pg->lcid),
		.liid = cpu_to_le32(liid),
	};

	return qpg_tx(pg, &msg, sizeof(msg), NULL, 0);
}

static int qpg_wait_riid(struct qpg *pg);

static int qpg_send_data(struct qpg *pg, const void *data, size_t len)
{
	struct {
		struct qpg_msg msg;
		__le32 chunk_size;
		__le32 left_size;
	} __packed hdr;
	int ret;

	ret = qpg_wait_riid(pg);
	if (ret)
		return ret;

	hdr.msg.cmd = cpu_to_le16(GLINK_CMD_TX_DATA);
	hdr.msg.param1 = cpu_to_le16(pg->lcid);
	hdr.msg.param2 = cpu_to_le32(pg->riid);
	hdr.chunk_size = cpu_to_le32(len);
	hdr.left_size = 0;

	pg->riid_avail = false;

	return qpg_tx(pg, &hdr, sizeof(hdr), data, len);
}

static void qpg_parse_sc8280xp_notify(struct qcom_pmic_glink_altmode *altmode,
				      const void *data, size_t len)
{
	const struct qpg_usbc_notify *notify = data;
	u8 mode;
	u16 svid;

	if (len != sizeof(*notify))
		return;

	svid = le32_to_cpu(notify->hdr.opcode) >> 16;
	if (svid != USB_TYPEC_DP_SID)
		return;

	mode = notify->payload[8] & SC8280XP_DPAM_MASK;
	if (mode < DPAM_HPD_A)
		return;

	altmode->port = notify->payload[0];
	altmode->orientation = qpg_orientation(notify->payload[1]);
	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->hpd = !!(notify->payload[8] & SC8280XP_HPD_STATE_MASK);
	altmode->hpd_irq = !!(notify->payload[8] & SC8280XP_HPD_IRQ_MASK);
	altmode->dp = true;
}

static void qpg_parse_sc8180x_notify(struct qcom_pmic_glink_altmode *altmode,
				     const void *data, size_t len)
{
	const struct qpg_usbc_sc8180x_notify *msg = data;
	u32 notification;
	u8 mode;
	u8 mux;

	if (len != sizeof(*msg))
		return;

	notification = le32_to_cpu(msg->notification);
	mux = (notification & SC8180X_MUX_MASK) >> 16;
	if (mux != 2)
		return;

	mode = (notification & SC8180X_MODE_MASK) >> 24;
	if (mode < DPAM_HPD_A)
		return;

	altmode->port = notification & SC8180X_PORT_MASK;
	altmode->orientation =
		qpg_orientation((notification & SC8180X_ORIENTATION_MASK) >> 8);
	altmode->pin_assignment = mode - DPAM_HPD_A;
	altmode->hpd = !!(notification & SC8180X_HPD_STATE_MASK);
	altmode->hpd_irq = !!(notification & SC8180X_HPD_IRQ_MASK);
	altmode->dp = true;
}

static void qpg_parse_pmic(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode,
			   const void *data, size_t len)
{
	const struct qpg_pmic_hdr *hdr = data;
	u16 opcode;

	if (len < sizeof(*hdr))
		return;

	opcode = le32_to_cpu(hdr->opcode) & 0xff;

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

	if (avail < sizeof(hdr))
		return -EAGAIN;

	qpg_rx_peek(pg, &hdr, 0, sizeof(hdr));
	chunk_size = le32_to_cpu(hdr.chunk_size);
	liid = le32_to_cpu(hdr.msg.param2);

	if (chunk_size > sizeof(payload) || avail < sizeof(hdr) + chunk_size)
		return -EAGAIN;

	qpg_rx_peek(pg, payload, sizeof(hdr), chunk_size);
	qpg_rx_advance(pg, ALIGN(sizeof(hdr) + chunk_size, 8));

	if (le16_to_cpu(hdr.msg.param1) == pg->lcid && liid == 1)
		qpg_parse_pmic(pg, altmode, payload, chunk_size);

	qpg_send_rx_done(pg, liid);

	return 0;
}

static int qpg_rx_intent(struct qpg *pg, size_t avail, u16 cid, u32 count)
{
	struct {
		struct qpg_msg msg;
		__le32 size;
		__le32 iid;
	} __packed intent;

	if (!count)
		return -EINVAL;

	if (avail < sizeof(intent))
		return -EAGAIN;

	qpg_rx_peek(pg, &intent, 0, sizeof(intent));
	qpg_rx_advance(pg, ALIGN(sizeof(intent), 8));

	if (cid == pg->lcid) {
		pg->riid_size = le32_to_cpu(intent.size);
		pg->riid = le32_to_cpu(intent.iid);
		pg->riid_avail = pg->riid_size > 0;
	}

	return 0;
}

static int qpg_rx_open(struct qpg *pg, size_t avail, u16 rcid, u32 name_len)
{
	char name[32] = {};

	if (name_len >= sizeof(name) || avail < ALIGN(sizeof(struct qpg_msg) + name_len, 8))
		return -EAGAIN;

	qpg_rx_peek(pg, name, sizeof(struct qpg_msg), name_len);
	qpg_rx_advance(pg, ALIGN(sizeof(struct qpg_msg) + name_len, 8));

	if (!strcmp(name, QPG_CHANNEL_NAME)) {
		pg->rcid = rcid;
		pg->remote_opened = true;
		qpg_send_simple(pg, GLINK_CMD_OPEN_ACK, pg->rcid, 0);
	}

	return 0;
}

static int qpg_poll(struct qpg *pg, struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg_msg msg;
	size_t avail;
	u16 cmd, param1;
	u32 param2;
	int ret = 0;

	avail = qpg_rx_avail(pg);
	if (avail < sizeof(msg))
		return -EAGAIN;

	qpg_rx_peek(pg, &msg, 0, sizeof(msg));
	cmd = le16_to_cpu(msg.cmd);
	param1 = le16_to_cpu(msg.param1);
	param2 = le32_to_cpu(msg.param2);

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
		ret = qpg_rx_open(pg, avail, param1, param2);
		break;
	case GLINK_CMD_OPEN_ACK:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		if (param1 == pg->lcid)
			pg->open_acked = true;
		break;
	case GLINK_CMD_INTENT:
		ret = qpg_rx_intent(pg, avail, param1, param2);
		break;
	case GLINK_CMD_TX_DATA:
	case GLINK_CMD_TX_DATA_CONT:
		ret = qpg_rx_data(pg, altmode, avail);
		break;
	case GLINK_CMD_RX_DONE:
	case GLINK_CMD_RX_DONE_W_REUSE:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		if (param2 == pg->riid)
			pg->riid_avail = cmd == GLINK_CMD_RX_DONE_W_REUSE;
		break;
	case GLINK_CMD_READ_NOTIF:
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		qpg_kick(pg);
		break;
	default:
		log_debug("pmic-glink: unhandled cmd %u\n", cmd);
		qpg_rx_advance(pg, ALIGN(sizeof(msg), 8));
		break;
	}

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
	return pg->open_acked && pg->remote_opened;
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

static bool qpg_done_altmode(struct qpg *pg,
			     struct qcom_pmic_glink_altmode *altmode)
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

	return qpg_drain_until(pg, &altmode, qpg_done_riid, 500);
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

	pg->pan_acked = false;

	return qpg_send_data(pg, &req, sizeof(req));
}

static int qpg_init(struct qpg *pg)
{
	struct ofnode_phandle_args args;
	ofnode ipcc;
	ofnode adsp;
	ofnode glink = ofnode_null();
	fdt_addr_t addr;
	size_t size;
	__le32 *descs;
	int ret;

	ret = uclass_first_device_err(UCLASS_SMEM, &pg->smem);
	if (ret)
		return ret;

	pg->remote_pid = 2;
	pg->ipcc_client = QPG_IPCC_CLIENT_LPASS;
	pg->ipcc_signal = QPG_IPCC_SIGNAL_GLINK_QMP;

	adsp = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-adsp-pas");
	if (ofnode_valid(adsp))
		ofnode_for_each_subnode(glink, adsp) {
			const char *label = ofnode_read_string(glink, "label");

			if (label && !strcmp(label, "lpass"))
				break;
		}

	if (ofnode_valid(glink)) {
		u32 remote_pid;

		if (!ofnode_read_u32(glink, "qcom,remote-pid", &remote_pid))
			pg->remote_pid = remote_pid;

		ret = ofnode_parse_phandle_with_args(glink, "mboxes",
						     "#mbox-cells", 0, 0,
						     &args);
		if (!ret && args.args_count >= 2) {
			ipcc = args.node;
			pg->ipcc_client = args.args[0];
			pg->ipcc_signal = args.args[1];
		} else {
			ipcc = ofnode_null();
		}
	} else {
		ipcc = ofnode_null();
	}

	if (!ofnode_valid(ipcc))
		ipcc = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-ipcc");
	if (!ofnode_valid(ipcc))
		ipcc = ofnode_by_compatible(ofnode_null(), "qcom,ipcc");
	if (!ofnode_valid(ipcc))
		return -ENOENT;

	addr = ofnode_get_addr(ipcc);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pg->ipcc = map_sysmem(addr, 0x1000);

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 32);
	if (ret && ret != -EEXIST)
		return ret;

	descs = smem_get(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_DESCRIPTOR,
			 &size);
	if (!descs || size != 32)
		return -EINVAL;

	pg->tx_tail = &descs[0];
	pg->tx_head = &descs[1];
	pg->rx_tail = &descs[2];
	pg->rx_head = &descs[3];

	ret = smem_alloc(pg->smem, pg->remote_pid, QPG_SMEM_XPRT_FIFO_0,
			 SZ_16K);
	if (ret && ret != -EEXIST)
		return ret;

	pg->tx_fifo = smem_get(pg->smem, pg->remote_pid,
			       QPG_SMEM_XPRT_FIFO_0, &pg->tx_len);
	pg->rx_fifo = smem_get(pg->smem, pg->remote_pid,
			       QPG_SMEM_XPRT_FIFO_1, &pg->rx_len);
	if (!pg->tx_fifo || !pg->rx_fifo)
		return -ENOENT;

	*pg->rx_tail = 0;
	*pg->tx_head = 0;
	pg->lcid = 1;

	return 0;
}

int qcom_pmic_glink_get_altmode(struct qcom_pmic_glink_altmode *altmode)
{
	struct qpg pg = {};
	int ret;

	if (!altmode)
		return -EINVAL;

	memset(altmode, 0, sizeof(*altmode));

	ret = qpg_init(&pg);
	if (ret)
		return ret;

	ret = qpg_send_version(&pg);
	if (ret)
		return ret;

	ret = qpg_drain_until(&pg, altmode, qpg_done_version, 1000);
	if (ret)
		return ret;

	ret = qpg_send_open(&pg);
	if (ret)
		return ret;

	ret = qpg_drain_until(&pg, altmode, qpg_done_open, 1000);
	if (ret)
		return ret;

	ret = qpg_send_rx_intent(&pg);
	if (ret)
		return ret;

	ret = qpg_send_altmode_req(&pg, ALTMODE_PAN_EN, 0);
	if (ret)
		return ret;

	ret = qpg_drain_until(&pg, altmode, qpg_done_pan_ack, 1000);
	if (ret)
		log_debug("pmic-glink: PAN enable ack timeout: %d\n", ret);

	ret = qpg_drain_until(&pg, altmode, qpg_done_altmode, 2500);
	if (ret)
		return ret;

	qpg_send_altmode_req(&pg, ALTMODE_PAN_ACK, altmode->port);

	return 0;
}
