/*
 * Copyright (c) 2022-2026, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_DRV_GPIO_H__
#define __AIC_DRV_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_hal_gpio.h"

void drv_pin_bias_set(unsigned int pin, unsigned int pull);
void drv_pin_drive_set(unsigned int pin, unsigned int strength);
void drv_pin_mux_set(unsigned int pin, unsigned int func);
unsigned int drv_pin_mux_get(unsigned int pin);
long drv_pin_get(const char *name);
int drv_pin_init(void);

#ifdef RT_USING_PM
int gpio_pm_register(rt_base_t pin, void (*restore_fn)(void *data), void *data);
void gpio_pm_unregister(rt_base_t pin);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __AIC_LL_GPIO_H__ */
