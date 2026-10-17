/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weijie.ding <weijie.ding@artinchip.com>
 */

#include <stdint.h>
#include <string.h>

#include <aic_core.h>
#include <aic_utils.h>
#include <aic_common.h>
#include <aic_hal_gpio.h>
#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#elif defined(KERNEL_BAREMETAL)
#include <console.h>
#endif
#ifdef LPKG_USING_FDTLIB
#include <libfdt.h>
#include <of.h>
#endif

extern const int aic_gpio_groups_list[];

#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static int list_pinmux(int argc, char *argv[])
{
    unsigned int group, pin, func, i, j;
    pin_name_t pin_name, pin_name_max;

    for (i = 0, j = 0; i < GPIO_GROUP_MAX && j < aic_gpio_group_size; i++) {
        if (i == aic_gpio_groups_list[j]) {
            pin_name = i * GPIO_GROUP_SIZE;
            pin_name_max = pin_name + GPIO_GROUP_SIZE;

            while (pin_name < pin_name_max) {
                group = GPIO_GROUP(pin_name);
                pin = GPIO_GROUP_PIN(pin_name);
                func = 0;

                hal_gpio_get_func(group, pin, &func);

#ifdef AIC_SE_GPIO_DRV
                if (func) {
                    if (group == SE_GPIO_GROUP_ID)
                        printf("SE.%u: function: %u\n", pin, func);
                    else
                        printf("P%c%u: function: %u\n", group + 'A', pin, func);
                }
#else
                if (func) {
                    printf("P%c%u: function: %u\n", group + 'A', pin, func);
                }
#endif

                pin_name++;
            }

            j++;
        }
    }

    return 0;
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(list_pinmux, list_pinmux, list pin function config);
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
CONSOLE_CMD(list_pinmux, list_pinmux, "list pin function config");
#endif
#endif // defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)

#ifdef LPKG_USING_FDTLIB
/*
 * DTS uses 32bit data to define the properties of a pin.
 *
 * |---------------------32bit---------------------|
 * |<----8---->|<----8---->|<----8---->|<-4->|<-4->|
 * |    group  |    pin    |    func   |pull | STR |
 *
 */
#define AIC_GROUP_OFFSET                (24)
#define AIC_PIN_OFFSET                  (16)
#define AIC_FUNC_OFFSET                 (8)
#define AIC_PULL_OFFSET                 (4)
#define AIC_STR_OFFSET                  (0)

#define AIC_DTS_PINMUX(group, pin, func, pull, strength)			\
					(((group - 'A') << AIC_GROUP_OFFSET) | \
					(pin << AIC_PIN_OFFSET) | \
					(func << AIC_FUNC_OFFSET) | \
					(pull << AIC_PULL_OFFSET) | \
					(strength << AIC_STR_OFFSET))

#define AIC_PINCTRL_GET_GROUP(pinmux)   ((pinmux >> AIC_GROUP_OFFSET) & 0xFF)
#define AIC_PINCTRL_GET_PIN(pinmux)     ((pinmux >> AIC_PIN_OFFSET) & 0xFF)
#define AIC_PINCTRL_GET_FUNC(pinmux)    ((pinmux >> AIC_FUNC_OFFSET) & 0xFF)
#define AIC_PINCTRL_GET_PULL(pinmux)    ((pinmux >> AIC_PULL_OFFSET) & 0xF)
#define AIC_PINCTRL_GET_STR(pinmux)     ((pinmux >> AIC_STR_OFFSET) & 0xF)

int pinmux_fdt_parse(void)
{
    u8 group, pin, func, pull, strength;
    ofnode pinctrl, node;
    int i, cell_cnt = 0;
    u32 pinmux = 0;

    of_find_node_by_path("/soc/pinctrl", &pinctrl);
    if (pinctrl.offset < 0)
    {
        pr_err("/soc/pinctrl node not found!\n");
        return -1;
    }

    if (!of_fdt_device_is_available(pinctrl))
    {
        pr_info("/soc/pinctrl is disabled\n");
        return 0;
    }

    /* find pin_deinit node, then deinit pin in this subnode */
    of_find_subnode(pinctrl, "pin_deinit", &node);
    if (node.offset > 0)
    {
        of_property_get_u32_array_number(node, "pinmux", &cell_cnt);

        for (i = 0; i < cell_cnt; i++)
        {
            of_property_read_u32_by_index(node, "pinmux", i, &pinmux);
            group = AIC_PINCTRL_GET_GROUP(pinmux);
            pin = AIC_PINCTRL_GET_PIN(pinmux);

            /* Deinit pin function*/
            hal_gpio_set_func(group, pin, 0);
        }
    }

    /* find pin_init node, then init pin in this subnode */
    of_find_subnode(pinctrl, "pin_init", &node);
    if (node.offset > 0)
    {
        of_property_get_u32_array_number(node, "pinmux", &cell_cnt);

        for (i = 0; i < cell_cnt; i++)
        {
            of_property_read_u32_by_index(node, "pinmux", i, &pinmux);
            group = AIC_PINCTRL_GET_GROUP(pinmux);
            pin = AIC_PINCTRL_GET_PIN(pinmux);
            func = AIC_PINCTRL_GET_FUNC(pinmux);
            pull = AIC_PINCTRL_GET_PULL(pinmux);
            strength = AIC_PINCTRL_GET_STR(pinmux);

            /* Init pin */
            hal_gpio_set_func(group, pin, func);
            hal_gpio_set_bias_pull(group, pin, pull);
            hal_gpio_set_drive_strength(group, pin, strength);
        }
    }

    return 0;
}
#endif

__WEAK void aic_board_pinmux_init(void)
{
    uint32_t i = 0;
    long pin = 0;
    unsigned int g;
    unsigned int p;

    for (i=0; i < aic_pinmux_config_size; i++) {
        pin = hal_gpio_name2pin(aic_pinmux_config[i].name);
        if (pin < 0)
            continue;
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_set_func(g, p, aic_pinmux_config[i].func);
        hal_gpio_set_bias_pull(g, p, aic_pinmux_config[i].bias);
        hal_gpio_set_drive_strength(g, p, aic_pinmux_config[i].drive);
    }
}

__WEAK void aic_board_pinmux_deinit(void)
{
    uint32_t i = 0;
    long pin = 0;
    unsigned int g;
    unsigned int p;

    for (i=0; i < aic_pinmux_config_size; i++) {
        if (aic_pinmux_config[i].flag)
            continue;

        pin = hal_gpio_name2pin(aic_pinmux_config[i].name);
        if (pin < 0)
            continue;
        g = GPIO_GROUP(pin);
        p = GPIO_GROUP_PIN(pin);
        hal_gpio_set_func(g, p, 0);
        hal_gpio_set_bias_pull(g, p, 0);
        hal_gpio_set_drive_strength(g, p, 0);
    }
}
