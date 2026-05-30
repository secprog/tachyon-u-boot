// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm AOSS QMP (Qualcomm Messaging Protocol) driver
 *
 * Communicates with the AOP (Always-On Processor) via shared memory
 * (MSGRAM) + IPCC doorbell. Used to toggle remote processor load states,
 * request clocks, and other AOP-managed resources.
 *
 * This follows the Linux drivers/soc/qcom/qcom_aoss.c on-wire protocol —
 * the descriptor table layout, handshake sequence, and message format are
 * identical.  Linux uses wait_event_timeout() driven by hardware
 * interrupts; U-Boot's wait_event_timeout() polls with
 * get_timer()/cpu_relax() (U-Boot is single-threaded and lacks wait
 * queues / interrupt-driven wakeups).
 *
 *   - probe: qmp_open() link+channel handshake via descriptor table
 *   - qmp_send(): write msg body at (msgram + mbox_offset + 4), length at
 *     (msgram + mbox_offset), kick IPCC, wait_event_timeout() for ack
 *   - remove: qmp_close() by writing QMP_STATE_DOWN to both states
 *
 * MSGRAM layout (first 0x40 bytes = descriptor table):
 *   0x00  QMP_DESC_MAGIC           (R)  "MAIL" = 0x4d41494c
 *   0x04  QMP_DESC_VERSION         (R)  must be 1
 *   0x08  QMP_DESC_FEATURES        (R)  unused
 *   0x0c  QMP_DESC_UCORE_LINK_STATE     (R)  AOP link state
 *   0x10  QMP_DESC_UCORE_LINK_STATE_ACK (W)  ack AOP link state
 *   0x14  QMP_DESC_UCORE_CH_STATE       (R)  AOP channel state
 *   0x18  QMP_DESC_UCORE_CH_STATE_ACK   (W)  ack AOP channel state
 *   0x1c  QMP_DESC_UCORE_MBOX_SIZE      (R)  AOP mailbox size
 *   0x20  QMP_DESC_UCORE_MBOX_OFFSET    (R)  AOP mailbox offset
 *   0x24  QMP_DESC_MCORE_LINK_STATE     (W)  Linux link state
 *   0x28  QMP_DESC_MCORE_LINK_STATE_ACK (R)  AOP ack of Linux link
 *   0x2c  QMP_DESC_MCORE_CH_STATE       (W)  Linux channel state
 *   0x30  QMP_DESC_MCORE_CH_STATE_ACK   (R)  AOP ack of Linux channel
 *   0x34  QMP_DESC_MCORE_MBOX_SIZE      (R)  Linux mailbox size
 *   0x38  QMP_DESC_MCORE_MBOX_OFFSET    (R)  Linux mailbox offset
 *
 * DT binding: compatible = "qcom,sc7280-aoss-qmp", "qcom,aoss-qmp"
 */

#include <dm.h>
#include <dm/device_compat.h>
#include <dm/uclass.h>
#include <mailbox.h>
#include <soc/qcom/qcom_aoss_qmp.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/compat.h>
#include <linux/kernel.h>
#include <stdarg.h>

/* Descriptor table offsets (u32 words relative to MSGRAM base) */
#define QMP_DESC_MAGIC			0x0
#define QMP_DESC_VERSION		0x4
#define QMP_DESC_FEATURES		0x8

/* AOP-side state (read-only for Linux) */
#define QMP_DESC_UCORE_LINK_STATE	0x0c
#define QMP_DESC_UCORE_LINK_STATE_ACK	0x10
#define QMP_DESC_UCORE_CH_STATE		0x14
#define QMP_DESC_UCORE_CH_STATE_ACK	0x18
#define QMP_DESC_UCORE_MBOX_SIZE	0x1c
#define QMP_DESC_UCORE_MBOX_OFFSET	0x20

/* Linux-side state (read/write) */
#define QMP_DESC_MCORE_LINK_STATE	0x24
#define QMP_DESC_MCORE_LINK_STATE_ACK	0x28
#define QMP_DESC_MCORE_CH_STATE		0x2c
#define QMP_DESC_MCORE_CH_STATE_ACK	0x30
#define QMP_DESC_MCORE_MBOX_SIZE	0x34
#define QMP_DESC_MCORE_MBOX_OFFSET	0x38

/* State values: lower 16 bits = UP, upper 16 bits = DOWN */
#define QMP_STATE_UP			GENMASK(15, 0)
#define QMP_STATE_DOWN			GENMASK(31, 16)

#define QMP_MAGIC			0x4d41494c	/* "MAIL" LE */
#define QMP_VERSION			1
#define QMP_MSG_LEN			64		/* 64-byte messages */
#define QMP_HANDSHAKE_TIMEOUT_MS	1000
#define QMP_REPLY_TIMEOUT_MS		2000

struct qcom_aoss_qmp_priv {
	void __iomem *msgram;		/* MSGRAM base */
	struct mbox_chan mbox_chan;	/* IPCC doorbell */
	u32 mbox_offset;		/* offset for message area (from DT) */
	u32 mbox_size;			/* size of message area */
};

static int qmp_kick(struct qcom_aoss_qmp_priv *priv)
{
	return mbox_send(&priv->mbox_chan, NULL);
}

/*
 * qmp_open() — link+channel handshake (same on-wire sequence as Linux).
 *
 * Linux uses wait_event_timeout() driven by hardware interrupts; U-Boot's
 * wait_event_timeout() polls with get_timer()/cpu_relax().
 *
 * 1. Validate magic and version
 * 2. Read our mailbox offset and size from descriptor table
 * 3. ACK remote link state, set our link state UP, wait for ACK
 * 4. Set our channel state UP, wait for remote channel UP, ACK it,
 *    wait for remote ACK
 */
static int qmp_open(struct udevice *dev, struct qcom_aoss_qmp_priv *priv)
{
	u32 val;
	int ret;

	/* Step 1: validate magic and version */
	if (readl(priv->msgram + QMP_DESC_MAGIC) != QMP_MAGIC) {
		dev_err(dev, "qcom-aoss-qmp: bad magic %08x\n",
			readl(priv->msgram + QMP_DESC_MAGIC));
		return -EINVAL;
	}

	val = readl(priv->msgram + QMP_DESC_VERSION);
	if (val != QMP_VERSION) {
		dev_err(dev, "qcom-aoss-qmp: unsupported version %u\n", val);
		return -EINVAL;
	}

	/* Step 2: read mailbox offset and size from Linux side descriptor */
	priv->mbox_offset = readl(priv->msgram + QMP_DESC_MCORE_MBOX_OFFSET);
	priv->mbox_size = readl(priv->msgram + QMP_DESC_MCORE_MBOX_SIZE);

	dev_dbg(dev, "qcom-aoss-qmp: mbox offset=%08x size=%u\n",
		priv->mbox_offset, priv->mbox_size);

	if (!priv->mbox_size) {
		dev_err(dev, "qcom-aoss-qmp: invalid mailbox size 0\n");
		return -EINVAL;
	}

	if (priv->mbox_offset + priv->mbox_size > 0x400) {
		dev_err(dev, "qcom-aoss-qmp: mailbox beyond MSGRAM (off=%x size=%x)\n",
			priv->mbox_offset, priv->mbox_size);
		return -EINVAL;
	}

	/* Step 3: link handshake */
	/* Ack remote core's link state */
	val = readl(priv->msgram + QMP_DESC_UCORE_LINK_STATE);
	writel(val, priv->msgram + QMP_DESC_UCORE_LINK_STATE_ACK);

	/* Set local core's link state to UP */
	writel(QMP_STATE_UP, priv->msgram + QMP_DESC_MCORE_LINK_STATE);
	ret = qmp_kick(priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: link kick failed ret=%d\n", ret);
		goto timeout_close_link;
	}

	ret = wait_event_timeout(NULL,
		readl(priv->msgram + QMP_DESC_MCORE_LINK_STATE_ACK) ==
			QMP_STATE_UP,
		QMP_HANDSHAKE_TIMEOUT_MS);
	if (!ret) {
		dev_err(dev, "qcom-aoss-qmp: link ack timeout after %u ms\n",
			QMP_HANDSHAKE_TIMEOUT_MS);
		goto timeout_close_link;
	}

	/* Step 4: channel handshake */
	writel(QMP_STATE_UP, priv->msgram + QMP_DESC_MCORE_CH_STATE);
	ret = qmp_kick(priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: channel kick failed ret=%d\n", ret);
		goto timeout_close_channel;
	}

	ret = wait_event_timeout(NULL,
		readl(priv->msgram + QMP_DESC_UCORE_CH_STATE) == QMP_STATE_UP,
		QMP_HANDSHAKE_TIMEOUT_MS);
	if (!ret) {
		dev_err(dev, "qcom-aoss-qmp: ucore channel up timeout after %u ms\n",
			QMP_HANDSHAKE_TIMEOUT_MS);
		goto timeout_close_channel;
	}

	/* Ack remote core's channel state */
	writel(QMP_STATE_UP, priv->msgram + QMP_DESC_UCORE_CH_STATE_ACK);
	ret = qmp_kick(priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: channel ack kick failed ret=%d\n", ret);
		goto timeout_close_channel;
	}

	ret = wait_event_timeout(NULL,
		readl(priv->msgram + QMP_DESC_MCORE_CH_STATE_ACK) ==
			QMP_STATE_UP,
		QMP_HANDSHAKE_TIMEOUT_MS);
	if (!ret) {
		dev_err(dev, "qcom-aoss-qmp: channel ack timeout after %u ms\n",
			QMP_HANDSHAKE_TIMEOUT_MS);
		goto timeout_close_channel;
	}

	return 0;

timeout_close_channel:
	writel(QMP_STATE_DOWN, priv->msgram + QMP_DESC_MCORE_CH_STATE);

timeout_close_link:
	writel(QMP_STATE_DOWN, priv->msgram + QMP_DESC_MCORE_LINK_STATE);
	qmp_kick(priv);

	return -ETIMEDOUT;
}

/*
 * qmp_close() — shutdown (same on-wire sequence as Linux).
 * Writes DOWN to our channel and link states, kicks AOP.
 */
static void qmp_close(struct qcom_aoss_qmp_priv *priv)
{
	writel(QMP_STATE_DOWN, priv->msgram + QMP_DESC_MCORE_CH_STATE);
	writel(QMP_STATE_DOWN, priv->msgram + QMP_DESC_MCORE_LINK_STATE);
	qmp_kick(priv);
}

/*
 * qmp_send() — message transmission (same on-wire sequence as Linux).
 *
 * 1. Format message into 64-byte buffer (zero-padded)
 * 2. Write body word-by-word at msgram + mbox_offset + sizeof(u32)
 * 3. Write length (always 64) at msgram + mbox_offset
 * 4. Read back the length to flush (acts as barrier)
 * 5. Kick IPCC
 * 6. wait_event_timeout() for AOP to clear the length field (ack)
 */
int qcom_aoss_qmp_send(struct udevice *dev, const char *fmt, ...)
{
	struct qcom_aoss_qmp_priv *priv = dev_get_priv(dev);
	char buf[QMP_MSG_LEN];
	va_list args;
	int len;
	int ret;
	int i;

	memset(buf, 0, sizeof(buf));
	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (len >= QMP_MSG_LEN) {
		dev_err(dev, "qcom-aoss-qmp: message too long (%d > %d)\n",
			len, QMP_MSG_LEN - 1);
		return -EINVAL;
	}

	dev_dbg(dev, "qcom-aoss-qmp: msg='%s' len=%d\n", buf, len);

	/*
	 * Write message body word-by-word at offset + 4.
	 * The MSGRAM only implements 32-bit accesses.
	 */
	for (i = 0; i < QMP_MSG_LEN / (int)sizeof(u32); i++) {
		u32 word;

		memcpy(&word, &buf[i * sizeof(u32)], sizeof(word));
		writel(word, priv->msgram + priv->mbox_offset +
		       sizeof(u32) + i * sizeof(u32));
	}

	/* Write the message length */
	writel(QMP_MSG_LEN, priv->msgram + priv->mbox_offset);

	/* Read back to flush write and guarantee AOP sees the data */
	readl(priv->msgram + priv->mbox_offset);

	/* Kick IPCC to wake AOP */
	ret = qmp_kick(priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: send kick failed ret=%d\n", ret);
		writel(0, priv->msgram + priv->mbox_offset);
		return ret;
	}

	/* Wait for AOP to acknowledge by clearing the length field */
	ret = wait_event_timeout(NULL,
		readl(priv->msgram + priv->mbox_offset) == 0,
		QMP_REPLY_TIMEOUT_MS);
	if (!ret) {
		dev_err(dev, "qcom-aoss-qmp: reply timeout after %u ms\n",
			QMP_REPLY_TIMEOUT_MS);
		/* Clear message from buffer so channel is ready for reuse */
		writel(0, priv->msgram + priv->mbox_offset);
		return -ETIMEDOUT;
	}

	return 0;
}

int qcom_aoss_qmp_load_state(struct udevice *dev, const char *name, bool on)
{
	return qcom_aoss_qmp_send(dev,
		"{class: image, res: load_state, name: %s, val: %s}",
		name, on ? "on" : "off");
}

int qcom_aoss_qmp_get_by_node(ofnode rproc, struct udevice **devp)
{
	struct ofnode_phandle_args args;
	int ret;

	ret = ofnode_parse_phandle_with_args(rproc, "qcom,qmp",
					     NULL, 0, 0, &args);
	if (ret)
		return ret;

	ret = uclass_get_device_by_ofnode(UCLASS_MISC, args.node, devp);
	if (ret)
		return ret;

	return 0;
}

static int qcom_aoss_qmp_probe(struct udevice *dev)
{
	struct qcom_aoss_qmp_priv *priv = dev_get_priv(dev);
	int ret;

	priv->msgram = dev_read_addr_ptr(dev);
	if (!priv->msgram) {
		dev_err(dev, "qcom-aoss-qmp: failed to map MSGRAM\n");
		return -EINVAL;
	}

	dev_dbg(dev, "qcom-aoss-qmp: msgram=%p\n", priv->msgram);

	/*
	 * Get the IPCC mailbox channel from DT.
	 * aoss_qmp DT node: mboxes = <&ipcc IPCC_CLIENT_AOP
	 *                              IPCC_MPROC_SIGNAL_GLINK_QMP>;
	 */
	ret = mbox_get_by_index(dev, 0, &priv->mbox_chan);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: failed to get IPCC mbox ret=%d\n",
			ret);
		return ret;
	}

	dev_dbg(dev, "qcom-aoss-qmp: IPCC mbox chan id=%08lx\n",
		priv->mbox_chan.id);

	/* Link+channel handshake (same on-wire sequence as Linux) */
	ret = qmp_open(dev, priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: handshake failed ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int qcom_aoss_qmp_remove(struct udevice *dev)
{
	struct qcom_aoss_qmp_priv *priv = dev_get_priv(dev);

	qmp_close(priv);

	return 0;
}

static const struct udevice_id qcom_aoss_qmp_ids[] = {
	{ .compatible = "qcom,sc7280-aoss-qmp" },
	{ .compatible = "qcom,aoss-qmp" },
	{ }
};

U_BOOT_DRIVER(qcom_aoss_qmp) = {
	.name		= "qcom_aoss_qmp",
	.id		= UCLASS_MISC,
	.of_match	= qcom_aoss_qmp_ids,
	.probe		= qcom_aoss_qmp_probe,
	.remove		= qcom_aoss_qmp_remove,
	.priv_auto	= sizeof(struct qcom_aoss_qmp_priv),
};
