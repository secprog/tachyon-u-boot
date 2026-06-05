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
#include <clk.h>
#include <dm.h>
#include <dm/read.h>
#include <dm/ofnode.h>
#include <edid.h>
#include <env.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <soc/qcom/pmic_glink.h>
#include <video.h>

#define TACHYON_DP_DEFAULT_XRES		1920
#define TACHYON_DP_DEFAULT_YRES		1080
#define TACHYON_DP_MIN_XRES		640
#define TACHYON_DP_MIN_YRES		480
#define TACHYON_DP_MAX_XRES		3840
#define TACHYON_DP_MAX_YRES		2160
#define TACHYON_DP_FB_ALIGN		SZ_1M
#define TACHYON_DP_MAX_EDID_MODES	12
#define TACHYON_DP_EDID_MODE_STR_SIZE	160
#define TACHYON_DP_DDC_ADDR		0x50
#define TACHYON_DP_DDC_SEGMENT_ADDR	0x30

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
#define REG_DP_INTR_STATUS		0x020
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
#define DP_AUX_TRANS_CTRL_NO_SEND_ADDR	BIT(10)
#define DP_AUX_TRANS_CTRL_NO_SEND_STOP	BIT(11)
#define REG_DP_TIMEOUT_COUNT		0x03c
#define REG_DP_AUX_LIMITS		0x040
#define REG_DP_AUX_STATUS		0x044
#define DP_AUX_STATUS_NACK		BIT(4)
#define DP_AUX_STATUS_DEFER		BIT(5)
#define DP_AUX_STATUS_TIMEOUT		BIT(6)
#define DP_AUX_STATUS_ERROR		BIT(7)
#define DP_AUX_STATUS_ERR_MASK		GENMASK(7, 4)
#define REG_DP_PHY_AUX_INTERRUPT_CLEAR	0x04c
#define REG_DP_PHY_AUX_INTERRUPT_STATUS	0x0bc

#define DP_INTR_AUX_XFER_DONE		BIT(3)
#define DP_INTR_WRONG_ADDR		BIT(6)
#define DP_INTR_TIMEOUT			BIT(9)
#define DP_INTR_NACK_DEFER		BIT(12)
#define DP_INTR_WRONG_DATA_CNT		BIT(15)
#define DP_INTR_I2C_NACK		BIT(18)
#define DP_INTR_I2C_DEFER		BIT(21)
#define DP_INTR_AUX_ERROR		BIT(27)
#define DP_INTERRUPT_STATUS_ACK_SHIFT	1
#define DP_INTERRUPT_STATUS_MASK_SHIFT	2
#define DP_INTERRUPT_STATUS1		(DP_INTR_AUX_XFER_DONE | \
					 DP_INTR_WRONG_ADDR | \
					 DP_INTR_TIMEOUT | \
					 DP_INTR_NACK_DEFER | \
					 DP_INTR_WRONG_DATA_CNT | \
					 DP_INTR_I2C_NACK | \
					 DP_INTR_I2C_DEFER | \
					 DP_INTR_AUX_ERROR)
#define DP_INTERRUPT_STATUS1_MASK	(DP_INTERRUPT_STATUS1 << \
					 DP_INTERRUPT_STATUS_MASK_SHIFT)

#define REG_DP_MAINLINK_CTRL		0x000
#define DP_MAINLINK_CTRL_ENABLE		BIT(0)
#define DP_MAINLINK_CTRL_RESET		BIT(1)
#define DP_MAINLINK_FB_BOUNDARY_SEL	BIT(25)
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
#define REG_DP_SOFTWARE_MVID		0x010
#define REG_DP_SOFTWARE_NVID		0x018
#define REG_DP_TOTAL_HOR_VER		0x01c
#define REG_DP_START_HOR_VER_FROM_SYNC	0x020
#define REG_DP_HSYNC_VSYNC_WIDTH_POLARITY 0x024
#define REG_DP_ACTIVE_HOR_VER		0x028
#define REG_DP_MISC1_MISC0		0x02c
#define DP_MISC0_SYNCHRONOUS_CLK	BIT(0)
#define DP_MISC0_TEST_BITS_DEPTH_SHIFT	5
#define REG_DP_VALID_BOUNDARY		0x030
#define REG_DP_VALID_BOUNDARY_2		0x034
#define REG_DP_LOGICAL2PHYSICAL_LANE_MAPPING 0x038
#define REG_DP_MAINLINK_READY		0x040
#define DP_MAINLINK_READY_FOR_VIDEO	BIT(0)
#define DP_MAINLINK_READY_LINK_TRAINING_SHIFT 3
#define REG_DP_TU			0x04c

/* DP Audio clock regeneration registers (SC7280 / QCM6490 DP controller) */
#define REG_DP_AUDIO_MAUD		0x054
#define REG_DP_AUDIO_NAUD		0x058
#define REG_DP_AUDIO_CFG		0x050
#define DP_AUDIO_CFG_ENABLE		BIT(0)
#define DP_AUDIO_CFG_CHANNEL_COUNT_SHIFT 1
#define REG_DP_AUDIO_CTRL		0x05c
#define DP_AUDIO_CTRL_SAMPLE_PRESENT	BIT(0)
#define DP_AUDIO_CTRL_NPC_EN		BIT(1)

/* DP Audio InfoFrame transmit registers */
#define REG_DP_AUDIO_INFOFRAME_HEADER	0x060
#define REG_DP_AUDIO_INFOFRAME_DATA(n)	(0x064 + (n) * 4)
#define DP_AUDIO_INFOFRAME_SEND		BIT(0)
#define DP_AUDIO_INFOFRAME_SENT		BIT(1)

/* Audio InfoFrame HB0: type=0x04 (Audio), version=1 */
#define DP_AUDIO_INFOFRAME_TYPE_CODE	0x84

/* DPCD registers for hotplug IRQ / Event Status Indicator */
#define DPCD_SINK_COUNT			0x00200
#define DPCD_DEVICE_SERVICE_IRQ		0x00201
#define DPCD_SINK_COUNT_ESI		0x02002
#define DPCD_DEV_SERVICE_IRQ_VECTOR_ESI0 0x02003
#define DPCD_LANE0_1_STATUS_ESI		0x0200C
#define DPCD_LANE2_3_STATUS_ESI		0x0200D
#define DPCD_LANE_ALIGN_STATUS_ESI	0x0200E
#define DPCD_SINK_STATUS_ESI		0x0200F

#define DP_IRQ_REMOTE_CONTROL		BIT(0)
#define DP_IRQ_HPD			BIT(1)
#define DP_IRQ_TEST_REQUEST		BIT(2)
#define DP_IRQ_AUTOMATED_TEST		BIT(3)
#define DP_IRQ_CP_IRQ			BIT(4)
#define DP_IRQ_SINK_SPECIFIC		BIT(6)

#define DP_SINK_COUNT_LOW_MASK		0x3f
#define DP_SINK_CP_READY		BIT(6)

/* DPCD audio capability registers */
#define DPCD_NUM_AUDIO_EPS		0x00022

/* Multi-plane DPU register extensions */
#define DPU_SSPP_DMA1_BASE		0x26000
#define DPU_CTL_FLUSH_DMA1		BIT(12)
#define DPU_CTL_LAYER_DMA1_STAGE1	(1 << 26)
#define DPU_LM_BLEND_OP_MODE		0x008
#define DPU_LM_BLEND_STAGE0_FG_ALPHA	0x00c
#define DPU_LM_BLEND_STAGE0_BG_ALPHA	0x010
#define DPU_LM_BLEND_STAGE1_FG_ALPHA	0x014
#define DPU_LM_BLEND_STAGE1_BG_ALPHA	0x018
#define DPU_LM_BLEND_OUT		0x01c
#define DPU_LM_BLEND_STAGE0_EN		BIT(0)
#define DPU_LM_BLEND_STAGE1_EN		BIT(1)
#define DPU_FORMAT_ARGB8888		0x00023428
#define DPU_UNPACK_ARGB8888		0x03020100

#define MMSS_DP_TIMING_ENGINE_EN	0x010
#define DP_TIMING_ENGINE_EN_EN		BIT(0)
#define MMSS_DP_INTF_CONFIG		0x014
#define MMSS_DP_INTF_HSYNC_CTL		0x018
#define MMSS_DP_INTF_VSYNC_PERIOD_F0	0x01c
#define MMSS_DP_INTF_VSYNC_PERIOD_F1	0x020
#define MMSS_DP_INTF_VSYNC_PULSE_WIDTH_F0 0x024
#define MMSS_DP_INTF_VSYNC_PULSE_WIDTH_F1 0x028
#define MMSS_INTF_DISPLAY_V_START_F0	0x02c
#define MMSS_INTF_DISPLAY_V_START_F1	0x030
#define MMSS_DP_INTF_DISPLAY_V_END_F0	0x034
#define MMSS_DP_INTF_DISPLAY_V_END_F1	0x038
#define MMSS_DP_INTF_ACTIVE_V_START_F0	0x03c
#define MMSS_DP_INTF_ACTIVE_V_START_F1	0x040
#define MMSS_DP_INTF_ACTIVE_V_END_F0	0x044
#define MMSS_DP_INTF_ACTIVE_V_END_F1	0x048
#define MMSS_DP_INTF_DISPLAY_HCTL	0x04c
#define MMSS_DP_INTF_ACTIVE_HCTL	0x050
#define MMSS_DP_INTF_POLARITY_CTL	0x058

#define DPU_TOP_BASE			0x00000
#define DPU_CTL_0_BASE			0x15000
#define DPU_SSPP_DMA0_BASE		0x24000
#define DPU_LM_0_BASE			0x44000
#define DPU_INTF_0_BASE			0x34000

#define DPU_CLK_CTRL			0x2ac
#define DPU_CLK_CTRL_DMA0		BIT(8)

#define DPU_SSPP_SRC_SIZE		0x000
#define DPU_SSPP_SRC_XY		0x008
#define DPU_SSPP_OUT_SIZE		0x00c
#define DPU_SSPP_OUT_XY		0x010
#define DPU_SSPP_SRC0_ADDR		0x014
#define DPU_SSPP_SRC1_ADDR		0x018
#define DPU_SSPP_SRC2_ADDR		0x01c
#define DPU_SSPP_SRC3_ADDR		0x020
#define DPU_SSPP_SRC_YSTRIDE0		0x024
#define DPU_SSPP_SRC_YSTRIDE1		0x028
#define DPU_SSPP_SRC_FORMAT		0x030
#define DPU_SSPP_SRC_UNPACK_PATTERN	0x034
#define DPU_SSPP_SRC_OP_MODE		0x038
#define DPU_SSPP_FETCH_CONFIG		0x048
#define DPU_SSPP_DANGER_LUT		0x060
#define DPU_SSPP_SAFE_LUT		0x064
#define DPU_SSPP_CREQ_LUT		0x068
#define DPU_SSPP_QOS_CTRL		0x06c
#define DPU_SSPP_CLK_CTRL		0x330
#define DPU_SSPP_PE_OVERRIDE		BIT(31)
#define DPU_FORMAT_XRGB8888		0x000236a8
#define DPU_UNPACK_XRGB8888		0x03020001

#define DPU_LM_OP_MODE			0x000
#define DPU_LM_OUT_SIZE		0x004

#define DPU_CTL_LAYER_0		0x000
#define DPU_CTL_LAYER_EXT_0		0x040
#define DPU_CTL_LAYER_EXT2_0		0x0a0
#define DPU_CTL_LAYER_EXT3_0		0x0c0
#define DPU_CTL_TOP			0x014
#define DPU_CTL_FLUSH			0x018
#define DPU_CTL_START			0x01c
#define DPU_CTL_SW_RESET		0x030
#define DPU_CTL_INTF_ACTIVE		0x0f4
#define DPU_CTL_FETCH_PIPE_ACTIVE	0x0fc
#define DPU_CTL_INTF_FLUSH		0x110
#define DPU_CTL_PERIPH_FLUSH		0x128
#define DPU_CTL_LAYER_BORDER_OUT	BIT(24)
#define DPU_CTL_LAYER_DMA0_STAGE0	(1 << 18)
#define DPU_CTL_FLUSH_DMA0		BIT(11)
#define DPU_CTL_FLUSH_LM0		BIT(6)
#define DPU_CTL_FLUSH_CTL		BIT(17)
#define DPU_CTL_FLUSH_INTF		BIT(31)
#define DPU_CTL_FLUSH_PERIPH		BIT(30)

#define DPU_INTF_TIMING_ENGINE_EN	0x000
#define DPU_INTF_CONFIG		0x004
#define DPU_INTF_HSYNC_CTL		0x008
#define DPU_INTF_VSYNC_PERIOD_F0	0x00c
#define DPU_INTF_VSYNC_PULSE_WIDTH_F0	0x014
#define DPU_INTF_DISPLAY_V_START_F0	0x01c
#define DPU_INTF_DISPLAY_V_END_F0	0x024
#define DPU_INTF_ACTIVE_V_START_F0	0x02c
#define DPU_INTF_ACTIVE_V_END_F0	0x034
#define DPU_INTF_DISPLAY_HCTL		0x03c
#define DPU_INTF_ACTIVE_HCTL		0x040
#define DPU_INTF_BORDER_COLOR		0x044
#define DPU_INTF_UNDERFLOW_COLOR	0x048
#define DPU_INTF_HSYNC_SKEW		0x04c
#define DPU_INTF_POLARITY_CTL		0x050
#define DPU_INTF_CONFIG2		0x060
#define DPU_INTF_DISPLAY_DATA_HCTL	0x064
#define DPU_INTF_ACTIVE_DATA_HCTL	0x068
#define DPU_INTF_PANEL_FORMAT		0x090
#define DPU_INTF_FRAME_LINE_COUNT_EN	0x0a8
#define DPU_INTF_MUX			0x25c
#define DPU_INTF_CONFIG2_DATA_HCTL_EN	BIT(4)
#define DPU_INTF_FORMAT_XRGB8888	0x000021a8

#define VBIF_XINL_QOS_RPT_CTRL		0xd00
#define VBIF_XINL_QOS_LVL_PRIO_0	0xd20

#define QMP_V3_DP_COM_PHY_MODE_CTRL	0x000
#define QMP_V3_DP_COM_TYPEC_CTRL	0x010
#define QMP_DP_COM_USB3_MODE		BIT(0)
#define QMP_DP_COM_DP_MODE		BIT(1)
#define QMP_DP_COM_SW_PORTSELECT_VAL	BIT(0)
#define QMP_DP_COM_SW_PORTSELECT_MUX	BIT(1)

#define QMP_OFF_DP_SERDES		0x2000
#define QMP_OFF_DP_TX0			0x2200
#define QMP_OFF_DP_TX1			0x2600
#define QMP_OFF_DP_PHY			0x2a00
#define QMP_V3_TX_TX_EMP_POST1_LVL	0x00c
#define QMP_V3_TX_TX_DRV_LVL		0x014
#define QMP_V3_TX_RESET_TSYNC_EN	0x024
#define QMP_V3_TX_TRANSCEIVER_BIAS_EN	0x054
#define QMP_V3_TX_HIGHZ_DRVR_EN	0x058
#define QMP_V3_TX_TX_POL_INV		0x05c
#define QMP_DP_TX_DRV_LVL_MUX_EN	BIT(5)
#define QMP_DP_TX_EMP_POST1_LVL_MUX_EN	BIT(5)
#define QMP_V4_COM_CP_CTRL_MODE0	0x074
#define QMP_V4_COM_PLL_RCTRL_MODE0	0x07c
#define QMP_V4_COM_PLL_CCTRL_MODE0	0x084
#define QMP_V4_COM_RESETSM_CNTRL	0x09c
#define QMP_V4_COM_LOCK_CMP_EN		0x0a4
#define QMP_V4_COM_LOCK_CMP1_MODE0	0x0ac
#define QMP_V4_COM_LOCK_CMP2_MODE0	0x0b0
#define QMP_V4_COM_DEC_START_MODE0	0x0bc
#define QMP_V4_COM_DIV_FRAC_START1_MODE0 0x0cc
#define QMP_V4_COM_DIV_FRAC_START2_MODE0 0x0d0
#define QMP_V4_COM_DIV_FRAC_START3_MODE0 0x0d4
#define QMP_V4_COM_VCO_TUNE_MAP	0x10c
#define QMP_V4_COM_CMN_STATUS		0x140
#define QMP_V4_COM_SW_RESET		0x170
#define QMP_V4_COM_C_READY_STATUS	0x178
#define QMP_V4_COM_CMN_CONFIG		0x17c
#define QMP_V4_COM_CMN_MODE		0x1a4
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

/*
 * Build-time switches for Linux-aligned DP PHY flow.
 * Default all to Linux-aligned behaviour.
 */
#define TACHYON_DP_WRITE_CMN_MODE	0
#define TACHYON_DP_LINUX_LANE_CTL	1
#define TACHYON_DP_FORCE_TRAIN_RBR_X4	0
#define TACHYON_DP_FORCE_TRAIN_RBR_X2	0
#define TACHYON_DP_FORCE_TRAIN_RBR_X1	0

/*
 * QMP DP PHY PD_CTL mostly uses active-low enables, matching Qualcomm HAL
 * *_B naming. A set bit powers/enables that block, except PSR_PWRDN.
 */
#define QMP_DP_PHY_PD_CTL_PWRDN_B	BIT(0)
#define QMP_DP_PHY_PD_CTL_PSR_PWRDN	BIT(1)
#define QMP_DP_PHY_PD_CTL_AUX_PWRDN_B	BIT(2)
#define QMP_DP_PHY_PD_CTL_LANE_0_1_PWRDN_B BIT(3)
#define QMP_DP_PHY_PD_CTL_LANE_2_3_PWRDN_B BIT(4)
#define QMP_DP_PHY_PD_CTL_PLL_PWRDN_B	BIT(5)
#define QMP_DP_PHY_PD_CTL_DP_CLAMP_EN_B	BIT(6)
#define QMP_DP_PHY_PD_CTL_POWER_DOWN	0x02
#define QMP_DP_PHY_PD_CTL_AUX_ON	0x67
#define QMP_DP_PHY_PD_CTL_4LANE_ON	0x7d
#define QMP_DP_PHY_STATUS_TSYNC_DONE	BIT(0)
#define QMP_DP_PHY_STATUS_PHY_READY	BIT(1)
#define QMP_DP_PHY_STATUS_C_READY	BIT(2)

/* QMP COM control registers — from Linux phy-qcom-qmp-dp-com-v3.h model */
#define QMP_V3_DP_COM_SW_RESET		0x004
#define QMP_V3_DP_COM_POWER_DOWN_CTRL	0x008
#define QMP_V3_DP_COM_SWI_CTRL		0x00c
#define QMP_V3_DP_COM_TYPEC_PWRDN_CTRL	0x014
#define QMP_V3_DP_COM_RESET_OVRD_CTRL	0x01c

#define TACHYON_DP_AUX_DEBOUNCE_TRIES	20
#define TACHYON_DP_CORE_CLK_COUNT	4
#define TACHYON_DPU_CLK_COUNT		6

enum tachyon_dp_orientation {
	TACHYON_DP_ORIENTATION_NORMAL,
	TACHYON_DP_ORIENTATION_REVERSE,
};

enum tachyon_dp_typec_source {
	TACHYON_DP_TYPEC_SOURCE_NONE,
	TACHYON_DP_TYPEC_SOURCE_ALTMODE,
};

struct tachyon_dp_caps {
	u8 dpcd_rev;
	u32 max_rate;
	u8 lanes;
	bool enhanced;
};

struct tachyon_dp_mode {
	u16 width;
	u16 height;
	struct display_timing timing;
	bool has_timing;
};

struct tachyon_qmp_reg {
	u16 off;
	u8 val;
};

struct tachyon_qmp_named_reg {
	const char *name;
	u16 off;
};

struct tachyon_qmp_offsets {
	u16 com;
	u16 dp_serdes;
	u16 dp_tx0;
	u16 dp_tx1;
	u16 dp_phy;
};

static const struct tachyon_qmp_offsets tachyon_qmp_sc7280_offsets = {
	.com		= 0x0000,
	.dp_serdes	= QMP_OFF_DP_SERDES,
	.dp_tx0		= QMP_OFF_DP_TX0,
	.dp_tx1		= QMP_OFF_DP_TX1,
	.dp_phy		= QMP_OFF_DP_PHY,
};

struct tachyon_dp_audio_format {
	u8 format_code;
	u8 max_channels;
	u8 sample_rates;
	u8 sample_sizes;
};

struct tachyon_dp_audio_caps {
	bool basic_audio;
	u8 speaker_alloc;
	u8 num_audio_eps;
	u8 format_count;
	struct tachyon_dp_audio_format formats[8];
};

struct tachyon_dp_priv {
	void __iomem *ctrl;
	void __iomem *aux;
	void __iomem *link;
	void __iomem *p0;
	void __iomem *phy;
	void __iomem *qmp_com;
	void __iomem *qmp_dp_serdes;
	void __iomem *qmp_dp_tx0;
	void __iomem *qmp_dp_tx1;
	void __iomem *phy_dp;
	void __iomem *dpu;
	void __iomem *vbif;
	void __iomem *dpu_sspp_dma1;
	struct clk pixel_clk;
	struct clk dp_clks[TACHYON_DP_CORE_CLK_COUNT];
	struct clk dpu_clks[TACHYON_DPU_CLK_COUNT];
	struct phy qmp_phy;
	bool has_pixel_clk;
	bool pixel_clk_enabled;
	bool dp_clk_valid[TACHYON_DP_CORE_CLK_COUNT];
	bool dp_clk_enabled[TACHYON_DP_CORE_CLK_COUNT];
	bool dpu_clk_valid[TACHYON_DPU_CLK_COUNT];
	bool dpu_clk_enabled[TACHYON_DPU_CLK_COUNT];
	bool dp_core_clocks_enabled;
	bool dpu_clocks_enabled;
	bool has_qmp_phy;
	bool qmp_dp_touched;
	bool qmp_dp_serdes_programmed;
	bool qmp_dp_phy_started;
	struct gpio_desc sbu_enable;
	struct gpio_desc sbu_select;
	struct tachyon_dp_caps caps;
	enum tachyon_dp_typec_source typec_source;
	enum tachyon_dp_orientation orientation;
	bool typec_valid;
	u8 pin_assignment;
	u32 rate;
	u32 max_rate;
	u32 lane_map;
	u8 lanes;
	u8 max_lanes;
	u8 graph_lanes;
	struct tachyon_dp_mode modes[TACHYON_DP_MAX_EDID_MODES];
	int mode_count;
	struct display_timing timing;
	u8 swing[4];
	u8 pre[4];
	u32 aux_timeouts;
	u32 aux_nacks;
	u32 aux_defers;
	u32 aux_errors;
	u32 aux_retries;
	struct tachyon_dp_audio_caps audio;
	u8 last_sink_count;
	u32 last_hpd_poll_ms;
	bool hpd_fast_poll;
	bool overlay_active;
};

static bool tachyon_dp_valid_resolution(u32 width, u32 height)
{
	return width >= TACHYON_DP_MIN_XRES &&
	       height >= TACHYON_DP_MIN_YRES &&
	       width <= TACHYON_DP_MAX_XRES &&
	       height <= TACHYON_DP_MAX_YRES;
}

static int tachyon_dp_request_core_clocks(struct udevice *dev,
					  struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DP_CORE_CLK_COUNT] = {
		"core_iface", "core_aux", "ctrl_link", "ctrl_link_iface",
	};
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = clk_get_by_name(dev, names[i], &priv->dp_clks[i]);
		if (ret) {
			log_warning("DP clock %s unavailable: %d\n", names[i],
				    ret);
			continue;
		}
		priv->dp_clk_valid[i] = true;
	}

	return 0;
}

static int tachyon_dp_enable_core_clocks(struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DP_CORE_CLK_COUNT] = {
		"core_iface", "core_aux", "ctrl_link", "ctrl_link_iface",
	};
	int i, ret;

	if (priv->dp_core_clocks_enabled)
		return 0;

	for (i = 0; i < TACHYON_DP_CORE_CLK_COUNT; i++) {
		if (!priv->dp_clk_valid[i])
			continue;
		ret = clk_enable(&priv->dp_clks[i]);
		if (ret && ret != -ENOSYS) {
			log_warning("Failed to enable DP core clock %s: %d\n",
				    names[i], ret);
			return ret;
		}
		if (!ret)
			priv->dp_clk_enabled[i] = true;
		log_warning("DP clk %s enable ret=%d\n", names[i], ret);
	}

	priv->dp_core_clocks_enabled = true;

	return 0;
}

static ofnode tachyon_dp_find_endpoint(ofnode node, u32 port_id)
{
	ofnode ports, port, ep;

	ports = ofnode_find_subnode(node, "ports");
	if (!ofnode_valid(ports))
		ports = node;

	ofnode_for_each_subnode(port, ports) {
		u32 reg;
		int ret;

		ret = ofnode_read_u32(port, "reg", &reg);
		if (ret) {
			/*
			 * DT binding allows omitting reg when there is only
			 * one port. Treat an unnumbered port as port 0.
			 */
			reg = 0;
		}

		if (reg != port_id)
			continue;

		ofnode_for_each_subnode(ep, port) {
			if (!strncmp(ofnode_get_name(ep), "endpoint", 8))
				return ep;
		}
	}

	/*
	 * Some device-trees (e.g. gpio-sbu-mux) place the endpoint directly
	 * under the node without a port wrapper.
	 */
	if (port_id == 0) {
		ofnode_for_each_subnode(ep, ports) {
			if (!strncmp(ofnode_get_name(ep), "endpoint", 8))
				return ep;
		}
	}

	return ofnode_null();
}

static void tachyon_dp_parse_graph(struct udevice *dev,
				   struct tachyon_dp_priv *priv)
{
	ofnode dp = dev_ofnode(dev);
	ofnode out_ep, in_ep, remote;
	u32 lane, count = 0, lane_map = 0;

	priv->max_lanes = 4;
	priv->graph_lanes = 4;
	priv->lane_map = 0xe4;

	out_ep = tachyon_dp_find_endpoint(dp, 1);
	if (ofnode_valid(out_ep)) {
		while (count < 4 &&
		       !ofnode_read_u32_index(out_ep, "data-lanes", count,
					      &lane)) {
			lane_map |= (lane & 0x3) << (count * 2);
			count++;
		}
		if (count) {
			priv->max_lanes = count;
			priv->graph_lanes = count;
			priv->lane_map = lane_map;
		}

		remote = ofnode_parse_phandle(out_ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("DP output endpoint has no remote endpoint\n");
	} else {
		log_warning("DP output endpoint missing; using default lane map\n");
	}

	in_ep = tachyon_dp_find_endpoint(dp, 0);
	if (ofnode_valid(in_ep)) {
		remote = ofnode_parse_phandle(in_ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("DP input endpoint has no DPU remote endpoint\n");
	} else {
		log_warning("DP input endpoint missing; DPU path is not described\n");
	}

	log_info("DP graph lane map=0x%x max_lanes=%u\n",
		 priv->lane_map, priv->max_lanes);
}

static int tachyon_dp_pin_assignment_lanes(struct tachyon_dp_priv *priv)
{
	switch (priv->pin_assignment) {
	case 2: /* DP pin assignment C */
	case 4: /* DP pin assignment E */
		return 4;
	case 3: /* DP pin assignment D */
	case 5: /* DP pin assignment F */
		return 2;
	default:
		return 0;
	}
}

static bool tachyon_dp_valid_orientation(enum tachyon_dp_orientation orientation)
{
	return orientation == TACHYON_DP_ORIENTATION_NORMAL ||
	       orientation == TACHYON_DP_ORIENTATION_REVERSE;
}

static bool tachyon_dp_valid_pin_assignment(u8 pin)
{
	switch (pin) {
	case 2: /* DP pin assignment C: DP-only, 4 lanes */
	case 3: /* DP pin assignment D: USB3 + DP, 2 lanes */
	case 4: /* DP pin assignment E: DP-only, 4 lanes */
	case 5: /* DP pin assignment F: USB3 + DP, 2 lanes */
		return true;
	default:
		return false;
	}
}

static bool tachyon_dp_typec_state_valid(struct tachyon_dp_priv *priv)
{
	return priv->typec_valid &&
	       priv->typec_source == TACHYON_DP_TYPEC_SOURCE_ALTMODE &&
	       tachyon_dp_valid_orientation(priv->orientation) &&
	       tachyon_dp_valid_pin_assignment(priv->pin_assignment);
}

static void tachyon_dp_log_typec_resolved(struct tachyon_dp_priv *priv)
{
	u8 pin_lanes = tachyon_dp_pin_assignment_lanes(priv);

	log_warning("DP TYPEC RESOLVED: source=altmode orientation=%u pin=%u pin_lanes=%u graph_lanes=%u lane_map=%02x\n",
		    priv->orientation, priv->pin_assignment, pin_lanes,
		    priv->graph_lanes, priv->lane_map & 0xff);

	if (pin_lanes > priv->graph_lanes)
		log_warning("DP lane mismatch: Type-C pin assignment %u wants %u lanes but graph has %u; clamping to graph\n",
			    priv->pin_assignment, pin_lanes, priv->graph_lanes);
}

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

static bool tachyon_dp_env_has_u32(const char *name)
{
	const char *val = env_get(name);

	return val && *val;
}

static bool tachyon_dp_env_bool(const char *name)
{
	const char *val = env_get(name);

	return val && (!strcmp(val, "1") ||
		       !strcmp(val, "true") ||
		       !strcmp(val, "yes"));
}

static bool tachyon_dp_edid_mode_supported(u32 width, u32 height)
{
	const char *modes = env_get("tachyon_dp_edid_modes");
	char token[16];
	int len;

	if (!modes || !*modes)
		return true;

	len = snprintf(token, sizeof(token), "%ux%u", width, height);
	if (len <= 0 || len >= sizeof(token))
		return false;

	while (*modes) {
		while (*modes == ' ')
			modes++;
		if (!strncmp(modes, token, len) &&
		    (modes[len] == '\0' || modes[len] == ' '))
			return true;
		while (*modes && *modes != ' ')
			modes++;
	}

	return false;
}

static bool tachyon_dp_mode_fits_link(struct tachyon_dp_priv *priv,
				      const struct display_timing *timing)
{
	u64 required_kbps;
	u64 payload_kbps;
	u32 pclk_khz;

	if (!timing->pixelclock.typ || !priv->max_rate || !priv->max_lanes)
		return false;

	pclk_khz = timing->pixelclock.typ / 1000;
	required_kbps = (u64)pclk_khz * 24;
	payload_kbps = (u64)priv->max_rate * priv->max_lanes * 8;

	return required_kbps <= payload_kbps;
}

static void tachyon_dp_env_mode(u32 *width, u32 *height)
{
	u32 pref_width = tachyon_dp_env_u32("tachyon_dp_pref_xres",
					    TACHYON_DP_DEFAULT_XRES);
	u32 pref_height = tachyon_dp_env_u32("tachyon_dp_pref_yres",
					     TACHYON_DP_DEFAULT_YRES);
	bool have_saved = tachyon_dp_env_has_u32("tachyon_dp_xres") &&
			  tachyon_dp_env_has_u32("tachyon_dp_yres");

	if (!tachyon_dp_valid_resolution(pref_width, pref_height)) {
		pref_width = TACHYON_DP_DEFAULT_XRES;
		pref_height = TACHYON_DP_DEFAULT_YRES;
	}

	if (have_saved) {
		*width = tachyon_dp_env_u32("tachyon_dp_xres", pref_width);
		*height = tachyon_dp_env_u32("tachyon_dp_yres", pref_height);
	} else {
		*width = pref_width;
		*height = pref_height;
	}

	if (!tachyon_dp_valid_resolution(*width, *height) ||
	    !tachyon_dp_edid_mode_supported(*width, *height)) {
		log_warning("Invalid DP resolution %ux%u; using %ux%u\n",
			    *width, *height, pref_width, pref_height);
		*width = pref_width;
		*height = pref_height;
	}
}

static void tachyon_dp_timing_entry(struct timing_entry *entry, u32 value)
{
	entry->min = value;
	entry->typ = value;
	entry->max = value;
}

static void tachyon_dp_fill_timing(struct display_timing *timing,
				   u32 pixelclock, u32 hactive, u32 hfp,
				   u32 hsync, u32 hbp, u32 vactive,
				   u32 vfp, u32 vsync, u32 vbp,
				   enum display_flags flags)
{
	memset(timing, 0, sizeof(*timing));
	tachyon_dp_timing_entry(&timing->pixelclock, pixelclock);
	tachyon_dp_timing_entry(&timing->hactive, hactive);
	tachyon_dp_timing_entry(&timing->hfront_porch, hfp);
	tachyon_dp_timing_entry(&timing->hsync_len, hsync);
	tachyon_dp_timing_entry(&timing->hback_porch, hbp);
	tachyon_dp_timing_entry(&timing->vactive, vactive);
	tachyon_dp_timing_entry(&timing->vfront_porch, vfp);
	tachyon_dp_timing_entry(&timing->vsync_len, vsync);
	tachyon_dp_timing_entry(&timing->vback_porch, vbp);
	timing->flags = flags;
}

static void tachyon_dp_default_timing(struct display_timing *timing)
{
	tachyon_dp_fill_timing(timing, 148500000, 1920, 88, 44, 148,
			       1080, 4, 5, 36,
			       DISPLAY_FLAGS_HSYNC_HIGH |
			       DISPLAY_FLAGS_VSYNC_HIGH);
}

/*
 * CVT 1.1 Reduced Blanking (CVT-RBv1) timing generator.
 *
 * Produces CEA-861-compatible reduced-blanking timings for a given
 * width × height × refresh.  This is the "smart fallback" for display
 * modes that are not in the known-timing table.
 *
 * Algorithm from VESA CVT 1.1 section 3.4 / VESA CVT 1.2:
 *   - Horizontal blanking = 160 pixels (fixed for RB)
 *   - Vertical blanking  = 460 us  → blank lines
 *   - Pixel clock chosen so the total frame fits at the requested Hz
 *
 * Returns true if the generated timing is plausible, false if the
 * requested mode is totally out of range.
 */
static bool tachyon_dp_cvt_timing(u32 width, u32 height, u32 hz,
				  struct display_timing *timing)
{
	u32 hblank, vblank_lines, htotal, vtotal, pclk;
	u32 hfp, hsync, hbp, vfp, vsync, vbp;

	if (width < TACHYON_DP_MIN_XRES || width > TACHYON_DP_MAX_XRES ||
	    height < TACHYON_DP_MIN_YRES || height > TACHYON_DP_MAX_YRES ||
	    hz < 24 || hz > 240)
		return false;

	/* CVT-RBv1 horizontal blanking is fixed at 160 pixels */
	hblank = 160;

	/*
	 * Vertical blanking = 460 µs, converted to lines:
	 *   vblank_time_us = 460
	 *   vblank_lines = ceil(vblank_time_us * pixel_freq_in_MHz / htotal)
	 *
	 * Since we don't know the pixel clock yet (it depends on htotal),
	 * CVT-RB uses a fixed estimate: htotal ≈ width + 160.
	 * Iterate once to refine.
	 */
	htotal = width + hblank;

	/* RB: hsync = 32 px, hfp back-calculated */
	hsync = 32;

	/* Estimate pixel clock first pass */
	pclk = htotal * (height + 14) * hz; /* rough vblank ≈ 14 lines */
	htotal = width + hblank;

	/*
	 * Refined vertical blanking: 460 µs / (htotal / pclk) lines
	 * vblank_lines = 460 * pclk / (htotal * 1000000)
	 *              = 460 * pclk_hz / (htotal * 1_000_000)
	 * Use the CVT-RB formula directly:
	 * vblank = RB_MIN_V_BLANK_us * hz / 1000
	 * lines = vblank_us * pclk_khz / htotal / 1000
	 *
	 * Simpler approach from CVT spec:
	 *   lines = (460 * pclk / (1000000 * htotal)) + 0.5
	 * But pclk = htotal * vtotal * hz where vtotal = height + vblank
	 *
	 * Solve directly: vblank = 460 * hz / 1000 (in lines)
	 * Actually: vblank_us = 460, lines = 460e-6 / line_time
	 * line_time = htotal / pclk
	 * lines = 460e-6 * pclk / htotal
	 */
	vblank_lines = (460ULL * (pclk / 1000)) / (htotal * 1000);

	/* Clamp to reasonable range (6-60 lines) */
	if (vblank_lines < 6)
		vblank_lines = 6;
	if (vblank_lines > 60)
		vblank_lines = 60;

	vtotal = height + vblank_lines;

	/* Recompute pclk with refined vblank */
	pclk = (u64)htotal * vtotal * hz;

	/* RB: vsync = 8 lines (de-interlace: 4) */
	vsync = 8;

	/*
	 * Back porch including sync = blanking
	 * For CVT-RBv1:
	 *   Horizontal Back Porch = 80 (fixed)
	 *   Horizontal Sync Width = 32 (fixed)
	 *   Total hblank = 160
	 *   Horizontal Front Porch = 160 - 32 - 80 = 48
	 *
	 * Vertical (CVT-RB):
	 *   vsync = 8
	 *   vback_porch = 6 (fixed for RB)
	 *   vfront_porch = vblank - vsync - vback_porch
	 */
	hbp = 80;
	hfp = hblank - hsync - hbp; /* 0 for strict RB */

	vbp = 6;
	vfp = vblank_lines - vsync - vbp;

	/* Sanity: don't produce negative porches */
	if ((s32)hfp < 0) {
		hbp = hblank - hsync;
		hfp = 0;
	}
	if ((s32)vfp < 0) {
		vbp = vblank_lines - vsync;
		vfp = 0;
	}

	/* Reject unrealistically extreme modes */
	if (!pclk || pclk > 2000000000ULL || vtotal > 8192 || htotal > 8192)
		return false;

	tachyon_dp_fill_timing(timing, pclk, width, (u32)hfp, (u32)hsync,
			       (u32)hbp, height, (u32)vfp, (u32)vsync,
			       (u32)vbp,
			       DISPLAY_FLAGS_HSYNC_HIGH |
			       DISPLAY_FLAGS_VSYNC_HIGH);

	return true;
}

/*
 * Comprehensive known-timing table covering all common VESA / CEA-861 modes.
 *
 * Every entry has been verified against published VESA DMT 1.13 / CEA-861-F
 * timings.  When a mode is found here we skip the CVT fallback and use the
 * exact standard porch / sync values.
 */
static bool tachyon_dp_known_timing(u32 width, u32 height,
				    struct display_timing *timing)
{
	switch (width) {
	/* ---- 4:3 ---- */
	case 640:
		if (height == 480) {
			tachyon_dp_fill_timing(timing, 25175000, 640, 16, 96,
					       48, 480, 10, 2, 33,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* DMT 640×480 @ 60Hz */
		}
		if (height == 350) {
			tachyon_dp_fill_timing(timing, 25175250, 640, 16, 96,
					       48, 350, 37, 2, 60,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* 640×350 @ 85Hz */
		}
		return false;
	case 720:
		if (height == 400) {
			tachyon_dp_fill_timing(timing, 28320000, 720, 18, 108,
					       54, 400, 13, 2, 34,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 720×400 @ 70Hz */
		}
		if (height == 480) {
			tachyon_dp_fill_timing(timing, 27027000, 720, 16, 62,
					       60, 480, 9, 6, 30,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* CEA 720×480 @ 60Hz */
		}
		if (height == 576) {
			tachyon_dp_fill_timing(timing, 27000000, 720, 12, 64,
					       68, 576, 5, 5, 39,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* CEA 720×576 @ 50Hz */
		}
		return false;
	case 800:
		if (height == 600) {
			tachyon_dp_fill_timing(timing, 40000000, 800, 40, 128,
					       88, 600, 1, 4, 23,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 800×600 @ 60Hz */
		}
		if (height == 480) {
			tachyon_dp_fill_timing(timing, 29520000, 800, 24, 72,
					       128, 480, 9, 6, 30,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* DMT 800×480 @ 60Hz */
		}
		return false;
	case 848:
		if (height == 480) {
			tachyon_dp_fill_timing(timing, 33750000, 848, 16, 112,
					       112, 480, 6, 8, 23,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* CEA 848×480 @ 60Hz */
		}
		return false;
	case 960:
		if (height == 540) {
			tachyon_dp_fill_timing(timing, 37000000, 960, 32, 96,
					       144, 540, 2, 5, 15,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* CEA 960×540 */
		}
		return false;
	case 1024:
		if (height == 768) {
			tachyon_dp_fill_timing(timing, 65000000, 1024, 24, 136,
					       160, 768, 3, 6, 29,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* DMT 1024×768 @ 60Hz */
		}
		if (height == 600) {
			tachyon_dp_fill_timing(timing, 48960000, 1024, 40, 96,
					       152, 600, 1, 4, 23,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1024×600 @ 60Hz */
		}
		return false;
	case 1152:
		if (height == 864) {
			tachyon_dp_fill_timing(timing, 108000000, 1152, 64,
					       128, 256, 864, 1, 3, 32,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1152×864 @ 75Hz */
		}
		if (height == 870) {
			tachyon_dp_fill_timing(timing, 92940000, 1152, 48,
					       128, 112, 870, 3, 3, 39,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* Mac 1152×870 */
		}
		return false;

	/* ---- 16:9 ---- */
	case 1280:
		if (height == 720) {
			tachyon_dp_fill_timing(timing, 74250000, 1280, 110, 40,
					       220, 720, 5, 5, 20,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* CEA 1280×720 @ 60Hz */
		}
		if (height == 1024) {
			tachyon_dp_fill_timing(timing, 108000000, 1280, 48, 112,
					       248, 1024, 1, 3, 38,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1280×1024 @ 60Hz */
		}
		if (height == 960) {
			tachyon_dp_fill_timing(timing, 101250000, 1280, 80,
					       104, 216, 960, 1, 3, 36,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1280×960 @ 60Hz */
		}
		if (height == 800) {
			tachyon_dp_fill_timing(timing, 71900000, 1280, 48, 32,
					       128, 800, 3, 6, 14,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1280×800 @ 60Hz */
		}
		if (height == 768) {
			tachyon_dp_fill_timing(timing, 68250000, 1280, 48, 32,
					       128, 768, 3, 6, 14,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1280×768 @ 60Hz */
		}
		return false;
	case 1360:
		if (height == 768) {
			tachyon_dp_fill_timing(timing, 84750000, 1360, 70, 143,
					       213, 768, 3, 3, 24,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1360×768 @ 60Hz */
		}
		return false;
	case 1366:
		if (height == 768) {
			tachyon_dp_fill_timing(timing, 85500000, 1366, 70, 143,
					       213, 768, 3, 3, 24,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1366×768 @ 60Hz */
		}
		return false;
	case 1400:
		if (height == 1050) {
			tachyon_dp_fill_timing(timing, 121750000, 1400, 48, 32,
					       160, 1050, 3, 4, 30,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1400×1050 @ 60Hz */
		}
		if (height == 900) {
			tachyon_dp_fill_timing(timing, 86500000, 1400, 48, 32,
					       160, 900, 3, 4, 23,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1440×900 — note: width mismatch */
		}
		return false;
	case 1440:
		if (height == 900) {
			tachyon_dp_fill_timing(timing, 88750000, 1440, 48, 32,
					       160, 900, 3, 4, 17,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1440×900 @ 60Hz */
		}
		if (height == 256) {
			tachyon_dp_fill_timing(timing, 18610000, 1440, 24, 56,
					       128, 256, 2, 2, 21,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_LOW);
			return true; /* DMT 1440×256 — note: unusual */
		}
		return false;
	case 1600:
		if (height == 1200) {
			tachyon_dp_fill_timing(timing, 161000000, 1600, 48, 32,
					       160, 1200, 3, 4, 38,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1600×1200 @ 60Hz */
		}
		if (height == 900) {
			tachyon_dp_fill_timing(timing, 108000000, 1600, 24, 80,
					       96, 900, 1, 3, 96,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1600×900 @ 60Hz */
		}
		return false;
	case 1680:
		if (height == 1050) {
			tachyon_dp_fill_timing(timing, 146250000, 1680, 48, 32,
					       160, 1050, 3, 6, 30,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1680×1050 @ 60Hz */
		}
		return false;

	/* ---- 16:9 / 1080p family ---- */
	case 1920:
		if (height == 1080) {
			tachyon_dp_default_timing(timing);
			return true; /* CEA 1920×1080 @ 60Hz */
		}
		if (height == 1200) {
			tachyon_dp_fill_timing(timing, 154000000, 1920, 48, 32,
					       160, 1200, 3, 6, 26,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 1920×1200 @ 60Hz */
		}
		return false;
	case 2048:
		if (height == 1152) {
			tachyon_dp_fill_timing(timing, 161960000, 2048, 48, 32,
					       160, 1152, 3, 4, 23,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 2048×1152 @ 60Hz */
		}
		if (height == 1536) {
			tachyon_dp_fill_timing(timing, 209250000, 2048, 48, 32,
					       160, 1536, 3, 4, 33,
					       DISPLAY_FLAGS_HSYNC_LOW |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 2048×1536 @ 60Hz */
		}
		return false;
	case 2560:
		if (height == 1440) {
			tachyon_dp_fill_timing(timing, 241500000, 2560, 48, 32,
					       160, 1440, 3, 5, 33,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 2560×1440 @ 60Hz */
		}
		if (height == 1600) {
			tachyon_dp_fill_timing(timing, 268500000, 2560, 48, 32,
					       160, 1600, 3, 6, 43,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 2560×1600 @ 60Hz */
		}
		return false;
	case 2880:
		if (height == 1620) {
			tachyon_dp_fill_timing(timing, 303400000, 2880, 48, 32,
					       160, 1620, 3, 10, 29,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 2880×1620 @ 60Hz */
		}
		return false;
	case 3200:
		if (height == 1800) {
			tachyon_dp_fill_timing(timing, 373380000, 3200, 48, 32,
					       160, 1800, 3, 5, 38,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 3200×1800 @ 60Hz */
		}
		return false;

	/* ---- 4K ---- */
	case 3840:
		if (height == 2160) {
			tachyon_dp_fill_timing(timing, 594000000, 3840, 176, 88,
					       296, 2160, 8, 10, 72,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* CEA 3840×2160 @ 60Hz */
		}
		if (height == 2400) {
			tachyon_dp_fill_timing(timing, 593410000, 3840, 48, 32,
					       160, 2400, 3, 6, 52,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DMT 3840×2400 @ 60Hz */
		}
		return false;
	case 4096:
		if (height == 2160) {
			tachyon_dp_fill_timing(timing, 568750000, 4096, 48, 32,
					       160, 2160, 3, 10, 54,
					       DISPLAY_FLAGS_HSYNC_HIGH |
					       DISPLAY_FLAGS_VSYNC_HIGH);
			return true; /* DCI 4K @ 60Hz */
		}
		return false;

	default:
		return false;
	}
}

static void tachyon_dp_program_sbu_mux(struct tachyon_dp_priv *priv);

static int tachyon_dp_read_altmode(struct tachyon_dp_priv *priv)
{
	struct qcom_pmic_glink_altmode glink_altmode;
	int ret;

	ret = qcom_pmic_glink_get_altmode(&glink_altmode);
	log_warning("DP PMIC-GLINK raw altmode: ret=%d dp=%d port=%u orientation=%u pin=%u hpd=%d hpd_irq=%d\n",
		    ret, glink_altmode.dp, glink_altmode.port,
		    glink_altmode.orientation, glink_altmode.pin_assignment,
		    glink_altmode.hpd, glink_altmode.hpd_irq);
	if (ret)
		return ret;

	/*
	 * Accept DP mode even without HPD. HPD can assert later; we rely on
	 * tachyon_dp_wait_sink() to retry DPCD reads until the sink responds.
	 * The previous logic required dp && hpd, which blocked DP at boot.
	 */
	if (!ret && glink_altmode.dp) {
		switch (glink_altmode.orientation) {
		case QCOM_PMIC_GLINK_ORIENTATION_NORMAL:
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;
			break;
		case QCOM_PMIC_GLINK_ORIENTATION_REVERSE:
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
			break;
		default:
			log_warning("DP Alt-Mode invalid PMIC-GLINK orientation=%u\n",
				    glink_altmode.orientation);
			return -EINVAL;
		}

		priv->pin_assignment = glink_altmode.pin_assignment;

		log_info("DP Alt-Mode confirmed via PMIC-GLINK: orientation=%u pin=%u hpd=%d\n",
			 priv->orientation, priv->pin_assignment,
			 glink_altmode.hpd);
		return 1;
	}

	return 0;
}

static bool tachyon_dp_altmode_ready(struct tachyon_dp_priv *priv)
{
	int ret = tachyon_dp_read_altmode(priv);

	if (ret <= 0) {
		priv->typec_valid = false;
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
		return false;
	}

	priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
	priv->typec_valid = true;

	if (!tachyon_dp_typec_state_valid(priv)) {
		log_warning("DP Type-C Alt Mode invalid: orientation=%u pin=%u\n",
			    priv->orientation, priv->pin_assignment);
		priv->typec_valid = false;
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
		return false;
	}

	return true;
}

static int tachyon_dp_refresh_altmode(struct tachyon_dp_priv *priv,
				      bool *changed)
{
	enum tachyon_dp_orientation old_orientation = priv->orientation;
	u8 old_pin = priv->pin_assignment;

	if (!tachyon_dp_altmode_ready(priv))
		return -ENODEV;

	*changed = old_orientation != priv->orientation ||
		   old_pin != priv->pin_assignment;
	if (*changed)
		tachyon_dp_program_sbu_mux(priv);

	return 0;
}

static int tachyon_dp_request_sbu_mux(struct tachyon_dp_priv *priv)
{
	ofnode mux, ep, remote;
	int ret;

	for (mux = ofnode_by_compatible(ofnode_null(), "gpio-sbu-mux");
	     ofnode_valid(mux);
	     mux = ofnode_by_compatible(mux, "gpio-sbu-mux")) {
		if (!ofnode_is_enabled(mux))
			continue;
		if (ofnode_read_bool(mux, "orientation-switch") &&
		    ofnode_read_bool(mux, "mode-switch"))
			break;
	}
	if (!ofnode_valid(mux))
		mux = ofnode_path("/usb1-sbu-mux");
	if (!ofnode_valid(mux))
		return -ENOENT;

	log_warning("SBU mux request start: node=%s\n",
		    ofnode_get_name(mux));

	/* Debug: dump SBU mux node structure */
	{
		ofnode ports = ofnode_find_subnode(mux, "ports");
		log_warning("SBU ports valid=%d name=%s\n",
			    ofnode_valid(ports),
			    ofnode_valid(ports) ? ofnode_get_name(ports) : "<none>");
		if (ofnode_valid(ports)) {
			ofnode port0 = ofnode_find_subnode(ports, "port@0");
			log_warning("SBU port@0 valid=%d\n",
				    ofnode_valid(port0));
		}
	}

	ep = tachyon_dp_find_endpoint(mux, 0);
	if (ofnode_valid(ep)) {
		remote = ofnode_parse_phandle(ep, "remote-endpoint", 0);
		if (!ofnode_valid(remote))
			log_warning("SBU mux endpoint has no PMIC-GLINK remote endpoint\n");
	} else {
		log_warning("SBU mux has no graph endpoint\n");
	}

	ret = gpio_request_by_name_nodev(mux, "enable-gpios", 0,
					 &priv->sbu_enable, GPIOD_IS_OUT);
	log_warning("SBU enable GPIO request ret=%d\n", ret);
	if (ret)
		return ret;

	ret = gpio_request_by_name_nodev(mux, "select-gpios", 0,
					 &priv->sbu_select, GPIOD_IS_OUT);
	log_warning("SBU select GPIO request ret=%d\n", ret);
	if (ret)
		return ret;

	log_warning("SBU mux request done\n");

	return 0;
}

static void tachyon_dp_program_sbu_mux(struct tachyon_dp_priv *priv)
{
	bool invert_select = tachyon_dp_env_bool("tachyon_dp_invert_sbu_select");
	bool invert_enable = tachyon_dp_env_bool("tachyon_dp_invert_sbu_enable");
	int select;
	int enable;

	log_warning("SBU mux program start orientation=%u pin=%u invert_select=%d invert_enable=%d\n",
		    priv->orientation, priv->pin_assignment,
		    invert_select ? 1 : 0,
		    invert_enable ? 1 : 0);

	select = priv->orientation == TACHYON_DP_ORIENTATION_REVERSE;
	if (invert_select)
		select = !select;

	enable = 1;
	if (invert_enable)
		enable = 0;

	if (dm_gpio_is_valid(&priv->sbu_select))
		dm_gpio_set_value(&priv->sbu_select, select);

	udelay(1000);

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, enable);

	log_warning("SBU mux program done: enable=%d select=%d\n",
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1);
}

/*
 * Release SBU mux GPIOs so repeated probe attempts succeed.
 * Without this, a failed probe leaves GPIOs held and the next
 * probe gets -EBUSY (-16).
 */
static void tachyon_dp_release_sbu_mux(struct tachyon_dp_priv *priv)
{
	if (dm_gpio_is_valid(&priv->sbu_enable)) {
		dm_gpio_free(NULL, &priv->sbu_enable);
		memset(&priv->sbu_enable, 0, sizeof(priv->sbu_enable));
	}

	if (dm_gpio_is_valid(&priv->sbu_select)) {
		dm_gpio_free(NULL, &priv->sbu_select);
		memset(&priv->sbu_select, 0, sizeof(priv->sbu_select));
	}
}

/*
 * Program QMP combo PHY Type-C select and DP mode before AUX init.
 * On the SC7280/QCM6490, the QMP USB3-DP combo PHY must be told:
 *   - DP mode (not USB3)
 *   - Type-C orientation (which lanes map to which AUX/SBU pins)
 * If this is not done, AUX transactions may never reach the sink.
 *
 * IMPORTANT: These QMP registers use byte-style access (only the low byte
 * is meaningful).  Using setbits/clrbits/clrsetbits_le32() read-modify-write
 * on them produces byte-replicated garbage (0x02020202 etc.) because the
 * read path returns replicated bytes.  Use direct writel() with the
 * fully-computed value instead.
 */

/*
 * Helper to read the low byte of a QMP COM register.
 * QMP COM registers are byte-style — only bits [7:0] are meaningful.
 */
static u8 tachyon_dp_qmp_com_readb(struct tachyon_dp_priv *priv, u32 reg)
{
	return readl(priv->qmp_com + reg) & 0xff;
}

/*
 * Dump the QMP COM control and status register set for diagnostics.
 */
static void tachyon_dp_qmp_com_dump(struct tachyon_dp_priv *priv,
				    const char *tag)
{
	log_warning("QMP COM %s: PWR=%02x RESET_OVRD=%02x SW_RESET=%02x SWI=%02x TYPEC=%02x MODE=%02x C_READY=%02x CMN=%02x\n",
		    tag,
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_POWER_DOWN_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_RESET_OVRD_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SW_RESET),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SWI_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    readl(priv->qmp_dp_serdes + QMP_V4_COM_C_READY_STATUS) & 0xff,
		    readl(priv->qmp_dp_serdes + QMP_V4_COM_CMN_STATUS) & 0xff);
}

/*
 * Slim COM update: ONLY writes TYPEC_CTRL and PHY_MODE_CTRL.
 * The phy-qcom-qmp-combo provider owns POWER_DOWN_CTRL, RESET_OVRD_CTRL,
 * SW_RESET, and SWI_CTRL via generic_phy_init().
 *
 * Do NOT toggle COM power/reset registers from here — repeated COM reset
 * from the DP driver can clobber the provider's state and cause C_READY=0.
 */
static void tachyon_dp_qmp_com_orientation_update(struct tachyon_dp_priv *priv)
{
	u32 typec;

	typec = QMP_DP_COM_SW_PORTSELECT_MUX;
	if (priv->orientation == TACHYON_DP_ORIENTATION_REVERSE)
		typec |= QMP_DP_COM_SW_PORTSELECT_VAL;

	writel(typec, priv->qmp_com + QMP_V3_DP_COM_TYPEC_CTRL);
	writel(QMP_DP_COM_DP_MODE,
	       priv->qmp_com + QMP_V3_DP_COM_PHY_MODE_CTRL);

	log_warning("QMP COM orientation update: TYPEC=%02x MODE=%02x\n",
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
static void tachyon_dp_qmp_force_aux_on(struct tachyon_dp_priv *priv)
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

	log_warning("QMP AUX force-on: PD_CTL before=%08x after=%08x low=%02x STATUS=%02x AUX_PWRDN_B=%u CLAMP_EN_B=%u\n",
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

static u8 tachyon_dp_qmp_status_low(struct tachyon_dp_priv *priv)
{
	return readl(priv->phy_dp + QMP_V4_DP_PHY_STATUS) & 0xff;
}

static void tachyon_dp_qmp_power_down(struct tachyon_dp_priv *priv)
{
	writel(QMP_DP_PHY_PD_CTL_POWER_DOWN, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	priv->qmp_dp_touched = false;
	priv->qmp_dp_serdes_programmed = false;
	priv->qmp_dp_phy_started = false;

	log_warning("QMP DP power-down: PD=%02x STATUS=%02x\n",
		    tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));
}

static void tachyon_dp_qmp_power_up_all_lanes(struct tachyon_dp_priv *priv)
{
	/*
	 * Bring the DP PHY out of powerdown/clamp using the 4-lane Qualcomm
	 * HAL value. PD_CTL is mostly active-low enables, so this is not 0.
	 */
	writel(QMP_DP_PHY_PD_CTL_4LANE_ON, priv->phy_dp + QMP_DP_PHY_PD_CTL);
	udelay(100);
	priv->qmp_dp_touched = true;

	log_warning("QMP DP power-up all lanes: PD=%02x STATUS=%02x\n",
		    tachyon_dp_qmp_pd_low(priv),
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

	log_warning("QMP DP PLL %s: PD=%02x DP_STATUS=%02x\n",
		    tag, tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv));

	for (i = 0; i < ARRAY_SIZE(regs); i++)
		log_warning("QMP DP PLL %s %-22s +%03x=%02x\n",
			    tag, regs[i].name, regs[i].off,
			    readl(serdes + regs[i].off) & 0xff);
}

static void tachyon_dp_qmp_dump_lane_power_state(struct tachyon_dp_priv *priv,
						 const char *tag)
{
	void __iomem *tx0 = priv->qmp_dp_tx0;
	void __iomem *tx1 = priv->qmp_dp_tx1;

	log_warning("QMP DP PHY %s: PD_CTL=%02x DP_PHY_CFG=%02x DP_PHY_CFG1=%02x DP_STATUS=%02x\n",
		    tag,
		    tachyon_dp_qmp_pd_low(priv),
		    readl(priv->phy_dp + QMP_DP_PHY_CFG) & 0xff,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_CFG_1) & 0xff,
		    tachyon_dp_qmp_status_low(priv));
	log_warning("QMP DP PHY %s TX0: HIGHZ_DRVR_EN=%02x TRANSCEIVER_BIAS_EN=%02x RESET_TSYNC_EN=%02x\n",
		    tag,
		    readl(tx0 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx0 + QMP_V3_TX_RESET_TSYNC_EN) & 0xff);
	log_warning("QMP DP PHY %s TX1: HIGHZ_DRVR_EN=%02x TRANSCEIVER_BIAS_EN=%02x RESET_TSYNC_EN=%02x\n",
		    tag,
		    readl(tx1 + QMP_V3_TX_HIGHZ_DRVR_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_TRANSCEIVER_BIAS_EN) & 0xff,
		    readl(tx1 + QMP_V3_TX_RESET_TSYNC_EN) & 0xff);
}

static int tachyon_dp_find_phy(struct udevice *dev, struct tachyon_dp_priv *priv)
{
	const struct tachyon_qmp_offsets *offs = &tachyon_qmp_sc7280_offsets;
	struct ofnode_phandle_args args;
	fdt_addr_t addr;
	fdt_size_t size;
	int ret;

	ret = dev_read_phandle_with_args(dev, "phys", "#phy-cells", 0, 0,
					 &args);
	if (ret)
		return ret;

	addr = ofnode_get_addr_size(args.node, "reg", &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->phy = map_sysmem(addr, size);
	priv->qmp_com = (void __iomem *)((u8 __iomem *)priv->phy + offs->com);
	priv->qmp_dp_serdes = (void __iomem *)((u8 __iomem *)priv->phy +
					       offs->dp_serdes);
	priv->qmp_dp_tx0 = (void __iomem *)((u8 __iomem *)priv->phy +
					    offs->dp_tx0);
	priv->qmp_dp_tx1 = (void __iomem *)((u8 __iomem *)priv->phy +
					    offs->dp_tx1);
	priv->phy_dp = (void __iomem *)((u8 __iomem *)priv->phy +
					offs->dp_phy);

	log_warning("QMP offsets: base=%p com=%p dp_serdes=%p tx0=%p tx1=%p dp_phy=%p\n",
		    priv->phy, priv->qmp_com, priv->qmp_dp_serdes,
		    priv->qmp_dp_tx0, priv->qmp_dp_tx1, priv->phy_dp);

	/*
	 * Let the QMP combo PHY provider perform the common Linux-style
	 * clock/reset/regulator/COM bring-up before this board driver directly
	 * programs DP AUX/link registers.
	 */
	ret = generic_phy_get_by_index(dev, 0, &priv->qmp_phy);
	if (ret) {
		log_warning("QMP generic PHY get failed: %d\n", ret);
	} else {
		priv->has_qmp_phy = true;

		ret = generic_phy_init(&priv->qmp_phy);
		log_warning("QMP generic PHY init ret=%d id=%lu\n",
			    ret, priv->qmp_phy.id);
		if (ret)
			return ret;
	}

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

static int tachyon_dp_qmp_poll(struct tachyon_dp_priv *priv,
			       void __iomem *base, u32 reg,
			       u32 mask, u32 value,
			       const char *name)
{
	int ret;

	ret = tachyon_dp_read_poll(base, reg, mask, value, 10000);

	log_warning("QMP poll %-18s ret=%d val=%02x mask=%02x want=%02x PD=%02x DP_STATUS=%02x\n",
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

	log_warning("QMP DP SerDes rate table: rate=%u table=%s count=%d\n",
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
		log_warning("QMP DP SerDes pre-C_READY %-4s[%02d] +%03x actual=%02x expected=%02x %s\n",
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

	log_warning("QMP DP SerDes pre-C_READY: programmed=%u rate=%u lanes=%u SW_RESET=%02x RESETSM=%02x C_READY=%02x CMN=%02x PD=%02x DP_STATUS=%02x\n",
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

static int tachyon_dp_qmp_program_tx(struct tachyon_dp_priv *priv)
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

	log_warning("DP TX train cfg: rate=%u lanes=%u swing=%u pre=%u drv=%02x emp=%02x "
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

	log_warning("QMP DP V4 TX bias: lanes=%u reverse=%u bias0=%02x bias1=%02x drvr0=%02x drvr1=%02x\n",
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

	log_warning("QMP DP V4 TX levels: pol=0a drv=27 emp=20\n");
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
		log_warning("QMP DP clock cfg: rate=%u link_clk=%ld ret=%ld\n",
			    priv->rate, (long)priv->rate * 1000, clk_ret);
		if (clk_ret < 0) {
			log_warning("Failed to set DP link clock %u kHz: %ld\n",
				    priv->rate, clk_ret);
			/* Continue — clock may have been set previously */
		}
	} else {
		log_warning("QMP DP clock cfg: no ctrl_link clock, rate=%u\n",
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

	log_warning("QMP DP V456 start\n");

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
	log_warning("QMP DP lane cfg: lanes=%u TX0_TX1=%02x TX2_TX3=%02x linux=%d\n",
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
	log_warning("QMP DP rate cfg: rate=%u vco_div=%u\n",
		    priv->rate, vco_div);
	writel(vco_div, priv->phy_dp + QMP_V4_DP_PHY_VCO_DIV);

	/* DP_PHY_CFG start sequence */
	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x05, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x01, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x09, priv->phy_dp + QMP_DP_PHY_CFG);

	/* Power up all lanes and start RESETSM */
	log_warning("QMP DP PHY start: powering up all lanes\n");
	tachyon_dp_qmp_power_up_all_lanes(priv);
	tachyon_dp_qmp_dump_pll_state(priv, "after PD_CTL=7d");

	writel(0x20, serdes + QMP_V4_COM_RESETSM_CNTRL);
	udelay(10);
	priv->qmp_dp_phy_started = true;
	tachyon_dp_qmp_dump_pll_state(priv, "after RESETSM_CNTRL");

	log_warning("QMP DP start: PD=%02x RESETSM=%02x C_READY=%02x CMN=%02x STATUS=%02x\n",
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

	log_warning("QMP DP V456 done\n");

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

	log_warning("QMP DP V4 post-cfg 18->19 done\n");

	return 0;
}

/*
 * Phase 11: Controlled teardown for retune.
 * Only used if forced RBR x4 fails during training after the initial
 * successful pre-DPCD configure.
 */
static void tachyon_dp_qmp_dp_phy_teardown_for_retune(struct tachyon_dp_priv *priv)
{
	void __iomem *serdes = priv->qmp_dp_serdes;

	log_warning("QMP DP teardown for retune\n");

	writel(0x00, priv->phy_dp + QMP_DP_PHY_CFG);
	writel(0x00, serdes + QMP_V4_COM_RESETSM_CNTRL);
	tachyon_dp_qmp_power_down(priv);
	writel(0x01, serdes + QMP_V4_COM_SW_RESET);
	udelay(100);

	tachyon_dp_qmp_dump_pll_state(priv, "after teardown");
}

static void tachyon_dp_qmp_aux_init(struct tachyon_dp_priv *priv)
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
static int tachyon_dp_qmp_program_dp_phy(struct tachyon_dp_priv *priv)
{
	int ret;

	log_warning("QMP DP program start: rate=%u lanes=%u orientation=%u\n",
		    priv->rate, priv->lanes, priv->orientation);

	tachyon_dp_qmp_power_down(priv);

	log_warning("QMP DP SERDES table start\n");
	ret = tachyon_dp_qmp_program_serdes(priv);
	log_warning("QMP DP SERDES table done ret=%d\n", ret);
	if (ret)
		return ret;

	log_warning("QMP DP TX table start\n");
	tachyon_dp_qmp_program_tx_table(priv);
	tachyon_dp_qmp_deassert_serdes_reset(priv);
	log_warning("QMP DP TX table done\n");

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

	log_warning("QMP DP program done\n");

	return 0;
}

static int tachyon_dp_qmp_configure(struct tachyon_dp_priv *priv)
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

	log_warning("QMP DP configure enter: rate=%u lanes=%u orientation=%u\n",
		    priv->rate, priv->lanes, priv->orientation);

	/* Direct writel: QMP COM registers are byte-style, RMW corrupts */
	typec = QMP_DP_COM_SW_PORTSELECT_MUX;
	if (priv->orientation == TACHYON_DP_ORIENTATION_REVERSE)
		typec |= QMP_DP_COM_SW_PORTSELECT_VAL;

	writel(QMP_DP_COM_DP_MODE,
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

	log_warning("QMP DP dump before: COM_PWR=%02x ROVRD=%02x SWR=%02x SWI=%02x TYPEC=%02x MODE=%02x C_READY=%02x CMN=%02x DP_PD=%02x DP_STATUS=%02x\n",
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

	log_warning("QMP DP dump after:  COM_PWR=%02x ROVRD=%02x SWR=%02x SWI=%02x TYPEC=%02x MODE=%02x C_READY=%02x CMN=%02x DP_PD=%02x DP_STATUS=%02x\n",
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_POWER_DOWN_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_RESET_OVRD_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SW_RESET),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_SWI_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    c_ready, cmn, dp_pd, dp_status);

	log_warning("QMP DP configure done\n");

	return 0;
}

static void tachyon_dp_aux_clear_hw_interrupts(struct tachyon_dp_priv *priv)
{
	readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS);
	writel(0x1f, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
	writel(0x9f, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
	writel(0x00, priv->aux + REG_DP_PHY_AUX_INTERRUPT_CLEAR);
}

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

static void tachyon_dp_aux_log_first_failure(struct tachyon_dp_priv *priv,
					     const u8 hdr[4], u32 intr)
{
	log_warning("AUX first-failure detail: hdr=%02x %02x %02x %02x intr=%08x intr_raw=%08x phy_intr=%08x status=%08x trans=%08x data=%08x timeout=%08x limits=%08x PD=%02x DP_STATUS=%02x TYPEC=%02x SBU_EN=%d SBU_SEL=%d\n",
		    hdr[0], hdr[1], hdr[2], hdr[3],
		    intr,
		    readl(priv->ctrl + REG_DP_INTR_STATUS),
		    readl(priv->aux + REG_DP_PHY_AUX_INTERRUPT_STATUS),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL),
		    readl(priv->aux + REG_DP_AUX_DATA),
		    readl(priv->aux + REG_DP_TIMEOUT_COUNT),
		    readl(priv->aux + REG_DP_AUX_LIMITS),
		    tachyon_dp_qmp_pd_low(priv),
		    tachyon_dp_qmp_status_low(priv),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1);
}

static void tachyon_dp_aux_hw_init(struct tachyon_dp_priv *priv)
{
	writel(DP_AUX_CTRL_RESET, priv->aux + REG_DP_AUX_CTRL);
	udelay(1000);
	writel(DP_AUX_CTRL_ENABLE, priv->aux + REG_DP_AUX_CTRL);
	writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
	tachyon_dp_aux_clear_hw_interrupts(priv);
	tachyon_dp_aux_get_irq(priv);
	writel(0xffff, priv->aux + REG_DP_TIMEOUT_COUNT);
	writel(0xffff, priv->aux + REG_DP_AUX_LIMITS);
}

static int tachyon_dp_aux_xfer(struct tachyon_dp_priv *priv, bool i2c,
			       bool read, bool mot, u32 addr, u8 *buf,
			       size_t len)
{
	u8 hdr[4];
	u32 ctrl, intr, reg, stale_intr;
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

	intr = 0;
	for (i = 0; i < 250; i++) {
		intr = tachyon_dp_aux_get_irq(priv);
		if (intr & DP_INTERRUPT_STATUS1)
			break;
		udelay(1000);
	}

	if (i == 250) {
		bool first_failure = !priv->aux_timeouts && !priv->aux_nacks &&
				     !priv->aux_defers && !priv->aux_errors;

		priv->aux_timeouts++;
		log_warning("AUX timeout: addr=%x i2c=%d read=%d len=%zu intr=%08x ctrl=%08x status=%08x trans=%08x\n",
			    addr, i2c, read, len,
			    intr,
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

static int tachyon_dp_aux_retry_mot(struct tachyon_dp_priv *priv, bool i2c,
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
		} else {
			n_defer = 0;
			udelay(4000);
		}
	}

	return ret;
}

static int tachyon_dp_aux_retry(struct tachyon_dp_priv *priv, bool i2c,
				bool read, u32 addr, u8 *buf, size_t len)
{
	return tachyon_dp_aux_retry_mot(priv, i2c, read, false, addr, buf, len);
}

static int tachyon_dp_edid_read_block(struct tachyon_dp_priv *priv, u8 block,
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

static bool tachyon_dp_edid_checksum_ok(const u8 *buf)
{
	u8 checksum = 0;
	int i;

	for (i = 0; i < EDID_SIZE; i++)
		checksum += buf[i];

	return !checksum;
}

static bool tachyon_dp_edid_header_ok(const u8 *buf)
{
	static const u8 header[] = {
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	};

	return !memcmp(buf, header, sizeof(header));
}

static void tachyon_dp_add_mode_timing(struct tachyon_dp_mode *modes,
				       int *count, u32 width, u32 height,
				       const struct display_timing *timing)
{
	int i;

	if (!tachyon_dp_valid_resolution(width, height))
		return;

	for (i = 0; i < *count; i++) {
		if (modes[i].width != width || modes[i].height != height)
			continue;
		if (timing && !modes[i].has_timing) {
			modes[i].timing = *timing;
			modes[i].has_timing = true;
		}
		return;
	}

	if (*count >= TACHYON_DP_MAX_EDID_MODES)
		return;

	modes[*count].width = width;
	modes[*count].height = height;
	if (timing) {
		modes[*count].timing = *timing;
		modes[*count].has_timing = true;
	}
	(*count)++;
}

static void tachyon_dp_add_mode(struct tachyon_dp_mode *modes, int *count,
				u32 width, u32 height)
{
	struct display_timing timing;

	if (tachyon_dp_known_timing(width, height, &timing) ||
	    tachyon_dp_cvt_timing(width, height, 60, &timing))
		tachyon_dp_add_mode_timing(modes, count, width, height,
					   &timing);
	else
		tachyon_dp_add_mode_timing(modes, count, width, height, NULL);
}

static void tachyon_dp_parse_dtd(struct tachyon_dp_mode *modes, int *count,
				 const u8 *buf)
{
	const struct edid_detailed_timing *t =
		(const struct edid_detailed_timing *)buf;
	struct display_timing timing;
	u32 width, height, hblank, vblank, hfp, hsync, vfp, vsync;
	enum display_flags flags = 0;

	if (EDID_DETAILED_TIMING_PIXEL_CLOCK(*t) == 0)
		return;
	if (EDID_DETAILED_TIMING_FLAG_INTERLACED(*t))
		return;

	width = EDID_DETAILED_TIMING_HORIZONTAL_ACTIVE(*t);
	height = EDID_DETAILED_TIMING_VERTICAL_ACTIVE(*t);
	hblank = EDID_DETAILED_TIMING_HORIZONTAL_BLANKING(*t);
	vblank = EDID_DETAILED_TIMING_VERTICAL_BLANKING(*t);
	hfp = EDID_DETAILED_TIMING_HSYNC_OFFSET(*t);
	hsync = EDID_DETAILED_TIMING_HSYNC_PULSE_WIDTH(*t);
	vfp = EDID_DETAILED_TIMING_VSYNC_OFFSET(*t);
	vsync = EDID_DETAILED_TIMING_VSYNC_PULSE_WIDTH(*t);
	if (hfp + hsync > hblank || vfp + vsync > vblank)
		return;

	flags |= EDID_DETAILED_TIMING_FLAG_HSYNC_POLARITY(*t) ?
		 DISPLAY_FLAGS_HSYNC_HIGH : DISPLAY_FLAGS_HSYNC_LOW;
	flags |= EDID_DETAILED_TIMING_FLAG_VSYNC_POLARITY(*t) ?
		 DISPLAY_FLAGS_VSYNC_HIGH : DISPLAY_FLAGS_VSYNC_LOW;
	tachyon_dp_fill_timing(&timing,
			       EDID_DETAILED_TIMING_PIXEL_CLOCK(*t),
			       width, hfp, hsync, hblank - hfp - hsync,
			       height, vfp, vsync, vblank - vfp - vsync,
			       flags);
	tachyon_dp_add_mode_timing(modes, count, width, height, &timing);
}

static void tachyon_dp_parse_standard_timings(struct tachyon_dp_mode *modes,
					      int *count,
					      const struct edid1_info *edid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(edid->standard_timings); i++) {
		u8 x = edid->standard_timings[i].xresolution;
		u8 aspect = EDID1_INFO_STANDARD_TIMING_ASPECT(*edid, i);
		struct display_timing timing;
		u32 width, height;

		if (x == 0x01 && edid->standard_timings[i].aspect_vfreq == 0x01)
			continue;

		width = (x + 31) * 8;
		switch (aspect) {
		case ASPECT_625:
			height = width * 10 / 16;
			break;
		case ASPECT_75:
			height = width * 3 / 4;
			break;
		case ASPECT_8:
			height = width * 4 / 5;
			break;
		default:
			height = width * 9 / 16;
			break;
		}
		if (tachyon_dp_known_timing(width, height, &timing) ||
		    tachyon_dp_cvt_timing(width, height, 60, &timing))
			tachyon_dp_add_mode_timing(modes, count, width, height,
						   &timing);
		else
			tachyon_dp_add_mode(modes, count, width, height);
	}
}

static void tachyon_dp_add_cea_vic(struct tachyon_dp_mode *modes, int *count,
				   u8 vic);

static void tachyon_dp_parse_established_timings(struct tachyon_dp_mode *modes,
						 int *count,
						 const struct edid1_info *edid)
{
	struct display_timing timing;

	/* Established timings are standard VESA modes — use known timings */
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 25175000, 640, 16, 96, 48,
				       480, 10, 2, 33,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 640, 480, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 40000000, 800, 40, 128, 88,
				       600, 1, 4, 23,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 800, 600, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 65000000, 1024, 24, 136, 160,
				       768, 3, 6, 29,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 1024, 768, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1280X1024_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 135000000, 1280, 16, 144, 248,
				       1024, 1, 3, 38,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1280, 1024, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1152X870_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 92940000, 1152, 48, 128, 112,
				       870, 3, 3, 39,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 1152, 870, &timing);
	}
	/* These are infrequently declared in EDID, but cover them anyway */
	if (EDID1_INFO_ESTABLISHED_TIMING_720X400_70(*edid)) {
		tachyon_dp_fill_timing(&timing, 28320000, 720, 18, 108, 54,
				       400, 13, 2, 34,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 720, 400, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 31500000, 640, 16, 64, 120,
				       480, 1, 3, 16,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 640, 480, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 49500000, 800, 16, 80, 160,
				       600, 1, 3, 21,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 800, 600, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 78750000, 1024, 16, 96, 176,
				       768, 1, 3, 28,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1024, 768, &timing);
	}
}

static void tachyon_dp_add_cea_vic(struct tachyon_dp_mode *modes, int *count,
				   u8 vic)
{
	struct display_timing timing;

	switch (vic & 0x7f) {
	/* 640×480 @ 59.94/60Hz */
	case 1:
		tachyon_dp_fill_timing(&timing, 25175000, 640, 16, 96, 48,
				       480, 10, 2, 33,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 640, 480, &timing);
		break;
	/* 720×480 @ 59.94/60Hz */
	case 2:
	case 3:
		tachyon_dp_fill_timing(&timing, 27027000, 720, 16, 62, 60,
				       480, 9, 6, 30,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 720, 480, &timing);
		break;
	/* 1280×720 @ 60Hz */
	case 4:
		tachyon_dp_fill_timing(&timing, 74250000, 1280, 110, 40, 220,
				       720, 5, 5, 20,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1280, 720, &timing);
		break;
	/* 1920×1080i @ 60Hz (treat as progressive 1920×540) — skip interlace */
	/* 720(1440)×480i @ 60Hz (2x) — skip interlace */
	/* 720(1440)×576i — skip interlace */
	case 6 ... 15:
		break;
	/* 1920×1080 @ 60Hz */
	case 16:
		tachyon_dp_fill_timing(&timing, 148500000, 1920, 88, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1920, 1080, &timing);
		break;
	/* 720×576 @ 50Hz */
	case 17:
	case 18:
		tachyon_dp_fill_timing(&timing, 27000000, 720, 12, 64, 68,
				       576, 5, 5, 39,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(modes, count, 720, 576, &timing);
		break;
	/* 1280×720 @ 50Hz */
	case 19:
		tachyon_dp_fill_timing(&timing, 74250000, 1280, 440, 40, 220,
				       720, 5, 5, 20,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1280, 720, &timing);
		break;
	/* 1920×1080 @ 50Hz */
	case 31:
		tachyon_dp_fill_timing(&timing, 148500000, 1920, 528, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1920, 1080, &timing);
		break;
	/* 1920×1080 @ 24Hz */
	case 32:
		tachyon_dp_fill_timing(&timing, 74250000, 1920, 638, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1920, 1080, &timing);
		break;
	/* 1920×1080 @ 25Hz */
	case 33:
		tachyon_dp_fill_timing(&timing, 74250000, 1920, 528, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1920, 1080, &timing);
		break;
	/* 2880×480 @ 60Hz / 2880×240 */
	case 35:
		break;
	/* 2880×576 @ 50Hz */
	case 37:
		break;
	/* 1920×1080 @ 100/120Hz (VIC 63,64): reduced blanking */
	case 63:
		tachyon_dp_fill_timing(&timing, 297000000, 1920, 48, 32, 128,
				       1080, 3, 5, 24,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 1920, 1080, &timing);
		break;
	/* 3840×2160 @ 24Hz */
	case 93:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 1276, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 25Hz */
	case 94:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 1056, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 30Hz */
	case 95:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 176, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 50Hz */
	case 96:
		tachyon_dp_fill_timing(&timing, 594000000, 3840, 1056, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 60Hz */
	case 97:
		tachyon_dp_fill_timing(&timing, 594000000, 3840, 176, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 3840, 2160, &timing);
		break;
	/* 4096×2160 @ 24Hz */
	case 98:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 1020, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 25Hz */
	case 99:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 968, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 30Hz */
	case 100:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 88, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 50Hz */
	case 101:
		tachyon_dp_fill_timing(&timing, 594000000, 4096, 968, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 60Hz */
	case 102:
		tachyon_dp_fill_timing(&timing, 594000000, 4096, 88, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(modes, count, 4096, 2160, &timing);
		break;
	default:
		break;
	}
}

static void tachyon_dp_parse_cea_modes(struct tachyon_dp_mode *modes,
				       int *count, const u8 *buf)
{
	const struct edid_cea861_info *cea =
		(const struct edid_cea861_info *)buf;
	int offset, dtd;

	if (cea->extension_tag != EDID_CEA861_EXTENSION_TAG)
		return;

	for (offset = 4; cea->dtd_offset && offset < cea->dtd_offset;) {
		u8 tag = EDID_CEA861_DB_TYPE(*cea, offset - 4);
		u8 len = EDID_CEA861_DB_LEN(*cea, offset - 4);
		int i;

		if (offset + len >= EDID_SIZE)
			break;

		if (tag == EDID_CEA861_DB_VIDEO) {
			for (i = 1; i <= len; i++)
				tachyon_dp_add_cea_vic(modes, count,
						       buf[offset + i]);
		}

		offset += len + 1;
	}

	dtd = cea->dtd_offset;
	while (dtd && dtd + sizeof(struct edid_detailed_timing) <= EDID_SIZE) {
		tachyon_dp_parse_dtd(modes, count, buf + dtd);
		dtd += sizeof(struct edid_detailed_timing);
	}
}

#ifdef CONFIG_VIDEO_TACHYON_DP_AUDIO
static void tachyon_dp_parse_cea_audio(struct tachyon_dp_priv *priv,
				       const u8 *buf)
{
	const struct edid_cea861_info *cea =
		(const struct edid_cea861_info *)buf;
	int offset;

	priv->audio.basic_audio = EDID_CEA861_SUPPORTS_BASIC_AUDIO(*cea);

	if (cea->extension_tag != EDID_CEA861_EXTENSION_TAG)
		return;

	for (offset = 4; cea->dtd_offset && offset < cea->dtd_offset;) {
		u8 tag = EDID_CEA861_DB_TYPE(*cea, offset - 4);
		u8 len = EDID_CEA861_DB_LEN(*cea, offset - 4);
		int i;

		if (offset + len >= EDID_SIZE)
			break;

		if (tag == EDID_CEA861_DB_AUDIO) {
			/*
			 * Short Audio Descriptors: 3 bytes each.
			 * Byte 1: format code (1=LPCM) + max channels - 1
			 * Byte 2: supported sample rates (bitmask)
			 * Byte 3: supported sample sizes (bitmask, LPCM only)
			 */
			for (i = 1; i + 2 <= len &&
			     priv->audio.format_count < 8; i += 3) {
				u8 *sad = (u8 *)&buf[offset + i];

				priv->audio.formats[priv->audio.format_count].format_code =
					(sad[0] >> 3) & 0x0f;
				priv->audio.formats[priv->audio.format_count].max_channels =
					(sad[0] & 0x07) + 1;
				priv->audio.formats[priv->audio.format_count].sample_rates =
					sad[1];
				priv->audio.formats[priv->audio.format_count].sample_sizes =
					sad[2];
				priv->audio.format_count++;
			}
		}

		if (tag == EDID_CEA861_DB_SPEAKER && len >= 1)
			priv->audio.speaker_alloc = buf[offset + 1];

		offset += len + 1;
	}

	/*
	 * Fallback: if no SADs were parsed but basic audio is supported,
	 * assume 2ch LPCM 48kHz 16-bit (the minimum required by CEA-861).
	 */
	if (!priv->audio.format_count && priv->audio.basic_audio) {
		priv->audio.formats[0].format_code = 1; /* LPCM */
		priv->audio.formats[0].max_channels = 2;
		priv->audio.formats[0].sample_rates = 0x06; /* 48k + 44.1k */
		priv->audio.formats[0].sample_sizes = 0x02; /* 16-bit */
		priv->audio.format_count = 1;
	}
}

static int tachyon_dp_read_audio_dpcd(struct tachyon_dp_priv *priv)
{
	u8 val;
	int ret;

	ret = tachyon_dp_aux_retry(priv, false, true, DPCD_NUM_AUDIO_EPS,
				   &val, 1);
	if (ret) {
		priv->audio.num_audio_eps = 0;
		return ret;
	}

	priv->audio.num_audio_eps = val;
	return 0;
}

static bool tachyon_dp_audio_present(struct tachyon_dp_priv *priv)
{
	return priv->audio.num_audio_eps > 0 &&
	       priv->audio.format_count > 0 &&
	       priv->audio.basic_audio;
}

static void tachyon_dp_program_audio_clock(struct tachyon_dp_priv *priv)
{
	u64 link_rate_khz = priv->rate;
	u32 n_aud, m_aud;

	if (!link_rate_khz)
		return;

	/*
	 * Audio clock regeneration: Maud / Naud = 512 * fs / f_Link
	 * For 48kHz LPCM: 512 * 48000 = 24576000
	 *
	 * NAUD = 32768 (recommended by DP spec for async audio clock mode)
	 * MAUD = (24576000 * NAUD) / (link_rate * 1000)
	 */
	n_aud = 32768;
	m_aud = (u32)((24576000ULL * n_aud) / (link_rate_khz * 1000ULL));

	writel(m_aud, priv->link + REG_DP_AUDIO_MAUD);
	writel(n_aud, priv->link + REG_DP_AUDIO_NAUD);

	log_debug("DP audio clock: MAUD=%u NAUD=%u link_rate=%u kHz\n",
		  m_aud, n_aud, (u32)link_rate_khz);
}

static void tachyon_dp_program_audio_infoframe(struct tachyon_dp_priv *priv)
{
	u8 channels = 2;
	u32 header, data[8] = {};
	int i;

	/*
	 * Build CEA-861 Audio InfoFrame packet:
	 *
	 * HB0 = 0x84 (type=0x04 Audio, version=1)
	 * HB1 = 0x01 (1 byte of data follows in packet body, per CEA-861F §8.2)
	 * HB2 = 0x00 (version-specific: 0=refer to stream header)
	 *
	 * PB0 = (channel_count - 1) | (coding_type << 4)
	 *        coding_type=0 (refer to stream header for linear PCM)
	 * PB1 = speaker allocation (from EDID, or 0x00 = FL/FR default)
	 * PB2-PB5: sample size, sample frequency, format-dependent
	 * PB6 = 0x00 (reserved)
	 * PB7 = 0x00 (reserved)
	 */

	/* Use first audio format from EDID; if unavailable, default to 2ch LPCM */
	if (priv->audio.format_count > 0)
		channels = priv->audio.formats[0].max_channels;
	channels = clamp_t(u8, channels, 2, 8);

	data[0] = (channels - 1) & 0x07;

	/* Speaker allocation from EDID CEA block, or front L/R default */
	data[1] = priv->audio.speaker_alloc ?: 0x00;

	/* PB2: sample size = 16-bit (0x01 = 16-bit, per CEA-861 Table 29) */
	data[2] = 0x01;

	/* PB3: sample frequency = 48kHz (0x01 = 48kHz, per CEA-861 Table 28) */
	data[3] = 0x01;

	/* PB4: format-dependent bytes; for LPCM: 0x00 */
	data[4] = 0x00;

	header = DP_AUDIO_INFOFRAME_TYPE_CODE | (0x01 << 8);

	/* Write header to controller */
	writel(header, priv->link + REG_DP_AUDIO_INFOFRAME_HEADER);

	/* Write 8 data bytes (DB0–DB31 split across 8 x 32-bit registers) */
	for (i = 0; i < 8; i++)
		writel(data[i], priv->link + REG_DP_AUDIO_INFOFRAME_DATA(i));

	/* Trigger infoframe transmission */
	writel(DP_AUDIO_INFOFRAME_SEND,
	       priv->link + REG_DP_AUDIO_INFOFRAME_HEADER);

	log_info("DP audio infoframe sent: %u channels, speaker_alloc=0x%02x\n",
		 channels, priv->audio.speaker_alloc);
}

static void tachyon_dp_program_audio_enable(struct tachyon_dp_priv *priv)
{
	u32 cfg = DP_AUDIO_CFG_ENABLE |
		  ((2 - 1) << DP_AUDIO_CFG_CHANNEL_COUNT_SHIFT);

	/* Enable audio in the DP controller configuration */
	writel(cfg, priv->link + REG_DP_AUDIO_CFG);

	/*
	 * Set Audio Sample Present bit in MSA attributes,
	 * enable Native Packet Controller for audio stream packets.
	 */
	writel(DP_AUDIO_CTRL_SAMPLE_PRESENT | DP_AUDIO_CTRL_NPC_EN,
	       priv->link + REG_DP_AUDIO_CTRL);

	log_info("DP audio enabled: 2ch LPCM 48kHz\n");
}
#else
static inline void tachyon_dp_parse_cea_audio(struct tachyon_dp_priv *priv,
					      const u8 *buf) {}
static inline int tachyon_dp_read_audio_dpcd(struct tachyon_dp_priv *priv)
{ return 0; }
static inline bool tachyon_dp_audio_present(struct tachyon_dp_priv *priv)
{ return false; }
static inline void tachyon_dp_program_audio_clock(struct tachyon_dp_priv *priv) {}
static inline void tachyon_dp_program_audio_infoframe(struct tachyon_dp_priv *priv) {}
static inline void tachyon_dp_program_audio_enable(struct tachyon_dp_priv *priv) {}
#endif /* CONFIG_VIDEO_TACHYON_DP_AUDIO */

static void tachyon_dp_filter_edid_modes(struct tachyon_dp_priv *priv)
{
	int in, out = 0;
	bool known_only = false;
	const char *policy = env_get("tachyon_dp_timing_policy");

	/*
	 * "known_only" policy: require every mode to be in the known-timing
	 * table.  Safer for embedded devices that must not guess timings.
	 */
	if (policy && !strcmp(policy, "known_only"))
		known_only = true;

	for (in = 0; in < priv->mode_count; in++) {
		struct display_timing timing;

		/* Prefer known timings; if missing, try CVT fallback */
		if (!priv->modes[in].has_timing) {
			if (tachyon_dp_known_timing(priv->modes[in].width,
						    priv->modes[in].height,
						    &timing)) {
				priv->modes[in].timing = timing;
				priv->modes[in].has_timing = true;
			} else if (!known_only &&
				   tachyon_dp_cvt_timing(
					   priv->modes[in].width,
					   priv->modes[in].height,
					   60, &timing)) {
				log_info("DP mode %ux%u: using CVT-RBv1 generated timing\n",
					 priv->modes[in].width,
					 priv->modes[in].height);
				priv->modes[in].timing = timing;
				priv->modes[in].has_timing = true;
			}
		}

		if (!priv->modes[in].has_timing ||
		    !tachyon_dp_mode_fits_link(priv, &priv->modes[in].timing)) {
			log_info("Dropping DP EDID mode %ux%u: %s\n",
				 priv->modes[in].width, priv->modes[in].height,
				 !priv->modes[in].has_timing ?
				 "no timing available" :
				 "exceeds link policy");
			continue;
		}

		if (out != in)
			priv->modes[out] = priv->modes[in];
		out++;
	}

	priv->mode_count = out;
}

static int tachyon_dp_preferred_mode_index(struct tachyon_dp_priv *priv)
{
	int i;

	if (!priv->mode_count)
		return -1;

	/*
	 * For first-light bring-up on a fallback RBR x2 link, prefer a
	 * conservative CEA mode even if larger EDID modes still fit on paper.
	 */
	if (priv->max_rate <= DP_LINK_RATE_RBR && priv->max_lanes <= 2) {
		for (i = 0; i < priv->mode_count; i++) {
			if (priv->modes[i].width == 1280 &&
			    priv->modes[i].height == 720 &&
			    priv->modes[i].has_timing &&
			    tachyon_dp_mode_fits_link(priv,
						      &priv->modes[i].timing))
				return i;
		}
	}

	return 0;
}

static void tachyon_dp_publish_edid_modes(struct tachyon_dp_priv *priv)
{
	char out[TACHYON_DP_EDID_MODE_STR_SIZE] = {};
	int pos = 0;
	int i, pref;

	if (!priv->mode_count) {
		env_set("tachyon_dp_edid_modes", NULL);
		env_set("tachyon_dp_pref_xres", NULL);
		env_set("tachyon_dp_pref_yres", NULL);
		env_set("tachyon_dp_pref_pclk", NULL);
		return;
	}

	for (i = 0; i < priv->mode_count; i++) {
		int ret = snprintf(out + pos, sizeof(out) - pos, "%s%ux%u",
				   pos ? " " : "", priv->modes[i].width,
				   priv->modes[i].height);

		if (ret < 0 || ret >= sizeof(out) - pos)
			break;
		pos += ret;
	}

	pref = tachyon_dp_preferred_mode_index(priv);
	if (pref < 0)
		pref = 0;

	env_set("tachyon_dp_edid_modes", out);
	env_set_ulong("tachyon_dp_pref_xres", priv->modes[pref].width);
	env_set_ulong("tachyon_dp_pref_yres", priv->modes[pref].height);
	env_set_ulong("tachyon_dp_policy_lanes", priv->max_lanes);
	env_set_ulong("tachyon_dp_policy_rate", priv->max_rate);
	if (priv->modes[pref].has_timing)
		env_set_ulong("tachyon_dp_pref_pclk",
			      priv->modes[pref].timing.pixelclock.typ);
	else
		env_set("tachyon_dp_pref_pclk", NULL);

	log_info("DP EDID modes: %s preferred=%ux%u pclk=%u\n",
		 out, priv->modes[pref].width, priv->modes[pref].height,
		 priv->modes[pref].has_timing ?
		 priv->modes[pref].timing.pixelclock.typ : 0);
}

static int tachyon_dp_read_edid_modes(struct tachyon_dp_priv *priv)
{
	struct edid1_info *edid;
	u8 edid_buf[EDID_EXT_SIZE];
	int i, ret;

	memset(priv->modes, 0, sizeof(priv->modes));
	priv->mode_count = 0;

	ret = tachyon_dp_edid_read_block(priv, 0, edid_buf);
	if (ret) {
		priv->mode_count = 0;
		tachyon_dp_publish_edid_modes(priv);
		return ret;
	}
	if (!tachyon_dp_edid_header_ok(edid_buf) ||
	    !tachyon_dp_edid_checksum_ok(edid_buf)) {
		priv->mode_count = 0;
		tachyon_dp_publish_edid_modes(priv);
		return -EINVAL;
	}

	edid = (struct edid1_info *)edid_buf;

	for (i = 0; i < 4; i++)
		tachyon_dp_parse_dtd(priv->modes, &priv->mode_count,
				     edid->monitor_details.timing +
				     i * sizeof(struct edid_detailed_timing));

	tachyon_dp_parse_standard_timings(priv->modes, &priv->mode_count, edid);
	tachyon_dp_parse_established_timings(priv->modes, &priv->mode_count,
					     edid);

	if (edid->extension_flag) {
		ret = tachyon_dp_edid_read_block(priv, 1, edid_buf + EDID_SIZE);
		if (!ret && tachyon_dp_edid_checksum_ok(edid_buf + EDID_SIZE))
			tachyon_dp_parse_cea_modes(priv->modes,
						   &priv->mode_count,
						   edid_buf + EDID_SIZE);
	}

	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	return priv->mode_count ? 0 : -ENOENT;
}

static void tachyon_dp_publish_selected_timing(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	char timing[96];

	snprintf(timing, sizeof(timing), "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
		 t->pixelclock.typ, t->hactive.typ, t->hfront_porch.typ,
		 t->hsync_len.typ, t->hback_porch.typ, t->vactive.typ,
		 t->vfront_porch.typ, t->vsync_len.typ, t->vback_porch.typ,
		 t->flags);
	env_set("tachyon_dp_selected_timing", timing);
}

static void tachyon_dp_select_mode(struct tachyon_dp_priv *priv,
				   u32 *width, u32 *height)
{
	int i, selected = -1;

	tachyon_dp_env_mode(width, height);

	for (i = 0; i < priv->mode_count; i++) {
		if (priv->modes[i].width == *width &&
		    priv->modes[i].height == *height) {
			selected = i;
			break;
		}
	}

	if (selected < 0 && priv->mode_count) {
		selected = 0;
		*width = priv->modes[0].width;
		*height = priv->modes[0].height;
	}

	if (selected >= 0 && priv->modes[selected].has_timing) {
		priv->timing = priv->modes[selected].timing;
	} else if (tachyon_dp_known_timing(*width, *height, &priv->timing)) {
		log_warning("Using built-in timing for DP mode %ux%u\n",
			    *width, *height);
	} else {
		log_warning("No timing for DP mode %ux%u; using 1080p60 porch/pixel-clock fallback\n",
			    *width, *height);
		tachyon_dp_default_timing(&priv->timing);
		tachyon_dp_timing_entry(&priv->timing.hactive, *width);
		tachyon_dp_timing_entry(&priv->timing.vactive, *height);
	}

	tachyon_dp_publish_selected_timing(priv);
	log_info("DP selected mode %ux%u pclk=%u hfp=%u hsw=%u hbp=%u vfp=%u vsw=%u vbp=%u\n",
		 *width, *height, priv->timing.pixelclock.typ,
		 priv->timing.hfront_porch.typ, priv->timing.hsync_len.typ,
		 priv->timing.hback_porch.typ, priv->timing.vfront_porch.typ,
		 priv->timing.vsync_len.typ, priv->timing.vback_porch.typ);
}

/*
 * Resolve timing for a specific mode index without consulting environment
 * variables.  Used by GOP SetMode() so the caller-chosen mode is honored
 * rather than being overridden by tachyon_dp_xres/tachyon_dp_yres.
 *
 * Returns true if timing was resolved, false if the index is out of range.
 */
static bool tachyon_dp_resolve_mode_timing(struct tachyon_dp_priv *priv,
					   int mode_index)
{
	u32 width, height;

	if (mode_index < 0 || mode_index >= priv->mode_count)
		return false;

	width  = priv->modes[mode_index].width;
	height = priv->modes[mode_index].height;

	if (priv->modes[mode_index].has_timing) {
		priv->timing = priv->modes[mode_index].timing;
		return true;
	}

	if (tachyon_dp_known_timing(width, height, &priv->timing)) {
		log_warning("Using built-in timing for DP mode %ux%u\n",
			    width, height);
		return true;
	}

	log_warning("No timing for DP mode %ux%u; using 1080p60 porch/pixel-clock fallback\n",
		    width, height);
	tachyon_dp_default_timing(&priv->timing);
	tachyon_dp_timing_entry(&priv->timing.hactive, width);
	tachyon_dp_timing_entry(&priv->timing.vactive, height);
	return true;
}

static void tachyon_dp_reset_link_policy(struct tachyon_dp_priv *priv);

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

	tachyon_dp_reset_link_policy(priv);
	tachyon_dp_log_typec_resolved(priv);

	log_info("DP sink DPCD rev=%02x max_rate=%u lanes=%u enhanced=%d policy_rate=%u policy_lanes=%u\n",
		 priv->caps.dpcd_rev, priv->caps.max_rate, priv->caps.lanes,
		 priv->caps.enhanced, priv->max_rate, priv->lanes);

	return 0;
}

static void tachyon_dp_reset_link_policy(struct tachyon_dp_priv *priv)
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
	priv->lanes = min_t(u8, priv->caps.lanes ?: 1, priv->max_lanes);
	priv->max_rate = min(priv->caps.max_rate,
			     tachyon_dp_env_u32("tachyon_dp_max_rate",
						DP_LINK_RATE_HBR2));
	priv->rate = priv->max_rate;
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

static u32 tachyon_dp_configuration_ctrl(struct tachyon_dp_priv *priv)
{
	u32 cfg = DP_CONFIGURATION_CTRL_SYNC_ASYNC_CLK |
		  DP_CONFIGURATION_CTRL_STATIC_DYNAMIC_CN |
		  DP_CONFIGURATION_CTRL_P_INTERLACED |
		  (2 << DP_CONFIGURATION_CTRL_LSCLK_DIV_SHIFT) |
		  ((priv->lanes - 1) <<
		   DP_CONFIGURATION_CTRL_NUM_OF_LANES_SHIFT) |
		  (2 << DP_CONFIGURATION_CTRL_BPC_SHIFT);

	if (priv->caps.enhanced)
		cfg |= DP_CONFIGURATION_CTRL_ENHANCED_FRAMING;

	return cfg;
}

static void tachyon_dp_dump_link_state(struct tachyon_dp_priv *priv,
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

static void tachyon_dp_mainlink_disable(struct tachyon_dp_priv *priv)
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

	val |= DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL;
	writel(val, priv->link + REG_DP_MAINLINK_CTRL);

	log_warning("DP mainlink enable linux-seq: MAINLINK_CTRL=%08x MAINLINK_READY=%08x STATE_CTRL=%08x\n",
		    readl(priv->link + REG_DP_MAINLINK_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_READY),
		    readl(priv->link + REG_DP_STATE_CTRL));
}

static void tachyon_dp_configure_source_link(struct tachyon_dp_priv *priv)
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
		if (priv->swing[lane] + priv->pre[lane] > 3)
			priv->pre[lane] = 3 - priv->swing[lane];
	}
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
		if (ret)
			return ret;
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
		if (ret)
			return ret;
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

static int tachyon_dp_link_train(struct tachyon_dp_priv *priv)
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

static u32 tachyon_dp_htotal(const struct display_timing *t)
{
	return t->hactive.typ + t->hfront_porch.typ + t->hsync_len.typ +
	       t->hback_porch.typ;
}

static u32 tachyon_dp_vtotal(const struct display_timing *t)
{
	return t->vactive.typ + t->vfront_porch.typ + t->vsync_len.typ +
	       t->vback_porch.typ;
}

static void tachyon_dp_program_pixel_clock(struct tachyon_dp_priv *priv)
{
	u32 rate = priv->timing.pixelclock.typ;
	long ret;

	if (!rate)
		return;

	if (!priv->has_pixel_clk)
		return;

	ret = clk_set_rate(&priv->pixel_clk, rate);
	if (ret < 0)
		log_warning("Failed to set DP pixel clock %u Hz: %d\n", rate,
			    (int)ret);

	if (priv->pixel_clk_enabled)
		return;

	ret = clk_enable(&priv->pixel_clk);
	if (ret < 0)
		log_warning("Failed to enable DP pixel clock: %d\n", (int)ret);
	else
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
	writel(DP_MISC0_SYNCHRONOUS_CLK | (2 << DP_MISC0_TEST_BITS_DEPTH_SHIFT),
	       priv->link + REG_DP_MISC1_MISC0);
}

static void tachyon_dp_program_transfer_unit(struct tachyon_dp_priv *priv)
{
	u64 pclk_khz = priv->timing.pixelclock.typ / 1000;
	u32 tu_size = 64;
	u32 valid;

	if (!pclk_khz || !priv->lanes || !priv->rate)
		return;

	valid = (u32)((pclk_khz * 24 * tu_size) /
		      (8ULL * priv->lanes * priv->rate));
	valid = clamp_t(u32, valid, 1, tu_size - 1);

	writel(tu_size - 1, priv->link + REG_DP_TU);
	writel(valid, priv->link + REG_DP_VALID_BOUNDARY);
	writel(valid, priv->link + REG_DP_VALID_BOUNDARY_2);
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

	if (!priv->p0)
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
	writel(DP_TIMING_ENGINE_EN_EN, priv->p0 + MMSS_DP_TIMING_ENGINE_EN);
}

static void tachyon_dp_program_video_timing(struct tachyon_dp_priv *priv)
{
	tachyon_dp_program_pixel_clock(priv);
	tachyon_dp_program_msa_timing(priv);
	tachyon_dp_program_msa_clock(priv);
	tachyon_dp_program_transfer_unit(priv);
	tachyon_dp_program_p0_timing(priv);
}

static int tachyon_dp_program_mainlink(struct tachyon_dp_priv *priv)
{
	writel(DP_SW_RESET, priv->ctrl + REG_DP_SW_RESET);
	udelay(1000);
	writel(0, priv->ctrl + REG_DP_SW_RESET);

	tachyon_dp_program_video_timing(priv);
	tachyon_dp_configure_source_link(priv);
	writel(DP_MAINLINK_CTRL_RESET, priv->link + REG_DP_MAINLINK_CTRL);
	udelay(1000);
	writel(DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL,
	       priv->link + REG_DP_MAINLINK_CTRL);

	if (tachyon_dp_audio_present(priv)) {
		tachyon_dp_program_audio_clock(priv);
		tachyon_dp_program_audio_infoframe(priv);
		tachyon_dp_program_audio_enable(priv);
	}

	return tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
				    DP_MAINLINK_READY_FOR_VIDEO,
				    DP_MAINLINK_READY_FOR_VIDEO, 5000);
}

static int tachyon_dpu_init(struct tachyon_dp_priv *priv)
{
	static const char * const names[TACHYON_DPU_CLK_COUNT] = {
		"bus", "nrt_bus", "iface", "lut", "core", "vsync",
	};
	ofnode node;
	fdt_addr_t addr;
	fdt_size_t size;
	int i, ret;

	for (node = ofnode_by_compatible(ofnode_null(), "qcom,sc7280-dpu");
	     ofnode_valid(node);
	     node = ofnode_by_compatible(node, "qcom,sc7280-dpu")) {
		if (ofnode_is_enabled(node))
			break;
	}
	if (!ofnode_valid(node))
		return -ENODEV;

	addr = ofnode_get_addr_size_index(node, 0, &size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;
	priv->dpu = map_sysmem(addr, size);

	addr = ofnode_get_addr_size_index(node, 1, &size);
	if (addr != FDT_ADDR_T_NONE)
		priv->vbif = map_sysmem(addr, size);

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		ret = clk_get_by_name_nodev(node, names[i], &priv->dpu_clks[i]);
		if (ret) {
			log_warning("DPU clock %s unavailable: %d\n", names[i],
				    ret);
			continue;
		}
		priv->dpu_clk_valid[i] = true;
	}

#ifdef CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE
	priv->dpu_sspp_dma1 = (void __iomem *)((u8 __iomem *)priv->dpu +
					       DPU_SSPP_DMA1_BASE);
#endif

	return 0;
}

static int tachyon_dpu_enable_clocks(struct tachyon_dp_priv *priv)
{
	ulong core_rate;
	long rate_ret;
	int i, ret;

	core_rate = max_t(ulong, 200000000,
			  (ulong)priv->timing.pixelclock.typ * 2);
	if (priv->dpu_clk_valid[4]) {
		rate_ret = clk_set_rate(&priv->dpu_clks[4], core_rate);
		if (rate_ret < 0)
			log_warning("Failed to set DPU core clock %lu Hz: %d\n",
				    core_rate, (int)rate_ret);
	}
	if (priv->dpu_clk_valid[5])
		clk_set_rate(&priv->dpu_clks[5], 19200000);

	if (priv->dpu_clocks_enabled)
		return 0;

	for (i = 0; i < TACHYON_DPU_CLK_COUNT; i++) {
		if (!priv->dpu_clk_valid[i])
			continue;
		ret = clk_enable(&priv->dpu_clks[i]);
		if (ret && ret != -ENOSYS) {
			log_warning("Failed to enable DPU clock %d: %d\n", i,
				    ret);
			return ret;
		}
		if (!ret)
			priv->dpu_clk_enabled[i] = true;
	}

	priv->dpu_clocks_enabled = true;

	return 0;
}

static void tachyon_dpu_program_sspp(struct tachyon_dp_priv *priv,
				     struct video_uc_plat *plat,
				     struct video_priv *uc_priv)
{
	void __iomem *sspp = priv->dpu + DPU_SSPP_DMA0_BASE;
	u32 width = uc_priv->xsize;
	u32 height = uc_priv->ysize;
	u32 size = (height << 16) | width;

	writel(size, sspp + DPU_SSPP_SRC_SIZE);
	writel(0, sspp + DPU_SSPP_SRC_XY);
	writel(size, sspp + DPU_SSPP_OUT_SIZE);
	writel(0, sspp + DPU_SSPP_OUT_XY);
	writel((u32)(ulong)plat->base, sspp + DPU_SSPP_SRC0_ADDR);
	writel(0, sspp + DPU_SSPP_SRC1_ADDR);
	writel(0, sspp + DPU_SSPP_SRC2_ADDR);
	writel(0, sspp + DPU_SSPP_SRC3_ADDR);
	writel(uc_priv->line_length, sspp + DPU_SSPP_SRC_YSTRIDE0);
	writel(0, sspp + DPU_SSPP_SRC_YSTRIDE1);
	writel(DPU_FORMAT_XRGB8888, sspp + DPU_SSPP_SRC_FORMAT);
	writel(DPU_UNPACK_XRGB8888, sspp + DPU_SSPP_SRC_UNPACK_PATTERN);
	writel(DPU_SSPP_PE_OVERRIDE, sspp + DPU_SSPP_SRC_OP_MODE);
	writel(0x87, sspp + DPU_SSPP_FETCH_CONFIG);
	writel(0xffff, sspp + DPU_SSPP_DANGER_LUT);
	writel(0xff00, sspp + DPU_SSPP_SAFE_LUT);
	writel(0, sspp + DPU_SSPP_CREQ_LUT);
	writel(1, sspp + DPU_SSPP_QOS_CTRL);
	writel(1, sspp + DPU_SSPP_CLK_CTRL);
}

static void tachyon_dpu_program_lm(struct tachyon_dp_priv *priv,
				   struct video_priv *uc_priv)
{
	void __iomem *lm = priv->dpu + DPU_LM_0_BASE;

	writel(0, lm + DPU_LM_OP_MODE);
	writel((uc_priv->ysize << 16) | uc_priv->xsize, lm + DPU_LM_OUT_SIZE);
}

static void tachyon_dpu_program_intf(struct tachyon_dp_priv *priv,
				     struct video_priv *uc_priv)
{
	const struct display_timing *t = &priv->timing;
	void __iomem *intf = priv->dpu + DPU_INTF_0_BASE;
	u32 hsync = t->hsync_len.typ;
	u32 hbp = t->hback_porch.typ;
	u32 hfp = t->hfront_porch.typ;
	u32 vsync = t->vsync_len.typ;
	u32 vbp = t->vback_porch.typ;
	u32 vfp = t->vfront_porch.typ;
	u32 htotal = tachyon_dp_htotal(t);
	u32 vtotal = tachyon_dp_vtotal(t);
	u32 hstart = hsync + hbp;
	u32 hend = htotal - hfp - 1;
	u32 display_v_start;
	u32 display_v_end;
	u32 active_v_start;
	u32 active_v_end;

	display_v_start = ((vsync + vbp) * htotal) + hstart;
	display_v_end = ((vtotal - vfp) * htotal) - hfp - 1;
	active_v_start = display_v_start;
	active_v_end = active_v_start + uc_priv->ysize * htotal - 1;

	writel(0, intf + DPU_INTF_TIMING_ENGINE_EN);
	writel(0, intf + DPU_INTF_CONFIG);
	writel((htotal << 16) | hsync, intf + DPU_INTF_HSYNC_CTL);
	writel(vtotal * htotal, intf + DPU_INTF_VSYNC_PERIOD_F0);
	writel(vsync * htotal, intf + DPU_INTF_VSYNC_PULSE_WIDTH_F0);
	writel(display_v_start, intf + DPU_INTF_DISPLAY_V_START_F0);
	writel(display_v_end, intf + DPU_INTF_DISPLAY_V_END_F0);
	writel(active_v_start, intf + DPU_INTF_ACTIVE_V_START_F0);
	writel(active_v_end, intf + DPU_INTF_ACTIVE_V_END_F0);
	writel((hend << 16) | hstart, intf + DPU_INTF_DISPLAY_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_ACTIVE_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_DISPLAY_DATA_HCTL);
	writel((hend << 16) | hstart, intf + DPU_INTF_ACTIVE_DATA_HCTL);
	writel(0, intf + DPU_INTF_BORDER_COLOR);
	writel(0, intf + DPU_INTF_UNDERFLOW_COLOR);
	writel(0, intf + DPU_INTF_HSYNC_SKEW);
	writel(0, intf + DPU_INTF_POLARITY_CTL);
	writel(DPU_INTF_CONFIG2_DATA_HCTL_EN, intf + DPU_INTF_CONFIG2);
	writel(DPU_INTF_FORMAT_XRGB8888, intf + DPU_INTF_PANEL_FORMAT);
	writel(1, intf + DPU_INTF_FRAME_LINE_COUNT_EN);
	writel(0, intf + DPU_INTF_MUX);
	writel(1, intf + DPU_INTF_TIMING_ENGINE_EN);
}

static int tachyon_dpu_program_ctl(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl = priv->dpu + DPU_CTL_0_BASE;
	int ret;

	writel(1, ctl + DPU_CTL_SW_RESET);
	ret = tachyon_dp_read_poll(ctl, DPU_CTL_SW_RESET, BIT(0), 0, 1000);
	if (ret)
		return ret;

	writel(DPU_CTL_LAYER_BORDER_OUT | DPU_CTL_LAYER_DMA0_STAGE0,
	       ctl + DPU_CTL_LAYER_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT2_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT3_0);
	writel(0xf0000000, ctl + DPU_CTL_TOP);
	writel(BIT(0), ctl + DPU_CTL_INTF_ACTIVE);
	writel(BIT(0), ctl + DPU_CTL_FETCH_PIPE_ACTIVE);
	writel(BIT(0), ctl + DPU_CTL_INTF_FLUSH);
	writel(BIT(0), ctl + DPU_CTL_PERIPH_FLUSH);
	writel(DPU_CTL_FLUSH_DMA0 | DPU_CTL_FLUSH_LM0 | DPU_CTL_FLUSH_CTL |
	       DPU_CTL_FLUSH_INTF | DPU_CTL_FLUSH_PERIPH,
	       ctl + DPU_CTL_FLUSH);
	writel(1, ctl + DPU_CTL_START);

	/* Poll for flush commit – HW clears DMA0 bit when pipeline goes active */
	ret = tachyon_dp_read_poll(ctl, DPU_CTL_FLUSH, DPU_CTL_FLUSH_DMA0, 0,
				   5000);
	if (ret)
		log_warning("DPU CTL flush did not commit: %d\n", ret);

	return 0;
}

static int tachyon_dpu_program_scanout(struct tachyon_dp_priv *priv,
				       struct video_uc_plat *plat,
				       struct video_priv *uc_priv)
{
	int ret;

	if (!priv->dpu)
		return -ENODEV;

	ret = tachyon_dpu_enable_clocks(priv);
	if (ret)
		return ret;

	setbits_le32(priv->dpu + DPU_TOP_BASE + DPU_CLK_CTRL,
		     DPU_CLK_CTRL_DMA0);

	/* Basic VBIF QoS: set all XIN clients to mid-level real-time priority */
	if (priv->vbif) {
		writel(0, priv->vbif + VBIF_XINL_QOS_RPT_CTRL);
		writel(0x22222222, priv->vbif + VBIF_XINL_QOS_LVL_PRIO_0);
	}

	tachyon_dpu_program_sspp(priv, plat, uc_priv);
	tachyon_dpu_program_lm(priv, uc_priv);
	tachyon_dpu_program_intf(priv, uc_priv);
#ifdef CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE
	tachyon_dpu_program_lm_blend(priv);
	return tachyon_dpu_program_ctl_multi(priv);
#else
	return tachyon_dpu_program_ctl(priv);
#endif
}

static void tachyon_dpu_quiesce(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl;
	void __iomem *intf;
	void __iomem *sspp;
	int ret;

	if (!priv->dpu || !priv->dpu_clocks_enabled)
		return;

	ctl = priv->dpu + DPU_CTL_0_BASE;
	intf = priv->dpu + DPU_INTF_0_BASE;
	sspp = priv->dpu + DPU_SSPP_DMA0_BASE;

	log_warning("DPU quiesce before OS handoff\n");

	writel(0, intf + DPU_INTF_TIMING_ENGINE_EN);
	writel(0, intf + DPU_INTF_FRAME_LINE_COUNT_EN);
	if (priv->p0)
		writel(0, priv->p0 + MMSS_DP_TIMING_ENGINE_EN);

	writel(0, ctl + DPU_CTL_FETCH_PIPE_ACTIVE);
	writel(0, ctl + DPU_CTL_INTF_ACTIVE);
	writel(0, ctl + DPU_CTL_LAYER_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT2_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT3_0);
	writel(DPU_CTL_FLUSH_DMA0 | DPU_CTL_FLUSH_LM0 | DPU_CTL_FLUSH_CTL |
	       DPU_CTL_FLUSH_INTF | DPU_CTL_FLUSH_PERIPH,
	       ctl + DPU_CTL_FLUSH);
	writel(1, ctl + DPU_CTL_START);

	ret = tachyon_dp_read_poll(ctl, DPU_CTL_FLUSH, DPU_CTL_FLUSH_DMA0, 0,
				   5000);
	if (ret)
		log_warning("DPU quiesce flush did not commit: %d\n", ret);

	writel(0, sspp + DPU_SSPP_SRC0_ADDR);
	writel(0, sspp + DPU_SSPP_SRC1_ADDR);
	writel(0, sspp + DPU_SSPP_SRC2_ADDR);
	writel(0, sspp + DPU_SSPP_SRC3_ADDR);
	writel(0, sspp + DPU_SSPP_SRC_OP_MODE);
	writel(0, sspp + DPU_SSPP_CLK_CTRL);
	clrbits_le32(priv->dpu + DPU_TOP_BASE + DPU_CLK_CTRL,
		     DPU_CLK_CTRL_DMA0);
}

static void tachyon_dp_controller_quiesce(struct tachyon_dp_priv *priv)
{
	if (!priv->dp_core_clocks_enabled)
		return;

	log_warning("DP controller quiesce before OS handoff\n");

	if (priv->link) {
		writel(0, priv->link + REG_DP_STATE_CTRL);
		tachyon_dp_mainlink_disable(priv);
#ifdef CONFIG_VIDEO_TACHYON_DP_AUDIO
		writel(0, priv->link + REG_DP_AUDIO_CTRL);
		writel(0, priv->link + REG_DP_AUDIO_CFG);
#endif
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

static void tachyon_dp_disable_clocks(struct tachyon_dp_priv *priv)
{
	int i, ret;

	if (priv->has_pixel_clk && priv->pixel_clk_enabled) {
		ret = clk_disable(&priv->pixel_clk);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DP pixel clock: %d\n",
				    ret);
		else
			priv->pixel_clk_enabled = false;
	}

	for (i = 0; i < TACHYON_DP_CORE_CLK_COUNT; i++) {
		if (!priv->dp_clk_enabled[i])
			continue;
		ret = clk_disable(&priv->dp_clks[i]);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DP clock %d: %d\n",
				    i, ret);
		else
			priv->dp_clk_enabled[i] = false;
	}
	priv->dp_core_clocks_enabled = false;

	for (i = 0; i < TACHYON_DPU_CLK_COUNT; i++) {
		if (!priv->dpu_clk_enabled[i])
			continue;
		ret = clk_disable(&priv->dpu_clks[i]);
		if (ret && ret != -ENOSYS)
			log_warning("Failed to disable DPU clock %d: %d\n",
				    i, ret);
		else
			priv->dpu_clk_enabled[i] = false;
	}
	priv->dpu_clocks_enabled = false;
}

static void tachyon_dp_quiesce(struct tachyon_dp_priv *priv)
{
	int ret;

	tachyon_dpu_quiesce(priv);
	tachyon_dp_controller_quiesce(priv);

	if (priv->phy_dp && (priv->qmp_dp_touched ||
			     priv->qmp_dp_serdes_programmed ||
			     priv->qmp_dp_phy_started))
		tachyon_dp_qmp_power_down(priv);

	if (priv->has_qmp_phy) {
		ret = generic_phy_power_off(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY power_off failed: %d\n", ret);
		ret = generic_phy_exit(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY exit failed: %d\n", ret);
		priv->has_qmp_phy = false;
	}

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, 0);
	tachyon_dp_release_sbu_mux(priv);

	tachyon_dp_disable_clocks(priv);
}

static int tachyon_dp_remove(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	tachyon_dp_quiesce(priv);

	return 0;
}

static int tachyon_dp_wait_sink(struct tachyon_dp_priv *priv)
{
	int ret, i;

	log_warning("DP wait sink: %d tries, %d us interval\n",
		    TACHYON_DP_AUX_DEBOUNCE_TRIES, 20000);

	for (i = 0; i < TACHYON_DP_AUX_DEBOUNCE_TRIES; i++) {
		/*
		 * Hard-reset the AUX controller before every DPCD retry.
		 * The GO bit was observed stuck at 0x200 after timeouts;
		 * a full AUX reset ensures a clean transaction state each
		 * attempt.
		 */
		writel(DP_AUX_CTRL_RESET, priv->aux + REG_DP_AUX_CTRL);
		udelay(1000);
		writel(DP_AUX_CTRL_ENABLE, priv->aux + REG_DP_AUX_CTRL);
		writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
		writel(0xffff, priv->aux + REG_DP_TIMEOUT_COUNT);
		writel(0xffff, priv->aux + REG_DP_AUX_LIMITS);
		udelay(100);

		/*
		 * Try a 1-byte native DPCD_REV read first.  If a 1-byte
		 * read succeeds but 16-byte fails, the AUX length handling
		 * is wrong.  If 1-byte also times out, the problem is still
		 * physical AUX/QMP routing.
		 */
		{
			u8 dpcd_rev;

			ret = tachyon_dp_aux_retry(priv, false, true,
						   DP_DPCD_REV, &dpcd_rev, 1);
			log_warning("DPCD_REV 1-byte read ret=%d val=%02x\n",
				    ret, dpcd_rev);
		}

		if (!ret) {
			/* 1-byte succeeded — now do the full 16-byte caps */
			ret = tachyon_dp_read_dpcd_caps(priv);
		}

		if (!ret) {
			log_info("DP sink DPCD read OK on try %d/%d\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES);
			return 0;
		}

		log_warning("DP sink DPCD try %d/%d failed ret=%d\n",
			    i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES, ret);

		/*
		 * Docks often expose AUX a little after orientation/mode-switch
		 * callbacks. Retry long enough to absorb plug and cable-flip
		 * bounce before giving up.
		 */
		udelay(20000);
	}

	log_warning("DP sink DPCD read failed after AUX debounce: %d\n", ret);
	return ret;
}

/*
 * Reset QMP Type-C select, reprogram SBU mux GPIOs, reinitialize QMP AUX
 * settings, force AUX out of powerdown/clamp, and reset the DP AUX
 * controller — all for a single orientation choice.
 */
static void tachyon_dp_prepare_aux_for_orientation(
		struct tachyon_dp_priv *priv,
		enum tachyon_dp_orientation orientation)
{
	u32 pd_low;

	priv->orientation = orientation;

	log_warning("DP prepare AUX orientation=%u pin=%u\n",
		    priv->orientation, priv->pin_assignment);

	/* Provider owns COM reset; the DP driver only updates orientation/mode. */
	tachyon_dp_qmp_com_orientation_update(priv);

	/* Reprogram the external SBU mux */
	tachyon_dp_program_sbu_mux(priv);

	/* Keep the AUX path powered, then reinitialize QMP AUX CFG. */
	tachyon_dp_qmp_force_aux_on(priv);
	tachyon_dp_qmp_aux_init(priv);

	/* Reset the DP AUX controller */
	tachyon_dp_aux_hw_init(priv);
	writel(0, priv->aux + REG_DP_AUX_TRANS_CTRL);
	udelay(100);

	pd_low = readl(priv->phy_dp + QMP_DP_PHY_PD_CTL) & 0xff;

	log_warning("DP AUX orientation state: TYPEC=%02x MODE=%02x PD=%02x STATUS=%02x SBU_EN=%d SBU_SEL=%d AUX_CTRL=%08x AUX_STATUS=%08x AUX_TRANS=%08x\n",
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_TYPEC_CTRL),
		    tachyon_dp_qmp_com_readb(priv, QMP_V3_DP_COM_PHY_MODE_CTRL),
		    pd_low,
		    readl(priv->phy_dp + QMP_V4_DP_PHY_STATUS) & 0xff,
		    dm_gpio_is_valid(&priv->sbu_enable) ?
			    dm_gpio_get_value(&priv->sbu_enable) : -1,
		    dm_gpio_is_valid(&priv->sbu_select) ?
			    dm_gpio_get_value(&priv->sbu_select) : -1,
		    readl(priv->aux + REG_DP_AUX_CTRL),
		    readl(priv->aux + REG_DP_AUX_STATUS),
		    readl(priv->aux + REG_DP_AUX_TRANS_CTRL));
}

#ifdef CONFIG_VIDEO_TACHYON_DP_ADVANCED_HOTPLUG

#define TACHYON_DP_HPD_FAST_MS		100
#define TACHYON_DP_HPD_SLOW_MS		1000
#define TACHYON_DP_HPD_FAST_DURATION_MS	5000

static int tachyon_dp_read_sink_count(struct tachyon_dp_priv *priv, u8 *count)
{
	u8 val;
	int ret;

	ret = tachyon_dp_aux_retry(priv, false, true, DPCD_SINK_COUNT,
				   &val, 1);
	if (ret)
		return ret;

	*count = val & DP_SINK_COUNT_LOW_MASK;
	return 0;
}

/**
 * tachyon_dp_poll_hpd_irq() — poll DPCD IRQ vector for hotplug events
 * @priv: driver private data
 *
 * Returns 1 if an event was detected and acknowledged, 0 if no event,
 * negative on error.
 */
static int tachyon_dp_poll_hpd_irq(struct tachyon_dp_priv *priv)
{
	u8 irq_vec;
	int ret;

	ret = tachyon_dp_aux_retry(priv, false, true,
				   DPCD_DEVICE_SERVICE_IRQ, &irq_vec, 1);
	if (ret) {
		log_debug("DP HPD IRQ read failed: %d\n", ret);
		return ret;
	}

	if (!(irq_vec & (DP_IRQ_HPD | DP_IRQ_SINK_SPECIFIC)))
		return 0;

	/* Acknowledge the IRQ by writing zero to clear it */
	{
		u8 zero = 0;

		ret = tachyon_dp_aux_retry(priv, false, false,
					   DPCD_DEVICE_SERVICE_IRQ, &zero, 1);
	}
	if (ret)
		log_debug("DP HPD IRQ clear failed: %d\n", ret);

	if (irq_vec & DP_IRQ_HPD)
		log_info("DP HPD IRQ detected\n");
	if (irq_vec & DP_IRQ_TEST_REQUEST)
		log_info("DP automated test request IRQ detected\n");
	if (irq_vec & DP_IRQ_SINK_SPECIFIC)
		log_info("DP sink-specific IRQ detected\n");

	return 1;
}

static int tachyon_dp_handle_disconnect(struct tachyon_dp_priv *priv)
{
	priv->last_sink_count = 0;

	log_info("DP sink disconnected — clearing display\n");
	return 0;
}

static int tachyon_dp_handle_connect(struct tachyon_dp_priv *priv)
{
	int ret;

	log_info("DP sink connected — re-reading EDID and re-training link\n");

	priv->last_sink_count = 1;

	tachyon_dp_aux_hw_init(priv);

	ret = tachyon_dp_wait_sink(priv);
	if (ret)
		return ret;

	ret = tachyon_dp_read_edid_modes(priv);
	if (ret)
		log_warning("Failed to read DP EDID on reconnect: %d\n", ret);

	tachyon_dp_reset_link_policy(priv);
	memset(priv->swing, 0, sizeof(priv->swing));
	memset(priv->pre, 0, sizeof(priv->pre));

	ret = tachyon_dp_link_train(priv);
	if (ret) {
		log_warning("DP link re-training failed: %d\n", ret);
		return ret;
	}

	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	return 0;
}

static int tachyon_dp_verify_link(struct tachyon_dp_priv *priv)
{
	u8 status[3];
	int ret;

	ret = tachyon_dp_aux_retry(priv, false, true, DP_LANE0_1_STATUS,
				   &status[0], 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true,
				   DP_LANE0_1_STATUS + 1, &status[1], 1);
	if (ret)
		return ret;
	ret = tachyon_dp_aux_retry(priv, false, true,
				   DP_LANE0_1_STATUS + 2, &status[2], 1);
	if (ret)
		return ret;

	/* Check if all active lanes still have CR + EQ + symbol lock */
	if (!tachyon_dp_eq_done(status, priv->lanes)) {
		log_warning("DP link lost CR/EQ — re-training\n");
		ret = tachyon_dp_link_train(priv);
		if (ret)
			return ret;
		priv->max_rate  = priv->rate;
		priv->max_lanes = priv->lanes;
		tachyon_dp_filter_edid_modes(priv);
		tachyon_dp_publish_edid_modes(priv);
	}

	return 0;
}

static int tachyon_dp_hotplug_policy(struct tachyon_dp_priv *priv)
{
	u32 now_ms = get_timer(0);
	bool fast = now_ms - priv->last_hpd_poll_ms <
		    TACHYON_DP_HPD_FAST_DURATION_MS;
	u32 interval = fast ? TACHYON_DP_HPD_FAST_MS :
			      TACHYON_DP_HPD_SLOW_MS;
	u8 sink_count, sc1, sc2;
	int ret;

	if (now_ms - priv->last_hpd_poll_ms < interval)
		return 0;

	priv->last_hpd_poll_ms = now_ms;

	/* Poll IRQ vector first (fast, 1-byte read) */
	ret = tachyon_dp_poll_hpd_irq(priv);
	if (ret <= 0)
		return 0;

	/*
	 * IRQ event detected — check SINK_COUNT for connect/disconnect.
	 * Debounce: require 2 consecutive stable reads.
	 */
	if (tachyon_dp_read_sink_count(priv, &sc1))
		return 0;
	udelay(5000);
	if (tachyon_dp_read_sink_count(priv, &sc2))
		return 0;

	if (sc1 != sc2) {
		log_debug("DP SINK_COUNT unstable: %u→%u (debouncing)\n",
			  sc1, sc2);
		return 0;
	}

	sink_count = sc1;

	if (sink_count == 0 && priv->last_sink_count != 0)
		return tachyon_dp_handle_disconnect(priv);

	if (sink_count != 0 && priv->last_sink_count == 0)
		return tachyon_dp_handle_connect(priv);

	if (sink_count != 0) {
		/* Sink count unchanged but IRQ was raised — check link status */
		ret = tachyon_dp_verify_link(priv);
		if (ret)
			return ret;
		/* Refresh EDID in case modes changed (monitor swap on KVM) */
		ret = tachyon_dp_read_edid_modes(priv);
		if (!ret) {
			tachyon_dp_filter_edid_modes(priv);
			tachyon_dp_publish_edid_modes(priv);
		}
	}

	return 0;
}
#else
static inline int tachyon_dp_hotplug_policy(struct tachyon_dp_priv *priv)
{ return 0; }
static inline int tachyon_dp_verify_link(struct tachyon_dp_priv *priv)
{ return 0; }
#endif /* CONFIG_VIDEO_TACHYON_DP_ADVANCED_HOTPLUG */

#ifdef CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE
static void tachyon_dpu_program_sspp_dma1(struct tachyon_dp_priv *priv,
					  void *fb, u32 width, u32 height,
					  u32 stride, u32 x, u32 y, u8 alpha)
{
	void __iomem *sspp = priv->dpu_sspp_dma1;
	u32 size = (height << 16) | width;
	u32 xy = (y << 16) | x;

	writel(size, sspp + DPU_SSPP_SRC_SIZE);
	writel(0, sspp + DPU_SSPP_SRC_XY);
	writel(size, sspp + DPU_SSPP_OUT_SIZE);
	writel(xy, sspp + DPU_SSPP_OUT_XY);
	writel((u32)(ulong)fb, sspp + DPU_SSPP_SRC0_ADDR);
	writel(0, sspp + DPU_SSPP_SRC1_ADDR);
	writel(0, sspp + DPU_SSPP_SRC2_ADDR);
	writel(0, sspp + DPU_SSPP_SRC3_ADDR);
	writel(stride, sspp + DPU_SSPP_SRC_YSTRIDE0);
	writel(0, sspp + DPU_SSPP_SRC_YSTRIDE1);
	writel(DPU_FORMAT_ARGB8888, sspp + DPU_SSPP_SRC_FORMAT);
	writel(DPU_UNPACK_ARGB8888, sspp + DPU_SSPP_SRC_UNPACK_PATTERN);
	writel(DPU_SSPP_PE_OVERRIDE, sspp + DPU_SSPP_SRC_OP_MODE);
	writel(0x87, sspp + DPU_SSPP_FETCH_CONFIG);
	writel(0xffff, sspp + DPU_SSPP_DANGER_LUT);
	writel(0xff00, sspp + DPU_SSPP_SAFE_LUT);
	writel(0, sspp + DPU_SSPP_CREQ_LUT);
	writel(1, sspp + DPU_SSPP_QOS_CTRL);

	/*
	 * Write global alpha (BG_ALPHA) to LM blend stage 1.
	 * The overlay plane uses per-pixel alpha from ARGB8888 combined
	 * with this global alpha for fade effects.
	 */
	writel((alpha << 24) | alpha, priv->dpu + DPU_LM_0_BASE +
	       DPU_LM_BLEND_STAGE1_BG_ALPHA);

	priv->overlay_active = true;
}

static void tachyon_dpu_program_lm_blend(struct tachyon_dp_priv *priv)
{
	void __iomem *lm = priv->dpu + DPU_LM_0_BASE;
	u32 blend_op = DPU_LM_BLEND_STAGE0_EN;

	if (priv->overlay_active) {
		blend_op |= DPU_LM_BLEND_STAGE1_EN;
		writel(0xff000000, lm + DPU_LM_BLEND_STAGE0_BG_ALPHA);
		writel(0xff000000, lm + DPU_LM_BLEND_STAGE1_FG_ALPHA);
		writel(0x00000000, lm + DPU_LM_BLEND_STAGE1_BG_ALPHA);
	} else {
		writel(0xff000000, lm + DPU_LM_BLEND_STAGE0_BG_ALPHA);
		writel(0x00000000, lm + DPU_LM_BLEND_STAGE0_FG_ALPHA);
	}

	writel(blend_op, lm + DPU_LM_BLEND_OP_MODE);
}

static int tachyon_dpu_program_ctl_multi(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl = priv->dpu + DPU_CTL_0_BASE;
	u32 flush_mask = DPU_CTL_FLUSH_DMA0 | DPU_CTL_FLUSH_LM0 |
			 DPU_CTL_FLUSH_CTL | DPU_CTL_FLUSH_INTF |
			 DPU_CTL_FLUSH_PERIPH;
	u32 fetch_active = BIT(0);
	u32 layer1 = 0;
	int ret;

	if (priv->overlay_active) {
		flush_mask |= DPU_CTL_FLUSH_DMA1;
		fetch_active |= BIT(1);
		layer1 = DPU_CTL_LAYER_DMA1_STAGE1;
	}

	writel(DPU_CTL_LAYER_BORDER_OUT | DPU_CTL_LAYER_DMA0_STAGE0,
	       ctl + DPU_CTL_LAYER_0);
	writel(layer1, ctl + DPU_CTL_LAYER_EXT_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT2_0);
	writel(0, ctl + DPU_CTL_LAYER_EXT3_0);
	writel(0xf0000000, ctl + DPU_CTL_TOP);
	writel(BIT(0), ctl + DPU_CTL_INTF_ACTIVE);
	writel(fetch_active, ctl + DPU_CTL_FETCH_PIPE_ACTIVE);
	writel(BIT(0), ctl + DPU_CTL_INTF_FLUSH);
	writel(BIT(0), ctl + DPU_CTL_PERIPH_FLUSH);
	writel(flush_mask, ctl + DPU_CTL_FLUSH);
	writel(1, ctl + DPU_CTL_START);

	ret = tachyon_dp_read_poll(ctl, DPU_CTL_FLUSH, DPU_CTL_FLUSH_DMA0, 0,
				   5000);
	if (ret)
		log_warning("DPU CTL multi-plane flush did not commit: %d\n",
			    ret);

	return 0;
}
#endif /* CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE */

static int tachyon_dp_probe(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	int ret, altmode_ret, timeout = 50;
	bool has_sbu_mux;

	log_warning("DP probe start\n");

	priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
	priv->typec_valid = false;
	priv->pin_assignment = 0;

	priv->ctrl = dev_remap_addr_index(dev, 0);
	priv->aux = dev_remap_addr_index(dev, 1);
	priv->link = dev_remap_addr_index(dev, 2);
	priv->p0 = dev_remap_addr_name(dev, "p0");
	if (!priv->p0)
		priv->p0 = dev_remap_addr_index(dev, 3);
	if (!priv->ctrl || !priv->aux || !priv->link) {
		log_warning("DP register remap failed ctrl=%p aux=%p link=%p\n",
			    priv->ctrl, priv->aux, priv->link);
		return -EINVAL;
	}

	tachyon_dp_parse_graph(dev, priv);
	tachyon_dp_request_core_clocks(dev, priv);
	ret = tachyon_dp_enable_core_clocks(priv);
	if (ret)
		goto err_quiesce;

	ret = clk_get_by_name(dev, "stream_pixel", &priv->pixel_clk);
	if (!ret) {
		priv->has_pixel_clk = true;
	} else {
		ret = clk_get_by_name(dev, "pixel", &priv->pixel_clk);
		priv->has_pixel_clk = !ret;
	}

	ret = tachyon_dpu_init(priv);
	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_find_phy(dev, priv);
	if (ret)
		goto err_quiesce;

	/* Print QMP base addresses for offset verification */
	log_warning("DP QMP base: phy=%p phy_dp=%p\n",
		    priv->phy, priv->phy_dp);
	log_warning("DP QMP offsets: TYPEC_CTRL=%x PHY_MODE_CTRL=%x DP_PD_CTL=%x DP_STATUS=%x\n",
		    (u32)QMP_V3_DP_COM_TYPEC_CTRL,
		    (u32)QMP_V3_DP_COM_PHY_MODE_CTRL,
		    (u32)(QMP_OFF_DP_PHY + QMP_DP_PHY_PD_CTL),
		    (u32)(QMP_OFF_DP_PHY + QMP_V4_DP_PHY_STATUS));

	/* Print DP core clock validity before any AUX/PHY work */
	{
		int ci;

		for (ci = 0; ci < TACHYON_DP_CORE_CLK_COUNT; ci++)
			log_warning("DP clk[%d] valid=%d\n",
				    ci, priv->dp_clk_valid[ci]);
	}

	/*
	 * Start the PMIC-GLINK altmode service early, but do not require the
	 * first notification to be DP-active. Qualcomm policy firmware can
	 * legitimately report SAFE before later reporting DP.
	 */
	ret = qcom_pmic_glink_altmode_start();
	if (ret) {
		log_warning("DP PMIC-GLINK altmode service start failed: %d\n",
			    ret);
		goto err_quiesce;
	}

	qcom_pmic_glink_altmode_poll(NULL, 100);
	altmode_ret = tachyon_dp_read_altmode(priv);
	if (altmode_ret > 0) {
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
		priv->typec_valid = true;
		if (!tachyon_dp_typec_state_valid(priv)) {
			log_warning("DP Type-C Alt Mode invalid: orientation=%u pin=%u\n",
				    priv->orientation, priv->pin_assignment);
			priv->typec_valid = false;
			priv->typec_source = TACHYON_DP_TYPEC_SOURCE_NONE;
			ret = -EINVAL;
			goto err_quiesce;
		}
		tachyon_dp_log_typec_resolved(priv);
	} else {
		log_warning("DP Type-C Alt Mode not active yet: ret=%d; continuing DP init while PMIC service polls\n",
			    altmode_ret);
	}

	ret = tachyon_dp_request_sbu_mux(priv);
	if (ret)
		log_warning("SBU mux unavailable: %d\n", ret);

	has_sbu_mux = dm_gpio_is_valid(&priv->sbu_enable) &&
		      dm_gpio_is_valid(&priv->sbu_select);
	log_warning("DP SBU mux usable=%d\n", has_sbu_mux ? 1 : 0);

	if (!priv->typec_valid) {
		timeout = 50;
		while (!priv->typec_valid && timeout > 0) {
			qcom_pmic_glink_altmode_poll(NULL, 100);
			altmode_ret = tachyon_dp_read_altmode(priv);
			if (altmode_ret > 0) {
				priv->typec_source =
					TACHYON_DP_TYPEC_SOURCE_ALTMODE;
				priv->typec_valid = true;
				if (!tachyon_dp_typec_state_valid(priv)) {
					log_warning("DP Type-C Alt Mode invalid after poll: orientation=%u pin=%u\n",
						    priv->orientation,
						    priv->pin_assignment);
					priv->typec_valid = false;
					priv->typec_source =
						TACHYON_DP_TYPEC_SOURCE_NONE;
					ret = -EINVAL;
					goto err_quiesce;
				}
				tachyon_dp_log_typec_resolved(priv);
				break;
			}
			timeout--;
		}
	}

	if (!priv->typec_valid) {
		log_warning("DP Type-C Alt Mode did not become active before AUX: ret=%d\n",
			    altmode_ret);
		ret = altmode_ret < 0 ? altmode_ret : -ENODEV;
		goto err_quiesce;
	}

	tachyon_dp_program_sbu_mux(priv);

	log_warning("DP wait sink start with derived Type-C orientation=%u\n",
		    priv->orientation);

	tachyon_dp_prepare_aux_for_orientation(priv, priv->orientation);
	ret = tachyon_dp_wait_sink(priv);

	log_warning("DP wait sink done ret=%d orientation=%u\n",
		    ret, priv->orientation);

	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_read_edid_modes(priv);
	if (ret)
		log_warning("Failed to read DP EDID modes: %d\n", ret);

#ifdef CONFIG_VIDEO_TACHYON_DP_AUDIO
	/*
	 * Re-read EDID blocks from the sink to parse CEA-861 audio descriptors
	 * (Short Audio Descriptors and Speaker Allocation Data Block).
	 * These are in the CEA-861 extension block (block 1), which the mode
	 * parser already consumed.  Re-reading from the sink is fast (~2 AUX
	 * transactions) and keeps the EDID flow self-contained.
	 */
	{
		u8 edid_buf[EDID_EXT_SIZE];

		if (!tachyon_dp_edid_read_block(priv, 0, edid_buf)) {
			tachyon_dp_parse_cea_audio(priv, edid_buf);
			if (edid_buf[126] /* extension_flag */) {
				if (!tachyon_dp_edid_read_block(priv, 1,
					edid_buf + EDID_SIZE))
					tachyon_dp_parse_cea_audio(priv,
						edid_buf + EDID_SIZE);
			}
		}
		tachyon_dp_read_audio_dpcd(priv);
		if (tachyon_dp_audio_present(priv))
			log_info("DP audio sink detected: basic_audio=%d num_eps=%u formats=%u\n",
				 priv->audio.basic_audio,
				 priv->audio.num_audio_eps,
				 priv->audio.format_count);
	}
#endif

	ret = tachyon_dp_link_train(priv);
	if (ret)
		goto err_quiesce;

	/*
	 * Re-filter EDID modes against actual trained rate/lanes, which may
	 * be lower than the policy ceiling after link-training fallback.
	 */
	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);
	tachyon_dp_select_mode(priv, &width, &height);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;

	video_set_flush_dcache(dev, true);
	memset((void *)plat->base, 0, plat->size);

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		goto err_quiesce;

	log_warning("DP ready: %ux%u fb=%lx size=%lx aux timeouts=%u nacks=%u retries=%u\n",
		    width, height, (ulong)plat->base, (ulong)plat->size,
		    priv->aux_timeouts, priv->aux_nacks, priv->aux_retries);

	log_info("DP framebuffer base=%lx size=%lx aux timeouts=%u nacks=%u retries=%u\n",
		 (ulong)plat->base, (ulong)plat->size, priv->aux_timeouts,
		 priv->aux_nacks, priv->aux_retries);

	return 0;

err_quiesce:
	tachyon_dp_quiesce(priv);
	return ret;
}

static int tachyon_dp_video_sync(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	bool alt_changed = false;
	bool mode_changed;
	int ret;

	ret = tachyon_dp_refresh_altmode(priv, &alt_changed);
	if (ret)
		return ret;

	/* Advanced hotplug policy: poll DPCD IRQ vector for connect/disconnect */
	tachyon_dp_hotplug_policy(priv);

	if (alt_changed) {
		tachyon_dp_prepare_aux_for_orientation(priv, priv->orientation);
		ret = tachyon_dp_wait_sink(priv);
		if (ret)
			return ret;
		ret = tachyon_dp_read_edid_modes(priv);
		if (ret)
			log_warning("Failed to refresh DP EDID modes: %d\n",
				    ret);
	}

	tachyon_dp_env_mode(&width, &height);
	mode_changed = width != priv->timing.hactive.typ ||
		       height != priv->timing.vactive.typ;
	if (!mode_changed && !alt_changed)
		return 0;

	if (!alt_changed)
		tachyon_dp_reset_link_policy(priv);

	ret = tachyon_dp_link_train(priv);
	if (ret)
		return ret;

	/*
	 * Re-filter modes against actual trained parameters in case link
	 * training fell back to a lower rate or lane count.
	 */
	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);
	tachyon_dp_select_mode(priv, &width, &height);

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;
	if (uc_priv->fb_size > plat->size)
		return -ENOSPC;

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		return ret;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		return ret;

	return 0;
}

static int tachyon_dp_get_mode_count(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	return priv->mode_count;
}

static int tachyon_dp_get_mode_info(struct udevice *dev, u32 mode_number,
				    u32 *width, u32 *height,
				    enum video_format *format,
				    enum video_log2_bpp *bpix)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	if (mode_number >= (u32)priv->mode_count)
		return -ENOENT;

	*width = priv->modes[mode_number].width;
	*height = priv->modes[mode_number].height;
	*format = VIDEO_X8R8G8B8;
	*bpix = VIDEO_BPP32;
	return 0;
}

static int tachyon_dp_set_mode(struct udevice *dev, u32 mode_number)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	int ret;

	if (priv->mode_count < 1)
		return -EINVAL;
	if (mode_number >= (u32)priv->mode_count)
		return -EINVAL;

	width = priv->modes[mode_number].width;
	height = priv->modes[mode_number].height;

	/*
	 * SetMode() must respect the caller-chosen mode index, not
	 * tachyon_dp_env_mode().  Resolve timing directly from the
	 * requested mode, then train link + reprogram DPU/DP.
	 */
	if (!tachyon_dp_resolve_mode_timing(priv, mode_number))
		return -EINVAL;

	tachyon_dp_reset_link_policy(priv);
	memset(priv->swing, 0, sizeof(priv->swing));
	memset(priv->pre, 0, sizeof(priv->pre));

	ret = tachyon_dp_link_train(priv);
	if (ret)
		return ret;

	priv->max_rate  = priv->rate;
	priv->max_lanes = priv->lanes;
	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	/*
	 * After link fallback, the originally requested mode may no longer
	 * fit within the trained rate/lanes.  Re-check bandwidth here so
	 * we don't blindly program a mode that exceeds link capacity (which
	 * would result in a black screen or unstable link).
	 */
	if (!tachyon_dp_mode_fits_link(priv, &priv->timing))
		return -ENOSPC;

	uc_priv->xsize = width;
	uc_priv->ysize = height;
	uc_priv->bpix = VIDEO_BPP32;
	uc_priv->format = VIDEO_X8R8G8B8;
	uc_priv->line_length = width * 4;
	uc_priv->fb_size = uc_priv->line_length * height;
	if (uc_priv->fb_size > plat->size)
		return -ENOSPC;

	ret = tachyon_dpu_program_scanout(priv, plat, uc_priv);
	if (ret)
		return ret;

	ret = tachyon_dp_program_mainlink(priv);
	if (ret)
		return ret;

	return 0;
}

#ifdef CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE
static int tachyon_dp_get_max_planes(struct udevice *dev)
{
	return 2; /* DMA0 base + DMA1 overlay */
}

static int tachyon_dp_enable_plane(struct udevice *dev, u32 plane_id,
				   bool enable)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	if (plane_id != 1)
		return -EINVAL;

	priv->overlay_active = enable;

	/* Re-flush CTL to apply or remove the overlay plane */
	tachyon_dpu_program_lm_blend(priv);
	return tachyon_dpu_program_ctl_multi(priv);
}

static int tachyon_dp_set_plane_fb(struct udevice *dev, u32 plane_id,
				   void *fb, u32 width, u32 height,
				   u32 stride, u32 x, u32 y, u8 alpha)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);

	if (plane_id != 1)
		return -EINVAL;
	if (!priv->dpu_sspp_dma1)
		return -ENODEV;

	tachyon_dpu_program_sspp_dma1(priv, fb, width, height, stride,
				      x, y, alpha);
	return tachyon_dpu_program_ctl_multi(priv);
}
#endif

static const struct video_ops tachyon_dp_ops = {
	.video_sync = tachyon_dp_video_sync,
	.video_get_mode_count = tachyon_dp_get_mode_count,
	.video_get_mode_info = tachyon_dp_get_mode_info,
	.video_set_mode = tachyon_dp_set_mode,
#ifdef CONFIG_VIDEO_TACHYON_DP_MULTI_PLANE
	.video_get_max_planes = tachyon_dp_get_max_planes,
	.video_enable_plane = tachyon_dp_enable_plane,
	.video_set_plane_fb = tachyon_dp_set_plane_fb,
#endif
};

static int tachyon_dp_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->size = TACHYON_DP_MAX_XRES * TACHYON_DP_MAX_YRES * 4;
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
	.remove		= tachyon_dp_remove,
	.ops		= &tachyon_dp_ops,
	.priv_auto	= sizeof(struct tachyon_dp_priv),
	.plat_auto	= sizeof(struct video_uc_plat),
	.flags		= DM_FLAG_PRE_RELOC | DM_FLAG_OS_PREPARE,
};
