/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_HAL_GPIO_H__
#define __AIC_HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_common.h"

#include "aic_gpio_id.h"

/* drive-strength */

#define PIN_DRV_33V_180_OHM 0
#define PIN_DRV_33V_90_OHM  1
#define PIN_DRV_33V_60_OHM  2
#define PIN_DRV_33V_45_OHM  3
#define PIN_DRV_33V_36_OHM  4
#define PIN_DRV_33V_30_OHM  5
#define PIN_DRV_33V_26_OHM  6
#define PIN_DRV_33V_23_OHM  7

#define PIN_DRV_18V_300_OHM 0
#define PIN_DRV_18V_150_OHM 1
#define PIN_DRV_18V_100_OHM 2
#define PIN_DRV_18V_75_OHM  3
#define PIN_DRV_18V_60_OHM  4
#define PIN_DRV_18V_50_OHM  5
#define PIN_DRV_18V_43_OHM  6
#define PIN_DRV_18V_38_OHM  7

/* bias-pull-down/up */

#define PIN_PULL_DIS  0
#define PIN_PULL_DOWN 2
#define PIN_PULL_UP   3

/* irq type */

#define PIN_IRQ_MODE_EDGE_FALLING 0
#define PIN_IRQ_MODE_EDGE_RISING  1
#define PIN_IRQ_MODE_LEVEL_LOW    2
#define PIN_IRQ_MODE_LEVEL_HIGH   3
#define PIN_IRQ_MODE_EDGE_BOTH    4

enum gpio_check_pincfg_type {
    GPIO_CHECK_PIN_FUN = 0,
    GPIO_CHECK_PIN_GEN_OE = 1,
    GPIO_CHECK_PIN_GEN_IE = 2,
    GPIO_CHECK_GEN_IRQ_MODE = 3,
    GPIO_CHECK_PIN_GEN_PIN_DRV = 4,
    GPIO_CHECK_PIN_GEN_PULL = 5
};

#define GPIO_GROUP_SIZE               32
#define GPIO_GROUP(pin_name)          ((pin_name) / GPIO_GROUP_SIZE)
#define GPIO_GROUP_PIN(pin_name)      ((pin_name) % GPIO_GROUP_SIZE)
#define GPIO_PIN_NAME(_g, _offset)    (((_g) * GPIO_GROUP_SIZE) + (_offset))

struct gpio_cfg {
    uint8_t port;
    uint8_t pin;
    uint8_t func;
    uint8_t pull;
    uint8_t driver;
};
#define AIC_PINMUX_BASE(port, pin, func, up, drv) \
                        {port - 'A', pin, func, up, drv}
#define AIC_PINMUX(port, pin, func) \
                        AIC_PINMUX_BASE(port, pin, func, PIN_PULL_DIS, 3)
#define AIC_PINMUX_UP(port, pin, func) \
                        AIC_PINMUX_BASE(port, pin, func, PIN_PULL_UP, 3)
#define AIC_PINMUX_DOWN(port, pin, func) \
                        AIC_PINMUX_BASE(port, pin, func, PIN_PULL_DOWN, 3)

int hal_gpio_name2pin(const char *name);
int hal_gpio_get_value(unsigned int group, unsigned int pin, unsigned int *pvalue);
int hal_gpio_set_value(unsigned int group, unsigned int pin, unsigned int value);
int hal_gpio_set_pin_value(unsigned int pin, unsigned int value);
int hal_gpio_clr_output(unsigned int group, unsigned int pin);
int hal_gpio_set_output(unsigned int group, unsigned int pin);
int hal_gpio_toggle_output(unsigned int group, unsigned int pin);
int hal_gpio_enable_irq_no_clr(unsigned int group, unsigned int pin);
int hal_gpio_enable_irq(unsigned int group, unsigned int pin);
int hal_gpio_disable_irq(unsigned int group, unsigned int pin);
int hal_gpio_group_get_irq_en(unsigned int group, unsigned int *pen);
int hal_gpio_group_set_irq_en(unsigned int group, unsigned int en);
int hal_gpio_group_get_irq_stat(unsigned int group, unsigned int *pstat);
int hal_gpio_group_set_irq_stat(unsigned int group, unsigned int stat);
int hal_gpio_get_irq_stat(unsigned int group, unsigned int pin, unsigned int *pstat);
int hal_gpio_clr_irq_stat(unsigned int group, unsigned int pin);
int hal_gpio_set_func(unsigned int group, unsigned int pin, unsigned int func);
int hal_gpio_get_func(unsigned int group, unsigned int pin, unsigned int *pfunc);
int hal_gpio_set_drive_strength(unsigned int group, unsigned int pin, unsigned int strength);
int hal_gpio_set_bias_pull(unsigned int group, unsigned int pin, unsigned int pull);
int hal_gpio_set_irq_mode(unsigned int group, unsigned int pin, unsigned int irq_mode);
int hal_gpio_direction_input(unsigned int group, unsigned int pin);
int hal_gpio_direction_output(unsigned int group, unsigned int pin);
int hal_gpio_set_debounce(unsigned int group, unsigned int pin, unsigned int debounce);

int hal_gpio_get_drive_strength(unsigned int group, unsigned int pin, unsigned int *pstrength);
int hal_gpio_get_irq_mode(unsigned int group, unsigned int pin, unsigned int *pirq_mode);
int hal_gpio_get_debounce(unsigned int group, unsigned int pin, unsigned int *pdebounce);

int hal_gpio_cfg(struct gpio_cfg *cfg, u32 cnt);
int hal_gpio_get_pincfg(unsigned int group, unsigned int pin, int check_type);
int hal_gpio_get_outcfg(unsigned int group, unsigned int pin, unsigned int *pvalue);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_HAL_GPIO_H__ */
