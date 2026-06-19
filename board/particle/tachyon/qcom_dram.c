// SPDX-License-Identifier: GPL-2.0+
/*
 * Memory layout parsing for Qualcomm.
 *
 * Copyright (c) 2025 Particle Industries, Inc.
 * Copyright (c) 2025 Linaro Limited.
 */

#include "qcom_dram.h"
#include <dm.h>
#include <log.h>
#include <sort.h>
#include <smem.h>
#include <linux/string.h>
#include <dm/device-internal.h>

int qcom_parse_memory_smem(qcom_mem_bank* banks, size_t nbanks)
{
    size_t size;
    int i, j = 0, ret;
    struct smem_ram_ptable *ram_ptable;
    struct smem_ram_ptn *p;

    struct udevice *dev = NULL;

    if (banks == NULL) {
        return -EINVAL;
    }
    ret = uclass_first_device_err(UCLASS_SMEM, &dev);
    if (ret) {
        debug("failed to uclass_get_device() %d\n", ret);
        return ret;
    }

    ram_ptable = smem_get(dev, -1 /* any */, SMEM_USABLE_RAM_PARTITION_TABLE, &size);
    if (!ram_ptable) {
        debug("Failed to find SMEM partition.\n");
        return -ENODEV;
    }

    /* Check validy of RAM */
    for (i = 0; i < RAM_NUM_PART_ENTRIES && j < CONFIG_NR_DRAM_BANKS && j < nbanks; i++) {
        p = &ram_ptable->parts[i];
        if (p->category != CATEGORY_SDRAM || p->type != TYPE_SYSMEM)
            continue;
        if (!p->size && !p->start)
            break;

        banks[j].start = p->start;
        banks[j].size = p->size;
        j++;
    }

    if (j == CONFIG_NR_DRAM_BANKS) {
        log_err("SMEM: More than CONFIG_NR_DRAM_BANKS (%u) entries!", CONFIG_NR_DRAM_BANKS);
    }

    qcom_sort_memory_banks(banks, j);

    return j;
}

/*
 * Look up a named region in the SMEM RAM partition table (item 402) by its
 * partition name (e.g. "MPSS_EFS", "ADSP_EFS", "TGCM").  Unlike
 * qcom_parse_memory_smem(), this does NOT filter on category/type because the
 * regions of interest are firmware carveouts, not system RAM.  Used by the
 * ACPI DSDT patcher to publish modem/ADSP/sensor memory windows to Windows.
 *
 * Returns 0 and fills @bank on success, or a negative errno if the SMEM table
 * is unavailable or no partition matches @name.
 */
int qcom_find_smem_region(const char *name, qcom_mem_bank *bank)
{
    size_t size;
    int i, ret;
    struct smem_ram_ptable *ram_ptable;
    struct smem_ram_ptn *p;
    struct udevice *dev = NULL;

    if (!name || !bank)
        return -EINVAL;

    ret = uclass_first_device_err(UCLASS_SMEM, &dev);
    if (ret)
        return ret;

    ram_ptable = smem_get(dev, -1 /* any */,
                          SMEM_USABLE_RAM_PARTITION_TABLE, &size);
    if (!ram_ptable)
        return -ENODEV;

    for (i = 0; i < RAM_NUM_PART_ENTRIES; i++) {
        p = &ram_ptable->parts[i];
        if (strncmp(p->name, name, RAM_PART_NAME_LENGTH))
            continue;

        bank->start = p->start;
        bank->size = p->size;
        return 0;
    }

    return -ENOENT;
}
