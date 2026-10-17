/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Geo.Dong <guojun.dong@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <rtthread.h>
#include "rtdevice.h"
#include "aic_core.h"
#include "rtconfig.h"
#include "aic_soc.h"


static const char sopts[] = "s:c:h";
static const struct option lopts[] = {
    {"start",       required_argument,  NULL, 's'},
    {"count",       required_argument,  NULL, 'c'},
    {"help",        no_argument,        NULL, 'h'},
    {0, 0, 0, 0}
};

static void usage(char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("\t -s, --start\t\tprint PIN start from PXn . Default as start from PA0\n");
    printf("\t -c, --count\t\tprint PIN count.\n");
    printf("\t -h, --help \n");
    printf("\n");
    printf("Example： \n");
    printf("     %s -s PA0 -c 2 \n", program);
}

struct gpio_info
{
    char        name;
    u8          group_index;
    u8          pin_max_size;
};


#if defined(AIC_CHIP_D13X) || defined(AIC_CHIP_G73X)
static const struct gpio_info pinmux_list[] = {
    {'A',   0,      16},
    {'B',   1,      18},
    {'C',   2,      12},
    {'D',   3,      28},
    {'E',   4,      18},
    {'U',   14,     2},
};
#elif defined(AIC_CHIP_D12X)
static const struct gpio_info pinmux_list[] = {
    {'A',   0,      12},
    {'B',   1,      22},
    {'C',   2,      12},
    {'D',   3,      28},
    {'E',   4,      21},
    {'F',   5,      2},
    {'S',   12,      4},
    {'M',   12,      4},
    {'N',   13,      3},
};
#elif defined(AIC_CHIP_D12P) || defined(AIC_CHIP_G72X)
static const struct gpio_info pinmux_list[] = {
    {'A',   0,      12},
    {'B',   1,      18},
    {'C',   2,      8},
    {'D',   3,      28},
    {'E',   4,      14},
};
#else
static const struct gpio_info pinmux_list[] = {
    {'A',   0,      12},
    {'B',   1,      12},
    {'C',   2,      7},
    {'D',   3,      28},
    {'E',   4,      20},
    {'F',   5,      16},
    {'U',   14,     2},
};
#endif

static u32 get_pin_cfg_reg(u32 group, u32 pin)
{
    return readl(GPIO_BASE + 0x80 + pin * 0x4 + group * 0x100);
}

static int show_pinmux_info(char start_group, u32 start_pin, u32 pin_count)
{
    for (u32 g = 0; g < sizeof(pinmux_list)/sizeof(struct gpio_info); g++) {
        if (pinmux_list[g].name == start_group) {
            for (u32 p = start_pin; p < start_pin + pin_count; p++) {
                if (p >= pinmux_list[g].pin_max_size)
                    break;
                u32 ret_val = get_pin_cfg_reg(pinmux_list[g].group_index, p);
                printf("P%c%u: 0x%08x, fun[%u], drv[%u], pull[%u], IE[%u], OE[%u], IE_FORCE[%u]\n",
                    pinmux_list[g].name, p, ret_val, ret_val&0xf, (ret_val >> 4)&0b111, (ret_val >> 8)&0b11,
                    (ret_val >> 12)&0b111, (ret_val >> 16)&0b1, (ret_val >> 17)&0b1);
            }
        }
    }

    return 0;
}

static int pinmux_check(int argc, char **argv)
{
    int     c = 0;
    char    start_group = 0;
    u32     start_pin = 0, pin_count = 0;

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    optind = 0;
    while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
        switch (c) {
        case 's':
            if (optarg == NULL || strlen(optarg) < 2) {
                printf("-s arg[%s] error!!!\n\n", optarg ? optarg : "null");
                return -1;
            } else {
                memcpy(&start_group, optarg+1, 1);
                start_pin = atoi(optarg+2);

                if (start_group < 'A') {
                    printf("start group out of range [%c]!\n\n", start_group);
                    return -1;
                }

                if (start_pin > 32) {
                    printf("start pin out of range [%u]!\n\n", start_pin);
                    return -1;
                }
            }
            break;
        case 'c':
            if (optarg == NULL || strlen(optarg) == 0) {
                printf("-c arg[%s] error!!!\n\n", optarg);
                return -1;
            } else {
                pin_count = atoi(optarg);
                if (pin_count > 32) {
                    printf("pin_count out of range [%u]!\n\n", pin_count);
                    return -1;
                }
            }
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }

    show_pinmux_info(start_group, start_pin, pin_count);

    return 0;
}

MSH_CMD_EXPORT(pinmux_check, pinmux check);

