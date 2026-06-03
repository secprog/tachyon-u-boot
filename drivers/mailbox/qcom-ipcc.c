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
#include <linux/bitops.h>

/* IPCC register offsets */
#define IPCC_REG_CONFIG		0x08
#define IPCC_REG_SEND_ID		0x0c
#define IPCC_REG_RECV_SIGNAL_ENABLE	0x14
#define IPCC_REG_RECV_SIGNAL_CLEAR	0x1c

#define IPCC_CLEAR_ON_RECV_RD		BIT(0)

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
	 * IPCC interrupts are not supported in U-Boot (the mailbox framework
	 * lacks an IRQ-driven path).  Callers must poll for replies:
	 * PMIC-GLINK polls SMEM FIFOs directly; QMP polls MSGRAM using
	 * wait_event_timeout() (get_timer()/cpu_relax()).
	 * Linux, by contrast, uses wait_event_timeout() driven by IPCC
	 * hardware interrupts.
	 *
	 * Return -ENODATA so mbox_recv() callers get a clean "no data".
	 */
	return -ENODATA;
}

int qcom_ipcc_prepare_signal(struct udevice *dev, u32 client_id, u32 signal_id)
{
	struct qcom_ipcc_priv *priv = dev_get_priv(dev);
	u32 hwirq = (client_id << 16) | signal_id;
	u32 config;

	if (!priv || !priv->base)
		return -ENODEV;

	/*
	 * Linux clears CLEAR_ON_RECV_RD at probe before setting up the IRQ
	 * domain. Keep the same hardware mode even though U-Boot polls SMEM
	 * instead of dispatching real IPCC IRQs.
	 */
	config = readl(priv->base + IPCC_REG_CONFIG);
	if (config & IPCC_CLEAR_ON_RECV_RD) {
		config &= ~IPCC_CLEAR_ON_RECV_RD;
		writel(config, priv->base + IPCC_REG_CONFIG);
	}

	/*
	 * Linux qcom_ipcc_unmask_irq() writes this hwirq to
	 * RECV_SIGNAL_ENABLE. Clear the same hwirq once first, because U-Boot
	 * has no IRQ handler to drain stale pending LPASS->APSS SMP2P signals.
	 */
	writel(hwirq, priv->base + IPCC_REG_RECV_SIGNAL_CLEAR);
	writel(hwirq, priv->base + IPCC_REG_RECV_SIGNAL_ENABLE);

	dev_dbg(dev, "qcom-ipcc: prepared recv client=%u signal=%u hwirq=%08x\n",
		client_id, signal_id, hwirq);

	return 0;
}

static int qcom_ipcc_probe(struct udevice *dev)
{
	struct qcom_ipcc_priv *priv = dev_get_priv(dev);
	u32 config;

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base) {
		dev_err(dev, "qcom-ipcc: failed to map registers\n");
		return -EINVAL;
	}

	config = readl(priv->base + IPCC_REG_CONFIG);
	if (config & IPCC_CLEAR_ON_RECV_RD) {
		config &= ~IPCC_CLEAR_ON_RECV_RD;
		writel(config, priv->base + IPCC_REG_CONFIG);
	}

	dev_dbg(dev, "qcom-ipcc: base=%p\n", priv->base);

	return 0;
}

static const struct udevice_id qcom_ipcc_ids[] = {
	{ .compatible = "qcom,sc7280-ipcc" },
	{ .compatible = "qcom,ipcc" },
	{ }
};

static const struct mbox_ops qcom_ipcc_mbox_ops = {
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
