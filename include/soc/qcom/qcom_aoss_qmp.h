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

#include <dm/ofnode.h>

struct udevice;

/**
 * qcom_aoss_qmp_send() - Send a formatted QMP message and wait for AOP reply.
 * @dev:	The QMP device (from DT phandle lookup).
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
 * qcom_aoss_qmp_load_state() - Toggle a remote processor load_state via QMP.
 * @dev:	The QMP device.
 * @name:	Processor name (e.g. "adsp", "cdsp", "slpi").
 *		Corresponds to the per-SoC remoteproc load_state string
 *		that Linux's qcom_q6v5_pas uses.
 * @on:		true to assert load_state on, false to deassert.
 *
 * Sends: "{class: image, res: load_state, name: @name, val: on/off}"
 *
 * Return: 0 on success, negative error code on failure.
 */
int qcom_aoss_qmp_load_state(struct udevice *dev, const char *name, bool on);

/**
 * qcom_aoss_qmp_get_by_node() - Resolve AOSS QMP device from remoteproc DT node.
 * @rproc:	ofnode of the remoteproc node (e.g. adsp-pas, cdsp-pas).
 *		The node should have a qcom,qmp = <&aoss_qmp>; phandle.
 * @devp:	Returns the probed QMP device.
 *
 * Return: 0 on success, negative error code on failure.
 */
int qcom_aoss_qmp_get_by_node(ofnode rproc, struct udevice **devp);

#endif /* __SOC_QCOM_AOSS_QMP_H__ */
