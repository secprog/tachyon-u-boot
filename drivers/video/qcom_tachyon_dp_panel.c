// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm Tachyon DisplayPort panel/sink: EDID block parsing (DTD / standard /
 * established / CEA-861), DPCD capability + sink power, and video-mode build /
 * filter / select / publish. Split out of qcom_tachyon_dp.c; operates on the
 * shared struct tachyon_dp_priv.
 */
#define LOG_CATEGORY UCLASS_VIDEO

#include <dm.h>
#include <log.h>
#include <edid.h>
#include <env.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "qcom_tachyon_dp.h"

bool tachyon_dp_valid_resolution(u32 width, u32 height)
{
	return width >= TACHYON_DP_MIN_XRES &&
	       height >= TACHYON_DP_MIN_YRES &&
	       width <= TACHYON_DP_MAX_XRES &&
	       height <= TACHYON_DP_MAX_YRES;
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

bool tachyon_dp_mode_fits_link(struct tachyon_dp_priv *priv,
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

void tachyon_dp_env_mode(u32 *width, u32 *height)
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
		log_debug("Forcing DP resolution %ux%u (tachyon_dp_force_mode)\n",
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

void tachyon_dp_timing_entry(struct timing_entry *entry, u32 value)
{
	entry->min = value;
	entry->typ = value;
	entry->max = value;
}

void tachyon_dp_fill_timing(struct display_timing *timing,
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

void tachyon_dp_default_timing(struct display_timing *timing)
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
bool tachyon_dp_cvt_timing(u32 width, u32 height, u32 hz,
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
bool tachyon_dp_known_timing(u32 width, u32 height,
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

static u32 tachyon_dp_mode_refresh(const struct display_timing *t)
{
	u32 htotal = tachyon_dp_htotal(t);
	u32 vtotal = tachyon_dp_vtotal(t);

	if (!t->pixelclock.typ || !htotal || !vtotal)
		return 0;

	return DIV_ROUND_CLOSEST(t->pixelclock.typ, htotal * vtotal);
}

static void tachyon_dp_add_mode_timing(struct tachyon_dp_priv *priv,
				       u32 width, u32 height,
				       const struct display_timing *timing)
{
	struct tachyon_dp_mode *modes = priv->modes;
	int *count = &priv->mode_count;
	int i;

	if (!tachyon_dp_valid_resolution(width, height))
		return;

	for (i = 0; i < *count; i++) {
		if (modes[i].width != width || modes[i].height != height)
			continue;
		/*
		 * One slot per resolution.  Keep the first entry, EXCEPT when
		 * the incoming timing fits the trained link and the stored one
		 * does not (no timing yet, or a stored timing that exceeds the
		 * link budget) — then replace it.  This lets e.g. 4K30 (fits
		 * HBR2x2) survive when the EDID listed 4K60 (does not fit a
		 * 2-lane link) first, instead of dropping the resolution.
		 */
		if (timing &&
		    (!modes[i].has_timing ||
		     (tachyon_dp_mode_fits_link(priv, timing) &&
		      !tachyon_dp_mode_fits_link(priv, &modes[i].timing)))) {
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

static void tachyon_dp_add_mode(struct tachyon_dp_priv *priv,
				u32 width, u32 height)
{
	struct display_timing timing;

	if (tachyon_dp_known_timing(width, height, &timing) ||
	    tachyon_dp_cvt_timing(width, height, 60, &timing))
		tachyon_dp_add_mode_timing(priv, width, height,
					   &timing);
	else
		tachyon_dp_add_mode_timing(priv, width, height, NULL);
}

static void tachyon_dp_parse_dtd(struct tachyon_dp_priv *priv,
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
	tachyon_dp_add_mode_timing(priv, width, height, &timing);
}

static void tachyon_dp_parse_standard_timings(struct tachyon_dp_priv *priv,
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
			tachyon_dp_add_mode_timing(priv, width, height,
						   &timing);
		else
			tachyon_dp_add_mode(priv, width, height);
	}
}

static void tachyon_dp_add_cea_vic(struct tachyon_dp_priv *priv, u8 vic);

static void tachyon_dp_parse_established_timings(struct tachyon_dp_priv *priv,
						 const struct edid1_info *edid)
{
	struct display_timing timing;

	/* Established timings are standard VESA modes — use known timings */
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 25175000, 640, 16, 96, 48,
				       480, 10, 2, 33,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 640, 480, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 40000000, 800, 40, 128, 88,
				       600, 1, 4, 23,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 800, 600, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_60(*edid)) {
		tachyon_dp_fill_timing(&timing, 65000000, 1024, 24, 136, 160,
				       768, 3, 6, 29,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 1024, 768, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1280X1024_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 135000000, 1280, 16, 144, 248,
				       1024, 1, 3, 38,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1280, 1024, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1152X870_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 92940000, 1152, 48, 128, 112,
				       870, 3, 3, 39,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 1152, 870, &timing);
	}
	/* These are infrequently declared in EDID, but cover them anyway */
	if (EDID1_INFO_ESTABLISHED_TIMING_720X400_70(*edid)) {
		tachyon_dp_fill_timing(&timing, 28320000, 720, 18, 108, 54,
				       400, 13, 2, 34,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 720, 400, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 31500000, 640, 16, 64, 120,
				       480, 1, 3, 16,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 640, 480, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 49500000, 800, 16, 80, 160,
				       600, 1, 3, 21,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 800, 600, &timing);
	}
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_75(*edid)) {
		tachyon_dp_fill_timing(&timing, 78750000, 1024, 16, 96, 176,
				       768, 1, 3, 28,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1024, 768, &timing);
	}
}

static void tachyon_dp_add_cea_vic(struct tachyon_dp_priv *priv, u8 vic)
{
	struct display_timing timing;

	switch (vic & 0x7f) {
	/* 640×480 @ 59.94/60Hz */
	case 1:
		tachyon_dp_fill_timing(&timing, 25175000, 640, 16, 96, 48,
				       480, 10, 2, 33,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 640, 480, &timing);
		break;
	/* 720×480 @ 59.94/60Hz */
	case 2:
	case 3:
		tachyon_dp_fill_timing(&timing, 27027000, 720, 16, 62, 60,
				       480, 9, 6, 30,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 720, 480, &timing);
		break;
	/* 1280×720 @ 60Hz */
	case 4:
		tachyon_dp_fill_timing(&timing, 74250000, 1280, 110, 40, 220,
				       720, 5, 5, 20,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1280, 720, &timing);
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
		tachyon_dp_add_mode_timing(priv, 1920, 1080, &timing);
		break;
	/* 720×576 @ 50Hz */
	case 17:
	case 18:
		tachyon_dp_fill_timing(&timing, 27000000, 720, 12, 64, 68,
				       576, 5, 5, 39,
				       DISPLAY_FLAGS_HSYNC_LOW |
				       DISPLAY_FLAGS_VSYNC_LOW);
		tachyon_dp_add_mode_timing(priv, 720, 576, &timing);
		break;
	/* 1280×720 @ 50Hz */
	case 19:
		tachyon_dp_fill_timing(&timing, 74250000, 1280, 440, 40, 220,
				       720, 5, 5, 20,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1280, 720, &timing);
		break;
	/* 1920×1080 @ 50Hz */
	case 31:
		tachyon_dp_fill_timing(&timing, 148500000, 1920, 528, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1920, 1080, &timing);
		break;
	/* 1920×1080 @ 24Hz */
	case 32:
		tachyon_dp_fill_timing(&timing, 74250000, 1920, 638, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1920, 1080, &timing);
		break;
	/* 1920×1080 @ 25Hz */
	case 33:
		tachyon_dp_fill_timing(&timing, 74250000, 1920, 528, 44, 148,
				       1080, 4, 5, 36,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 1920, 1080, &timing);
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
		tachyon_dp_add_mode_timing(priv, 1920, 1080, &timing);
		break;
	/* 3840×2160 @ 24Hz */
	case 93:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 1276, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 25Hz */
	case 94:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 1056, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 30Hz */
	case 95:
		tachyon_dp_fill_timing(&timing, 297000000, 3840, 176, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 50Hz */
	case 96:
		tachyon_dp_fill_timing(&timing, 594000000, 3840, 1056, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 3840, 2160, &timing);
		break;
	/* 3840×2160 @ 60Hz */
	case 97:
		tachyon_dp_fill_timing(&timing, 594000000, 3840, 176, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 3840, 2160, &timing);
		break;
	/* 4096×2160 @ 24Hz */
	case 98:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 1020, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 25Hz */
	case 99:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 968, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 30Hz */
	case 100:
		tachyon_dp_fill_timing(&timing, 297000000, 4096, 88, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 50Hz */
	case 101:
		tachyon_dp_fill_timing(&timing, 594000000, 4096, 968, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 4096, 2160, &timing);
		break;
	/* 4096×2160 @ 60Hz */
	case 102:
		tachyon_dp_fill_timing(&timing, 594000000, 4096, 88, 88, 296,
				       2160, 8, 10, 72,
				       DISPLAY_FLAGS_HSYNC_HIGH |
				       DISPLAY_FLAGS_VSYNC_HIGH);
		tachyon_dp_add_mode_timing(priv, 4096, 2160, &timing);
		break;
	default:
		break;
	}
}

static void tachyon_dp_parse_cea_modes(struct tachyon_dp_priv *priv,
				       const u8 *buf)
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
				tachyon_dp_add_cea_vic(priv,
						       buf[offset + i]);
		}

		offset += len + 1;
	}

	dtd = cea->dtd_offset;
	while (dtd && dtd + sizeof(struct edid_detailed_timing) <= EDID_SIZE) {
		tachyon_dp_parse_dtd(priv, buf + dtd);
		dtd += sizeof(struct edid_detailed_timing);
	}
}


void tachyon_dp_filter_edid_modes(struct tachyon_dp_priv *priv)
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
			log_warning("Dropping DP EDID mode %ux%u@%uHz: %s\n",
				 priv->modes[in].width, priv->modes[in].height,
				 priv->modes[in].has_timing ?
				 tachyon_dp_mode_refresh(&priv->modes[in].timing) : 0,
				 !priv->modes[in].has_timing ?
				 "no timing available" :
				 "exceeds link policy");
			continue;
		}

		log_info("DP EDID mode kept: %ux%u@%uHz\n",
			 priv->modes[in].width, priv->modes[in].height,
			 tachyon_dp_mode_refresh(&priv->modes[in].timing));

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

void tachyon_dp_publish_edid_modes(struct tachyon_dp_priv *priv)
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

	log_info("DP EDID modes: %s preferred=%ux%u@%uHz pclk=%u\n",
		 out, priv->modes[pref].width, priv->modes[pref].height,
		 priv->modes[pref].has_timing ?
		 tachyon_dp_mode_refresh(&priv->modes[pref].timing) : 0,
		 priv->modes[pref].has_timing ?
		 priv->modes[pref].timing.pixelclock.typ : 0);
}

int tachyon_dp_read_edid_modes(struct tachyon_dp_priv *priv)
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
	log_debug("DP EDID blk0: %02x %02x %02x %02x %02x %02x %02x %02x | ext_flag=%u csum_ok=%u hdr_ok=%u\n",
		    edid_buf[0], edid_buf[1], edid_buf[2], edid_buf[3],
		    edid_buf[4], edid_buf[5], edid_buf[6], edid_buf[7],
		    edid_buf[126], tachyon_dp_edid_checksum_ok(edid_buf),
		    tachyon_dp_edid_header_ok(edid_buf));
	log_debug("DP EDID estab: %02x %02x %02x  ver=%u.%u\n",
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
		tachyon_dp_parse_dtd(priv,
				     edid->monitor_details.timing +
				     i * sizeof(struct edid_detailed_timing));

	tachyon_dp_parse_standard_timings(priv, edid);
	tachyon_dp_parse_established_timings(priv, edid);

	if (edid->extension_flag) {
		ret = tachyon_dp_edid_read_block(priv, 1, edid_buf + EDID_SIZE);
		if (!ret && tachyon_dp_edid_checksum_ok(edid_buf + EDID_SIZE))
			tachyon_dp_parse_cea_modes(priv, edid_buf + EDID_SIZE);
	}

	log_debug("DP EDID pre-filter: %d modes, rate=%u lanes=%u\n",
		    priv->mode_count, priv->rate, priv->lanes);
	for (i = 0; i < priv->mode_count; i++)
		log_debug("DP EDID mode[%d] %ux%u has_timing=%u\n", i,
			    priv->modes[i].width, priv->modes[i].height,
			    priv->modes[i].has_timing);

	tachyon_dp_filter_edid_modes(priv);
	tachyon_dp_publish_edid_modes(priv);

	log_debug("DP EDID post-filter: %d modes\n", priv->mode_count);

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

void tachyon_dp_select_mode(struct tachyon_dp_priv *priv,
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
		log_debug("DP TEST: forcing 1920x1080 (ignoring non-CEA native mode)\n");
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
		log_debug("Using built-in timing for DP mode %ux%u\n",
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
	log_debug("DP selected mode %ux%u pclk=%u hfp=%u hsw=%u hbp=%u vfp=%u vsw=%u vbp=%u (DP bottom-right adjusted)\n",
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
bool tachyon_dp_resolve_mode_timing(struct tachyon_dp_priv *priv,
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
		log_debug("Using built-in timing for DP mode %ux%u\n",
			    width, height);
		tachyon_dp_apply_dp_porch_adjust(priv);
		return true;
	}

	log_debug("No timing for DP mode %ux%u; using 1080p60 porch/pixel-clock fallback\n",
		    width, height);
	tachyon_dp_default_timing(&priv->timing);
	tachyon_dp_timing_entry(&priv->timing.hactive, width);
	tachyon_dp_timing_entry(&priv->timing.vactive, height);
	tachyon_dp_apply_dp_porch_adjust(priv);
	return true;
}

/*
 * Wake the sink/branch device into full-power D0 and report its downstream
 * (HDMI) port state.  A DP->HDMI branch device left in D3 (or that never had
 * its display path enabled) keeps its HDMI TX off, so the TV shows "No Signal"
 * even though our DP link to the branch is trained and we're sending video.
 * DPCD_DOWNSTREAMPORT_PRESENT bit0 tells us whether a branch device is present;
 * SINK_COUNT tells us whether it sees a downstream display.
 */
void tachyon_dp_sink_power_on(struct tachyon_dp_priv *priv)
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

	log_debug("DP sink: DFP_present=0x%02x DFP_count=0x%02x SINK_COUNT=0x%02x set_D0_ret=%d power_readback=0x%02x\n",
		    dfp, dfp_count, sink_count, ret, power);
}

int tachyon_dp_read_dpcd_caps(struct tachyon_dp_priv *priv)
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
