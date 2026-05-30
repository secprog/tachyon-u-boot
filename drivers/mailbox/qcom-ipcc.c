// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm IPCC (Inter-Processor Communication Controller) mailbox driver
 *
 * The IPCC is a simple doorbell mechanism: writing (client_id << 16 | signal_id)
 * to the SEND_ID register triggers an interrupt on the remote processor.
 * There is no data payload — the signal itself is the message.
 *
 * Linux reference: drivers/mailbox/qcom-ipcc.c
 * DT binding: compatible = "qcom,sc7280-ipcc", "qcom,ipcc"
 *             #mbox-cells = <2>  →  <client_id, signal_id>
 */

#include <dm.h>
#include <dm/device_compat.h>
#include <mailbox-uclass.h>
#include <asm/io.h>

/* IPCC register offsets */
#define IPCC_REG_SEND_ID		0x0c

struct qcom_ipcc_priv {
	void __iomem *base;
};

/*
 * Pack client_id and signal_id into chan->id: (client << 16) | signal.
 * This matches the hardware encoding written to IPCC_REG_SEND_ID.
 */
static int qcom_ipcc_of_xlate(struct mbox_chan *chan,
			      struct ofnode_phandle_args *args)
{
	if (args->args_count != 2) {
		dev_err(chan->dev, "qcom-ipcc: expected #mbox-cells=<2> got %d\n",
			args->args_count);
		return -EINVAL;
	}

	chan->id = ((u32)args->args[0] << 16) | (u32)args->args[1];

	return 0;
}

static int qcom_ipcc_request(struct mbox_chan *chan)
{
	/* No per-channel setup needed — IPCC channels are stateless */
	return 0;
}

static int qcom_ipcc_free(struct mbox_chan *chan)
{
	return 0;
}

static int qcom_ipcc_send(struct mbox_chan *chan, const void *data)
{
	struct qcom_ipcc_priv *priv = dev_get_priv(chan->dev);

	dev_dbg(chan->dev, "qcom-ipcc: send id=%08lx (client=%lu signal=%lu)\n",
		chan->id, chan->id >> 16, chan->id & 0xffff);

	writel(chan->id, priv->base + IPCC_REG_SEND_ID);

	return 0;
}

static int qcom_ipcc_recv(struct mbox_chan *chan, void *data)
{
	/*
	 * IPCC interrupts/receives are not needed in U-Boot.
	 * PMIC-GLINK polls SMEM FIFOs. QMP polls MSGRAM for reply magic.
	 * Return -ENODATA so mbox_recv() callers get a clean "no data".
	 */
	return -ENODATA;
}

static int qcom_ipcc_probe(struct udevice *dev)
{
	struct qcom_ipcc_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base) {
		dev_err(dev, "qcom-ipcc: failed to map registers\n");
		return -EINVAL;
	}

	dev_dbg(dev, "qcom-ipcc: base=%p\n", priv->base);

	return 0;
}

static const struct udevice_id qcom_ipcc_ids[] = {
	{ .compatible = "qcom,sc7280-ipcc" },
	{ .compatible = "qcom,ipcc" },
	{ }
};

struct mbox_ops qcom_ipcc_mbox_ops = {
	.of_xlate	= qcom_ipcc_of_xlate,
	.request	= qcom_ipcc_request,
	.rfree		= qcom_ipcc_free,
	.send		= qcom_ipcc_send,
	.recv		= qcom_ipcc_recv,
};

U_BOOT_DRIVER(qcom_ipcc) = {
	.name		= "qcom_ipcc",
	.id		= UCLASS_MAILBOX,
	.of_match	= qcom_ipcc_ids,
	.probe		= qcom_ipcc_probe,
	.priv_auto	= sizeof(struct qcom_ipcc_priv),
	.ops		= &qcom_ipcc_mbox_ops,
};
