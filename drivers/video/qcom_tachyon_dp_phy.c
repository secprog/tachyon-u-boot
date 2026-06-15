// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort QMP combo-PHY programming (DP mode): SerDes,
 * TX, PLL and DP-PHY register sequences.  Split out of qcom_tachyon_dp.c;
 * operates on the shared struct tachyon_dp_priv passed by the dp_display parent.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <clk.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "qcom_tachyon_dp.h"

/*
 * Helper to read the low byte of a QMP COM register.
 * QMP COM registers are byte-style — only bits [7:0] are meaningful.
 */
u8 tachyon_dp_qmp_com_readb(struct tachyon_dp_priv *priv, u32 reg)
{
	return readl(priv->qmp_com + reg) & 0xff;
}

/*
 * Slim COM update: ONLY writes TYPEC_CTRL and PHY_MODE_CTRL.
 * The phy-qcom-qmp-combo provider owns POWER_DOWN_CTRL, RESET_OVRD_CTRL,
 * SW_RESET, and SWI_CTRL via generic_phy_init().
 *
 * Do NOT toggle COM power/reset registers from here — repeated COM reset
 * from the DP driver can clobber the provider's state and cause C_READY=0.
 */
/*
 * QMP combo PHY_MODE_CTRL is a bitfield: bit0=USB3, bit1=DP (matches Linux
 * phy-qcom-qmp-combo).  A pin-D/F sink is 2-lane DP + USB3 and needs USB3+DP
 * mode (0x03) so the combo routes the 2 DP lanes onto the correct physical
 * pins; a pin-C/E sink is 4-lane DP-only (0x02).  Driving a 2-lane sink in
 * DP-only mode lands DP on the wrong lanes -> sink never sees the main link
 * (clock-recovery fails, lane status 00) even though AUX (on the SBU mux) works.
 */
u8 tachyon_dp_qmp_phy_mode(struct tachyon_dp_priv *priv)
{
	u8 dp_lanes = tachyon_dp_pin_assignment_lanes(priv);

	/*
	 * Without a valid Type-C pin assignment (e.g. an early call before the
	 * forced/diagnostic path has chosen one) the pin lane count is 0; fall
	 * back to the link's resolved lane count so the combo split always
	 * tracks the number of DP lanes we actually drive.
	 */
	if (!dp_lanes)
		dp_lanes = priv->lanes ? priv->lanes : priv->max_lanes;

	if (dp_lanes >= 4)
		return QMP_DP_COM_DP_MODE;
	return QMP_DP_COM_DP_MODE | QMP_DP_COM_USB3_MODE;
}

void tachyon_dp_qmp_com_orientation_update(struct tachyon_dp_priv *priv)
{
	u32 typec;

	typec = QMP_DP_COM_SW_PORTSELECT_MUX;
	if (priv->orientation == TACHYON_DP_ORIENTATION_REVERSE)
		typec |= QMP_DP_COM_SW_PORTSELECT_VAL;

	writel(typec, priv->qmp_com + QMP_V3_DP_COM_TYPEC_CTRL);
	writel(tachyon_dp_qmp_phy_mode(priv),
	       priv->qmp_com + QMP_V3_DP_COM_PHY_MODE_CTRL);

	log_debug("QMP COM orientation update: TYPEC=%02x MODE=%02x\n",
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL));
}

static void tachyon_dp_qmp_aux_only_power_on(struct tachyon_dp_priv *priv)
{
	writel(QMP_DP_PHY_PD_CTL_POWER_DOWN, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	writel(QMP_DP_PHY_PD_CTL_AUX_ON, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	priv->qmp_dp_touched = true;
}

/*
 * Keep AUX powered without downgrading a fully-ready DP PHY. Most PD_CTL bits
 * are active-low enables, so a set *_B bit means that block is powered/enabled.
 *
 * IMPORTANT: Like other QMP byte-style registers, use direct writel()
 * with a fully-computed low-byte value. Do NOT use clrbits_le32() here;
 * it reads back byte-replicated garbage and the RMW may not work.
 */
void tachyon_dp_qmp_force_aux_on(struct tachyon_dp_priv *priv)
{
	u32 before, after;
	u8 status;

	before = readl(priv->phy_dp + QMP_DP_PHY_PD_CTL);
	status = readl(priv->phy_dp + QMP_V4_DP_PHY_STATUS) & 0xff;

	if (status & QMP_DP_PHY_STATUS_PHY_READY) {
		if ((before & 0xff) != QMP_DP_PHY_PD_CTL_4LANE_ON) {
			writel(QMP_DP_PHY_PD_CTL_4LANE_ON,
			       priv->phy_dp + QMP_DP_PHY_PD_CTL);
			udelay(100);
		}
	} else {
		tachyon_dp_qmp_aux_only_power_on(priv);
	}

	after = readl(priv->phy_dp + QMP_DP_PHY_PD_CTL);

	log_debug("QMP AUX force-on: PD_CTL before=%08x after=%08x low=%02x STATUS=%02x AUX_PWRDN_B=%u CLAMP_EN_B=%u\n",
		    before,
		    after,
		    after & 0xff,
		    status,
		    !!((after & 0xff) & QMP_DP_PHY_PD_CTL_AUX_PWRDN_B),
		    !!((after & 0xff) & QMP_DP_PHY_PD_CTL_DP_CLAMP_EN_B));
}

static u8 tachyon_dp_qmp_pd_low(struct tachyon_dp_priv *priv)
{
	return readl(priv->phy_dp + QMP_DP_PHY_PD_CTL) & 0xff;
}

u8 tachyon_dp_qmp_status_low(struct tachyon_dp_priv *priv)
{
	return readl(priv->phy_dp + QMP_V4_DP_PHY_STATUS) & 0xff;
}

void tachyon_dp_qmp_power_down(struct tachyon_dp_priv *priv)
{
	writel(QMP_DP_PHY_PD_CTL_POWER_DOWN, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	priv->qmp_dp_touched = false;
	priv->qmp_dp_serdes_programmed = false;
	priv->qmp_dp_phy_started = false;

	log_debug("QMP DP power-down: PD=%02x STATUS=%02x\n",
		    tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));
}

static void tachyon_dp_qmp_power_up_all_lanes(struct tachyon_dp_priv *priv)
{
	bool reverse = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE;
	/*
	 * PD_CTL out of powerdown/clamp, powering up ONLY the DP lanes actually
	 * in use.  A 2-lane (pin D/F) link must leave the other pair powered down
	 * so DP lands on the correct physical lanes; the old unconditional 0x7d
	 * (4-lane) powered all four even for a 2-lane combo (MODE=0x03), which can
	 * mis-route the link.  Matches Linux qmp_combo_configure_dp_mode():
	 *   base = PWRDN_B|AUX_PWRDN_B|PLL_PWRDN_B|DP_CLAMP_EN_B (0x65)
	 *   + LANE_0_1_PWRDN_B if 4-lane OR reversed
	 *   + LANE_2_3_PWRDN_B if 4-lane OR normal
	 * => 4-lane=0x7d, 2-lane normal=0x75, 2-lane reverse=0x6d.
	 * (PD_CTL bits are active-low enables: a set *_B bit powers that block.)
	 */
	u8 pd = QMP_DP_PHY_PD_CTL_PWRDN_B | QMP_DP_PHY_PD_CTL_AUX_PWRDN_B |
		QMP_DP_PHY_PD_CTL_PLL_PWRDN_B | QMP_DP_PHY_PD_CTL_DP_CLAMP_EN_B;

	if (priv->lanes >= 4 || reverse)
		pd |= QMP_DP_PHY_PD_CTL_LANE_0_1_PWRDN_B;
	if (priv->lanes >= 4 || !reverse)
		pd |= QMP_DP_PHY_PD_CTL_LANE_2_3_PWRDN_B;

	writel(pd, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	priv->qmp_dp_touched = true;

	log_debug("QMP DP power-up lanes=%u reverse=%d: PD_CTL=%02x STATUS=%02x\n",
		    priv->lanes, reverse, tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));
}

static void tachyon_dp_qmp_dump_pll_state(struct tachyon_dp_priv *priv,
					  const char *tag)
{
	void __iomem *serdes = priv->qmp_dp_serdes;
	static const struct tachyon_qmp_named_reg regs[] = {
		{ "SW_RESET", QMP_V4_COM_SW_RESET },
		{ "RESETSM_CNTRL", QMP_V4_COM_RESETSM_CNTRL },
		{ "C_READY_STATUS", QMP_V4_COM_C_READY_STATUS },
		{ "CMN_STATUS", QMP_V4_COM_CMN_STATUS },
		{ "CMN_CONFIG", QMP_V4_COM_CMN_CONFIG },
		{ "CMN_MODE", QMP_V4_COM_CMN_MODE },
		{ "DEC_START_MODE0", QMP_V4_COM_DEC_START_MODE0 },
		{ "DIV_FRAC_START1_MODE0", QMP_V4_COM_DIV_FRAC_START1_MODE0 },
		{ "DIV_FRAC_START2_MODE0", QMP_V4_COM_DIV_FRAC_START2_MODE0 },
		{ "DIV_FRAC_START3_MODE0", QMP_V4_COM_DIV_FRAC_START3_MODE0 },
		{ "CP_CTRL_MODE0", QMP_V4_COM_CP_CTRL_MODE0 },
		{ "PLL_RCTRL_MODE0", QMP_V4_COM_PLL_RCTRL_MODE0 },
		{ "PLL_CCTRL_MODE0", QMP_V4_COM_PLL_CCTRL_MODE0 },
		{ "LOCK_CMP1_MODE0", QMP_V4_COM_LOCK_CMP1_MODE0 },
		{ "LOCK_CMP2_MODE0", QMP_V4_COM_LOCK_CMP2_MODE0 },
		{ "LOCK_CMP_EN", QMP_V4_COM_LOCK_CMP_EN },
		{ "VCO_TUNE_MAP", QMP_V4_COM_VCO_TUNE_MAP },
	};
	int i;

	log_debug("QMP DP PLL %s: PD=%02x DP_STATUS=%02x\n",
		    tag, tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));

	for (i = 0; i < ARRAY_SIZE(regs); i++)
		log_debug("QMP DP PLL %s %-22s +%03x=%02x\n",
			    tag, regs[i].name, regs[i].off,
			    readl(serdes + regs[i].off) & 0xff);
}

static void tachyon_dp_qmp_dump_lane_power_state(struct tachyon_dp_priv *priv,
						 const char *tag)
{
	void __iomem *tx0 = priv->qmp_dp_tx0;
	void __iomem *tx1 = priv->qmp_dp_tx1;

	log_debug("QMP DP PHY %s: PD_CTL=%02x DP_PHY_CFG=%02x DP_PHY_CFG1=%02x DP_STATUS=%02x\n",
		    tag,
		    tachyon_dp_qmp_pd_low(priv),
		    readl(priv->phy_dp + QMP_DP_PHY_CFG) & 0xff,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_CFG_1) & 0xff,
		    tachyon_dp_qmp_status_low(priv));
	log_debug("QMP DP PHY %s TX0: HIGHZ_DRVR_EN=%02x TRANSCEIVER_BIAS_EN=%02x RESET_TSYNC_EN=%02x\n",
		    tag,
		    readl(tx0 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_RESET_TSYNC_EN) & 0xff);
	log_debug("QMP DP PHY %s TX1: HIGHZ_DRVR_EN=%02x TRANSCEIVER_BIAS_EN=%02x RESET_TSYNC_EN=%02x\n",
		    tag,
		    readl(tx1 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_RESET_TSYNC_EN) & 0xff);
}

static int tachyon_dp_qmp_poll(struct tachyon_dp_priv *priv,
			       void __iomem *base, u32 reg,
			       u32 mask, u32 value,
			       const char *name)
{
	int ret;

	ret = tachyon_dp_read_poll(base, reg, mask, value, 10000);

	log_debug("QMP poll %-18s ret=%d val=%02x mask=%02x want=%02x PD=%02x DP_STATUS=%02x\n",
		    name,
		    ret,
		    readl(base + reg) & 0xff,
		    mask & 0xff,
		    value & 0xff,
		    tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));

	return ret;
}

static void tachyon_qmp_write_table(void __iomem *base,
				    const struct tachyon_qmp_reg *regs,
				    int count)
{
	int i;

	for (i = 0; i < count; i++)
		writel(regs[i].val, base + regs[i].off);
}

static const struct tachyon_qmp_reg qmp_v4_dp_serdes_tbl[] = {
	{ 0x184, 0x05 }, { 0x094, 0x3b }, { 0x04c, 0x02 },
	{ 0x048, 0x0c }, { 0x050, 0x06 }, { 0x154, 0x30 },
	{ 0x058, 0x0f }, { 0x084, 0x36 }, { 0x07c, 0x16 },
	{ 0x074, 0x06 }, { 0x17c, 0x02 }, { 0x0ec, 0x3f },
	{ 0x0f0, 0x00 }, { 0x10c, 0x00 }, { 0x0cc, 0x00 },
	{ 0x00c, 0x0a }, { 0x168, 0x0a }, { 0x108, 0x00 },
	{ 0x044, 0x17 }, { 0x174, 0x1f },
};

static const struct tachyon_qmp_reg qmp_v4_dp_serdes_rbr_tbl[] = {
	{ 0x158, 0x05 }, { 0x0bc, 0x69 }, { 0x0d0, 0x80 },
	{ 0x0d4, 0x07 }, { 0x0ac, 0x6f }, { 0x0b0, 0x08 },
	{ 0x0a4, 0x04 }, { 0x1bc, 0x22 },
};

static const struct tachyon_qmp_reg qmp_v4_dp_serdes_hbr_tbl[] = {
	{ 0x158, 0x03 }, { 0x0bc, 0x69 }, { 0x0d0, 0x80 },
	{ 0x0d4, 0x07 }, { 0x0ac, 0x0f }, { 0x0b0, 0x0e },
	{ 0x0a4, 0x08 }, { 0x1bc, 0x22 },
};

static const struct tachyon_qmp_reg qmp_v4_dp_serdes_hbr2_tbl[] = {
	{ 0x158, 0x01 }, { 0x0bc, 0x8c }, { 0x0d0, 0x00 },
	{ 0x0d4, 0x0a }, { 0x0ac, 0x1f }, { 0x0b0, 0x1c },
	{ 0x0a4, 0x08 }, { 0x1bc, 0x11 },
};

static const struct tachyon_qmp_reg qmp_v4_dp_serdes_hbr3_tbl[] = {
	{ 0x158, 0x00 }, { 0x0bc, 0x69 }, { 0x0d0, 0x80 },
	{ 0x0d4, 0x07 }, { 0x0ac, 0x2f }, { 0x0b0, 0x2a },
	{ 0x0a4, 0x08 }, { 0x1bc, 0x00 },
};

static const struct tachyon_qmp_reg qmp_v4_dp_tx_tbl[] = {
	{ 0x0e8, 0x40 }, { 0x020, 0x30 }, { 0x02c, 0x3b },
	{ 0x008, 0x0f }, { 0x01c, 0x03 }, { 0x0b8, 0x0f },
	{ 0x060, 0x00 }, { 0x0bc, 0x00 }, { 0x03c, 0x11 },
	{ 0x040, 0x11 }, { 0x024, 0x04 }, { 0x05c, 0x0a },
	{ 0x014, 0x2a }, { 0x00c, 0x20 },
};

static void tachyon_dp_qmp_rate_serdes_table(struct tachyon_dp_priv *priv,
					     const struct tachyon_qmp_reg **regs,
					     int *count)
{
	const char *name;

	switch (priv->rate) {
	case DP_LINK_RATE_RBR:
		*regs = qmp_v4_dp_serdes_rbr_tbl;
		*count = ARRAY_SIZE(qmp_v4_dp_serdes_rbr_tbl);
		name = "RBR";
		break;
	case DP_LINK_RATE_HBR:
		*regs = qmp_v4_dp_serdes_hbr_tbl;
		*count = ARRAY_SIZE(qmp_v4_dp_serdes_hbr_tbl);
		name = "HBR";
		break;
	case DP_LINK_RATE_HBR3:
		*regs = qmp_v4_dp_serdes_hbr3_tbl;
		*count = ARRAY_SIZE(qmp_v4_dp_serdes_hbr3_tbl);
		name = "HBR3";
		break;
	case DP_LINK_RATE_HBR2:
	default:
		*regs = qmp_v4_dp_serdes_hbr2_tbl;
		*count = ARRAY_SIZE(qmp_v4_dp_serdes_hbr2_tbl);
		name = "HBR2";
		break;
	}

	log_debug("QMP DP SerDes rate table: rate=%u table=%s count=%d\n",
		    priv->rate, name, *count);
}

static int tachyon_dp_qmp_program_serdes(struct tachyon_dp_priv *priv)
{
	void __iomem *serdes = priv->qmp_dp_serdes;
	const struct tachyon_qmp_reg *rate_tbl;
	int rate_tbl_count;

	/*
	 * Linux qmp_combo_dp_power_on() initializes DP SerDes before
	 * programming DP TX and DP PHY registers. Keep the same ordering
	 * here so C_READY is not polled against an unprogrammed PLL.
	 *
	 * Phase 2: Explicitly guard CMN_MODE — Linux defines
	 * QSERDES_V4_COM_CMN_MODE but does NOT write it in the
	 * V4 DP SerDes table for this PHY.
	 */
#if TACHYON_DP_WRITE_CMN_MODE
	writel(0x04, serdes + QMP_V4_COM_CMN_MODE);
	udelay(10);
#endif

	writel(1, serdes + QMP_V4_COM_SW_RESET);
	udelay(10);
	tachyon_dp_qmp_dump_pll_state(priv, "after SW_RESET=1");

	tachyon_qmp_write_table(serdes, qmp_v4_dp_serdes_tbl,
				ARRAY_SIZE(qmp_v4_dp_serdes_tbl));

	tachyon_dp_qmp_rate_serdes_table(priv, &rate_tbl, &rate_tbl_count);

	tachyon_qmp_write_table(serdes, rate_tbl, rate_tbl_count);
	tachyon_dp_qmp_dump_pll_state(priv, "after SerDes table");

	return 0;
}

static void tachyon_dp_qmp_program_tx_table(struct tachyon_dp_priv *priv)
{
	tachyon_qmp_write_table(priv->qmp_dp_tx0, qmp_v4_dp_tx_tbl,
				ARRAY_SIZE(qmp_v4_dp_tx_tbl));
	tachyon_qmp_write_table(priv->qmp_dp_tx1, qmp_v4_dp_tx_tbl,
				ARRAY_SIZE(qmp_v4_dp_tx_tbl));
}

static void tachyon_dp_qmp_deassert_serdes_reset(struct tachyon_dp_priv *priv)
{
	void __iomem *serdes = priv->qmp_dp_serdes;

	/* De-assert SW reset; all init values are now latched */
	writel(0, serdes + QMP_V4_COM_SW_RESET);
	udelay(10);
	priv->qmp_dp_serdes_programmed = true;
	tachyon_dp_qmp_dump_pll_state(priv, "after SW_RESET=0");
}

static void tachyon_dp_qmp_dump_serdes_table(struct tachyon_dp_priv *priv,
					     const char *name,
					     const struct tachyon_qmp_reg *regs,
					     int count)
{
	void __iomem *serdes = priv->qmp_dp_serdes;
	u8 actual;
	int i;

	for (i = 0; i < count; i++) {
		actual = readl(serdes + regs[i].off) & 0xff;
		log_debug("QMP DP SerDes pre-C_READY %-4s[%02d] +%03x actual=%02x expected=%02x %s\n",
			    name, i, regs[i].off, actual, regs[i].val,
			    actual == regs[i].val ? "ok" : "MISMATCH");
	}
}

static void tachyon_dp_qmp_dump_serdes_pre_ready(struct tachyon_dp_priv *priv)
{
	void __iomem *serdes = priv->qmp_dp_serdes;
	const struct tachyon_qmp_reg *rate_tbl;
	int rate_tbl_count;

	tachyon_dp_qmp_rate_serdes_table(priv, &rate_tbl, &rate_tbl_count);

	log_debug("QMP DP SerDes pre-C_READY: programmed=%u rate=%u lanes=%u SW_RESET=%02x RESETSM=%02x C_READY=%02x CMN=%02x PD=%02x DP_STATUS=%02x\n",
		    priv->qmp_dp_serdes_programmed,
		    priv->rate,
		    priv->lanes,
		    readl(serdes + QMP_V4_COM_SW_RESET) & 0xff,
		    readl(serdes + QMP_V4_COM_RESETSM_CNTRL) & 0xff,
		    readl(serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
		    readl(serdes + QMP_V4_COM_CMN_STATUS) & 0xff,
		    tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));

	tachyon_dp_qmp_dump_serdes_table(priv, "base",
					 qmp_v4_dp_serdes_tbl,
					 ARRAY_SIZE(qmp_v4_dp_serdes_tbl));
	tachyon_dp_qmp_dump_serdes_table(priv, "rate",
					 rate_tbl, rate_tbl_count);
}

static const u8 qmp_dp_v3_pre_hbr3_hbr2[4][4] = {
	{ 0x00, 0x0c, 0x15, 0x1a },
	{ 0x02, 0x0e, 0x16, 0xff },
	{ 0x02, 0x11, 0xff, 0xff },
	{ 0x04, 0xff, 0xff, 0xff },
};

static const u8 qmp_dp_v3_swing_hbr3_hbr2[4][4] = {
	{ 0x02, 0x12, 0x16, 0x1a },
	{ 0x09, 0x19, 0x1f, 0xff },
	{ 0x10, 0x1f, 0xff, 0xff },
	{ 0x1f, 0xff, 0xff, 0xff },
};

static const u8 qmp_dp_v3_pre_hbr_rbr[4][4] = {
	{ 0x00, 0x0c, 0x14, 0x19 },
	{ 0x00, 0x0b, 0x12, 0xff },
	{ 0x00, 0x0b, 0xff, 0xff },
	{ 0x04, 0xff, 0xff, 0xff },
};

static const u8 qmp_dp_v3_swing_hbr_rbr[4][4] = {
	{ 0x08, 0x0f, 0x16, 0x1f },
	{ 0x11, 0x1e, 0x1f, 0xff },
	{ 0x19, 0x1f, 0xff, 0xff },
	{ 0x1f, 0xff, 0xff, 0xff },
};

int tachyon_dp_qmp_program_tx(struct tachyon_dp_priv *priv)
{
	void __iomem *tx0 = priv->qmp_dp_tx0;
	void __iomem *tx1 = priv->qmp_dp_tx1;
	const u8 (*swing_tbl)[4];
	const u8 (*pre_tbl)[4];
	u8 swing = 0, pre = 0;
	u8 swing_cfg, pre_cfg;
	int i;

	for (i = 0; i < priv->lanes; i++) {
		swing = max(swing, priv->swing[i]);
		pre = max(pre, priv->pre[i]);
	}
	swing = min_t(u8, swing, 3);
	pre = min_t(u8, pre, 3);

	if (priv->rate <= DP_LINK_RATE_HBR) {
		swing_tbl = qmp_dp_v3_swing_hbr_rbr;
		pre_tbl = qmp_dp_v3_pre_hbr_rbr;
	} else {
		swing_tbl = qmp_dp_v3_swing_hbr3_hbr2;
		pre_tbl = qmp_dp_v3_pre_hbr3_hbr2;
	}

	while ((swing_tbl[swing][pre] == 0xff ||
		pre_tbl[swing][pre] == 0xff) && pre)
		pre--;
	while ((swing_tbl[swing][pre] == 0xff ||
		pre_tbl[swing][pre] == 0xff) && swing)
		swing--;

	if (swing_tbl[swing][pre] == 0xff || pre_tbl[swing][pre] == 0xff)
		return -EINVAL;

	swing_cfg = swing_tbl[swing][pre] | QMP_DP_TX_DRV_LVL_MUX_EN;
	pre_cfg = pre_tbl[swing][pre] | QMP_DP_TX_EMP_POST1_LVL_MUX_EN;

	writel(swing_cfg, tx0 + QMP_V3_TX_TX_DRV_LVL);
	writel(pre_cfg, tx0 + QMP_V3_TX_TX_EMP_POST1_LVL);
	writel(swing_cfg, tx1 + QMP_V3_TX_TX_DRV_LVL);
	writel(pre_cfg, tx1 + QMP_V3_TX_TX_EMP_POST1_LVL);

	log_debug("DP TX train cfg: rate=%u lanes=%u swing=%u pre=%u drv=%02x emp=%02x "
		    "tx0_drv=%02x tx0_emp=%02x tx0_highz=%02x tx0_bias=%02x tx0_pol=%02x "
		    "tx1_drv=%02x tx1_emp=%02x tx1_highz=%02x tx1_bias=%02x tx1_pol=%02x\n",
		    priv->rate, priv->lanes, swing, pre, swing_cfg, pre_cfg,
		    readl(tx0 + QMP_V3_TX_TX_DRV_LVL) & 0xff,
		    readl(tx0 + QMP_V3_TX_TX_EMP_POST1_LVL) & 0xff,
		    readl(tx0 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_TX_POL_INV) & 0xff,
		    readl(tx1 + QMP_V3_TX_TX_DRV_LVL) & 0xff,
		    readl(tx1 + QMP_V3_TX_TX_EMP_POST1_LVL) & 0xff,
		    readl(tx1 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_TX_POL_INV) & 0xff);

	/*
	 * TX bias and polarity are handled separately by
	 * tachyon_dp_qmp_v4_program_tx_bias() and
	 * tachyon_dp_qmp_program_tx_levels() in the Linux-aligned
	 * V4 DP PHY configure path.
	 */

	return 0;
}

/*
 * Phase 5: Linux-like V4 lane/orientation TX bias programming.
 * Mirrors the per-lane HIGHZ_DRVR_EN and TRANSCEIVER_BIAS_EN
 * logic from Linux qmp_v4_configure_dp_phy().
 */
static void tachyon_dp_qmp_v4_program_tx_bias(struct tachyon_dp_priv *priv)
{
	void __iomem *tx0 = priv->qmp_dp_tx0;
	void __iomem *tx1 = priv->qmp_dp_tx1;
	bool reverse = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE;
	u32 bias0_en, bias1_en;
	u32 drvr0_en, drvr1_en;

	if (priv->lanes == 1) {
		bias0_en = reverse ? 0x3e : 0x15;
		bias1_en = reverse ? 0x15 : 0x3e;
		drvr0_en = reverse ? 0x13 : 0x10;
		drvr1_en = reverse ? 0x10 : 0x13;
	} else if (priv->lanes == 2) {
		bias0_en = reverse ? 0x3f : 0x15;
		bias1_en = reverse ? 0x15 : 0x3f;
		drvr0_en = 0x10;
		drvr1_en = 0x10;
	} else {
		bias0_en = 0x3f;
		bias1_en = 0x3f;
		drvr0_en = 0x10;
		drvr1_en = 0x10;
	}

	writel(drvr0_en, tx0 + QMP_V3_TX_HIGHZ_DRVR_EN);
	writel(bias0_en, tx0 + QMP_V3_TX_TRANSCEIVER_BIAS_EN);
	writel(drvr1_en, tx1 + QMP_V3_TX_HIGHZ_DRVR_EN);
	writel(bias1_en, tx1 + QMP_V3_TX_TRANSCEIVER_BIAS_EN);

	log_debug("QMP DP V4 TX bias: lanes=%u reverse=%u bias0=%02x bias1=%02x drvr0=%02x drvr1=%02x\n",
		    priv->lanes, reverse ? 1 : 0,
		    bias0_en, bias1_en, drvr0_en, drvr1_en);
}

/*
 * Phase 7: Linux-like TX polarity / drive / pre-emphasis defaults.
 * These match Linux qmp_v4_configure_dp_phy() TX defaults.
 * Link-training voltage/pre-emphasis updates via program_tx() happen
 * separately and can override these defaults.
 */
static void tachyon_dp_qmp_program_tx_levels(struct tachyon_dp_priv *priv)
{
	void __iomem *tx0 = priv->qmp_dp_tx0;
	void __iomem *tx1 = priv->qmp_dp_tx1;

	writel(0x0a, tx0 + QMP_V3_TX_TX_POL_INV);
	writel(0x0a, tx1 + QMP_V3_TX_TX_POL_INV);

	writel(0x27, tx0 + QMP_V3_TX_TX_DRV_LVL);
	writel(0x27, tx1 + QMP_V3_TX_TX_DRV_LVL);

	writel(0x20, tx0 + QMP_V3_TX_TX_EMP_POST1_LVL);
	writel(0x20, tx1 + QMP_V3_TX_TX_EMP_POST1_LVL);

	log_debug("QMP DP V4 TX levels: pol=0a drv=27 emp=20\n");
}

/*
 * Phase 9: Configure DP link clock for the current rate.
 * Returns 0 even if the clock framework is a no-op; log clearly.
 */
static int tachyon_dp_qmp_configure_dp_clocks(struct tachyon_dp_priv *priv)
{
	long clk_ret;

	if (priv->dp_clk_valid[2]) {
		clk_ret = clk_set_rate(&priv->dp_clks[2], priv->rate * 1000);
		log_debug("QMP DP clock cfg: rate=%u link_clk=%ld ret=%ld\n",
			    priv->rate, (long)priv->rate * 1000, clk_ret);
		if (clk_ret < 0) {
			log_warning("Failed to set DP link clock %u kHz: %ld\n",
				    priv->rate, clk_ret);
			/* Continue — clock may have been set previously */
		}
	} else {
		log_debug("QMP DP clock cfg: no ctrl_link clock, rate=%u\n",
			    priv->rate);
	}

	return 0;
}

/*
 * Phase 4: Linux-like qmp_v456_configure_dp_phy().
 * Implements the full V456 DP PHY start sequence including polls.
 * This replaces the old program_dp_phy_regs() + start_dp_phy() +
 * inline poll sequence.
 */
static int tachyon_dp_qmp_v456_configure_dp_phy(struct tachyon_dp_priv *priv)
{
	void __iomem *serdes = priv->qmp_dp_serdes;
	u32 mode;
	u32 vco_div;
	u32 tx01, tx23;
	int ret;

	log_debug("QMP DP V456 start\n");

	/* DP_PHY_CFG_1 = 0x0f */
	writel(0x0f, priv->phy_dp + QMP_V4_DP_PHY_CFG_1);

	/* Configure DP mode (orientation-dependent) */
	mode = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE ?
	       0x4c : 0x5c;
	writel(mode, priv->phy_dp + QMP_DP_PHY_MODE);

	/* AUX configuration */
	writel(0x13, priv->phy_dp + QMP_DP_PHY_AUX_CFG1);
	writel(0xa4, priv->phy_dp + QMP_DP_PHY_AUX_CFG2);

	/* Lane control — Linux-aligned values */
#if TACHYON_DP_LINUX_LANE_CTL
	tx01 = 0x05;
	tx23 = 0x05;
#else
	tx01 = priv->lanes > 0 ? 0x01 : 0x00;
	tx23 = priv->lanes > 2 ? 0x05 : 0x00;
#endif
	writel(tx01, priv->phy_dp + QMP_V4_DP_PHY_TX0_TX1_LANE_CTL);
	writel(tx23, priv->phy_dp + QMP_V4_DP_PHY_TX2_TX3_LANE_CTL);
	log_debug("QMP DP lane cfg: lanes=%u TX0_TX1=%02x TX2_TX3=%02x linux=%d\n",
		    priv->lanes,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_TX0_TX1_LANE_CTL) & 0xff,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_TX2_TX3_LANE_CTL) & 0xff,
		    TACHYON_DP_LINUX_LANE_CTL);

	/* Configure DP clocks */
	tachyon_dp_qmp_configure_dp_clocks(priv);

	/* VCO divider per rate */
	switch (priv->rate) {
	case DP_LINK_RATE_RBR:
	case DP_LINK_RATE_HBR:
		vco_div = 0x1;
		break;
	case DP_LINK_RATE_HBR2:
		vco_div = 0x2;
		break;
	default:
		vco_div = 0x0;
		break;
	}
	log_debug("QMP DP rate cfg: rate=%u vco_div=%u\n",
		    priv->rate, vco_div);
	writel(vco_div, priv->phy_dp + QMP_V4_DP_PHY_VCO_DIV);

	/*
	 * Power up all lanes (PD_CTL=0x7d) BEFORE the DP_PHY_CFG strobe, matching
	 * the edk2 HALDPLib reference (hal_dp_alt_mode_phy_1_3_0.c: PD_CTL set at
	 * :165-179, before the CFG strobe at :224-227). The old order strobed
	 * DP_PHY_CFG while the PHY was still powered down (PD_CTL=0x02), so the
	 * PLL state machine never reached C_READY/PHY_READY (DP_STATUS=00).
	 */
	log_debug("QMP DP PHY start: powering up all lanes (before CFG strobe)\n");
	tachyon_dp_qmp_power_up_all_lanes(priv);
	tachyon_dp_qmp_dump_pll_state(priv, "after PD_CTL=7d");

	/* DP_PHY_CFG start sequence */
	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x05, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x09, priv->phy_dp + QMP_DP_PHY_CFG);

	/* Start the reset state machine */
	writel(0x20, serdes + QMP_V4_COM_RESETSM_CNTRL);
	udelay(10);
	priv->qmp_dp_phy_started = true;
	tachyon_dp_qmp_dump_pll_state(priv, "after RESETSM_CNTRL");

	log_debug("QMP DP start: PD=%02x RESETSM=%02x C_READY=%02x CMN=%02x STATUS=%02x\n",
		    tachyon_dp_qmp_pd_low(priv),
		    readl(serdes + QMP_V4_COM_RESETSM_CNTRL) & 0xff,
		    readl(serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
		    readl(serdes + QMP_V4_COM_CMN_STATUS) & 0xff,
		    tachyon_dp_qmp_status_low(priv));

	/* Poll C_READY */
	tachyon_dp_qmp_dump_serdes_pre_ready(priv);
	tachyon_dp_qmp_dump_pll_state(priv, "before C_READY poll");

	ret = tachyon_dp_qmp_poll(priv,
				  priv->qmp_dp_serdes,
				  QMP_V4_COM_C_READY_STATUS,
				  BIT(0), BIT(0), "C_READY");
	if (ret) {
		log_warning("QMP DP FAIL_STAGE=C_READY ret=%d C_READY=%02x CMN=%02x PD=%02x DP_STATUS=%02x\n",
			    ret,
			    readl(serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
			    readl(serdes + QMP_V4_COM_CMN_STATUS) & 0xff,
			    tachyon_dp_qmp_pd_low(priv),
			    tachyon_dp_qmp_status_low(priv));
		tachyon_dp_qmp_dump_pll_state(priv, "FAIL C_READY");
		return ret;
	}

	/* Poll CMN_STATUS bit0 */
	ret = tachyon_dp_qmp_poll(priv,
				  priv->qmp_dp_serdes,
				  QMP_V4_COM_CMN_STATUS,
				  BIT(0), BIT(0), "CMN_STATUS bit0");
	if (ret) {
		log_warning("QMP DP FAIL_STAGE=CMN_BIT0 ret=%d C_READY=%02x CMN=%02x\n",
			    ret,
			    readl(serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
			    readl(serdes + QMP_V4_COM_CMN_STATUS) & 0xff);
		tachyon_dp_qmp_dump_pll_state(priv, "FAIL CMN_BIT0");
		return ret;
	}

	/* Poll CMN_STATUS bit1 */
	ret = tachyon_dp_qmp_poll(priv,
				  priv->qmp_dp_serdes,
				  QMP_V4_COM_CMN_STATUS,
				  BIT(1), BIT(1), "CMN_STATUS bit1");
	if (ret) {
		log_warning("QMP DP FAIL_STAGE=CMN_BIT1 ret=%d C_READY=%02x CMN=%02x\n",
			    ret,
			    readl(serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
			    readl(serdes + QMP_V4_COM_CMN_STATUS) & 0xff);
		tachyon_dp_qmp_dump_pll_state(priv, "FAIL CMN_BIT1");
		return ret;
	}

	/* DP_PHY_CFG = 0x19 */
	writel(0x19, priv->phy_dp + QMP_DP_PHY_CFG);

	/* Poll DP_PHY_STATUS PHY_READY (bit0) */
	ret = tachyon_dp_qmp_poll(priv, priv->phy_dp, QMP_V4_DP_PHY_STATUS,
				  QMP_DP_PHY_STATUS_PHY_READY,
				  QMP_DP_PHY_STATUS_PHY_READY,
				  "DP_PHY_STATUS PHY_READY");
	if (ret) {
		log_warning("QMP DP FAIL_STAGE=DP_PHY_READY ret=%d PD=%02x STATUS=%02x\n",
			    ret,
			    tachyon_dp_qmp_pd_low(priv),
			    tachyon_dp_qmp_status_low(priv));
		tachyon_dp_qmp_dump_lane_power_state(priv, "FAIL PHY_READY");
		return ret;
	}

	/* Poll DP_PHY_STATUS TSYNC_DONE (bit1) */
	ret = tachyon_dp_qmp_poll(priv, priv->phy_dp, QMP_V4_DP_PHY_STATUS,
				  QMP_DP_PHY_STATUS_TSYNC_DONE,
				  QMP_DP_PHY_STATUS_TSYNC_DONE,
				  "DP_PHY_STATUS TSYNC_DONE");
	if (ret) {
		log_warning("QMP DP WARN_STAGE=DP_TSYNC_DONE ret=%d PD=%02x STATUS=%02x\n",
			    ret,
			    tachyon_dp_qmp_pd_low(priv),
			    tachyon_dp_qmp_status_low(priv));
	}

	log_debug("QMP DP V456 done\n");

	return 0;
}

/*
 * Phase 6: Linux-like qmp_v4_configure_dp_phy().
 * Calls v456_configure for the common sequence, then adds V4-specific
 * TX bias programming and the 0x18->delay->0x19 re-lock sequence.
 */
static int tachyon_dp_qmp_v4_configure_dp_phy(struct tachyon_dp_priv *priv)
{
	int ret;

	ret = tachyon_dp_qmp_v456_configure_dp_phy(priv);
	if (ret)
		return ret;

	/*
	 * Linux says this has to be done after enabling link clock
	 * on at least the 7nm DP PHY.
	 */
	tachyon_dp_qmp_v4_program_tx_bias(priv);

	writel(0x18, priv->phy_dp + QMP_DP_PHY_CFG);
	udelay(2000);
	writel(0x19, priv->phy_dp + QMP_DP_PHY_CFG);

	ret = tachyon_dp_qmp_poll(priv, priv->phy_dp, QMP_V4_DP_PHY_STATUS,
				  QMP_DP_PHY_STATUS_TSYNC_DONE,
				  QMP_DP_PHY_STATUS_TSYNC_DONE,
				  "DP_PHY_STATUS V4 TSYNC");
	if (ret)
		log_warning("QMP DP WARN_STAGE=V4_TSYNC ret=%d PD=%02x STATUS=%02x\n",
			    ret,
			    tachyon_dp_qmp_pd_low(priv),
			    tachyon_dp_qmp_status_low(priv));

	tachyon_dp_qmp_program_tx_levels(priv);

	log_debug("QMP DP V4 post-cfg 18->19 done\n");

	return 0;
}

void tachyon_dp_qmp_aux_init(struct tachyon_dp_priv *priv)
{
	writel(0x00, priv->phy_dp + QMP_DP_PHY_AUX_CFG0);
	writel(0x13, priv->phy_dp + QMP_DP_PHY_AUX_CFG1);
	writel(0xa4, priv->phy_dp + QMP_DP_PHY_AUX_CFG2);
	writel(0x00, priv->phy_dp + QMP_DP_PHY_AUX_CFG3);
	writel(0x0a, priv->phy_dp + QMP_DP_PHY_AUX_CFG4);
	writel(0x26, priv->phy_dp + QMP_DP_PHY_AUX_CFG5);
	writel(0x0a, priv->phy_dp + QMP_DP_PHY_AUX_CFG6);
	writel(0x03, priv->phy_dp + QMP_DP_PHY_AUX_CFG7);
	writel(0xb7, priv->phy_dp + QMP_DP_PHY_AUX_CFG8);
	writel(0x03, priv->phy_dp + QMP_DP_PHY_AUX_CFG9);
}

/*
 * Linux-aligned DP PHY programming:
 *   power_down -> program_serdes -> program_tx_table ->
 *   deassert_serdes_reset ->
 *   v4_configure_dp_phy (V456 start + polls + V4 bias + re-lock + TX levels)
 */
int tachyon_dp_qmp_program_dp_phy(struct tachyon_dp_priv *priv)
{
	int ret;

	log_debug("QMP DP program start: rate=%u lanes=%u orientation=%u\n",
		    priv->rate, priv->lanes, priv->orientation);

	tachyon_dp_qmp_power_down(priv);

	log_debug("QMP DP SERDES table start\n");
	ret = tachyon_dp_qmp_program_serdes(priv);
	log_debug("QMP DP SERDES table done ret=%d\n", ret);
	if (ret)
		return ret;

	log_debug("QMP DP TX table start\n");
	tachyon_dp_qmp_program_tx_table(priv);
	tachyon_dp_qmp_deassert_serdes_reset(priv);
	log_debug("QMP DP TX table done\n");

	/*
	 * Linux-aligned V4 DP PHY configure:
	 * Replaces old program_dp_phy_regs() + start_dp_phy() +
	 * inline poll sequence with the full Linux flow:
	 *   V456 start -> C_READY/CMN/DP_PHY_STATUS polls ->
	 *   V4 TX bias -> 0x18->delay->0x19 re-lock ->
	 *   TX levels defaults.
	 */
	ret = tachyon_dp_qmp_v4_configure_dp_phy(priv);
	if (ret)
		return ret;

	log_debug("QMP DP program done\n");

	return 0;
}

int tachyon_dp_qmp_configure(struct tachyon_dp_priv *priv)
{
	u32 typec;
	int ret;
	u32 com_pwr, rovrd, swr, swi, c_ready, cmn, dp_pd, dp_status;

	if (!priv->rate || !priv->lanes) {
		log_warning("QMP DP configure invalid training state: rate=%u lanes=%u\n",
			    priv->rate, priv->lanes);
		return -EINVAL;
	}

	if (priv->lanes > 4) {
		log_warning("QMP DP configure invalid lane count: %u\n",
			    priv->lanes);
		return -EINVAL;
	}

	log_debug("QMP DP configure enter: rate=%u lanes=%u orientation=%u\n",
		    priv->rate, priv->lanes, priv->orientation);

	/* Direct writel: QMP COM registers are byte-style, RMW corrupts */
	typec = QMP_DP_COM_SW_PORTSELECT_MUX;
	if (priv->orientation == TACHYON_DP_ORIENTATION_REVERSE)
		typec |= QMP_DP_COM_SW_PORTSELECT_VAL;

	writel(tachyon_dp_qmp_phy_mode(priv),
	       priv->qmp_com + QMP_V3_DP_COM_PHY_MODE_CTRL);
	writel(typec,
	       priv->qmp_com + QMP_V3_DP_COM_TYPEC_CTRL);

	/* Compact register dump before DP SerDes programming */
	com_pwr = tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_POWER_DOWN_CTRL);
	rovrd   = tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_RESET_OVRD_CTRL);
	swr     = tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SW_RESET);
	swi     = tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SWI_CTRL);
	c_ready = readl(priv->qmp_dp_serdes + QMP_V4_COM_C_READY_STATUS) & 0xff;
	cmn     = readl(priv->qmp_dp_serdes + QMP_V4_COM_CMN_STATUS) & 0xff;
	dp_pd   = tachyon_dp_qmp_pd_low(priv);
	dp_status = tachyon_dp_qmp_status_low(priv);

	log_debug("QMP DP dump before: COM_PWR=%02x ROVRD=%02x SWR=%02x SWI=%02x TYPEC=%02x MODE=%02x C_READY=%02x CMN=%02x DP_PD=%02x DP_STATUS=%02x\n",
		    com_pwr, rovrd, swr, swi,
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    c_ready, cmn, dp_pd, dp_status);

	tachyon_dp_qmp_aux_init(priv);
	ret = tachyon_dp_qmp_program_dp_phy(priv);
	if (ret)
		return ret;

	/* Compact register dump after DP SerDes programming */
	c_ready = readl(priv->qmp_dp_serdes + QMP_V4_COM_C_READY_STATUS) & 0xff;
	cmn     = readl(priv->qmp_dp_serdes + QMP_V4_COM_CMN_STATUS) & 0xff;
	dp_pd   = tachyon_dp_qmp_pd_low(priv);
	dp_status = tachyon_dp_qmp_status_low(priv);

	log_debug("QMP DP dump after:  COM_PWR=%02x ROVRD=%02x SWR=%02x SWI=%02x TYPEC=%02x MODE=%02x C_READY=%02x CMN=%02x DP_PD=%02x DP_STATUS=%02x\n",
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_POWER_DOWN_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_RESET_OVRD_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SW_RESET),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SWI_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    c_ready, cmn, dp_pd, dp_status);

	log_debug("QMP DP configure done\n");

	return 0;
}

bool tachyon_dp_qmp_phy_ready(struct tachyon_dp_priv *priv)
{
	u8 status;

	if (!priv->phy_dp)
		return false;

	status = tachyon_dp_qmp_status_low(priv);
	return !!(status & QMP_DP_PHY_STATUS_PHY_READY);
}
