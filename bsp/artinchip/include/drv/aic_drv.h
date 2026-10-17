/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_DRV_H_
#define _ARTINCHIP_AIC_DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_hal.h>
#include "aic_drv_irq.h"
#include "aic_drv_gpio.h"
#include "aic_drv_uart.h"
#ifdef AIC_GE_DRV
#include "aic_drv_ge.h"
#endif
#include "drv_qspi.h"
#include "drv_efuse.h"
#ifdef AIC_DMA_DRV
#include "drv_dma.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_DRV_H_ */
