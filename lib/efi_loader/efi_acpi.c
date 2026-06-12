// SPDX-License-Identifier: GPL-2.0+
/*
 *  EFI application ACPI tables support
 *
 *  Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#define LOG_CATEGORY LOGC_EFI

#include <efi_loader.h>
#include <log.h>
#include <mapmem.h>
#include <acpi/acpi_table.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

static const efi_guid_t acpi_guid = EFI_ACPI_TABLE_GUID;

/*
 * Install the ACPI table as a configuration table.
 *
 * Return:	status code
 */
efi_status_t efi_acpi_register(void)
{
	ulong addr, start, end;
	efi_status_t ret;

	/*
	 * The ACPI tables MUST be mapped as EFI_ACPI_RECLAIM_MEMORY and the RSDP
	 * MUST be installed as an EFI configuration table: unlike x86, ARM (and
	 * therefore Windows-on-ARM) has no legacy memory scan to discover the
	 * RSDP, and leaving the tables in boot-services/conventional memory lets
	 * the OS reuse those pages after ExitBootServices and crash.
	 *
	 * When the tables live in the bloblist (CONFIG_BLOBLIST_TABLES) only the
	 * table sub-region needs re-marking; the broad gd->arch.table_start area
	 * used by the non-bloblist path is not populated. We deliberately mark
	 * just that sub-region rather than skipping the install entirely (the
	 * previous behaviour, which left ARM EFI payloads with no ACPI at all).
	 */
	if (IS_ENABLED(CONFIG_BLOBLIST_TABLES)) {
		start = ALIGN_DOWN(gd->arch.table_start_high, EFI_PAGE_MASK);
		end = ALIGN(gd->arch.table_end_high, EFI_PAGE_MASK);
		ret = efi_add_memory_map(start, end - start,
					 EFI_ACPI_RECLAIM_MEMORY);
		if (ret != EFI_SUCCESS)
			return ret;
	} else {
		/* Mark space used for tables */
		start = ALIGN_DOWN(gd->arch.table_start, EFI_PAGE_MASK);
		end = ALIGN(gd->arch.table_end, EFI_PAGE_MASK);
		ret = efi_add_memory_map(start, end - start,
					 EFI_ACPI_RECLAIM_MEMORY);
		if (ret != EFI_SUCCESS)
			return ret;
		if (gd->arch.table_start_high) {
			start = ALIGN_DOWN(gd->arch.table_start_high, EFI_PAGE_MASK);
			end = ALIGN(gd->arch.table_end_high, EFI_PAGE_MASK);
			ret = efi_add_memory_map(start, end - start,
						 EFI_ACPI_RECLAIM_MEMORY);
			if (ret != EFI_SUCCESS)
				return ret;
		}
	}

	addr = gd_acpi_start();
	log_debug("EFI using ACPI tables at %lx\n", addr);

	/* And expose them to our EFI payload */
	return efi_install_configuration_table(&acpi_guid,
					       (void *)(ulong)addr);
}
