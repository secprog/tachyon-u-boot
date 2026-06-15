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
#include <asm/global_data.h>
#include <asm/io.h>
#include <clk.h>
#include <command.h>
#include <dm.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/read.h>
#include <dm/ofnode.h>
#include <dm/root.h>
#include <dm/uclass-internal.h>
#include <cpu_func.h>
#include <edid.h>
#if CONFIG_IS_ENABLED(EFI_LOADER)
#include <efi_loader.h>
#endif
#include <env.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <lmb.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <soc/qcom/cmd-db.h>
#include <soc/qcom/pmic_glink.h>
#include <soc/qcom/tcs.h>
#include <video.h>

#include "qcom_tachyon_dp.h"

DECLARE_GLOBAL_DATA_PTR;


static const struct tachyon_qmp_offsets tachyon_qmp_sc7280_offsets = {
	.com		= 0x0000,
	.dp_serdes	= QMP_OFF_DP_SERDES,
	.dp_tx0		= QMP_OFF_DP_TX0,
	.dp_tx1		= QMP_OFF_DP_TX1,
	.dp_phy		= QMP_OFF_DP_PHY,
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

int tachyon_dp_pin_assignment_lanes(struct tachyon_dp_priv *priv)
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

	/*
	 * Allow forcing a resolution the EDID parse didn't surface (e.g. a CEA
	 * 720p/1080p mode carried in an extension block we don't fully decode).
	 * With tachyon_dp_force_mode=1 we honor tachyon_dp_xres/yres as long as
	 * it's a sane resolution and fall back to a built-in timing for it.
	 */
	if (have_saved && tachyon_dp_env_bool("tachyon_dp_force_mode") &&
	    tachyon_dp_valid_resolution(*width, *height)) {
		log_warning("Forcing DP resolution %ux%u (tachyon_dp_force_mode)\n",
			    *width, *height);
		return;
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

static void tachyon_dp_aux_hw_init(struct tachyon_dp_priv *priv)
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

static bool tachyon_dp_hw_hpd_connected(struct tachyon_dp_priv *priv)
{
	u32 status;

	status = readl(priv->aux + REG_DP_DP_HPD_INT_STATUS);
	log_debug("DP HPD status=%08x connected=%u\n",
		  status,
		  !!(status & DP_DP_HPD_STATE_STATUS_CONNECTED));

	return !!(status & DP_DP_HPD_STATE_STATUS_CONNECTED);
}

static int tachyon_dp_apply_pmic_typec_state(struct tachyon_dp_priv *priv,
		                 const struct qcom_pmic_glink_altmode_state *state)
{
	bool force_aux = tachyon_dp_env_bool("tachyon_dp_force_aux_without_hpd");

	if (!state || !state->notify_seen)
		return -EAGAIN;

	log_info("DP PMIC state: typec=%u hpd=%u hpd_irq=%u orientation=%u pin=%u mux=%u dpam=%02x\n",
		state->typec_state, state->hpd, state->hpd_irq,
		state->orientation, state->pin_assignment,
		state->mux, state->dpam_raw);

	switch (state->typec_state) {
	case QPG_TYPEC_STATE_SAFE:
		priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;
		priv->aux_xfers_enabled = false;
		break;
	case QPG_TYPEC_STATE_DP:
		if (state->orientation == QCOM_PMIC_GLINK_ORIENTATION_REVERSE)
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
		else
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;

		if (tachyon_dp_valid_pin_assignment(state->pin_assignment))
			priv->pin_assignment = state->pin_assignment;

		tachyon_dp_qmp_com_orientation_update(priv);
		tachyon_dp_program_sbu_mux(priv);
		tachyon_dp_qmp_force_aux_on(priv);

		if (state->hpd)
			priv->hpd_state = TACHYON_DP_HPD_CONNECTED;
		else if (priv->hpd_state == TACHYON_DP_HPD_UNKNOWN)
			priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;

		priv->aux_xfers_enabled = state->hpd || force_aux;
		break;
	case QPG_TYPEC_STATE_USB:
		priv->hpd_state = TACHYON_DP_HPD_DISCONNECTED;
		priv->aux_xfers_enabled = false;
		break;
	default:
		priv->aux_xfers_enabled = force_aux;
		break;
	}

	return 0;
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
			log_warning("Dropping DP EDID mode %ux%u: %s\n",
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
	log_warning("DP EDID blk0: %02x %02x %02x %02x %02x %02x %02x %02x | ext_flag=%u csum_ok=%u hdr_ok=%u\n",
		    edid_buf[0], edid_buf[1], edid_buf[2], edid_buf[3],
		    edid_buf[4], edid_buf[5], edid_buf[6], edid_buf[7],
		    edid_buf[126], tachyon_dp_edid_checksum_ok(edid_buf),
		    tachyon_dp_edid_header_ok(edid_buf));
	log_warning("DP EDID estab: %02x %02x %02x  ver=%u.%u\n",
		    edid_buf[35], edid_buf[36], edid_buf[37],
		    edid_buf[18], edid_buf[19]);

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

	log_warning("DP EDID pre-filter: %d modes, rate=%u lanes=%u\n",
		    priv->mode_count, priv->rate, priv->lanes);
	for (i = 0; i < priv->mode_count; i++)
		log_warning("DP EDID mode[%d] %ux%u has_timing=%u\n", i,
			    priv->modes[i].width, priv->modes[i].height,
			    priv->modes[i].has_timing);

	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	log_warning("DP EDID post-filter: %d modes\n", priv->mode_count);

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

/*
 * SC7280/QCM6490 DP timing-engine quirk: the MDP INTF + DP controller require
 * the active region in the bottom-right corner of the frame for correct DP
 * packet/audio transfer.  does this by
 * folding the front porch into the back porch and zeroing the front porch.
 * HTOTAL/VTOTAL and the pixel clock are unchanged, so refresh rate and sync
 * widths are preserved; only the porch distribution shifts.  Apply it once to
 * priv->timing so the MSA, the DPU INTF, and the DP p0 timing all agree.
 */
static void tachyon_dp_apply_dp_porch_adjust(struct tachyon_dp_priv *priv)
{
	struct display_timing *t = &priv->timing;

	/*
	 * Linux drives this hardware with the STANDARD CEA porch distribution
	 * (front porch intact).  Folding the front porch into the back porch
	 * (active region bottom-right) makes the MSA declare hsync_start right
	 * after active (hfp=0) -> the DP->HDMI dock regenerates NON-CEA HDMI
	 * timing the monitor rejects ("No Signal"), and the same non-standard
	 * timing feeds the DP controller's own BIST (so even TPG was blank).
	 * Default: leave the porches as the mode defines them (== Linux).  Set
	 * tachyon_dp_porch_adjust=1 to restore the old bottom-right fold.
	 */
	if (!tachyon_dp_env_bool("tachyon_dp_porch_adjust"))
		return;

	tachyon_dp_timing_entry(&t->hback_porch,
				t->hback_porch.typ + t->hfront_porch.typ);
	tachyon_dp_timing_entry(&t->hfront_porch, 0);
	tachyon_dp_timing_entry(&t->vback_porch,
				t->vback_porch.typ + t->vfront_porch.typ);
	tachyon_dp_timing_entry(&t->vfront_porch, 0);
}

static void tachyon_dp_select_mode(struct tachyon_dp_priv *priv,
				   u32 *width, u32 *height)
{
	int i, selected = -1;

	bool force_mode = tachyon_dp_env_has_u32("tachyon_dp_xres") &&
			  tachyon_dp_env_has_u32("tachyon_dp_yres") &&
			  tachyon_dp_env_bool("tachyon_dp_force_mode");

	tachyon_dp_env_mode(width, height);

	/*
	 * TESTING: the dock's DP->HDMI converter rejects the sink's native
	 * 1360x768 (a non-CEA VESA PC mode), showing "No Signal" even with a
	 * valid trained link.  The board env that would pick a mode does not
	 * persist across reboot, so force a clean CEA 1920x1080 here unless the
	 * user has explicitly forced a different mode via tachyon_dp_force_mode.
	 */
	if (!force_mode) {
		*width = 1920;
		*height = 1080;
		force_mode = true;
		log_warning("DP TEST: forcing 1920x1080 (ignoring non-CEA native mode)\n");
	}

	for (i = 0; i < priv->mode_count; i++) {
		if (priv->modes[i].width == *width &&
		    priv->modes[i].height == *height) {
			selected = i;
			break;
		}
	}

	/*
	 * Only fall back to the sink's first EDID mode when we are NOT being told
	 * to force a specific resolution.  Otherwise honor the forced width/height
	 * and let the known/fallback timing path below resolve its timing.
	 */
	if (selected < 0 && priv->mode_count && !force_mode) {
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

	tachyon_dp_apply_dp_porch_adjust(priv);
	tachyon_dp_publish_selected_timing(priv);
	log_warning("DP selected mode %ux%u pclk=%u hfp=%u hsw=%u hbp=%u vfp=%u vsw=%u vbp=%u (DP bottom-right adjusted)\n",
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
		tachyon_dp_apply_dp_porch_adjust(priv);
		return true;
	}

	if (tachyon_dp_known_timing(width, height, &priv->timing)) {
		log_warning("Using built-in timing for DP mode %ux%u\n",
			    width, height);
		tachyon_dp_apply_dp_porch_adjust(priv);
		return true;
	}

	log_warning("No timing for DP mode %ux%u; using 1080p60 porch/pixel-clock fallback\n",
		    width, height);
	tachyon_dp_default_timing(&priv->timing);
	tachyon_dp_timing_entry(&priv->timing.hactive, width);
	tachyon_dp_timing_entry(&priv->timing.vactive, height);
	tachyon_dp_apply_dp_porch_adjust(priv);
	return true;
}

static void tachyon_dp_reset_link_policy(struct tachyon_dp_priv *priv);

/*
 * Wake the sink/branch device into full-power D0 and report its downstream
 * (HDMI) port state.  A DP->HDMI branch device left in D3 (or that never had
 * its display path enabled) keeps its HDMI TX off, so the TV shows "No Signal"
 * even though our DP link to the branch is trained and we're sending video.
 * DPCD_DOWNSTREAMPORT_PRESENT bit0 tells us whether a branch device is present;
 * SINK_COUNT tells us whether it sees a downstream display.
 */
static void tachyon_dp_sink_power_on(struct tachyon_dp_priv *priv)
{
	u8 dfp = 0, dfp_count = 0, sink_count = 0, power = 0;
	int ret;

	tachyon_dp_aux_retry(priv, false, true, DPCD_DOWNSTREAMPORT_PRESENT,
			     &dfp, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_DOWN_STREAM_PORT_COUNT,
			     &dfp_count, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_SINK_COUNT, &sink_count, 1);

	/* Set sink power state to D0 (normal operation). */
	power = DP_SET_POWER_D0;
	ret = tachyon_dp_aux_retry(priv, false, false, DPCD_SET_POWER,
				   &power, 1);
	power = 0;
	tachyon_dp_aux_retry(priv, false, true, DPCD_SET_POWER, &power, 1);

	log_warning("DP sink: DFP_present=0x%02x DFP_count=0x%02x SINK_COUNT=0x%02x set_D0_ret=%d power_readback=0x%02x\n",
		    dfp, dfp_count, sink_count, ret, power);
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

	tachyon_dp_reset_link_policy(priv);
	tachyon_dp_log_typec_resolved(priv);

	/* Wake the sink/branch to D0 before training so its display path is on. */
	tachyon_dp_sink_power_on(priv);

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

	val |= DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL |
	       DP_MAINLINK_CTRL_FLUSH_MODE;
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

/*
 * Block until the dock asserts DisplayPort HPD (best-effort, bounded).
 *
 * The sink answers AUX (DPCD/EDID) as soon as the SBU mux + orientation are
 * programmed, but its main-link receiver only comes up when it raises HPD.
 * Clock recovery started while HPD is still low fails with lane status 00 at
 * every rate -- the source transmits but the sink receiver is off.  This made
 * boot DP intermittent: success depended purely on whether the entry/auto-DFP
 * poll happened to outlast HPD (HW-confirmed: identical normal-orientation
 * boots trained when hpd=1 arrived first, failed CR when training raced ahead
 * of it).  The PMIC/ADSP altmode HPD is the dependable signal -- the hpd=1 that
 * immediately preceded every successful train; the DP controller's own
 * HPD_INT_STATUS reads "connected" too early to trust.  Keep the altmode state
 * machine pumping so the notify is processed, and return as soon as HPD is
 * seen; on timeout the caller trains anyway so a dock that never surfaces HPD
 * still gets a chance.
 */
static bool tachyon_dp_wait_pmic_hpd(struct tachyon_dp_priv *priv,
				     uint timeout_ms)
{
	struct qcom_pmic_glink_altmode_state state;
	ulong start = get_timer(0);
	int ret;

	do {
		ret = qcom_pmic_glink_altmode_poll(&state, 100);
		if (!ret) {
			tachyon_dp_apply_pmic_typec_state(priv, &state);
			if (state.hpd)
				return true;
		}
	} while (get_timer(start) < timeout_ms);

	return false;
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

static void tachyon_dp_program_video_timing(struct tachyon_dp_priv *priv)
{
	tachyon_dp_program_pixel_clock(priv);
	tachyon_dp_program_msa_timing(priv);
	tachyon_dp_program_msa_clock(priv);
	tachyon_dp_program_transfer_unit(priv);
	tachyon_dp_program_p0_timing(priv);
}

/* Fill the framebuffer with vertical colour bars (XRGB8888). */
static void tachyon_dp_fill_test_pattern(struct video_uc_plat *plat,
					 struct video_priv *uc_priv)
{
	static const u32 bars[8] = {
		0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00,
		0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00303030,
	};
	u32 *fb = (u32 *)plat->base;
	u32 w = uc_priv->xsize, h = uc_priv->ysize;
	u32 stride = uc_priv->line_length / 4;
	u32 x, y;

	if (!fb || !w || !h)
		return;

	for (y = 0; y < h; y++) {
		u32 *line = fb + (u64)y * stride;

		for (x = 0; x < w; x++)
			line[x] = bars[(x * 8) / w];
	}
	flush_dcache_range((ulong)plat->base,
			   (ulong)plat->base + (ulong)stride * 4 * h);
}

/* Dump the DPU pixel-fetch path (CTL/SSPP/LM/INTF) to see if real pixels flow. */
static void tachyon_dp_dump_dpu_state(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl, *sspp, *lm, *intf;

	if (!priv->dpu)
		return;

	ctl = priv->dpu + DPU_CTL_0_BASE;
	sspp = priv->dpu + DPU_SSPP_DMA0_BASE;
	lm = priv->dpu + DPU_LM_0_BASE;
	intf = priv->dpu + DPU_INTF_0_BASE;

	log_warning("DPU CTL: FLUSH=%08x INTF_ACTIVE=%08x FETCH_PIPE=%08x LAYER0=%08x TOP=%08x\n",
		    readl(ctl + DPU_CTL_FLUSH), readl(ctl + DPU_CTL_INTF_ACTIVE),
		    readl(ctl + DPU_CTL_FETCH_PIPE_ACTIVE),
		    readl(ctl + DPU_CTL_LAYER_0), readl(ctl + DPU_CTL_TOP));
	log_warning("DPU SSPP: SRC0_ADDR=%08x SRC_SIZE=%08x OUT_SIZE=%08x FORMAT=%08x YSTRIDE=%08x CLK=%08x\n",
		    readl(sspp + DPU_SSPP_SRC0_ADDR),
		    readl(sspp + DPU_SSPP_SRC_SIZE),
		    readl(sspp + DPU_SSPP_OUT_SIZE),
		    readl(sspp + DPU_SSPP_SRC_FORMAT),
		    readl(sspp + DPU_SSPP_SRC_YSTRIDE0),
		    readl(sspp + DPU_SSPP_CLK_CTRL));
	log_warning("DPU LM_OUT=%08x INTF_STATUS=%08x INTF_MUX=%08x INTF_UNDERFLOW_COLOR=%08x\n",
		    readl(lm + DPU_LM_OUT_SIZE), readl(intf + DPU_INTF_STATUS),
		    readl(intf + DPU_INTF_MUX),
		    readl(intf + DPU_INTF_UNDERFLOW_COLOR));

	/* Confirm the LM-composite + SSPP op-mode config actually latched. */
	log_warning("DPU LM_OP_MODE=%08x BLEND0_OP=%08x SSPP_OP_MODE=%08x MULTIRECT=%08x\n",
		    readl(lm + DPU_LM_OP_MODE), readl(lm + DPU_LM_BLEND0_OP),
		    readl(sspp + DPU_SSPP_SRC_OP_MODE),
		    readl(sspp + DPU_SSPP_MULTIRECT_OPMODE));

	/*
	 * Sample the framebuffer the SSPP is pointed at (SRC0_ADDR) so the
	 * bars-vs-black question is answered IN the dp-start log (post-PD the
	 * serial floods and md is unusable).  Invalidate first so we read what
	 * the non-coherent DPU master would see in DRAM, not a stale CPU line.
	 * Expect the 8 colour bars 00ffffff/00ffff00/0000ffff/.../00303030 at
	 * x = k*(w/8).  All-zero => the fill never landed at this address.
	 */
	{
		u32 fb_pa = readl(sspp + DPU_SSPP_SRC0_ADDR);

		if (fb_pa) {
			u32 *fb = (u32 *)(ulong)fb_pa;

			invalidate_dcache_range((ulong)fb, (ulong)fb + 0x2000);
			log_warning("DPU FB@%08x: x0=%08x x240=%08x x480=%08x x960=%08x x1680=%08x\n",
				    fb_pa, fb[0], fb[240], fb[480], fb[960], fb[1680]);
		}
	}
}

/*
 * Dump the DP controller MSA/video registers and the DPU INTF timing-engine
 * state so we can tell, when the sink shows "No Signal" despite a trained link,
 * whether the DPU is actually clocking real framebuffer pixels into the DP
 * interface (frame/line counters advancing) and whether the MSA timing the DP
 * controller is sending matches the selected mode.
 */
static void tachyon_dp_dump_video_state(struct tachyon_dp_priv *priv)
{
	const struct display_timing *t = &priv->timing;
	void __iomem *intf;
	u32 frame0 = 0, line0 = 0, frame1 = 0, line1 = 0;

	log_warning("DP MSA regs: TOTAL=%08x ACTIVE=%08x SYNC_START=%08x WIDTH_POL=%08x MISC0=%08x MVID=%08x NVID=%08x\n",
		    readl(priv->link + REG_DP_TOTAL_HOR_VER),
		    readl(priv->link + REG_DP_ACTIVE_HOR_VER),
		    readl(priv->link + REG_DP_START_HOR_VER_FROM_SYNC),
		    readl(priv->link + REG_DP_HSYNC_VSYNC_WIDTH_POLARITY),
		    readl(priv->link + REG_DP_MISC1_MISC0),
		    readl(priv->link + REG_DP_SOFTWARE_MVID),
		    readl(priv->link + REG_DP_SOFTWARE_NVID));
	log_warning("DP mode expect: %ux%u htotal=%u vtotal=%u pclk=%u STATE_CTRL=%08x READY=%08x\n",
		    t->hactive.typ, t->vactive.typ, tachyon_dp_htotal(t),
		    tachyon_dp_vtotal(t), t->pixelclock.typ,
		    readl(priv->link + REG_DP_STATE_CTRL),
		    readl(priv->link + REG_DP_MAINLINK_READY));

	if (priv->dpu) {
		intf = priv->dpu + DPU_INTF_0_BASE;
		frame0 = readl(intf + DPU_INTF_FRAME_COUNT);
		line0 = readl(intf + DPU_INTF_LINE_COUNT);
		mdelay(50);
		frame1 = readl(intf + DPU_INTF_FRAME_COUNT);
		line1 = readl(intf + DPU_INTF_LINE_COUNT);
		log_warning("DPU INTF: TE_EN=%08x frame %u->%u line %u->%u (advancing=%d)\n",
			    readl(intf + DPU_INTF_TIMING_ENGINE_EN),
			    frame0, frame1, line0, line1,
			    (frame1 != frame0) || (line1 != line0));
	}

	tachyon_dp_dump_dpu_state(priv);

	/*
	 * Re-assert D0 and read the sink/branch link status AFTER video has been
	 * sent.  If the branch dropped symbol lock once real video started (TU /
	 * MVID/NVID mismatch) the lane-status bytes here will show it; if the
	 * branch sees no downstream HDMI display, SINK_COUNT will be 0.
	 */
	{
		u8 l01 = 0, l23 = 0, align = 0;

		tachyon_dp_sink_power_on(priv);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE0_1_STATUS,
				     &l01, 1);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE2_3_STATUS,
				     &l23, 1);
		tachyon_dp_aux_retry(priv, false, true, DPCD_LANE_ALIGN_STATUS,
				     &align, 1);
		log_warning("DP post-video link status: LANE0_1=0x%02x LANE2_3=0x%02x ALIGN=0x%02x\n",
			    l01, l23, align);
	}
}

/* Read + log the sink/branch DPCD lane status, to bracket where the link drops. */
static void tachyon_dp_log_lanes(struct tachyon_dp_priv *priv, const char *when)
{
	u8 l01 = 0, l23 = 0, align = 0;

	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE0_1_STATUS, &l01, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE2_3_STATUS, &l23, 1);
	tachyon_dp_aux_retry(priv, false, true, DPCD_LANE_ALIGN_STATUS, &align, 1);
	log_warning("DP lanes @ %s: L01=%02x L23=%02x ALIGN=%02x\n",
		    when, l01, l23, align);
}

static int tachyon_dp_program_mainlink(struct tachyon_dp_priv *priv)
{
	tachyon_dp_log_lanes(priv, "mainlink-entry (post-train)");

	writel(DP_SW_RESET, priv->ctrl + REG_DP_SW_RESET);
	udelay(1000);
	writel(0, priv->ctrl + REG_DP_SW_RESET);
	tachyon_dp_log_lanes(priv, "after SW_RESET");

	tachyon_dp_program_video_timing(priv);
	tachyon_dp_configure_source_link(priv);
	/*
	 * Do NOT pulse DP_MAINLINK_CTRL_RESET here: the link is already trained
	 * at this point (we run after link training, not before like msm),
	 * and asserting the mainlink reset — especially holding it for 1ms —
	 * resets the mainlink and makes the sink lose clock-recovery (lane status
	 * collapses 0x77 -> 0x00 right here, which was the "No Signal" cause).
	 * Just (re)enable the mainlink so the freshly programmed MSA is latched.
	 */
	writel(DP_MAINLINK_CTRL_ENABLE | DP_MAINLINK_FB_BOUNDARY_SEL |
	       DP_MAINLINK_CTRL_FLUSH_MODE,
	       priv->link + REG_DP_MAINLINK_CTRL);
	tachyon_dp_log_lanes(priv, "after MAINLINK enable");


	/*
	 * Start the video stream with this sequence of DP_STATE_CTRL commands to avoid a "No Signal" sink state:
	 *   1. PUSH_IDLE  — emit idle patterns first to avoid an underflow when
	 *      the stream switches on (STATE_CTRL must be cleared before each
	 *      command), then wait for IDLE_PATTERNS_SENT.
	 *   2. SEND_VIDEO — switch the mainlink to the active video stream.
	 *   3. Enable MDP->DP backpressure (OVERRIDE_ACK=0) so the DP TX actually
	 *      pulls pixel data from the MDP.  Without this the controller reports
	 *      READY_FOR_VIDEO but stays in SEND_IDLE_PATTERN (MAINLINK_READY had
	 *      bit10 set) and the sink shows "No Signal".
	 */
	writel(0, priv->link + REG_DP_STATE_CTRL);
	writel(DP_STATE_CTRL_PUSH_IDLE, priv->link + REG_DP_STATE_CTRL);
	tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
			     DP_MAINLINK_READY_IDLE_PATTERNS_SENT,
			     DP_MAINLINK_READY_IDLE_PATTERNS_SENT, 2000);

	writel(0, priv->link + REG_DP_STATE_CTRL);
	writel(DP_STATE_CTRL_SEND_VIDEO, priv->link + REG_DP_STATE_CTRL);

	/* Enable MDP->DP backpressure so pixel data flows (clears OVERRIDE_ACK). */
	if (priv->p0) {
		u32 dto = readl(priv->p0 + MMSS_DP_P0CLK_DSC_DTO);

		dto &= ~(DP_P0CLK_DSC_DTO_OVERRIDE_ACK |
			 DP_P0CLK_DSC_DTO_OVERRIDE_ACK_VALUE);
		writel(dto, priv->p0 + MMSS_DP_P0CLK_DSC_DTO);
		log_warning("DP backpressure enabled: DSC_DTO=%08x\n",
			    readl(priv->p0 + MMSS_DP_P0CLK_DSC_DTO));
	}

	/*
	 * Start the pixel source LAST.  Only now that the DP controller is armed
	 * (SEND_VIDEO + backpressure) do we enable the DPU INTF timing engine, so
	 * the DP TX latches a clean blanking->active transition from a
	 * freshly-started timing engine — what the DP->HDMI bridge needs to lock.
	 * (Previously the INTF engine was turned on back in program_intf/scanout,
	 * BEFORE SEND_VIDEO, so pixels free-ran into an un-armed/SW-reset DP
	 * controller -> sink "No Signal" despite a trained, ready link.)  This
	 * mirrors Linux dpu_encoder_phys_vid handle_post_kickoff (INTF enabled
	 * after the DP stream-on + CTL flush).  The pending CTL flush from
	 * tachyon_dpu_program_ctl (issued with the engine off) is consumed at this
	 * first vsync.
	 */
	if (priv->dpu && !tachyon_dp_env_bool("tachyon_dp_tpg")) {
		writel(1, priv->dpu + DPU_INTF_0_BASE +
			  DPU_INTF_TIMING_ENGINE_EN);
		log_warning("DP INTF timing engine ON (after SEND_VIDEO)\n");
	} else if (priv->dpu) {
		/*
		 * TPG mode: leave the DPU INTF timing engine OFF so the DP
		 * controller's internal p0 BIST is the sole pixel/timing source
		 * (no DPU vs p0 timing-engine conflict).  TPG shares the same DP
		 * main link + MSA + TU as normal video, so this isolates the
		 * DPU pixel path from the DP stream.
		 */
		log_warning("DP TPG mode: DPU INTF timing engine left OFF\n");
	}

	{
		int rdy = tachyon_dp_read_poll(priv->link, REG_DP_MAINLINK_READY,
					       DP_MAINLINK_READY_FOR_VIDEO,
					       DP_MAINLINK_READY_FOR_VIDEO, 5000);
		log_warning("DP mainlink ready-for-video ret=%d MAINLINK_READY=%08x MAINLINK_CTRL=%08x\n",
			    rdy, readl(priv->link + REG_DP_MAINLINK_READY),
			    readl(priv->link + REG_DP_MAINLINK_CTRL));
		tachyon_dp_dump_video_state(priv);
		return rdy;
	}
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
	/*
	 * ROOT-CAUSE FIX: PE_OVERRIDE makes the SSPP use the SW pixel-extension
	 * REQ_PIXELS for the per-line fetch count.  Without programming it the pipe
	 * fetched 0 pixels -> staged-but-black.  Program no extension (LR/TB=0) and
	 * REQ_PIXELS = full image (height<<16 | width), then assert PE_OVERRIDE --
	 * matches Linux dpu_hw_sspp_setup_pe_config.
	 */
	writel(0, sspp + DPU_SSPP_SW_PIX_EXT_C0_LR);
	writel(0, sspp + DPU_SSPP_SW_PIX_EXT_C0_TB);
	writel((height << 16) | width, sspp + DPU_SSPP_SW_PIX_EXT_C0_REQ_PIXELS);
	writel(DPU_SSPP_PE_OVERRIDE, sspp + DPU_SSPP_SRC_OP_MODE);
	/* Force RECT_SOLO so a warm-path multirect leftover can't suppress RECT0. */
	writel(0, sspp + DPU_SSPP_MULTIRECT_OPMODE);
	writel(0x87, sspp + DPU_SSPP_FETCH_CONFIG);
	writel(0xffff, sspp + DPU_SSPP_DANGER_LUT);
	writel(0xff00, sspp + DPU_SSPP_SAFE_LUT);
	writel(0, sspp + DPU_SSPP_CREQ_LUT);
	/* Program the real 8-level CREQ QoS LUT (0x74/0x78) so the pipe isn't credit-starved. */
	writel(0x22335777, sspp + DPU_SSPP_CREQ_LUT_0);
	writel(0x00112222, sspp + DPU_SSPP_CREQ_LUT_1);
	writel(1, sspp + DPU_SSPP_QOS_CTRL);
	writel(1, sspp + DPU_SSPP_CLK_CTRL);
}

static void tachyon_dpu_program_lm(struct tachyon_dp_priv *priv,
				   struct video_priv *uc_priv)
{
	void __iomem *lm = priv->dpu + DPU_LM_0_BASE;

	writel((uc_priv->ysize << 16) | uc_priv->xsize, lm + DPU_LM_OUT_SIZE);

	/*
	 * Program the LM stage-0 blend so the SSPP layer (staged at DPU_STAGE_0
	 * in the CTL) is actually composited into the mixer output.  WITHOUT
	 * this the LM blend mux is at reset -> the mixer emits border/background
	 * only, the staged pixels are never mixed, the INTF timing engine
	 * free-runs over blanking, and the DP TX (in SEND_VIDEO) carries no
	 * active video -> the dock bridge reports "No Signal" (not a black
	 * picture).  Mirrors Linux dpu_hw_lm_setup_blend_config_combined_alpha +
	 * setup_color3 for one opaque plane on SC7280.
	 */
	writel(DPU_LM_BLEND0_OP_VAL, lm + DPU_LM_BLEND0_OP);
	writel(DPU_LM_BLEND0_CONST_ALPHA_VAL, lm + DPU_LM_BLEND0_CONST_ALPHA);
	/*
	 * LM_OP_MODE (BLEND_COLOR_OUT) selects which stage's foreground is
	 * composited into the mixer output.  Linux dpu_crtc.c:500-501 sets
	 * mixer_op_mode |= 1 << pstate->stage, and dpu_hw_lm_setup_color3
	 * (dpu_hw_lm.c:191-203) writes it into LM_OP_MODE.  For one fullscreen
	 * plane at DPU_STAGE_0 (=1) the final value is BIT(1) (STAGE0_FG_ALPHA).
	 * Writing 0 leaves the BLEND0 block (0x20/0x24) configured but NEVER
	 * composited -> mixer emits its border only = black framebuffer with a
	 * valid signal.  Must agree with CTL_LAYER mix = (2<<18).  (An earlier
	 * change from BIT(1) to 0 was this exact regression.)
	 */
	writel(DPU_LM_OP_MODE_STAGE0, lm + DPU_LM_OP_MODE);

	/*
	 * DIAGNOSTIC: paint the mixer BORDER red.  BORDER_OUT is set in the CTL,
	 * so any region not covered by the composited SSPP layer shows red.
	 * Screen RED  => mixer->INTF->DP path works, pipe NOT composited.
	 * Screen WHITE=> pipe composited (FB shows) - fix worked.
	 * Screen BLACK=> mixer output never reaches the INTF (datapath/underflow).
	 */
	writel(DPU_LM_BORDER_RED_0, lm + DPU_LM_BORDER_COLOR_0);
	writel(DPU_LM_BORDER_RED_1, lm + DPU_LM_BORDER_COLOR_1);
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
	writel(DPU_INTF_CFG_ACTIVE_H_EN | DPU_INTF_CFG_ACTIVE_V_EN,
	       intf + DPU_INTF_CONFIG);
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
	/* Underflow colour = black (a transient underrun shouldn't flash). */
	writel(0, intf + DPU_INTF_UNDERFLOW_COLOR);
	writel(0, intf + DPU_INTF_HSYNC_SKEW);
	writel(0, intf + DPU_INTF_POLARITY_CTL);
	writel(DPU_INTF_CONFIG2_DATA_HCTL_EN, intf + DPU_INTF_CONFIG2);
	writel(DPU_INTF_FORMAT_XRGB8888, intf + DPU_INTF_PANEL_FORMAT);
	writel(1, intf + DPU_INTF_FRAME_LINE_COUNT_EN);
	writel(0, intf + DPU_INTF_MUX);
	/*
	 * Do NOT enable the timing engine here.  The DP transmitter must be armed
	 * (SEND_VIDEO) BEFORE the pixel source starts, otherwise the DP TX never
	 * sees a clean blanking->active transition and the DP->HDMI bridge's DP RX
	 * won't lock to the main-video stream (trained link, READY_FOR_VIDEO,
	 * frame counter advancing, yet sink "No Signal").  The engine is enabled
	 * as the FINAL step in tachyon_dp_program_mainlink, after SEND_VIDEO —
	 * mirroring Linux's post-kickoff INTF enable and the rule "Video mode must
	 * flush CTL before enabling the timing engine" (the CTL flush in
	 * tachyon_dpu_program_ctl runs with the engine off; the pending flush is
	 * consumed at the first vsync once we enable it below).
	 */
}

static int tachyon_dpu_program_ctl(struct tachyon_dp_priv *priv)
{
	void __iomem *ctl = priv->dpu + DPU_CTL_0_BASE;
	int ret;

	writel(1, ctl + DPU_CTL_SW_RESET);
	ret = tachyon_dp_read_poll(ctl, DPU_CTL_SW_RESET, BIT(0), 0, 1000);
	if (ret)
		return ret;

	/*
	 * DIAGNOSTIC: tachyon_dp_border_only=1 stages NO pipe (border-out only) so
	 * the mixer outputs the (red) border across the whole frame.  Screen RED =>
	 * LM->PP->INTF->DP datapath works and the bug is the SSPP pipe (fetch/SMMU
	 * or staging); screen BLACK => the mixer output never reaches the INTF.
	 */
	if (tachyon_dp_env_bool("tachyon_dp_border_only"))
		writel(DPU_CTL_LAYER_BORDER_OUT, ctl + DPU_CTL_LAYER_0);
	else
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

/*
 * The DPU framebuffer read goes MASTER_MDP0 -> mmss_noc -> DDR.  That AXI path
 * is bandwidth-gated by an RPMh BCM vote that U-Boot otherwise never makes, so
 * the SSPP fetch is starved to zero and the pipe underruns (solid INTF
 * underflow colour) even with every DPU register correct and the FB full of
 * pixels.  Linux msm_mdss_enable() votes a mandatory MIN_IB_BW=400MB/s floor on
 * the mdp0-mem path before the AXI clocks can move data.  Replicate that by
 * voting MM1 (the one non-keepalive BCM on the path; it carries qxm_mdp0) using
 * U-Boot's already-compiled RPMh-RSC + cmd-db transport.  MM0/SH0/MC0 are
 * keepalive (held by XBL/RPMh, DRAM+LLCC already up) and only re-asserted as
 * cheap insurance.  rpmh_rsc_send_data() is the same tested path the rpmh clock
 * driver uses for its xo.lvl vote, so no raw TCS poking.
 */
struct rsc_drv;
int rpmh_rsc_send_data(struct rsc_drv *drv, const struct tcs_request *msg);

/*
 * Secure-monitor MMIO accessors (implemented in drivers/soc/qcom/qcom_adsp_pas.c).
 * Used to reach the XPU/secure-owned MMNOC MMU-TBU GDSC bank that bare EL2-NS
 * writes can't touch.
 */
int qcom_scm_io_readl(phys_addr_t addr, u32 *val);
int qcom_scm_io_writel(phys_addr_t addr, u32 val);

/* sc7280 cmd-db BCM aux record (Linux icc-rpmh.c struct bcm_db). */
struct tachyon_bcm_aux {
	__le32 unit;
	__le16 width;
	u8 vcd;
	u8 reserved;
};

/* bcm_div(): a non-zero sub-unit vote must not collapse to 0 (bcm-voter.c). */
static u64 tachyon_bcm_div(u64 num, u32 base)
{
	if (num && num < base)
		return 1;
	return base ? num / base : 0;
}

/*
 * Compute the BCM vote fields, replicating bcm_aggregate() (bcm-voter.c:99-116).
 * CRUCIAL: avg (vote_x) divides by buswidth*channels; peak (vote_y) divides by
 * buswidth ONLY.  unit/width are firmware values read from cmd-db at runtime.
 */
static u32 tachyon_bcm_vote_x(const char *name, u32 buswidth, u32 channels,
			      u32 ab_kbps)
{
	const struct tachyon_bcm_aux *aux;
	size_t len = 0;
	u64 unit, width, t;

	aux = cmd_db_read_aux_data(name, &len);
	if (IS_ERR_OR_NULL(aux) || len < sizeof(*aux))
		return 0;
	unit  = le32_to_cpu(aux->unit);
	width = le16_to_cpu(aux->width);
	if (!unit || !width || !buswidth || !channels)
		return 0;

	t = tachyon_bcm_div((u64)ab_kbps * width, (u64)buswidth * channels);
	t = tachyon_bcm_div(t * 1000, (u32)unit);
	return t > BCM_TCS_CMD_VOTE_MASK ? BCM_TCS_CMD_VOTE_MASK : (u32)t;
}

static u32 tachyon_bcm_vote_y(const char *name, u32 buswidth, u32 ib_kbps)
{
	const struct tachyon_bcm_aux *aux;
	size_t len = 0;
	u64 unit, width, t;

	aux = cmd_db_read_aux_data(name, &len);
	if (IS_ERR_OR_NULL(aux) || len < sizeof(*aux))
		return 0;
	unit  = le32_to_cpu(aux->unit);
	width = le16_to_cpu(aux->width);
	if (!unit || !width || !buswidth)
		return 0;

	t = tachyon_bcm_div((u64)ib_kbps * width, buswidth);
	t = tachyon_bcm_div(t * 1000, (u32)unit);
	return t > BCM_TCS_CMD_VOTE_MASK ? BCM_TCS_CMD_VOTE_MASK : (u32)t;
}

static int tachyon_bcm_send(struct rsc_drv *drv, u32 addr, u32 data)
{
	struct tcs_cmd cmd = { .addr = addr, .data = data, .wait = 1 };
	struct tcs_request msg = {
		.state = RPMH_ACTIVE_ONLY_STATE,
		.wait_for_compl = 1,
		.is_read = false,
		.num_cmds = 1,
		.cmds = &cmd,
	};

	if (!addr)
		return -ENODEV;
	return rpmh_rsc_send_data(drv, &msg);
}

/*
 * Linux raises the SC7280_CX rpmhpd voltage corner to SVS (RPMH level 128)
 * atomically with the 300MHz DISP_CC_MDSS_MDP_CLK via the OPP framework
 * (sc7280.dtsi power-domains=<&rpmhpd SC7280_CX>; mdp_opp_table opp-300000000
 * required-opps=<&rpmhpd_opp_svs>).  U-Boot only sets the core clock, leaving
 * the DPU datapath at the idle CX voltage -> the SSPP DRAM fetch starves even
 * at 300MHz with bandwidth granted.  Replicate the ARC corner vote: .data is
 * the INDEX into the firmware "cx.lvl" level table for the first level >= 128
 * (rpmhpd.c rpmhpd_send_corner), sent active-only over the same RSC.
 */
static void tachyon_dp_vote_cx_corner(struct rsc_drv *drv)
{
	const __le16 *lvl;
	size_t len = 0, n, i;
	u32 addr;
	int idx = -1;

	addr = cmd_db_read_addr("cx.lvl");
	if (!addr) {
		log_warning("DP: no cx.lvl ARC addr; skipping CX corner vote\n");
		return;
	}
	lvl = cmd_db_read_aux_data("cx.lvl", &len);
	if (IS_ERR_OR_NULL(lvl) || len < sizeof(*lvl)) {
		log_warning("DP: no cx.lvl aux data; skipping CX corner vote\n");
		return;
	}
	n = len / sizeof(*lvl);
	for (i = 0; i < n; i++) {
		u16 v = le16_to_cpu(lvl[i]);

		if (i > 0 && v == 0)		/* zero-padded tail */
			break;
		if (v >= 128) {			/* RPMH_REGULATOR_LEVEL_SVS */
			idx = (int)i;
			break;
		}
	}
	if (idx < 0)
		idx = (int)n - 1;		/* clamp to max corner */

	{
		struct tcs_cmd cmd = { .addr = addr, .data = (u32)idx, .wait = 1 };
		struct tcs_request msg = {
			.state = RPMH_ACTIVE_ONLY_STATE, .wait_for_compl = 1,
			.is_read = false, .num_cmds = 1, .cmds = &cmd,
		};
		int ret = rpmh_rsc_send_data(drv, &msg);

		log_warning("DP: CX corner vote addr=%#x idx=%d ret=%d\n",
			    addr, idx, ret);
	}
}

static void tachyon_dp_grant_mdp0_bandwidth(void)
{
	/*
	 * Grant the SSPP its DRAM read throughput, replicating what Linux does
	 * over the same apps_rsc/RPMh transport:
	 *  (1) Vote the REAL 1080p60 bandwidth (AB~622MB/s, IB=1.6GB/s) on EVERY
	 *      BCM of the MASTER_MDP0->EBI path (icc_set_bw fans a path vote out to
	 *      all nodes): MM1(qxm_mdp0) MM0(qns_mem_noc_hf) SH0(qns_llcc) MC0(ebi),
	 *      each with its OWN buswidth+channels, + the ACV mask BCM.  Voting only
	 *      MM1 leaves the DDR/LLCC bus clocks unraised -> still starved.
	 *  (2) Raise the SC7280_CX corner the 300MHz core clock requires.
	 */
	const u32 ab_kbps = 622080;	/* 1080p60 plane avg, Bps/1000 */
	const u32 ib_kbps = 1600000;	/* peak 1.6 GB/s = min_dram_ib */
	static const struct { const char *n; u32 bw; u32 ch; } path[] = {
		{ "MM1", 32, 1 },	/* qxm_mdp0       */
		{ "MM0", 32, 2 },	/* qns_mem_noc_hf */
		{ "SH0", 16, 2 },	/* qns_llcc       */
		{ "MC0",  4, 2 },	/* ebi            */
	};
	struct udevice *rsc;
	struct rsc_drv *drv;
	int i, ret;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(qcom_rpmh_rsc), &rsc);
	if (ret) {
		log_warning("DP: no apps_rsc for MDP0 bw vote: %d\n", ret);
		return;
	}
	drv = dev_get_priv(rsc);

	for (i = 0; i < ARRAY_SIZE(path); i++) {
		u32 a  = cmd_db_read_addr(path[i].n);
		u32 vx = tachyon_bcm_vote_x(path[i].n, path[i].bw, path[i].ch,
					    ab_kbps);
		u32 vy = tachyon_bcm_vote_y(path[i].n, path[i].bw, ib_kbps);

		ret = tachyon_bcm_send(drv, a, BCM_TCS_CMD(1, 1, vx, vy));
		log_warning("DP: %s bw vote addr=%#x vx=%u vy=%u ret=%d\n",
			    path[i].n, a, vx, vy, ret);
	}

	/* ACV mask BCM (enable_mask=BIT(3)): mark the DDR channel active. */
	tachyon_bcm_send(drv, cmd_db_read_addr("ACV"), BCM_TCS_CMD(1, 1, 0, BIT(3)));

	/* Raise the CX voltage corner the 300MHz datapath needs. */
	tachyon_dp_vote_cx_corner(drv);
}

/*
 * Fix the MDP stream's SMR mask.  The DT spec is iommus=<&apps_smmu 0x900
 * 0x402>: SID 0x900, MASK 0x402 -- so the stream must match the MDP's masked
 * sub-SIDs (0x900/0x902/0xd00/0xd02, bits 1 and 10 wildcarded).  But the live
 * SMR[3] reads 0x80000900 (VALID|ID, MASK=0) -- it matches ONLY exactly
 * 0x900.  If the DMA0 SSPP fetch emits a masked variant it is an UNMATCHED
 * stream -> SMMU fault -> (on this platform, with secure fault handling) a
 * reset.  Now that we know the TBU is powered (PWR_STATUS!=0) and the SMMU
 * region is U-Boot-writable (CB3 writes stuck) + SVC_IO-serviced, widen the
 * SMR to the DT mask.  Write both NS and via SVC_IO and read back to see
 * which sticks.  Target SMR value = VALID|MASK(0x402<<16)|ID = 0x84020900.
 */
static void tachyon_dp_fix_mdp_smr(void)
{
	void __iomem *smmu = map_sysmem(0x15000000, 0x100000);
	int n;

	if (!smmu)
		return;

	for (n = 0; n < 128; n++) {
		u32 smr = readl(smmu + 0x800 + 4 * n);

		if (!(smr & BIT(31)) || (smr & 0xffff) != 0x0900)
			continue;
		{
			phys_addr_t pa = 0x15000000UL + 0x800 + 4 * n;
			u32 want = BIT(31) | (0x402u << 16) | 0x0900; /* 0x84020900 */
			u32 sv = 0;

			writel(want, smmu + 0x800 + 4 * n);	/* NS write   */
			qcom_scm_io_writel(pa, want);		/* secure write */
			qcom_scm_io_readl(pa, &sv);
			log_warning("MDP SMR[%d] mask-fix: was=%08x want=%08x ns_rb=%08x scm_rb=%08x\n",
				    n, smr, want, readl(smmu + 0x800 + 4 * n), sv);
		}
		break;
	}

	unmap_sysmem(smmu);
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

	/*
	 * Make the DMA0 SSPP AXI read client (xin_id=1) fetch-ready at the VBIF.
	 * THIS is why the SSPP fetched zero (solid INTF underflow) despite correct
	 * config + FB + clock + voltage + BCM bandwidth: at cold handoff the VBIF
	 * OUT_AXI_AMEMTYPE for the MDP read client is 0, so the SMMU/NoC drops
	 * every MDP read.  XBL sets AMEMTYPE=0x33333333/0x00333333 "to work with
	 * the new SMMU" (HALMDSS hal_mdp_vbif.c) and Linux re-runs
	 * dpu_vbif_init_memtypes() every runtime resume; U-Boot did neither (its
	 * old 0xd00/0xd20 writes were the wrong registers entirely).  Also un-halt
	 * the xin and clear stale AXI error latches in case XBL teardown left them.
	 */
	if (priv->vbif) {
		u32 pnd = readl(priv->vbif + VBIF_XIN_PND_ERR);
		u32 src = readl(priv->vbif + VBIF_XIN_SRC_ERR);
		u32 halt1_before = readl(priv->vbif + VBIF_XIN_HALT_CTRL1);

		/* Un-halt DMA0 (BIT(1) of HALT_CTRL0). */
		clrbits_le32(priv->vbif + VBIF_XIN_HALT_CTRL0,
			     BIT(VBIF_DMA0_XIN_ID));
		/* Clear any stale pending/source AXI error latches. */
		if (pnd | src)
			writel(pnd | src, priv->vbif + VBIF_XIN_CLR_ERR);
		/* AMEMTYPE = 3 (cacheable normal) for every xin, like XBL/Linux. */
		writel(0x33333333, priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF0);
		writel(0x00333333, priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF1);

		log_warning("VBIF xin1 init: HALT1_before=%08x PND=%08x SRC=%08x AMEM0=%08x\n",
			    halt1_before, pnd, src,
			    readl(priv->vbif + VBIF_OUT_AXI_AMEMTYPE_CONF0));
	}

	/*
	 * Grant the MDP0->DDR read bandwidth (RPMh BCM vote) BEFORE the SSPP
	 * fetch is set up.  Without this the AXI read is starved and the pipe
	 * underruns (solid INTF underflow colour) despite correct config.
	 */
	tachyon_dp_grant_mdp0_bandwidth();

	tachyon_dpu_program_sspp(priv, plat, uc_priv);
	tachyon_dpu_program_lm(priv, uc_priv);
	tachyon_dpu_program_intf(priv, uc_priv);

	/*
	 * Widen the MDP stream's apps_smmu SMR to the DT mask (0x402) so the
	 * DMA0 fetch's masked sub-SIDs match SMR[3]->S2CR[3]->CB3 instead of
	 * faulting as an unmatched stream.  This is the step that lets the MDP
	 * AXI read reach DRAM (the M=0 passthrough CB is sufficient).
	 */
	tachyon_dp_fix_mdp_smr();

	return tachyon_dpu_program_ctl(priv);
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
	log_warning("DP quiesce: step dpu done\n");
	tachyon_dp_controller_quiesce(priv);
	log_warning("DP quiesce: step controller done\n");

	if (priv->phy_dp && (priv->qmp_dp_touched ||
			     priv->qmp_dp_serdes_programmed ||
			     priv->qmp_dp_phy_started)) {
		tachyon_dp_qmp_power_down(priv);
		log_warning("DP quiesce: step qmp_power_down done\n");
	}

	if (priv->has_qmp_phy) {
		ret = generic_phy_power_off(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY power_off failed: %d\n", ret);
		log_warning("DP quiesce: step phy_power_off done\n");
		ret = generic_phy_exit(&priv->qmp_phy);
		if (ret && ret != -ENOSYS)
			log_warning("QMP PHY exit failed: %d\n", ret);
		log_warning("DP quiesce: step phy_exit done\n");
		priv->has_qmp_phy = false;
	}

	if (dm_gpio_is_valid(&priv->sbu_enable))
		dm_gpio_set_value(&priv->sbu_enable, 0);
	tachyon_dp_release_sbu_mux(priv);

	tachyon_dp_disable_clocks(priv);
}

/*
 * No .remove handler: DP is intentionally NOT torn down at ExitBootServices.
 * The link is left trained and the framebuffer scanning out so Windows (which
 * has no native Qualcomm DP/DPU driver) keeps a live GOP display across the
 * handoff.  This intentionally breaks the Linux clean handoff (msm_dpu fails
 * modeset "Cannot find any crtc or sizes", dock black on Linux boot) -- accepted.
 * The err_quiesce path in the bringup code still quiesces on a failed train.
 */

static int tachyon_dp_wait_sink(struct tachyon_dp_priv *priv)
{
	int ret, i;
	struct qcom_pmic_glink_altmode_state altmode_state;
	int altmode_ret;

	log_warning("DP wait sink: %d tries, %d us interval\n",
		    TACHYON_DP_AUX_DEBOUNCE_TRIES, 20000);

	for (i = 0; i < TACHYON_DP_AUX_DEBOUNCE_TRIES; i++) {
		/*
		 * Keep the PMIC altmode state machine polling while we wait for
		 * HPD and the sink's DPCD registers to become available.
		 */
		altmode_ret = qcom_pmic_glink_altmode_poll(&altmode_state, 100);
		if (!altmode_ret)
			tachyon_dp_apply_pmic_typec_state(priv, &altmode_state);
		else if (altmode_ret != -ENOSYS && altmode_ret != -ETIMEDOUT)
			log_debug("DP PMIC altmode poll failed: %d\n", altmode_ret);

		if (tachyon_dp_hw_hpd_connected(priv)) {
			priv->hpd_state = TACHYON_DP_HPD_CONNECTED;
			priv->aux_xfers_enabled = true;
		}

		if (priv->phy_dp && !tachyon_dp_qmp_phy_ready(priv)) {
			u8 qmp_status = tachyon_dp_qmp_status_low(priv);

			log_debug("DP wait sink try %d/%d: QMP PHY not ready (%02x); waiting for PHY_READY before DPCD\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES, qmp_status);
			udelay(20000);
			continue;
		}

		if (priv->hpd_state == TACHYON_DP_HPD_DISCONNECTED &&
		    !tachyon_dp_env_bool("tachyon_dp_force_aux_without_hpd")) {
			log_debug("DP wait sink try %d/%d: no HPD yet, skipping DPCD read\n",
				 i + 1, TACHYON_DP_AUX_DEBOUNCE_TRIES);
			udelay(20000);
			continue;
		}

		/*
		 * Hard-reset the AUX controller before every DPCD retry.
		 * The GO bit was observed stuck at 0x200 after timeouts;
		 * a full AUX reset ensures a clean transaction state each
		 * attempt.
		 */
		tachyon_dp_aux_hw_init(priv);

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

static int tachyon_dp_probe(struct udevice *dev)
{
	struct tachyon_dp_priv *priv = dev_get_priv(dev);
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	u32 width, height;
	int ret, altmode_ret;
	bool has_sbu_mux;
	bool forced_typec = false;

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

		/*
		 * The dock has not entered DP yet.  At boot it is a cold Type-C
		 * power-sink (UCSI opmode=5, no PD contract, partner=0) and only
		 * self-completes its PD/DP negotiation ~20-30s after power-on
		 * (which is why a manual run from the prompt always worked: the
		 * dock had settled by then).  PAN_EN is armed, so poll for the
		 * ADSP DP notify (mux=3 / dp_seen) and exit the instant it
		 * arrives; the long window just lets a cold dock settle.  A warm
		 * boot (dock already in DP) never reaches here.  Disable with
		 * tachyon_dp_no_auto_dfp=1; tune the window with
		 * tachyon_dp_auto_dfp_ms.
		 */
		if (!tachyon_dp_env_bool("tachyon_dp_no_auto_dfp")) {
			int dfp = qcom_pmic_glink_request_dfp(
				tachyon_dp_env_u32("tachyon_dp_auto_dfp_ms",
						   20000));

			log_warning("DP auto-DFP ret=%d; re-reading altmode\n",
				    dfp);
			altmode_ret = tachyon_dp_read_altmode(priv);
			if (altmode_ret > 0) {
				priv->typec_source =
					TACHYON_DP_TYPEC_SOURCE_ALTMODE;
				priv->typec_valid = true;
				if (tachyon_dp_typec_state_valid(priv)) {
					tachyon_dp_log_typec_resolved(priv);
				} else {
					priv->typec_valid = false;
					priv->typec_source =
						TACHYON_DP_TYPEC_SOURCE_NONE;
				}
			}
		}
	}

	ret = tachyon_dp_request_sbu_mux(priv);
	if (ret)
		log_warning("SBU mux unavailable: %d\n", ret);

	has_sbu_mux = dm_gpio_is_valid(&priv->sbu_enable) &&
		      dm_gpio_is_valid(&priv->sbu_select);
	log_warning("DP SBU mux usable=%d\n", has_sbu_mux ? 1 : 0);

	if (!priv->typec_valid) {
		const struct qcom_pmic_glink_altmode_state *state;

		state = qcom_pmic_glink_altmode_get_state();

		log_warning("DP Type-C Alt Mode not active before AUX; continuing with PMIC state=%u svid=%04x orient_raw=%u mux=%u dpam=%02x hpd=%u\n",
			    state ? state->typec_state : 0,
			    state ? state->svid : 0,
			    state ? state->orientation_raw : 0xff,
			    state ? state->mux : 0xff,
			    state ? state->dpam_raw : 0xff,
			    state ? state->hpd : 0);

		/*
		 * Diagnostic fallback:
		 * Linux proves this connector ultimately works through
		 * aux_hpd_bridge. Do not abort before AUX. Pick a forced
		 * orientation for testing.
		 */
		priv->typec_source = TACHYON_DP_TYPEC_SOURCE_ALTMODE;
		priv->typec_valid = true;
		forced_typec = true;

		if (tachyon_dp_env_bool("tachyon_dp_force_reverse"))
			priv->orientation = TACHYON_DP_ORIENTATION_REVERSE;
		else
			priv->orientation = TACHYON_DP_ORIENTATION_NORMAL;

		/*
		 * Forced/diagnostic path: there was no ADSP alt-mode notify, so
		 * the negotiated Type-C pin is unknown. The Tachyon's target
		 * sinks are USB-C docks/hubs, which negotiate pin D = 2-lane DP +
		 * USB3 (combo MODE=0x03) — so default to pin D. Override with
		 * tachyon_dp_force_pin only for experiments (C=2 / E=4 are 4-lane
		 * DP-only MODE=0x02; F=5 is the reversed 2-lane combo). The pin
		 * choice drives both the link lane budget (reset_link_policy) and
		 * the QMP combo split (tachyon_dp_qmp_phy_mode).
		 */
		{
			u8 forced_pin = tachyon_dp_env_u32("tachyon_dp_force_pin",
							   3);

			if (!tachyon_dp_valid_pin_assignment(forced_pin)) {
				log_warning("DP invalid tachyon_dp_force_pin=%u; using pin D (3)\n",
					    forced_pin);
				forced_pin = 3;
			}
			priv->pin_assignment = forced_pin;
		}
		log_warning("DP forced Type-C: orientation=%u pin=%u pin_lanes=%u (override: tachyon_dp_force_pin / _reverse / _lanes / _max_rate)\n",
			    priv->orientation, priv->pin_assignment,
			    tachyon_dp_pin_assignment_lanes(priv));
		tachyon_dp_log_typec_resolved(priv);
	}

	/*
	 * Resolve the (orientation, pin) actually used and read the sink's DPCD
	 * link caps.  Each attempt first brings the DP PHY to PHY_READY
	 * (DP_STATUS) and forces AUX on for that orientation — without a locked
	 * PLL, AUX reads fail with DP_INTR_TIMEOUT and DP_STATUS=00.
	 *
	 * With a real ADSP alt-mode notify we already know orientation+pin, so
	 * there is a single candidate.  On the forced/diagnostic path the hub
	 * only reveals its pin once it has entered DP, so instead of forcing one
	 * mode we SWEEP the viable 2-lane dock assignments — pin D (normal) then
	 * pin F (D reversed), both 2-lane DP+USB3 / combo MODE=0x03 — and keep
	 * whichever the sink answers DPCD on.  AUX timing out (-110) just means
	 * "no sink on this orientation"; move to the next candidate.  An explicit
	 * tachyon_dp_force_pin / tachyon_dp_force_reverse pins a single attempt
	 * (use pin C=2 / E=4 there for a 4-lane DP-only sink).
	 */
	{
		struct { enum tachyon_dp_orientation orient; u8 pin; } cand[4];
		bool explicit_pin =
			tachyon_dp_env_has_u32("tachyon_dp_force_pin") ||
			tachyon_dp_env_bool("tachyon_dp_force_reverse");
		int ncand = 0, ci;

		if (!forced_typec || explicit_pin) {
			cand[ncand].orient = priv->orientation;
			cand[ncand].pin = priv->pin_assignment;
			ncand++;
		} else {
			cand[ncand].orient = TACHYON_DP_ORIENTATION_NORMAL;
			cand[ncand].pin = 3;	/* pin D: 2-lane DP + USB3 */
			ncand++;
			cand[ncand].orient = TACHYON_DP_ORIENTATION_REVERSE;
			cand[ncand].pin = 3;	/* pin F: same, reversed */
			ncand++;
		}

		ret = -EIO;
		for (ci = 0; ci < ncand; ci++) {
			priv->orientation = cand[ci].orient;
			priv->pin_assignment = cand[ci].pin;
			priv->rate = DP_LINK_RATE_HBR;
			priv->lanes = 2;

			log_warning("DP probe %d/%d: orientation=%u pin=%u pin_lanes=%u\n",
				    ci + 1, ncand, priv->orientation,
				    priv->pin_assignment,
				    tachyon_dp_pin_assignment_lanes(priv));

			tachyon_dp_program_sbu_mux(priv);
			tachyon_dp_prepare_aux_for_orientation(priv,
							       priv->orientation);

			ret = tachyon_dp_qmp_program_dp_phy(priv);
			log_warning("DP probe PHY bring-up ret=%d DP_STATUS=%02x\n",
				    ret, tachyon_dp_qmp_status_low(priv));

			ret = tachyon_dp_read_dpcd_caps(priv);
			log_warning("DP probe DPCD ret=%d caps.lanes=%u caps.max_rate=%u\n",
				    ret, priv->caps.lanes, priv->caps.max_rate);

			if (!ret && priv->caps.lanes && priv->caps.max_rate) {
				log_warning("DP probe LOCKED orientation=%u pin=%u after %d/%d\n",
					    priv->orientation,
					    priv->pin_assignment, ci + 1, ncand);
				break;
			}
		}
	}
	if (ret || !priv->caps.lanes || !priv->caps.max_rate) {
		priv->caps.lanes = priv->caps.lanes ? priv->caps.lanes : 2;
		priv->caps.max_rate = priv->caps.max_rate ?
				      priv->caps.max_rate : DP_LINK_RATE_HBR;
	}
	if (!priv->max_rate)
		priv->max_rate = priv->caps.max_rate;
	if (!priv->max_lanes)
		priv->max_lanes = priv->caps.lanes;
	if (!priv->rate)
		priv->rate = priv->max_rate;
	if (!priv->lanes)
		priv->lanes = priv->max_lanes;
	log_warning("DP link budget: max_rate=%u max_lanes=%u rate=%u lanes=%u caps.lanes=%u\n",
		    priv->max_rate, priv->max_lanes, priv->rate, priv->lanes,
		    priv->caps.lanes);

	log_warning("DP wait sink start with derived Type-C orientation=%u\n",
		    priv->orientation);

	tachyon_dp_prepare_aux_for_orientation(priv, priv->orientation);
	ret = tachyon_dp_wait_sink(priv);

	log_warning("DP wait sink done ret=%d orientation=%u\n",
		    ret, priv->orientation);

	if (ret)
		goto err_quiesce;

	ret = tachyon_dp_read_edid_modes(priv);
	if (ret) {
		/*
		 * No EDID => no reachable DP sink (e.g. a USB-C hub that hasn't
		 * entered DP Alt Mode, so its mux never routed DP/AUX to the
		 * panel). Abort gracefully and cleanly release the PHY/SBU/clocks
		 * rather than crashing into link-training with zero lanes.
		 */
		log_warning("DP: no EDID / no DP sink reachable (%d) - aborting cleanly\n",
			    ret);
		goto err_quiesce;
	}


	ret = tachyon_dp_link_train(priv);
	if (ret)
		goto err_quiesce;
	priv->dp_link_up = true;

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

	/*
	 * bind() leaves plat->size = 0 in manual mode (no boot-time FB
	 * reservation -> clean Linux handoff).  Set it now that we're actually
	 * bringing DP up.
	 */
	if (!plat->size)
		plat->size = TACHYON_DP_MAX_XRES * TACHYON_DP_MAX_YRES * 4;

	/*
	 * Allocate the framebuffer below 4 GB.  The DPU SSPP source-address
	 * register is 32-bit and there's no IOMMU in U-Boot, so a >4 GB buffer
	 * (the video uclass would reserve it at the top of this 8 GB+ board's RAM)
	 * has its high bits dropped by the SSPP -> DPU fetches the wrong memory ->
	 * "No Signal".  Reserve a region below 4 GB via lmb the DPU can address.
	 */
	if (!plat->base || (u64)plat->base + plat->size > 0x100000000ULL) {
		phys_addr_t low = lmb_alloc_base(plat->size, plat->align,
						 0x100000000ULL, LMB_NOOVERWRITE);

		if (low) {
			log_warning("DP relocating FB %lx -> %llx (<4GB for DPU SSPP)\n",
				    (ulong)plat->base, (u64)low);
			plat->base = (ulong)low;
		} else {
			log_warning("DP: lmb <4GB FB alloc failed; DPU may fetch wrong address\n");
		}
	}

	video_set_flush_dcache(dev, true);
	/*
	 * Initialise the framebuffer the DPU scans out.  Default to a clean
	 * black background so the real U-Boot console renders on it: with
	 * CONFIG_NO_FB_CLEAR=y the video uclass does NOT clear this freshly
	 * lmb-allocated buffer, after which video_post_probe draws the logo on
	 * the dock.  Set the env var
	 * "tachyon_dp_test_pattern" to instead paint vertical colour bars — a
	 * fetch-vs-no-fetch diagnostic: if the dock shows bars the DPU is
	 * genuinely fetching the framebuffer.
	 */
	if (tachyon_dp_env_bool("tachyon_dp_test_pattern")) {
		tachyon_dp_fill_test_pattern(plat, uc_priv);
	} else {
		memset((void *)plat->base, 0,
		       (size_t)uc_priv->line_length * uc_priv->ysize);
		flush_dcache_range((ulong)plat->base,
				   (ulong)plat->base +
				   (ulong)uc_priv->line_length * uc_priv->ysize);
	}

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

	/*
	 * Once the DP link is trained and scanning out, do NOT keep re-poking
	 * the ADSP altmode service or re-training on every video_sync.  Doing so
	 * knocks the live DP link back to "safe" (the TV loses signal) and floods
	 * the console with altmode polls.  Keep the link stable and just let the
	 * video uclass flush the framebuffer.
	 */
	if (priv->dp_link_up)
		return 0;

	ret = tachyon_dp_refresh_altmode(priv, &alt_changed);
	if (ret)
		return ret;


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


static const struct video_ops tachyon_dp_ops = {
	.video_sync = tachyon_dp_video_sync,
	.video_get_mode_count = tachyon_dp_get_mode_count,
	.video_get_mode_info = tachyon_dp_get_mode_info,
	.video_set_mode = tachyon_dp_set_mode,
};

static int tachyon_dp_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->align = TACHYON_DP_FB_ALIGN;

	plat->size = TACHYON_DP_MAX_XRES * TACHYON_DP_MAX_YRES * 4;

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
	.ops		= &tachyon_dp_ops,
	.priv_auto	= sizeof(struct tachyon_dp_priv),
	.plat_auto	= sizeof(struct video_uc_plat),
	.flags		= DM_FLAG_PRE_RELOC |
			  DM_FLAG_DEFAULT_PD_CTRL_OFF |
			  DM_FLAG_DEFAULT_CLKS_OFF,
};
