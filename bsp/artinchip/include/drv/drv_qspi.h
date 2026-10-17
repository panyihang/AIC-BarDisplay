/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_DRV_QSPI_H_
#define _AIC_DRV_QSPI_H_
#include <hal_qspi.h>

#ifdef __cplusplus
extern "C" {
#endif

rt_err_t aic_qspi_bus_attach_device(const char *bus_name, const char *device_name,
                                    rt_uint32_t pin, rt_uint8_t data_line_width,
                                    void (*enter_qspi_mode)(), void (*exit_qspi_mode)());

#ifdef __cplusplus
}
#endif

#endif /* _AIC_DRV_QSPI_H_ */
