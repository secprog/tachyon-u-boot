// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm MDSS display-subsystem bus shim for U-Boot.
 *
 * The MDSS node in the Linux device tree uses "qcom,sc7280-mdss" which
 * U-Boot's simple-bus binding does not recognise by default.  This tiny
 * driver binds the MDSS node as a UCLASS_SIMPLE_BUS so that child
 * devices (DSI, DP, PHYs, etc.) are probed automatically without
 * modifying the Linux-visible compatible string.
 */

#include <dm.h>

static const struct udevice_id qcom_mdss_bus_ids[] = {
	{ .compatible = "qcom,sc7280-mdss" },
	{ }
};

U_BOOT_DRIVER(qcom_mdss_bus) = {
	.name		= "qcom_mdss_bus",
	.id		= UCLASS_SIMPLE_BUS,
	.of_match	= qcom_mdss_bus_ids,
	.flags		= DM_FLAG_PRE_RELOC,
};
