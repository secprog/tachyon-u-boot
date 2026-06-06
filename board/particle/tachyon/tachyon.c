// SPDX-License-Identifier: GPL-2.0+
/*
 * Board init file for Particle Tachyon
 *
 * (C) Copyright 2025 Particle Industries, Inc.
 */


#include <asm/global_data.h>
#include <log.h>
#include "qcom_dram.h"
#include <fdt_support.h>
#include <init.h>
#include <stdbool.h>
#include <dm/util.h>
#include <blk.h>
#include <command.h>
#include <debug_uart.h>
#include <env.h>
#include <part.h>
#include <scsi.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <memalign.h>
#include <net-common.h>
#include <fs.h>
#include <power/pmic.h>
#include <dm/ofnode.h>
#include <video.h>
#include <video_console.h>

#include "efs.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_CMD_TACHYON_DP
int tachyon_dp_cmd(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[]);
#endif

static efs_context s_efs = {};
typedef struct blkdev_context {
	struct blk_desc* desc;
	struct disk_partition info;
	bool mounted;
} blkdev_context;
blkdev_context s_efs_blk = {};

typedef struct fs_context {
	struct blk_desc* desc;
	struct disk_partition info;
	int partnum;
	void* fdt;
} fs_context;

#define TACHYON_FSG_PARTITION_NAME "fsg"
#define TACHYON_USERDATA_PARTITION_NAME "userdata"
#define TACHYON_FSG_WIFI_MAC_PATH "/nvm/num/4678"
#define TACHYON_FSG_BLUETOOTH_MAC_PATH "/nvm/num/447"
#define TACHYON_USERDATA_OVERLAYS_PATH "/boot"
#define TACHYON_USERDATA_OVERLAYS_FILE "overlays.txt"
#define TACHYON_OVERLAYS_PREFIX "overlays="

#define TACHYON_OS_TYPE_DT_NODE "sdam-os-type"
#define TACHYON_OS_TYPE_HLOS (0x01)
#define TACHYON_DP_DEFAULT_XRES 1920
#define TACHYON_DP_DEFAULT_YRES 1080
#define TACHYON_DP_MIN_XRES 640
#define TACHYON_DP_MIN_YRES 480
#define TACHYON_DP_MAX_XRES 3840
#define TACHYON_DP_MAX_YRES 2160
#define TACHYON_RESOLUTION_MENU_FIRST 1
#define TACHYON_RESOLUTION_MENU_MAX 8
#define TACHYON_BOOTMENU_FASTBOOT "Enable fastboot mode=run fastboot"
#define TACHYON_BOOTMENU_RESET "Reset device=reset"

// #define DEBUG

// #ifdef DEBUG
// #undef debug
// #define debug(fmt, args...) do { printf(fmt, ##args); } while (0)
// #endif // DEBUG

#define CHECK(_expr) \
	({ \
		const typeof(_expr) _ret = _expr; \
		if (_ret < 0) { \
			return _ret; \
		} \
		_ret; \
	})

int board_early_init_f(void)
{

	qcom_mem_bank banks[CONFIG_NR_DRAM_BANKS] = {};
	int num = qcom_parse_memory_smem(banks, CONFIG_NR_DRAM_BANKS);

	if (num > 0) {
		memset(qcom_get_memory_banks(), 0, sizeof(qcom_mem_bank) * CONFIG_NR_DRAM_BANKS);
		memcpy(qcom_get_memory_banks(), banks, sizeof(qcom_mem_bank) * num);

		phys_size_t ram_end = 0;
		for (int i = 0; i < num; i++) {
			ram_end = max(ram_end, banks[i].start + banks[i].size);
		}
		gd->ram_base = banks[0].start;
		gd->ram_size = ram_end - gd->ram_base;
	}

	return 0;
}

loff_t fsg_read(void* buf, loff_t offset, loff_t size, void* ctx) {
	blkdev_context* blk = (blkdev_context*)ctx;

	if (size <= 0) {
		return 0;
	}

	// Just one block should be fine, EFS driver won't request more than
	// the underlying EFS block size anyway
	size_t block_size = blk->desc->blksz;

	loff_t start_block = offset / block_size;
	loff_t skip_bytes = offset % block_size;
	loff_t block_count = DIV_ROUND_UP(size + skip_bytes, block_size);

	u8* block_buf = memalign(ARCH_DMA_MINALIGN, block_size * block_count);
	if (!block_buf) {
		return -ENOMEM;
	}

	if (blk_dread(blk->desc, blk->info.start + start_block, block_count, block_buf) == block_count) {
		memcpy(buf, block_buf + skip_bytes, size);
	} else {
		size = -1;
	}
	free(block_buf);
	return size;
}

int efs_logger(efs_loglevel level, void* ctx, const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);
	int r = vprintf(fmt, args);
	printf("\n");
	va_end(args);
	return r;
}

int tachyon_find_partition(const char* name, struct blk_desc** block, struct disk_partition* info) {
	struct udevice* dev = NULL;
	struct blk_desc* desc = NULL;
	int devnum = -1;
	int partnum = -1;

	uclass_foreach_dev_probe(UCLASS_BLK, dev) {
		if (device_get_uclass_id(dev) != UCLASS_BLK) {
			continue;
		}

		desc = dev_get_uclass_plat(dev);
		if (!desc || desc->part_type == PART_TYPE_UNKNOWN) {
			continue;
		}
		devnum = desc->devnum;
		partnum = part_get_info_by_name(desc, name, info);

		if (partnum >= 0) {
			*block = desc;
			return partnum;
		}
	}

	return -ENOENT;
}

static int tachyon_parse_resolution(const char* value, u32* width, u32* height) {
	char* end = NULL;
	ulong parsed_width;
	ulong parsed_height;

	if (!value || !width || !height) {
		return -EINVAL;
	}

	parsed_width = simple_strtoul(value, &end, 10);
	if (end == value || (*end != 'x' && *end != 'X')) {
		return -EINVAL;
	}

	value = end + 1;
	parsed_height = simple_strtoul(value, &end, 10);
	if (end == value) {
		return -EINVAL;
	}

	while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
		end++;
	}

	if (*end != '\0') {
		return -EINVAL;
	}

	if (parsed_width < TACHYON_DP_MIN_XRES ||
	    parsed_height < TACHYON_DP_MIN_YRES ||
	    parsed_width > TACHYON_DP_MAX_XRES ||
	    parsed_height > TACHYON_DP_MAX_YRES) {
		return -ERANGE;
	}

	*width = parsed_width;
	*height = parsed_height;
	return 0;
}

static int tachyon_set_resolution_env(u32 width, u32 height) {
	int ret;

	ret = env_set_ulong("tachyon_dp_xres", width);
	if (ret) {
		return ret;
	}

	return env_set_ulong("tachyon_dp_yres", height);
}

static bool tachyon_resolution_token(const char** modes, char* token,
				     size_t token_size) {
	const char* pos = *modes;
	size_t len = 0;

	while (*pos == ' ') {
		pos++;
	}

	if (!*pos) {
		*modes = pos;
		return false;
	}

	while (pos[len] && pos[len] != ' ') {
		len++;
	}

	if (!len || len >= token_size) {
		*modes = pos + len;
		return false;
	}

	memcpy(token, pos, len);
	token[len] = '\0';
	*modes = pos + len;
	return true;
}

static const char* tachyon_resolution_modes(void) {
	const char* modes = env_get("tachyon_dp_edid_modes");

	if (modes && *modes) {
		return modes;
	}

	return "1920x1080";
}

static int tachyon_resolution_build_menu(void) {
	const char* modes;
	struct udevice* vid = NULL;
	int idx = TACHYON_RESOLUTION_MENU_FIRST;
	int old;

	uclass_first_device_err(UCLASS_VIDEO, &vid);
	modes = tachyon_resolution_modes();

	while (*modes && idx < TACHYON_RESOLUTION_MENU_FIRST +
	       TACHYON_RESOLUTION_MENU_MAX) {
		char token[16];
		char name[16];
		char value[96];
		u32 width, height;

		if (!tachyon_resolution_token(&modes, token, sizeof(token))) {
			continue;
		}
		if (tachyon_parse_resolution(token, &width, &height)) {
			continue;
		}

		snprintf(name, sizeof(name), "bootmenu_%d", idx);
		snprintf(value, sizeof(value),
			 "Display: %s=tachyon resolution set %s save; bootmenu",
			 token, token);
		env_set(name, value);
		idx++;
	}

	{
		char name[16];

		snprintf(name, sizeof(name), "bootmenu_%d", idx++);
		env_set(name, TACHYON_BOOTMENU_FASTBOOT);
		snprintf(name, sizeof(name), "bootmenu_%d", idx++);
		env_set(name, TACHYON_BOOTMENU_RESET);
	}

	for (old = idx; old <= TACHYON_RESOLUTION_MENU_FIRST +
	     TACHYON_RESOLUTION_MENU_MAX + 3; old++) {
		char name[16];

		snprintf(name, sizeof(name), "bootmenu_%d", old);
		env_set(name, NULL);
	}

	return 0;
}

static int tachyon_apply_resolution(u32 width, u32 height) {
	struct udevice* vid = NULL;
	struct udevice* con = NULL;
	struct video_uc_plat* plat;
	struct video_priv* vid_priv;
	int ret;

	ret = uclass_first_device_err(UCLASS_VIDEO, &vid);
	if (ret) {
		return ret;
	}

	plat = dev_get_uclass_plat(vid);
	vid_priv = dev_get_uclass_priv(vid);
	if (width * height * VNBYTES(vid_priv->bpix) > plat->size) {
		return -ENOSPC;
	}

	vid_priv->xsize = width;
	vid_priv->ysize = height;
	vid_priv->line_length = width * VNBYTES(vid_priv->bpix);
	vid_priv->fb_size = vid_priv->line_length * height;

	uclass_foreach_dev_probe(UCLASS_VIDEO_CONSOLE, con) {
		struct vidconsole_priv* vc_priv;

		if (con->parent != vid) {
			continue;
		}

		vc_priv = dev_get_uclass_priv(con);
		if (!vc_priv->x_charsize || !vc_priv->y_charsize) {
			continue;
		}

		if (vid_priv->rot % 2) {
			vc_priv->cols = height / vc_priv->x_charsize;
			vc_priv->rows = width / vc_priv->y_charsize;
			vc_priv->xsize_frac = VID_TO_POS(height);
		} else {
			vc_priv->cols = width / vc_priv->x_charsize;
			vc_priv->rows = height / vc_priv->y_charsize;
			vc_priv->xsize_frac = VID_TO_POS(width);
		}
		vc_priv->xcur_frac = vc_priv->xstart_frac;
		vc_priv->ycur = 0;
	}

	ret = video_clear(vid);
	if (ret) {
		return ret;
	}

	return video_sync(vid, true);
}

static int tachyon_set_resolution(u32 width, u32 height, bool save) {
	int ret;

	ret = tachyon_set_resolution_env(width, height);
	if (ret) {
		return ret;
	}

	ret = tachyon_apply_resolution(width, height);
	if (ret && ret != -ENODEV) {
		printf("Failed to apply display resolution: %d\n", ret);
		return ret;
	}

	if (save) {
		ret = env_save();
		if (ret) {
			printf("Failed to save display resolution: %d\n", ret);
			return ret;
		}
	}

	printf("Display resolution set to %ux%u%s\n", width, height,
	       save ? " and saved" : "");
	return 0;
}

static int do_tachyon_resolution(struct cmd_tbl* cmdtp, int flag, int argc,
				 char* const argv[]) {
	u32 width = env_get_ulong("tachyon_dp_xres", 10,
				  TACHYON_DP_DEFAULT_XRES);
	u32 height = env_get_ulong("tachyon_dp_yres", 10,
				   TACHYON_DP_DEFAULT_YRES);
	int ret;

	if (argc < 2) {
		return CMD_RET_USAGE;
	}

	if (!strcmp(argv[1], "get")) {
		printf("%ux%u\n", width, height);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "list")) {
		const char* modes = tachyon_resolution_modes();

		while (*modes) {
			char token[16];

			if (tachyon_resolution_token(&modes, token,
						     sizeof(token)))
				printf("%s\n", token);
		}
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "menu")) {
		ret = tachyon_resolution_build_menu();
		if (ret) {
			printf("Failed to build display menu: %d\n", ret);
			return CMD_RET_FAILURE;
		}
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "load")) {
		ret = env_reload();
		if (ret) {
			printf("Failed to load display resolution: %d\n", ret);
			return CMD_RET_FAILURE;
		}
		width = env_get_ulong("tachyon_dp_xres", 10,
				      TACHYON_DP_DEFAULT_XRES);
		height = env_get_ulong("tachyon_dp_yres", 10,
				       TACHYON_DP_DEFAULT_YRES);
		return tachyon_set_resolution(width, height, false) ?
		       CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "save")) {
		ret = env_save();
		if (ret) {
			printf("Failed to save display resolution: %d\n", ret);
			return CMD_RET_FAILURE;
		}
		printf("Display resolution saved as %ux%u\n", width, height);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "set")) {
		bool save = argc >= 4 && !strcmp(argv[3], "save");

		if (argc < 3) {
			return CMD_RET_USAGE;
		}

		ret = tachyon_parse_resolution(argv[2], &width, &height);
		if (ret) {
			printf("Invalid display resolution '%s'\n", argv[2]);
			return CMD_RET_FAILURE;
		}

		return tachyon_set_resolution(width, height, save) ?
		       CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

static int do_tachyon(struct cmd_tbl* cmdtp, int flag, int argc,
		      char* const argv[]) {
	if (argc < 2) {
		return CMD_RET_USAGE;
	}

	if (!strcmp(argv[1], "resolution")) {
		return do_tachyon_resolution(cmdtp, flag, argc - 1, argv + 1);
	}

#ifdef CONFIG_CMD_TACHYON_DP
	if (!strcmp(argv[1], "dp")) {
		return tachyon_dp_cmd(cmdtp, flag, argc, argv);
	}
#endif

	return CMD_RET_USAGE;
}

#ifdef CONFIG_CMD_TACHYON_DP
#define TACHYON_DP_CMD_HELP \
	"\n" \
	"dp start\n" \
	"dp probe\n" \
	"dp sync\n" \
	"dp status"
#else
#define TACHYON_DP_CMD_HELP ""
#endif

U_BOOT_CMD(
	tachyon, 5, 1, do_tachyon,
	"Tachyon board utilities",
	"resolution get\n"
	"resolution list\n"
	"resolution load\n"
	"resolution menu\n"
	"resolution save\n"
	"resolution set <width>x<height> [save]"
	TACHYON_DP_CMD_HELP
);

static int tachyon_setup_efs(void) {
	bool mounted = s_efs_blk.mounted;
	struct blk_desc* desc = NULL;
	struct disk_partition info = {};
	s_efs_blk.mounted = 0;

	int partnum = CHECK(tachyon_find_partition(TACHYON_FSG_PARTITION_NAME, &desc, &info));
	printf("Found '%s' partition %d:%d block size=%lu/%lu\n", TACHYON_FSG_PARTITION_NAME, desc->devnum, partnum, desc->blksz, info.blksz);

	s_efs_blk.info = info;
	s_efs_blk.desc = desc;

	if (mounted) {
		s_efs_blk.mounted = mounted;
		return 0;
	}

	efs_ops ops = {
		.ctx = &s_efs_blk,
		.read = fsg_read,
		.logger = efs_logger
	};

	int r = efs_mount(&s_efs, ops);
	s_efs_blk.mounted = r == 0;
	return r;
}

static int efs_read_file(const char* filename, void* buffer, loff_t size) {
	efs_file* f = NULL;
	CHECK(efs_open(&s_efs, &f, filename, 0));
	int r = efs_read(&s_efs, f, buffer, 0, size);
	efs_close(&s_efs, f);
	return r;
}

static int parse_overlay_list(char* buf, int (*cb)(const char*, struct fs_context*), struct fs_context* ctx) {
	if (!buf || !cb) {
		return -EINVAL;
	}

	char *p = buf;
	char *eq = strstr(p, TACHYON_OVERLAYS_PREFIX);
	if (eq != NULL) {
		p = eq + sizeof(TACHYON_OVERLAYS_PREFIX) - 1;
	}

	for (char *q = p; *q != '\0'; q++) {
		if (*q == '\r' || *q == '\n' || *q == '\t') {
			*q = ' ';
		}
	}

	while (*p != '\0') {
		while (*p != '\0' && isspace((unsigned char)*p)) {
			p++;
		}

		if (*p == '\0') {
			break;
		}

		char* name = p;

		while (*p != '\0' && !isspace((unsigned char)*p)) {
			p++;
		}

		if (*p != '\0') {
			*p = '\0';
			p++;
		}

		int ret = cb(name, ctx);
		printf("Processing overlay '%s': %d\n", name, ret);
	}

	return 0;
}

int tachyon_apply_overlay(const char* name, struct fs_context* ctx) {
	char path[EFS_MAX_PATH] = {}; // XXX
	CHECK(snprintf(path, sizeof(path), "%s/%s", TACHYON_USERDATA_OVERLAYS_PATH, name));
	loff_t size = 0;
	CHECK(fs_set_blk_dev_with_part(ctx->desc, ctx->partnum));
	CHECK(fs_size(path, &size));

	void* overlay = calloc(size, 1);
	if (!overlay) {
		return -ENOMEM;
	}
	loff_t bytes_read = 0;
	CHECK(fs_set_blk_dev_with_part(ctx->desc, ctx->partnum));
	int ret = fs_read(path, (ulong)overlay, 0, 0, &bytes_read);
	if (ret < 0) {
		free(overlay);
		return ret;
	}

	printf("Read overlay '%s' size=%llu\n", path, size);

	if (fdt_check_header(overlay) != 0) {
		printf("Overlay '%s' is not a valid FDT\n", path);
		free(overlay);
		return -EINVAL;
	}

	loff_t grow = size + 0x1000; // headroom

	int r = fdt_increase_size(ctx->fdt, grow);
	if (r < 0) {
		if (r == -FDT_ERR_NOSPACE) {
			printf("Packing FDT\n");
			fdt_pack(ctx->fdt);
			r = fdt_increase_size(ctx->fdt, grow);
		}
		if (r < 0) {
			printf("Failed to grow FDT: %d", r);
		}
	}

	if (r >= 0) {
		r = fdt_overlay_apply(ctx->fdt, overlay);
	}

	free(overlay);

	return 0;
}

int tachyon_load_overlays(void* fdt) {
	fs_context ctx = {};

	ctx.partnum = CHECK(tachyon_find_partition(TACHYON_USERDATA_PARTITION_NAME, &ctx.desc, &ctx.info));
	printf("Found '%s' partition %d:%d block size=%lu/%lu\n", TACHYON_USERDATA_PARTITION_NAME, ctx.desc->devnum, ctx.partnum, ctx.desc->blksz, ctx.info.blksz);

	loff_t size = 0;
	CHECK(fs_set_blk_dev_with_part(ctx.desc, ctx.partnum));
	CHECK(fs_size(TACHYON_USERDATA_OVERLAYS_PATH "/" TACHYON_USERDATA_OVERLAYS_FILE, &size));
	char* overlay_list = calloc(size + 1, 1);
	if (!overlay_list) {
		return -ENOMEM;
	}
	loff_t read_bytes = 0;
	CHECK(fs_set_blk_dev_with_part(ctx.desc, ctx.partnum));
	int r = fs_read(TACHYON_USERDATA_OVERLAYS_PATH "/" TACHYON_USERDATA_OVERLAYS_FILE, (ulong)overlay_list, 0, size, &read_bytes);
	if (r < 0) {
		free(overlay_list);
		return r;
	}

	ctx.fdt = fdt;

	int ret = parse_overlay_list(overlay_list, tachyon_apply_overlay, &ctx);

	free(overlay_list);

	return ret;
}

int tachyon_pmic_configure(void) {
	struct udevice* dev = NULL;

	uclass_foreach_dev_probe(UCLASS_PMIC, dev) {
		ofnode node, subnode;

		ofnode_for_each_subnode(node, dev_ofnode(dev)) {
			ofnode_for_each_subnode(subnode, node) {
				const char* name = ofnode_get_name(subnode);
				if (name && !strncmp(name, TACHYON_OS_TYPE_DT_NODE, strlen(TACHYON_OS_TYPE_DT_NODE))) {
					u32 sdam_base_reg = 0;
					u32 sdam_reg[2] = {};

					CHECK(ofnode_read_u32(node, "reg", &sdam_base_reg));
					CHECK(ofnode_read_u32_array(subnode, "reg", sdam_reg, sizeof(sdam_reg) / sizeof(sdam_reg[0])));

					uint reg = sdam_base_reg + sdam_reg[0];

					u32 value = CHECK(pmic_reg_read(dev, reg));
					value |= TACHYON_OS_TYPE_HLOS;
					CHECK(pmic_reg_write(dev, reg, value));
					printf("Set OS type to HLOS in reg 0x%04x\n", reg);

					return 0;
				}
			}
		}
	}
	return -ENOENT;
}

/*
 * Board late-init hook (overrides the __weak qcom_late_init in
 * arch/arm/mach-snapdragon/board.c).  Set the PMIC SDAM "OS type" bit to HLOS
 * at U-Boot start, not only on the OS-boot DT-fixup path (ft_system_setup).
 *
 * The ADSP USB Type-C firmware reads this bit at LPM init: when it reads
 * "bootloader" it forces bPANEn=0 and runs a charging-only Type-C stack that
 * never reports DisplayPort alt-mode pin assignment / HPD.  Setting it to HLOS
 * before the DP path boots the ADSP lets the ADSP run the full alt-mode/DP
 * notification path the way it does under Linux.
 */
void qcom_late_init(void)
{
	int r = tachyon_pmic_configure();
	if (r < 0)
		printf("Failed to set OS type to HLOS at init: %d\n", r);
}

int tachyon_system_setup(void *fdt) {
	CHECK(tachyon_setup_efs());

	u8 mac[ARP_HLEN] = {};
	if (!eth_env_get_enetaddr("wlanaddr", mac)) {
		if (efs_read_file(TACHYON_FSG_WIFI_MAC_PATH, mac, ARP_HLEN) == ARP_HLEN) {
			printf("Found WiFi MAC address in FSG: %02x:%02x:%02x:%02x:%02x:%02x\n",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			const char* path = fdt_get_alias(fdt, "wlan");
			if (path) {
				do_fixup_by_path(fdt, path, "local-mac-address", mac, ARP_HLEN, 1);
				do_fixup_by_path(fdt, path, "mac-address", mac, ARP_HLEN, 1);
				printf("WiFi MAC fixed up in %s\n", path);
			}
		}
	}

	if (!eth_env_get_enetaddr("btaddr", mac)) {
		if (efs_read_file(TACHYON_FSG_BLUETOOTH_MAC_PATH, mac, ARP_HLEN) == ARP_HLEN) {
			// Bluetooth MAC is stored in reverse
			u8 reversed[ARP_HLEN] = {};
			for (int i = 0; i < sizeof(mac); i++) {
				reversed[i] = mac[sizeof(mac) - 1 - i];
			}
			printf("Found Bluetooth MAC address in FSG: %02x:%02x:%02x:%02x:%02x:%02x\n",
					reversed[0], reversed[1], reversed[2], reversed[3], reversed[4], reversed[5]);
			const char* path = fdt_get_alias(fdt, "bluetooth");
			if (path) {
				do_fixup_by_path(fdt, path, "local-bd-address", mac, ARP_HLEN, 1);
				printf("Bluetooth MAC fixed up in %s\n", path);
			}
		}
	}

	int r = tachyon_pmic_configure();
	if (r < 0) {
		printf("Failed to configure PMIC: %d\n", r);
	}

	return 0;
}

int ft_system_setup(void *fdt, struct bd_info *bd)
{
	int ret = tachyon_system_setup(fdt);
	if (ret < 0) {
		printf("Failed to setup\n");
	}

	tachyon_load_overlays(fdt); // ignore errors

	return 0;
}
