// SPDX-License-Identifier: GPL-2.0+
/*
 *  EFI application disk support
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define LOG_CATEGORY LOGC_EFI

#include <dm.h>
#include <efi_loader.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <video.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static const efi_guid_t efi_gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/**
 * struct efi_gop_obj - graphical output protocol object
 *
 * @header:	EFI object header
 * @ops:	graphical output protocol interface
 * @info:	graphical output mode information
 * @mode:	graphical output mode
 * @bpix:	bits per pixel
 * @fb:		frame buffer
 * @vdev:	underlying video uclass device (for video_set_mode etc.)
 * @plat:	video uclass platform data (for fb_base reference)
 */
struct efi_gop_obj {
	struct efi_object header;
	struct efi_gop ops;
	struct efi_gop_mode_info info;
	struct efi_gop_mode mode;
	/* Fields we only have access to during init */
	u32 bpix;
	void *fb;
	struct udevice *vdev;
	struct video_uc_plat *plat;
};

static efi_status_t EFIAPI gop_query_mode(struct efi_gop *this, u32 mode_number,
					  efi_uintn_t *size_of_info,
					  struct efi_gop_mode_info **info)
{
	struct efi_gop_obj *gopobj;
	struct video_ops *vops;
	u32 width, height;
	enum video_format fmt;
	enum video_log2_bpp bpix;
	int ret;

	EFI_ENTRY("%p, %x, %p, %p", this, mode_number, size_of_info, info);

	if (!this || !size_of_info || !info)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	gopobj = container_of(this, struct efi_gop_obj, ops);
	vops = video_get_ops(gopobj->vdev);

	if (vops && vops->video_get_mode_info &&
	    vops->video_get_mode_count(gopobj->vdev) > 0) {
		ret = vops->video_get_mode_info(gopobj->vdev, mode_number,
						&width, &height, &fmt, &bpix);
		if (ret)
			return EFI_EXIT(EFI_INVALID_PARAMETER);
	} else {
		/* Single-mode fallback: only mode 0 is valid */
		if (mode_number)
			return EFI_EXIT(EFI_INVALID_PARAMETER);
		width = gopobj->info.width;
		height = gopobj->info.height;
		fmt = VIDEO_X8R8G8B8;
		bpix = VIDEO_BPP32;
	}

	ret = efi_allocate_pool(EFI_BOOT_SERVICES_DATA, sizeof(**info),
				(void **)info);
	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	(*info)->version = 0;
	(*info)->width = width;
	(*info)->height = height;
	(*info)->pixels_per_scanline = width;

	if (bpix == VIDEO_BPP32) {
		if (fmt == VIDEO_X2R10G10B10) {
			(*info)->pixel_format = EFI_GOT_BITMASK;
			(*info)->pixel_bitmask[0] = 0x3ff00000; /* red */
			(*info)->pixel_bitmask[1] = 0x000ffc00; /* green */
			(*info)->pixel_bitmask[2] = 0x000003ff; /* blue */
			(*info)->pixel_bitmask[3] = 0xc0000000; /* reserved */
		} else {
			(*info)->pixel_format = EFI_GOT_BGRA8;
		}
	} else {
		(*info)->pixel_format = EFI_GOT_BITMASK;
		(*info)->pixel_bitmask[0] = 0xf800; /* red */
		(*info)->pixel_bitmask[1] = 0x07e0; /* green */
		(*info)->pixel_bitmask[2] = 0x001f; /* blue */
	}

	*size_of_info = sizeof(**info);

	return EFI_EXIT(EFI_SUCCESS);
}

static __always_inline struct efi_gop_pixel efi_vid30_to_blt_col(u32 vid)
{
	struct efi_gop_pixel blt = {
		.reserved = 0,
	};

	blt.blue  = (vid & 0x3ff) >> 2;
	vid >>= 10;
	blt.green = (vid & 0x3ff) >> 2;
	vid >>= 10;
	blt.red   = (vid & 0x3ff) >> 2;
	return blt;
}

static __always_inline u32 efi_blt_col_to_vid30(struct efi_gop_pixel *blt)
{
	return (u32)(blt->red   << 2) << 20 |
	       (u32)(blt->green << 2) << 10 |
	       (u32)(blt->blue  << 2);
}

static __always_inline struct efi_gop_pixel efi_vid16_to_blt_col(u16 vid)
{
	struct efi_gop_pixel blt = {
		.reserved = 0,
	};

	blt.blue  = (vid & 0x1f) << 3;
	vid >>= 5;
	blt.green = (vid & 0x3f) << 2;
	vid >>= 6;
	blt.red   = (vid & 0x1f) << 3;
	return blt;
}

static __always_inline u16 efi_blt_col_to_vid16(struct efi_gop_pixel *blt)
{
	return (u16)(blt->red   >> 3) << 11 |
	       (u16)(blt->green >> 2) <<  5 |
	       (u16)(blt->blue  >> 3);
}

static __always_inline efi_status_t gop_blt_int(struct efi_gop *this,
						struct efi_gop_pixel *bufferp,
						u32 operation, efi_uintn_t sx,
						efi_uintn_t sy, efi_uintn_t dx,
						efi_uintn_t dy,
						efi_uintn_t width,
						efi_uintn_t height,
						efi_uintn_t delta,
						efi_uintn_t vid_bpp)
{
	struct efi_gop_obj *gopobj = container_of(this, struct efi_gop_obj, ops);
	efi_uintn_t i, j, linelen, slineoff = 0, dlineoff, swidth, dwidth;
	u32 *fb32 = gopobj->fb;
	u16 *fb16 = gopobj->fb;
	struct efi_gop_pixel *buffer = __builtin_assume_aligned(bufferp, 4);

	if (delta) {
		/* Check for 4 byte alignment */
		if (delta & 3)
			return EFI_INVALID_PARAMETER;
		linelen = delta >> 2;
	} else {
		linelen = width;
	}

	/* Check source rectangle */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
		break;
	case EFI_BLT_BUFFER_TO_VIDEO:
		if (sx + width > linelen)
			return EFI_INVALID_PARAMETER;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
	case EFI_BLT_VIDEO_TO_VIDEO:
		if (sx + width > gopobj->info.width ||
		    sy + height > gopobj->info.height)
			return EFI_INVALID_PARAMETER;
		break;
	default:
		return EFI_INVALID_PARAMETER;
	}

	/* Check destination rectangle */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
	case EFI_BLT_BUFFER_TO_VIDEO:
	case EFI_BLT_VIDEO_TO_VIDEO:
		if (dx + width > gopobj->info.width ||
		    dy + height > gopobj->info.height)
			return EFI_INVALID_PARAMETER;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		if (dx + width > linelen)
			return EFI_INVALID_PARAMETER;
		break;
	}

	/* Calculate line width */
	switch (operation) {
	case EFI_BLT_BUFFER_TO_VIDEO:
		swidth = linelen;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
	case EFI_BLT_VIDEO_TO_VIDEO:
		swidth = gopobj->info.width;
		if (!vid_bpp)
			return EFI_UNSUPPORTED;
		break;
	case EFI_BLT_VIDEO_FILL:
		swidth = 0;
		break;
	}

	switch (operation) {
	case EFI_BLT_BUFFER_TO_VIDEO:
	case EFI_BLT_VIDEO_FILL:
	case EFI_BLT_VIDEO_TO_VIDEO:
		dwidth = gopobj->info.width;
		if (!vid_bpp)
			return EFI_UNSUPPORTED;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		dwidth = linelen;
		break;
	}

	slineoff = swidth * sy;
	dlineoff = dwidth * dy;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			struct efi_gop_pixel pix;

			/* Read source pixel */
			switch (operation) {
			case EFI_BLT_VIDEO_FILL:
				pix = *buffer;
				break;
			case EFI_BLT_BUFFER_TO_VIDEO:
				pix = buffer[slineoff + j + sx];
				break;
			case EFI_BLT_VIDEO_TO_BLT_BUFFER:
			case EFI_BLT_VIDEO_TO_VIDEO:
				if (vid_bpp == 32)
					pix = *(struct efi_gop_pixel *)&fb32[
						slineoff + j + sx];
				else if (vid_bpp == 30)
					pix = efi_vid30_to_blt_col(fb32[
						slineoff + j + sx]);
				else
					pix = efi_vid16_to_blt_col(fb16[
						slineoff + j + sx]);
				break;
			}

			/* Write destination pixel */
			switch (operation) {
			case EFI_BLT_VIDEO_TO_BLT_BUFFER:
				buffer[dlineoff + j + dx] = pix;
				break;
			case EFI_BLT_BUFFER_TO_VIDEO:
			case EFI_BLT_VIDEO_FILL:
			case EFI_BLT_VIDEO_TO_VIDEO:
				if (vid_bpp == 32)
					fb32[dlineoff + j + dx] = *(u32 *)&pix;
				else if (vid_bpp == 30)
					fb32[dlineoff + j + dx] =
						efi_blt_col_to_vid30(&pix);
				else
					fb16[dlineoff + j + dx] =
						efi_blt_col_to_vid16(&pix);
				break;
			}
		}
		slineoff += swidth;
		dlineoff += dwidth;
	}

	return EFI_SUCCESS;
}

static efi_uintn_t gop_get_bpp(struct efi_gop *this)
{
	struct efi_gop_obj *gopobj = container_of(this, struct efi_gop_obj, ops);
	efi_uintn_t vid_bpp = 0;

	switch (gopobj->bpix) {
	case VIDEO_BPP32:
		if (gopobj->info.pixel_format == EFI_GOT_BGRA8)
			vid_bpp = 32;
		else
			vid_bpp = 30;
		break;
	case VIDEO_BPP16:
		vid_bpp = 16;
		break;
	}

	return vid_bpp;
}

/*
 * GCC can't optimize our BLT function well, but we need to make sure that
 * our 2-dimensional loop gets executed very quickly, otherwise the system
 * will feel slow.
 *
 * By manually putting all obvious branch targets into functions which call
 * our generic BLT function with constants, the compiler can successfully
 * optimize for speed.
 */
static efi_status_t gop_blt_video_fill(struct efi_gop *this,
				       struct efi_gop_pixel *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_FILL, sx, sy, dx,
			   dy, width, height, delta, vid_bpp);
}

static efi_status_t gop_blt_buf_to_vid16(struct efi_gop *this,
					 struct efi_gop_pixel *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 16);
}

static efi_status_t gop_blt_buf_to_vid30(struct efi_gop *this,
					 struct efi_gop_pixel *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 30);
}

static efi_status_t gop_blt_buf_to_vid32(struct efi_gop *this,
					 struct efi_gop_pixel *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 32);
}

static efi_status_t gop_blt_vid_to_vid(struct efi_gop *this,
				       struct efi_gop_pixel *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, vid_bpp);
}

static efi_status_t gop_blt_vid_to_buf(struct efi_gop *this,
				       struct efi_gop_pixel *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_TO_BLT_BUFFER, sx, sy,
			   dx, dy, width, height, delta, vid_bpp);
}

/**
 * gop_set_mode() - set graphical output mode
 *
 * This function implements the SetMode() service.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		the graphical output protocol
 * @mode_number:	the mode to be set
 * Return:		status code
 */
static efi_status_t EFIAPI gop_set_mode(struct efi_gop *this, u32 mode_number)
{
	struct efi_gop_obj *gopobj;
	struct video_ops *vops;
	struct video_priv *priv;
	struct efi_gop_pixel buffer = {0, 0, 0, 0};
	efi_uintn_t vid_bpp;
	int ret;

	EFI_ENTRY("%p, %x", this, mode_number);

	if (!this)
		return EFI_EXIT(EFI_INVALID_PARAMETER);

	gopobj = container_of(this, struct efi_gop_obj, ops);
	vops = video_get_ops(gopobj->vdev);

	if (vops && vops->video_set_mode &&
	    vops->video_get_mode_count &&
	    vops->video_get_mode_count(gopobj->vdev) > 0) {
		/* Real hardware mode switch via the video driver */
		ret = vops->video_set_mode(gopobj->vdev, mode_number);
		if (ret)
			return EFI_EXIT(EFI_UNSUPPORTED);

		/* Reflect new mode in GOP fields */
		priv = dev_get_uclass_priv(gopobj->vdev);
		gopobj->info.width = priv->xsize;
		gopobj->info.height = priv->ysize;
		gopobj->info.pixels_per_scanline = priv->xsize;
		gopobj->bpix = priv->bpix;
		if (priv->bpix == VIDEO_BPP32) {
			if (priv->format == VIDEO_X2R10G10B10) {
				gopobj->info.pixel_format = EFI_GOT_BITMASK;
				gopobj->info.pixel_bitmask[0] = 0x3ff00000;
				gopobj->info.pixel_bitmask[1] = 0x000ffc00;
				gopobj->info.pixel_bitmask[2] = 0x000003ff;
				gopobj->info.pixel_bitmask[3] = 0xc0000000;
			} else {
				gopobj->info.pixel_format = EFI_GOT_BGRA8;
			}
		} else {
			gopobj->info.pixel_format = EFI_GOT_BITMASK;
			gopobj->info.pixel_bitmask[0] = 0xf800;
			gopobj->info.pixel_bitmask[1] = 0x07e0;
			gopobj->info.pixel_bitmask[2] = 0x001f;
		}
		gopobj->mode.mode = mode_number;
		gopobj->fb = map_sysmem(gopobj->mode.fb_base,
					gopobj->mode.fb_size);
		video_sync_all();
	} else {
		/* Legacy single-mode: only mode 0 = clear FB */
		if (mode_number)
			return EFI_EXIT(EFI_UNSUPPORTED);
		vid_bpp = gop_get_bpp(this);
		gopobj->mode.mode = 0;
		ret = gop_blt_video_fill(this, &buffer, EFI_BLT_VIDEO_FILL,
					 0, 0, 0, 0, gopobj->info.width,
					 gopobj->info.height, 0, vid_bpp);
		if (ret != EFI_SUCCESS)
			return EFI_EXIT(ret);
	}

	return EFI_EXIT(EFI_SUCCESS);
}

/*
 * Copy rectangle.
 *
 * This function implements the Blt service of the EFI_GRAPHICS_OUTPUT_PROTOCOL.
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:	EFI_GRAPHICS_OUTPUT_PROTOCOL
 * @buffer:	pixel buffer
 * @sx:		source x-coordinate
 * @sy:		source y-coordinate
 * @dx:		destination x-coordinate
 * @dy:		destination y-coordinate
 * @width:	width of rectangle
 * @height:	height of rectangle
 * @delta:	length in bytes of a line in the pixel buffer (optional)
 * Return:	status code
 */
static efi_status_t EFIAPI gop_blt(struct efi_gop *this,
				   struct efi_gop_pixel *buffer,
				   u32 operation, efi_uintn_t sx,
				   efi_uintn_t sy, efi_uintn_t dx,
				   efi_uintn_t dy, efi_uintn_t width,
				   efi_uintn_t height, efi_uintn_t delta)
{
	efi_status_t ret = EFI_INVALID_PARAMETER;
	efi_uintn_t vid_bpp;

	EFI_ENTRY("%p, %p, %u, %zu, %zu, %zu, %zu, %zu, %zu, %zu", this,
		  buffer, operation, sx, sy, dx, dy, width, height, delta);

	vid_bpp = gop_get_bpp(this);

	/* Allow for compiler optimization */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
		ret = gop_blt_video_fill(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	case EFI_BLT_BUFFER_TO_VIDEO:
		/* This needs to be super-fast, so duplicate for 16/32bpp */
		if (vid_bpp == 32)
			ret = gop_blt_buf_to_vid32(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		else if (vid_bpp == 30)
			ret = gop_blt_buf_to_vid30(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		else
			ret = gop_blt_buf_to_vid16(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		break;
	case EFI_BLT_VIDEO_TO_VIDEO:
		ret = gop_blt_vid_to_vid(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		ret = gop_blt_vid_to_buf(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	default:
		ret = EFI_INVALID_PARAMETER;
	}

	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	video_sync_all();

	return EFI_EXIT(EFI_SUCCESS);
}

/*
 * Install graphical output protocol.
 *
 * If no supported video device exists this is not considered as an
 * error.
 */
efi_status_t efi_gop_register(void)
{
	struct efi_gop_obj *gopobj;
	u32 bpix, format, col, row;
	int mode_count = 1;
	u64 fb_base, fb_size;
	efi_status_t ret;
	struct udevice *vdev;
	struct video_priv *priv;
	struct video_uc_plat *plat;
	struct video_ops *vops;

	/* We only support a single video output device for now */
	if (uclass_first_device_err(UCLASS_VIDEO, &vdev)) {
		debug("WARNING: No video device\n");
		return EFI_SUCCESS;
	}

	priv = dev_get_uclass_priv(vdev);
	bpix = priv->bpix;
	format = priv->format;
	col = video_get_xsize(vdev);
	row = video_get_ysize(vdev);

	plat = dev_get_uclass_plat(vdev);
	fb_base = IS_ENABLED(CONFIG_VIDEO_COPY) ? plat->copy_base : plat->base;
	fb_size = plat->size;

	vops = video_get_ops(vdev);
	if (vops && vops->video_get_mode_count) {
		mode_count = vops->video_get_mode_count(vdev);
		if (mode_count < 1)
			mode_count = 1;
	}

	switch (bpix) {
	case VIDEO_BPP16:
	case VIDEO_BPP32:
		break;
	default:
		/* So far, we only work in 16 or 32 bit mode */
		debug("WARNING: Unsupported video mode\n");
		return EFI_SUCCESS;
	}

	gopobj = calloc(1, sizeof(*gopobj));
	if (!gopobj) {
		printf("ERROR: Out of memory\n");
		return EFI_OUT_OF_RESOURCES;
	}

	/* Store references for SetMode() and QueryMode() callbacks */
	gopobj->vdev = vdev;
	gopobj->plat = plat;

	/* Hook up to the device list */
	efi_add_handle(&gopobj->header);

	/* Fill in object data */
	ret = efi_add_protocol(&gopobj->header, &efi_gop_guid,
			       &gopobj->ops);
	if (ret != EFI_SUCCESS) {
		printf("ERROR: Failure adding GOP protocol\n");
		return ret;
	}
	gopobj->ops.query_mode = gop_query_mode;
	gopobj->ops.set_mode = gop_set_mode;
	gopobj->ops.blt = gop_blt;
	gopobj->ops.mode = &gopobj->mode;

	gopobj->mode.max_mode = mode_count;
	gopobj->mode.info = &gopobj->info;
	gopobj->mode.info_size = sizeof(gopobj->info);

	gopobj->mode.fb_base = fb_base;
	gopobj->mode.fb_size = fb_size;

	gopobj->info.version = 0;
	gopobj->info.width = col;
	gopobj->info.height = row;
	if (bpix == VIDEO_BPP32)
	{
		if (format == VIDEO_X2R10G10B10) {
			gopobj->info.pixel_format = EFI_GOT_BITMASK;
			gopobj->info.pixel_bitmask[0] = 0x3ff00000; /* red */
			gopobj->info.pixel_bitmask[1] = 0x000ffc00; /* green */
			gopobj->info.pixel_bitmask[2] = 0x000003ff; /* blue */
			gopobj->info.pixel_bitmask[3] = 0xc0000000; /* reserved */
		} else {
			gopobj->info.pixel_format = EFI_GOT_BGRA8;
		}
	} else {
		gopobj->info.pixel_format = EFI_GOT_BITMASK;
		gopobj->info.pixel_bitmask[0] = 0xf800; /* red */
		gopobj->info.pixel_bitmask[1] = 0x07e0; /* green */
		gopobj->info.pixel_bitmask[2] = 0x001f; /* blue */
	}
	gopobj->info.pixels_per_scanline = col;
	gopobj->bpix = bpix;
	gopobj->fb = map_sysmem(fb_base, fb_size);

	/*
	 * Re-mark the framebuffer as EFI_LOADER_DATA so Windows-on-ARM's
	 * winload maps it during early boot.  winload's on-demand mapper
	 * (BlMmMapPhysicalAddress) only maps a physical page that is already
	 * a member of its loader-descriptor DB; EfiMemoryMappedIO/Reserved
	 * regions are ingested too late (after winload first touches the FB,
	 * e.g. a dc cvac, which then faults), whereas EFI_LOADER_DATA regions
	 * are merged into the DB at the earliest loader stage.  The FB is a
	 * real DRAM lmb allocation, so LOADER_DATA is reclaim-safe (it is RAM,
	 * and the OS re-inits the display via its own driver post-handoff).
	 *
	 * We carve the FB from whatever type it currently is
	 * (overlap_conventional=false) and re-add it with EFI_MEMORY_WC for
	 * framebuffer performance and to prevent stale cache lines from
	 * causing a black/blank display after ExitBootServices().
	 */
	ret = efi_add_memory_map_attr(fb_base, fb_size,
				      EFI_LOADER_DATA,
				      EFI_MEMORY_WC);
	if (ret != EFI_SUCCESS)
		log_warning("Failed to reserve FB memory map entry: %lx\n",
			    ret & ~EFI_ERROR_MASK);

	return EFI_SUCCESS;
}
