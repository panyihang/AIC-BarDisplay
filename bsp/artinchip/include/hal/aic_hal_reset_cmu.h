/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_HAL_CMU_RESET_H__
#define __AIC_HAL_CMU_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AIC_RESET_AUTH_REQUEST      BIT(0)
#define AIC_RESET_AUTH_LOCK         BIT(1)

struct aic_reset_signal {
    u32 offset;
    u32 bit;
    u32 flags;
};

#ifdef __cplusplus
}
#endif

#endif /* __AIC_HAL_CMU_RESET_H__ */
