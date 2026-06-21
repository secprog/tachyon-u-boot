// SPDX-License-Identifier: GPL-2.0+
/*
 * ACPI Support for Particle Tachyon (QCM6490)
 * 
 * MADT (GICC/GICD/GICR/ITS) is generated via U-Boot's driver model:
 *   - armv8_cpu driver: fills GICC entries per CPU
 *   - arm_gic_v3 driver: fills GICD + GICR entries
 *   - arm_gic_v3_its driver: fills the ITS entry
 * 
 * Board-specific tables (FADT, IORT, PPTT, DSDT, CSRT, MCFG, TPM2)
 * remain as custom writers or fill hooks here.
 *
 * Based on the extracted ACPI tables from FAT12.bin
 *
 * Copyright (c) 2024-2025 Particle Industries, Inc.
 */

#include <cpu.h>
#include <env.h>
#include <log.h>
#include <tables_csum.h>
#include <string.h>
#include <acpi/acpi_table.h>
#include <bloblist.h>
#include <linux/err.h>
#include <asm/acpi_table.h>
#include <asm/armv8/sec_firmware.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <mapmem.h>
#include <smem.h>

#include "qcom_dram.h"

#define AML_NAME_OP		0x08
#define AML_BYTE_PREFIX		0x0a
#define AML_WORD_PREFIX		0x0b
#define AML_DWORD_PREFIX	0x0c
#define AML_STRING_PREFIX	0x0d
#define AML_QWORD_PREFIX	0x0e

#define QCOM_SMEM_MPSSEFS	"MPSS_EFS"
#define QCOM_SMEM_ADSPEFS	"ADSP_EFS"
#define QCOM_SMEM_TGCM		"TGCM"
#define QCOM_SMEM_SOCINFO	137

#define TACHYON_QCM6490_SOC_ID		497
#define TACHYON_QCM6490_FAMILY_ID	118
#define TACHYON_STORAGE_UFS		1
#define TACHYON_STORAGE_UFS_SUBTYPE	0
#define TACHYON_UFS3_ENABLED		1
#define TACHYON_UFS3_SUBTYPE		0
#define TACHYON_PLATFORM_SUBTYPE_IOT	1
#define TACHYON_SKU_VALUE		1
#define TACHYON_SDDR_REGION		4
#define TACHYON_UAON_DISABLED		0
#define TACHYON_QCM6490_JTAG_ID		0x001970e1
#define TACHYON_QCM6490_DEFAULT_SKU_ID	3

#define TACHYON_QFPROM_FEAT_CONFIG_ROW0	0x00784180
#define TACHYON_QFPROM_SKU_MASK		0x0ff00000
#define TACHYON_QFPROM_SKU_SHIFT	20
#define TACHYON_EMULATION_TYPE_REG	0x01fc8004
#define TACHYON_EMULATION_TYPE_MASK	0x3

#define QCOM_CSRT_DESC_100C_UID		0x00000001

static int tachyon_acpi_patch_name(struct acpi_table_header *dsdt,
				   const char name[ACPI_NAME_LEN],
				   const void *data, size_t data_len)
{
	u8 *end = (u8 *)dsdt + dsdt->length;
	u8 *ptr;

	for (ptr = (u8 *)dsdt + sizeof(*dsdt);
	     ptr + 6 + data_len <= end; ptr++) {
		u8 *obj;
		size_t obj_len;

		if (*ptr != AML_NAME_OP || memcmp(ptr + 1, name, ACPI_NAME_LEN))
			continue;

		obj = ptr + 1 + ACPI_NAME_LEN;

		switch (*obj) {
		case AML_BYTE_PREFIX:
			obj_len = 1;
			obj++;
			break;
		case AML_WORD_PREFIX:
			obj_len = 2;
			obj++;
			break;
		case AML_DWORD_PREFIX:
			obj_len = 4;
			obj++;
			break;
		case AML_QWORD_PREFIX:
			obj_len = 8;
			obj++;
			break;
		case AML_STRING_PREFIX:
			obj++;
			obj_len = strnlen((char *)obj, (size_t)(end - obj)) + 1;
			break;
		default:
			log_warning("DSDT Name(%4.4s) uses unsupported AML object 0x%02x\n",
				    name, *obj);
			return -EINVAL;
		}

		if (obj + obj_len > end)
			return -EINVAL;
		if (obj_len != data_len) {
			log_warning("DSDT Name(%4.4s) object is %lu bytes, expected %lu\n",
				    name, (ulong)obj_len, (ulong)data_len);
			return -EINVAL;
		}

		memcpy(obj, data, data_len);
		return 0;
	}

	log_warning("DSDT Name(%4.4s) not found\n", name);
	return -ENOENT;
}

static int tachyon_acpi_patch_u32(struct acpi_table_header *dsdt,
				  const char name[ACPI_NAME_LEN], u32 value)
{
	return tachyon_acpi_patch_name(dsdt, name, &value, sizeof(value));
}

static int tachyon_acpi_patch_u16(struct acpi_table_header *dsdt,
				  const char name[ACPI_NAME_LEN], u16 value)
{
	return tachyon_acpi_patch_name(dsdt, name, &value, sizeof(value));
}

static int tachyon_acpi_patch_u64(struct acpi_table_header *dsdt,
				  const char name[ACPI_NAME_LEN], u64 value)
{
	return tachyon_acpi_patch_name(dsdt, name, &value, sizeof(value));
}

static void tachyon_get_smem_region32(const char *name, u32 *base, u32 *size)
{
	qcom_mem_bank bank;
	int ret;

	*base = 0;
	*size = 0;

	ret = qcom_find_smem_region(name, &bank);
	if (ret) {
		log_warning("SMEM region %s not found for ACPI DSDT patching\n",
			    name);
		return;
	}

	if (bank.start > 0xffffffffULL || bank.size > 0xffffffffULL) {
		log_warning("SMEM region %s does not fit 32-bit ACPI fields\n",
			    name);
		return;
	}
	if (bank.size && bank.start > 0x100000000ULL - bank.size) {
		log_warning("SMEM region %s end does not fit 32-bit ACPI fields\n",
			    name);
		return;
	}

	*base = (u32)bank.start;
	*size = (u32)bank.size;
}

static u64 tachyon_get_smem_item_addr(unsigned int item)
{
	struct udevice *dev = NULL;
	size_t size;
	void *ptr;
	int ret;

	ret = uclass_first_device_err(UCLASS_SMEM, &dev);
	if (ret)
		return 0;

	ptr = smem_get(dev, -1, item, &size);
	if (IS_ERR_OR_NULL(ptr) || !size)
		return 0;

	return (u64)(ulong)ptr;
}

static u32 tachyon_read_sku_id(void)
{
	return (readl((void __iomem *)TACHYON_QFPROM_FEAT_CONFIG_ROW0) &
		TACHYON_QFPROM_SKU_MASK) >> TACHYON_QFPROM_SKU_SHIFT;
}

static u32 tachyon_read_emulation_type(void)
{
	return readl((void __iomem *)TACHYON_EMULATION_TYPE_REG) &
		TACHYON_EMULATION_TYPE_MASK;
}

int acpi_patch_dsdt(struct acpi_ctx *ctx, struct acpi_table_header *dsdt)
{
	u32 rmtb, rmtx, adsp_base, adsp_size, adsp_half, tcma, tcml;
	u32 sidv = 0, sidt, emul;
	u16 svmj, svmi;
	u64 sosi;
	int ret = 0;

	(void)ctx;

	tachyon_get_smem_region32(QCOM_SMEM_MPSSEFS, &rmtb, &rmtx);
	tachyon_get_smem_region32(QCOM_SMEM_ADSPEFS, &adsp_base, &adsp_size);
	tachyon_get_smem_region32(QCOM_SMEM_TGCM, &tcma, &tcml);

	adsp_half = adsp_size / 2;
	svmj = (sidv >> 16) & 0xffff;
	svmi = sidv & 0xffff;
	sidt = tachyon_read_sku_id();
	if (!sidt)
		sidt = TACHYON_QCM6490_DEFAULT_SKU_ID;
	emul = tachyon_read_emulation_type();
	sosi = tachyon_get_smem_item_addr(QCOM_SMEM_SOCINFO);

	ret |= tachyon_acpi_patch_u32(dsdt, "SOID", TACHYON_QCM6490_SOC_ID);
	ret |= tachyon_acpi_patch_u32(dsdt, "STOR", TACHYON_STORAGE_UFS);
	ret |= tachyon_acpi_patch_u32(dsdt, "SIDV", sidv);
	ret |= tachyon_acpi_patch_u16(dsdt, "SVMJ", svmj);
	ret |= tachyon_acpi_patch_u16(dsdt, "SVMI", svmi);
	ret |= tachyon_acpi_patch_u16(dsdt, "SDFE", TACHYON_QCM6490_FAMILY_ID);
	ret |= tachyon_acpi_patch_u64(dsdt, "SIDM", 0);
	ret |= tachyon_acpi_patch_u32(dsdt, "SUFS", TACHYON_STORAGE_UFS_SUBTYPE);
	ret |= tachyon_acpi_patch_u32(dsdt, "PUS3", TACHYON_UFS3_ENABLED);
	ret |= tachyon_acpi_patch_u32(dsdt, "SUS3", TACHYON_UFS3_SUBTYPE);
	ret |= tachyon_acpi_patch_u32(dsdt, "SIDT", sidt);
	ret |= tachyon_acpi_patch_u32(dsdt, "SJTG", TACHYON_QCM6490_JTAG_ID);
	ret |= tachyon_acpi_patch_u32(dsdt, "EMUL", emul);
	ret |= tachyon_acpi_patch_u32(dsdt, "PLST", TACHYON_PLATFORM_SUBTYPE_IOT);
	ret |= tachyon_acpi_patch_u32(dsdt, "SKUV", TACHYON_SKU_VALUE);
	ret |= tachyon_acpi_patch_u32(dsdt, "SDDR", TACHYON_SDDR_REGION);
	ret |= tachyon_acpi_patch_u32(dsdt, "UAON", TACHYON_UAON_DISABLED);
	ret |= tachyon_acpi_patch_u64(dsdt, "SOSI", sosi);

	ret |= tachyon_acpi_patch_u32(dsdt, "RMTB", rmtb);
	ret |= tachyon_acpi_patch_u32(dsdt, "RMTX", rmtx);
	ret |= tachyon_acpi_patch_u32(dsdt, "RFAB", adsp_base);
	ret |= tachyon_acpi_patch_u32(dsdt, "RFAS", adsp_half);
	ret |= tachyon_acpi_patch_u32(dsdt, "RFMB", adsp_base + adsp_half);
	ret |= tachyon_acpi_patch_u32(dsdt, "RFMS", adsp_half);
	ret |= tachyon_acpi_patch_u32(dsdt, "TCMA", tcma);
	ret |= tachyon_acpi_patch_u32(dsdt, "TCML", tcml);

	return ret;
}

/*
 * QCM6490 SoC ACPI Support for Windows ARM64 boot
 *
 * PSCI is provided by TF-A (Trusted Firmware-A) at EL3.
 * U-Boot runs at EL2 (non-secure) and does NOT implement its own PSCI.
 * The upstream DT (sc7280.dtsi) already declares:
 *   psci { compatible = "arm,psci-1.0"; method = "smc"; }
 * and all 8 CPU nodes have enable-method = "psci".
 *
 * U-Boot's only role is to:
 * 1. Declare PSCI compliance in FADT (arm_boot_arch field)
 * 2. Provide correct ACPI tables so Windows can discover CPUs and use SMP
 *
 * PSCI_RESET (default y on armv8) handles system reset via SMC to TF-A
 * independently of the ARMV8_PSCI framework.
 *
 * MADT sub-table generation uses the driver model:
 *   - CPU driver (armv8_cpu.c, compatible "qcom,kryo"): GICC entries
 *   - GICv3 driver (gic-v3-its.c, compatible "arm,gic-v3"): GICD + GICR
 *   - GICv3 ITS driver (compatible "arm,gic-v3-its"): ITS entry
 */

void acpi_fill_fadt(struct acpi_fadt *fadt)
{
	/*
	 * Set FADT fields for ARM64 ACPI:
	 *
	 * FADT Flags (offset 112):
	 * - ACPI_FADT_HW_REDUCED_ACPI (bit 20): Required for ARM (no legacy hardware)
	 * - ACPI_FADT_LOW_PWR_IDLE_S0 (bit 21): Support for S0 low power idle
	 */
	fadt->flags = ACPI_FADT_HW_REDUCED_ACPI | ACPI_FADT_LOW_PWR_IDLE_S0;

	/*
	 * ARM Boot Architecture Flags (offset 244):
	 * - ACPI_ARM_PSCI_COMPLIANT (bit 0): Tells Windows that PSCI is
	 *   available for CPU bring-up (SMP support)
	 *
	 * Note: The actual PSCI implementation runs in TF-A at EL3.
	 * U-Boot just needs to declare compliance here.
	 */
	fadt->arm_boot_arch = ACPI_ARM_PSCI_COMPLIANT;

	/*
	 * Set preferred power management profile for embedded/IoT device
	 * ACPI_PM_MOBILE = 2 (mobile/embedded device)
	 */
	fadt->preferred_pm_profile = ACPI_PM_MOBILE;

	/*
	 * Set FADT revision to 6 (ACPI 6.0+)
	 * This is required for ARM64 support
	 */
	fadt->header.revision = ACPI_FADT_REV_ACPI_6_0;
}

/*
 * MADT (Multiple APIC Description Table) body — GICv3
 *
 * We emit the GIC sub-tables from fixed constants taken from the known-good
 * Qualcomm Windows firmware MADT for this exact SoC
 * instead of via U-Boot's
 * driver model.  U-Boot has NO DM driver for "arm,gic-v3": the generic path
 * (arch/arm acpi_fill_madt -> armv8_cpu_fill_madt) cannot resolve the GIC as an
 * interrupt-parent, so it would emit zero GICC entries, and nothing at all
 * would emit the GICD that Windows requires to drive the distributor.  This
 * override (the arch acpi_fill_madt is __weak) reproduces the reference table
 * byte-for-byte in meaning:
 *
 *   QCM6490 / SC7280, GICv3 distributor @ 0x17a00000
 *     - 8 CPUs, MPIDR = cluster<<8 : 0x000, 0x100, ... 0x700
 *     - per-CPU redistributor frame at 0x17a60000 + n * 0x20000
 *     - PMU/perf interrupt  PPI 7 -> GSIV 23 (0x17)
 *     - vGIC maintenance irq PPI 9 -> GSIV 25 (0x19)
 *     - DynamIQ efficiency classes: 4x little (0), 3x mid (1), 1x big (2)
 */
#define TACHYON_GICD_BASE	0x17a00000UL
#define TACHYON_GICR_BASE	0x17a60000ULL
#define TACHYON_GICR_STRIDE	0x20000ULL
#define TACHYON_GIC_NUM_CPUS	8
#define TACHYON_GIC_PERF_GSIV	23
#define TACHYON_GIC_VGIC_IRQ	25
#define TACHYON_GIC_VERSION_V3	3

void *acpi_fill_madt(struct acpi_madt *madt, struct acpi_ctx *ctx)
{
	static const u8 efficiency[TACHYON_GIC_NUM_CPUS] = {
		0, 0, 0, 0, 1, 1, 1, 2
	};
	struct acpi_madt_gicc *gicc;
	struct acpi_madt_gicd *gicd;
	int i;

	(void)madt;

	/* One GICC (CPU interface) sub-table per core */
	for (i = 0; i < TACHYON_GIC_NUM_CPUS; i++) {
		gicc = ctx->current;
		acpi_write_madt_gicc(gicc,
				     i,			/* CPU interface number / UID */
				     TACHYON_GIC_PERF_GSIV,
				     0,			/* phys_base (GICv3: not used) */
				     0,			/* GICV base */
				     0,			/* GICH base */
				     TACHYON_GIC_VGIC_IRQ,
				     TACHYON_GICR_BASE +
					(u64)i * TACHYON_GICR_STRIDE,
				     (u64)i << 8,	/* MPIDR: aff1 = core */
				     efficiency[i]);
		acpi_inc(ctx, gicc->length);
	}

	/* Single GICD (distributor) sub-table */
	gicd = ctx->current;
	acpi_write_madt_gicd(gicd, 0, TACHYON_GICD_BASE, TACHYON_GIC_VERSION_V3);
	acpi_inc(ctx, gicd->length);

	return ctx->current;
}

/*
 * IORT (IO Remapping Table) support — REQUIRED for Qualcomm Windows boot
 *
 * Describes the SMMU (System MMU) and GIC ITS topology so Windows can
 * correctly configure DMA remapping and MSI interrupt translation.
 *
 * QCM6490 (SC7280) IORT topology:
 *   ITS Group (GIC ITS @ 0x17a40000, ID 0)
 *     └── SMMUv2 (ARM MMU-500 @ 0x15000000)
 *           ├── PCIe0 Root Complex (segment 0, SID base 0x1c00)
 *           ├── PCIe1 Root Complex (segment 1, SID base 0x1c80)
 *           ├── UFS0 Named Component (SID 0x80)
 *           └── USB1 Named Component (SID 0xe0)
 *
 * Without this table:
 * - Windows may fail to boot or use polled I/O
 * - DMA from PCIe/Storage may not work correctly
 * - MSI/MSI-X interrupts may not be delivered
 */
int acpi_fill_iort(struct acpi_ctx *ctx)
{
	/*
	 * IORT DROPPED for Windows bring-up. The table advertised (a) a GIC
	 * ITS group that exists in neither MADT nor silicon, and (b) the
	 * apps_smmu (0x15000000) as an OS-manageable MMU-500. Under the QHEE
	 * (Gunyah) hypervisor the SMMU globals are hyp-owned (the NS OS may
	 * touch stage-1 context banks only); a generic Windows arm-smmu
	 * driver writing SMMU_sCR0/stream-table is an XPU violation -> TZ
	 * PS_HOLD reset before any kernel output. With no IORT every master
	 * runs untranslated/1:1, which is the actual runtime state under the
	 * hypervisor's bypass. Emitting zero nodes makes the generic writer
	 * drop the table (it returns -ENOENT on an empty IORT).
	 * Re-add a correct IORT (no ITS, real SMMU model/ownership) later.
	 */
	return 0;
#if 0
	u32 its_offset, smmu_offset, ufs_offset, usb_offset;

	/*
	 * GIC ITS (Interrupt Translation Service) Group node
	 *
	 * Type: 0x00 (ITS Group)
	 * ITS ID: 0 — must match the gic_its_id field of the MADT GIC ITS entry
	 *         (type 0x0F) generated by the arm_gic_v3_its driver.
	 *
	 * Windows uses this to discover which ITS provides MSI translation
	 * for the SMMU's own control interrupts.
	 */
	u32 identifiers[] = { 0 };

	its_offset = acpi_iort_add_its_group(ctx, ARRAY_SIZE(identifiers),
					     identifiers);

	/*
	 * SMMUv2 (ARM MMU-500) node
	 *
	 * Type: 0x03 (SMMU)
	 * Base: 0x15000000 — apps_smmu from sc7280.dtsi
	 * Size: 0x100000 (1MB — same as DT reg size)
	 * Model: 0 (generic MMU-500)
	 *
	 * Interrupts (from sc7280.dtsi apps_smmu node):
	 * - Global interrupt: GIC_SPI 65 → ACPI GSIV 97 (SPI=65 + 32)
	 * - Context interrupts: pre-configured by hypervisor, not listed here
	 *   (SMMU is in bypass for all streams under Gunyah hyp)
	 *
	 * Flags: COHERENT_WALK — SMMU page table walks are cache-coherent
	 *        (matches dma-coherent property in DT)
	 *
	 * ID mapping: SMMU control interrupts → ITS Group (offset its_offset)
	 *             Input range 0-0xFFFF (all SIDs)
	 */
	u32 global_gsiv[4] = { 97, 0, 0, 0 };
	u32 global_flags[4] = { 0, 0, 0, 0 };  /* 0 = level-triggered */

	struct acpi_iort_id_mapping map_smmu[] = {{
		0,           /* input_base: lowest StreamID to match */
		0xffff,      /* id_count: number of StreamIDs */
		0,           /* output_base: lowest output ID */
		its_offset,  /* output_reference: offset to ITS Group node */
		0            /* flags: single-mapping = 0 (range mapping) */
	}};

	smmu_offset = acpi_iort_add_smmu(ctx,
		0x15000000,                 /* base_address (apps_smmu) */
		0x100000,                   /* span (1MB register space) */
		0,                          /* model (generic MMU-500) */
		ACPI_IORT_SMMU_COHERENT_WALK, /* flags (dma-coherent) */
		global_gsiv,                /* global interrupt GSIVs */
		global_flags,               /* global interrupt flags */
		0,                          /* num_ctx_irq (none — hyp owns) */
		NULL,                       /* ctx_irq array */
		0,                          /* num_pmu_irq (none) */
		NULL,                       /* pmu_irq array */
		ARRAY_SIZE(map_smmu),       /* num_mappings */
		map_smmu);                  /* ID mapping array */

	/*
	 * UFS Named Component node
	 *
	 * Type: 0x01 (Named Component)
	 * Device: \_SB.UFS0 — matches the DSDT UFS0 device HID "QCOM24A5"
	 *
	 * The UFS host controller is a named component (non-PCIe device)
	 * that has its own StreamID and must be explicitly mapped to the
	 * SMMU.  Without this entry the SMMU may not translate UFS DMA,
	 * causing storage access failures under Windows.
	 *
	 * Hardware details from sc7280.dtsi:
	 *   Base:      0x01d84000  (ufs_mem_hc)
	 *   StreamID:  0x80        (iommus = <&apps_smmu 0x80 0x0>)
	 *   Cache-coherent (dma-coherent)
	 *
	 * Flags: STALL_SUPPORTED — UFS supports stall-based SMMU fault
	 *        recovery.  PASID bits = 0 (no PASID support).
	 *
	 * ID mapping: UFS SID 0x80 → SMMU (offset smmu_offset)
	 *             Single mapping (not a range) — exact SID match
	 */
	u32 ufs_flags = ACPI_IORT_NC_STALL_SUPPORTED;

	struct acpi_iort_id_mapping map_ufs[] = {{
		0x80,                        /* input_base: UFS StreamID */
		1,                           /* id_count: single SID */
		0x80,                        /* output_base: same SID at SMMU */
		smmu_offset,                 /* output_reference: offset to SMMU */
		ACPI_IORT_ID_SINGLE_MAPPING  /* flags: exact match */
	}};

	ufs_offset = acpi_iort_add_named_component(ctx,
		ufs_flags,                  /* node_flags */
		BIT(0) | BIT(56),           /* memory_properties (CacheCoherent + CPM) */
		64,                         /* memory_address_limit (64-bit) */
		"\\_SB.UFS0",               /* device_name (ACPI path from DSDT) */
		ARRAY_SIZE(map_ufs),        /* num_mappings */
		map_ufs);                   /* ID mapping array */

	(void)ufs_offset;  /* reference kept for clarity; not chained to */

	/*
	 * USB1 Named Component node
	 *
	 * Type: 0x01 (Named Component)
	 * Device: \_SB.USB1 - matches the DSDT USB1 device HID "QCOM0AA1"
	 *
	 * The Windows installer commonly boots from this controller, so its
	 * non-PCI DMA StreamID must be visible in IORT for reliable setup I/O.
	 *
	 * Hardware details from sc7280.dtsi:
	 *   Base:      0x0a600000  (usb_1)
	 *   StreamID:  0xe0        (iommus = <&apps_smmu 0xe0 0x0>)
	 *   Cache-coherent (dma-coherent)
	 *
	 * ID mapping: USB1 SID 0xe0 -> SMMU (offset smmu_offset)
	 *             Single mapping (not a range) - exact SID match
	 */
	struct acpi_iort_id_mapping map_usb1[] = {{
		0xe0,                        /* input_base: USB1 StreamID */
		1,                           /* id_count: single SID */
		0xe0,                        /* output_base: same SID at SMMU */
		smmu_offset,                 /* output_reference: offset to SMMU */
		ACPI_IORT_ID_SINGLE_MAPPING  /* flags: exact match */
	}};

	usb_offset = acpi_iort_add_named_component(ctx,
		0,                          /* node_flags */
		BIT(0) | BIT(56),           /* memory_properties (CacheCoherent + CPM) */
		64,                         /* memory_address_limit (64-bit) */
		"\\_SB.USB1",               /* device_name (ACPI path from DSDT) */
		ARRAY_SIZE(map_usb1),       /* num_mappings */
		map_usb1);                  /* ID mapping array */

	(void)usb_offset;  /* reference kept for clarity; not chained to */

	/*
	 * PCIe Root Complex nodes
	 *
	 * Type: 0x02 (PCI Root Complex)
	 * Memory address size limit: 64 (64-bit addressing)
	 *
	 * Memory access properties:
	 *   bit 0  = Cache Coherent (PCIe is cache-coherent on this SoC)
	 *   bit 56 = CCA/CPM (Coherent Processing Memory — ARM-specific)
	 *
	 * ID mapping: PCIe Requester ID → SMMU (offset smmu_offset)
	 *             Input range 0-0xFFFF covers all 16-bit RIDs
	 *
	 * The iommu-map from sc7280.dtsi maps per-RC SID bases:
	 *   PCIe0: <0x0 &apps_smmu 0x1c00 0x1>  (SID base 0x1c00)
	 *   PCIe1: <0x0 &apps_smmu 0x1c80 0x1>  (SID base 0x1c80)
	 *
	 * Windows requires one IORT RC node per PCI segment group.
	 * Each RC's pci_segment_number must match the corresponding
	 * MCFG entry and DSDT _SEG value.
	 */

	/* PCIe0: segment 0, SID base 0x1c00 */
	{
		struct acpi_iort_id_mapping map_rc0[] = {{
			0,            /* input_base: lowest RequesterID */
			0xffff,       /* id_count: number of RequesterIDs */
			0x1c00,       /* output_base: SID base for PCIe0 */
			smmu_offset,  /* output_reference: offset to SMMU node */
			0             /* flags: range mapping */
		}};

		acpi_iort_add_rc(ctx,
			BIT(0) | BIT(56),       /* mem_access_properties */
			0,                      /* ats_attributes */
			0,                      /* pci_segment_number (PCIe0 = seg 0) */
			64,                     /* memory_address_size_limit */
			ARRAY_SIZE(map_rc0),
			map_rc0);
	}

	/* PCIe1: segment 1, SID base 0x1c80 */
	{
		struct acpi_iort_id_mapping map_rc1[] = {{
			0,            /* input_base */
			0xffff,       /* id_count */
			0x1c80,       /* output_base: SID base for PCIe1 */
			smmu_offset,  /* output_reference: offset to SMMU node */
			0             /* flags: range mapping */
		}};

		acpi_iort_add_rc(ctx,
			BIT(0) | BIT(56),       /* mem_access_properties */
			0,                      /* ats_attributes */
			1,                      /* pci_segment_number (PCIe1 = seg 1) */
			64,                     /* memory_address_size_limit */
			ARRAY_SIZE(map_rc1),
			map_rc1);
	}

	return 0;
#endif
}

/*
 * PPTT (Processor Properties Topology Table) support
 * Defines CPU/cache topology for Windows
 */
static int tachyon_write_pptt(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	u32 l1_offsets[8], l2_offsets[8];
	u32 l3_offset, l2_offset, cluster_offset;

	header = ctx->current;
	ctx->tab_start = ctx->current;

	memset(header, 0, sizeof(struct acpi_table_header));

	acpi_fill_header(header, "PPTT");
	header->revision = acpi_get_table_revision(ACPITAB_PPTT);
	acpi_inc(ctx, sizeof(*header));

	/*
	 * Define cache topology for QCM6490 (SC7280):
	 * - L1i and L1d per CPU (32KB each)
	 * - L2 per cluster (512KB per cluster)
	 * - L3 shared by all CPUs (4MB)
	 */

	/* L3 Cache - shared by all 8 CPUs */
	l3_offset = acpi_pptt_add_cache(ctx,
		ACPI_PPTT_ALL_VALID, 0,
		0x400000,  /* 4MB L3 cache */
		0x800,    /* Number of sets */
		16,       /* Associativity (16-way) */
		ACPI_PPTT_READ_ALLOC | ACPI_PPTT_WRITE_ALLOC |
		(ACPI_PPTT_CACHE_TYPE_UNIFIED << ACPI_PPTT_CACHE_TYPE_SHIFT),
		64);      /* Cache line size */

	/* L2 Cache - per cluster (512KB) */
	l2_offset = acpi_pptt_add_cache(ctx,
		ACPI_PPTT_ALL_VALID, 0,
		0x80000,  /* 512KB L2 per cluster */
		0x400,    /* Number of sets */
		8,        /* Associativity (8-way) */
		ACPI_PPTT_READ_ALLOC | ACPI_PPTT_WRITE_ALLOC |
		(ACPI_PPTT_CACHE_TYPE_UNIFIED << ACPI_PPTT_CACHE_TYPE_SHIFT),
		64);

	/* Cluster node (contains L2 cache) */
	cluster_offset = acpi_pptt_add_proc(ctx,
		ACPI_PPTT_PHYSICAL_PACKAGE |
		ACPI_PPTT_CHILDREN_IDENTICAL,
		0, 0, 1, &l2_offset);

	/* L1 Data Cache - 32KB per CPU */
	for (int i = 0; i < 8; i++) {
		l1_offsets[i] = acpi_pptt_add_cache(ctx,
			ACPI_PPTT_ALL_VALID, 0,
			0x8000,   /* 32KB L1d */
			0x100,    /* Number of sets */
			2,        /* Associativity (2-way) */
			ACPI_PPTT_READ_ALLOC | ACPI_PPTT_WRITE_ALLOC |
			(ACPI_PPTT_CACHE_TYPE_DATA << ACPI_PPTT_CACHE_TYPE_SHIFT),
			64);
	}

	/* L1 Instruction Cache - 32KB per CPU */
	for (int i = 0; i < 8; i++) {
		l2_offsets[i] = acpi_pptt_add_cache(ctx,
			ACPI_PPTT_ALL_BUT_WRITE_POL, 0,
			0x8000,   /* 32KB L1i */
			0x100,    /* Number of sets */
			2,        /* Associativity (2-way) */
			ACPI_PPTT_READ_ALLOC |
			(ACPI_PPTT_CACHE_TYPE_INSTR << ACPI_PPTT_CACHE_TYPE_SHIFT),
			64);
	}

	/* CPU nodes - leaf nodes with L1 caches */
	for (int i = 0; i < 8; i++) {
		u32 caches[2] = { l1_offsets[i], l2_offsets[i] };
		acpi_pptt_add_proc(ctx,
			ACPI_PPTT_CHILDREN_IDENTICAL |
			ACPI_PPTT_NODE_IS_LEAF |
			ACPI_PPTT_PROC_ID_VALID,
			cluster_offset, i, 2, caches);
	}

	header->length = ctx->current - ctx->tab_start;
	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);

	return 0;
}

ACPI_WRITER(5pptt, "PPTT", tachyon_write_pptt, 0);

/*
 * GTDT (Generic Timer Description Table) support
 * ARM Generic Timer interrupts for Windows
 */

/* ACPI GTDT "GT Block" platform timer (one memory-mapped timer + one frame). */
struct gtdt_gt_block {
	u8 type;		/* 0 = GT Block */
	u16 length;		/* 0x3C */
	u8 reserved;
	u64 cntctlbase;		/* GT Block CntCTLBase */
	u32 timer_count;	/* number of GT Block timer frames */
	u32 timer_offset;	/* offset from block start to frame 0 */
	/* GT Block Timer frame 0 */
	u8 frame_number;
	u8 reserved2[3];
	u64 cntbase;		/* CntBaseN */
	u64 cntel0base;		/* CntEL0BaseN */
	u32 phys_gsiv;
	u32 phys_flags;
	u32 virt_gsiv;
	u32 virt_flags;
	u32 common_flags;
} __packed;

static int tachyon_write_gtdt(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct acpi_gtdt *gtdt;

	gtdt = ctx->current;
	header = &gtdt->header;

	memset(gtdt, 0, sizeof(struct acpi_gtdt));

	acpi_fill_header(header, "GTDT");
	header->length = sizeof(struct acpi_gtdt);
	header->revision = acpi_get_table_revision(ACPITAB_GTDT);

	/*
	 * ARM Generic Timer interrupts. ACPI GSIVs map 1:1 to GIC INTIDs.
	 *   GSIV 29 secure EL1, 30 non-secure EL1, 27 virtual EL1, 26 EL2.
	 *
	 * Flags: bit0 clear = level-triggered, bit1 = polarity (set = active-low).
	 * The SC7280 *Linux* DT marks these level-low (0x2), but the proven
	 * Windows-on-ARM Kodiak GTDT ships flags = 0x0 (level, active-high) for
	 * all four GSIVs. Windows is the consumer of this ACPI table, so match the
	 * WoA value -- a wrong timer polarity mis-delivers the very timer IRQ the
	 * NT kernel takes during early GIC/timer bring-up.
	 */
	gtdt->cnt_ctrl_base = 0xFFFFFFFFFFFFFFFF; /* Not used */
	gtdt->sec_el1_gsiv = 29;  /* Secure EL1 timer (0x1D) */
	gtdt->sec_el1_flags = 0;
	gtdt->el1_gsiv = 30;      /* Non-secure EL1 timer (0x1E) */
	gtdt->el1_flags = 0;
	gtdt->virt_el1_gsiv = 27; /* Virtual EL1 timer (0x1B) */
	gtdt->virt_el1_flags = 0;
	gtdt->el2_gsiv = 26;      /* Non-secure EL2 timer (0x1A) */
	gtdt->el2_flags = 0;
	gtdt->cnt_read_base = 0xFFFFFFFFFFFFFFFF;

	/*
	 * Append one "GT Block" (memory-mapped generic timer) to match the proven
	 * Kodiak (SC7280) Windows GTDT, which we otherwise omit.  Block @
	 * 0x17C20000, one frame @ 0x17C21000 (EL0 @ 0x17C22000), physical-timer
	 * GSIV 0x28, virtual-timer GSIV 0x26, AlwaysOn.  Windows' Qualcomm HAL
	 * consults this during early timer bring-up -- exactly where the NT kernel
	 * currently faults -- rather than relying solely on the CP15 sysreg timer.
	 */
	gtdt->plat_timer_count = 1;
	gtdt->plat_timer_offset = sizeof(struct acpi_gtdt);
	{
		struct gtdt_gt_block *gb =
			(void *)((u8 *)gtdt + sizeof(struct acpi_gtdt));

		memset(gb, 0, sizeof(*gb));
		gb->type = 0;			/* GT Block */
		gb->length = sizeof(*gb);	/* 0x3C */
		gb->cntctlbase = 0x17C20000;
		gb->timer_count = 1;
		gb->timer_offset = 0x14;	/* to frame 0 */
		gb->frame_number = 0;
		gb->cntbase = 0x17C21000;
		gb->cntel0base = 0x17C22000;
		gb->phys_gsiv = 0x28;
		gb->virt_gsiv = 0x26;
		gb->common_flags = 0x02;	/* AlwaysOn */
	}
	header->length = sizeof(struct acpi_gtdt) + sizeof(struct gtdt_gt_block);

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, gtdt);
	acpi_inc(ctx, header->length);

	return 0;
}

ACPI_WRITER(5gtdt, "GTDT", tachyon_write_gtdt, 0);

/*
 * MCFG (PCI Express MMIO Config Space) support
 * ECAM for PCIe controllers
 *
 * The generic ACPI code already registers an MCFG writer
 * (lib/acpi/mcfg.c: ACPI_WRITER(5mcfg, "MCFG", acpi_write_mcfg, 0)).
 * That writer calls the weak acpi_fill_mcfg() to append MMConfig
 * entries.  We implement that hook here so we don't create a
 * duplicate linker-list entry with the same name.
 */
int acpi_fill_mcfg(struct acpi_ctx *ctx)
{
	/*
	 * MCFG DROPPED for Windows bring-up. U-Boot never powers/clocks the
	 * PCIe controllers (CONFIG_PCI unset, GCC_PCIE GDSCs stay collapsed),
	 * so an ECAM/config-space read by Windows' early PCI enumeration
	 * stalls the NoC -> TrustZone pulls PS_HOLD -> silent hard reset
	 * before any kernel output. Returning -ENOENT makes the generic
	 * writer cleanly skip the table (it must be -ENOENT specifically;
	 * any other errno aborts all ACPI generation). The DSDT also forces
	 * PCI0/PCI1 _STA=0 (PRP0/PRP1=Zero) so the bridges are never probed.
	 * Re-add ECAM once a PCIe controller+clock driver brings the RCs up.
	 */
	return -ENOENT;
}

/*
 * CSRT (Core System Resource Table) support
 *
 * The generic CSRT writer in lib/acpi/csrt.c owns the ACPI table header and
 * calls this board hook to append resource groups. Qualcomm's Windows BSP
 * expects two vendor-defined groups matching the QCOMEDK2 CSRT. Keep the
 * private 0x100c descriptor as sparse 32-bit fields instead of embedding a
 * 69 KiB opaque table image.
 */
#define QCOM_CSRT_VENDOR_ID		((u32)'Q' | ((u32)'C' << 8) | \
					 ((u32)'O' << 16) | ((u32)'M' << 24))
#define QCOM_CSRT_GROUP_100B_LENGTH	56
#define QCOM_CSRT_GROUP_100C_LENGTH	69166
#define QCOM_CSRT_DESC_100B_LENGTH	32
#define QCOM_CSRT_DESC_100C_LENGTH	69142
#define QCOM_CSRT_DESC_100C_PAYLOAD	(QCOM_CSRT_DESC_100C_LENGTH - \
					 sizeof(struct acpi_csrt_descriptor))

struct qcom_csrt_word {
	u16 offset;
	u32 value;
};

static const u32 qcom_csrt_100b_payload[] = {
	0x00000002, 0x17c10000, 0x00000000, 0x00007ffd, 0x00000020,
};

static const struct qcom_csrt_word qcom_csrt_100c_words[] = {
	{ 0x0000, 0x00000001 }, { 0x0004, 0x00000002 }, { 0x0008, 0x00000040 },
	{ 0x000c, 0x00000021 }, { 0x0010, 0x00001618 }, { 0x0014, 0x00004e20 },
	{ 0x0018, 0x000017e6 }, { 0x001c, 0x0000001e }, { 0x0020, 0x00006606 },
	{ 0x0024, 0x00000001 }, { 0x0028, 0x000067e6 }, { 0x002c, 0x00004e20 },
	{ 0x0030, 0x0000bff6 }, { 0x0034, 0x0000144c }, { 0x0038, 0x00000001 },
	{ 0x003c, 0x00000001 }, { 0x0040, 0x15000000 }, { 0x0058, 0x0017c02c },
	{ 0x0060, 0x00000001 }, { 0x0064, 0x80000000 }, { 0x007c, 0x00000010 },
	{ 0x0084, 0x000001fe }, { 0x0088, 0x0000000a }, { 0x008c, 0x000000e0 },
	{ 0x0090, 0x00000060 }, { 0x0094, 0x00010400 }, { 0x0098, 0x03000000 },
	{ 0x009c, 0x04010000 }, { 0x00a0, 0x00000001 }, { 0x00a4, 0x00000300 },
	{ 0x00a8, 0x00020402 }, { 0x00ac, 0x03000000 }, { 0x00b0, 0x08000000 },
	{ 0x00b4, 0x001c0001 }, { 0x00b8, 0x00000203 }, { 0x00bc, 0x00010801 },
	{ 0x00c0, 0x010d0000 }, { 0x00c4, 0x08200000 }, { 0x00c8, 0x001c0001 },
	{ 0x00cc, 0x00000203 }, { 0x00d0, 0x00010821 }, { 0x00d4, 0x010d0000 },
	{ 0x00d8, 0x08400000 }, { 0x00dc, 0x001c0001 }, { 0x00e0, 0x00000203 },
	{ 0x00e4, 0x00010841 }, { 0x00e8, 0x010d0000 }, { 0x00ec, 0x08600000 },
	{ 0x00f0, 0x001c0001 }, { 0x00f4, 0x00000203 }, { 0x00f8, 0x00010861 },
	{ 0x00fc, 0x010d0000 }, { 0x0100, 0x08800000 }, { 0x0104, 0x001c0001 },
	{ 0x0108, 0x00000203 }, { 0x010c, 0x00010881 }, { 0x0110, 0x010d0000 },
	{ 0x0114, 0x08a00000 }, { 0x0118, 0x001c0001 }, { 0x011c, 0x00000203 },
	{ 0x0120, 0x000108a1 }, { 0x0124, 0x010d0000 }, { 0x0128, 0x08c00000 },
	{ 0x012c, 0x001c0001 }, { 0x0130, 0x00000203 }, { 0x0134, 0x000108c1 },
	{ 0x0138, 0x010d0000 }, { 0x013c, 0x08e00000 }, { 0x0140, 0x001c0001 },
	{ 0x0144, 0x00000203 }, { 0x0148, 0x000108e1 }, { 0x014c, 0x010d0000 },
	{ 0x0150, 0x0c000000 }, { 0x0154, 0x001c0001 }, { 0x0158, 0x00000203 },
	{ 0x015c, 0x00010c01 }, { 0x0160, 0x010d0000 }, { 0x0164, 0x0c200000 },
	{ 0x0168, 0x001c0001 }, { 0x016c, 0x00000203 }, { 0x0170, 0x00010c21 },
	{ 0x0174, 0x010d0000 }, { 0x0178, 0x0c400000 }, { 0x017c, 0x001c0001 },
	{ 0x0180, 0x00000203 }, { 0x0184, 0x00010c41 }, { 0x0188, 0x010d0000 },
	{ 0x018c, 0x0c600000 }, { 0x0190, 0x001c0001 }, { 0x0194, 0x00000203 },
	{ 0x0198, 0x00010c61 }, { 0x019c, 0x010d0000 }, { 0x01a0, 0x0c800000 },
	{ 0x01a4, 0x001c0001 }, { 0x01a8, 0x00000203 }, { 0x01ac, 0x00010c81 },
	{ 0x01b0, 0x010d0000 }, { 0x01b4, 0x0ca00000 }, { 0x01b8, 0x001c0001 },
	{ 0x01bc, 0x00000203 }, { 0x01c0, 0x00010ca1 }, { 0x01c4, 0x010d0000 },
	{ 0x01c8, 0x0cc00000 }, { 0x01cc, 0x001c0001 }, { 0x01d0, 0x00000203 },
	{ 0x01d4, 0x00010cc1 }, { 0x01d8, 0x010d0000 }, { 0x01dc, 0x0ce00000 },
	{ 0x01e0, 0x001c0001 }, { 0x01e4, 0x00000203 }, { 0x01e8, 0x00010ce1 },
	{ 0x01ec, 0x010d0000 }, { 0x01f0, 0x20000000 }, { 0x01f4, 0x001c0001 },
	{ 0x01f8, 0x00000203 }, { 0x01fc, 0x00012001 }, { 0x0200, 0x010d0000 },
	{ 0x0204, 0x20200000 }, { 0x0208, 0x001c0001 }, { 0x020c, 0x00000203 },
	{ 0x0210, 0x00012021 }, { 0x0214, 0x010d0000 }, { 0x0218, 0x20400000 },
	{ 0x021c, 0x001c0001 }, { 0x0220, 0x00000203 }, { 0x0224, 0x00012041 },
	{ 0x0228, 0x010d0000 }, { 0x022c, 0x20620000 }, { 0x0230, 0x001c0001 },
	{ 0x0234, 0x00000203 }, { 0x0238, 0x00012080 }, { 0x023c, 0x0203001c },
	{ 0x0240, 0x20810000 }, { 0x0244, 0x00000001 }, { 0x0248, 0x0000010d },
	{ 0x024c, 0x000120a0 }, { 0x0250, 0x0203001c }, { 0x0254, 0x20a10000 },
	{ 0x0258, 0x00000001 }, { 0x025c, 0x0000010d }, { 0x0260, 0x000120c0 },
	{ 0x0264, 0x0203001c }, { 0x0268, 0x20e00000 }, { 0x026c, 0x001c0001 },
	{ 0x0270, 0x00000203 }, { 0x0274, 0x00012100 }, { 0x0278, 0x0203001c },
	{ 0x027c, 0x21010000 }, { 0x0280, 0x00000001 }, { 0x0284, 0x0000010d },
	{ 0x0288, 0x00012120 }, { 0x028c, 0x0203001c }, { 0x0290, 0x21210000 },
	{ 0x0294, 0x00000001 }, { 0x0298, 0x0000010d }, { 0x029c, 0x00012140 },
	{ 0x02a0, 0x0203001c }, { 0x02a4, 0x21410000 }, { 0x02a8, 0x00000001 },
	{ 0x02ac, 0x0000010d }, { 0x02b0, 0x00010440 }, { 0x02b4, 0x03000000 },
	{ 0x02b8, 0x04200000 }, { 0x02bc, 0x00000001 }, { 0x02c0, 0x00000300 },
	{ 0x02c4, 0x000204e4 }, { 0x02c8, 0x02030000 }, { 0x02cc, 0x04e60000 },
	{ 0x02d0, 0x00000002 }, { 0x02d4, 0x00000203 }, { 0x02d8, 0x000204ea },
	{ 0x02dc, 0x01160000 }, { 0x02e0, 0x04f20000 }, { 0x02e4, 0x00000001 },
	{ 0x02e8, 0x00000203 }, { 0x02ec, 0x000104f3 }, { 0x02f0, 0x02090000 },
	{ 0x02f4, 0x04f40000 }, { 0x02f8, 0x00000002 }, { 0x02fc, 0x00000203 },
	{ 0x0300, 0x000204f6 }, { 0x0304, 0x02030000 }, { 0x0308, 0x04f80000 },
	{ 0x030c, 0x00000002 }, { 0x0310, 0x00000203 }, { 0x0314, 0x000204fa },
	{ 0x0318, 0x01160000 }, { 0x031c, 0x04fc0000 }, { 0x0320, 0x00000001 },
	{ 0x0324, 0x00000209 }, { 0x0328, 0x000104fd }, { 0x032c, 0x02090000 },
	{ 0x0330, 0x04fe0000 }, { 0x0334, 0x00000001 }, { 0x0338, 0x00000209 },
	{ 0x033c, 0x000104ff }, { 0x0340, 0x02030000 }, { 0x0344, 0x09000000 },
	{ 0x0348, 0x001e0001 }, { 0x034c, 0x00000203 }, { 0x0350, 0x00010901 },
	{ 0x0354, 0x020a001c }, { 0x0358, 0x09020000 }, { 0x035c, 0x001e0001 },
	{ 0x0360, 0x00000203 }, { 0x0364, 0x00010903 }, { 0x0368, 0x01110000 },
	{ 0x036c, 0x0d000000 }, { 0x0370, 0x001e0001 }, { 0x0374, 0x00000203 },
	{ 0x0378, 0x00010d01 }, { 0x037c, 0x020a001c }, { 0x0380, 0x0d020000 },
	{ 0x0384, 0x001e0001 }, { 0x0388, 0x00000203 }, { 0x038c, 0x00010d03 },
	{ 0x0390, 0x01110000 }, { 0x0398, 0x00000001 }, { 0x039c, 0x00000203 },
	{ 0x03a0, 0x00010001 }, { 0x03a4, 0x02030000 }, { 0x03a8, 0x04800000 },
	{ 0x03ac, 0x00000001 }, { 0x03b0, 0x00000203 }, { 0x03b4, 0x00010481 },
	{ 0x03b8, 0x02030000 }, { 0x03bc, 0x04820000 }, { 0x03c0, 0x00000001 },
	{ 0x03c4, 0x00000203 }, { 0x03c8, 0x00010483 }, { 0x03cc, 0x02030000 },
	{ 0x03d0, 0x04840000 }, { 0x03d4, 0x00000001 }, { 0x03d8, 0x00000203 },
	{ 0x03dc, 0x00080488 }, { 0x03e0, 0x03000000 }, { 0x03e4, 0x18200000 },
	{ 0x03e8, 0x00000001 }, { 0x03ec, 0x00000116 }, { 0x03f0, 0x00011821 },
	{ 0x03f4, 0x01160000 }, { 0x03f8, 0x18220000 }, { 0x03fc, 0x00000001 },
	{ 0x0400, 0x00000116 }, { 0x0404, 0x00011825 }, { 0x0408, 0x01160000 },
	{ 0x040c, 0x18260000 }, { 0x0410, 0x00000001 }, { 0x0414, 0x00000103 },
	{ 0x0418, 0x00011827 }, { 0x041c, 0x01160000 }, { 0x0420, 0x18280000 },
	{ 0x0424, 0x00000001 }, { 0x0428, 0x00000116 }, { 0x042c, 0x00011829 },
	{ 0x0430, 0x01160000 }, { 0x0434, 0x182a0000 }, { 0x0438, 0x00000001 },
	{ 0x043c, 0x00000116 }, { 0x0440, 0x0001182b }, { 0x0444, 0x01160000 },
	{ 0x0448, 0x182c0000 }, { 0x044c, 0x00000001 }, { 0x0450, 0x00000116 },
	{ 0x0454, 0x00011832 }, { 0x0458, 0x01160000 }, { 0x045c, 0x18330000 },
	{ 0x0460, 0x00000001 }, { 0x0464, 0x00000116 }, { 0x0468, 0x00011834 },
	{ 0x046c, 0x01160000 }, { 0x0470, 0x18000000 }, { 0x0474, 0x00000001 },
	{ 0x0478, 0x00000106 }, { 0x047c, 0x00011801 }, { 0x0480, 0x02030000 },
	{ 0x0484, 0x18020000 }, { 0x0488, 0x00000001 }, { 0x048c, 0x00000130 },
	{ 0x0490, 0x00011803 }, { 0x0494, 0x02030000 }, { 0x0498, 0x18040000 },
	{ 0x049c, 0x00000001 }, { 0x04a0, 0x00000203 }, { 0x04a4, 0x00011805 },
	{ 0x04a8, 0x02030000 }, { 0x04ac, 0x18060000 }, { 0x04b0, 0x00000001 },
	{ 0x04b4, 0x00000203 }, { 0x04b8, 0x00011807 }, { 0x04bc, 0x02030000 },
	{ 0x04c0, 0x180f0000 }, { 0x04c4, 0x00000001 }, { 0x04c8, 0x00000203 },
	{ 0x04cc, 0x00011860 }, { 0x04d0, 0x01160000 }, { 0x04d4, 0x18610000 },
	{ 0x04d8, 0x00000001 }, { 0x04dc, 0x00000116 }, { 0x04e0, 0x00011862 },
	{ 0x04e4, 0x01160000 }, { 0x04e8, 0x18630000 }, { 0x04ec, 0x00000001 },
	{ 0x04f0, 0x00000116 }, { 0x04f4, 0x00011864 }, { 0x04f8, 0x01160000 },
	{ 0x04fc, 0x18680000 }, { 0x0500, 0x00000001 }, { 0x0504, 0x00000116 },
	{ 0x0508, 0x00011869 }, { 0x050c, 0x01160000 }, { 0x0510, 0x18400000 },
	{ 0x0514, 0x00000001 }, { 0x0518, 0x00000116 }, { 0x051c, 0x00011848 },
	{ 0x0520, 0x01160000 }, { 0x0524, 0x18490000 }, { 0x0528, 0x00000001 },
	{ 0x052c, 0x00000116 }, { 0x0530, 0x0001184a }, { 0x0534, 0x01160000 },
	{ 0x0538, 0x184b0000 }, { 0x053c, 0x00000001 }, { 0x0540, 0x00000116 },
	{ 0x0544, 0x0001184c }, { 0x0548, 0x01160000 }, { 0x054c, 0x184d0000 },
	{ 0x0550, 0x00000001 }, { 0x0554, 0x00000116 }, { 0x0558, 0x00011181 },
	{ 0x055c, 0x0203003c }, { 0x0560, 0x11820000 }, { 0x0564, 0x003c0001 },
	{ 0x0568, 0x00000203 }, { 0x056c, 0x00011183 }, { 0x0570, 0x0203003c },
	{ 0x0574, 0x11840000 }, { 0x0578, 0x003c0001 }, { 0x057c, 0x00000203 },
	{ 0x0580, 0x00011185 }, { 0x0584, 0x0203003c }, { 0x0588, 0x11860000 },
	{ 0x058c, 0x003c0001 }, { 0x0590, 0x00000203 }, { 0x0594, 0x00011187 },
	{ 0x0598, 0x0203003c }, { 0x059c, 0x11880000 }, { 0x05a0, 0x003c0001 },
	{ 0x05a4, 0x00000203 }, { 0x05a8, 0x00011189 }, { 0x05ac, 0x020a003c },
	{ 0x05b0, 0x118b0000 }, { 0x05b4, 0x003c0001 }, { 0x05b8, 0x00000203 },
	{ 0x05bc, 0x0001118c }, { 0x05c0, 0x0203003c }, { 0x05c4, 0x118d0000 },
	{ 0x05c8, 0x003c0001 }, { 0x05cc, 0x00000203 }, { 0x05d0, 0x0001118e },
	{ 0x05d4, 0x0203003c }, { 0x05d8, 0x118f0000 }, { 0x05dc, 0x003c0001 },
	{ 0x05e0, 0x00000203 }, { 0x05e4, 0x000111a0 }, { 0x05e8, 0x011e0000 },
	{ 0x05ec, 0x11a10000 }, { 0x05f0, 0x003c0001 }, { 0x05f4, 0x00000203 },
	{ 0x05f8, 0x000111a2 }, { 0x05fc, 0x0203003c }, { 0x0600, 0x11a30000 },
	{ 0x0604, 0x003c0001 }, { 0x0608, 0x00000203 }, { 0x060c, 0x000111a4 },
	{ 0x0610, 0x0203003c }, { 0x0614, 0x11a50000 }, { 0x0618, 0x003c0001 },
	{ 0x061c, 0x00000203 }, { 0x0620, 0x000111a6 }, { 0x0624, 0x0203003c },
	{ 0x0628, 0x11a70000 }, { 0x062c, 0x003c0001 }, { 0x0630, 0x00000203 },
	{ 0x0634, 0x000111a8 }, { 0x0638, 0x0203003c }, { 0x063c, 0x11a90000 },
	{ 0x0640, 0x003c0001 }, { 0x0644, 0x0000020a }, { 0x0648, 0x000111aa },
	{ 0x064c, 0x012a0000 }, { 0x0650, 0x11ab0000 }, { 0x0654, 0x003c0001 },
	{ 0x0658, 0x00000203 }, { 0x065c, 0x000111ac }, { 0x0660, 0x0203003c },
	{ 0x0664, 0x11ad0000 }, { 0x0668, 0x003c0001 }, { 0x066c, 0x00000203 },
	{ 0x0670, 0x000111ae }, { 0x0674, 0x0203003c }, { 0x0678, 0x11af0000 },
	{ 0x067c, 0x003c0001 }, { 0x0680, 0x00000203 }, { 0x0684, 0x00011581 },
	{ 0x0688, 0x0203003c }, { 0x068c, 0x15820000 }, { 0x0690, 0x003c0001 },
	{ 0x0694, 0x00000203 }, { 0x0698, 0x00011583 }, { 0x069c, 0x0203003c },
	{ 0x06a0, 0x15840000 }, { 0x06a4, 0x003c0001 }, { 0x06a8, 0x00000203 },
	{ 0x06ac, 0x00011585 }, { 0x06b0, 0x0203003c }, { 0x06b4, 0x15860000 },
	{ 0x06b8, 0x003c0001 }, { 0x06bc, 0x00000203 }, { 0x06c0, 0x00011587 },
	{ 0x06c4, 0x0203003c }, { 0x06c8, 0x15880000 }, { 0x06cc, 0x003c0001 },
	{ 0x06d0, 0x00000203 }, { 0x06d4, 0x00011589 }, { 0x06d8, 0x020a003c },
	{ 0x06dc, 0x158b0000 }, { 0x06e0, 0x003c0001 }, { 0x06e4, 0x00000203 },
	{ 0x06e8, 0x0001158c }, { 0x06ec, 0x0203003c }, { 0x06f0, 0x158d0000 },
	{ 0x06f4, 0x003c0001 }, { 0x06f8, 0x00000203 }, { 0x06fc, 0x0001158e },
	{ 0x0700, 0x0203003c }, { 0x0704, 0x158f0000 }, { 0x0708, 0x003c0001 },
	{ 0x070c, 0x00000203 }, { 0x0710, 0x000115a0 }, { 0x0714, 0x011e0000 },
	{ 0x0718, 0x15a10000 }, { 0x071c, 0x003c0001 }, { 0x0720, 0x00000203 },
	{ 0x0724, 0x000115a2 }, { 0x0728, 0x0203003c }, { 0x072c, 0x15a30000 },
	{ 0x0730, 0x003c0001 }, { 0x0734, 0x00000203 }, { 0x0738, 0x000115a4 },
	{ 0x073c, 0x0203003c }, { 0x0740, 0x15a50000 }, { 0x0744, 0x003c0001 },
	{ 0x0748, 0x00000203 }, { 0x074c, 0x000115a6 }, { 0x0750, 0x0203003c },
	{ 0x0754, 0x15a70000 }, { 0x0758, 0x003c0001 }, { 0x075c, 0x00000203 },
	{ 0x0760, 0x000115a8 }, { 0x0764, 0x0203003c }, { 0x0768, 0x15a90000 },
	{ 0x076c, 0x003c0001 }, { 0x0770, 0x0000020a }, { 0x0774, 0x000115aa },
	{ 0x0778, 0x012a0000 }, { 0x077c, 0x15ab0000 }, { 0x0780, 0x003c0001 },
	{ 0x0784, 0x00000203 }, { 0x0788, 0x000115ac }, { 0x078c, 0x0203003c },
	{ 0x0790, 0x15ad0000 }, { 0x0794, 0x003c0001 }, { 0x0798, 0x00000203 },
	{ 0x079c, 0x000115ae }, { 0x07a0, 0x0203003c }, { 0x07a4, 0x15af0000 },
	{ 0x07a8, 0x003c0001 }, { 0x07ac, 0x00000203 }, { 0x07b0, 0x000104a0 },
	{ 0x07b4, 0x01030000 }, { 0x07b8, 0x04c00000 }, { 0x07bc, 0x00000001 },
	{ 0x07c0, 0x00000103 }, { 0x07c4, 0x00010123 }, { 0x07c8, 0x01030000 },
	{ 0x07cc, 0x01240000 }, { 0x07d0, 0x00000001 }, { 0x07d4, 0x00000300 },
	{ 0x07d8, 0x00010125 }, { 0x07dc, 0x01160000 }, { 0x07e0, 0x01360000 },
	{ 0x07e4, 0x00000001 }, { 0x07e8, 0x00000103 }, { 0x07ec, 0x00010043 },
	{ 0x07f0, 0x01030000 }, { 0x07f4, 0x00440000 }, { 0x07f8, 0x00000001 },
	{ 0x07fc, 0x00000300 }, { 0x0800, 0x00010045 }, { 0x0804, 0x01160000 },
	{ 0x0808, 0x00560000 }, { 0x080c, 0x00000001 }, { 0x0810, 0x00000103 },
	{ 0x0814, 0x000100c0 }, { 0x0818, 0x01030000 }, { 0x081c, 0x01000000 },
	{ 0x0820, 0x00000001 }, { 0x0824, 0x00000103 }, { 0x0828, 0x00010060 },
	{ 0x082c, 0x01030000 }, { 0x0830, 0x04600000 }, { 0x0834, 0x00000001 },
	{ 0x0838, 0x00000300 }, { 0x083c, 0x00010080 }, { 0x0840, 0x01030000 },
	{ 0x0844, 0x00a00000 }, { 0x0848, 0x00000001 }, { 0x084c, 0x00000103 },
	{ 0x0850, 0x000100e0 }, { 0x0854, 0x01030000 }, { 0x0858, 0x21800000 },
	{ 0x085c, 0x001c0001 }, { 0x0860, 0x00000203 }, { 0x0864, 0x00012181 },
	{ 0x0868, 0x0209001c }, { 0x086c, 0x21830000 }, { 0x0870, 0x001c0001 },
	{ 0x0874, 0x0000020a }, { 0x0878, 0x00012184 }, { 0x087c, 0x020b001c },
	{ 0x0880, 0x21850000 }, { 0x0884, 0x001c0001 }, { 0x0888, 0x00000209 },
	{ 0x088c, 0x000121a0 }, { 0x0890, 0x0203001c }, { 0x0894, 0x21a20000 },
	{ 0x0898, 0x00000001 }, { 0x089c, 0x0000010c }, { 0x08a0, 0x000121a4 },
	{ 0x08a4, 0x020b001c }, { 0x08a8, 0x21870000 }, { 0x08ac, 0x001c0001 },
	{ 0x08b0, 0x00000203 }, { 0x08b4, 0x00021c00 }, { 0x08b8, 0x01030000 },
	{ 0x08bc, 0x1c020000 }, { 0x08c0, 0x00000001 }, { 0x08c4, 0x00000119 },
	{ 0x08c8, 0x00011c03 }, { 0x08cc, 0x01180000 }, { 0x08d0, 0x1c040000 },
	{ 0x08d4, 0x00000004 }, { 0x08d8, 0x00000103 }, { 0x08dc, 0x00081c08 },
	{ 0x08e0, 0x01030000 }, { 0x08e4, 0x1c100000 }, { 0x08e8, 0x00000010 },
	{ 0x08ec, 0x00000103 }, { 0x08f0, 0x00201c20 }, { 0x08f4, 0x01030000 },
	{ 0x08f8, 0x1c400000 }, { 0x08fc, 0x00000040 }, { 0x0900, 0x00000103 },
	{ 0x0904, 0x00021c80 }, { 0x0908, 0x01030000 }, { 0x090c, 0x1c820000 },
	{ 0x0910, 0x00000001 }, { 0x0914, 0x00000119 }, { 0x0918, 0x00011c83 },
	{ 0x091c, 0x01180000 }, { 0x0920, 0x1c840000 }, { 0x0924, 0x00000004 },
	{ 0x0928, 0x00000103 }, { 0x092c, 0x00081c88 }, { 0x0930, 0x01030000 },
	{ 0x0934, 0x1c900000 }, { 0x0938, 0x00000010 }, { 0x093c, 0x00000103 },
	{ 0x0940, 0x00201ca0 }, { 0x0944, 0x01030000 }, { 0x0948, 0x1cc00000 },
	{ 0x094c, 0x00000040 }, { 0x0950, 0x00000103 }, { 0x1480, 0x0000018c },
	{ 0x1484, 0x00000001 }, { 0x1488, 0x00000001 }, { 0x148c, 0x03da0000 },
	{ 0x1494, 0x03d92004 }, { 0x149c, 0x00000001 }, { 0x14a0, 0x80000000 },
	{ 0x14a4, 0x03d92000 }, { 0x14ac, 0x00000001 }, { 0x14b0, 0x80000000 },
	{ 0x14c8, 0x00000010 }, { 0x14d0, 0x0000001e }, { 0x14d4, 0x0000000a },
	{ 0x14d8, 0x0000000e }, { 0x14dc, 0x00000060 }, { 0x14e0, 0x00010000 },
	{ 0x14e4, 0x0203003d }, { 0x14e8, 0x00010000 }, { 0x14ec, 0x003d0001 },
	{ 0x14f0, 0x00020203 }, { 0x14f4, 0x00010002 }, { 0x14f8, 0x020a003c },
	{ 0x14fc, 0x00030000 }, { 0x1500, 0x00000001 }, { 0x1504, 0x00000126 },
	{ 0x1508, 0x00010004 }, { 0x150c, 0x0203003c }, { 0x1510, 0x00050000 },
	{ 0x1514, 0x003c0001 }, { 0x1518, 0x00000203 }, { 0x151c, 0x00010007 },
	{ 0x1520, 0x0203003c }, { 0x1524, 0x04000000 }, { 0x1528, 0x003d0001 },
	{ 0x152c, 0x00000203 }, { 0x1530, 0x00010401 }, { 0x1534, 0x0203003d },
	{ 0x1538, 0x04020002 }, { 0x153c, 0x003c0001 }, { 0x1540, 0x0000020a },
	{ 0x1544, 0x00010403 }, { 0x1548, 0x01260000 }, { 0x154c, 0x04040000 },
	{ 0x1550, 0x003c0001 }, { 0x1554, 0x00000203 }, { 0x1558, 0x00010405 },
	{ 0x155c, 0x0203003c }, { 0x1560, 0x04070000 }, { 0x1564, 0x003c0001 },
	{ 0x1568, 0x00000203 }, { 0x160c, 0x0000000e }, { 0x1610, 0x00000002 },
	{ 0x1614, 0x24000003 }, { 0x1618, 0x000e0124 }, { 0x161c, 0x00020000 },
	{ 0x1620, 0x00040000 }, { 0x1624, 0x01242400 }, { 0x1628, 0x0000000e },
	{ 0x162c, 0x00000002 }, { 0x1630, 0x20000005 }, { 0x1634, 0x000e0124 },
	{ 0x1638, 0x00020000 }, { 0x163c, 0x00060000 }, { 0x1640, 0x01242000 },
	{ 0x1644, 0x0000000e }, { 0x1648, 0x00000002 }, { 0x164c, 0x20000007 },
	{ 0x1650, 0x000e0124 }, { 0x1654, 0x00020000 }, { 0x1658, 0x00090000 },
	{ 0x165c, 0x01242400 }, { 0x1660, 0x0000000e }, { 0x1664, 0x00000002 },
	{ 0x1668, 0x24000033 }, { 0x166c, 0x000e0124 }, { 0x1670, 0x00020000 },
	{ 0x1674, 0x000a0000 }, { 0x1678, 0x01242400 }, { 0x167c, 0x0000000e },
	{ 0x1680, 0x00000002 }, { 0x1684, 0x2400000b }, { 0x1688, 0x000e0124 },
	{ 0x168c, 0x00020000 }, { 0x1690, 0x000c0000 }, { 0x1694, 0x01242000 },
	{ 0x1698, 0x0000000e }, { 0x169c, 0x00000002 }, { 0x16a0, 0x2400001f },
	{ 0x16a4, 0x000e0124 }, { 0x16a8, 0x00020000 }, { 0x16ac, 0x000d0000 },
	{ 0x16b0, 0x01242400 }, { 0x16b4, 0x0000000e }, { 0x16b8, 0x00000002 },
	{ 0x16bc, 0x2400000e }, { 0x16c0, 0x000e0024 }, { 0x16c4, 0x00020000 },
	{ 0x16c8, 0x020f0000 }, { 0x16cc, 0x02202000 }, { 0x16d0, 0x0000000e },
	{ 0x16d4, 0x00000002 }, { 0x16d8, 0x20000210 }, { 0x16dc, 0x000e0220 },
	{ 0x16e0, 0x00020000 }, { 0x16e4, 0x00110000 }, { 0x16e8, 0x01242000 },
	{ 0x16ec, 0x0000000e }, { 0x16f0, 0x00000002 }, { 0x16f4, 0x20000016 },
	{ 0x16f8, 0x000e0124 }, { 0x16fc, 0x00020000 }, { 0x1700, 0x00180000 },
	{ 0x1704, 0x01242400 }, { 0x1708, 0x0000000e }, { 0x170c, 0x00000002 },
	{ 0x1710, 0x24000019 }, { 0x1714, 0x000e0124 }, { 0x1718, 0x00020000 },
	{ 0x171c, 0x011a0000 }, { 0x1720, 0x01242400 }, { 0x1724, 0x0000000e },
	{ 0x1728, 0x00000002 }, { 0x172c, 0x2400001b }, { 0x1730, 0x000e0124 },
	{ 0x1734, 0x00020000 }, { 0x1738, 0x00260000 }, { 0x173c, 0x00242400 },
	{ 0x1740, 0x0000000e }, { 0x1744, 0x00000002 }, { 0x1748, 0x2000001e },
	{ 0x174c, 0x000e0124 }, { 0x1750, 0x00020000 }, { 0x1754, 0x00200000 },
	{ 0x1758, 0x01242400 }, { 0x175c, 0x0000000e }, { 0x1760, 0x00000002 },
	{ 0x1764, 0x20000021 }, { 0x1768, 0x000e0124 }, { 0x176c, 0x00020000 },
	{ 0x1770, 0x01220000 }, { 0x1774, 0x01242400 }, { 0x1778, 0x0000000e },
	{ 0x177c, 0x00000002 }, { 0x1780, 0x24000024 }, { 0x1784, 0x000e0124 },
	{ 0x1788, 0x00020000 }, { 0x178c, 0x00250000 }, { 0x1790, 0x01242400 },
	{ 0x1794, 0x0000000e }, { 0x1798, 0x00000002 }, { 0x179c, 0x24000028 },
	{ 0x17a0, 0x000e0124 }, { 0x17a4, 0x00020000 }, { 0x17a8, 0x00290000 },
	{ 0x17ac, 0x01242400 }, { 0x17b0, 0x0000000e }, { 0x17b4, 0x00000002 },
	{ 0x17b8, 0x2400002a }, { 0x17bc, 0x000e0124 }, { 0x17c0, 0x00020000 },
	{ 0x17c4, 0x002b0000 }, { 0x17c8, 0x01202000 }, { 0x17cc, 0x0000000e },
	{ 0x17d0, 0x00000002 }, { 0x17d4, 0x2400002c }, { 0x17d8, 0x00010024 },
	{ 0x65f8, 0x00100000 }, { 0x65fc, 0x00010000 }, { 0x6600, 0x02010000 },
	{ 0x6604, 0x01024200 }, { 0x6608, 0x00100000 }, { 0x660c, 0x00010000 },
	{ 0x6610, 0x02020000 }, { 0x6614, 0x01024200 }, { 0x6618, 0x00100000 },
	{ 0x661c, 0x00010000 }, { 0x6620, 0x02050000 }, { 0x6624, 0x01024200 },
	{ 0x6628, 0x00100000 }, { 0x662c, 0x00010000 }, { 0x6630, 0x02060000 },
	{ 0x6634, 0x01024200 }, { 0x6638, 0x00100000 }, { 0x663c, 0x00010000 },
	{ 0x6640, 0x010a0000 }, { 0x6644, 0x01024200 }, { 0x6648, 0x00100000 },
	{ 0x664c, 0x00010000 }, { 0x6650, 0x0c080000 }, { 0x6654, 0x01024200 },
	{ 0x6658, 0x00100000 }, { 0x665c, 0x00010000 }, { 0x6660, 0x02070000 },
	{ 0x6664, 0x00034200 }, { 0x6668, 0x00100000 }, { 0x666c, 0x00010000 },
	{ 0x6670, 0x02080000 }, { 0x6674, 0x00034200 }, { 0x6678, 0x00100000 },
	{ 0x667c, 0x00010000 }, { 0x6680, 0x0c170000 }, { 0x6684, 0x00004200 },
	{ 0x6688, 0x00100000 }, { 0x668c, 0x00010000 }, { 0x6690, 0x0c0f0000 },
	{ 0x6694, 0x00004200 }, { 0x6698, 0x00100000 }, { 0x669c, 0x00010000 },
	{ 0x66a0, 0x0c190000 }, { 0x66a4, 0x00004200 }, { 0x66a8, 0x00100000 },
	{ 0x66ac, 0x00010000 }, { 0x66b0, 0x0a020000 }, { 0x66b4, 0x00004200 },
	{ 0x66b8, 0x00100000 }, { 0x66bc, 0x00010000 }, { 0x66c0, 0x06010000 },
	{ 0x66c4, 0x00034200 }, { 0x66c8, 0x00100000 }, { 0x66cc, 0x00010000 },
	{ 0x66d0, 0x01020000 }, { 0x66d4, 0x00034200 }, { 0x66d8, 0x00100000 },
	{ 0x66dc, 0x00010000 }, { 0x66e0, 0x03050000 }, { 0x66e4, 0x00034200 },
	{ 0x66e8, 0x00100000 }, { 0x66ec, 0x00010000 }, { 0x66f0, 0x03100000 },
	{ 0x66f4, 0x0003c200 }, { 0x66f8, 0x00100000 }, { 0x66fc, 0x00010000 },
	{ 0x6700, 0x05010000 }, { 0x6704, 0x00034200 }, { 0x6708, 0x00100000 },
	{ 0x670c, 0x00010000 }, { 0x6710, 0x05020000 }, { 0x6714, 0x00034200 },
	{ 0x6718, 0x00100000 }, { 0x671c, 0x00010000 }, { 0x6720, 0x09020000 },
	{ 0x6724, 0x00034200 }, { 0x6728, 0x00100000 }, { 0x672c, 0x00010000 },
	{ 0x6730, 0x01090000 }, { 0x6734, 0x00034200 }, { 0x6738, 0x00100000 },
	{ 0x673c, 0x00010000 }, { 0x6740, 0x01120000 }, { 0x6744, 0x00034200 },
	{ 0x6748, 0x00100000 }, { 0x674c, 0x00010000 }, { 0x6750, 0x030f0000 },
	{ 0x6754, 0x01024200 }, { 0x6758, 0x00100000 }, { 0x675c, 0x00010000 },
	{ 0x6760, 0x0c020000 }, { 0x6764, 0x00034200 }, { 0x6768, 0x00100000 },
	{ 0x676c, 0x00010000 }, { 0x6770, 0x030b0000 }, { 0x6774, 0x00034200 },
	{ 0x6778, 0x00100000 }, { 0x677c, 0x00010000 }, { 0x6780, 0x0c160000 },
	{ 0x6784, 0x01024200 }, { 0x6788, 0x00100000 }, { 0x678c, 0x00010000 },
	{ 0x6790, 0x000a0000 }, { 0x6794, 0x01027000 }, { 0x6798, 0x00100000 },
	{ 0x679c, 0x00010000 }, { 0x67a0, 0x090a0000 }, { 0x67a4, 0x01024200 },
	{ 0x67a8, 0x00100000 }, { 0x67ac, 0x00010000 }, { 0x67b0, 0x0f020000 },
	{ 0x67b4, 0x00034200 }, { 0x67b8, 0x00100000 }, { 0x67bc, 0x00010000 },
	{ 0x67c0, 0x0f030000 }, { 0x67c4, 0x00034200 }, { 0x67c8, 0x00100000 },
	{ 0x67cc, 0x00010000 }, { 0x67d0, 0x090b0000 }, { 0x67d4, 0x00004200 },
	{ 0x67d8, 0x58100000 }, { 0x67dc, 0x00020000 }, { 0x67e0, 0x12340000 },
	{ 0x67e8, 0x000a0000 }, { 0x67ec, 0x00040000 }, { 0x67f0, 0x005c0000 },
	{ 0x67f4, 0x000d0000 }, { 0x67f8, 0x00040000 }, { 0x67fc, 0x00840000 },
	{ 0x6800, 0x00360000 }, { 0x6804, 0x00100000 }, { 0x6808, 0x00b80000 },
	{ 0x680c, 0x00040000 }, { 0x6810, 0x00040000 }, { 0x6814, 0x04180000 },
	{ 0x6818, 0x04280000 }, { 0x681c, 0x13880000 }, { 0x6820, 0x00180000 },
	{ 0x6824, 0x00040000 }, { 0x6828, 0x17b00000 }, { 0x682c, 0x18100000 },
	{ 0x6830, 0x40000000 }, { 0x6834, 0x02010000 }, { 0x6838, 0x02024200 },
	{ 0x683c, 0x02054200 }, { 0x6840, 0x02064200 }, { 0x6844, 0x030f4200 },
	{ 0x6848, 0x010a4200 }, { 0x684c, 0x0c084200 }, { 0x6850, 0x0c164200 },
	{ 0x6854, 0x000a4200 }, { 0x6858, 0x090a7000 }, { 0x685c, 0x0c1e4200 },
	{ 0x6860, 0x0c164200 }, { 0x6864, 0x02014200 }, { 0x6868, 0x02024200 },
	{ 0x686c, 0x02054200 }, { 0x6870, 0x02064200 }, { 0x6874, 0x030f4200 },
	{ 0x6878, 0x030b4200 }, { 0x687c, 0x0c024200 }, { 0x6880, 0x0a014200 },
	{ 0x6884, 0x010a4200 }, { 0x6888, 0x0c1d4200 }, { 0x688c, 0x03104200 },
	{ 0x6890, 0x0000c200 }, { 0x6894, 0x00000300 }, { 0x6898, 0x10000000 },
	{ 0x68a0, 0x50000000 }, { 0x68a4, 0x00000355 }, { 0x68a8, 0x10000000 },
	{ 0x68b0, 0xb0000000 }, { 0x68b4, 0x00000355 }, { 0x68b8, 0x10000000 },
	{ 0x68c0, 0x10000000 }, { 0x68c4, 0x00000aac }, { 0x68c8, 0x10000000 },
	{ 0x68d0, 0x20000000 }, { 0x68d4, 0x00000aac }, { 0x68d8, 0x10000000 },
	{ 0x68e4, 0x00000aab }, { 0x68e8, 0x10000000 }, { 0x68f0, 0x70000000 },
	{ 0x68f4, 0x00000014 }, { 0x68f8, 0x10000000 }, { 0x6900, 0x30000000 },
	{ 0x6904, 0x00000338 }, { 0x6908, 0x10000000 }, { 0x6910, 0x90000000 },
	{ 0x6914, 0x00000338 }, { 0x6918, 0x10000000 }, { 0x6920, 0xa0000000 },
	{ 0x6924, 0x00000338 }, { 0x6928, 0x10000000 }, { 0x6930, 0xc0000000 },
	{ 0x6934, 0x00000338 }, { 0x6938, 0x10000000 }, { 0x6944, 0x000003c0 },
	{ 0x6948, 0x10000000 }, { 0x6950, 0x20000000 }, { 0x6954, 0x000003c0 },
	{ 0x6958, 0x10000000 }, { 0x6960, 0x40000000 }, { 0x6964, 0x000003c0 },
	{ 0x6968, 0x10000000 }, { 0x6974, 0x000003c4 }, { 0x6978, 0x10000000 },
	{ 0x6980, 0x10000000 }, { 0x6984, 0x00000394 }, { 0x6988, 0x10000000 },
	{ 0x6990, 0xa0000000 }, { 0x6994, 0x000003c0 }, { 0x6998, 0x10000000 },
	{ 0x69a0, 0x60000000 }, { 0x69a4, 0x00000339 }, { 0x69a8, 0x10000000 },
	{ 0x69b0, 0xe0000000 }, { 0x69b4, 0x00000331 }, { 0x69b8, 0x10000000 },
	{ 0x69c4, 0x00000330 }, { 0x69c8, 0x10000000 }, { 0x69d0, 0x20000000 },
	{ 0x69d4, 0x000001f6 }, { 0x69d8, 0x10000000 }, { 0x69e0, 0x10000000 },
	{ 0x69e4, 0x00000b5e }, { 0x69e8, 0x10000000 }, { 0x69f4, 0x00000c2d },
	{ 0x69f8, 0x10000000 }, { 0x6a00, 0x50000000 }, { 0x6a04, 0x00000014 },
	{ 0x6a08, 0x10000000 }, { 0x6a10, 0x40000000 }, { 0x6a14, 0x000001f7 },
	{ 0x6a18, 0x10000000 }, { 0x6a24, 0x00000c2e }, { 0x6a28, 0x10000000 },
	{ 0x6a30, 0xc0000000 }, { 0x6a34, 0x000003d3 }, { 0x6a38, 0x10000000 },
	{ 0x6a44, 0x00000a30 }, { 0x6a48, 0x10000000 }, { 0x6a54, 0x000008a0 },
	{ 0x6a58, 0x10000000 }, { 0x6a60, 0x70000000 }, { 0x6a64, 0x00000391 },
	{ 0x6a68, 0x10000000 }, { 0x6a70, 0x80000000 }, { 0x6a74, 0x00000391 },
	{ 0x6a78, 0x10000000 }, { 0x6a80, 0xd0000000 }, { 0x6a84, 0x00000391 },
	{ 0x6a88, 0x10000000 }, { 0x6a94, 0x00000391 }, { 0x6a98, 0x10000000 },
	{ 0x6aa0, 0x10000000 }, { 0x6aa4, 0x00000391 }, { 0x6aa8, 0x10000000 },
	{ 0x6ab0, 0x20000000 }, { 0x6ab4, 0x00000391 }, { 0x6ab8, 0x10000000 },
	{ 0x6ac0, 0x90000000 }, { 0x6ac4, 0x00000391 }, { 0x6ac8, 0x10000000 },
	{ 0x6ad0, 0x60000000 }, { 0x6ad4, 0x00000391 }, { 0x6ad8, 0x10000000 },
	{ 0x6ae0, 0x50000000 }, { 0x6ae4, 0x00000391 }, { 0x6ae8, 0x10000000 },
	{ 0x6af4, 0x00000394 }, { 0x6af8, 0x10000000 }, { 0x6b00, 0x10000000 },
	{ 0x6b04, 0x00000390 }, { 0x6b08, 0x10000000 }, { 0x6b10, 0xb0000000 },
	{ 0x6b14, 0x00000339 }, { 0x6b18, 0x10000000 }, { 0x6b20, 0xd0000000 },
	{ 0x6b24, 0x00000019 }, { 0x6b28, 0x10000000 }, { 0x6b30, 0x70000000 },
	{ 0x6b34, 0x000001f7 }, { 0x6b38, 0x10000000 }, { 0x6b44, 0x00000c2c },
	{ 0x6b48, 0x10000000 }, { 0x6b50, 0xb0000000 }, { 0x6b54, 0x00000a00 },
	{ 0x6b58, 0x10000000 }, { 0x6b60, 0x40000000 }, { 0x6b64, 0x00000a00 },
	{ 0x6b68, 0x10000000 }, { 0x6b74, 0x00000a01 }, { 0x6b78, 0x10000000 },
	{ 0x6b80, 0x60000000 }, { 0x6b84, 0x00000a01 }, { 0x6b88, 0x10000000 },
	{ 0x6b90, 0x80000000 }, { 0x6b94, 0x00000a31 }, { 0x6b98, 0x10000000 },
	{ 0x6ba0, 0x10000000 }, { 0x6ba4, 0x00000a34 }, { 0x6ba8, 0x10000000 },
	{ 0x6bb0, 0x10000000 }, { 0x6bb4, 0x000003d8 }, { 0x6bb8, 0x10000000 },
	{ 0x6bc0, 0x10000000 }, { 0x6bc4, 0x00000aab }, { 0x6bc8, 0x10000000 },
	{ 0x6bd4, 0x00000078 }, { 0x6bd8, 0x10000000 }, { 0x6be4, 0x00000a35 },
	{ 0x6be8, 0x10000000 }, { 0x6bf0, 0x00010000 }, { 0x6bf4, 0x000c0000 },
	{ 0x6bf8, 0x00080000 }, { 0x6bfc, 0x000b0000 }, { 0x6c00, 0x00010000 },
	{ 0x7f88, 0x00050000 }, { 0x7f8c, 0x00060000 }, { 0x7f90, 0x00070000 },
	{ 0x7f94, 0x00090000 }, { 0x7f98, 0x00330000 }, { 0x7f9c, 0x000a0000 },
	{ 0x7fa0, 0x000b0000 }, { 0x7fa4, 0x000c0000 }, { 0x7fa8, 0x00110000 },
	{ 0x7fac, 0x00160000 }, { 0x7fb0, 0x00180000 }, { 0x7fb4, 0x00190000 },
	{ 0x7fb8, 0x001a0000 }, { 0x7fbc, 0x001b0000 }, { 0x7fc0, 0x001e0000 },
	{ 0x7fc4, 0x00210000 }, { 0x7fc8, 0x00220000 }, { 0x7fcc, 0x00240000 },
	{ 0x7fd0, 0x00260000 }, { 0x7fd4, 0x00280000 }, { 0x7fd8, 0x000f0000 },
	{ 0x7fdc, 0x00200000 }, { 0x7fe0, 0x00250000 }, { 0x7fe4, 0x002b0000 },
	{ 0x7fe8, 0x00010000 }, { 0xbfe8, 0x00010000 },
};

static void qcom_csrt_write_group(struct acpi_ctx *ctx, u32 length,
				  u16 device_id, u16 revision)
{
	struct acpi_csrt_group *group = ctx->current;

	memset(group, 0, sizeof(*group));
	group->length = length;
	group->vendor_id = QCOM_CSRT_VENDOR_ID;
	group->device_id = device_id;
	group->revision = revision;
	acpi_inc(ctx, sizeof(*group));
}

static void qcom_csrt_write_descriptor(struct acpi_ctx *ctx, u32 length,
				       u16 type, u16 subtype, u32 uid)
{
	struct acpi_csrt_descriptor *desc = ctx->current;

	desc->length = length;
	desc->type = type;
	desc->subtype = subtype;
	desc->uid = uid;
	acpi_inc(ctx, sizeof(*desc));
}

static void qcom_csrt_apply_words(void *payload)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(qcom_csrt_100c_words); i++)
		memcpy((u8 *)payload + qcom_csrt_100c_words[i].offset,
		       &qcom_csrt_100c_words[i].value,
		       sizeof(qcom_csrt_100c_words[i].value));
}

int acpi_fill_csrt(struct acpi_ctx *ctx)
{
	void *payload;

	/*
	 * CSRT DROPPED for Windows bring-up: a ~69 KB hand-transcribed
	 * QCOMEDK2 vendor blob of unverified provenance, not consumed before
	 * the kernel runs its own drivers. Removing it eliminates an unknown
	 * descriptor (and dead weight) with zero risk to reaching kernel
	 * entry. -ENOENT makes the generic writer cleanly skip the table.
	 * (Body left in place but unreachable so the static helpers/payloads
	 * stay referenced; re-enable by removing this return.)
	 */
	return -ENOENT;

	qcom_csrt_write_group(ctx, QCOM_CSRT_GROUP_100B_LENGTH, 0x100b, 0);
	qcom_csrt_write_descriptor(ctx, QCOM_CSRT_DESC_100B_LENGTH,
				   0x0002, 0x0000, 0x00000001);
	memcpy(ctx->current, qcom_csrt_100b_payload,
	       sizeof(qcom_csrt_100b_payload));
	acpi_inc(ctx, sizeof(qcom_csrt_100b_payload));

	qcom_csrt_write_group(ctx, QCOM_CSRT_GROUP_100C_LENGTH, 0x100c, 1);
	qcom_csrt_write_descriptor(ctx, QCOM_CSRT_DESC_100C_LENGTH,
				   0x0004, 0x0001, QCOM_CSRT_DESC_100C_UID);
	payload = ctx->current;
	memset(payload, 0, QCOM_CSRT_DESC_100C_PAYLOAD);
	qcom_csrt_apply_words(payload);
	acpi_inc(ctx, QCOM_CSRT_DESC_100C_PAYLOAD);

	return 0;
}

/*
 * BGRT (Boot Graphics Resource Table) support
 * Shows OEM logo during boot
 */
static __maybe_unused int tachyon_write_bgrt(struct acpi_ctx *ctx,
					     const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct acpi_bgrt *bgrt;

	header = ctx->current;
	bgrt = (struct acpi_bgrt *)header;
	memset(bgrt, 0, sizeof(*bgrt));

	acpi_fill_header(header, "BGRT");
	header->length = sizeof(*bgrt);
	header->revision = acpi_get_table_revision(ACPITAB_BGRT);

	bgrt->version = 1;
	bgrt->status = 0;
	bgrt->image_type = 0;

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);
	acpi_inc(ctx, sizeof(struct acpi_bgrt));

	return 0;
}

/*
 * BGRT DROPPED for Windows bring-up: it points at a NULL boot-image
 * address (image_address is never set), a cosmetic logo table that at worst
 * makes the OS graphics hand-off poke address 0. Unregister it for the
 * minimal build. Re-add once a real boot logo buffer is populated.
 */
/* ACPI_WRITER(5bgrt, "BGRT", tachyon_write_bgrt, 0); */

/*
 * DBG2 (Debug Port Table 2) support
 * Defines debug UART for Windows debugging
 */
static int tachyon_write_dbg2(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_dbg2_header *dbg2 = (struct acpi_dbg2_header *)ctx->current;
	struct acpi_gen_regaddr address;

	/*
	 * win_serial=0 (basic-boot test): omit DBG2 so winload finds no debug
	 * UART to map -> no 0x994000 LoaderData mapping -> no ntoskrnl DC-ZVA
	 * alignment fault.  Paired with the same gate in efi_setup.c + SPCR.
	 */
	if (env_get_yesno("win_serial") == 0)
		return 0;

	/*
	 * The debug UART is Qualcomm GENI serial engine QUP_0_SE_5 at 0x994000
	 * (MMIO).  Port subtype ARM_SBSA_GENERIC is the best fit for a
	 * non-standard ARM debug UART.  The device path MUST be the fully
	 * qualified namespace path of a real DSDT device: \_SB.UARD in dsdt.asl.
	 *
	 * ntoskrnl maps this Device-nGnRnE.  The QUP window is presented to the
	 * OS as EfiMemoryMappedIO (efi_setup.c), so this access matches the QHEE
	 * HLOS stage-2 Device mapping and does NOT alignment-fault.  (Earlier we
	 * wrongly suppressed DBG2/SPCR; the fix is Device typing, not omission.)
	 */
	memset(&address, '\0', sizeof(address));
	address.space_id = ACPI_ADDRESS_SPACE_MEMORY;
	address.bit_width = 32;
	address.access_size = ACPI_ACCESS_SIZE_DWORD_ACCESS;
	address.addrl = 0x994000;  /* \_SB.UARD base, matches dsdt.asl UARD._CRS */

	acpi_create_dbg2(dbg2, ACPI_DBG2_SERIAL_PORT,
			 ACPI_DBG2_ARM_SBSA_GENERIC,
			 &address, 0x1000, "\\_SB.UARD");

	acpi_add_table(ctx, dbg2);
	acpi_inc_align(ctx, dbg2->header.length);

	return 0;
}

ACPI_WRITER(5dbg2, "DBG2", tachyon_write_dbg2, 0);

/*
 * SPCR (Serial Port Console Redirection) — board override
 *
 * The generic acpi_write_spcr() auto-discovers the serial device, but the
 * Qualcomm GENI driver implements no .getinfo, so the generic table comes out
 * malformed (interface_type = UNKNOWN/0xFF, address = 0) and never sets the
 * interrupt / terminal-type fields at all — Windows then has no usable console
 * descriptor.  The generic writer is __weak, so we override it (the existing
 * ACPI_WRITER(5spcr) linker-list entry redirects to this strong symbol; no
 * second writer / no duplicate table).
 *
 * Console = \_SB.UARD (QUP_0_SE_5) = serial0 @ 0x00994000, GSIV 638, 115200 8N1.
 * Interface type 0x11 is the Qualcomm SDM845/GENI UART class Windows' inbox
 * driver binds to.
 *
 * NOTE (HW-proven 2026-06-21): the SPCR console MUST stay on 0x994000.
 * Pointing it at Radxa's QUP1_SE4 @ 0xA90000 hung winload pre-SVAM (that SE
 * is not powered/early-mapped on Tachyon), and so did dropping serial.  The
 * 0x994000 reclaim fault is fixed in efi_boottime.c (mdfix retag), not by
 * moving the console.
 */
#define TACHYON_SPCR_GENI_IFACE		0x11	/* Qualcomm SDM845/GENI UART */
#define TACHYON_UART5_BASE		0x00994000UL	/* QUP0_SE5 — SPCR console + DBG2 */
#define TACHYON_UART5_GSIV		638	/* GIC_SPI 606 + 32 */
#define TACHYON_SPCR_INT_TYPE_GIC	0x08	/* ARMH GIC interrupt */
#define TACHYON_SPCR_BAUD_115200	7
#define TACHYON_SPCR_TERMINAL_ANSI	3

int acpi_write_spcr(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_spcr *spcr = ctx->current;
	struct acpi_table_header *header = &spcr->header;

	(void)entry;

	/*
	 * win_serial=0 (basic-boot test): omit SPCR so winload has no console
	 * UART to early-map.  See efi_setup.c gate + tachyon_write_dbg2().
	 */
	if (env_get_yesno("win_serial") == 0)
		return 0;

	memset(spcr, 0, sizeof(*spcr));

	acpi_fill_header(header, "SPCR");
	header->length = sizeof(struct acpi_spcr);
	header->revision = 2;

	spcr->interface_type = TACHYON_SPCR_GENI_IFACE;

	spcr->serial_port.space_id    = ACPI_ADDRESS_SPACE_MEMORY;
	spcr->serial_port.bit_width   = 32;
	spcr->serial_port.bit_offset  = 0;
	spcr->serial_port.access_size = ACPI_ACCESS_SIZE_DWORD_ACCESS;
	spcr->serial_port.addrl       = TACHYON_UART5_BASE;  /* 0x994000 — winload needs this page early (HW-proven) */
	spcr->serial_port.addrh       = 0;

	spcr->interrupt_type = TACHYON_SPCR_INT_TYPE_GIC;
	spcr->pc_interrupt   = 0;
	spcr->interrupt      = TACHYON_UART5_GSIV;

	spcr->baud_rate     = TACHYON_SPCR_BAUD_115200;
	spcr->parity        = 0;
	spcr->stop_bits     = 1;
	spcr->flow_control  = 0;
	spcr->terminal_type = TACHYON_SPCR_TERMINAL_ANSI;

	/* Not a PCI device */
	spcr->pci_device_id = 0xffff;
	spcr->pci_vendor_id = 0xffff;

	header->checksum = table_compute_checksum((void *)spcr, header->length);

	acpi_add_table(ctx, spcr);
	acpi_inc(ctx, header->length);

	return 0;
}

/*
 * FACS (Firmware ACPI Control Structure)
 *
 * The generic ACPI code already writes FACS at writer priority 1
 * (lib/acpi/facs.c: ACPI_WRITER(1facs, "FACS", acpi_write_facs, 0))
 * and stores it in ctx->facs for FADT to reference.
 *
 * FACS is NOT listed as a normal XSDT table — it is pointed to by
 * the FADT firmware_ctrl / x_firmware_ctrl fields.  Publishing a
 * second FACS via acpi_add_table() is incorrect per the ACPI spec.
 * We therefore remove the custom Tachyon FACS writer entirely.
 */

/*
 * TPM2 (Trusted Platform Module 2.0) Table
 * Qualcomm Windows firmware publishes this as a legacy/vendor-specific
 * TPM2 start method table.
 */
#if IS_ENABLED(CONFIG_TPM_V2)
#define TACHYON_TPM2_START_METHOD_QCOM	9
#define TACHYON_TPM2_QCOM_RESERVED_SIZE	32

struct __packed tachyon_acpi_tpm2_qcom {
	struct acpi_table_header header;
	u32 reserved;
	u64 control_area;
	u32 start_method;
	u8 reserved2[TACHYON_TPM2_QCOM_RESERVED_SIZE];
};

static int tachyon_write_tpm2(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct tachyon_acpi_tpm2_qcom *tpm2;

	if (!IS_ENABLED(CONFIG_TPM_V2))
		return -ENOENT;

	tpm2 = ctx->current;
	header = &tpm2->header;
	memset(tpm2, 0, sizeof(*tpm2));

	acpi_fill_header(header, "TPM2");
	header->length = sizeof(*tpm2);
	header->revision = acpi_get_table_revision(ACPITAB_TPM2);

	/*
	 * Qualcomm Windows firmware publishes TPM2 with legacy/vendor-specific
	 * start method 9 and no generic CRB control area. Do not advertise
	 * method 11 here unless a real CRB + ARM SMC/HVC ABI is available.
	 */
	tpm2->control_area = 0;
	tpm2->start_method = TACHYON_TPM2_START_METHOD_QCOM;

	header->checksum = table_compute_checksum(tpm2, header->length);

	acpi_inc(ctx, tpm2->header.length);
	acpi_add_table(ctx, tpm2);

	return 0;
}

ACPI_WRITER(5tpm2, "TPM2", tachyon_write_tpm2, 0);
#endif
