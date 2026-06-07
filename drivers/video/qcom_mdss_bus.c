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
	/*
	 * Probing a child (e.g. the DP controller) probes this parent bus too,
	 * and the DM core then auto-enables the MDSS node's power-domain + clocks.
	 * The DP controller (aux/link/ahb @ 0xae90000) lives UNDER the MDSS power
	 * domain, so this auto-power MUST happen or its AUX block reads back all
	 * zeros (writes don't stick) and AUX never works.
	 *
	 * NOTE: this powers MDSS at boot, which wedges Linux's MDSS/GPU IOMMU
	 * bring-up at OS handoff (msm-mdss -EINVAL, adreno get_pages -28).  That
	 * is the documented trade for a U-Boot-DP build; for a clean Linux-DP
	 * handoff, disable the DP path entirely in the defconfig instead.
	 */
	.flags		= DM_FLAG_PRE_RELOC,
};
