/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_gpio.h"
#include "aic_gpio_id.h"

/* GPIO register offset */

#define GROUP_FACTOR 0x100
#define PIN_FACTOR   0x4

#define GEN_IN_STAT_REG  0x0
#define GEN_OUT_CFG_REG  0x4
#define GEN_TRQ_EN_REG   0x8
#define GEN_IRQ_STAT_REG 0xC
#define GEN_OUT_CLR_REG  0x10
#define GEN_OUT_SET_REG  0x14
#define GEN_OUT_TOG_REG  0x18
#define PIN_CFG_REG      0x80

/* PIN_CFG register field */

#define PIN_FUN_SHIFT      0
#define PIN_FUN_MASK       0xF
#define PIN_DRV_SHIFT      4
#define PIN_DRV_MASK       0x7
#define PIN_PULL_SHIFT     8
#define PIN_PULL_MASK      0x3
#define PIN_IRQ_MODE_SHIFT 12
#define PIN_IRQ_MODE_MASK  0x7
#define PIN_DIR_SHIFT      16
#define PIN_DIR_MASK       0x3
#define PIN_DIR_INPUT      0x1
#define PIN_DIR_OUTPUT     0x2
#define PIN_SPE_IE_FORCE   (0x1 << 18)
#define PIN_DEBOUNCE_SHIFT 20
#define PIN_DEBOUNCE_MASK  0xFFF

#define PIN_GEN_OE                  BIT(17)
#define PIN_GEN_IE                  BIT(16)
#define PIN_GEN_IRQ_MODE_MASK       GENMASK(14, 12)
#define PIN_GEN_IRQ_MODE_SHIFT      12
#define PIN_GEN_PIN_PULL_MASK       GENMASK(9, 8)
#define PIN_GEN_PIN_PULL_SHIFT      8
#define PIN_GEN_PIN_DRV_MASK        GENMASK(6, 4)
#define PIN_GEN_PIN_DRV_SHIFT       4
#define PIN_GEN_PIN_FUN             GENMASK(3, 0)

volatile void *gen_reg(u32 group, u32 offset)
{
#ifdef AIC_SE_GPIO_DRV
    if (group == SE_GPIO_GROUP_ID)
        return (volatile void *)(offset + SE_GPIO_BASE);
#endif

    return (volatile void *)(group * GROUP_FACTOR + offset + GPIO_BASE);
}

volatile void *cfg_reg(u32 group, u32 pin)
{
#ifdef AIC_SE_GPIO_DRV
    if (group == SE_GPIO_GROUP_ID)
        return (volatile void *)(pin * PIN_FACTOR + PIN_CFG_REG + SE_GPIO_BASE);
#endif

    return (volatile void *)(group * GROUP_FACTOR + pin * PIN_FACTOR + PIN_CFG_REG + GPIO_BASE);
}

#ifdef AIC_SE_GPIO_DRV
int hal_se_gpio_name2pin(const char *name)
{
    unsigned int pin = 0;
    int hw_port_num, hw_pin_num = 0;

    hw_port_num = (int)('I' - 'A');
    hw_pin_num = name[3] - '0';
    pin = hw_port_num * GPIO_GROUP_SIZE + hw_pin_num;

    return pin;
}
#endif

int hal_common_gpio_name2pin(const char *name)
{
    unsigned int pin = 0;
    int hw_port_num, hw_pin_num = 0;
    int i;

#ifdef AIC_GPIO_PS_ALIAS_EN
    /* D12P specific: PS.x maps to PM.x (same hardware group G=12) */
    if (name[1] == 'S')
        hw_port_num = 12;  /* PS maps to PM_GROUP */
    else
        hw_port_num = (int)(name[1] - GPIO_GROUP_BEGIN);
#else
    hw_port_num = (int)(name[1] - GPIO_GROUP_BEGIN);
#endif

    for (i = 3; i < strlen(name); i++)
    {
        hw_pin_num *= 10;
        hw_pin_num += name[i] - '0';
    }

    pin = hw_port_num * GPIO_GROUP_SIZE + hw_pin_num;

    return pin;
}

int hal_gpio_name2pin(const char *name)
{
    int name_len;

    name_len = strlen(name);
    if (name_len < 4)
    {
        return -1;
    }

    if ((name[0] == 'P') && (name[1] >= GPIO_GROUP_BEGIN) && (name[2] == '.'))
        return hal_common_gpio_name2pin(name);
#ifdef AIC_SE_GPIO_DRV
    if ((name[0] == 'S') && (name[1] == SE_GPIO_GROUP_BEGIN) && (name[2] == '.'))
        return hal_se_gpio_name2pin(name);
#endif
    return -1;
}

int hal_gpio_get_value(unsigned int group, unsigned int pin, unsigned int *pvalue)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_IN_STAT_REG));

    *pvalue = !!(val & (1 << pin));

    return 0;
}

int hal_gpio_get_outcfg(unsigned int group, unsigned int pin, unsigned int *pvalue)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_OUT_CFG_REG));

    *pvalue = !!(val & (1 << pin));

    return 0;
}

int hal_gpio_set_value(unsigned int group, unsigned int pin, unsigned int value)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_OUT_CFG_REG));

    val &= ~(1 << pin);
    val |= (!!value) << pin;

    writel(val, gen_reg(group, GEN_OUT_CFG_REG));

    return 0;
}

int hal_gpio_set_pin_value(unsigned int pin, unsigned int value)
{
    unsigned int g;
    unsigned int p;

    CHECK_PARAM(pin < (GPIO_GROUP_MAX * GPIO_GROUP_SIZE), -EINVAL);

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_value(g, p, value);

    return 0;
}

int hal_gpio_clr_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_CLR_REG));

    return 0;
}

int hal_gpio_set_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_SET_REG));

    return 0;
}

int hal_gpio_toggle_output(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_OUT_TOG_REG));

    return 0;
}

int hal_gpio_enable_irq_no_clr(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    val |= (1 << pin);

    writel(val, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}

int hal_gpio_enable_irq(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    hal_gpio_clr_irq_stat(group, pin);
    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    val |= (1 << pin);

    writel(val, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}

int hal_gpio_disable_irq(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    val &= ~(1 << pin);

    writel(val, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}

int hal_gpio_group_get_irq_en(unsigned int group, unsigned int *pen)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX, -EINVAL);

    val = readl(gen_reg(group, GEN_TRQ_EN_REG));

    *pen = val;

    return 0;
}

int hal_gpio_group_set_irq_en(unsigned int group, unsigned int en)
{
    CHECK_PARAM(group < GPIO_GROUP_MAX, -EINVAL);

    writel(en, gen_reg(group, GEN_TRQ_EN_REG));

    return 0;
}


int hal_gpio_group_get_irq_stat(unsigned int group, unsigned int *pstat)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX, -EINVAL);

    val = readl(gen_reg(group, GEN_IRQ_STAT_REG));

    *pstat = val;

    return 0;
}

int hal_gpio_group_set_irq_stat(unsigned int group, unsigned int stat)
{
    CHECK_PARAM(group < GPIO_GROUP_MAX, -EINVAL);

    writel(stat, gen_reg(group, GEN_IRQ_STAT_REG));

    return 0;
}

int hal_gpio_get_irq_stat(unsigned int group, unsigned int pin, unsigned int *pstat)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(gen_reg(group, GEN_IRQ_STAT_REG));

    *pstat = !!(val & (1 << pin));

    return 0;
}

int hal_gpio_clr_irq_stat(unsigned int group, unsigned int pin)
{
    unsigned int val;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = 1 << pin;

    writel(val, gen_reg(group, GEN_IRQ_STAT_REG));

    return 0;
}

int hal_gpio_set_func(unsigned int group, unsigned int pin, unsigned int func)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_FUN_MASK << PIN_FUN_SHIFT);
    val |= (func & PIN_FUN_MASK) << PIN_FUN_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_get_func(unsigned int group, unsigned int pin, unsigned int *pfunc)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    *pfunc = (val & PIN_FUN_MASK) >> PIN_FUN_SHIFT;

    return 0;
}

int hal_gpio_set_drive_strength(unsigned int group, unsigned int pin, unsigned int strength)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DRV_MASK << PIN_DRV_SHIFT);
    val |= (strength & PIN_DRV_MASK) << PIN_DRV_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_bias_pull(unsigned int group, unsigned int pin, unsigned int pull)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_PULL_MASK << PIN_PULL_SHIFT);
    val |= (pull & PIN_PULL_MASK) << PIN_PULL_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_irq_mode(unsigned int group, unsigned int pin, unsigned int irq_mode)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_IRQ_MODE_MASK << PIN_IRQ_MODE_SHIFT);
    val |= (irq_mode & PIN_IRQ_MODE_MASK) << PIN_IRQ_MODE_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_direction_input(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DIR_MASK << PIN_DIR_SHIFT);
    val |= PIN_DIR_INPUT << PIN_DIR_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_direction_output(unsigned int group, unsigned int pin)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~(PIN_DIR_MASK << PIN_DIR_SHIFT);
    val |= PIN_DIR_OUTPUT << PIN_DIR_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_set_debounce(unsigned int group, unsigned int pin, unsigned int debounce)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);

    val = readl(cfg_reg(group, pin));

    val &= ~((u32)PIN_DEBOUNCE_MASK << PIN_DEBOUNCE_SHIFT);
    val |= (debounce & PIN_DEBOUNCE_MASK) << PIN_DEBOUNCE_SHIFT;

    writel(val, cfg_reg(group, pin));

    return 0;
}

int hal_gpio_cfg(struct gpio_cfg *cfg, u32 cnt)
{
    u32 i = 0;

    for (i = 0; i < cnt; i++, cfg++) {
        if (!cfg) {
            //hal_log_err("%d gpio_cfg is NULL", i);
            return -1;
        }

        hal_gpio_set_func(cfg->port, cfg->pin, cfg->func);
        hal_gpio_set_drive_strength(cfg->port, cfg->pin, cfg->driver);
        hal_gpio_set_bias_pull(cfg->port, cfg->pin, cfg->pull);
    }
    return 0;
}

int hal_gpio_get_pincfg(unsigned int group, unsigned int pin, int check_type)
{
    unsigned int val = 0;
    unsigned int pincfg_val = 0;

    pincfg_val = readl(cfg_reg(group, pin));

    switch (check_type) {
    case GPIO_CHECK_PIN_GEN_OE:
        if (!(pincfg_val & PIN_GEN_OE))
            return -1;
        break;
    case GPIO_CHECK_PIN_GEN_IE:
        if (!(pincfg_val & PIN_GEN_IE))
            return -1;
        break;
    case GPIO_CHECK_PIN_FUN:
        val = PIN_GEN_PIN_FUN & pincfg_val;
        break;
    case GPIO_CHECK_GEN_IRQ_MODE:
        val = (PIN_GEN_IRQ_MODE_MASK & pincfg_val) >> PIN_GEN_IRQ_MODE_SHIFT;
        break;
    case GPIO_CHECK_PIN_GEN_PIN_DRV:
        val = (PIN_GEN_PIN_DRV_MASK & pincfg_val) >> PIN_GEN_PIN_DRV_SHIFT;
        break;
    case GPIO_CHECK_PIN_GEN_PULL:
        val = (PIN_GEN_PIN_PULL_MASK & pincfg_val) >> PIN_GEN_PIN_PULL_SHIFT;
        break;
    default:
        pr_info("CHECK TYPE: Unknown(%d)\n", check_type);
        break;
    }

    return val;
}

/* Get drive strength of a GPIO pin */
int hal_gpio_get_drive_strength(unsigned int group, unsigned int pin, unsigned int *pstrength)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);
    CHECK_PARAM(pstrength != NULL, -EINVAL);

    val = readl(cfg_reg(group, pin));
    *pstrength = (val & PIN_DRV_MASK) >> PIN_DRV_SHIFT;

    return 0;
}

/* Get irq mode of a GPIO pin */
/* TODO: IRQ - may be removed later */
int hal_gpio_get_irq_mode(unsigned int group, unsigned int pin, unsigned int *pirq_mode)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);
    CHECK_PARAM(pirq_mode != NULL, -EINVAL);

    val = readl(cfg_reg(group, pin));
    *pirq_mode = (val & PIN_IRQ_MODE_MASK) >> PIN_IRQ_MODE_SHIFT;

    return 0;
}

/* Get debounce value of a GPIO pin */
int hal_gpio_get_debounce(unsigned int group, unsigned int pin, unsigned int *pdebounce)
{
    unsigned int val = 0;

    CHECK_PARAM(group < GPIO_GROUP_MAX && pin < 32, -EINVAL);
    CHECK_PARAM(pdebounce != NULL, -EINVAL);

    val = readl(cfg_reg(group, pin));
    *pdebounce = (val >> PIN_DEBOUNCE_SHIFT) & PIN_DEBOUNCE_MASK;

    return 0;
}
