/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Qualcomm AOSS QMP (Always-On Subsystem Qualcomm Messaging Protocol) driver
 *
 * Provides a simple API for communicating with the AOP (Always-On Processor)
 * via the QMP MSGRAM + IPCC kick mechanism.
 *
 * Linux reference: drivers/soc/qcom/qcom_aoss.c
 */

#ifndef __SOC_QCOM_AOSS_QMP_H__
#define __SOC_QCOM_AOSS_QMP_H__

struct udevice;

/**
 * qcom_aoss_qmp_send() - Send a formatted QMP message and wait for AOP reply.
 * @dev:	The QMP device (from uclass_get_device_by_driver or DT lookup).
 * @fmt:	Printf-style format string for the QMP message body.
 * @...:	Format arguments.
 *
 * The QMP message is written to the AOSS MSGRAM, IPCC is kicked to notify AOP,
 * and the function polls for QMP_MAGIC_REPLY.
 *
 * Return: 0 on success, negative error code on timeout or failure.
 */
int qcom_aoss_qmp_send(struct udevice *dev, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

/**
 * qcom_aoss_qmp_load_state() - Toggle ADSP load_state via QMP.
 * @dev:	The QMP device.
 * @on:		true to assert load_state on, false to deassert.
 *
 * Convenience wrapper that sends:
 *   "{class: image, res: load_state, name: adsp, val: on/off}"
 *
 * Return: 0 on success, negative error code on failure.
 */
int qcom_aoss_qmp_load_state(struct udevice *dev, bool on);

#endif /* __SOC_QCOM_AOSS_QMP_H__ */
