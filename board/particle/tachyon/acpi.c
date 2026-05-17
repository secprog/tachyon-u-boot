// SPDX-License-Identifier: GPL-2.0+
/*
 * ACPI Support for Particle Tachyon (QCM6490)
 * Properly implemented using U-Boot's ACPI helper functions
 * based on the extracted ACPI tables from FAT12.bin
 *
 * Copyright (c) 2024-2025 Particle Industries, Inc.
 */

#include <cpu.h>
#include <tables_csum.h>
#include <string.h>
#include <acpi/acpi_table.h>
#include <asm/acpi_table.h>
#include <asm/armv8/sec_firmware.h>
#include <dm/uclass.h>
#include <dm/device.h>

/*
 * QCM6490 SoC ACPI Support for Windows ARM64 boot
 *
 * PSCI is handled by TF-A (Trusted Firmware-A) at EL3.
 * U-Boot's role is to:
 * 1. Declare PSCI compliance in FADT (arm_boot_arch field)
 * 2. Provide correct ACPI tables so Windows can boot and use SMP
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
 * MADT (Multiple APIC Description Table) support
 * Defines GIC (Generic Interrupt Controller) structures for ARM
 *
 * QCM6490 uses GICv3 interrupt controller
 * - GICD (Distributor): 0x17a00000 (from extracted table)
 * - GICR (Redistributor): Calculated from CPU entries
 * - 8 CPU cores: 4x Cortex-A55 + 4x Cortex-A78
 */
int acpi_fill_madt(struct acpi_madt *madt, struct acpi_ctx *ctx)
{
	struct acpi_madt_gicd *gicd;
	struct acpi_madt_gicr *gicr;
	struct acpi_madt_gicc *gicc;

	/*
	 * GICD (GIC Distributor) - Type 0x0C
	 * Base address: 0x17a00000 (from extracted table)
	 * GIC version: 3 (GICv3)
	 */
	gicd = ctx->current;
	gicd->type = ACPI_APIC_GICD;
	gicd->length = sizeof(struct acpi_madt_gicd);
	gicd->reserved = 0;
	gicd->gic_id = 0;
	gicd->phys_base = 0x17a00000;  /* GICD base from extracted table */
	gicd->reserved2 = 0;
	gicd->gic_version = 3;  /* GICv3 */
	gicd->reserved3[0] = 0;
	gicd->reserved3[1] = 0;
	gicd->reserved3[2] = 0;

	acpi_inc(ctx, gicd->length);

	/*
	 * GICR (GIC Redistributor) - Type 0x0E
	 * Base address: 0x17a60000 (GICR base, calculated)
	 * Length: 0x100000 (1MB for redistributor region)
	 */
	gicr = ctx->current;
	gicr->type = ACPI_APIC_GICR;
	gicr->length = sizeof(struct acpi_madt_gicr);
	gicr->reserved = 0;
	gicr->discovery_range_base_address = 0x17a60000;  /* GICR base */
	gicr->discovery_range_length = 0x100000;  /* GICR range for 8 CPUs */

	acpi_inc(ctx, gicr->length);

	/*
	 * GICC (GIC CPU Interface) entries - Type 0x0B
	 * Create 8 entries for the 8 CPU cores
	 *
	 * MPIDR values for QCM6490 (SC7280):
	 * - CPU 0-3 (Cortex-A55): MPIDR = 0x00000000, 0x00000100, 0x00000200, 0x00000300
	 * - CPU 4-7 (Cortex-A78): MPIDR = 0x00000400, 0x00000500, 0x00000600, 0x00000700
	 *
	 * Note: The extracted table had wrong MPIDR values (0x1900000000).
	 * We use the correct MPIDR values based on the device tree.
	 */
	for (int i = 0; i < 8; i++) {
		gicc = ctx->current;
		gicc->type = ACPI_APIC_GICC;
		gicc->length = sizeof(struct acpi_madt_gicc);
		gicc->reserved = 0;
		gicc->cpu_if_num = i;
		gicc->processor_id = i;
		gicc->flags = ACPI_MADTF_ENABLED;

		/* Parking protocol not needed when PSCI is present */
		gicc->parking_proto = 0;

		/* PMU/perf interrupt (PPI 23) */
		gicc->perf_gsiv = 23;

		/* MPIDR for this CPU */
		gicc->mpidr = (i << 8);  /* Simple MPIDR: 0x000, 0x100, 0x200, etc. */

		/* GICR base for this CPU */
		gicc->gicr_base = 0x17a60000 + (i * 0x20000);

		/* Parked address - not used with PSCI */
		gicc->parked_addr = 0;

		/* Virtual GIC interface - not used on production systems */
		gicc->gicv = 0;
		gicc->gich = 0;
		gicc->vgic_maint_irq = 0;

		/* Efficiency class: A55 = 0 (more efficient), A78 = 1 (less efficient) */
		gicc->efficiency = (i < 4) ? 0 : 1;

		gicc->reserved2 = 0;
		gicc->spi_overflow_irq = 0;

		acpi_inc(ctx, gicc->length);
	}

	return 0;
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

	acpi_inc(ctx, header->length);
	acpi_add_table(ctx, header);

	return 0;
}

ACPI_WRITER(5pptt, "PPTT", tachyon_write_pptt, 0);

/*
 * GTDT (Generic Timer Description Table) support
 * ARM Generic Timer interrupts for Windows
 */
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
	 * ARM Generic Timer interrupts (PPI - Private Peripheral Interrupts)
	 * These match the device tree timer node interrupts:
	 * - Secure EL1 timer: PPI 29
	 * - Non-secure EL1 timer: PPI 30
	 * - Virtual EL1 timer: PPI 27
	 * - EL2 timer: PPI 26
	 *
	 * Note: The extracted table had these as 0. We use the correct values.
	 */
	gtdt->cnt_ctrl_base = 0xFFFFFFFFFFFFFFFF; /* Not used */
	gtdt->sec_el1_gsiv = 29;  /* Secure EL1 timer */
	gtdt->sec_el1_flags = GTDT_FLAG_INT_ACTIVE_LOW;
	gtdt->el1_gsiv = 30;      /* Non-secure EL1 timer */
	gtdt->el1_flags = GTDT_FLAG_INT_ACTIVE_LOW;
	gtdt->virt_el1_gsiv = 27; /* Virtual EL1 timer */
	gtdt->virt_el1_flags = GTDT_FLAG_INT_ACTIVE_LOW;
	gtdt->el2_gsiv = 26;      /* EL2 timer */
	gtdt->el2_flags = GTDT_FLAG_INT_ACTIVE_LOW;
	gtdt->cnt_read_base = 0xFFFFFFFFFFFFFFFF;

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, gtdt);
	acpi_inc(ctx, sizeof(struct acpi_gtdt));

	return 0;
}

ACPI_WRITER(5gtdt, "GTDT", tachyon_write_gtdt, 0);

/*
 * MCFG (PCI Express MMIO Config Space) support
 * ECAM for PCIe controllers
 */
static int tachyon_write_mcfg(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct acpi_mcfg_mmconfig *mmconfig;
	u32 size;

	header = ctx->current;
	acpi_fill_header(header, "MCFG");
	header->length = sizeof(struct acpi_table_header) + sizeof(struct acpi_mcfg_mmconfig);
	header->revision = acpi_get_table_revision(ACPITAB_MCFG);

	/*
	 * PCIe ECAM (Enhanced Configuration Access Mechanism)
	 * Base: 0x40000000 (from extracted table)
	 * Segment: 0
	 * Start Bus: 0
	 * End Bus: 255 (full bus range)
	 */
	size = acpi_create_mcfg_mmconfig((void *)ctx->current,
		(0x40000000,  /* ECAM base from extracted table */
		0,           /* PCI segment group number */
		0,           /* Start bus number */
		255);         /* End bus number */

	acpi_inc(ctx, size);
	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);

	return 0;
}

ACPI_WRITER(5mcfg, "MCFG", tachyon_write_mcfg, 0);

/*
 * BGRT (Boot Graphics Resource Table) support
 * Shows OEM logo during boot
 */
static int tachyon_write_bgrt(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;

	header = ctx->current;

	acpi_fill_header(header, "BGRT");
	header->length = sizeof(struct acpi_bgrt);
	header->revision = acpi_get_table_revision(ACPITAB_BGRT);

	/* BGRT: Version 1, Status 0 (displayed), Image type 0 (BMP) */
	struct acpi_bgrt *bgrt = (struct acpi_bgrt *)header;
	bgrt->version = 1;
	bgrt->status = 0;  /* Image displayed */
	bgrt->image_type = 0;  /* BMP format */
	bgrt->offset_x = 0;
	bgrt->offset_y = 0;
	/* image_address is set by firmware if logo is present */

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);
	acpi_inc(ctx, sizeof(struct acpi_bgrt));

	return 0;
}

ACPI_WRITER(5bgrt, "BGRT", tachyon_write_bgrt, 0);

/*
 * CSRT (Core System Resource Table) support
 * Defines DMA controllers and their channels
 */
static int tachyon_write_csrt(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;

	header = ctx->current;

	acpi_fill_header(header, "CSRT");
	/* CSRT length from extracted table: 69258 bytes */
	header->length = 69258;
	header->revision = 0;  /* CSRT revision 0 */

	/*
	 * CSRT contains DMA controller descriptions.
	 * The extracted table is 69258 bytes - quite large.
	 * For now, create a minimal CSRT. If DMA is needed,
	 * this should be expanded with proper DMA controller info.
	 */

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);
	acpi_inc(ctx, header->length);

	return 0;
}

ACPI_WRITER(5csrt, "CSRT", tachyon_write_csrt, 0);

/*
 * DBG2 (Debug Port Table 2) support
 * Defines debug UART for Windows debugging
 */
static int tachyon_write_dbg2(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct acpi_dbg2_device *dbg2;
	u32 *addr_size;

	header = ctx->current;

	acpi_fill_header(header, "DBG2");
	header->length = sizeof(struct acpi_dbg2_header) + sizeof(struct acpi_dbg2_device) + 16;
	header->revision = 0;  /* DBG2 revision 0 */

	/* DBG2 header */
	struct acpi_dbg2_header *dbg2_hdr = (struct acpi_dbg2_header *)header;
	dbg2_hdr->devices_offset = sizeof(struct acpi_dbg2_header);
	dbg2_hdr->devices_count = 1;

	/* DBG2 device (UART5 - debug console) */
	dbg2 = (struct acpi_dbg2_device *)((void *)header + sizeof(struct acpi_dbg2_header));
	dbg2->revision = 0;
	dbg2->length = sizeof(struct acpi_dbg2_device) + 16;
	dbg2->address_count = 1;
	dbg2->namespace_string_length = 4;
	dbg2->namespace_string_offset = dbg2->length;
	dbg2->oem_data_length = 0;
	dbg2->oem_data_offset = dbg2->length + dbg2->namespace_string_length;
	dbg2->port_type = ACPI_DBG2_ARM_SBSA_32BIT;
	dbg2->port_subtype = ACPI_DBG2_ARM_SBSA_GENERIC;
	dbg2->reserved = 0;

	/* Namespace string: "\_SB_.UART5" */
	char *ns = (char *)dbg2 + dbg2->namespace_string_offset;
	memcpy(ns, "\\_SB.UART5", 10);

	/* Address (GICC base from device tree) */
	addr_size = (u32 *)((void *)dbg2 + dbg2->length);
	addr_size[0] = 0x994000;  /* UART5 base from qcm6490-tachyon-u-boot.dtsi */
	addr_size[1] = 0;      /* Base address high */
	addr_size[2] = 0x1000;  /* Address size */
	addr_size[3] = 0x1000;  /* Size of address region */

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);
	acpi_inc(ctx, header->length);

	return 0;
}

ACPI_WRITER(5dbg2, "DBG2", tachyon_write_dbg2, 0);

/*
 * SPCR (Serial Port Console Redirection) support
 * Defines console redirection for Windows
 */
static int tachyon_write_spcr(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_table_header *header;
	struct acpi_spcr *spcr;

	header = ctx->current;
	spcr = (struct acpi_spcr *)header;

	acpi_fill_header(header, "SPCR");
	header->length = sizeof(struct acpi_spcr);
	header->revision = 2;  /* SPCR revision 2 */

	/* SPCR: 16550-compatible UART at 0x994000 */
	spcr->interface_type = ACPI_DBG2_16550_COMPATIBLE;
	spcr->base_address.space_id = ACPI_ADDRESS_SPACE_MEMORY;
	spcr->base_address.bit_width = 8;
	spcr->base_address.bit_offset = 0;
	spcr->base_address.access_size = ACPI_ACCESS_SIZE_BYTE_ACCESS;
	spcr->base_address.addrl = 0x994000;  /* UART5 base */
	spcr->base_address.addrh = 0;
	spcr->interrupt_type = 1;  /* GIC interrupt */
	spcr->gsiv = 115;  /* UART5 interrupt (from device tree) */
	spcr->baud_rate = 115200;
	spcr->parity = 0;
	spcr->stop_bits = 1;
	spcr->flow_control = 0;
	spcr->terminal_type = 0;
	spcr->pci_device_id = 0xffff;
	spcr->pci_vendor_id = 0xffff;
	spcr->pci_bus = 0;
	spcr->pci_device = 0;
	spcr->pci_function = 0;
	spcr->pci_flags = 0;
	spcr->pci_segment = 0;

	header->checksum = table_compute_checksum(header, header->length);

	acpi_add_table(ctx, header);
	acpi_inc(ctx, sizeof(struct acpi_spcr));

	return 0;
}

ACPI_WRITER(5spcr, "SPCR", tachyon_write_spcr, 0);

/*
 * FACS (Firmware ACPI Control Structure) support
 * Used for ACPI S4 (hibernate) support
 */
static int tachyon_write_facs(struct acpi_ctx *ctx, const struct acpi_writer *entry)
{
	struct acpi_facs *facs;

	facs = ctx->current;

	memset(facs, 0, sizeof(struct acpi_facs));
	memcpy(facs->signature, "FACS", 4);
	facs->length = sizeof(struct acpi_facs);
	facs->hardware_signature = 0;
	facs->firmware_waking_vector = 0;
	facs->global_lock = 0;
	facs->flags = 0;
	facs->x_firmware_waking_vector_l = 0;
	facs->x_firmware_waking_vector_h = 0;
	facs->version = 1;  /* FACS version 1 */
	facs->ospm_flags = 0;

	acpi_add_table(ctx, facs);
	acpi_inc(ctx, sizeof(struct acpi_facs));

	return 0;
}

ACPI_WRITER(5facs, "FACS", tachyon_write_facs, 0);
