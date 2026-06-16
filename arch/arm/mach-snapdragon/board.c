// SPDX-License-Identifier: GPL-2.0+
/*
 * Common initialisation for Qualcomm Snapdragon boards.
 *
 * Copyright (c) 2024 Linaro Ltd.
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#define LOG_CATEGORY LOGC_BOARD
#define pr_fmt(fmt) "QCOM: " fmt

#include <asm/armv8/mmu.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/system.h>
#include <dm/device.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <dm/uclass.h>
#include <dm/read.h>
#include <power/regulator.h>
#include <env.h>
#include <fdt_support.h>
#include <init.h>
#include <linux/arm-smccc.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/psci.h>
#include <linux/sizes.h>
#include <lmb.h>
#include <malloc.h>
#include <fdt_support.h>
#include <usb.h>
#include <sort.h>
#include <time.h>
#include <soc/qcom/qcom_adsp_pas.h>

#include "qcom-priv.h"

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region rbx_mem_map[CONFIG_NR_DRAM_BANKS + 2] = { { 0 } };

struct mm_region *mem_map = rbx_mem_map;

qcom_mem_bank prevbl_ddr_banks[CONFIG_NR_DRAM_BANKS] __section(".data") = { 0 };

qcom_mem_bank* qcom_get_memory_banks(void) {
	return prevbl_ddr_banks;
}

static int ddr_bank_cmp(const void *v1, const void *v2)
{
	const qcom_mem_bank *res1 = v1, *res2 = v2;

	if (!res1->size)
		return 1;
	if (!res2->size)
		return -1;

	return (res1->start >> 24) - (res2->start >> 24);
}

void qcom_sort_memory_banks(qcom_mem_bank* banks, size_t size) {
	qsort(banks, size, sizeof(banks[0]), ddr_bank_cmp);
}

int dram_init(void)
{
	/*
	 * gd->ram_base / ram_size have been setup already
	 * in qcom_parse_memory().
	 */
	return 0;
}

/* This has to be done post-relocation since gd->bd isn't preserved */
static void qcom_configure_bi_dram(void)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		gd->bd->bi_dram[i].start = prevbl_ddr_banks[i].start;
		gd->bd->bi_dram[i].size = prevbl_ddr_banks[i].size;
	}
}

int dram_init_banksize(void)
{
	qcom_configure_bi_dram();

	return 0;
}

static void qcom_parse_memory(const void *fdt)
{
	int offset;
	const fdt64_t *memory;
	int memsize;
	phys_addr_t ram_end = 0;
	int i, j, banks;

	offset = fdt_path_offset(fdt, "/memory");
	if (offset < 0) {
		log_err("No memory node found in device tree!\n");
		return;
	}

	memory = fdt_getprop(fdt, offset, "reg", &memsize);
	if (!memory) {
		log_err("No memory configuration was provided by the previous bootloader!\n");
		return;
	}

	banks = min(memsize / (2 * sizeof(u64)), (ulong)CONFIG_NR_DRAM_BANKS);

	if (memsize / sizeof(u64) > CONFIG_NR_DRAM_BANKS * 2)
		log_err("Provided more than the max of %d memory banks\n", CONFIG_NR_DRAM_BANKS);

	if (banks > CONFIG_NR_DRAM_BANKS)
		log_err("Provided more memory banks than we can handle\n");

	for (i = 0, j = 0; i < banks * 2; i += 2, j++) {
		prevbl_ddr_banks[j].start = get_unaligned_be64(&memory[i]);
		prevbl_ddr_banks[j].size = get_unaligned_be64(&memory[i + 1]);
		/* SM8650 boards sometimes have empty regions! */
		if (!prevbl_ddr_banks[j].size) {
			j--;
			continue;
		}
		ram_end = max(ram_end, prevbl_ddr_banks[j].start + prevbl_ddr_banks[j].size);
	}

	/* Sort our RAM banks -_- */
	qcom_sort_memory_banks(prevbl_ddr_banks, banks);

	gd->ram_base = prevbl_ddr_banks[0].start;
	gd->ram_size = ram_end - gd->ram_base;
	debug("ram_base = %#011lx, ram_size = %#011llx, ram_end = %#011llx\n",
	      gd->ram_base, gd->ram_size, ram_end);
}

static void show_psci_version(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);

	debug("PSCI:  v%ld.%ld\n",
	      PSCI_VERSION_MAJOR(res.a0),
	      PSCI_VERSION_MINOR(res.a0));
}

/* We support booting U-Boot with an internal DT when running as a first-stage bootloader
 * or for supporting quirky devices where it's easier to leave the downstream DT in place
 * to improve ABL compatibility. Otherwise, we use the DT provided by ABL.
 */
int board_fdt_blob_setup(void **fdtp)
{
	struct fdt_header *fdt;
	bool internal_valid, external_valid;
	int ret = 0;

	fdt = (struct fdt_header *)get_prev_bl_fdt_addr();
	external_valid = fdt && !fdt_check_header(fdt);
	internal_valid = !fdt_check_header(*fdtp);

	/*
	 * There is no point returning an error here, U-Boot can't do anything useful in this situation.
	 * Bail out while we can still print a useful error message.
	 */
	if (!internal_valid && !external_valid)
		panic("Internal FDT is invalid and no external FDT was provided! (fdt=%#llx)\n",
		      (phys_addr_t)fdt);

	if (internal_valid) {
		debug("Using built in FDT\n");
		ret = -EEXIST;
	} else {
		debug("Using external FDT\n");
		/* So we can use it before returning */
		*fdtp = fdt;
	}

	/*
	 * Parse the /memory node while we're here,
	 * this makes it easy to do other things early.
	 */
	qcom_parse_memory(*fdtp);

	return ret;
}

void reset_cpu(void)
{
	psci_system_reset();
}

/*
 * Some Qualcomm boards require GPIO configuration when switching USB modes.
 * Support setting this configuration via pinctrl state.
 */
int board_usb_init(int index, enum usb_init_type init)
{
	struct udevice *usb;
	int ret = 0;

	/* USB device */
	ret = uclass_find_device_by_seq(UCLASS_USB, index, &usb);
	if (ret) {
		printf("Cannot find USB device\n");
		return ret;
	}

	ret = dev_read_stringlist_search(usb, "pinctrl-names",
					 "device");
	/* No "device" pinctrl state, so just bail */
	if (ret < 0)
		return 0;

	/* Select "default" or "device" pinctrl */
	switch (init) {
	case USB_INIT_HOST:
		pinctrl_select_state(usb, "default");
		break;
	case USB_INIT_DEVICE:
		pinctrl_select_state(usb, "device");
		break;
	default:
		debug("Unknown usb_init_type %d\n", init);
		break;
	}

	return 0;
}

/*
 * Some boards still need board specific init code, they can implement that by
 * overriding this function.
 *
 * FIXME: get rid of board specific init code
 */
void __weak qcom_board_init(void)
{
}

int board_init(void)
{
	show_psci_version();
	qcom_of_fixup_nodes();
	qcom_board_init();
	return 0;
}

/**
 * out_len includes the trailing null space
 */
static int get_cmdline_option(const char *cmdline, const char *key, char *out, int out_len)
{
	const char *p, *p_end;
	int len;

	p = strstr(cmdline, key);
	if (!p)
		return -ENOENT;

	p += strlen(key);
	p_end = strstr(p, " ");
	if (!p_end)
		return -ENOENT;

	len = p_end - p;
	if (len > out_len)
		len = out_len;

	strncpy(out, p, len);
	out[len] = '\0';

	return 0;
}

/* The bootargs are populated by the previous stage bootloader */
static const char *get_cmdline(void)
{
	ofnode node;
	static const char *cmdline = NULL;

	if (cmdline)
		return cmdline;

	node = ofnode_path("/chosen");
	if (!ofnode_valid(node))
		return NULL;

	cmdline = ofnode_read_string(node, "bootargs");

	return cmdline;
}

void qcom_set_serialno(void)
{
	const char *cmdline = get_cmdline();
	char serial[32];

	if (!cmdline) {
		log_debug("Failed to get bootargs\n");
		return;
	}

	get_cmdline_option(cmdline, "androidboot.serialno=", serial, sizeof(serial));
	if (serial[0] != '\0')
		env_set("serial#", serial);
}

/* Sets up the "board", and "soc" environment variables as well as constructing the devicetree
 * path, with a few quirks to handle non-standard dtb filenames. This is not meant to be a
 * comprehensive solution to automatically picking the DTB, but aims to be correct for the
 * majority case. For most devices it should be possible to make this algorithm work by
 * adjusting the root compatible property in the U-Boot DTS. Handling devices with multiple
 * variants that are all supported by a single U-Boot image will require implementing device-
 * specific detection.
 */
static void configure_env(void)
{
	const char *first_compat, *last_compat;
	char *tmp;
	char buf[32] = { 0 };
	/*
	 * Most DTB filenames follow the scheme: qcom/<soc>-[vendor]-<board>.dtb
	 * The vendor is skipped when it's a Qualcomm reference board, or the
	 * db845c.
	 */
	char dt_path[64] = { 0 };
	int compat_count, ret;
	ofnode root;

	root = ofnode_root();
	/* This is almost always 2, but be explicit that we want the first and last compatibles
	 * not the first and second.
	 */
	compat_count = ofnode_read_string_count(root, "compatible");
	if (compat_count < 2) {
		log_warning("%s: only one root compatible bailing!\n", __func__);
		return;
	}

	/* The most specific device compatible (e.g. "thundercomm,db845c") */
	ret = ofnode_read_string_index(root, "compatible", 0, &first_compat);
	if (ret < 0) {
		log_warning("Can't read first compatible\n");
		return;
	}

	/* The last compatible is always the SoC compatible */
	ret = ofnode_read_string_index(root, "compatible", compat_count - 1, &last_compat);
	if (ret < 0) {
		log_warning("Can't read second compatible\n");
		return;
	}

	/* Copy the second compat (e.g. "qcom,sdm845") into buf */
	strlcpy(buf, last_compat, sizeof(buf) - 1);
	tmp = buf;

	/* strsep() is destructive, it replaces the comma with a \0 */
	if (!strsep(&tmp, ",")) {
		log_warning("second compatible '%s' has no ','\n", buf);
		return;
	}

	/* tmp now points to just the "sdm845" part of the string */
	env_set("soc", tmp);

	/* Now figure out the "board" part from the first compatible */
	memset(buf, 0, sizeof(buf));
	strlcpy(buf, first_compat, sizeof(buf) - 1);
	tmp = buf;

	/* The Qualcomm reference boards (RBx, HDK, etc)  */
	if (!strncmp("qcom", buf, strlen("qcom"))) {
		/*
		 * They all have the first compatible as "qcom,<soc>-<board>"
		 * (e.g. "qcom,qrb5165-rb5"). We extract just the part after
		 * the dash.
		 */
		if (!strsep(&tmp, "-")) {
			log_warning("compatible '%s' has no '-'\n", buf);
			return;
		}
		/* tmp is now "rb5" */
		env_set("board", tmp);
	} else {
		if (!strsep(&tmp, ",")) {
			log_warning("compatible '%s' has no ','\n", buf);
			return;
		}
		/* for thundercomm we just want the bit after the comma (e.g. "db845c"),
		 * for all other boards we replace the comma with a '-' and take both
		 * (e.g. "oneplus-enchilada")
		 */
		if (!strncmp("thundercomm", buf, strlen("thundercomm"))) {
			env_set("board", tmp);
		} else {
			*(tmp - 1) = '-';
			env_set("board", buf);
		}
	}

	/* Now build the full path name */
	snprintf(dt_path, sizeof(dt_path), "qcom/%s-%s.dtb",
		 env_get("soc"), env_get("board"));
	env_set("fdtfile", dt_path);

	qcom_set_serialno();
}

void __weak qcom_late_init(void)
{
}

#define KERNEL_COMP_SIZE	SZ_64M
#ifdef CONFIG_FASTBOOT_BUF_SIZE
#define FASTBOOT_BUF_SIZE CONFIG_FASTBOOT_BUF_SIZE
#else
#define FASTBOOT_BUF_SIZE 0
#endif

#define addr_alloc(size) lmb_alloc(size, SZ_2M)

/* Stolen from arch/arm/mach-apple/board.c */
int board_late_init(void)
{
	u32 status = 0;
	phys_addr_t addr;
	struct fdt_header *fdt_blob = (struct fdt_header *)gd->fdt_blob;

	/* We need to be fairly conservative here as we support boards with just 1G of TOTAL RAM */
	addr = addr_alloc(SZ_128M);
	status |= env_set_hex("kernel_addr_r", addr);
	status |= env_set_hex("loadaddr", addr);
	status |= env_set_hex("ramdisk_addr_r", addr_alloc(SZ_128M));
	status |= env_set_hex("kernel_comp_addr_r", addr_alloc(KERNEL_COMP_SIZE));
	status |= env_set_hex("kernel_comp_size", KERNEL_COMP_SIZE);
	if (IS_ENABLED(CONFIG_FASTBOOT))
		status |= env_set_hex("fastboot_addr_r", addr_alloc(FASTBOOT_BUF_SIZE));
	status |= env_set_hex("scriptaddr", addr_alloc(SZ_4M));
	status |= env_set_hex("pxefile_addr_r", addr_alloc(SZ_4M));
	addr = addr_alloc(SZ_2M);
	status |= env_set_hex("fdt_addr_r", addr);

	if (status)
		log_warning("%s: Failed to set run time variables\n", __func__);

	/* By default copy U-Boots FDT, it will be used as a fallback */
	memcpy((void *)addr, (void *)gd->fdt_blob, fdt32_to_cpu(fdt_blob->totalsize));

	configure_env();
	qcom_late_init();

	/*
	 * Bring up the ADSP remoteproc independently of pmic-glink so the
	 * Type-C/DisplayPort GLINK path finds it already running.  Best-effort:
	 * a boot failure must not abort U-Boot (the rest of the system still
	 * boots, just without Type-C/DP).  qcom_adsp_pas_boot() is a no-op stub
	 * when CONFIG_QCOM_ADSP_PAS is disabled.
	 */
	if (IS_ENABLED(CONFIG_QCOM_ADSP_PAS)) {
		int adsp_ret = qcom_adsp_pas_boot();

		if (adsp_ret)
			log_warning("%s: ADSP PAS boot ret=%d (Type-C/DP unavailable)\n",
				    __func__, adsp_ret);
	}

	/*
	 * Probe the DisplayPort video device so DP comes up autonomously at
	 * boot (trains the link, lights the dock, leaves a live GOP framebuffer
	 * for Windows).  The video uclass does NOT probe it on its own here:
	 * CONFIG_SYS_CONSOLE_IS_IN_ENV makes stdio_add_devices skip its
	 * probe-all loop, and stdout carries no vidconsole.  Best-effort -- a DP
	 * failure (e.g. no dock attached) must not abort boot; tachyon_dp_probe
	 * quiesces cleanly on error.
	 *
	 * A COLD dock needs to be left alone until it has settled into a PD
	 * contract: the probe's first act is qpg_open_session(), whose UCSI
	 * PPM_RESET, if it fires while the dock is still mid-negotiation, knocks
	 * the dock to opmode=5 and the contract never forms -- HW-confirmed (with
	 * no delay a cold boot lands at opmode=5/mux=0/no-DP).  So delay the probe
	 * past the cold dock's ~30s settle window; tunable via
	 * tachyon_dp_settle_ms.  A WARM reboot keeps its contract (opmode=3) and a
	 * settled dock is NOT disturbed by open_session, so the delay is only
	 * needed for cold -- a future refinement could detect warm and skip it.
	 */
	if (IS_ENABLED(CONFIG_VIDEO_QCOM_TACHYON_DP)) {
		ulong settle_ms = env_get_ulong("tachyon_dp_settle_ms", 10,
						18000);
		int dp_ret;

		if (settle_ms)
			mdelay(settle_ms);

		dp_ret = uclass_probe_all(UCLASS_VIDEO);
		if (dp_ret)
			log_warning("%s: DP video probe ret=%d\n", __func__,
				    dp_ret);

		/*
		 * Route the U-Boot console onto the dock now that DP is up.  The
		 * video probe registered a "vidconsole" stdio device, but it did
		 * not exist back at console_init_r (CONFIG_SYS_CONSOLE_IS_IN_ENV
		 * skips the probe-all there), so stdout is still serial-only.
		 * Re-setting stdout/stderr fires the on_console env callback, which
		 * re-evaluates the CONSOLE_MUX iomux and adds the vidconsole --
		 * serial stays, so the serial console is unaffected and the
		 * prompt/output also render on the dock.  Only when the probe
		 * succeeded (the vidconsole is present); a no-op otherwise.
		 */
		if (!dp_ret) {
			env_set("stdout", "serial,vidconsole");
			env_set("stderr", "serial,vidconsole");
		}
	}

	/* Configure the dfu_string for capsule updates */
	qcom_configure_capsule_updates();

	return 0;
}

static void build_mem_map(void)
{
	int i, j;

	/*
	 * Ensure the peripheral block is sized to correctly cover the address range
	 * up to the first memory bank.
	 * Don't map the first page to ensure that we actually trigger an abort on a
	 * null pointer access rather than just hanging.
	 * FIXME: we should probably split this into more precise regions
	 */
	mem_map[0].phys = 0x1000;
	mem_map[0].virt = mem_map[0].phys;
	mem_map[0].size = gd->bd->bi_dram[0].start - mem_map[0].phys;
	mem_map[0].attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN;

	for (i = 1, j = 0; i < ARRAY_SIZE(rbx_mem_map) - 1 && gd->bd->bi_dram[j].size; i++, j++) {
		mem_map[i].phys = gd->bd->bi_dram[j].start;
		mem_map[i].virt = mem_map[i].phys;
		mem_map[i].size = gd->bd->bi_dram[j].size;
		mem_map[i].attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) | \
				   PTE_BLOCK_INNER_SHARE;
	}

	mem_map[i].phys = UINT64_MAX;
	mem_map[i].size = 0;

#ifdef DEBUG
	debug("Configured memory map:\n");
	for (i = 0; mem_map[i].size; i++)
		debug("  0x%016llx - 0x%016llx: entry %d\n",
		      mem_map[i].phys, mem_map[i].phys + mem_map[i].size, i);
#endif
}

u64 get_page_table_size(void)
{
	return SZ_1M;
}

static int fdt_cmp_res(const void *v1, const void *v2)
{
	const struct fdt_resource *res1 = v1, *res2 = v2;

	/*
	 * Return a clamped -1/0/1, NOT (res1->start - res2->start): .start is a
	 * 64-bit phys_addr_t, and truncating the difference to int flips sign
	 * whenever two regions differ by more than INT_MAX (e.g. 0x4cd000 vs
	 * 0xfc200000 -- Tachyon has ~40 reserved nodes spanning >2 GB). A
	 * mis-sorted list makes carve_out_reserved_memory()'s merge compute
	 * (res[i].end - start) with end < start -> size_t underflow -> a multi-GB
	 * range -> the MMU walker runs off mapped DRAM and panics
	 * ("PTE ... should be a table").
	 */
	if (res1->start < res2->start)
		return -1;
	if (res1->start > res2->start)
		return 1;
	return 0;
}

/*
 * Bumped from 32: the Tachyon DTB carries more /reserved-memory no-map nodes
 * than the qcs404 baseline, and for the Windows handoff EVERY secure carveout
 * must be made non-cacheable (a missed one keeps a cacheable line that
 * winload's post-EBS writeback faults on the XPU -> silent hang).
 */
#define N_RESERVED_REGIONS 64

/*
 * Apply a protective attribute to every /reserved-memory no-map region.
 *
 * @noncacheable: false -> map them PTE_TYPE_FAULT (qcs404). Must run BEFORE the
 *                MMU/dcache is enabled; prevents the cache-prefetcher touching
 *                them and trapping to EL3.
 *                true  -> make them Normal NON-Cacheable (MT_NORMAL_NC, MAIR
 *                index 3) but VALID/ACCESSIBLE, via mmu_change_region_attr().
 *                Like the FAULT path, MUST run BEFORE dcache_enable(): the
 *                regions are then never cached, so nothing is ever dirty to
 *                flush (a runtime cache-clean of these ranges itself faults the
 *                XPU) and no cacheable line ever exists. Used on Tachyon
 *                (QCM6490/SC7280): winload cleans the dcache after
 *                ExitBootServices, and a writeback of any cacheable line over an
 *                XPU/TZ-protected carveout (PIL, SMEM, AOP, TZ/hyp) is rejected
 *                by the SC7280 XPU -> the AXI access never completes -> the AP
 *                spins (silent hang before SetVirtualAddressMap). Normal-NC (not
 *                device, not FAULT) so U-Boot's own unaligned SMEM/ACPI reads of
 *                these ranges still work. Mirrors the working Mu-Silicium/Kodiak
 *                firmware, which maps these regions uncached.
 */
static void carve_out_reserved_memory(bool noncacheable)
{
	static struct fdt_resource res[N_RESERVED_REGIONS] = { 0 };
	int parent, rmem, count, i = 0;
	phys_addr_t start;
	size_t size;

	/* Some reserved nodes must be carved out, as the cache-prefetcher may otherwise
	 * attempt to access them, causing a security exception.
	 */
	parent = fdt_path_offset(gd->fdt_blob, "/reserved-memory");
	if (parent <= 0) {
		log_err("No reserved memory regions found\n");
		return;
	}

	/* Collect the reserved memory regions */
	fdt_for_each_subnode(rmem, gd->fdt_blob, parent) {
		const fdt32_t *ptr;
		int len;
		if (!fdt_getprop(gd->fdt_blob, rmem, "no-map", NULL))
			continue;

		if (i == N_RESERVED_REGIONS) {
			log_err("Too many reserved regions!\n");
			break;
		}

		/* Read the address and size out from the reg property. Doing this "properly" with
		 * fdt_get_resource() takes ~70ms on SDM845, but open-coding the happy path here
		 * takes <1ms... Oh the woes of no dcache.
		 */
		ptr = fdt_getprop(gd->fdt_blob, rmem, "reg", &len);
		if (ptr) {
			/* Qualcomm devices use #address/size-cells = <2> but all reserved regions are within
			 * the 32-bit address space. So we can cheat here for speed.
			 */
			res[i].start = fdt32_to_cpu(ptr[1]);
			res[i].end = res[i].start + fdt32_to_cpu(ptr[3]);
			/*
			 * For the Tachyon Normal-NC path, only carve DRAM regions.
			 * Sub-DRAM no-map nodes (e.g. wlan_ce@4cd000) live in
			 * device/MMIO space; their 2M-aligned range would retype real
			 * MMIO to Normal-NC, and they aren't the cacheable secure-DRAM
			 * the Windows handoff cares about. qcs404 (FAULT) keeps all.
			 */
			if (!noncacheable || res[i].start >= gd->bd->bi_dram[0].start)
				i++;
		}
	}

	/* Sort the reserved memory regions by address */
	count = i;
	qsort(res, count, sizeof(struct fdt_resource), fdt_cmp_res);

	/* Now set the right attributes for them. Often a lot of the regions are tightly packed together
	 * so we can optimise the number of calls to mmu_change_region_attr() by combining adjacent
	 * regions.
	 */
	start = ALIGN_DOWN(res[0].start, SZ_2M);
	size = ALIGN(res[0].end - start, SZ_2M);
	for (i = 1; i <= count; i++) {
		/* We ideally want to 2M align everything for more efficient pagetables, but we must avoid
		 * overwriting reserved memory regions which shouldn't be mapped as FAULT (like those with
		 * compatible properties).
		 * If within 2M of the previous region, bump the size to include this region. Otherwise
		 * start a new region.
		 */
		if (i == count || start + size < res[i].start - SZ_2M) {
			debug("  0x%016llx - 0x%016llx: %s\n", start,
			      start + size,
			      noncacheable ? "non-cacheable" : "fault");
			if (noncacheable)
				/*
				 * Normal Non-Cacheable (MT_NORMAL_NC): keeps the
				 * block VALID + accessible (unlike FAULT) so
				 * U-Boot's own unaligned SMEM/ACPI reads still
				 * work, and execute-never. Use mmu_change_region_attr
				 * (NOT mmu_set_region_dcache_behaviour): the latter
				 * FLUSHES the region, and a cache clean of an
				 * XPU/TZ-protected region faults the XPU and resets
				 * the AP (confirmed by the scarveout test). This
				 * runs BEFORE dcache_enable(), so the regions were
				 * never cached -> nothing to write back -> no XPU
				 * fault, and they stay non-cacheable through the
				 * Windows handoff (no line for winload to clean).
				 */
				mmu_change_region_attr(start, size,
					PTE_TYPE_VALID |
					PTE_BLOCK_MEMTYPE(MT_NORMAL_NC) |
					PTE_BLOCK_PXN | PTE_BLOCK_UXN);
			else
				mmu_change_region_attr(start, size,
						       PTE_TYPE_FAULT);
			/* If this is the final region then quit here before we index
			 * out of bounds...
			 */
			if (i == count)
				break;
			start = ALIGN_DOWN(res[i].start, SZ_2M);
			size = ALIGN(res[i].end - start, SZ_2M);
		} else {
			/* Bump size if this region is immediately after the previous one */
			size = ALIGN(res[i].end - start, SZ_2M);
		}
	}
}

/* This function open-codes setup_all_pgtables() so that we can
 * insert additional mappings *before* turning on the MMU.
 */
void enable_caches(void)
{
	u64 tlb_addr = gd->arch.tlb_addr;
	u64 tlb_size = gd->arch.tlb_size;
	u64 pt_size;
	ulong carveout_start;

	gd->arch.tlb_fillptr = tlb_addr;

	build_mem_map();

	icache_enable();

	/* Create normal system page tables */
	setup_pgtables();

	pt_size = (uintptr_t)gd->arch.tlb_fillptr -
		  (uintptr_t)gd->arch.tlb_addr;
	debug("Primary pagetable size: %lluKiB\n", pt_size / 1024);

	/* Create emergency page tables */
	gd->arch.tlb_size -= pt_size;
	gd->arch.tlb_addr = gd->arch.tlb_fillptr;
	setup_pgtables();
	gd->arch.tlb_emerg = gd->arch.tlb_addr;
	gd->arch.tlb_addr = tlb_addr;
	gd->arch.tlb_size = tlb_size;

	/*
	 * qcs404: FAULT-map the no-map carveouts BEFORE the MMU/dcache come up
	 * (prevents the cache-prefetcher from touching them and trapping to EL3).
	 *
	 * NOTE: a Tachyon "map the secure carveouts Normal-NC here" path was tried
	 * to fix the Windows winload hand-off hang and is DISPROVEN/removed: it did
	 * NOT change the hang -- Windows still dies in the exact same place (after
	 * ExitBootServices completes, before SetVirtualAddressMap is entered), so
	 * secure-region cacheability is NOT the cause. (An apparent DP regression
	 * seen during that test turned out to be an unplugged monitor cable, not
	 * the carveout -- but the Windows result alone is reason enough to drop it.)
	 * Do NOT re-enable carve_out_reserved_memory(true) on Tachyon.
	 */
	if (fdt_node_check_compatible(gd->fdt_blob, 0, "qcom,qcs404") == 0) {
		carveout_start = get_timer(0);
		/* Takes ~20-50ms on SDM845 */
		carve_out_reserved_memory(false);
		debug("carveout time: %lums\n", get_timer(carveout_start));
	}
	dcache_enable();
}
