// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm ADSP PAS bootstrap command
 *
 * Boots the ADSP/LPASS remote processor in isolation (no DP / PMIC-GLINK
 * consumer path).  Useful for isolating ADSP bring-up from the Type-C /
 * DisplayPort Alt-Mode stack.
 *
 * (C) Copyright 2025 Particle Industries, Inc.
 */

#include <command.h>
#include <soc/qcom/qcom_adsp_pas.h>
#include <log.h>

static int do_adsp_boot(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	int ret;

	log_warning("adsp: PAS boot start (standalone, no PMIC-GLINK)\n");
	ret = qcom_adsp_pas_boot();
	log_warning("adsp: PAS boot ret=%d\n", ret);

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	adsp, 1, 0, do_adsp_boot,
	"Boot ADSP PAS standalone (isolated from DP/PMIC-GLINK)",
	""
);
