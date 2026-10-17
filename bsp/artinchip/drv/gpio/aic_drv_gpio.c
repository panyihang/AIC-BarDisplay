/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_gpio.h"
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#endif
#include "aic_utils.h"
#ifdef RT_USING_PM
#include <drivers/pm.h>
#endif

#ifdef RT_USING_PIN

#ifdef RT_USING_PM
/* GPIO PM state save/restore for deep sleep */
#define GPIO_PM_MAX_ENTRIES  16

struct gpio_pm_state {
    rt_base_t pin;
    uint8_t func;
    uint8_t direction;
    uint8_t pull;
    uint8_t drive;

    uint8_t irq_mode;
    uint8_t irq_enabled;
    uint16_t debounce_val;
};

struct gpio_pm_entry {
    struct gpio_pm_state state;
    void (*restore_fn)(void *data);
    void *data;
    uint8_t valid;
};

static struct gpio_pm_entry gpio_pm_table[GPIO_PM_MAX_ENTRIES];
static int gpio_pm_count = 0;

/* Register a GPIO for PM save/restore */
int gpio_pm_register(rt_base_t pin, void (*restore_fn)(void *data), void *data)
{
    int i;
    unsigned int g, p;
    unsigned int temp_val;

    if (gpio_pm_count >= GPIO_PM_MAX_ENTRIES) {
        rt_kprintf("GPIO PM: table full, cannot register pin %ld\n", pin);
        return -RT_ENOMEM;
    }

    /* Check if already registered */
    for (i = 0; i < gpio_pm_count; i++) {
        if (gpio_pm_table[i].valid && gpio_pm_table[i].state.pin == pin) {
            rt_kprintf("GPIO PM: pin %ld already registered\n", pin);
            return -RT_EBUSY;
        }
    }

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    /* Save current GPIO configuration */
    gpio_pm_table[gpio_pm_count].state.pin = pin;

    /* Get function select */
    hal_gpio_get_func(g, p, &temp_val);
    gpio_pm_table[gpio_pm_count].state.func = (uint8_t)temp_val;

    /* Get pull configuration */
    gpio_pm_table[gpio_pm_count].state.pull = (uint8_t)hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_PULL);

    /* Get drive strength */
    hal_gpio_get_drive_strength(g, p, &temp_val);
    gpio_pm_table[gpio_pm_count].state.drive = (uint8_t)temp_val;

    /* Get irq mode */
    gpio_pm_table[gpio_pm_count].state.irq_mode = (uint8_t)hal_gpio_get_pincfg(g, p, GPIO_CHECK_GEN_IRQ_MODE);

    /* Get direction */
    temp_val = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
    gpio_pm_table[gpio_pm_count].state.direction = (temp_val == 0) ? 2 : 1;

    /* Get irq enable status */
    temp_val = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_IE);
    gpio_pm_table[gpio_pm_count].state.irq_enabled = (temp_val == 1) ? 1 : 0;

    /* Get debounce value */
    hal_gpio_get_debounce(g, p, &temp_val);
    gpio_pm_table[gpio_pm_count].state.debounce_val = (uint16_t)temp_val;

    /* Save restore callback */
    gpio_pm_table[gpio_pm_count].restore_fn = restore_fn;
    gpio_pm_table[gpio_pm_count].data = data;
    gpio_pm_table[gpio_pm_count].valid = 1;

    gpio_pm_count++;

    rt_kprintf("GPIO PM: registered pin %ld (group=%d, pin=%d)\n", pin, g, p);
    return RT_EOK;
}

/* Unregister a GPIO from PM save/restore */
void gpio_pm_unregister(rt_base_t pin)
{
    int i;

    for (i = 0; i < gpio_pm_count; i++) {
        if (gpio_pm_table[i].valid && gpio_pm_table[i].state.pin == pin) {
            gpio_pm_table[i].valid = 0;
            rt_kprintf("GPIO PM: unregistered pin %ld\n", pin);
            return;
        }
    }
}

/* Save all registered GPIO states before deep sleep */
static void gpio_pm_save_states(void)
{
    int i;

    pr_debug("Registered GPIO count: %d\n", gpio_pm_count);

    for (i = 0; i < gpio_pm_count; i++) {
        if (!gpio_pm_table[i].valid)
            continue;

        pr_debug("  Pin %ld: func=%d, dir=%d, pull=%d, drive=%d",
                   gpio_pm_table[i].state.pin,
                   gpio_pm_table[i].state.func,
                   gpio_pm_table[i].state.direction,
                   gpio_pm_table[i].state.pull,
                   gpio_pm_table[i].state.drive);

        pr_debug(", irq_mode=%d, irq_en=%d", gpio_pm_table[i].state.irq_mode, gpio_pm_table[i].state.irq_enabled);

        pr_debug(", debounce=%d\n", gpio_pm_table[i].state.debounce_val);
    }
}

/* Restore all registered GPIO states after deep sleep wakeup */
static void gpio_pm_restore_states(void)
{
    int i;
    unsigned int g, p;

    pr_debug("Registered GPIO count: %d\n", gpio_pm_count);

    for (i = 0; i < gpio_pm_count; i++) {
        if (!gpio_pm_table[i].valid)
            continue;

        g = GPIO_GROUP(gpio_pm_table[i].state.pin);
        p = GPIO_GROUP_PIN(gpio_pm_table[i].state.pin);

        /* Restore function select */
        hal_gpio_set_func(g, p, gpio_pm_table[i].state.func);

        /* Restore drive strength */
        hal_gpio_set_drive_strength(g, p, gpio_pm_table[i].state.drive);

        /* Restore pull configuration */
        hal_gpio_set_bias_pull(g, p, gpio_pm_table[i].state.pull);

        /* Restore debounce */
        hal_gpio_set_debounce(g, p, gpio_pm_table[i].state.debounce_val);

        /* Restore irq mode */
        hal_gpio_set_irq_mode(g, p, gpio_pm_table[i].state.irq_mode);

        /* Restore direction */
        if (gpio_pm_table[i].state.direction == 2) {
            hal_gpio_direction_output(g, p);
        } else {  /* Input */
            hal_gpio_direction_input(g, p);
        }

        /* Restore output value if direction is output */
        if (gpio_pm_table[i].state.direction == 2) {
            unsigned int out_val;
            hal_gpio_get_outcfg(g, p, &out_val);
            if (out_val) {
                hal_gpio_set_output(g, p);
            } else {
                hal_gpio_clr_output(g, p);
            }
        }

        /* Re-enable IRQ if it was enabled */
        if (gpio_pm_table[i].state.irq_enabled)
        {
            pr_debug("    Re-enabling IRQ...\n");
            hal_gpio_enable_irq(g, p);
        }

        /* Call custom restore function if provided */
        if (gpio_pm_table[i].restore_fn)
        {
            gpio_pm_table[i].restore_fn(gpio_pm_table[i].data);
        }

        pr_debug("  Restored pin %ld\n", gpio_pm_table[i].state.pin);
    }
}
#endif

void drv_pin_bias_set(unsigned int pin, unsigned int pull)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_bias_pull(g, p, pull);
}

void drv_pin_drive_set(unsigned int pin, unsigned int strength)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_drive_strength(g, p, strength);
}

void drv_pin_mux_set(unsigned int pin, unsigned int func)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, func);
}

unsigned int drv_pin_mux_get(unsigned int pin)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int func;

    hal_gpio_get_func(g, p, &func);

    return func;
}

void drv_pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    hal_gpio_set_func(g, p, 1);
    switch (mode)
    {
    case PIN_MODE_INPUT:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_INPUT_PULLUP:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DOWN);
        hal_gpio_direction_input(g, p);
        break;
    case PIN_MODE_OUTPUT:
    case PIN_MODE_OUTPUT_OD:
    default:
        hal_gpio_set_bias_pull(g, p, PIN_PULL_DIS);
        hal_gpio_direction_output(g, p);
        break;
    }
}

void drv_pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    if (PIN_LOW == value)
    {
        hal_gpio_clr_output(g, p);
    }
    else
    {
        int ret;

        hal_gpio_set_output(g, p);
        ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_OE);
        if (ret < 0) {
            pr_err("Set the output pin failed\n");
        } else {
            pr_debug("Set the output pin successfully\n");
        }
    }
}

int drv_pin_read(struct rt_device *device, rt_base_t pin)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int value = PIN_LOW;

    hal_gpio_get_value(g, p, &value);

    return value;
}

#ifdef AIC_GPIO_IRQ_DRV_EN
rt_err_t drv_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
                             rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);
    unsigned int irq_mode = 0;

    switch (mode)
    {
    case PIN_IRQ_MODE_RISING:
        irq_mode=PIN_IRQ_MODE_EDGE_RISING;
        break;
    case PIN_IRQ_MODE_FALLING:
        irq_mode=PIN_IRQ_MODE_EDGE_FALLING;
        break;
    case PIN_IRQ_MODE_RISING_FALLING:
        irq_mode=PIN_IRQ_MODE_EDGE_BOTH;
        break;
    case PIN_IRQ_MODE_HIGH_LEVEL:
        irq_mode=PIN_IRQ_MODE_LEVEL_HIGH;
        break;
    case PIN_IRQ_MODE_LOW_LEVEL:
        irq_mode=PIN_IRQ_MODE_LEVEL_LOW;
        break;
    }
    hal_gpio_set_irq_mode(g, p, irq_mode);

    aicos_request_irq(AIC_GPIO_TO_IRQ(pin), (irq_handler_t)hdr, 0, "pin", args);

    return RT_EOK;
}

rt_err_t drv_pin_detach_irq(struct rt_device *device, rt_int32_t pin)
{
    return RT_EOK;
}

extern void * g_irqvector[];
irqreturn_t drv_gpio_group_irqhandler(int irq, void * data)
{
    unsigned int g = irq - GPIO_IRQn;
    unsigned int stat = 0;
    unsigned int mask = 0;
    unsigned int i = 0;
    unsigned int gpio_irq = 0;

    hal_gpio_group_get_irq_stat(g, &stat);
    hal_gpio_group_get_irq_en(g, &mask);
    stat &= mask;

    for (i=0; i<32; i++) {
        if (!(stat & (1U<<i)))
            continue;

        gpio_irq = AIC_GPIO_TO_IRQ(g*GPIO_GROUP_SIZE + i);
        drv_irq_call_isr(gpio_irq);
        if (g_irqvector[gpio_irq])
            hal_gpio_group_set_irq_stat(g, (1U<<i));
    }

    return IRQ_HANDLED;
}

unsigned int pin_group_irq_en = 0;
rt_err_t drv_pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint32_t enabled)
{
    unsigned int g = GPIO_GROUP(pin);
    unsigned int p = GPIO_GROUP_PIN(pin);

    if (enabled) {
        int ret;

        if (!(pin_group_irq_en & (1<<g))) {
            aicos_request_irq(GPIO_IRQn + g, drv_gpio_group_irqhandler, 0, "pin_group", NULL);
            aicos_irq_enable(GPIO_IRQn + g);
            pin_group_irq_en |= (1<<g);
        }
        hal_gpio_enable_irq(g, p);
        ret = hal_gpio_get_pincfg(g, p, GPIO_CHECK_PIN_GEN_IE);
        if (ret < 0) {
            pr_err("Set the input pin failed\n");
        } else {
            pr_debug("Set the input pin successfully\n");
        }

    } else {
        hal_gpio_disable_irq(g, p);
    }
    return RT_EOK;
}
#endif

rt_base_t drv_pin_get(const char *name)
{
    return hal_gpio_name2pin(name);
}

static const struct rt_pin_ops _drv_pin_ops =
{
    drv_pin_mode,
    drv_pin_write,
    drv_pin_read,
#ifdef AIC_GPIO_IRQ_DRV_EN
    drv_pin_attach_irq,
    drv_pin_detach_irq,
    drv_pin_irq_enable,
#else
    RT_NULL,
    RT_NULL,
    RT_NULL,
#endif
    drv_pin_get,
};

#ifdef RT_USING_PM
static int drv_pin_suspend_late(const struct rt_device *dev, rt_uint8_t pm_mode)
{
    switch (pm_mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
        /* do nothing */
        break;
    case PM_SLEEP_MODE_DEEP:
        /* save dynamic GPIO states */
        gpio_pm_save_states();
        break;
    default:
        break;
    }
    return RT_EOK;
}

static void drv_pin_resume_early(const struct rt_device *dev, rt_uint8_t pm_mode)
{
    switch (pm_mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
        /* do nothing */
        break;
    case PM_SLEEP_MODE_DEEP:
        /* restore dynamic GPIO states */
        gpio_pm_restore_states();
        break;
    default:
        break;
    }
}

static const struct rt_device_pm_ops drv_pin_pm_ops =
{
    SET_LATE_DEVICE_PM_OPS(drv_pin_suspend_late, drv_pin_resume_early)
    NULL,
};
#endif

int drv_pin_init(void)
{
    int ret = RT_EOK;

    ret = rt_device_pin_register("pin", &_drv_pin_ops, RT_NULL);

    return ret;
}
INIT_BOARD_EXPORT(drv_pin_init);

#ifdef RT_USING_PM
/* Register PIN device to PM framework (must be called after heap initialization) */
static int drv_pin_pm_register(void)
{
    struct rt_device *pin_dev;
    int ret = RT_EOK;

    pin_dev = rt_device_find("pin");
    if (pin_dev) {
        rt_pm_device_register(pin_dev, &drv_pin_pm_ops);
    } else {
        rt_kprintf("[PIN PM] ERROR: pin device not found!\n");
        ret = RT_ERROR;
    }

    return ret;
}
INIT_DEVICE_EXPORT(drv_pin_pm_register);
#endif

#elif defined(KERNEL_BAREMETAL) || defined(KERNEL_FREERTOS)

#ifdef AIC_GPIO_IRQ_DRV_EN
#define MAX_GPIO_IRQ_GROUP 8

irqreturn_t drv_gpio_group_irqhandler(int irq, void * data)
{
    unsigned int g = irq - GPIO_IRQn;
    unsigned int stat = 0;
    unsigned int mask = 0;
    unsigned int i = 0;
    unsigned int gpio_irq = 0;

    hal_gpio_group_get_irq_stat(g, &stat);
    hal_gpio_group_get_irq_en(g, &mask);
    stat &= mask;

    for (i=0; i<32; i++) {
        if (!(stat & (1U<<i)))
            continue;

        gpio_irq = AIC_GPIO_TO_IRQ(g*GPIO_GROUP_SIZE + i);
        drv_irq_call_isr(gpio_irq);
    }

    hal_gpio_group_set_irq_stat(g, 0xFFFFFFFF);

    return IRQ_HANDLED;
}

void drv_gpio_group_irq_init(void)
{
    int g = 0;

    for (g = 0; g < MAX_GPIO_IRQ_GROUP; g++)
    {
        aicos_request_irq(GPIO_IRQn + g, drv_gpio_group_irqhandler, 0, "pin_group", NULL);
        aicos_irq_enable(GPIO_IRQn + g);
    }
}
#endif

int drv_pin_init(void)
{
    int ret = 0;

    #ifdef AIC_GPIO_IRQ_DRV_EN
    drv_gpio_group_irq_init();
    #endif

    return ret;
}
#endif /*RT_USING_PIN */


