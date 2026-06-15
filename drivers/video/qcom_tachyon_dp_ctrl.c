// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort controller: link training (TP1 CR / TP2 EQ),
 * source-link + mainlink config, and the video path -- pixel-clock M/N, MSA,
 * the bit-exact transfer-unit (calc_tu) fixed-point math, and p0/video timing.
 * Split out of qcom_tachyon_dp.c; operates on the shared struct tachyon_dp_priv.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <clk.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include "qcom_tachyon_dp.h"

static u32 tachyon_dp_configuration_ctrl(struct tachyon_dp_priv *priv)
{
	/*
	 *  - SYNC_ASYNC_CLK (bit0) + STATIC_DYNAMIC_CN (bit1): synchronous clock
	 *    + static Mvid.  Linux msm dp_ctrl sets BOTH ("sync clock & static
	 *    Mvid", dp_ctrl.c:421-422); we previously set only bit1.
	 *  - P_INTERLACED (bit2): in this controller SETTING the bit selects
	 *    PROGRESSIVE — Linux dp_ctrl.c:418 ORs DP_CONFIGURATION_CTRL_P_INTERLACED
	 *    with the comment "progressive video".  We previously CLEARED it, which
	 *    tells the TX the stream is interlaced -> a progressive DP->HDMI dock
	 *    rejects the MSA = "No Signal".  (The macro name is misleading; set=prog.)
	 *  - BPC = 1 (8bpc) for our 24bpp XRGB8888 pixels.
	 */
	u32 cfg = DP_CONFIGURATION_CTRL_SYNC_ASYNC_CLK |
		  DP_CONFIGURATION_CTRL_STATIC_DYNAMIC_CN |
		  DP_CONFIGURATION_CTRL_P_INTERLACED |
		  (2 << DP_CONFIGURATION_CTRL_LSCLK_DIV_SHIFT) |
		  ((priv->lanes - 1) <<
		   DP_CONFIGURATION_CTRL_NUM_OF_LANES_SHIFT) |
		  (1 << DP_CONFIGURATION_CTRL_BPC_SHIFT);

	if (priv->caps.enhanced)
		cfg |= DP_CONFIGURATION_CTRL_ENHANCED_FRAMING;

	return cfg;
}

void tachyon_dp_mainlink_disable(struct tachyon_dp_priv *priv)
{
	u32 val = readl(priv->link + REG_DP_MAINLINK_CTRL);

	val &= ~(DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_CTRL_RESET);
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	log_warning("DP mainlink disable: MAINLINK_CTRL=%08x\n",
		    readl(priv->link + REG_DP_MAINLINK_CTRL));
}

static void tachyon_dp_mainlink_enable_training(struct tachyon_dp_priv *priv)
{
	u32 val = readl(priv->link + REG_DP_MAINLINK_CTRL);

	val &= ~(DP_MAINLINK_CTRL_RESET | DP_MAINLINK_CTRL_ENABLE);
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	val |= DP_MAINLINK_CTRL_RESET;
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	val &= ~DP_MAINLINK_CTRL_RESET;
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	val |= DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL |
	       DP_MAINLINK_CTRL_FLUSH_MODE;
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	log_warning("DP mainlink enable linux-seq: MAINLINK_CTRL=%08x MAINLINK_READY=%08x STATE_CTRL=%08x\n",
		    readl(priv->link + REG_DP_MAINLINK_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_READY),
		    readl(priv->link + REG_DP_STATE_CTRL));
}

void tachyon_dp_configure_source_link(struct tachyon_dp_priv *priv)
{
	u32 cfg = tachyon_dp_configuration_ctrl(priv);

	writel(priv->lane_map, priv->link + REG_DP_LOGICAL2PHYSICAL_LANE_MAPPING);
	writel(cfg, priv->link + REG_DP_CONFIGURATION_CTRL);

	log_warning("DP source link cfg: rate=%u bw=%02x lanes=%u enhanced=%u lane_map=%08x cfg=%08x\n",
		    priv->rate, tachyon_dp_bw_code(priv->rate), priv->lanes,
		    priv->caps.enhanced ? 1 : 0, priv->lane_map, cfg);
}

static int tachyon_dp_set_pattern_state_bit(struct tachyon_dp_priv *priv,
					    u32 state_bit)
{
	u32 state;
	u32 ready_bit;
	int ret;

	if (!state_bit || state_bit > 2)
		return -EINVAL;

	state = BIT(state_bit - 1);
	ready_bit = state << DP_MAINLINK_READY_LINK_TRAINING_SHIFT;

	log_warning("DP source pattern %u select: STATE_CTRL=%08x ready_bit=%08x\n",
		    state_bit, state, ready_bit);

	writel(0, priv->link + REG_DP_STATE_CTRL);
	writel(state, priv->link + REG_DP_STATE_CTRL);

	tachyon_dp_dump_link_state(priv, "after source pattern select");

	ret = tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
				   ready_bit, ready_bit, 10000);
	if (ret) {
		log_warning("DP source pattern %u not ready: ret=%d MAINLINK_READY=%08x STATE_CTRL=%08x MAINLINK_CTRL=%08x CONFIG_CTRL=%08x\n",
			    state_bit, ret,
			    readl(priv->link + REG_DP_MAINLINK_READY),
			    readl(priv->link + REG_DP_STATE_CTRL),
			    readl(priv->link + REG_DP_MAINLINK_CTRL),
			    readl(priv->link + REG_DP_CONFIGURATION_CTRL));
		tachyon_dp_dump_link_state(priv, "source pattern not ready");
		return ret;
	}

	log_warning("DP source pattern %u ready: MAINLINK_READY=%08x ready_bit=%08x\n",
		    state_bit, readl(priv->link + REG_DP_MAINLINK_READY),
		    ready_bit);

	tachyon_dp_dump_link_state(priv, "after source pattern ready");

	return 0;
}

static int tachyon_dp_program_training_set(struct tachyon_dp_priv *priv)
{
	u8 training[4] = {}, rb[4] = {};
	u8 i;
	int ret;

	ret = tachyon_dp_qmp_program_tx(priv);
	if (ret)
		return ret;

	for (i = 0; i < priv->lanes; i++)
		training[i] = tachyon_dp_train_set(priv, i);

	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_LANE0_SET,
				   training, priv->lanes);
	if (ret)
		return ret;

	tachyon_dp_dump_link_state(priv, "after lane train set");

	ret = tachyon_dp_aux_retry(priv, false, true, DP_TRAINING_LANE0_SET,
				   rb, sizeof(rb));
	if (ret) {
		log_warning("DP DPCD lane set verify failed: ret=%d\n", ret);
		return ret;
	}

	log_warning("DP DPCD lane set verify: lane0=%02x lane1=%02x lane2=%02x lane3=%02x\n",
		    rb[0], rb[1], rb[2], rb[3]);

	for (i = 0; i < priv->lanes; i++) {
		if (rb[i] != training[i]) {
			log_warning("DP DPCD lane set mismatch: lane=%u got=%02x expected=%02x\n",
				    i, rb[i], training[i]);
			return -EIO;
		}
	}

	return 0;
}

static int tachyon_dp_link_train_at(struct tachyon_dp_priv *priv, u32 rate,
				    u8 lanes)
{
	u8 link[2], rb[2], pattern, pattern_rb, status[6], adj[2];
	int ret, tries;
	u8 lane;

#if TACHYON_DP_FORCE_TRAIN_RBR_X4
	if (rate != DP_LINK_RATE_RBR || lanes != 4)
		log_warning("DP force RBR x4 overrides requested %u kHz x %u lanes\n",
			    rate, lanes);
	rate = DP_LINK_RATE_RBR;
	lanes = 4;
#endif
#if TACHYON_DP_FORCE_TRAIN_RBR_X2
	if (rate != DP_LINK_RATE_RBR || lanes != 2)
		log_warning("DP force RBR x2 overrides requested %u kHz x %u lanes\n",
			    rate, lanes);
	rate = DP_LINK_RATE_RBR;
	lanes = 2;
#endif
#if TACHYON_DP_FORCE_TRAIN_RBR_X1
	if (rate != DP_LINK_RATE_RBR || lanes != 1)
		log_warning("DP force RBR x1 overrides requested %u kHz x %u lanes\n",
			    rate, lanes);
	rate = DP_LINK_RATE_RBR;
	lanes = 1;
#endif

	lanes = min_t(u8, lanes, priv->max_lanes ?: 1);
	lanes = min_t(u8, lanes, priv->caps.lanes ?: 1);

	priv->rate = rate;
	priv->lanes = lanes;
	memset(priv->swing, 0, sizeof(priv->swing));
	memset(priv->pre, 0, sizeof(priv->pre));

	log_warning("DP training: rate=%u kHz lanes=%u\n",
		    priv->rate, priv->lanes);

	log_warning("DP TRAIN STEP: disable mainlink\n");
	tachyon_dp_mainlink_disable(priv);
	tachyon_dp_dump_link_state(priv, "after mainlink disable");

	ret = tachyon_dp_qmp_configure(priv);
	if (ret)
		return ret;

	log_warning("DP orientation summary: orientation=%u TYPEC=%02x PHY_MODE=%02x SBU_EN=%d SBU_SEL=%d lane_count=%u\n",
		    priv->orientation,
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1,
		    priv->lanes);

	log_warning("DP TRAIN STEP: mainlink training enable\n");
	tachyon_dp_mainlink_enable_training(priv);
	tachyon_dp_dump_link_state(priv, "after mainlink training enable");

	log_warning("DP TRAIN STEP: source link config\n");
	tachyon_dp_configure_source_link(priv);
	tachyon_dp_dump_link_state(priv, "after source link config");

	link[0] = tachyon_dp_bw_code(priv->rate);
	link[1] = priv->lanes |
		  (priv->caps.enhanced ? DP_ENHANCED_FRAME_CAP : 0);
	log_warning("DP TRAIN STEP: sink link config\n");
	tachyon_dp_dump_link_state(priv, "before DPCD link cfg");
	ret = tachyon_dp_aux_retry(priv, false, false, DP_LINK_BW_SET, link,
				   sizeof(link));
	if (ret)
		return ret;
	log_warning("DP DPCD link cfg: rate=%u lanes=%u enhanced=%u lane_count_reg=%02x\n",
		    priv->rate, priv->lanes, priv->caps.enhanced, link[1]);
	tachyon_dp_dump_link_state(priv, "after DPCD link cfg");

	ret = tachyon_dp_aux_retry(priv, false, true, DP_LINK_BW_SET, rb,
				   sizeof(rb));
	if (ret)
		return ret;
	log_warning("DP DPCD link cfg verify: bw=%02x lane_count=%02x expected_bw=%02x expected_lane=%02x\n",
		    rb[0], rb[1], link[0], link[1]);
	if (memcmp(rb, link, sizeof(link)))
		return -EIO;

	pattern = 0;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_DOWNSPREAD_CTRL,
				   &pattern, 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true, DP_DOWNSPREAD_CTRL,
				   &pattern_rb, 1);
	if (ret)
		return ret;
	log_warning("DP DPCD downspread verify: downspread=%02x expected=%02x\n",
		    pattern_rb, pattern);
	if (pattern_rb != pattern)
		return -EIO;

	pattern = 1;
	ret = tachyon_dp_aux_retry(priv, false, false,
				   DP_MAIN_LINK_CHANNEL_CODING_SET, &pattern,
				   1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true,
				   DP_MAIN_LINK_CHANNEL_CODING_SET,
				   &pattern_rb, 1);
	if (ret)
		return ret;
	log_warning("DP DPCD coding verify: coding=%02x expected=%02x\n",
		    pattern_rb, pattern);
	if (pattern_rb != pattern)
		return -EIO;

	log_warning("DP TRAIN STEP: source TP1 select\n");
	ret = tachyon_dp_set_pattern_state_bit(priv, 1);
	if (ret)
		return ret;

	log_warning("DP TRAIN STEP: sink TP1 select\n");
	pattern = DP_TRAINING_PATTERN_1 | DP_LINK_SCRAMBLING_DISABLE;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
				   &pattern, 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true, DP_TRAINING_PATTERN_SET,
				   &pattern_rb, 1);
	if (ret)
		return ret;
	log_warning("DP DPCD TP verify: pattern_set=%02x expected=%02x\n",
		    pattern_rb, pattern);
	if (pattern_rb != pattern)
		return -EIO;
	tachyon_dp_dump_link_state(priv, "after sink TP1");

	/* --- Training Pattern 1 (clock recovery) --- */
	for (tries = 0; tries < 5; tries++) {
		ret = tachyon_dp_program_training_set(priv);
		if (ret)
			return ret;
		udelay(10000);
		log_warning("DP TRAIN STEP: lane status read\n");
		tachyon_dp_dump_link_state(priv, "before CR status read");
		ret = tachyon_dp_aux_retry(priv, false, true,
					   DP_LANE0_1_STATUS, status, 6);
		if (ret) {
			log_warning("DP CR lane-status read FAILED ret=%d (try %d)\n",
				    ret, tries);
			return ret;
		}
		log_warning("DP CR try%d: l01=%02x l23=%02x align=%02x adj01=%02x adj23=%02x cr_done=%d\n",
			    tries, status[0], status[1], status[2],
			    status[4], status[5],
			    tachyon_dp_cr_done(status, lanes));
		if (tachyon_dp_cr_done(status, lanes))
			break;
		memcpy(adj, &status[4], sizeof(adj));
		tachyon_dp_apply_adjust(priv, adj);
	}

	if (tries == 5) {
		log_warning("DP clock recovery failed: status=%02x %02x %02x\n",
			    status[0], status[1], status[2]);
		tachyon_dp_dump_link_state(priv, "clock recovery failed");
		return -EIO;
	}

	log_info("DP clock recovery OK after %d tries: lanes[0-1]=%02x lanes[2-3]=%02x align=%02x\n",
		 tries + 1, status[0], status[1], status[2]);

	/* --- Training Pattern 2 (channel equalization) --- */
	log_warning("DP TRAIN STEP: source TP2 select\n");
	ret = tachyon_dp_set_pattern_state_bit(priv, 2);
	if (ret)
		return ret;

	log_warning("DP TRAIN STEP: sink TP2 select\n");
	pattern = DP_TRAINING_PATTERN_2 | DP_LINK_SCRAMBLING_DISABLE;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
				   &pattern, 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true, DP_TRAINING_PATTERN_SET,
				   &pattern_rb, 1);
	if (ret)
		return ret;
	log_warning("DP DPCD TP verify: pattern_set=%02x expected=%02x\n",
		    pattern_rb, pattern);
	if (pattern_rb != pattern)
		return -EIO;
	tachyon_dp_dump_link_state(priv, "after sink TP2");

	for (tries = 0; tries < 5; tries++) {
		ret = tachyon_dp_program_training_set(priv);
		if (ret)
			return ret;
		udelay(10000);
		log_warning("DP TRAIN STEP: lane status read\n");
		tachyon_dp_dump_link_state(priv, "before EQ status read");
		ret = tachyon_dp_aux_retry(priv, false, true,
					   DP_LANE0_1_STATUS, status, 6);
		if (ret) {
			log_warning("DP EQ lane-status read FAILED ret=%d (try %d)\n",
				    ret, tries);
			return ret;
		}
		log_warning("DP EQ try%d: l01=%02x l23=%02x align=%02x adj01=%02x adj23=%02x eq_done=%d\n",
			    tries, status[0], status[1], status[2],
			    status[4], status[5],
			    tachyon_dp_eq_done(status, lanes));
		if (tachyon_dp_eq_done(status, lanes))
			break;
		memcpy(adj, &status[4], sizeof(adj));
		tachyon_dp_apply_adjust(priv, adj);
	}

	if (tries == 5) {
		log_warning("DP channel equalization failed: status=%02x %02x %02x\n",
			    status[0], status[1], status[2]);
		tachyon_dp_dump_link_state(priv, "channel EQ failed");
		return -EIO;
	}

	log_info("DP channel EQ OK after %d tries\n", tries + 1);
	for (lane = 0; lane < lanes; lane++)
		log_info("  Lane %d: swing=%u pre=%u\n", lane,
			 priv->swing[lane], priv->pre[lane]);

	pattern = DP_TRAINING_PATTERN_DISABLE;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
				   &pattern, 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true, DP_TRAINING_PATTERN_SET,
				   &pattern_rb, 1);
	if (ret)
		return ret;
	log_warning("DP DPCD TP verify: pattern_set=%02x expected=%02x\n",
		    pattern_rb, pattern);
	if (pattern_rb != pattern)
		return -EIO;
	writel(DP_STATE_CTRL_SEND_VIDEO, priv->link + REG_DP_STATE_CTRL);
	tachyon_dp_dump_link_state(priv, "after training disable");

	return 0;
}

int tachyon_dp_link_train(struct tachyon_dp_priv *priv)
{
	static const u32 rates[] = {
		DP_LINK_RATE_HBR3, DP_LINK_RATE_HBR2, DP_LINK_RATE_HBR,
		DP_LINK_RATE_RBR,
	};
	static const u8 lane_counts[] = { 4, 2, 1 };
	u32 policy_rate = priv->rate;
	u8 policy_lanes = priv->lanes;
	int r, l, ret = -EIO;

	log_warning("DP train policy: typec=altmode orientation=%u pin=%u sink_lanes=%u graph_lanes=%u pin_lanes=%u policy_lanes=%u max_rate=%u lane_map=%02x\n",
		    priv->orientation, priv->pin_assignment,
		    priv->caps.lanes, priv->graph_lanes,
		    tachyon_dp_pin_assignment_lanes(priv), policy_lanes,
		    priv->max_rate, priv->lane_map & 0xff);

	/*
	 * Guard: without valid sink DPCD (lane count / link rate) there is no
	 * reachable DP sink — refuse to train rather than program the link/DPU
	 * with zero lanes (which previously hard-crashed the device).
	 */
	if (!priv->caps.lanes || !priv->max_rate) {
		log_warning("DP: no valid sink (lanes=%u max_rate=%u) - skipping link training\n",
			    priv->caps.lanes, priv->max_rate);
		return -ENODEV;
	}

	/*
	 * Wait for the dock to raise DisplayPort HPD before driving the link.
	 * Training before the sink's main-link receiver is ready fails clock
	 * recovery (lane status 00) at every rate; HPD is the reliable
	 * "receiver ready" signal and removes the boot-time intermittency.
	 * Disable with tachyon_dp_no_hpd_wait; tune via tachyon_dp_hpd_wait_ms.
	 */
	if (!tachyon_dp_env_bool("tachyon_dp_no_hpd_wait")) {
		uint hpd_ms = tachyon_dp_env_u32("tachyon_dp_hpd_wait_ms", 8000);

		log_warning("DP link train: PMIC HPD %s before training\n",
			    tachyon_dp_wait_pmic_hpd(priv, hpd_ms) ?
				    "asserted" : "wait timed out, training anyway");
	}

#if TACHYON_DP_FORCE_TRAIN_RBR_X4
	/*
	 * Phase 10: Known-good retune test.
	 * Only try RBR x4 to isolate "second configure after lock"
	 * vs. "non-RBR or reduced-lane specific" failures.
	 */
	log_warning("DP force RBR x4 training test\n");
	ret = tachyon_dp_link_train_at(priv, DP_LINK_RATE_RBR, 4);
	if (!ret) {
		log_info("DP link trained at RBR x4 (forced)\n");
		return 0;
	}
	log_warning("DP force RBR x4 failed: %d\n", ret);
	return ret;
#endif
#if TACHYON_DP_FORCE_TRAIN_RBR_X2
	log_warning("DP force RBR x2 training test\n");
	ret = tachyon_dp_link_train_at(priv, DP_LINK_RATE_RBR, 2);
	if (!ret) {
		log_info("DP link trained at RBR x2 (forced)\n");
		return 0;
	}
	log_warning("DP force RBR x2 failed: %d\n", ret);
	return ret;
#endif
#if TACHYON_DP_FORCE_TRAIN_RBR_X1
	log_warning("DP force RBR x1 training test\n");
	ret = tachyon_dp_link_train_at(priv, DP_LINK_RATE_RBR, 1);
	if (!ret) {
		log_info("DP link trained at RBR x1 (forced)\n");
		return 0;
	}
	log_warning("DP force RBR x1 failed: %d\n", ret);
	return ret;
#endif

	for (r = 0; r < ARRAY_SIZE(rates); r++) {
		if (rates[r] > policy_rate)
			continue;
		for (l = 0; l < ARRAY_SIZE(lane_counts); l++) {
			if (lane_counts[l] > policy_lanes)
				continue;
			log_warning("DP link train attempt: rate=%u lanes=%u\n",
				    rates[r], lane_counts[l]);
			ret = tachyon_dp_link_train_at(priv, rates[r],
						       lane_counts[l]);
			log_warning("DP link train attempt rate=%u lanes=%u ret=%d\n",
				    rates[r], lane_counts[l], ret);
			if (!ret) {
				log_info("DP link trained at %u kHz x %u lanes\n",
					 rates[r], lane_counts[l]);
				return 0;
			}
			log_warning("DP training failed at %u kHz x %u lanes: %d\n",
				    rates[r], lane_counts[l], ret);
		}
	}

	return ret;
}

u32 tachyon_dp_htotal(const struct display_timing *t)
{
	return t->hactive.typ + t->hfront_porch.typ + t->hsync_len.typ +
	       t->hback_porch.typ;
}

u32 tachyon_dp_vtotal(const struct display_timing *t)
{
	return t->vactive.typ + t->vfront_porch.typ + t->vsync_len.typ +
	       t->vback_porch.typ;
}

/* Implemented in drivers/clk/qcom/clock-sc7280-dispcc.c */
void sc7280_dispcc_set_dp_pixel_mn(u32 m, u32 n);

/*
 * Compute the DP pixel-clock M/N divider: pixel_clk = input_clk * M / N, so
 * M/N = pixel_khz / input_khz reduced by their GCD and scaled into 16 bits.
 * (Self-contained; avoids a dependency on CONFIG_RATIONAL.)
 */
static void tachyon_dp_pixel_mn(unsigned long input_khz, unsigned long pixel_khz,
				u32 *m, u32 *n)
{
	unsigned long a = pixel_khz, b = input_khz, x = a, y = b, g;

	while (y) {
		unsigned long t = x % y;

		x = y;
		y = t;
	}
	g = x ? x : 1;
	a /= g;
	b /= g;
	while (a > 0xffff || b > 0xffff) {
		a >>= 1;
		b >>= 1;
	}
	*m = a ? a : 1;
	*n = b ? b : 1;
}

/*
 * Program the DP pixel clock.  The dispcc DP pixel RCG is sourced from the DP
 * PHY PLL VCO_DIV output (= link_rate*10/pixel_div) and must be divided down to
 * the mode pixel clock by an M/N divider.  Compute M/N here (we know both the
 * link rate and the pixel rate) and hand them to the dispcc driver before
 * enabling — otherwise the pixel clock runs at the full VCO_DIV rate (~1.35 GHz
 * for HBR2), ~9x too fast, and the sink cannot lock the video (No Signal).
 * Mirrors Linux msm dp_ctrl msm_dp_ctrl_config_msa().
 */
static void tachyon_dp_program_pixel_clock(struct tachyon_dp_priv *priv)
{
	u32 rate = priv->timing.pixelclock.typ;
	unsigned long pixel_div, dispcc_input_khz;
	u32 pixel_khz, m = 0, n = 0;
	long ret;

	if (!rate || !priv->has_pixel_clk) {
		log_warning("DP pixel clock SKIPPED: rate=%u has_pixel_clk=%d\n",
			    rate, priv->has_pixel_clk);
		return;
	}

	switch (priv->rate) {
	case DP_LINK_RATE_HBR3:
		pixel_div = 6;
		break;
	case DP_LINK_RATE_HBR2:
		pixel_div = 4;
		break;
	default:		/* RBR, HBR */
		pixel_div = 2;
		break;
	}

	pixel_khz = rate / 1000;
	dispcc_input_khz = ((unsigned long)priv->rate * 10) / pixel_div;
	tachyon_dp_pixel_mn(dispcc_input_khz, pixel_khz, &m, &n);
	sc7280_dispcc_set_dp_pixel_mn(m, n);
	log_warning("DP pixel clk M/N: link=%u input_khz=%lu pixel_khz=%u -> M=%u N=%u\n",
		    priv->rate, dispcc_input_khz, pixel_khz, m, n);

	ret = clk_set_rate(&priv->pixel_clk, rate);
	log_warning("DP pixel clock set_rate %u Hz -> ret=%ld\n", rate, ret);

	if (priv->pixel_clk_enabled)
		return;

	ret = clk_enable(&priv->pixel_clk);
	log_warning("DP pixel clock enable ret=%ld\n", ret);
	if (ret >= 0)
		priv->pixel_clk_enabled = true;
}

static void tachyon_dp_program_msa_timing(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	u32 htotal = tachyon_dp_htotal(t);
	u32 vtotal = tachyon_dp_vtotal(t);
	u32 hsync_start = t->hactive.typ + t->hfront_porch.typ;
	u32 vsync_start = t->vactive.typ + t->vfront_porch.typ;
	u32 total, sync_start, width_polarity, active;

	total = (vtotal << 16) | htotal;
	sync_start = ((vtotal - vsync_start) << 16) |
		     (htotal - hsync_start);
	width_polarity = (t->vsync_len.typ << 16) | t->hsync_len.typ;
	if (t->flags & DISPLAY_FLAGS_VSYNC_LOW)
		width_polarity |= BIT(31);
	if (t->flags & DISPLAY_FLAGS_HSYNC_LOW)
		width_polarity |= BIT(15);
	active = (t->vactive.typ << 16) | t->hactive.typ;

	writel(total, priv->link + REG_DP_TOTAL_HOR_VER);
	writel(sync_start, priv->link + REG_DP_START_HOR_VER_FROM_SYNC);
	writel(width_polarity,
	       priv->link + REG_DP_HSYNC_VSYNC_WIDTH_POLARITY);
	writel(active, priv->link + REG_DP_ACTIVE_HOR_VER);
}

static void tachyon_dp_program_msa_clock(struct tachyon_dp_priv *priv)
{
	u64 pclk_khz = priv->timing.pixelclock.typ / 1000;
	u32 nvid = 0x8000;
	u32 mvid;

	if (!pclk_khz || !priv->rate)
		return;

	if (priv->rate >= DP_LINK_RATE_HBR3)
		nvid *= 3;
	else if (priv->rate >= DP_LINK_RATE_HBR2)
		nvid *= 2;

	mvid = (u32)((pclk_khz * nvid) / priv->rate);
	if (!mvid)
		mvid = 1;

	writel(mvid, priv->link + REG_DP_SOFTWARE_MVID);
	writel(nvid, priv->link + REG_DP_SOFTWARE_NVID);
	/*
	 * MISC0[7:5] = component bit depth: 0=6bpc, 1=8bpc, 2=10bpc.  We scan out
	 * 8bpc XRGB8888 pixels, so this MUST declare 8bpc (1).  A previous value
	 * of 2 (10bpc) made the MSA disagree with the actual pixel data, which a
	 * DP->HDMI dock converter rejects -> sink shows "No Signal" even though
	 * the link is trained and MAINLINK_READY_FOR_VIDEO is asserted.
	 * Matches Linux msm dp_ctrl (DP_TEST_BIT_DEPTH_8 = 1 << 5).
	 */
	writel(DP_MISC0_SYNCHRONOUS_CLK | (1 << DP_MISC0_TEST_BITS_DEPTH_SHIFT),
	       priv->link + REG_DP_MISC1_MISC0);
	log_warning("DP MSA: mvid=%u nvid=%u misc0=%08x pclk_khz=%llu rate=%u\n",
		    mvid, nvid, readl(priv->link + REG_DP_MISC1_MISC0),
		    pclk_khz, priv->rate);
}

/*
 * --------------------------------------------------------------------------
 * DP transfer-unit (TU) calculation -- bit-exact port of the Linux msm DP
 * driver's _dp_ctrl_calc_tu() + helpers (drivers/gpu/drm/msm/dp/dp_ctrl.c),
 * verified against the standalone reference tu_calc_ref.c.  Computes
 * TU_SIZE / VALID_BOUNDARY_LINK / DELAY_START_LINK and the boundary-
 * moderation parameters for ANY mode (the old code hardcoded the 1080p
 * values and fell back to a boundary-moderation-less calc otherwise, which
 * the dock would not frame-lock).  DRM 32.32 signed fixed-point exactly as
 * include/drm/drm_fixed.h.
 * --------------------------------------------------------------------------
 */
#define TDP_FXP_POINT		32
#define TDP_FXP_ONE		(1ULL << TDP_FXP_POINT)
#define TDP_FXP_ALMOST_ONE	(TDP_FXP_ONE - 1ULL)

static u64 tdp_div64_u64_rem(u64 dividend, u64 divisor, s64 *rem)
{
	*rem = (s64)(dividend % divisor);
	return dividend / divisor;
}

static int tdp_fixp2int(s64 a)
{
	return (int)(a >> TDP_FXP_POINT);
}

static int tdp_fixp2int_ceil(s64 a)
{
	if (a >= 0)
		return tdp_fixp2int(a + TDP_FXP_ALMOST_ONE);
	return tdp_fixp2int(a - TDP_FXP_ALMOST_ONE);
}

static unsigned int tdp_fixp_msbset(s64 a)
{
	unsigned int shift, sign = (a >> 63) & 1;

	for (shift = 62; shift > 0; --shift)
		if (((a >> shift) & 1) != sign)
			return shift;
	return 0;
}

static s64 tdp_fixp_mul(s64 a, s64 b)
{
	unsigned int shift = tdp_fixp_msbset(a) + tdp_fixp_msbset(b);
	s64 result;

	if (shift > 61) {
		shift = shift - 61;
		a >>= (shift >> 1) + (shift & 1);
		b >>= shift >> 1;
	} else {
		shift = 0;
	}

	result = a * b;

	if (shift > TDP_FXP_POINT)
		return result << (shift - TDP_FXP_POINT);
	if (shift < TDP_FXP_POINT)
		return result >> (TDP_FXP_POINT - shift);
	return result;
}

static s64 tdp_fixp_div(s64 a, s64 b)
{
	unsigned int shift = 62 - tdp_fixp_msbset(a);
	s64 result;

	a <<= shift;
	if (shift < TDP_FXP_POINT)
		b >>= (TDP_FXP_POINT - shift);

	result = a / b;

	if (shift > TDP_FXP_POINT)
		return result >> (shift - TDP_FXP_POINT);
	return result;
}

static s64 tdp_fixp_from_fraction(s64 a, s64 b)
{
	bool a_neg = a < 0;
	bool b_neg = b < 0;
	u64 a_abs = a_neg ? (u64)(-a) : (u64)a;
	u64 b_abs = b_neg ? (u64)(-b) : (u64)b;
	s64 rem_s;
	u64 rem, res_abs;
	s64 res;
	u32 i;

	res_abs = tdp_div64_u64_rem(a_abs, b_abs, &rem_s);
	rem = (u64)rem_s;

	for (i = TDP_FXP_POINT; i != 0; --i) {
		rem <<= 1;
		res_abs <<= 1;
		if (rem >= b_abs) {
			res_abs |= 1;
			rem -= b_abs;
		}
	}

	res_abs += (rem << 1) >= b_abs ? 1 : 0;

	res = (s64)res_abs;
	if (a_neg ^ b_neg)
		res = -res;
	return res;
}

struct tdp_tu_input {
	u64 lclk;		/* 162, 270, 540, 810 */
	u64 pclk_khz;
	u64 hactive;
	u64 hporch;		/* bp + fp + pulse */
	int nlanes;
	int bpp;
	int pixel_enc;		/* 444, 420, 422 */
	int dsc_en;
	int async_en;
	int fec_en;
	int compress_ratio;
	int num_of_dsc_slices;
};

struct tdp_tu_table {
	u8 valid_boundary_link;
	u16 delay_start_link;
	bool boundary_moderation_en;
	u8 valid_lower_boundary_link;
	u8 upper_boundary_count;
	u8 lower_boundary_count;
	u8 tu_size_minus1;
};

struct tdp_tu_algo {
	s64 lclk_fp;
	s64 pclk_fp;
	s64 lwidth;
	s64 lwidth_fp;
	s64 hbp_relative_to_pclk;
	s64 hbp_relative_to_pclk_fp;
	int nlanes;
	int bpp;
	int pixelEnc;
	int dsc_en;
	int async_en;
	int bpc;

	unsigned int delay_start_link_extra_pixclk;
	int extra_buffer_margin;
	s64 ratio_fp;
	s64 original_ratio_fp;

	s64 err_fp;
	s64 n_err_fp;
	s64 n_n_err_fp;
	int tu_size;
	int tu_size_desired;
	int tu_size_minus1;

	int valid_boundary_link;
	s64 resulting_valid_fp;
	s64 total_valid_fp;
	s64 effective_valid_fp;
	s64 effective_valid_recorded_fp;
	int n_tus;
	int n_tus_per_lane;
	int paired_tus;
	int remainder_tus;
	int remainder_tus_upper;
	int remainder_tus_lower;
	int extra_bytes;
	int filler_size;
	int delay_start_link;

	int extra_pclk_cycles;
	int extra_pclk_cycles_in_link_clk;
	s64 ratio_by_tu_fp;
	s64 average_valid2_fp;
	int new_valid_boundary_link;
	int remainder_symbols_exist;
	int n_symbols;
	s64 n_remainder_symbols_per_lane_fp;
	s64 last_partial_tu_fp;
	s64 TU_ratio_err_fp;

	int n_tus_incl_last_incomplete_tu;
	int extra_pclk_cycles_tmp;
	int extra_pclk_cycles_in_link_clk_tmp;
	int extra_required_bytes_new_tmp;
	int filler_size_tmp;
	int lower_filler_size_tmp;
	int delay_start_link_tmp;

	bool boundary_moderation_en;
	int boundary_mod_lower_err;
	int upper_boundary_count;
	int lower_boundary_count;
	int i_upper_boundary_count;
	int i_lower_boundary_count;
	int valid_lower_boundary_link;
	int even_distribution_BF;
	int even_distribution_legacy;
	int even_distribution;
	int min_hblank_violated;
	s64 delay_start_time_fp;
	s64 hbp_time_fp;
	s64 hactive_time_fp;
	s64 diff_abs_fp;

	s64 ratio;
};

/* _tu_param_compare: 0 if a==b, 1 if a>b, 2 if a<b */
static int tdp_tu_param_compare(s64 a, s64 b)
{
	u32 a_sign, b_sign;
	s64 a_temp, b_temp, minus_1;

	if (a == b)
		return 0;

	minus_1 = tdp_fixp_from_fraction(-1, 1);

	a_sign = ((a >> 32) & 0x80000000) ? 1 : 0;
	b_sign = ((b >> 32) & 0x80000000) ? 1 : 0;

	if (a_sign > b_sign)
		return 2;
	else if (b_sign > a_sign)
		return 1;

	if (!a_sign && !b_sign) {
		if (a > b)
			return 1;
		else
			return 2;
	} else {
		a_temp = tdp_fixp_mul(a, minus_1);
		b_temp = tdp_fixp_mul(b, minus_1);

		if (a_temp > b_temp)
			return 2;
		else
			return 1;
	}
}

static void tdp_tu_update_timings(struct tdp_tu_input *in,
				  struct tdp_tu_algo *tu)
{
	int nlanes = in->nlanes;
	int dsc_num_slices = in->num_of_dsc_slices;
	int dsc_num_bytes = 0;
	int numerator;
	s64 pclk_dsc_fp;
	s64 dwidth_dsc_fp;
	s64 hbp_dsc_fp;
	int tot_num_eoc_symbols = 0;
	int tot_num_hor_bytes = 0;
	int tot_num_dummy_bytes = 0;
	int dwidth_dsc_bytes = 0;
	int eoc_bytes = 0;
	s64 temp1_fp, temp2_fp, temp3_fp;

	tu->lclk_fp = tdp_fixp_from_fraction(in->lclk, 1);
	tu->pclk_fp = tdp_fixp_from_fraction(in->pclk_khz, 1000);
	tu->lwidth = in->hactive;
	tu->hbp_relative_to_pclk = in->hporch;
	tu->nlanes = in->nlanes;
	tu->bpp = in->bpp;
	tu->pixelEnc = in->pixel_enc;
	tu->dsc_en = in->dsc_en;
	tu->async_en = in->async_en;
	tu->lwidth_fp = tdp_fixp_from_fraction(in->hactive, 1);
	tu->hbp_relative_to_pclk_fp = tdp_fixp_from_fraction(in->hporch, 1);

	if (tu->pixelEnc == 420) {
		temp1_fp = tdp_fixp_from_fraction(2, 1);
		tu->pclk_fp = tdp_fixp_div(tu->pclk_fp, temp1_fp);
		tu->lwidth_fp = tdp_fixp_div(tu->lwidth_fp, temp1_fp);
		/* Matches Linux msm dp_ctrl.c verbatim: literal 2, not temp1_fp. */
		tu->hbp_relative_to_pclk_fp =
			tdp_fixp_div(tu->hbp_relative_to_pclk_fp, 2);
	}

	if (tu->pixelEnc == 422) {
		switch (tu->bpp) {
		case 24:
			tu->bpp = 16;
			tu->bpc = 8;
			break;
		case 30:
			tu->bpp = 20;
			tu->bpc = 10;
			break;
		default:
			tu->bpp = 16;
			tu->bpc = 8;
			break;
		}
	} else {
		tu->bpc = tu->bpp / 3;
	}

	if (!in->dsc_en)
		goto fec_check;

	temp1_fp = tdp_fixp_from_fraction(in->compress_ratio, 100);
	temp2_fp = tdp_fixp_from_fraction(in->bpp, 1);
	temp3_fp = tdp_fixp_div(temp2_fp, temp1_fp);
	temp2_fp = tdp_fixp_mul(tu->lwidth_fp, temp3_fp);

	temp1_fp = tdp_fixp_from_fraction(8, 1);
	temp3_fp = tdp_fixp_div(temp2_fp, temp1_fp);

	numerator = tdp_fixp2int(temp3_fp);

	dsc_num_bytes = dsc_num_slices ? numerator / dsc_num_slices : 0;
	eoc_bytes = dsc_num_bytes % nlanes;
	tot_num_eoc_symbols = nlanes * dsc_num_slices;
	tot_num_hor_bytes = dsc_num_bytes * dsc_num_slices;
	tot_num_dummy_bytes = (nlanes - eoc_bytes) * dsc_num_slices;

	dwidth_dsc_bytes = (tot_num_hor_bytes + tot_num_eoc_symbols +
			    (eoc_bytes == 0 ? 0 : tot_num_dummy_bytes));

	dwidth_dsc_fp = tdp_fixp_from_fraction(dwidth_dsc_bytes, 3);

	temp2_fp = tdp_fixp_mul(tu->pclk_fp, dwidth_dsc_fp);
	temp1_fp = tdp_fixp_div(temp2_fp, tu->lwidth_fp);
	pclk_dsc_fp = temp1_fp;

	temp1_fp = tdp_fixp_div(pclk_dsc_fp, tu->pclk_fp);
	temp2_fp = tdp_fixp_mul(tu->hbp_relative_to_pclk_fp, temp1_fp);
	hbp_dsc_fp = temp2_fp;

	tu->pclk_fp = pclk_dsc_fp;
	tu->lwidth_fp = dwidth_dsc_fp;
	tu->hbp_relative_to_pclk_fp = hbp_dsc_fp;

fec_check:
	if (in->fec_en) {
		temp1_fp = tdp_fixp_from_fraction(976, 1000); /* 0.976 */
		tu->lclk_fp = tdp_fixp_mul(tu->lclk_fp, temp1_fp);
	}
}

static void tdp_tu_valid_boundary_calc(struct tdp_tu_algo *tu)
{
	s64 temp1_fp, temp2_fp, temp, temp1, temp2;
	int compare_result_1, compare_result_2, compare_result_3;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);

	tu->new_valid_boundary_link = tdp_fixp2int_ceil(temp2_fp);

	temp = (tu->i_upper_boundary_count * tu->new_valid_boundary_link +
		tu->i_lower_boundary_count * (tu->new_valid_boundary_link - 1));
	tu->average_valid2_fp = tdp_fixp_from_fraction(temp,
				(tu->i_upper_boundary_count +
				 tu->i_lower_boundary_count));

	temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tu->lwidth_fp;
	temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);
	temp2_fp = tdp_fixp_div(temp1_fp, tu->average_valid2_fp);
	tu->n_tus = tdp_fixp2int(temp2_fp);
	if ((temp2_fp & 0xFFFFFFFF) > 0xFFFFF000)
		tu->n_tus += 1;

	temp1_fp = tdp_fixp_from_fraction(tu->n_tus, 1);
	temp2_fp = tdp_fixp_mul(temp1_fp, tu->average_valid2_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->n_symbols, 1);
	temp2_fp = temp1_fp - temp2_fp;
	temp1_fp = tdp_fixp_from_fraction(tu->nlanes, 1);
	temp2_fp = tdp_fixp_div(temp2_fp, temp1_fp);
	tu->n_remainder_symbols_per_lane_fp = temp2_fp;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	tu->last_partial_tu_fp =
		tdp_fixp_div(tu->n_remainder_symbols_per_lane_fp, temp1_fp);

	if (tu->n_remainder_symbols_per_lane_fp != 0)
		tu->remainder_symbols_exist = 1;
	else
		tu->remainder_symbols_exist = 0;

	temp1_fp = tdp_fixp_from_fraction(tu->n_tus, tu->nlanes);
	tu->n_tus_per_lane = tdp_fixp2int(temp1_fp);

	tu->paired_tus = (int)((tu->n_tus_per_lane) /
			(tu->i_upper_boundary_count + tu->i_lower_boundary_count));

	tu->remainder_tus = tu->n_tus_per_lane - tu->paired_tus *
			(tu->i_upper_boundary_count + tu->i_lower_boundary_count);

	if ((tu->remainder_tus - tu->i_upper_boundary_count) > 0) {
		tu->remainder_tus_upper = tu->i_upper_boundary_count;
		tu->remainder_tus_lower = tu->remainder_tus -
					  tu->i_upper_boundary_count;
	} else {
		tu->remainder_tus_upper = tu->remainder_tus;
		tu->remainder_tus_lower = 0;
	}

	temp = tu->paired_tus * (tu->i_upper_boundary_count *
		tu->new_valid_boundary_link + tu->i_lower_boundary_count *
		(tu->new_valid_boundary_link - 1)) +
		(tu->remainder_tus_upper * tu->new_valid_boundary_link) +
		(tu->remainder_tus_lower * (tu->new_valid_boundary_link - 1));
	tu->total_valid_fp = tdp_fixp_from_fraction(temp, 1);

	if (tu->remainder_symbols_exist) {
		temp1_fp = tu->total_valid_fp +
			   tu->n_remainder_symbols_per_lane_fp;
		temp2_fp = tdp_fixp_from_fraction(tu->n_tus_per_lane, 1);
		temp2_fp = temp2_fp + tu->last_partial_tu_fp;
		temp1_fp = tdp_fixp_div(temp1_fp, temp2_fp);
	} else {
		temp2_fp = tdp_fixp_from_fraction(tu->n_tus_per_lane, 1);
		temp1_fp = tdp_fixp_div(tu->total_valid_fp, temp2_fp);
	}
	tu->effective_valid_fp = temp1_fp;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);
	tu->n_n_err_fp = tu->effective_valid_fp - temp2_fp;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);
	tu->n_err_fp = tu->average_valid2_fp - temp2_fp;

	tu->even_distribution = tu->n_tus % tu->nlanes == 0 ? 1 : 0;

	temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tu->lwidth_fp;
	temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);
	temp2_fp = tdp_fixp_div(temp1_fp, tu->average_valid2_fp);

	if (temp2_fp)
		tu->n_tus_incl_last_incomplete_tu = tdp_fixp2int_ceil(temp2_fp);
	else
		tu->n_tus_incl_last_incomplete_tu = 0;

	temp1 = 0;
	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = tdp_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = tu->average_valid2_fp - temp2_fp;
	temp2_fp = tdp_fixp_from_fraction(tu->n_tus_incl_last_incomplete_tu, 1);
	temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);

	if (temp1_fp)
		temp1 = tdp_fixp2int_ceil(temp1_fp);

	temp = tu->i_upper_boundary_count * tu->nlanes;
	temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
	temp2_fp = tdp_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->new_valid_boundary_link, 1);
	temp2_fp = temp1_fp - temp2_fp;
	temp1_fp = tdp_fixp_from_fraction(temp, 1);
	temp2_fp = tdp_fixp_mul(temp1_fp, temp2_fp);

	if (temp2_fp)
		temp2 = tdp_fixp2int_ceil(temp2_fp);
	else
		temp2 = 0;
	tu->extra_required_bytes_new_tmp = (int)(temp1 + temp2);

	temp1_fp = tdp_fixp_from_fraction(8, tu->bpp);
	temp2_fp = tdp_fixp_from_fraction(tu->extra_required_bytes_new_tmp, 1);
	temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);

	if (temp1_fp)
		tu->extra_pclk_cycles_tmp = tdp_fixp2int_ceil(temp1_fp);
	else
		tu->extra_pclk_cycles_tmp = 0;

	temp1_fp = tdp_fixp_from_fraction(tu->extra_pclk_cycles_tmp, 1);
	temp2_fp = tdp_fixp_div(tu->lclk_fp, tu->pclk_fp);
	temp1_fp = tdp_fixp_mul(temp1_fp, temp2_fp);

	if (temp1_fp)
		tu->extra_pclk_cycles_in_link_clk_tmp =
			tdp_fixp2int_ceil(temp1_fp);
	else
		tu->extra_pclk_cycles_in_link_clk_tmp = 0;

	tu->filler_size_tmp = tu->tu_size - tu->new_valid_boundary_link;
	tu->lower_filler_size_tmp = tu->filler_size_tmp + 1;

	tu->delay_start_link_tmp = tu->extra_pclk_cycles_in_link_clk_tmp +
				   tu->lower_filler_size_tmp +
				   tu->extra_buffer_margin;

	temp1_fp = tdp_fixp_from_fraction(tu->delay_start_link_tmp, 1);
	tu->delay_start_time_fp = tdp_fixp_div(temp1_fp, tu->lclk_fp);

	compare_result_1 = tdp_tu_param_compare(tu->n_n_err_fp, tu->diff_abs_fp);
	if (compare_result_1 == 2)
		compare_result_1 = 1;
	else
		compare_result_1 = 0;

	compare_result_2 = tdp_tu_param_compare(tu->n_n_err_fp, tu->err_fp);
	if (compare_result_2 == 2)
		compare_result_2 = 1;
	else
		compare_result_2 = 0;

	compare_result_3 = tdp_tu_param_compare(tu->hbp_time_fp,
						tu->delay_start_time_fp);
	if (compare_result_3 == 2)
		compare_result_3 = 0;
	else
		compare_result_3 = 1;

	if (((tu->even_distribution == 1) ||
	     ((tu->even_distribution_BF == 0) &&
	      (tu->even_distribution_legacy == 0))) &&
	    tu->n_err_fp >= 0 && tu->n_n_err_fp >= 0 &&
	    compare_result_2 &&
	    (compare_result_1 || (tu->min_hblank_violated == 1)) &&
	    (tu->new_valid_boundary_link - 1) > 0 &&
	    compare_result_3 &&
	    (tu->delay_start_link_tmp <= 1023)) {
		tu->upper_boundary_count = tu->i_upper_boundary_count;
		tu->lower_boundary_count = tu->i_lower_boundary_count;
		tu->err_fp = tu->n_n_err_fp;
		tu->boundary_moderation_en = true;
		tu->tu_size_desired = tu->tu_size;
		tu->valid_boundary_link = tu->new_valid_boundary_link;
		tu->effective_valid_recorded_fp = tu->effective_valid_fp;
		tu->even_distribution_BF = 1;
		tu->delay_start_link = tu->delay_start_link_tmp;
	} else if (tu->boundary_mod_lower_err == 0) {
		compare_result_1 = tdp_tu_param_compare(tu->n_n_err_fp,
							tu->diff_abs_fp);
		if (compare_result_1 == 2)
			tu->boundary_mod_lower_err = 1;
	}
}

static void tdp_dp_calc_tu(struct tdp_tu_input *in, struct tdp_tu_table *tu_table)
{
	struct tdp_tu_algo _tu;
	struct tdp_tu_algo *tu = &_tu;
	int compare_result_1, compare_result_2;
	u64 temp = 0;
	s64 temp_fp = 0, temp1_fp = 0, temp2_fp = 0;

	s64 LCLK_FAST_SKEW_fp = tdp_fixp_from_fraction(6, 10000);	/* 0.0006 */
	s64 const_p49_fp = tdp_fixp_from_fraction(49, 100);		/* 0.49 */
	s64 const_p56_fp = tdp_fixp_from_fraction(56, 100);		/* 0.56 */
	s64 RATIO_SCALE_fp = tdp_fixp_from_fraction(1001, 1000);

	u8 DP_BRUTE_FORCE = 1;
	s64 BRUTE_FORCE_THRESHOLD_fp = tdp_fixp_from_fraction(1, 10);	/* 0.1 */
	unsigned int EXTRA_PIXCLK_CYCLE_DELAY = 4;
	unsigned int HBLANK_MARGIN = 4;

	memset(tu, 0, sizeof(*tu));

	tdp_tu_update_timings(in, tu);

	tu->err_fp = tdp_fixp_from_fraction(1000, 1);

	temp1_fp = tdp_fixp_from_fraction(4, 1);
	temp2_fp = tdp_fixp_mul(temp1_fp, tu->lclk_fp);
	temp_fp = tdp_fixp_div(temp2_fp, tu->pclk_fp);
	tu->extra_buffer_margin = tdp_fixp2int_ceil(temp_fp);

	temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tdp_fixp_mul(tu->pclk_fp, temp1_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->nlanes, 1);
	temp2_fp = tdp_fixp_div(temp2_fp, temp1_fp);
	tu->ratio_fp = tdp_fixp_div(temp2_fp, tu->lclk_fp);

	tu->original_ratio_fp = tu->ratio_fp;
	tu->boundary_moderation_en = false;
	tu->upper_boundary_count = 0;
	tu->lower_boundary_count = 0;
	tu->i_upper_boundary_count = 0;
	tu->i_lower_boundary_count = 0;
	tu->valid_lower_boundary_link = 0;
	tu->even_distribution_BF = 0;
	tu->even_distribution_legacy = 0;
	tu->even_distribution = 0;
	tu->delay_start_time_fp = 0;

	tu->err_fp = tdp_fixp_from_fraction(1000, 1);
	tu->n_err_fp = 0;
	tu->n_n_err_fp = 0;

	tu->ratio = tdp_fixp2int(tu->ratio_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->nlanes, 1);
	tdp_div64_u64_rem(tu->lwidth_fp, temp1_fp, &temp2_fp);
	if (temp2_fp != 0 && !tu->ratio && tu->dsc_en == 0) {
		tu->ratio_fp = tdp_fixp_mul(tu->ratio_fp, RATIO_SCALE_fp);
		tu->ratio = tdp_fixp2int(tu->ratio_fp);
		if (tu->ratio)
			tu->ratio_fp = tdp_fixp_from_fraction(1, 1);
	}

	if (tu->ratio > 1)
		tu->ratio = 1;

	if (tu->ratio == 1)
		goto tu_size_calc;

	compare_result_1 = tdp_tu_param_compare(tu->ratio_fp, const_p49_fp);
	if (!compare_result_1 || compare_result_1 == 1)
		compare_result_1 = 1;
	else
		compare_result_1 = 0;

	compare_result_2 = tdp_tu_param_compare(tu->ratio_fp, const_p56_fp);
	if (!compare_result_2 || compare_result_2 == 2)
		compare_result_2 = 1;
	else
		compare_result_2 = 0;

	if (tu->dsc_en && compare_result_1 && compare_result_2)
		HBLANK_MARGIN += 4;

tu_size_calc:
	for (tu->tu_size = 32; tu->tu_size <= 64; tu->tu_size++) {
		temp1_fp = tdp_fixp_from_fraction(tu->tu_size, 1);
		temp2_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);
		temp = tdp_fixp2int_ceil(temp2_fp);
		temp1_fp = tdp_fixp_from_fraction(temp, 1);
		tu->n_err_fp = temp1_fp - temp2_fp;

		if (tu->n_err_fp < tu->err_fp) {
			tu->err_fp = tu->n_err_fp;
			tu->tu_size_desired = tu->tu_size;
		}
	}

	tu->tu_size_minus1 = tu->tu_size_desired - 1;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
	temp2_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);
	tu->valid_boundary_link = tdp_fixp2int_ceil(temp2_fp);

	temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tu->lwidth_fp;
	temp2_fp = tdp_fixp_mul(temp2_fp, temp1_fp);

	temp1_fp = tdp_fixp_from_fraction(tu->valid_boundary_link, 1);
	temp2_fp = tdp_fixp_div(temp2_fp, temp1_fp);
	tu->n_tus = tdp_fixp2int(temp2_fp);
	if ((temp2_fp & 0xFFFFFFFF) > 0xFFFFF000)
		tu->n_tus += 1;

	tu->even_distribution_legacy = tu->n_tus % tu->nlanes == 0 ? 1 : 0;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
	temp2_fp = tdp_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->valid_boundary_link, 1);
	temp2_fp = temp1_fp - temp2_fp;
	temp1_fp = tdp_fixp_from_fraction(tu->n_tus + 1, 1);
	temp2_fp = tdp_fixp_mul(temp1_fp, temp2_fp);

	temp = tdp_fixp2int(temp2_fp);
	if (temp && temp2_fp)
		tu->extra_bytes = tdp_fixp2int_ceil(temp2_fp);
	else
		tu->extra_bytes = 0;

	temp1_fp = tdp_fixp_from_fraction(tu->extra_bytes, 1);
	temp2_fp = tdp_fixp_from_fraction(8, tu->bpp);
	temp1_fp = tdp_fixp_mul(temp1_fp, temp2_fp);

	if (temp && temp1_fp)
		tu->extra_pclk_cycles = tdp_fixp2int_ceil(temp1_fp);
	else
		tu->extra_pclk_cycles = tdp_fixp2int(temp1_fp);

	temp1_fp = tdp_fixp_div(tu->lclk_fp, tu->pclk_fp);
	temp2_fp = tdp_fixp_from_fraction(tu->extra_pclk_cycles, 1);
	temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);

	if (temp1_fp)
		tu->extra_pclk_cycles_in_link_clk = tdp_fixp2int_ceil(temp1_fp);
	else
		tu->extra_pclk_cycles_in_link_clk = tdp_fixp2int(temp1_fp);

	tu->filler_size = tu->tu_size_desired - tu->valid_boundary_link;

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
	tu->ratio_by_tu_fp = tdp_fixp_mul(tu->ratio_fp, temp1_fp);

	tu->delay_start_link = tu->extra_pclk_cycles_in_link_clk +
			       tu->filler_size + tu->extra_buffer_margin;

	tu->resulting_valid_fp =
		tdp_fixp_from_fraction(tu->valid_boundary_link, 1);

	temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
	temp2_fp = tdp_fixp_div(tu->resulting_valid_fp, temp1_fp);
	tu->TU_ratio_err_fp = temp2_fp - tu->original_ratio_fp;

	temp1_fp = tdp_fixp_from_fraction(HBLANK_MARGIN, 1);
	temp1_fp = tu->hbp_relative_to_pclk_fp - temp1_fp;
	tu->hbp_time_fp = tdp_fixp_div(temp1_fp, tu->pclk_fp);

	temp1_fp = tdp_fixp_from_fraction(tu->delay_start_link, 1);
	tu->delay_start_time_fp = tdp_fixp_div(temp1_fp, tu->lclk_fp);

	compare_result_1 = tdp_tu_param_compare(tu->hbp_time_fp,
						tu->delay_start_time_fp);
	if (compare_result_1 == 2)
		tu->min_hblank_violated = 1;

	tu->hactive_time_fp = tdp_fixp_div(tu->lwidth_fp, tu->pclk_fp);

	compare_result_2 = tdp_tu_param_compare(tu->hactive_time_fp,
						tu->delay_start_time_fp);
	if (compare_result_2 == 2)
		tu->min_hblank_violated = 1;

	tu->delay_start_time_fp = 0;

	tu->delay_start_link_extra_pixclk = EXTRA_PIXCLK_CYCLE_DELAY;
	tu->diff_abs_fp = tu->resulting_valid_fp - tu->ratio_by_tu_fp;

	temp = tdp_fixp2int(tu->diff_abs_fp);
	if (!temp && tu->diff_abs_fp <= 0xffff)
		tu->diff_abs_fp = 0;

	if (tu->diff_abs_fp < 0)
		tu->diff_abs_fp = tdp_fixp_mul(tu->diff_abs_fp, -1);

	tu->boundary_mod_lower_err = 0;
	if ((tu->diff_abs_fp != 0 &&
	     ((tu->diff_abs_fp > BRUTE_FORCE_THRESHOLD_fp) ||
	      (tu->even_distribution_legacy == 0) ||
	      (DP_BRUTE_FORCE == 1))) ||
	    (tu->min_hblank_violated == 1)) {
		do {
			tu->err_fp = tdp_fixp_from_fraction(1000, 1);

			temp1_fp = tdp_fixp_div(tu->lclk_fp, tu->pclk_fp);
			temp2_fp = tdp_fixp_from_fraction(
					tu->delay_start_link_extra_pixclk, 1);
			temp1_fp = tdp_fixp_mul(temp2_fp, temp1_fp);

			if (temp1_fp)
				tu->extra_buffer_margin =
					tdp_fixp2int_ceil(temp1_fp);
			else
				tu->extra_buffer_margin = 0;

			temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
			temp1_fp = tdp_fixp_mul(tu->lwidth_fp, temp1_fp);

			if (temp1_fp)
				tu->n_symbols = tdp_fixp2int_ceil(temp1_fp);
			else
				tu->n_symbols = 0;

			for (tu->tu_size = 32; tu->tu_size <= 64; tu->tu_size++) {
				for (tu->i_upper_boundary_count = 1;
				     tu->i_upper_boundary_count <= 15;
				     tu->i_upper_boundary_count++) {
					for (tu->i_lower_boundary_count = 1;
					     tu->i_lower_boundary_count <= 15;
					     tu->i_lower_boundary_count++) {
						tdp_tu_valid_boundary_calc(tu);
					}
				}
			}
			tu->delay_start_link_extra_pixclk--;
		} while (tu->boundary_moderation_en != true &&
			 tu->boundary_mod_lower_err == 1 &&
			 tu->delay_start_link_extra_pixclk != 0);

		if (tu->boundary_moderation_en == true) {
			temp1_fp = tdp_fixp_from_fraction(
				(tu->upper_boundary_count *
				 tu->valid_boundary_link +
				 tu->lower_boundary_count *
				 (tu->valid_boundary_link - 1)), 1);
			temp2_fp = tdp_fixp_from_fraction(
				(tu->upper_boundary_count +
				 tu->lower_boundary_count), 1);
			tu->resulting_valid_fp = tdp_fixp_div(temp1_fp, temp2_fp);

			temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
			tu->ratio_by_tu_fp =
				tdp_fixp_mul(tu->original_ratio_fp, temp1_fp);

			tu->valid_lower_boundary_link =
				tu->valid_boundary_link - 1;

			temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
			temp1_fp = tdp_fixp_mul(tu->lwidth_fp, temp1_fp);
			temp2_fp = tdp_fixp_div(temp1_fp, tu->resulting_valid_fp);
			tu->n_tus = tdp_fixp2int(temp2_fp);

			tu->tu_size_minus1 = tu->tu_size_desired - 1;
			tu->even_distribution_BF = 1;

			temp1_fp = tdp_fixp_from_fraction(tu->tu_size_desired, 1);
			temp2_fp = tdp_fixp_div(tu->resulting_valid_fp, temp1_fp);
			tu->TU_ratio_err_fp = temp2_fp - tu->original_ratio_fp;
		}
	}

	temp2_fp = tdp_fixp_mul(LCLK_FAST_SKEW_fp, tu->lwidth_fp);

	if (temp2_fp)
		temp = tdp_fixp2int_ceil(temp2_fp);
	else
		temp = 0;

	temp1_fp = tdp_fixp_from_fraction(tu->nlanes, 1);
	temp2_fp = tdp_fixp_mul(tu->original_ratio_fp, temp1_fp);
	temp1_fp = tdp_fixp_from_fraction(tu->bpp, 8);
	temp2_fp = tdp_fixp_div(temp1_fp, temp2_fp);
	temp1_fp = tdp_fixp_from_fraction(temp, 1);
	temp2_fp = tdp_fixp_mul(temp1_fp, temp2_fp);
	temp = tdp_fixp2int(temp2_fp);

	if (tu->async_en)
		tu->delay_start_link += (int)temp;

	temp1_fp = tdp_fixp_from_fraction(tu->delay_start_link, 1);
	tu->delay_start_time_fp = tdp_fixp_div(temp1_fp, tu->lclk_fp);

	tu_table->valid_boundary_link		= tu->valid_boundary_link;
	tu_table->delay_start_link		= tu->delay_start_link;
	tu_table->boundary_moderation_en	= tu->boundary_moderation_en;
	tu_table->valid_lower_boundary_link	= tu->valid_lower_boundary_link;
	tu_table->upper_boundary_count		= tu->upper_boundary_count;
	tu_table->lower_boundary_count		= tu->lower_boundary_count;
	tu_table->tu_size_minus1		= tu->tu_size_minus1;
}

static void tachyon_dp_program_transfer_unit(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	struct tdp_tu_input in;
	struct tdp_tu_table tut;
	u32 tu_reg, vb_reg, vb2_reg;

	if (!priv->rate || !priv->lanes || !t->pixelclock.typ || !t->hactive.typ)
		return;

	memset(&in, 0, sizeof(in));
	memset(&tut, 0, sizeof(tut));

	in.lclk = priv->rate / 1000;		/* link symbol clock: 162/270/540/810 */
	in.pclk_khz = t->pixelclock.typ / 1000;
	in.hactive = t->hactive.typ;
	in.hporch = (u64)t->hfront_porch.typ + t->hsync_len.typ +
		    t->hback_porch.typ;
	in.nlanes = priv->lanes;
	in.bpp = 24;				/* 8bpc RGB */
	in.pixel_enc = 444;
	in.compress_ratio = 100;

	tdp_dp_calc_tu(&in, &tut);

	/* Register packing per msm_dp_ctrl_setup_tr_unit(). */
	tu_reg = tut.tu_size_minus1;
	vb_reg = tut.valid_boundary_link |
		 ((u32)tut.delay_start_link << REG_DP_DELAY_START_LINK_SHIFT);
	vb2_reg = ((u32)tut.valid_lower_boundary_link << 1) |
		  ((u32)tut.upper_boundary_count << 16) |
		  ((u32)tut.lower_boundary_count << 20);
	if (tut.boundary_moderation_en)
		vb2_reg |= BIT(0);

	writel(tu_reg, priv->link + REG_DP_TU);
	writel(vb_reg, priv->link + REG_DP_VALID_BOUNDARY);
	writel(vb2_reg, priv->link + REG_DP_VALID_BOUNDARY_2);

	log_warning("DP TU: %ux%u lanes=%u TU=%02x VB=%08x VB2=%08x (tu_size=%u valid=%u delay=%u mod=%u)\n",
		    t->hactive.typ, t->vactive.typ, priv->lanes,
		    tu_reg, vb_reg, vb2_reg, tut.tu_size_minus1 + 1,
		    tut.valid_boundary_link, tut.delay_start_link,
		    tut.boundary_moderation_en);
}

static void tachyon_dp_program_p0_timing(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	u32 htotal = tachyon_dp_htotal(t);
	u32 vtotal = tachyon_dp_vtotal(t);
	u32 hsync_start = t->hactive.typ + t->hfront_porch.typ;
	u32 hsync_end = hsync_start + t->hsync_len.typ;
	u32 vsync_start = t->vactive.typ + t->vfront_porch.typ;
	u32 display_v_start, display_v_end;
	u32 hsync_start_x, hsync_end_x;
	u32 hsync_ctl, display_hctl;
	bool tpg = tachyon_dp_env_bool("tachyon_dp_tpg");

	if (!priv->p0)
		return;

	/*
	 * The p0 MMSS_DP_INTF_* timing engine is the DP controller's built-in
	 * Test Pattern Generator path (Linux msm_dp_panel_tpg_enable).  For
	 * normal DPU-sourced video the DP controller slaves off the DPU INTF and
	 * Linux NEVER enables this engine; enabling it without a BIST pixel
	 * source (the old behaviour) made the DP TX run an empty internal timing
	 * engine instead of the DPU stream -> trained link but sink "No Signal".
	 * Default: leave it off (DPU INTF drives).  Set tachyon_dp_tpg=1 to emit
	 * the internal checkered pattern (proves DP/PHY/dock/monitor end to end).
	 */
	if (!tpg)
		return;

	display_v_start = ((vtotal - vsync_start) * htotal) +
			  (htotal - hsync_start);
	display_v_end = ((vtotal - (vsync_start - t->vactive.typ)) *
			 htotal) - 1;
	display_v_end -= hsync_start - t->hactive.typ;
	hsync_start_x = htotal - hsync_start;
	hsync_end_x = htotal - (hsync_start - t->hactive.typ) - 1;
	hsync_ctl = (htotal << 16) | (hsync_end - hsync_start);
	display_hctl = (hsync_end_x << 16) | hsync_start_x;

	writel(hsync_ctl, priv->p0 + MMSS_DP_INTF_HSYNC_CTL);
	writel(vtotal * htotal, priv->p0 + MMSS_DP_INTF_VSYNC_PERIOD_F0);
	writel(t->vsync_len.typ * htotal,
	       priv->p0 + MMSS_DP_INTF_VSYNC_PULSE_WIDTH_F0);
	writel(0, priv->p0 + MMSS_DP_INTF_VSYNC_PERIOD_F1);
	writel(0, priv->p0 + MMSS_DP_INTF_VSYNC_PULSE_WIDTH_F1);
	writel(display_hctl, priv->p0 + MMSS_DP_INTF_DISPLAY_HCTL);
	writel(0, priv->p0 + MMSS_DP_INTF_ACTIVE_HCTL);
	writel(display_v_start, priv->p0 + MMSS_INTF_DISPLAY_V_START_F0);
	writel(display_v_end, priv->p0 + MMSS_DP_INTF_DISPLAY_V_END_F0);
	writel(0, priv->p0 + MMSS_INTF_DISPLAY_V_START_F1);
	writel(0, priv->p0 + MMSS_DP_INTF_DISPLAY_V_END_F1);
	writel(0, priv->p0 + MMSS_DP_INTF_ACTIVE_V_START_F0);
	writel(0, priv->p0 + MMSS_DP_INTF_ACTIVE_V_END_F0);
	writel(0, priv->p0 + MMSS_DP_INTF_ACTIVE_V_START_F1);
	writel(0, priv->p0 + MMSS_DP_INTF_ACTIVE_V_END_F1);
	writel(0, priv->p0 + MMSS_DP_INTF_POLARITY_CTL);
	writel(readl(priv->p0 + MMSS_DP_INTF_CONFIG),
	       priv->p0 + MMSS_DP_INTF_CONFIG);
	/* BIST pixel source (checkered) -> then arm the p0 timing engine. */
	writel(DP_TPG_CHECKERED_RECT_PATTERN,
	       priv->p0 + MMSS_DP_TPG_MAIN_CONTROL);
	writel(DP_TPG_VIDEO_CONFIG_BPP_8BIT | DP_TPG_VIDEO_CONFIG_RGB,
	       priv->p0 + MMSS_DP_TPG_VIDEO_CONFIG);
	writel(DP_BIST_ENABLE_DPBIST_EN, priv->p0 + MMSS_DP_BIST_ENABLE);
	writel(DP_TIMING_ENGINE_EN_EN, priv->p0 + MMSS_DP_TIMING_ENGINE_EN);
	log_warning("DP TPG checkered pattern ON (p0 timing engine + BIST)\n");
}

void tachyon_dp_program_video_timing(struct tachyon_dp_priv *priv)
{
	tachyon_dp_program_pixel_clock(priv);
	tachyon_dp_program_msa_timing(priv);
	tachyon_dp_program_msa_clock(priv);
	tachyon_dp_program_transfer_unit(priv);
	tachyon_dp_program_p0_timing(priv);
}

void tachyon_dp_controller_quiesce(struct tachyon_dp_priv *priv)
{
	if (!priv->dp_core_clocks_enabled)
		return;

	log_warning("DP controller quiesce before OS handoff\n");

	/*
	 * Only tear down the link/mainlink if it was actually brought up.
	 * Touching the link registers (STATE_CTRL / MAINLINK_CTRL) when the DP
	 * link was never trained (e.g. AUX/EDID failed) faults the DP controller
	 * and hard-resets the SoC.
	 */
	if (priv->link && priv->dp_link_up) {
		writel(0, priv->link + REG_DP_STATE_CTRL);
		tachyon_dp_mainlink_disable(priv);
	}

	if (priv->aux) {
		writel(DP_AUX_CTRL_RESET, priv->aux + REG_DP_AUX_CTRL);
		udelay(100);
		writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
		writel(0, priv->aux + REG_DP_AUX_CTRL);
	}

	if (priv->ctrl) {
		writel(DP_SW_RESET, priv->ctrl + REG_DP_SW_RESET);
		udelay(100);
		writel(0, priv->ctrl + REG_DP_SW_RESET);
	}
}
