// SPDX-License-Identifier: GPL-2.0+
/*
 * OF_LIVE devicetree fixup.
 *
 * This file implements runtime fixups for Qualcomm DT to improve
 * compatibility with U-Boot. This includes adjusting the USB nodes
 * to only use USB high-speed, as well as remapping volume buttons
 * to behave as up/down for navigating U-Boot.
 *
 * We use OF_LIVE for this rather than early FDT fixup for a couple
 * of reasons: it has a much nicer API, is most likely more efficient,
 * and our changes are only applied to U-Boot. This allows us to use a
 * DT designed for Linux, run U-Boot with a modified version, and then
 * boot Linux with the original FDT.
 *
 * Copyright (c) 2024 Linaro Ltd.
 *   Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#define pr_fmt(fmt) "of_fixup: " fmt

#include <dt-bindings/input/linux-event-codes.h>
#include <dm/device.h>
#include <dm/of_access.h>
#include <dm/of.h>
#include <fdt_support.h>
#include <linker_lists.h>
#include <linux/errno.h>
#include <stdlib.h>
#include <time.h>

/*
 * has_driver_for_node() - is a PHY driver compiled in that can bind to @np?
 *
 * Matches @np's "compatible" strings against the of_match table of every
 * UCLASS_PHY driver in the linker list. Used to decide whether U-Boot can
 * actually drive a controller's SuperSpeed PHY.
 */
static bool has_driver_for_node(const struct device_node *np)
{
	struct driver *drv = ll_entry_start(struct driver, driver);
	const int n_drv = ll_entry_count(struct driver, driver);
	const char *compat;
	int i, idx;

	for (idx = 0; !of_property_read_string_index(np, "compatible", idx,
						     &compat); idx++) {
		for (i = 0; i < n_drv; i++) {
			const struct udevice_id *id;

			if (drv[i].id != UCLASS_PHY || !drv[i].of_match)
				continue;
			for (id = drv[i].of_match; id->compatible; id++)
				if (!strcmp(id->compatible, compat))
					return true;
		}
	}

	return false;
}

/*
 * find_ssphy_node() - resolve the SuperSpeed (usb3) PHY node of a DWC3 node.
 *
 * Walks "phy-names" for the "usb3-phy"/"usb3_phy" entry and returns the
 * matching "phys" phandle target. Uses the *-with-args parser because the
 * phys list mixes a 0-cell HS PHY and a 1-cell QMP combo PHY.
 */
static struct device_node *find_ssphy_node(struct device_node *dwc3)
{
	struct of_phandle_args args;
	const char *name;
	int i;

	for (i = 0; !of_property_read_string_index(dwc3, "phy-names", i, &name);
	     i++) {
		if (strcmp(name, "usb3-phy") && strcmp(name, "usb3_phy"))
			continue;
		if (of_parse_phandle_with_args(dwc3, "phys", "#phy-cells", 0, i,
					       &args))
			return NULL;
		return args.np;
	}

	return NULL;
}

/*
 * U-Boot historically supported only USB high-speed on Qualcomm DWC3
 * controllers, so this fixup strips the SuperSpeed PHY and forces
 * maximum-speed=high-speed (keeping a Linux DT usable). Backport of upstream's
 * auto-detection: if U-Boot has a PHY driver for the controller's SuperSpeed
 * PHY, keep SuperSpeed and skip the downgrade. On the Tachyon this keeps
 * usb_1's QMP combo PHY ("qcom,sc7280-qmp-usb3-dp-phy") so a USB stick on the
 * USB-C dock enumerates. (Also needs the GCC USB3 pipe-clk mux re-parent in
 * clock-sc7280.c and the SS connector graph in the board DTS.)
 */
static int fixup_qcom_dwc3(struct device_node *glue_np)
{
	struct device_node *dwc3, *ssphy_np;
	int ret, len, hsphy_idx = 1;
	const __be32 *phandles;
	const char *second_phy_name;

	debug("Fixing up %s\n", glue_np->name);

	/* Find the DWC3 node itself */
	dwc3 = of_find_compatible_node(glue_np, NULL, "snps,dwc3");
	if (!dwc3) {
		if (!of_get_property(glue_np, "phys", NULL) ||
		    !of_get_property(glue_np, "phy-names", NULL)) {
			log_err("Failed to find dwc3 node\n");
			return -ENOENT;
		}

		log_debug("Using flattened DWC3 node %s\n", glue_np->name);
		dwc3 = glue_np;
	}

	/* Keep SuperSpeed if U-Boot has a driver for the USB3 PHY. */
	ssphy_np = find_ssphy_node(dwc3);
	if (ssphy_np && has_driver_for_node(ssphy_np)) {
		/*
		 * Keep SuperSpeed. usb_1 is dr_mode="otg"; it binds as a host
		 * via the OTG->host default in dwc3_glue_bind_common() (this
		 * fixup runs after dm_scan, so a dr_mode rewrite here would be
		 * too late to affect binding anyway).
		 */
		return 0;
	}

	/* Tell the glue driver to configure the wrapper for high-speed only operation */
	ret = of_write_prop(glue_np, "qcom,select-utmi-as-pipe-clk", 0, NULL);
	if (ret) {
		log_err("Failed to add property 'qcom,select-utmi-as-pipe-clk': %d\n", ret);
		return ret;
	}

	phandles = of_get_property(dwc3, "phys", &len);
	len /= sizeof(*phandles);
	if (len == 1) {
		log_debug("Only one phy, not a superspeed controller\n");
		return 0;
	}

	/* Figure out if the superspeed phy is present and if so then which phy is it? */
	ret = of_property_read_string_index(dwc3, "phy-names", 1, &second_phy_name);
	if (ret == -ENODATA) {
		log_debug("Only one phy, not a super-speed controller\n");
		return 0;
	} else if (ret) {
		log_err("Failed to read second phy name: %d\n", ret);
		return ret;
	}

	if (!strncmp("usb3-phy", second_phy_name, strlen("usb3-phy"))) {
		log_debug("Second phy isn't superspeed (is '%s') assuming first phy is SS\n",
			  second_phy_name);
		hsphy_idx = 0;
	}

	/* Overwrite the "phys" property to only contain the high-speed phy */
	ret = of_write_prop(dwc3, "phys", sizeof(*phandles), phandles + hsphy_idx);
	if (ret) {
		log_err("Failed to overwrite 'phys' property: %d\n", ret);
		return ret;
	}

	/* Overwrite "phy-names" to only contain a single entry */
	ret = of_write_prop(dwc3, "phy-names", strlen("usb2-phy"), "usb2-phy");
	if (ret) {
		log_err("Failed to overwrite 'phy-names' property: %d\n", ret);
		return ret;
	}

	ret = of_write_prop(dwc3, "maximum-speed", strlen("high-speed"), "high-speed");
	if (ret) {
		log_err("Failed to set 'maximum-speed' property: %d\n", ret);
		return ret;
	}

	return 0;
}

static void fixup_usb_nodes(void)
{
	struct device_node *glue_np = NULL;
	int ret;

	while ((glue_np = of_find_compatible_node(glue_np, NULL, "qcom,dwc3"))) {
		ret = fixup_qcom_dwc3(glue_np);
		if (ret)
			log_warning("Failed to fixup node %s: %d\n", glue_np->name, ret);
	}
}

/* Remove all references to the rpmhpd device */
static void fixup_power_domains(void)
{
	struct device_node *pd = NULL, *np = NULL;
	struct property *prop;
	const __be32 *val;

	if (IS_ENABLED(CONFIG_QCOM_RPMH_POWER_DOMAIN))
		return;

	/* All Qualcomm platforms name the rpm(h)pd "power-controller" */
	for_each_of_allnodes(pd) {
		if (pd->name && !strcmp("power-controller", pd->name))
			break;
	}

	/* Sanity check that this is indeed a power domain controller */
	if (!of_find_property(pd, "#power-domain-cells", NULL)) {
		log_err("Found power-controller but it doesn't have #power-domain-cells\n");
		return;
	}

	/* Remove all references to the power domain controller */
	for_each_of_allnodes(np) {
		if (!(prop = of_find_property(np, "power-domains", NULL)))
			continue;

		val = prop->value;
		if (val[0] == cpu_to_fdt32(pd->phandle))
			of_remove_property(np, prop);
	}
}

#define time_call(func, ...) \
	do { \
		u64 start = timer_get_us(); \
		func(__VA_ARGS__); \
		debug(#func " took %lluus\n", timer_get_us() - start); \
	} while (0)

void qcom_of_fixup_nodes(void)
{
	time_call(fixup_usb_nodes);
	time_call(fixup_power_domains);
}

int ft_board_setup(void *blob, struct bd_info __maybe_unused *bd)
{
	struct fdt_header *fdt = blob;
	int node;

	/* We only want to do this fix-up for the RB1 board, quick return for all others */
	if (!fdt_node_check_compatible(fdt, 0, "qcom,qrb4210-rb2"))
		return 0;

	fdt_for_each_node_by_compatible(node, blob, 0, "snps,dwc3") {
		log_debug("%s: Setting 'dr_mode' to OTG\n", fdt_get_name(blob, node, NULL));
		fdt_setprop_string(fdt, node, "dr_mode", "otg");
		break;
	}

	return 0;
}
