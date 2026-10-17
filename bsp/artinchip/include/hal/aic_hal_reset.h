/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_HAL_RESET_H__
#define __AIC_HAL_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_hal_reset_cmu.h"
#include "aic_reset_id.h"

int hal_reset_assert(uint32_t rst_id);
int hal_reset_deassert(uint32_t rst_id);
int hal_reset_status(uint32_t rst_id);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_HAL_RESET_H__ */
