// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm AOSS QMP (Qualcomm Messaging Protocol) driver
 *
 * Communicates with the AOP (Always-On Processor) via shared memory
 * (MSGRAM) + IPCC doorbell. Used to toggle remote processor load states,
 * request clocks, and other AOP-managed resources.
 *
 * Linux reference: drivers/soc/qcom/qcom_aoss.c
 * DT binding: compatible = "qcom,sc7280-aoss-qmp", "qcom,aoss-qmp"
 */

#include <dm.h>
#include <dm/device_compat.h>
#include <dm/uclass.h>
#include <mailbox.h>
#include <malloc.h>
#include <soc/qcom/qcom_aoss_qmp.h>
#include <asm/io.h>
#include <linux/delay.h>

/* MSGRAM layout (offsets in u32 units, 0x0..0x3ff = 256 u32 words) */
#define QMP_OFF_MAGIC		0	/* offset 0x00 */
#define QMP_OFF_VERSION		1	/* offset 0x04 */
#define QMP_OFF_FEATURES	2	/* offset 0x08 */
#define QMP_OFF_MSG_LEN		3	/* offset 0x0c */
#define QMP_OFF_MSG_BODY	4	/* offset 0x10 */

#define QMP_MAGIC		0x4d41494c	/* "MAIL" little-endian */
#define QMP_MAGIC_REPLY		0x4c49414d	/* "LIAM" little-endian */
#define QMP_VERSION		1
#define QMP_MSGRAM_SIZE		0x400		/* 1KB */
#define QMP_MSG_BODY_MAX	(QMP_MSGRAM_SIZE - (QMP_OFF_MSG_BODY * 4))
#define QMP_REPLY_TIMEOUT_MS	2000		/* 2 seconds */
#define QMP_REPLY_POLL_MS	10

struct qcom_aoss_qmp_priv {
	void __iomem *msgram;
	struct mbox_chan mbox_chan;
};

static int qmp_wait_for_reply(struct qcom_aoss_qmp_priv *priv)
{
	int retry;

	for (retry = 0; retry < (QMP_REPLY_TIMEOUT_MS / QMP_REPLY_POLL_MS);
	     retry++) {
		mdelay(QMP_REPLY_POLL_MS);

		if (readl(priv->msgram + QMP_OFF_MAGIC) == QMP_MAGIC_REPLY)
			return 0;
	}

	return -ETIMEDOUT;
}

int qcom_aoss_qmp_send(struct udevice *dev, const char *fmt, ...)
{
	struct qcom_aoss_qmp_priv *priv = dev_get_priv(dev);
	char *buf;
	va_list args;
	int msglen;
	int ret;

	buf = malloc(QMP_MSG_BODY_MAX);
	if (!buf)
		return -ENOMEM;

	va_start(args, fmt);
	msglen = vsnprintf(buf, QMP_MSG_BODY_MAX, fmt, args);
	va_end(args);

	if (msglen < 0 || msglen >= QMP_MSG_BODY_MAX) {
		free(buf);
		return -EINVAL;
	}

	dev_dbg(dev, "qcom-aoss-qmp: msg='%s' len=%d\n", buf, msglen);

	/* Write message to MSGRAM */
	writel(QMP_MAGIC, priv->msgram + QMP_OFF_MAGIC);
	writel(QMP_VERSION, priv->msgram + QMP_OFF_VERSION);
	writel(0, priv->msgram + QMP_OFF_FEATURES);
	writel(msglen, priv->msgram + QMP_OFF_MSG_LEN);
	memcpy((void *)(priv->msgram + QMP_OFF_MSG_BODY), buf, msglen);

	free(buf);

	/* Kick IPCC to notify AOP — this replaces the hardcoded register write */
	ret = mbox_send(&priv->mbox_chan, NULL);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: IPCC kick failed ret=%d\n", ret);
		return ret;
	}

	/* Poll for AOP reply */
	ret = qmp_wait_for_reply(priv);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: reply timeout after %d ms\n",
			QMP_REPLY_TIMEOUT_MS);
		return ret;
	}

	return 0;
}

int qcom_aoss_qmp_load_state(struct udevice *dev, bool on)
{
	return qcom_aoss_qmp_send(dev,
		"{class: image, res: load_state, name: adsp, val: %s}",
		on ? "on" : "off");
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
	 * aoss_qmp DT node has: mboxes = <&ipcc IPCC_CLIENT_AOP IPCC_MPROC_SIGNAL_GLINK_QMP>;
	 */
	ret = mbox_get_by_index(dev, 0, &priv->mbox_chan);
	if (ret) {
		dev_err(dev, "qcom-aoss-qmp: failed to get IPCC mbox ret=%d\n",
			ret);
		return ret;
	}

	dev_dbg(dev, "qcom-aoss-qmp: IPCC mbox chan id=%08lx\n",
		priv->mbox_chan.id);

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
	.priv_auto	= sizeof(struct qcom_aoss_qmp_priv),
};
