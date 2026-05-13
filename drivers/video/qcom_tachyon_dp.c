// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal Qualcomm MSM DisplayPort bring-up for Particle Tachyon.
 *
 * This is intentionally board-scoped: it consumes the Tachyon USB-C/DP graph
 * and the SC7280 DP register layout used by QCM6490, then exposes the selected
 * framebuffer through U-Boot's video uclass so EFI GOP uses the same buffer.
 */

#define LOG_CATEGORY UCLASS_VIDEO

#include <asm/gpio.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/read.h>
#include <dm/ofnode.h>
#include <env.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <video.h>

#define TACHYON_DP_DEFAULT_XRES		1920
#define TACHYON_DP_DEFAULT_YRES		1080
#define TACHYON_DP_FB_ALIGN		SZ_1M

#define DP_LINK_RATE_RBR		162000
#define DP_LINK_RATE_HBR		270000
#define DP_LINK_RATE_HBR2		540000
#define DP_LINK_RATE_HBR3		810000

#define DP_LINK_BW_SET			0x100
#define DP_TRAINING_PATTERN_SET		0x102
#define DP_TRAINING_LANE0_SET		0x103
#define DP_DOWNSPREAD_CTRL		0x107
#define DP_MAIN_LINK_CHANNEL_CODING_SET	0x108
#define DP_LANE0_1_STATUS		0x202
#define DP_DPCD_REV			0x000
#define DP_MAX_LINK_RATE		0x001
#define DP_MAX_LANE_COUNT		0x002
#define DP_ENHANCED_FRAME_CAP		BIT(7)

#define DP_LINK_BW_1_62			0x06
#define DP_LINK_BW_2_7			0x0a
#define DP_LINK_BW_5_4			0x14
#define DP_LINK_BW_8_1			0x1e

#define DP_TRAINING_PATTERN_DISABLE	0
#define DP_TRAINING_PATTERN_1		1
#define DP_TRAINING_PATTERN_2		2
#define DP_LINK_SCRAMBLING_DISABLE	BIT(5)
#define DP_TRAIN_MAX_SWING_REACHED	BIT(2)
#define DP_TRAIN_MAX_PRE_EMPHASIS_REACHED BIT(5)

#define DP_LANE_CR_DONE			BIT(0)
#define DP_LANE_CHANNEL_EQ_DONE		BIT(1)
#define DP_LANE_SYMBOL_LOCKED		BIT(2)
#define DP_INTERLANE_ALIGN_DONE		BIT(0)

#define REG_DP_SW_RESET			0x010
#define DP_SW_RESET			BIT(0)
#define REG_DP_AUX_CTRL			0x030
#define DP_AUX_CTRL_ENABLE		BIT(0)
#define DP_AUX_CTRL_RESET		BIT(1)
#define REG_DP_AUX_DATA			0x034
#define DP_AUX_DATA_READ		BIT(0)
#define DP_AUX_DATA_OFFSET		8
#define DP_AUX_DATA_MASK		0x0000ff00
#define DP_AUX_DATA_INDEX_WRITE		BIT(31)
#define REG_DP_AUX_TRANS_CTRL		0x038
#define DP_AUX_TRANS_CTRL_I2C		BIT(8)
#define DP_AUX_TRANS_CTRL_GO		BIT(9)
#define REG_DP_TIMEOUT_COUNT		0x03c
#define REG_DP_AUX_LIMITS		0x040
#define REG_DP_AUX_STATUS		0x044

#define REG_DP_MAINLINK_CTRL		0x000
#define DP_MAINLINK_CTRL_ENABLE		BIT(0)
#define REG_DP_STATE_CTRL		0x004
#define DP_STATE_CTRL_LINK_TRAINING_PATTERN1 BIT(0)
#define DP_STATE_CTRL_LINK_TRAINING_PATTERN2 BIT(1)
#define DP_STATE_CTRL_SEND_VIDEO	BIT(7)
#define REG_DP_CONFIGURATION_CTRL	0x008
#define DP_CONFIGURATION_CTRL_SYNC_ASYNC_CLK BIT(0)
#define DP_CONFIGURATION_CTRL_STATIC_DYNAMIC_CN BIT(1)
#define DP_CONFIGURATION_CTRL_P_INTERLACED BIT(2)
#define DP_CONFIGURATION_CTRL_NUM_OF_LANES_SHIFT 4
#define DP_CONFIGURATION_CTRL_ENHANCED_FRAMING BIT(6)
#define DP_CONFIGURATION_CTRL_BPC_SHIFT	8
#define DP_CONFIGURATION_CTRL_LSCLK_DIV_SHIFT 13
#define REG_DP_LOGICAL2PHYSICAL_LANE_MAPPING 0x038
#define REG_DP_MAINLINK_READY		0x040
#define DP_MAINLINK_READY_FOR_VIDEO	BIT(0)

#define QMP_V3_DP_COM_PHY_MODE_CTRL	0x000
#define QMP_V3_DP_COM_TYPEC_CTRL	0x010
#define QMP_DP_COM_USB3_MODE		BIT(0)
#define QMP_DP_COM_DP_MODE		BIT(1)
#define QMP_DP_COM_SW_PORTSELECT_VAL	BIT(0)
#define QMP_DP_COM_SW_PORTSELECT_MUX	BIT(1)

#define QMP_OFF_DP_PHY			0x2a00
#define QMP_DP_PHY_CFG			0x010
#define QMP_V4_DP_PHY_CFG_1		0x014
#define QMP_DP_PHY_PD_CTL		0x018
#define QMP_DP_PHY_MODE			0x01c
#define QMP_DP_PHY_AUX_CFG0		0x020
#define QMP_DP_PHY_AUX_CFG1		0x024
#define QMP_DP_PHY_AUX_CFG2		0x028
#define QMP_DP_PHY_AUX_CFG3		0x02c
#define QMP_DP_PHY_AUX_CFG4		0x030
#define QMP_DP_PHY_AUX_CFG5		0x034
#define QMP_DP_PHY_AUX_CFG6		0x038
#define QMP_DP_PHY_AUX_CFG7		0x03c
#define QMP_DP_PHY_AUX_CFG8		0x040
#define QMP_DP_PHY_AUX_CFG9		0x044
#define QMP_V4_DP_PHY_VCO_DIV		0x070
#define QMP_V4_DP_PHY_TX0_TX1_LANE_CTL	0x078
#define QMP_V4_DP_PHY_TX2_TX3_LANE_CTL	0x09c
#define QMP_V4_DP_PHY_STATUS		0x0dc
#define QMP_DP_PHY_PD_CTL_PWRDN		BIT(0)
#define QMP_DP_PHY_PD_CTL_PSR_PWRDN	BIT(1)
#define QMP_DP_PHY_PD_CTL_AUX_PWRDN	BIT(2)
#define QMP_DP_PHY_PD_CTL_LANE_2_3_PWRDN BIT(4)
#define QMP_DP_PHY_PD_CTL_PLL_PWRDN	BIT(5)
#define QMP_DP_PHY_PD_CTL_DP_CLAMP_EN	BIT(6)

#define TACHYON_DP_AUX_DEBOUNCE_TRIES	20

enum tachyon_dp_orientation {
	TACHYON_DP_ORIENTATION_NORMAL,
	TACHYON_DP_ORIENTATION_REVERSE,
};

struct tachyon_dp_caps {
	u8 dpcd_rev;
	u32 max_rate;
	u8 lanes;
	bool enhanced;
};

struct tachyon_dp_priv {
	void __iomem *ctrl;
	void __iomem *aux;
	void __iomem *link;
	void __iomem *phy;
	void __iomem *phy_dp;
	struct gpio_desc sbu_enable;
	struct gpio_desc sbu_select;
	struct tachyon_dp_caps caps;
	enum tachyon_dp_orientation orientation;
	u8 pin_assignment;
	u32 rate;
	u8 lanes;
	u8 swing[4];
	u8 pre[4];
	u32 aux_timeouts;
	u32 aux_nacks;
	u32 aux_retries;
};

static u32 tachyon_dp_env_u32(const char *name, u32 fallback)
{
	const char *val = env_get(name);
	char *end;
	ulong parsed;

	if (!val || !*val)
		return fallback;

	parsed = simple_strtoul(val, &end, 0);
	if (end == val)
		return fallback;

	return parsed;
}

static bool tachyon_dp_altmode_ready(struct tachyon_dp_priv *priv)
{
	const char *altmode = env_get("tachyon_dp_altmode");

	/*
	 * U-Boot does not yet have a QRTR/PMIC-GLINK transport, so the policy
	 * source is an explicit board handoff/env flag. Keep the gate hard:
	 * without confirmed DP mode, do not touch the DP controller.
	 */
	if (!altmode || strcmp(altmode, "dp"))
		return false;

	priv->orientation = tachyon_dp_env_u32("tachyon_dp_orientation", 0) ?
			    TACHYON_DP_ORIENTATION_REVERSE :
			    TACHYON_DP_ORIENTATION_NORMAL;
	priv->pin_assignment = tachyon_dp_env_u32("tachyon_dp_pin_assignment", 2);

	return true;
}

static int tachyon_dp_request_sbu_mux(struct tachyon_dp_priv *priv)
{
	ofnode mux = ofnode_path("/usb1-sbu-mux");
	int ret;

	if (!ofnode_valid(mux))
		return -ENOENT;

	ret = gpio_request_by_name_nodev(mux, "enable-gpios", 0,
					 &priv->sbu_enable, GPIOD_IS_OUT);
	if (ret)
		return ret;

	ret = gpio_request_by_name_nodev(mux, "select-gpios", 0,
					 &priv->sbu_select, GPIOD_IS_OUT);
	if (ret)
		return ret;

	return 0;
}

static void tachyon_dp_program_sbu_mux(struct tachyon_dp_priv *priv)
{
	if (dm_gpio_is_valid(&priv->sbu_select))
		dm_gpio_set_value(&priv->sbu_select,
				  priv->orientation ==
				  TACHYON_DP_ORIENTATION_REVERSE);

	udelay(1000);

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, 1);
}

static int tachyon_dp_find_phy(struct udevice *dev, struct tachyon_dp_priv *priv)
{
	struct ofnode_phandle_args args;
	fdt_addr_t addr;
	fdt_size_t size;
	int ret;

	ret = dev_read_phandle_with_args(dev, "phys", "#phy-cells", 0, 0, &args);
	if (ret)
		return ret;

	addr = ofnode_get_addr_size(args.node, "reg", &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->phy = map_sysmem(addr, size);
	priv->phy_dp = (void __iomem *)((u8 __iomem *)priv->phy + QMP_OFF_DP_PHY);

	return 0;
}

static int tachyon_dp_read_poll(void __iomem *base, u32 reg, u32 mask,
				u32 value, u32 timeout_us)
{
	u32 status;

	while (timeout_us--) {
		status = readl(base + reg);
		if ((status & mask) == value)
			return 0;
		udelay(1);
	}

	return -ETIMEDOUT;
}

static void tachyon_dp_qmp_aux_init(struct tachyon_dp_priv *priv)
{
	writel(QMP_DP_PHY_PD_CTL_PWRDN | QMP_DP_PHY_PD_CTL_PSR_PWRDN |
	       QMP_DP_PHY_PD_CTL_AUX_PWRDN | QMP_DP_PHY_PD_CTL_PLL_PWRDN |
	       QMP_DP_PHY_PD_CTL_DP_CLAMP_EN,
	       priv->phy_dp + QMP_DP_PHY_PD_CTL);

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

static int tachyon_dp_qmp_configure(struct tachyon_dp_priv *priv)
{
	u32 mode = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE ?
		   0x4c : 0x5c;
	u32 vco_div;
	int ret;

	setbits_le32(priv->phy + QMP_V3_DP_COM_PHY_MODE_CTRL, QMP_DP_COM_DP_MODE);
	clrbits_le32(priv->phy + QMP_V3_DP_COM_PHY_MODE_CTRL, QMP_DP_COM_USB3_MODE);

	clrsetbits_le32(priv->phy + QMP_V3_DP_COM_TYPEC_CTRL,
			QMP_DP_COM_SW_PORTSELECT_VAL | QMP_DP_COM_SW_PORTSELECT_MUX,
			QMP_DP_COM_SW_PORTSELECT_MUX |
			(priv->orientation == TACHYON_DP_ORIENTATION_REVERSE ?
			 QMP_DP_COM_SW_PORTSELECT_VAL : 0));

	tachyon_dp_qmp_aux_init(priv);

	writel(0x0f, priv->phy_dp + QMP_V4_DP_PHY_CFG_1);
	writel(mode, priv->phy_dp + QMP_DP_PHY_MODE);
	writel(0x13, priv->phy_dp + QMP_DP_PHY_AUX_CFG1);
	writel(0xa4, priv->phy_dp + QMP_DP_PHY_AUX_CFG2);
	writel(priv->lanes > 2 ? 0x05 : 0x01,
	       priv->phy_dp + QMP_V4_DP_PHY_TX0_TX1_LANE_CTL);
	writel(priv->lanes > 2 ? 0x05 : 0x00,
	       priv->phy_dp + QMP_V4_DP_PHY_TX2_TX3_LANE_CTL);

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
	writel(vco_div, priv->phy_dp + QMP_V4_DP_PHY_VCO_DIV);

	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x05, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x09, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x19, priv->phy_dp + QMP_DP_PHY_CFG);

	ret = tachyon_dp_read_poll(priv->phy_dp, QMP_V4_DP_PHY_STATUS,
				   BIT(0) | BIT(1), BIT(0) | BIT(1), 10000);
	if (ret)
		log_warning("DP PHY did not report full lock: %d\n", ret);

	return ret;
}

static void tachyon_dp_aux_hw_init(struct tachyon_dp_priv *priv)
{
	writel(DP_AUX_CTRL_RESET, priv->aux + REG_DP_AUX_CTRL);
	udelay(1000);
	writel(DP_AUX_CTRL_ENABLE, priv->aux + REG_DP_AUX_CTRL);
	writel(0xffff, priv->aux + REG_DP_TIMEOUT_COUNT);
	writel(0xffff, priv->aux + REG_DP_AUX_LIMITS);
}

static int tachyon_dp_aux_transfer(struct tachyon_dp_priv *priv, bool i2c,
				   bool read, u32 addr, u8 *buf, size_t len)
{
	u8 hdr[4];
	u32 ctrl, reg;
	size_t i;

	if (!len || len > 16)
		return -EINVAL;

	hdr[0] = (addr >> 16) & 0xf;
	if (read)
		hdr[0] |= BIT(4);
	hdr[1] = addr >> 8;
	hdr[2] = addr;
	hdr[3] = len - 1;

	writel(DP_AUX_DATA_INDEX_WRITE, priv->aux + REG_DP_AUX_DATA);

	for (i = 0; i < sizeof(hdr); i++) {
		reg = DP_AUX_DATA_INDEX_WRITE | ((u32)hdr[i] << DP_AUX_DATA_OFFSET);
		writel(reg, priv->aux + REG_DP_AUX_DATA);
	}

	if (!read) {
		for (i = 0; i < len; i++) {
			reg = DP_AUX_DATA_INDEX_WRITE | ((u32)buf[i] << DP_AUX_DATA_OFFSET);
			writel(reg, priv->aux + REG_DP_AUX_DATA);
		}
	}

	ctrl = DP_AUX_TRANS_CTRL_GO;
	if (i2c)
		ctrl |= DP_AUX_TRANS_CTRL_I2C;

	writel(ctrl, priv->aux + REG_DP_AUX_TRANS_CTRL);

	for (i = 0; i < 250; i++) {
		reg = readl(priv->aux + REG_DP_AUX_STATUS);
		if (!(readl(priv->aux + REG_DP_AUX_TRANS_CTRL) & DP_AUX_TRANS_CTRL_GO))
			break;
		udelay(1000);
	}

	if (i == 250) {
		priv->aux_timeouts++;
		return -ETIMEDOUT;
	}

	if (reg & GENMASK(7, 4)) {
		priv->aux_nacks++;
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

static int tachyon_dp_aux_retry(struct tachyon_dp_priv *priv, bool i2c,
				bool read, u32 addr, u8 *buf, size_t len)
{
	int ret, retry;

	for (retry = 0; retry < 5; retry++) {
		ret = tachyon_dp_aux_transfer(priv, i2c, read, addr, buf, len);
		if (!ret)
			return 0;
		priv->aux_retries++;
		udelay(4000);
	}

	return ret;
}

static int tachyon_dp_read_dpcd_caps(struct tachyon_dp_priv *priv)
{
	u8 dpcd[16];
	int ret;

	ret = tachyon_dp_aux_retry(priv, false, true, DP_DPCD_REV, dpcd,
				   sizeof(dpcd));
	if (ret)
		return ret;

	priv->caps.dpcd_rev = dpcd[0];
	priv->caps.lanes = dpcd[DP_MAX_LANE_COUNT] & 0x1f;
	priv->caps.enhanced = dpcd[DP_MAX_LANE_COUNT] & DP_ENHANCED_FRAME_CAP;

	switch (dpcd[DP_MAX_LINK_RATE]) {
	case DP_LINK_BW_8_1:
		priv->caps.max_rate = DP_LINK_RATE_HBR3;
		break;
	case DP_LINK_BW_5_4:
		priv->caps.max_rate = DP_LINK_RATE_HBR2;
		break;
	case DP_LINK_BW_2_7:
		priv->caps.max_rate = DP_LINK_RATE_HBR;
		break;
	default:
		priv->caps.max_rate = DP_LINK_RATE_RBR;
		break;
	}

	priv->lanes = min_t(u8, priv->caps.lanes ?: 1,
			    priv->pin_assignment == 2 ? 4 : 2);
	priv->rate = min(priv->caps.max_rate,
			 tachyon_dp_env_u32("tachyon_dp_max_rate",
					    DP_LINK_RATE_HBR2));

	log_info("DP sink DPCD rev=%02x max_rate=%u lanes=%u enhanced=%d\n",
		 priv->caps.dpcd_rev, priv->caps.max_rate, priv->caps.lanes,
		 priv->caps.enhanced);

	return 0;
}

static u8 tachyon_dp_bw_code(u32 rate)
{
	if (rate >= DP_LINK_RATE_HBR3)
		return DP_LINK_BW_8_1;
	if (rate >= DP_LINK_RATE_HBR2)
		return DP_LINK_BW_5_4;
	if (rate >= DP_LINK_RATE_HBR)
		return DP_LINK_BW_2_7;
	return DP_LINK_BW_1_62;
}

static bool tachyon_dp_cr_done(u8 *status, u8 lanes)
{
	u8 lane;

	for (lane = 0; lane < lanes; lane++) {
		if (!(status[lane >> 1] & (DP_LANE_CR_DONE << ((lane & 1) * 4))))
			return false;
	}

	return true;
}

static bool tachyon_dp_eq_done(u8 *status, u8 lanes)
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

static u8 tachyon_dp_train_set(struct tachyon_dp_priv *priv, u8 lane)
{
	u8 val = priv->swing[lane] | (priv->pre[lane] << 3);

	if (priv->swing[lane] >= 3)
		val |= DP_TRAIN_MAX_SWING_REACHED;
	if (priv->pre[lane] >= 3)
		val |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

	return val;
}

static void tachyon_dp_apply_adjust(struct tachyon_dp_priv *priv, u8 *adj)
{
	u8 lane;

	for (lane = 0; lane < priv->lanes; lane++) {
		u8 raw = adj[lane >> 1] >> ((lane & 1) * 4);

		priv->swing[lane] = min_t(u8, raw & 0x3, 3);
		priv->pre[lane] = min_t(u8, (raw >> 2) & 0x3, 3);
	}
}

static int tachyon_dp_program_training_set(struct tachyon_dp_priv *priv)
{
	u8 training[4] = {};
	u8 i;

	for (i = 0; i < priv->lanes; i++)
		training[i] = tachyon_dp_train_set(priv, i);

	return tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_LANE0_SET,
				    training, priv->lanes);
}

static int tachyon_dp_link_train_at(struct tachyon_dp_priv *priv, u32 rate,
				    u8 lanes)
{
	u8 link[2], pattern, status[6], adj[2];
	int ret, tries;

	priv->rate = rate;
	priv->lanes = lanes;
	memset(priv->swing, 0, sizeof(priv->swing));
	memset(priv->pre, 0, sizeof(priv->pre));

	ret = tachyon_dp_qmp_configure(priv);
	if (ret)
		return ret;

	link[0] = tachyon_dp_bw_code(rate);
	link[1] = lanes | (priv->caps.enhanced ? DP_ENHANCED_FRAME_CAP : 0);
	ret = tachyon_dp_aux_retry(priv, false, false, DP_LINK_BW_SET, link,
				   sizeof(link));
	if (ret)
		return ret;

	pattern = 0;
	tachyon_dp_aux_retry(priv, false, false, DP_DOWNSPREAD_CTRL,
			     &pattern, 1);
	pattern = 1;
	tachyon_dp_aux_retry(priv, false, false,
			     DP_MAIN_LINK_CHANNEL_CODING_SET, &pattern, 1);

	pattern = DP_TRAINING_PATTERN_1 | DP_LINK_SCRAMBLING_DISABLE;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
				   &pattern, 1);
	if (ret)
		return ret;

	writel(DP_STATE_CTRL_LINK_TRAINING_PATTERN1, priv->link + REG_DP_STATE_CTRL);

	for (tries = 0; tries < 5; tries++) {
		ret = tachyon_dp_program_training_set(priv);
		if (ret)
			return ret;
		udelay(10000);
		ret = tachyon_dp_aux_retry(priv, false, true,
					   DP_LANE0_1_STATUS, status, 6);
		if (ret)
			return ret;
		if (tachyon_dp_cr_done(status, lanes))
			break;
		memcpy(adj, &status[4], sizeof(adj));
		tachyon_dp_apply_adjust(priv, adj);
	}
	if (tries == 5)
		return -EIO;

	pattern = DP_TRAINING_PATTERN_2 | DP_LINK_SCRAMBLING_DISABLE;
	ret = tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
				   &pattern, 1);
	if (ret)
		return ret;

	writel(DP_STATE_CTRL_LINK_TRAINING_PATTERN2, priv->link + REG_DP_STATE_CTRL);

	for (tries = 0; tries < 5; tries++) {
		ret = tachyon_dp_program_training_set(priv);
		if (ret)
			return ret;
		udelay(10000);
		ret = tachyon_dp_aux_retry(priv, false, true,
					   DP_LANE0_1_STATUS, status, 6);
		if (ret)
			return ret;
		if (tachyon_dp_eq_done(status, lanes))
			break;
		memcpy(adj, &status[4], sizeof(adj));
		tachyon_dp_apply_adjust(priv, adj);
	}
	if (tries == 5)
		return -EIO;

	pattern = DP_TRAINING_PATTERN_DISABLE;
	tachyon_dp_aux_retry(priv, false, false, DP_TRAINING_PATTERN_SET,
			     &pattern, 1);
	writel(DP_STATE_CTRL_SEND_VIDEO, priv->link + REG_DP_STATE_CTRL);

	return 0;
}

static int tachyon_dp_link_train(struct tachyon_dp_priv *priv)
{
	static const u32 rates[] = {
		DP_LINK_RATE_HBR3, DP_LINK_RATE_HBR2, DP_LINK_RATE_HBR,
		DP_LINK_RATE_RBR,
	};
	static const u8 lane_counts[] = { 4, 2, 1 };
	int r, l, ret = -EIO;

	for (r = 0; r < ARRAY_SIZE(rates); r++) {
		if (rates[r] > priv->rate)
			continue;
		for (l = 0; l < ARRAY_SIZE(lane_counts); l++) {
			if (lane_counts[l] > priv->lanes)
				continue;
			ret = tachyon_dp_link_train_at(priv, rates[r],
						       lane_counts[l]);
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

static void tachyon_dp_program_mainlink(struct tachyon_dp_priv *priv)
{
	u32 cfg = DP_CONFIGURATION_CTRL_SYNC_ASYNC_CLK |
		  DP_CONFIGURATION_CTRL_STATIC_DYNAMIC_CN |
		  DP_CONFIGURATION_CTRL_P_INTERLACED |
		  (2 << DP_CONFIGURATION_CTRL_LSCLK_DIV_SHIFT) |
		  ((priv->lanes - 1) << DP_CONFIGURATION_CTRL_NUM_OF_LANES_SHIFT) |
		  (2 << DP_CONFIGURATION_CTRL_BPC_SHIFT);

	if (priv->caps.enhanced)
		cfg |= DP_CONFIGURATION_CTRL_ENHANCED_FRAMING;

	writel(DP_SW_RESET, priv->ctrl + REG_DP_SW_RESET);
	udelay(1000);
	writel(0, priv->ctrl + REG_DP_SW_RESET);

	writel(0xe4, priv->link + REG_DP_LOGICAL2PHYSICAL_LANE_MAPPING);
	writel(cfg, priv->link + REG_DP_CONFIGURATION_CTRL);
	writel(DP_MAINLINK_CTRL_ENABLE, priv->link + REG_DP_MAINLINK_CTRL);
	tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
			     DP_MAINLINK_READY_FOR_VIDEO,
			     DP_MAINLINK_READY_FOR_VIDEO, 5000);
}

static int tachyon_dp_wait_sink(struct tachyon_dp_priv *priv)
{
	int ret, i;

	for (i = 0; i < TACHYON_DP_AUX_DEBOUNCE_TRIES; i++) {
		ret = tachyon_dp_read_dpcd_caps(priv);
		if (!ret)
			return 0;

		/*
		 * Docks often expose AUX a little after orientation/mode-switch
		 * callbacks. Retry long enough to absorb plug and cable-flip
		 * bounce before giving up.
		 */
		udelay(20000);
	}

	return ret;
}

static int tachyon_dp_probe(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	int ret;

	priv->ctrl = dev_remap_addr_index(dev, 0);
	priv->aux = dev_remap_addr_index(dev, 1);
	priv->link = dev_remap_addr_index(dev, 2);
	if (!priv->ctrl || !priv->aux || !priv->link)
		return -EINVAL;

	ret = tachyon_dp_find_phy(dev, priv);
	if (ret)
		return ret;

	if (!tachyon_dp_altmode_ready(priv)) {
		log_warning("DP Alt-Mode not confirmed; set tachyon_dp_altmode=dp\n");
		return -ENODEV;
	}

	ret = tachyon_dp_request_sbu_mux(priv);
	if (ret)
		log_warning("SBU mux unavailable: %d\n", ret);
	else
		tachyon_dp_program_sbu_mux(priv);

	tachyon_dp_qmp_aux_init(priv);
	tachyon_dp_aux_hw_init(priv);

	ret = tachyon_dp_wait_sink(priv);
	if (ret)
		return ret;

	ret = tachyon_dp_link_train(priv);
	if (ret)
		return ret;

	tachyon_dp_program_mainlink(priv);

	uc_priv->xsize = tachyon_dp_env_u32("tachyon_dp_xres",
					    TACHYON_DP_DEFAULT_XRES);
	uc_priv->ysize = tachyon_dp_env_u32("tachyon_dp_yres",
					    TACHYON_DP_DEFAULT_YRES);
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;

	video_set_flush_dcache(dev, true);
	memset((void *)plat->base, 0, plat->size);

	log_info("DP framebuffer base=%lx size=%lx aux timeouts=%u nacks=%u retries=%u\n",
		 (ulong)plat->base, (ulong)plat->size, priv->aux_timeouts,
		 priv->aux_nacks, priv->aux_retries);

	return 0;
}

static int tachyon_dp_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	u32 width = tachyon_dp_env_u32("tachyon_dp_xres",
				       TACHYON_DP_DEFAULT_XRES);
	u32 height = tachyon_dp_env_u32("tachyon_dp_yres",
					TACHYON_DP_DEFAULT_YRES);

	plat->size = width * height * 4;
	plat->align = TACHYON_DP_FB_ALIGN;

	return 0;
}

static const struct udevice_id tachyon_dp_ids[] = {
	{ .compatible = "qcom,sc7280-dp" },
	{ }
};

U_BOOT_DRIVER(tachyon_dp) = {
	.name		= "tachyon_dp",
	.id		= UCLASS_VIDEO,
	.of_match	= tachyon_dp_ids,
	.bind		= tachyon_dp_bind,
	.probe		= tachyon_dp_probe,
	.priv_auto	= sizeof(struct tachyon_dp_priv),
	.plat_auto	= sizeof(struct video_uc_plat),
};
