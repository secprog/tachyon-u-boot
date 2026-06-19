// SPDX-License-Identifier: GPL-2.0+
/*
 * UEFI Graphics Output Protocol (GOP) test application
 *
 * Copyright 2026 Particle / Tachyon
 *
 * This standalone EFI application exercises the EFI_GRAPHICS_OUTPUT_PROTOCOL
 * exactly the way an OS loader (the Windows / Linux EFI stub) sees it. It is
 * meant to validate that U-Boot exposes a correct, usable GOP on the Tachyon
 * DP dock.
 *
 * It does the following:
 *
 *   1. Locates the Graphics Output Protocol.
 *   2. Prints the current mode (resolution, pixel format, framebuffer base
 *      and size, pixels-per-scanline).
 *   3. Enumerates every advertised mode via QueryMode().
 *   4. Draws a sequence of Blt() test patterns:
 *        - clear screen
 *        - SMPTE-style vertical colour bars (VIDEO_FILL)
 *        - a white frame border (VIDEO_FILL)
 *        - a horizontal gradient pushed from a CPU buffer (BUFFER_TO_VIDEO)
 *        - a copy of that gradient (VIDEO_TO_VIDEO)
 *   5. Reads pixels back from the framebuffer (VIDEO_TO_BLT_BUFFER) and
 *      verifies they match what was written -> PASS/FAIL.
 *   6. Animates a rotating, gradient-shaded solid 3D cube (Gouraud-filled
 *      faces, depth-sorted, double-buffered with one BUFFER_TO_VIDEO blt
 *      per frame, all integer fixed-point math) until a key is pressed.
 *   7. Returns to the U-Boot console.
 *
 * Build: produces lib/efi_loader/goptest.efi (see lib/efi_loader/Makefile).
 * Run on the device, e.g.:
 *
 *     tachyon dp start            # bring the DP dock up first
 *     load mmc 1 $kernel_addr_r goptest.efi
 *     bootefi $kernel_addr_r
 */

#include <efi_api.h>

static const efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

static struct efi_system_table *systable;
static struct efi_boot_services *boottime;
static struct efi_simple_text_output_protocol *con_out;

/* ---- tiny freestanding console helpers (no libc string/printf) ---------- */

static void print(const u16 *s)
{
	con_out->output_string(con_out, (u16 *)s);
}

/* Print an unsigned value as decimal. */
static void print_dec(u64 value)
{
	u16 buf[21];
	int i = 20;

	buf[i] = u'\0';
	if (!value) {
		print(u"0");
		return;
	}
	while (value) {
		buf[--i] = u'0' + (value % 10);
		value /= 10;
	}
	print(&buf[i]);
}

/* Print "0x" followed by @digits hex digits of @value. */
static void print_hex(u64 value, int digits)
{
	u16 buf[19];
	int i;

	buf[0] = u'0';
	buf[1] = u'x';
	for (i = 0; i < digits; i++) {
		u32 nibble = (value >> ((digits - 1 - i) * 4)) & 0xf;

		buf[2 + i] = nibble < 10 ? u'0' + nibble : u'a' + nibble - 10;
	}
	buf[2 + digits] = u'\0';
	print(buf);
}

/* ---- GOP drawing helpers ------------------------------------------------ */

static struct efi_gop *gop;
static u32 scr_w, scr_h;

/* Solid-fill a rectangle with one colour. */
static efi_status_t fill(u8 r, u8 g, u8 b, u32 x, u32 y, u32 w, u32 h)
{
	struct efi_gop_pixel px;

	px.red = r;
	px.green = g;
	px.blue = b;
	px.reserved = 0;

	return gop->blt(gop, &px, EFI_BLT_VIDEO_FILL,
			0, 0, x, y, w, h, 0);
}

static const u16 *fmt_name(u32 fmt)
{
	switch (fmt) {
	case EFI_GOT_RGBA8:
		return u"RGBA8 (32bpp)";
	case EFI_GOT_BGRA8:
		return u"BGRA8 (32bpp)";
	case EFI_GOT_BITMASK:
		return u"BitMask";
	default:
		return u"BltOnly/Unknown";
	}
}

/**
 * print_mode_info() - dump the current GOP mode to the console
 */
static void print_mode_info(void)
{
	struct efi_gop_mode *mode = gop->mode;
	struct efi_gop_mode_info *info = mode->info;

	print(u"\r\n=== Graphics Output Protocol ===\r\n");

	print(u"Current mode   : ");
	print_dec(mode->mode);
	print(u" of ");
	print_dec(mode->max_mode);
	print(u"\r\n");

	print(u"Resolution     : ");
	print_dec(info->width);
	print(u" x ");
	print_dec(info->height);
	print(u"\r\n");

	print(u"Pixel format   : ");
	print(fmt_name(info->pixel_format));
	print(u"\r\n");

	if (info->pixel_format == EFI_GOT_BITMASK) {
		print(u"Pixel bitmask  : R=");
		print_hex(info->pixel_bitmask[0], 8);
		print(u" G=");
		print_hex(info->pixel_bitmask[1], 8);
		print(u" B=");
		print_hex(info->pixel_bitmask[2], 8);
		print(u"\r\n");
	}

	print(u"Pixels/scanline: ");
	print_dec(info->pixels_per_scanline);
	print(u"\r\n");

	print(u"Framebuffer    : ");
	print_hex(mode->fb_base, 16);
	print(u" .. ");
	print_hex(mode->fb_base + mode->fb_size, 16);
	print(u"\r\n");

	print(u"Framebuffer sz : ");
	print_dec(mode->fb_size);
	print(u" bytes (");
	print_dec(mode->fb_size >> 10);
	print(u" KiB)\r\n");

	print(u"Info size      : ");
	print_dec(mode->info_size);
	print(u" bytes\r\n");
}

/**
 * enumerate_modes() - list every mode advertised by QueryMode()
 */
static void enumerate_modes(void)
{
	u32 i;

	print(u"\r\nAdvertised modes:\r\n");
	for (i = 0; i < gop->mode->max_mode; i++) {
		struct efi_gop_mode_info *mi;
		efi_uintn_t size;
		efi_status_t ret;

		ret = gop->query_mode(gop, i, &size, &mi);
		if (ret != EFI_SUCCESS) {
			print(u"  [");
			print_dec(i);
			print(u"] QueryMode failed\r\n");
			continue;
		}
		print(u"  [");
		print_dec(i);
		print(u"] ");
		print_dec(mi->width);
		print(u" x ");
		print_dec(mi->height);
		print(u"  ");
		print(fmt_name(mi->pixel_format));
		if (i == gop->mode->mode)
			print(u"  <- active");
		print(u"\r\n");
		boottime->free_pool(mi);
	}
}

/**
 * draw_patterns() - paint colour bars, a border and a gradient
 */
static void draw_patterns(void)
{
	/*
	 * SMPTE-ish bar colours: white, yellow, cyan, green,
	 * magenta, red, blue, black.
	 */
	static const u8 bars[8][3] = {
		{0xff, 0xff, 0xff}, {0xff, 0xff, 0x00},
		{0x00, 0xff, 0xff}, {0x00, 0xff, 0x00},
		{0xff, 0x00, 0xff}, {0xff, 0x00, 0x00},
		{0x00, 0x00, 0xff}, {0x00, 0x00, 0x00},
	};
	u32 barw = scr_w / 8;
	u32 i, x;

	print(u"\r\nDrawing test patterns...\r\n");

	/* Clear to dark grey */
	fill(0x10, 0x10, 0x10, 0, 0, scr_w, scr_h);

	/* Vertical colour bars across the top 2/3 of the screen */
	for (i = 0; i < 8; i++) {
		x = i * barw;
		/* last bar absorbs the rounding remainder */
		u32 w = (i == 7) ? (scr_w - x) : barw;

		fill(bars[i][0], bars[i][1], bars[i][2],
		     x, 0, w, (scr_h * 2) / 3);
	}

	/* White 4px frame border around the whole screen */
	fill(0xff, 0xff, 0xff, 0, 0, scr_w, 4);
	fill(0xff, 0xff, 0xff, 0, scr_h - 4, scr_w, 4);
	fill(0xff, 0xff, 0xff, 0, 0, 4, scr_h);
	fill(0xff, 0xff, 0xff, scr_w - 4, 0, 4, scr_h);
}

/**
 * gradient_test() - BUFFER_TO_VIDEO + VIDEO_TO_VIDEO
 *
 * Builds a horizontal red->blue gradient in a CPU buffer, pushes it to the
 * lower-left of the screen, then copies it to the lower-right.
 *
 * Return: EFI_SUCCESS or an error from allocate_pool / blt.
 */
static efi_status_t gradient_test(void)
{
	u32 gw = scr_w / 3;
	u32 gh = scr_h / 5;
	u32 gx = scr_w / 12;
	u32 gy = (scr_h * 7) / 10;
	struct efi_gop_pixel *buf;
	efi_status_t ret;
	u32 px, py;

	if (!gw || !gh)
		return EFI_SUCCESS;

	ret = boottime->allocate_pool(EFI_BOOT_SERVICES_DATA,
				      (efi_uintn_t)gw * gh * sizeof(*buf),
				      (void **)&buf);
	if (ret != EFI_SUCCESS) {
		print(u"  gradient: allocate_pool failed\r\n");
		return ret;
	}

	for (py = 0; py < gh; py++) {
		for (px = 0; px < gw; px++) {
			struct efi_gop_pixel *p = &buf[py * gw + px];

			p->red = 0xff - (px * 0xff / gw);
			p->green = py * 0xff / gh;
			p->blue = px * 0xff / gw;
			p->reserved = 0;
		}
	}

	/* Push CPU buffer to the framebuffer (delta = 0 -> tightly packed) */
	ret = gop->blt(gop, buf, EFI_BLT_BUFFER_TO_VIDEO,
		       0, 0, gx, gy, gw, gh, 0);
	if (ret == EFI_SUCCESS) {
		/* Copy the on-screen gradient to the right-hand side */
		u32 dx = scr_w - gw - (scr_w / 12);

		gop->blt(gop, NULL, EFI_BLT_VIDEO_TO_VIDEO,
			 gx, gy, dx, gy, gw, gh, 0);
	} else {
		print(u"  gradient: BUFFER_TO_VIDEO failed\r\n");
	}

	boottime->free_pool(buf);
	return ret;
}

/**
 * readback_test() - VIDEO_TO_BLT_BUFFER round-trip verification
 *
 * Fills a small patch with a known colour, reads it back from the
 * framebuffer and checks the pixels match (within a tolerance to cover
 * lossy 16bpp / 10bpc modes).
 *
 * Return: true on PASS.
 */
static bool readback_test(void)
{
	const u8 tr = 0x40, tg = 0x80, tb = 0xc0;
	const u32 n = 32;
	u32 rx = scr_w / 2 - n / 2;
	u32 ry = scr_h / 2 - n / 2;
	struct efi_gop_pixel *buf;
	efi_status_t ret;
	bool ok = true;
	u32 i;

	print(u"\r\nReadback verification (VIDEO_TO_BLT_BUFFER): ");

	ret = boottime->allocate_pool(EFI_BOOT_SERVICES_DATA,
				      n * n * sizeof(*buf), (void **)&buf);
	if (ret != EFI_SUCCESS) {
		print(u"allocate_pool failed\r\n");
		return false;
	}

	/* Paint a known patch, then read it back */
	fill(tr, tg, tb, rx, ry, n, n);
	ret = gop->blt(gop, buf, EFI_BLT_VIDEO_TO_BLT_BUFFER,
		       rx, ry, 0, 0, n, n, n * sizeof(*buf));
	if (ret != EFI_SUCCESS) {
		print(u"blt failed (");
		print_hex(ret & ~EFI_ERROR_MASK, 2);
		print(u")\r\n");
		boottime->free_pool(buf);
		return false;
	}

	/* Tolerance 8 covers 16bpp (3-bit) and 10bpc (2-bit) rounding */
	for (i = 0; i < n * n; i++) {
		int dr = (int)buf[i].red - tr;
		int dg = (int)buf[i].green - tg;
		int db = (int)buf[i].blue - tb;

		if (dr < 0)
			dr = -dr;
		if (dg < 0)
			dg = -dg;
		if (db < 0)
			db = -db;
		if (dr > 8 || dg > 8 || db > 8) {
			ok = false;
			break;
		}
	}

	if (ok) {
		print(u"PASS\r\n");
	} else {
		print(u"FAIL (wrote ");
		print_hex(tr, 2);
		print(u"/");
		print_hex(tg, 2);
		print(u"/");
		print_hex(tb, 2);
		print(u" read ");
		print_hex(buf[i].red, 2);
		print(u"/");
		print_hex(buf[i].green, 2);
		print(u"/");
		print_hex(buf[i].blue, 2);
		print(u")\r\n");
	}

	boottime->free_pool(buf);
	return ok;
}

/* ---- rotating 3D cube ---------------------------------------------------- */

/*
 * Everything below is integer fixed-point: a freestanding EFI app has no
 * libm, so there is no sin()/cos() and no floating point. Cube coordinates
 * use FP (1.0 == FP). Sines/cosines use TRIG (1.0 == TRIG, i.e. Q10).
 */
#define FP	256	/* fixed-point "one" for model coordinates */
#define TRIG	1024	/* fixed-point "one" for sine/cosine (Q10) */

/**
 * isin() - integer sine, Bhaskara I approximation
 *
 * @deg:	angle in degrees
 * Return:	sin(deg) scaled by TRIG (range -TRIG..TRIG)
 */
static int isin(int deg)
{
	int neg = 0, t, val;

	deg %= 360;
	if (deg < 0)
		deg += 360;
	if (deg >= 180) {
		deg -= 180;
		neg = 1;
	}
	/* sin(d) ~= 4*d*(180-d) / (40500 - d*(180-d)) */
	t = deg * (180 - deg);
	val = (4 * t * TRIG) / (40500 - t);
	return neg ? -val : val;
}

static int icos(int deg)
{
	return isin(deg + 90);
}

/* The 8 cube corners (+/-1 on each axis) and the 6 quad faces. */
static const signed char cube_v[8][3] = {
	{-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
	{-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
};

static const unsigned char cube_f[6][4] = {
	{0, 1, 2, 3},	/* z- */
	{4, 5, 6, 7},	/* z+ */
	{0, 1, 5, 4},	/* y- */
	{3, 2, 6, 7},	/* y+ */
	{0, 3, 7, 4},	/* x- */
	{1, 2, 6, 5},	/* x+ */
};

/**
 * rotate() - rotate a point about the X, then Y, then Z axes
 *
 * @x, @y, @z:	point coordinates (updated in place)
 * @ax, @ay, @az: rotation angles in degrees
 */
static void rotate(int *x, int *y, int *z, int ax, int ay, int az)
{
	int sx = isin(ax), cx = icos(ax);
	int sy = isin(ay), cy = icos(ay);
	int sz = isin(az), cz = icos(az);
	int X = *x, Y = *y, Z = *z, t;

	/* about X: (y, z) */
	t = (Y * cx - Z * sx) / TRIG;
	Z = (Y * sx + Z * cx) / TRIG;
	Y = t;
	/* about Y: (x, z) */
	t = (X * cy + Z * sy) / TRIG;
	Z = (Z * cy - X * sy) / TRIG;
	X = t;
	/* about Z: (x, y) */
	t = (X * cz - Y * sz) / TRIG;
	Y = (X * sz + Y * cz) / TRIG;
	X = t;

	*x = X;
	*y = Y;
	*z = Z;
}

/**
 * fill_tri() - rasterize a Gouraud-shaded (colour-interpolated) triangle
 *
 * Colours are interpolated across the triangle using integer barycentric
 * weights, giving a smooth gradient. Clipped to the @s x @s back-buffer.
 *
 * @bb:		back-buffer, @s x @s pixels
 * @s:		side length of the back-buffer
 * @x0..@y2:	the three vertices (screen coordinates)
 * @c0..@c2:	the colour at each vertex
 */
static void fill_tri(struct efi_gop_pixel *bb, int s,
		     int x0, int y0, int x1, int y1, int x2, int y2,
		     struct efi_gop_pixel c0, struct efi_gop_pixel c1,
		     struct efi_gop_pixel c2)
{
	int minx, maxx, miny, maxy, px, py, area;

	minx = x0; if (x1 < minx) minx = x1; if (x2 < minx) minx = x2;
	maxx = x0; if (x1 > maxx) maxx = x1; if (x2 > maxx) maxx = x2;
	miny = y0; if (y1 < miny) miny = y1; if (y2 < miny) miny = y2;
	maxy = y0; if (y1 > maxy) maxy = y1; if (y2 > maxy) maxy = y2;
	if (minx < 0)
		minx = 0;
	if (miny < 0)
		miny = 0;
	if (maxx > s - 1)
		maxx = s - 1;
	if (maxy > s - 1)
		maxy = s - 1;

	/* twice the signed area; also the sum of the barycentric weights */
	area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
	if (!area)
		return;

	for (py = miny; py <= maxy; py++) {
		for (px = minx; px <= maxx; px++) {
			/* w0/w1/w2 are the (unnormalized) weights of c0/c1/c2 */
			int w0 = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1);
			int w1 = (x0 - x2) * (py - y2) - (y0 - y2) * (px - x2);
			int w2 = (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);

			/* inside if all weights share the sign of the area */
			if (!((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
			      (w0 <= 0 && w1 <= 0 && w2 <= 0)))
				continue;

			bb[py * s + px].red =
				(w0 * c0.red + w1 * c1.red + w2 * c2.red) / area;
			bb[py * s + px].green =
				(w0 * c0.green + w1 * c1.green +
				 w2 * c2.green) / area;
			bb[py * s + px].blue =
				(w0 * c0.blue + w1 * c1.blue +
				 w2 * c2.blue) / area;
			bb[py * s + px].reserved = 0;
		}
	}
}

/**
 * cube_demo() - animate a rotating, gradient-shaded solid cube
 *
 * Each cube corner gets a colour (the classic RGB colour-cube: corners are
 * black/red/green/blue/yellow/cyan/magenta/white) and the 6 faces are filled
 * with those colours interpolated across them -> smooth per-face gradients.
 * Faces are drawn far-to-near (painter's algorithm) so they occlude
 * correctly. Rendered into a CPU back-buffer and pushed with a single
 * BUFFER_TO_VIDEO blt per frame.
 */
static void cube_demo(void)
{
	/* colour at each of the 8 corners (RGB colour cube) */
	struct efi_gop_pixel vcol[8];
	struct efi_simple_text_input_protocol *con_in = systable->con_in;
	struct efi_gop_pixel *bb;
	int s = scr_w < scr_h ? scr_w : scr_h;
	int ox, oy, cx, cy, f, i;
	int ax = 0, ay = 0, az = 0;
	efi_status_t ret;
	long frame;

	if (s > 360)
		s = 360;	/* bound per-frame fill cost so it stays smooth */
	if (s < 64) {
		print(u"\r\nScreen too small for cube demo.\r\n");
		return;
	}
	ox = (scr_w - s) / 2;	/* centre the render window on screen */
	oy = (scr_h - s) / 2;
	cx = s / 2;
	cy = s / 2;
	f = (s * 5) / 8;	/* focal length (keeps the cube inside the box) */

	ret = boottime->allocate_pool(EFI_BOOT_SERVICES_DATA,
				      (efi_uintn_t)s * s * sizeof(*bb),
				      (void **)&bb);
	if (ret != EFI_SUCCESS) {
		print(u"\r\ncube: allocate_pool failed\r\n");
		return;
	}

	/* Map each corner (-1/+1 per axis) to a colour channel (0/255). */
	for (i = 0; i < 8; i++) {
		vcol[i].red = cube_v[i][0] > 0 ? 255 : 0;
		vcol[i].green = cube_v[i][1] > 0 ? 255 : 0;
		vcol[i].blue = cube_v[i][2] > 0 ? 255 : 0;
		vcol[i].reserved = 0;
	}

	/* Black out the whole screen once; only the cube box updates after. */
	fill(0, 0, 0, 0, 0, scr_w, scr_h);
	print(u"cube: buffer ok, box=");
	print_dec(s);
	print(u" - rendering (press a key to stop)\r\n");

	/*
	 * Bounded frame count: even if console input never reaches us (so the
	 * key-to-exit below can't fire), the demo still returns on its own
	 * instead of hanging the board.
	 */
	for (frame = 0; frame < 900; frame++) {
		int sxv[8], syv[8], zrv[8];
		int fz[6], order[6];
		struct efi_input_key key;
		int a, b, k;

		/* clear the back-buffer to black */
		memset(bb, 0, (efi_uintn_t)s * s * sizeof(*bb));

		/* transform + project the 8 corners */
		for (i = 0; i < 8; i++) {
			int X = cube_v[i][0] * FP;
			int Y = cube_v[i][1] * FP;
			int Z = cube_v[i][2] * FP;
			int denom;

			rotate(&X, &Y, &Z, ax, ay, az);
			zrv[i] = Z;
			denom = 4 * FP + Z;	/* camera 4 units back */
			if (denom < 1)
				denom = 1;
			sxv[i] = cx + (f * X) / denom;
			syv[i] = cy - (f * Y) / denom;	/* flip: screen y is down */
		}

		/* depth key per face (sum of corner z); larger == farther */
		for (i = 0; i < 6; i++) {
			fz[i] = zrv[cube_f[i][0]] + zrv[cube_f[i][1]] +
				zrv[cube_f[i][2]] + zrv[cube_f[i][3]];
			order[i] = i;
		}
		/* insertion-sort faces far -> near (painter's algorithm) */
		for (a = 1; a < 6; a++) {
			int key_i = order[a], key_z = fz[key_i];

			for (b = a - 1; b >= 0 && fz[order[b]] < key_z; b--)
				order[b + 1] = order[b];
			order[b + 1] = key_i;
		}

		/* fill each face (two gradient triangles) far -> near */
		for (k = 0; k < 6; k++) {
			const unsigned char *fc = cube_f[order[k]];

			fill_tri(bb, s,
				 sxv[fc[0]], syv[fc[0]], sxv[fc[1]], syv[fc[1]],
				 sxv[fc[2]], syv[fc[2]],
				 vcol[fc[0]], vcol[fc[1]], vcol[fc[2]]);
			fill_tri(bb, s,
				 sxv[fc[0]], syv[fc[0]], sxv[fc[2]], syv[fc[2]],
				 sxv[fc[3]], syv[fc[3]],
				 vcol[fc[0]], vcol[fc[2]], vcol[fc[3]]);
		}

		if (!frame)
			print(u"cube: frame 0 drawn, calling Blt...\r\n");

		ret = gop->blt(gop, bb, EFI_BLT_BUFFER_TO_VIDEO,
			       0, 0, ox, oy, s, s, 0);

		if (!frame) {
			print(u"cube: first Blt ret=");
			print_hex(ret & ~EFI_ERROR_MASK, 2);
			print(u"\r\n");
		}
		if (ret != EFI_SUCCESS) {
			print(u"cube: Blt failed - stopping\r\n");
			break;
		}

		/* heartbeat so 'alive but slow' is distinguishable from a hang */
		if (frame && !(frame & 63)) {
			print(u"cube: frame ");
			print_dec(frame);
			print(u"\r\n");
		}

		/* tumble */
		ax += 2;
		ay += 3;
		az += 1;

		if (con_in &&
		    con_in->read_key_stroke(con_in, &key) == EFI_SUCCESS)
			break;

		boottime->stall(16000);		/* ~throttle the frame rate */
	}

	print(u"cube: done\r\n");
	boottime->free_pool(bb);
}

/**
 * wait_key_timeout() - poll for a key for up to @ms milliseconds
 *
 * Non-blocking by design: we never sit in wait_for_event(), so a console
 * whose input never reaches the EFI con_in cannot wedge the app here.
 *
 * @ms:	maximum time to wait, in milliseconds
 */
static void wait_key_timeout(unsigned int ms)
{
	struct efi_simple_text_input_protocol *con_in = systable->con_in;
	struct efi_input_key key;
	unsigned int i;

	if (!con_in)
		return;
	for (i = 0; i < ms; i += 50) {
		if (con_in->read_key_stroke(con_in, &key) == EFI_SUCCESS)
			return;
		boottime->stall(50000);		/* 50 ms */
	}
}

/**
 * efi_main() - entry point of the EFI application
 *
 * @handle:	handle of the loaded image
 * @systab:	system table
 * Return:	status code
 */
efi_status_t EFIAPI efi_main(efi_handle_t handle,
			     struct efi_system_table *systab)
{
	efi_status_t ret;
	bool pass;

	systable = systab;
	boottime = systable->boottime;
	con_out = systable->con_out;

	print(u"\r\nUEFI GOP / graphics test\r\n");

	ret = boottime->locate_protocol(&gop_guid, NULL, (void **)&gop);
	if (ret != EFI_SUCCESS || !gop) {
		print(u"ERROR: no Graphics Output Protocol found.\r\n");
		print(u"Bring the display up first (e.g. 'tachyon dp start').\r\n");
		goto out;
	}

	scr_w = gop->mode->info->width;
	scr_h = gop->mode->info->height;

	print_mode_info();
	enumerate_modes();

	if (!scr_w || !scr_h) {
		print(u"\r\nResolution is 0 - skipping draw tests.\r\n");
		ret = EFI_DEVICE_ERROR;
		goto out;
	}

	draw_patterns();
	gradient_test();
	pass = readback_test();

	print(u"\r\n=== Static test result: ");
	print(pass ? u"GOP OK ===\r\n" : u"GOP readback FAILED ===\r\n");

	print(u"\r\nStarting rotating 3D cube (press a key to start now)...\r\n");
	wait_key_timeout(3000);

	cube_demo();
	ret = pass ? EFI_SUCCESS : EFI_DEVICE_ERROR;

out:
	/* Return control to the U-Boot console */
	boottime->exit(handle, ret, 0, NULL);

	/* Not reached */
	return ret;
}
