/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SOC_QCOM_ADSP_PAS_H
#define __SOC_QCOM_ADSP_PAS_H

#if CONFIG_IS_ENABLED(QCOM_ADSP_PAS)
int qcom_adsp_pas_boot(void);
#else
static inline int qcom_adsp_pas_boot(void)
{
	return 0;
}
#endif

#endif /* __SOC_QCOM_ADSP_PAS_H */
