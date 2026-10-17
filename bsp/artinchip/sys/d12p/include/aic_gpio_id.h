/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_GPIO_ID_H__
#define __AIC_GPIO_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_GROUP_BEGIN        'A'

#define AIC_GPIO_PS_ALIAS_EN    1

enum {
    PA_GROUP,
    PB_GROUP,
    PC_GROUP,
    PD_GROUP,
    PE_GROUP,
    PF_GROUP,
    PM_GROUP = 12,
    PN_GROUP,

#ifdef FPGA_BOARD_ARTINCHIP
    PP_GROUP = 15,
#endif
    GPIO_GROUP_MAX,
};

extern const int aic_gpio_groups_list[];
extern const int aic_gpio_group_size;

#define PA_BASE  0
#define PB_BASE  32
#define PC_BASE  64
#define PD_BASE  96
#define PE_BASE  128
#define PF_BASE  160
#define PM_BASE  384
#define PN_BASE  416

#define GPIOA(n) (PA_BASE + (n))
#define GPIOB(n) (PB_BASE + (n))
#define GPIOC(n) (PC_BASE + (n))
#define GPIOD(n) (PD_BASE + (n))
#define GPIOE(n) (PE_BASE + (n))
#define GPIOF(n) (PF_BASE + (n))
#define GPIOM(n) (PM_BASE + (n))
#define GPION(n) (PN_BASE + (n))

/* PS is hardware alias for PM (same register group) */
#ifdef AIC_GPIO_PS_ALIAS_EN
#define GPIOS(n) (PM_BASE + (n))
#endif

typedef enum {
    PA0  = GPIOA(0),
    PA1  = GPIOA(1),
    PA2  = GPIOA(2),
    PA3  = GPIOA(3),
    PA4  = GPIOA(4),
    PA5  = GPIOA(5),
    PA6  = GPIOA(6),
    PA7  = GPIOA(7),
    PA8  = GPIOA(8),
    PA9  = GPIOA(9),
    PA10 = GPIOA(10),
    PA11 = GPIOA(11),

    PB0  = GPIOB(0),
    PB1  = GPIOB(1),
    PB2  = GPIOB(2),
    PB3  = GPIOB(3),
    PB4  = GPIOB(4),
    PB5  = GPIOB(5),
    PB6  = GPIOB(6),
    PB7  = GPIOB(7),
    PB8  = GPIOB(8),
    PB9  = GPIOB(9),
    PB10 = GPIOB(10),
    PB11 = GPIOB(11),
    PB12 = GPIOB(12),
    PB13 = GPIOB(13),
    PB14 = GPIOB(14),
    PB15 = GPIOB(15),
    PB16 = GPIOB(16),
    PB17 = GPIOB(17),
    PB18 = GPIOB(18),

    PC0 = GPIOC(0),
    PC1 = GPIOC(1),
    PC2 = GPIOC(2),
    PC3 = GPIOC(3),
    PC4 = GPIOC(4),
    PC5 = GPIOC(5),
    PC6 = GPIOC(6),
    PC7 = GPIOC(7),
    PC8  = GPIOC(8),
    PC9  = GPIOC(9),
    PC10 = GPIOC(10),
    PC11 = GPIOC(11),

    PD0  = GPIOD(0),
    PD1  = GPIOD(1),
    PD2  = GPIOD(2),
    PD3  = GPIOD(3),
    PD4  = GPIOD(4),
    PD5  = GPIOD(5),
    PD6  = GPIOD(6),
    PD7  = GPIOD(7),
    PD8  = GPIOD(8),
    PD9  = GPIOD(9),
    PD10 = GPIOD(10),
    PD11 = GPIOD(11),
    PD12 = GPIOD(12),
    PD13 = GPIOD(13),
    PD14 = GPIOD(14),
    PD15 = GPIOD(15),
    PD16 = GPIOD(16),
    PD17 = GPIOD(17),
    PD18 = GPIOD(18),
    PD19 = GPIOD(19),
    PD20 = GPIOD(20),
    PD21 = GPIOD(21),
    PD22 = GPIOD(22),
    PD23 = GPIOD(23),
    PD24 = GPIOD(24),
    PD25 = GPIOD(25),
    PD26 = GPIOD(26),
    PD27 = GPIOD(27),

    PE0  = GPIOE(0),
    PE1  = GPIOE(1),
    PE2  = GPIOE(2),
    PE3  = GPIOE(3),
    PE4  = GPIOE(4),
    PE5  = GPIOE(5),
    PE6  = GPIOE(6),
    PE7  = GPIOE(7),
    PE8  = GPIOE(8),
    PE9  = GPIOE(9),
    PE10 = GPIOE(10),
    PE11 = GPIOE(11),
    PE12 = GPIOE(12),
    PE13 = GPIOE(13),
    PE14 = GPIOE(14),
    PE15 = GPIOE(15),
    PE16 = GPIOE(16),
    PE17 = GPIOE(17),

    PF0  = GPIOF(0),
    PF1  = GPIOF(1),

    PM0  = GPIOM(0),
    PM1  = GPIOM(1),
    PM2  = GPIOM(2),
    PM3  = GPIOM(3),

#ifdef AIC_GPIO_PS_ALIAS_EN
    /* PS aliases (same as PM) */
    PS0  = GPIOS(0),
    PS1  = GPIOS(1),
    PS2  = GPIOS(2),
    PS3  = GPIOS(3),
#endif

    PN0  = GPION(0),
    PN1  = GPION(1),
    PN2  = GPION(2),

    /* To aviod compile warnings. */
    GPIO_MAX_PIN,
} pin_name_t;

#ifdef __cplusplus
}
#endif

#endif /* __AIC_GPIO_ID_H__ */
