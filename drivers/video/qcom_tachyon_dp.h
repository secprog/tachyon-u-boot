/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Shared declarations for the Qualcomm Tachyon DisplayPort driver family:
 * the dp_display parent (qcom_tachyon_dp.c) plus its controller modules
 * (aux/panel/link/ctrl) and the DPU + PHY child devices.  Register macros,
 * enums, the shared per-device session struct (tachyon_dp_priv), and the
 * cross-module prototypes.  Not a UAPI header -- internal to drivers/video/.
 */
#ifndef __QCOM_TACHYON_DP_H__
#define __QCOM_TACHYON_DP_H__

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <clk.h>
#include <generic-phy.h>
#include <asm/gpio.h>
#include <fdtdec.h>

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

/* DP AUX controller registers. Offsets mirror Linux drm/msm/dp/dp_reg.h
 * for the SC7280/QCM6490 DP controller where available.
 */
#define REG_DP_SW_RESET			0x010
#define DP_SW_RESET			BIT(0)
#define REG_DP_INTR_STATUS		0x020
#define REG_DP_AUX_CTRL			0x030
#define DP_AUX_CTRL_ENABLE		BIT(0)
#define DP_AUX_CTRL_RESET		BIT(1)
#define REG_DP_DP_HPD_INT_STATUS	0x04
#define DP_DP_HPD_STATE_STATUS_MASK	0xe0000000
#define DP_DP_HPD_STATE_STATUS_SHIFT	29
#define DP_DP_HPD_STATE_STATUS_CONNECTED	(2 << DP_DP_HPD_STATE_STATUS_SHIFT)
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
/*
 * MAINLINK_CTRL FLUSH_MODE[24:23] = 3 (SDE_PERIPH_UPDATE): make MSA/SDP updates
 * latch into the live stream.  Linux msm_dp_setup_peripheral_flush() ORs this
 * into MAINLINK_CTRL (dp_reg.h DP_MAINLINK_FLUSH_MODE_SDE_PERIPH_UPDATE =
 * FIELD_PREP(GENMASK(24,23),3)); without it the freshly-programmed MSA may
 * never reach the active stream -> trained link, READY_FOR_VIDEO, but blank.
 */
#define DP_MAINLINK_CTRL_FLUSH_MODE	(3 << 23)
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
#define REG_DP_DELAY_START_LINK_SHIFT	16
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
#define DPCD_DOWNSTREAMPORT_PRESENT	0x00005
#define DPCD_DOWN_STREAM_PORT_COUNT	0x00007
#define DPCD_LANE0_1_STATUS		0x00202
#define DPCD_LANE2_3_STATUS		0x00203
#define DPCD_LANE_ALIGN_STATUS		0x00204
#define DPCD_SET_POWER			0x00600
#define DP_SET_POWER_D0			0x01
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
/*
 * DP controller built-in Test Pattern Generator (BIST) — p0 region.  In Linux
 * (msm_dp_panel_tpg_enable) the p0 MMSS_DP_INTF_* timing engine + these BIST/TPG
 * registers are ONLY used to drive an internal checkered pattern, bypassing the
 * DPU entirely.  Normal DPU-sourced video never enables the p0 timing engine.
 */
#define MMSS_DP_BIST_ENABLE		0x000
#define DP_BIST_ENABLE_DPBIST_EN	BIT(0)
#define MMSS_DP_TPG_MAIN_CONTROL	0x060
#define DP_TPG_CHECKERED_RECT_PATTERN	0x100
#define MMSS_DP_TPG_VIDEO_CONFIG	0x064
#define DP_TPG_VIDEO_CONFIG_BPP_8BIT	0x01
#define DP_TPG_VIDEO_CONFIG_RGB		0x04
/*
 * DP_P0CLK_DSC_DTO (p0 region, abs 0xae9107c = p0 + 0x07c).  Controls MDP->DP
 * backpressure: OVERRIDE_ACK (bit1) = 0 enables backpressure so the DP TX pulls
 * pixel data from the MDP; = 1 stops the data flow.  OVERRIDE_ACK_VALUE (bit2).
 * Without this the DP controller stays in SEND_IDLE_PATTERN and never emits real
 * video, so the sink sees "No Signal".
 */
#define MMSS_DP_P0CLK_DSC_DTO		0x07c
#define DP_P0CLK_DSC_DTO_OVERRIDE_ACK	BIT(1)
#define DP_P0CLK_DSC_DTO_OVERRIDE_ACK_VALUE BIT(2)

/* DP_STATE_CTRL link commands */
#define DP_STATE_CTRL_PUSH_IDLE		BIT(8)
/* DP_MAINLINK_READY status bits */
#define DP_MAINLINK_READY_IDLE_PATTERNS_SENT BIT(1)

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
/*
 * SSPP software pixel-extension registers.  When SRC_OP_MODE.PE_OVERRIDE is set
 * the HW uses these REQ_PIXELS counts for the per-line fetch instead of deriving
 * them from SRC_SIZE.  Linux ALWAYS pairs PE_OVERRIDE with REQ_PIXELS=(h<<16)|w
 * (dpu_hw_sspp setup_pe_config); leaving them at reset 0 makes the pipe fetch 0
 * pixels -> the staged plane is transparent/black even though composited.
 */
#define DPU_SSPP_SW_PIX_EXT_C0_LR	0x100
#define DPU_SSPP_SW_PIX_EXT_C0_TB	0x104
#define DPU_SSPP_SW_PIX_EXT_C0_REQ_PIXELS 0x108
/*
 * SSPP multirect op-mode.  Linux always programs this to RECT_SOLO (0) for a
 * single (non-SmartDMA) pipe (dpu_hw_sspp.c:147-178 via dpu_plane.c).  DMA0 on
 * sc7280 has SMART_DMA_V2, so a prior UEFI/XBL GOP stage could leave it in a
 * multirect mode across a warm path -> RECT0 would not output.  Force SOLO.
 */
#define DPU_SSPP_MULTIRECT_OPMODE	0x170
/*
 * 8-level CREQ QoS LUT (DPU >=v4 / sc7280 core_major_ver=7): the real CREQ LUT is
 * at 0x74/0x78, NOT the legacy 4-level 0x68.  With QOS_CTRL danger/safe enabled
 * but the 8-level CREQ LUT left at 0, the pipe can be credit-starved.  Linux
 * sc7180_qos_linear = 0x0011222222335777 -> LUT_0(0x74)=0x22335777,
 * LUT_1(0x78)=0x00112222.
 */
#define DPU_SSPP_CREQ_LUT_0		0x074
#define DPU_SSPP_CREQ_LUT_1		0x078
/*
 * SSPP SRC_FORMAT for XRGB8888: the per-component bpc fields must all be BPC8
 * (=3).  The old 0x000236a8 declared sub-8bpc components, so the DPU fetched
 * our 8bpc XRGB8888 framebuffer as if it were lower depth -> wrong pixels into
 * the DP INTF.  0x000236ff has all bpc fields = 3 (matches Linux dpu_hw_sspp
 * src_format for XRGB8888: mdp_format.c BPC8A/BPC8/BPC8/BPC8, BPC8=3).
 */
#define DPU_FORMAT_XRGB8888		0x000236ff
#define DPU_UNPACK_XRGB8888		0x03020001

#define DPU_LM_OP_MODE			0x000
#define DPU_LM_OUT_SIZE		0x004
/*
 * LM stage-0 blend registers (SC7280 sc7180_lm_sblk: blendstage_base[0]=0x20,
 * relative to the mixer base).  LM_BLEND0_OP=stage_base+0x00, CONST_ALPHA=+0x04
 * (Linux dpu_hw_lm.c).  Values for one opaque foreground plane over background:
 *   OP          = FG_ALPHA_FG_CONST(0) | BG_ALPHA_BG_CONST(1<<8)        = 0x100
 *   CONST_ALPHA = (bg>>8) | ((fg>>8)<<16), fg=0xffff bg=0               = 0x00ff0000
 *   LM_OP_MODE  = BIT(stage) for DPU_STAGE_0                            = BIT(1)
 */
#define DPU_LM_BLEND0_OP		0x020
#define DPU_LM_BLEND0_CONST_ALPHA	0x024
#define DPU_LM_BLEND0_OP_VAL		0x00000100
#define DPU_LM_BLEND0_CONST_ALPHA_VAL	0x00ff0000
#define DPU_LM_OP_MODE_STAGE0		BIT(1)
/* LM border color (mixer output where no pipe composites) — RED diagnostic. */
#define DPU_LM_BORDER_COLOR_0		0x008
#define DPU_LM_BORDER_COLOR_1		0x010
#define DPU_LM_BORDER_RED_0		0x00000fff	/* R=0xfff | G<<16=0 */
#define DPU_LM_BORDER_RED_1		0x0fff0000	/* B=0 | A=0xfff<<16 */

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
/*
 * CTL_LAYER stage field for SSPP_DMA0.  Linux dpu_hw_ctl_setup_blendstage
 * (dpu_hw_ctl.c:532-552) loops i=0..stages and writes mix=(i+1) for the pipe at
 * stage_cfg->stage[i].  The fullscreen plane is placed at pstate->stage =
 * DPU_STAGE_0 (dpu_plane.c:838) and DPU_STAGE_0 = 1 (DPU_STAGE_BASE=0 is the
 * background), so stage_cfg->stage[1] = DMA0 -> i=1 -> mix = (1+1) = 2 ->
 * (2<<18).  ctl_blend_config[SSPP_DMA0] = {idx 0, shift 18}; BORDER_OUT always set.
 * This MUST agree with LM_OP_MODE = (1<<DPU_STAGE_0) = BIT(1) and the BLEND0
 * block at LM+0x20.  (mix=1 selects DPU_STAGE_BASE, which has NO blend block, so
 * the pipe is never composited -> mixer emits border only = black screen with a
 * valid signal.  An earlier change to 1<<18 was this exact regression.)
 */
#define DPU_CTL_LAYER_DMA0_STAGE0	(2 << 18)
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
#define DPU_INTF_FRAME_COUNT		0x0ac
#define DPU_INTF_LINE_COUNT		0x0b0
#define DPU_INTF_MUX			0x25c
#define DPU_INTF_STATUS			0x26c
#define DPU_INTF_CONFIG2_DATA_HCTL_EN	BIT(4)
/* INTF_CONFIG active-region enables for a DP interface (Linux dpu_hw_intf
 * dp_intf path ORs both into INTF_CONFIG; without them the active HCTL/VCTL
 * data window is not emitted -> sink sees blanking only = "No Signal"). */
#define DPU_INTF_CFG_ACTIVE_H_EN	BIT(29)
#define DPU_INTF_CFG_ACTIVE_V_EN	BIT(30)
/*
 * INTF PANEL_FORMAT for RGB888: bpc fields all BPC8(=3) plus 0x21<<8.  Old
 * 0x000021a8 declared sub-8bpc; 0x0000213f = BPC8 for g/b/r + 0x21<<8 (matches
 * Linux dpu_hw_intf_setup_timing_engine panel_format for RGB888).
 */
#define DPU_INTF_FORMAT_XRGB8888	0x0000213f

#define VBIF_XINL_QOS_RPT_CTRL		0xd00
#define VBIF_XINL_QOS_LVL_PRIO_0	0xd20
/* VBIF xin (AXI master) init registers — XBL/Linux program these; U-Boot must too. */
#define VBIF_OUT_AXI_AMEMTYPE_CONF0	0x160	/* xins 0-7,  3-bit memtype each */
#define VBIF_OUT_AXI_AMEMTYPE_CONF1	0x164	/* xins 8-13 */
#define VBIF_XIN_PND_ERR		0x190
#define VBIF_XIN_SRC_ERR		0x194
#define VBIF_XIN_CLR_ERR		0x19c
#define VBIF_XIN_HALT_CTRL0		0x200	/* write BIT(xin) to halt */
#define VBIF_XIN_HALT_CTRL1		0x204	/* read BIT(xin): 1 = halted */
#define VBIF_DMA0_XIN_ID		1

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

enum tachyon_dp_hpd_state {
	TACHYON_DP_HPD_UNKNOWN,
	TACHYON_DP_HPD_DISCONNECTED,
	TACHYON_DP_HPD_CONNECTED,
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
	/* true only once the DP link/mainlink has actually been set up; gates the
	 * link-register teardown in quiesce (touching link regs on a never-trained
	 * link faults the DP controller -> hard reset). */
	bool dp_link_up;
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
	bool aux_enabled;
	bool aux_xfers_enabled;
	enum tachyon_dp_hpd_state hpd_state;
	ulong last_pmic_poll_ms;
};

/*
 * Generic poll helper shared across the DP modules (aux/link/ctrl/phy).
 * static inline in the header avoids a cross-file symbol.
 */
static inline int tachyon_dp_read_poll(void __iomem *base, u32 reg, u32 mask,
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

/* Implemented in qcom_tachyon_dp.c, called by the PHY module. */
int tachyon_dp_pin_assignment_lanes(struct tachyon_dp_priv *priv);

/* Implemented in qcom_tachyon_dp.c, called by the LINK module. */
bool tachyon_dp_typec_state_valid(struct tachyon_dp_priv *priv);

/* PHY module (qcom_tachyon_dp_phy.c) entry points called by the parent. */
int tachyon_dp_qmp_configure(struct tachyon_dp_priv *priv);
u8 tachyon_dp_qmp_com_readb(struct tachyon_dp_priv *priv, u32 reg);
void tachyon_dp_qmp_com_orientation_update(struct tachyon_dp_priv *priv);
void tachyon_dp_qmp_force_aux_on(struct tachyon_dp_priv *priv);
int tachyon_dp_qmp_program_tx(struct tachyon_dp_priv *priv);
int tachyon_dp_qmp_program_dp_phy(struct tachyon_dp_priv *priv);
void tachyon_dp_qmp_aux_init(struct tachyon_dp_priv *priv);
void tachyon_dp_qmp_power_down(struct tachyon_dp_priv *priv);
bool tachyon_dp_qmp_phy_ready(struct tachyon_dp_priv *priv);
u8 tachyon_dp_qmp_status_low(struct tachyon_dp_priv *priv);
u8 tachyon_dp_qmp_phy_mode(struct tachyon_dp_priv *priv);

/* DPU module (qcom_tachyon_dp_dpu.c) entry points called by the parent. */
struct video_uc_plat;
struct video_priv;
int tachyon_dpu_init(struct tachyon_dp_priv *priv);
int tachyon_dpu_program_scanout(struct tachyon_dp_priv *priv,
				struct video_uc_plat *plat,
				struct video_priv *uc_priv);
void tachyon_dpu_quiesce(struct tachyon_dp_priv *priv);
void tachyon_dp_dump_dpu_state(struct tachyon_dp_priv *priv);

/*
 * Parent helpers (qcom_tachyon_dp.c) called by the DPU module.  htotal/vtotal
 * will become the CTRL module later but stay in the parent .c for now.
 */
struct display_timing;
u32 tachyon_dp_htotal(const struct display_timing *t);
u32 tachyon_dp_vtotal(const struct display_timing *t);
bool tachyon_dp_env_bool(const char *name);

/* AUX module (qcom_tachyon_dp_aux.c) entry points called by the parent. */
void tachyon_dp_aux_hw_init(struct tachyon_dp_priv *priv);
bool tachyon_dp_hw_hpd_connected(struct tachyon_dp_priv *priv);
int tachyon_dp_aux_retry_mot(struct tachyon_dp_priv *priv, bool i2c,
			     bool read, bool mot, u32 addr, u8 *buf,
			     size_t len);
int tachyon_dp_aux_retry(struct tachyon_dp_priv *priv, bool i2c,
			 bool read, u32 addr, u8 *buf, size_t len);
int tachyon_dp_edid_read_block(struct tachyon_dp_priv *priv, u8 block,
			       u8 *buf);

/*
 * Parent helpers (qcom_tachyon_dp.c) called by the PANEL module.  These stay in
 * the parent .c (also used by the future CTRL/LINK code) but are un-static'd so
 * the panel mode-build / DPCD-caps path can reach them.
 */
u32 tachyon_dp_env_u32(const char *name, u32 fallback);
bool tachyon_dp_env_has_u32(const char *name);
void tachyon_dp_reset_link_policy(struct tachyon_dp_priv *priv);
void tachyon_dp_log_typec_resolved(struct tachyon_dp_priv *priv);

/* LINK module (qcom_tachyon_dp_link.c) entry points called by the parent/CTRL. */
u8 tachyon_dp_bw_code(u32 rate);
void tachyon_dp_dump_link_state(struct tachyon_dp_priv *priv,
				const char *tag);
bool tachyon_dp_cr_done(u8 *status, u8 lanes);
bool tachyon_dp_eq_done(u8 *status, u8 lanes);
u8 tachyon_dp_train_set(struct tachyon_dp_priv *priv, u8 lane);
void tachyon_dp_apply_adjust(struct tachyon_dp_priv *priv, u8 *adj);
void tachyon_dp_log_lanes(struct tachyon_dp_priv *priv, const char *when);

/* PANEL module (qcom_tachyon_dp_panel.c) entry points called by the parent. */
struct display_timing;
struct timing_entry;
bool tachyon_dp_valid_resolution(u32 width, u32 height);
bool tachyon_dp_mode_fits_link(struct tachyon_dp_priv *priv,
			       const struct display_timing *timing);
void tachyon_dp_env_mode(u32 *width, u32 *height);
void tachyon_dp_timing_entry(struct timing_entry *entry, u32 value);
void tachyon_dp_fill_timing(struct display_timing *timing,
			    u32 pixelclock, u32 hactive, u32 hfp,
			    u32 hsync, u32 hbp, u32 vactive,
			    u32 vfp, u32 vsync, u32 vbp,
			    enum display_flags flags);
void tachyon_dp_default_timing(struct display_timing *timing);
bool tachyon_dp_cvt_timing(u32 width, u32 height, u32 hz,
			   struct display_timing *timing);
bool tachyon_dp_known_timing(u32 width, u32 height,
			     struct display_timing *timing);
int tachyon_dp_read_edid_modes(struct tachyon_dp_priv *priv);
void tachyon_dp_filter_edid_modes(struct tachyon_dp_priv *priv);
void tachyon_dp_publish_edid_modes(struct tachyon_dp_priv *priv);
void tachyon_dp_select_mode(struct tachyon_dp_priv *priv,
			    u32 *width, u32 *height);
bool tachyon_dp_resolve_mode_timing(struct tachyon_dp_priv *priv,
				    int mode_index);
void tachyon_dp_sink_power_on(struct tachyon_dp_priv *priv);
int tachyon_dp_read_dpcd_caps(struct tachyon_dp_priv *priv);

#endif /* __QCOM_TACHYON_DP_H__ */
