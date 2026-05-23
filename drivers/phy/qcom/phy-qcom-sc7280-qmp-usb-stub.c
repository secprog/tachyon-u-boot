// SPDX-License-Identifier: GPL-2.0+
/*
 * SC7280 QMP USB3/DP Combo PHY Stub Driver
 *
 * This is a minimal stub to satisfy the generic DWC3 driver's requirement
 * for resolving the PHY array. The actual DP training and register
 * initialization is handled internally by the Tachyon DP driver,
 * and the USB portion defaults to USB 2.0 (or is initialized by earlier firmware).
 */

#include <dm.h>
#include <generic-phy.h>

static int qcom_sc7280_qmp_usb_stub_power_on(struct phy *phy)
{
	return 0;
}

static int qcom_sc7280_qmp_usb_stub_power_off(struct phy *phy)
{
	return 0;
}

static int qcom_sc7280_qmp_usb_stub_init(struct phy *phy)
{
	return 0;
}

static int qcom_sc7280_qmp_usb_stub_exit(struct phy *phy)
{
	return 0;
}

static struct phy_ops qcom_sc7280_qmp_usb_stub_ops = {
	.init = qcom_sc7280_qmp_usb_stub_init,
	.exit = qcom_sc7280_qmp_usb_stub_exit,
	.power_on = qcom_sc7280_qmp_usb_stub_power_on,
	.power_off = qcom_sc7280_qmp_usb_stub_power_off,
};

static const struct udevice_id qcom_sc7280_qmp_usb_stub_ids[] = {
	{ .compatible = "qcom,sc7280-qmp-usb3-dp-phy" },
	{ }
};

U_BOOT_DRIVER(qcom_sc7280_qmp_usb_stub) = {
	.name		= "qcom_sc7280_qmp_usb_stub",
	.id		= UCLASS_PHY,
	.of_match	= qcom_sc7280_qmp_usb_stub_ids,
	.ops		= &qcom_sc7280_qmp_usb_stub_ops,
};
