/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_UTILS_H_
#define __AIC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FLAG_WAKEUP_SOURCE                  (1)
#define FLAG_POWER_PIN                      (2)

struct aic_pinmux
{
    unsigned char       func;
    unsigned char       bias;
    unsigned char       drive;
    char *              name;
    /* This flag indicates whether the pin is the wakeup source.
     *
     * For example:
     * 1.The I2C0 is the wakeup source, then you need to set the flag
     *   of I2C0 pinmux to 1.
     *   {4, PIN_PULL_DIS, 3, "PD.6", FLAG_WAKEUP_SOURCE}, // SCK
     *   {4, PIN_PULL_DIS, 3, "PD.7", FLAG_WAKEUP_SOURCE}, // SDA
     *
     * 2.The GPIO PD6 is the wakeup source, then you need to set the flag
     *   of PD6 pinmux to 1.
     *   {1, PIN_PULL_DIS, 3, "PD.6", FLAG_WAKEUP_SOURCE},
     */
     unsigned char       flag;
};

extern struct aic_pinmux aic_pinmux_config[];
extern uint32_t aic_pinmux_config_size;

void hexdump(unsigned char *buf, unsigned long len, int groupsize);
void hexdump_msg(char *msg, unsigned char *buf, unsigned long len, int groupsize);

void show_speed(char *msg, unsigned long len, unsigned long us);
#ifdef LPKG_USING_FDTLIB
int pinmux_fdt_parse(void);
#endif

void aic_board_pinmux_init(void);
void aic_board_pinmux_deinit(void);

long long int str2int(char *str);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_UTILS_H_ */
