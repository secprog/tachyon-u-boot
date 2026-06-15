// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort link policy + training status: rate/lane policy,
 * BW-code mapping, clock-recovery / equalization / symbol-lock checks, and
 * voltage-swing / pre-emphasis adjustment. Split out of qcom_tachyon_dp.c;
 * operates on the shared struct tachyon_dp_priv.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include "qcom_tachyon_dp.h"

void tachyon_dp_reset_link_policy(struct tachyon_dp_priv *priv)
{
	if (!tachyon_dp_typec_state_valid(priv)) {
		log_warning("DP cannot compute link policy without valid Type-C Alt Mode state\n");
		priv->max_lanes = 0;
		priv->lanes = 0;
		priv->max_rate = 0;
		priv->rate = 0;
		return;
	}

	priv->max_lanes = min_t(u8, priv->graph_lanes ?: 1,
				tachyon_dp_pin_assignment_lanes(priv));

	/*
	 * Optional hard override for bring-up: tachyon_dp_force_lanes caps the
	 * trained lane count (1-4) regardless of pin/sink, so a flaky high lane
	 * can be ruled out without a rebuild.
	 */
	if (tachyon_dp_env_has_u32("tachyon_dp_force_lanes")) {
		u8 fl = tachyon_dp_env_u32("tachyon_dp_force_lanes",
					   priv->max_lanes);

		if (fl >= 1 && fl <= 4)
			priv->max_lanes = min_t(u8, priv->max_lanes, fl);
	}

	priv->lanes = min_t(u8, priv->caps.lanes ?: 1, priv->max_lanes);
	priv->max_rate = min(priv->caps.max_rate,
			     tachyon_dp_env_u32("tachyon_dp_max_rate",
						DP_LINK_RATE_HBR2));
	priv->rate = priv->max_rate;
}

u8 tachyon_dp_bw_code(u32 rate)
{
	if (rate >= DP_LINK_RATE_HBR3)
		return DP_LINK_BW_8_1;
	if (rate >= DP_LINK_RATE_HBR2)
		return DP_LINK_BW_5_4;
	if (rate >= DP_LINK_RATE_HBR)
		return DP_LINK_BW_2_7;
	return DP_LINK_BW_1_62;
}

void tachyon_dp_dump_link_state(struct tachyon_dp_priv *priv,
				const char *tag)
{
	log_warning("DP LINK %s: STATE_CTRL=%08x MAINLINK_CTRL=%08x "
		    "MAINLINK_READY=%08x CONFIG_CTRL=%08x "
		    "SOFTWARE_MVID=%08x SOFTWARE_NVID=%08x TOTAL_HOR_VER=%08x "
		    "START_HOR_VER=%08x ACTIVE_HOR_VER=%08x POLARITY=%08x "
		    "MISC1_MISC0=%08x VALID_BOUNDARY=%08x VALID_BOUNDARY2=%08x "
		    "TU=%08x\n",
		    tag,
		    readl(priv->link + REG_DP_STATE_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_READY),
		    readl(priv->link + REG_DP_CONFIGURATION_CTRL),
		    readl(priv->link + REG_DP_SOFTWARE_MVID),
		    readl(priv->link + REG_DP_SOFTWARE_NVID),
		    readl(priv->link + REG_DP_TOTAL_HOR_VER),
		    readl(priv->link + REG_DP_START_HOR_VER_FROM_SYNC),
		    readl(priv->link + REG_DP_ACTIVE_HOR_VER),
		    readl(priv->link + REG_DP_HSYNC_VSYNC_WIDTH_POLARITY),
		    readl(priv->link + REG_DP_MISC1_MISC0),
		    readl(priv->link + REG_DP_VALID_BOUNDARY),
		    readl(priv->link + REG_DP_VALID_BOUNDARY_2),
		    readl(priv->link + REG_DP_TU));
}

bool tachyon_dp_cr_done(u8 *status, u8 lanes)
{
	u8 lane;

	for (lane = 0; lane < lanes; lane++) {
		if (!(status[lane >> 1] & (DP_LANE_CR_DONE << ((lane & 1) * 4))))
			return false;
	}

	return true;
}

bool tachyon_dp_eq_done(u8 *status, u8 lanes)
{
	u8 lane;

	if (!(status[2] & DP_INTERLANE_ALIGN_DONE))
		return false;

	for (lane = 0; lane < lanes; lane++) {
		u8 bits = status[lane >> 1] >> ((lane & 1) * 4);

		if ((bits & (DP_LANE_CR_DONE | DP_LANE_CHANNEL_EQ_DONE |
			     DP_LANE_SYMBOL_LOCKED)) !=
		    (DP_LANE_CR_DONE | DP_LANE_CHANNEL_EQ_DONE |
		     DP_LANE_SYMBOL_LOCKED))
			return false;
	}

	return true;
}

u8 tachyon_dp_train_set(struct tachyon_dp_priv *priv, u8 lane)
{
	u8 val = priv->swing[lane] | (priv->pre[lane] << 3);

	if (priv->swing[lane] >= 3)
		val |= DP_TRAIN_MAX_SWING_REACHED;
	if (priv->pre[lane] >= 3)
		val |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

	return val;
}

void tachyon_dp_apply_adjust(struct tachyon_dp_priv *priv, u8 *adj)
{
	u8 lane;

	for (lane = 0; lane < priv->lanes; lane++) {
		u8 raw = adj[lane >> 1] >> ((lane & 1) * 4);

		priv->swing[lane] = min_t(u8, raw & 0x3, 3);
		priv->pre[lane] = min_t(u8, (raw >> 2) & 0x3, 3);
		if (priv->swing[lane] + priv->pre[lane] > 3)
			priv->pre[lane] = 3 - priv->swing[lane];
	}
}

/* Read + log the sink/branch DPCD lane status, to bracket where the link drops. */
void tachyon_dp_log_lanes(struct tachyon_dp_priv *priv, const char *when)
{
	u8 l01 = 0, l23 = 0, align = 0;

	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE0_1_STATUS, &l01, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE2_3_STATUS, &l23, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE_ALIGN_STATUS, &align, 1);
	log_warning("DP lanes @ %s: L01=%02x L23=%02x ALIGN=%02x\n",
		    when, l01, l23, align);
}
