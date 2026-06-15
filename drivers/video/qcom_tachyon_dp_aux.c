// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort AUX channel: native + I2C-over-AUX transactions,
 * retry/recovery, DPCD access plumbing, HW HPD read, and EDID block reads.
 * Split out of qcom_tachyon_dp.c; operates on the shared struct tachyon_dp_priv.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <edid.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "qcom_tachyon_dp.h"

static void tachyon_dp_aux_clear_hw_interrupts(struct tachyon_dp_priv *priv);
static void tachyon_dp_aux_log_first_failure(struct tachyon_dp_priv *priv,
					     const u8 *hdr, u32 intr);

static u32 tachyon_dp_aux_get_irq(struct tachyon_dp_priv *priv)
{
	u32 intr, ack;

	intr = readl(priv->ctrl + REG_DP_INTR_STATUS);
	intr &= ~DP_INTERRUPT_STATUS1_MASK;
	ack = (intr & DP_INTERRUPT_STATUS1) << DP_INTERRUPT_STATUS_ACK_SHIFT;
	writel(ack | DP_INTERRUPT_STATUS1_MASK,
	       priv->ctrl + REG_DP_INTR_STATUS);

	return intr;
}

void tachyon_dp_aux_hw_init(struct tachyon_dp_priv *priv)
{
	writel(DP_AUX_CTRL_RESET, priv->aux + REG_DP_AUX_CTRL);
	udelay(1000);
	writel(DP_AUX_CTRL_ENABLE, priv->aux + REG_DP_AUX_CTRL);
	writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
	writel(0xffff, priv->aux + REG_DP_TIMEOUT_COUNT);
	writel(0xffff, priv->aux + REG_DP_AUX_LIMITS);
	tachyon_dp_aux_clear_hw_interrupts(priv);
	udelay(100);
}

static int tachyon_dp_aux_decode_intr(u32 intr)
{
	if (intr & DP_INTR_AUX_ERROR)
		return -EIO;
	if (intr & DP_INTR_WRONG_ADDR)
		return -EREMOTEIO;
	if (intr & DP_INTR_TIMEOUT)
		return -ETIMEDOUT;
	if (intr & DP_INTR_NACK_DEFER)
		return -EAGAIN;
	if (intr & DP_INTR_I2C_NACK)
		return -EREMOTEIO;
	if (intr & DP_INTR_I2C_DEFER)
		return -EAGAIN;
	if (intr & DP_INTR_AUX_XFER_DONE)
		return 0;

	return -EIO;
}

bool tachyon_dp_hw_hpd_connected(struct tachyon_dp_priv *priv)
{
	u32 status;

	status = readl(priv->aux + REG_DP_DP_HPD_INT_STATUS);
	log_debug("DP HPD status=%08x connected=%u\n",
		  status,
		  !!(status & DP_DP_HPD_STATE_STATUS_CONNECTED));

	return !!(status & DP_DP_HPD_STATE_STATUS_CONNECTED);
}

static void tachyon_dp_aux_clear_hw_interrupts(struct tachyon_dp_priv *priv)
{
	readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS);
	writel(0x1f, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
	writel(0x9f, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
	writel(0x00, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
}

/*
 * Dump the AUX command header and the relevant DP/PHY register state once, on
 * the first AUX failure of a session.  Callers gate this on a first_failure
 * check so the full state is captured without spamming every retry.
 */
static void tachyon_dp_aux_log_first_failure(struct tachyon_dp_priv *priv,
					     const u8 *hdr, u32 intr)
{
	log_warning("AUX first-failure: hdr=%02x %02x %02x %02x intr=%08x decoded=%d ctrl=%08x status=%08x trans=%08x phy_intr=%08x dp_intr=%08x\n",
		    hdr[0], hdr[1], hdr[2], hdr[3], intr,
		    tachyon_dp_aux_decode_intr(intr),
		    readl(priv->aux + REG_DP_AUX_CTRL),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL),
		    readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS),
		    readl(priv->ctrl + REG_DP_INTR_STATUS));
}

static void tachyon_dp_aux_reset_linux(struct tachyon_dp_priv *priv)
{
	u32 ctrl = readl(priv->aux + REG_DP_AUX_CTRL);

	log_warning("DP AUX reset: ctrl_before=%08x\n", ctrl);

	ctrl |= DP_AUX_CTRL_RESET;
	writel(ctrl, priv->aux + REG_DP_AUX_CTRL);
	mdelay(1);

	ctrl &= ~DP_AUX_CTRL_RESET;
	writel(ctrl, priv->aux + REG_DP_AUX_CTRL);
	mdelay(1);
	log_warning("DP AUX reset: ctrl_after=%08x\n", ctrl);
}

static int tachyon_dp_aux_xfer(struct tachyon_dp_priv *priv, bool i2c,
			       bool read, bool mot, u32 addr, u8 *buf,
			       size_t len)
{
	u8 hdr[4];
	u32 ctrl, intr, reg, stale_intr, phy_intr;
	int decoded;
	size_t i;

	if (!len || len > 16)
		return -EINVAL;

	hdr[0] = (addr >> 16) & 0xf;
	if (read)
		hdr[0] |= BIT(4);
	hdr[1] = addr >> 8;
	hdr[2] = addr;
	hdr[3] = len - 1;

	writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
	tachyon_dp_aux_clear_hw_interrupts(priv);
	stale_intr = tachyon_dp_aux_get_irq(priv);

	for (i = 0; i < sizeof(hdr); i++) {
		reg = ((u32)hdr[i] << DP_AUX_DATA_OFFSET) &
		      DP_AUX_DATA_MASK;
		if (i == 0)
			reg |= DP_AUX_DATA_INDEX_WRITE;
		writel(reg, priv->aux + REG_DP_AUX_DATA);
	}

	if (!read) {
		for (i = 0; i < len; i++) {
			reg = ((u32)buf[i] << DP_AUX_DATA_OFFSET) &
			      DP_AUX_DATA_MASK;
			writel(reg, priv->aux + REG_DP_AUX_DATA);
		}
	}

	ctrl = DP_AUX_TRANS_CTRL_GO;
	if (i2c)
		ctrl |= DP_AUX_TRANS_CTRL_I2C;
	if (mot)
		ctrl |= DP_AUX_TRANS_CTRL_NO_SEND_STOP;

	/*
	 * Log pre-GO state for diagnostics.  If the controller is not
	 * idle (non-zero TRANS_CTRL) or STATUS already shows errors,
	 * the transaction will likely fail before it begins.
	 */
	log_warning("AUX start: addr=%x i2c=%d read=%d mot=%d len=%zu ctrl=%08x status=%08x trans=%08x stale_intr=%08x phy_intr=%08x\n",
		    addr, i2c, read, mot, len,
		    readl(priv->aux + REG_DP_AUX_CTRL),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL),
		    stale_intr,
		    readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS));

	writel(ctrl, priv->aux + REG_DP_AUX_TRANS_CTRL);

	/*
	 * Confirm the GO bit was accepted.  If it doesn't read back as
	 * set, the controller may be in reset or the clock may be gated.
	 */
	log_warning("AUX go: trans=%08x status=%08x intr_raw=%08x\n",
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->ctrl + REG_DP_INTR_STATUS));

	intr = 0;	phy_intr = 0;
	for (i = 0; i < 250; i++) {
		intr = tachyon_dp_aux_get_irq(priv);
		phy_intr = readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS);
		if (intr & DP_INTERRUPT_STATUS1)
			break;
		udelay(1000);
	}

	decoded = tachyon_dp_aux_decode_intr(intr);

	if (i == 250) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_timeouts++;
		log_warning("AUX timeout: addr=%x i2c=%d read=%d len=%zu intr=%08x phy_intr=%08x decoded=%d ctrl=%08x status=%08x trans=%08x\n",
			    addr, i2c, read, len,
			    intr,
			    phy_intr,
			    decoded,
			    readl(priv->aux + REG_DP_AUX_CTRL),
			    readl(priv->aux + REG_DP_AUX_STATUS),
			    readl(priv->aux + REG_DP_AUX_TRANS_CTRL));
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -ETIMEDOUT;
	}

	log_warning("AUX done: addr=%x i2c=%d read=%d len=%zu intr=%08x phy_intr=%08x status=%08x trans=%08x\n",
		    addr, i2c, read, len,
		    intr,
		    readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL));

	if (intr & DP_INTR_AUX_ERROR) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_errors++;
		tachyon_dp_aux_clear_hw_interrupts(priv);
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EIO;
	}

	if (intr & DP_INTR_WRONG_ADDR) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_nacks++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EREMOTEIO;
	}

	if (intr & DP_INTR_WRONG_DATA_CNT) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_errors++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EIO;
	}

	if (intr & DP_INTR_TIMEOUT) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_timeouts++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -ETIMEDOUT;
	}

	if (intr & (DP_INTR_NACK_DEFER | DP_INTR_I2C_DEFER)) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_defers++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EAGAIN;
	}

	if (intr & DP_INTR_I2C_NACK) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_nacks++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EREMOTEIO;
	}

	if (!(intr & DP_INTR_AUX_XFER_DONE)) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_errors++;
		if (first_failure)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		return -EIO;
	}

	reg = readl(priv->aux + REG_DP_AUX_STATUS);
	if (reg & DP_AUX_STATUS_ERR_MASK) {
		log_debug("DP AUX status error: addr=%x i2c=%d read=%d len=%zu status=%08x\n",
			  addr, i2c, read, len, reg);
		if (reg & DP_AUX_STATUS_TIMEOUT) {
			bool first_failure = !priv->aux_timeouts &&
					     !priv->aux_nacks &&
					     !priv->aux_defers &&
					     !priv->aux_errors;

			priv->aux_timeouts++;
			if (first_failure)
				tachyon_dp_aux_log_first_failure(priv, hdr, intr);
			return -ETIMEDOUT;
		}
		if (reg & DP_AUX_STATUS_DEFER) {
			bool first_failure = !priv->aux_timeouts &&
					     !priv->aux_nacks &&
					     !priv->aux_defers &&
					     !priv->aux_errors;

			priv->aux_defers++;
			if (first_failure)
				tachyon_dp_aux_log_first_failure(priv, hdr, intr);
			return -EAGAIN;
		}
		if (reg & DP_AUX_STATUS_NACK) {
			bool first_failure = !priv->aux_timeouts &&
					     !priv->aux_nacks &&
					     !priv->aux_defers &&
					     !priv->aux_errors;

			priv->aux_nacks++;
			if (first_failure)
				tachyon_dp_aux_log_first_failure(priv, hdr, intr);
			return -EREMOTEIO;
		}
		if (!priv->aux_timeouts && !priv->aux_nacks &&
		    !priv->aux_defers && !priv->aux_errors)
			tachyon_dp_aux_log_first_failure(priv, hdr, intr);
		priv->aux_errors++;
		return -EIO;
	}

	if (read) {
		writel(DP_AUX_DATA_INDEX_WRITE | DP_AUX_DATA_READ,
		       priv->aux + REG_DP_AUX_DATA);
		readl(priv->aux + REG_DP_AUX_DATA);

		for (i = 0; i < len; i++) {
			reg = readl(priv->aux + REG_DP_AUX_DATA);
			buf[i] = (reg & DP_AUX_DATA_MASK) >> DP_AUX_DATA_OFFSET;
		}
	}

	return 0;
}

int tachyon_dp_aux_retry_mot(struct tachyon_dp_priv *priv, bool i2c,
			     bool read, bool mot, u32 addr, u8 *buf,
			     size_t len)
{
	int ret, retry, n_defer = 0;

	for (retry = 0; retry < 8; retry++) {
		ret = tachyon_dp_aux_xfer(priv, i2c, read, mot, addr, buf, len);
		if (!ret)
			return 0;
		priv->aux_retries++;
		if (ret == -EAGAIN) {
			/* DP spec: give up after 7 consecutive DEFERs */
			if (++n_defer >= 7)
				break;
			udelay(8000);
		} else if (ret == -ETIMEDOUT || ret == -EIO ||
			   ret == -EREMOTEIO) {
			n_defer = 0;
			tachyon_dp_aux_reset_linux(priv);
			tachyon_dp_aux_hw_init(priv);
			udelay(4000);
		} else {
			n_defer = 0;
			udelay(4000);
		}
	}

	return ret;
}

int tachyon_dp_aux_retry(struct tachyon_dp_priv *priv, bool i2c,
			 bool read, u32 addr, u8 *buf, size_t len)
{
	return tachyon_dp_aux_retry_mot(priv, i2c, read, false, addr, buf, len);
}

int tachyon_dp_edid_read_block(struct tachyon_dp_priv *priv, u8 block,
			       u8 *buf)
{
	u8 segment = block / 2;
	u8 offset = (block & 1) ? EDID_SIZE : 0;
	size_t done = 0;
	int ret;

	if (segment) {
		/* MOT=1: keep bus open for the address write that follows */
		ret = tachyon_dp_aux_retry_mot(priv, true, false, true,
					       TACHYON_DP_DDC_SEGMENT_ADDR,
					       &segment, 1);
		if (ret)
			return ret;
	}

	/* MOT=1: keep bus open; the read burst follows */
	ret = tachyon_dp_aux_retry_mot(priv, true, false, true,
				       TACHYON_DP_DDC_ADDR, &offset, 1);
	if (ret)
		return ret;

	while (done < EDID_SIZE) {
		size_t len = min_t(size_t, 16, EDID_SIZE - done);
		/* MOT=0 only on the final chunk to release the bus */
		bool last = (done + len >= EDID_SIZE);

		ret = tachyon_dp_aux_retry_mot(priv, true, true, !last,
					       TACHYON_DP_DDC_ADDR,
					       buf + done, len);
		if (ret)
			return ret;
		done += len;
	}

	return 0;
}
