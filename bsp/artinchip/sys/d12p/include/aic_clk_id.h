/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_AIC_CLK_ID_H__
#define __ARTINCHIP_AIC_CLK_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

enum clk_id {
    CLK_DUMMY,
    /* Fixed rate clock */
    CLK_OSC24M,
    CLK_OSC32K,
    CLK_APB1,
    /* PLL clock */
    CLK_PLL_INT0,
    CLK_PLL_INT1,
    CLK_PLL_FRA0,
    CLK_PLL_FRA1,
    CLK_PLL_FRA2,
    /* fixed factor clock */
    CLK_AXI0,
    CLK_AHB0,
    CLK_AHB1,
    CLK_APB0,
    /* system clock */
    CLK_OUT0,
    CLK_CPU,
    /* Peripheral clock */
    CLK_WDT,
    CLK_SCLK,
    CLK_PIX,
    CLK_DMA,
    CLK_DCE,
    CLK_USBD,
    CLK_USBH0,
    CLK_USB_PHY0,
    CLK_CANFD0,
    CLK_XSPI,
    CLK_QSPI0,
    CLK_QSPI1,
    CLK_QSPI2,
    CLK_QSPI3,
    CLK_QSPI4,
    CLK_QSPI5,
    CLK_SDMC0,
    CLK_SDMC1,
    CLK_SYSCFG,
    CLK_SPIENC,
    CLK_PWMCS,
    CLK_MTOP,
    CLK_I2S0,
    CLK_CODEC,
    CLK_GPIO,
    CLK_UART0,
    CLK_UART1,
    CLK_UART2,
    CLK_UART3,
    CLK_UART4,
    CLK_RGB,
    CLK_MIPIDSI,
    CLK_DVP,
    CLK_DE,
    CLK_GE,
    CLK_VE,
    CLK_SID,
    CLK_RTC,
    CLK_GTC,
    CLK_I2C0,
    CLK_I2C1,
    CLK_XPWM0,
    CLK_XPWM1,
    CLK_XPWM2,
    CLK_XPWM3,
    CLK_ADCIM,
    CLK_GPAI,
    CLK_RTP,
    CLK_TSEN,
    CLK_CIR,
    CLK_XPWM4,
    CLK_XPWM5,
    CLK_XPWM6,
    CLK_XPWM7,
    AIC_CLK_NUM,
};

/* frequence */

#define CLOCK1_FREQ             48000000
#define CLOCK2_FREQ             62500000
#define CLOCK3_FREQ             120000000
#define CLOCK4_FREQ             62500000
#define CLOCK5_FREQ             100000000
#define CLOCK6_FREQ             48000000
#define CLOCK_120M              120000000
#define CLOCK_100M              100000000
#define CLOCK_72M               72000000
#define CLOCK_60M               60000000
#define CLOCK_50M               50000000
#define CLOCK_36M               36000000
#define CLOCK_30M               30000000
#define CLOCK_AUDIO             24576000
#define CLOCK_24M               24000000
#define CLOCK_12M               12000000
#define CLOCK_4M                4000000
#define CLOCK_1M                1000000
#define CLOCK_32K               32768
#define CLOCK2_FREQ_D2          31250000
#define CLOCK2_FREQ_D4          15625000

/*--- ArtInChip CMU register offsets ---*/

#define PLL_INT0_GEN_REG        (0x0000)
#define PLL_INT1_GEN_REG        (0x0004)
#define PLL_FRA0_GEN_REG        (0x0020)
#define PLL_FRA1_GEN_REG        (0x0024)
#define PLL_FRA2_GEN_REG        (0x0028)
#define PLL_INT0_CFG_REG        (0x0040)
#define PLL_INT1_CFG_REG        (0x0044)
#define PLL_FRA0_CFG_REG        (0x0060)
#define PLL_FRA1_CFG_REG        (0x0064)
#define PLL_FRA2_CFG_REG        (0x0068)
#define PLL_FRA0_SDM_REG        (0x0080)
#define PLL_FRA1_SDM_REG        (0x0084)
#define PLL_FRA2_SDM_REG        (0x0088)
#define PLL_COM_REG             (0x00A0)
#define PLL_IN_REG              (0x00A4)
#define CLK_OUT0_REG            (0x00E0)
#define CLK_CPU_REG             (0x0180)
#define CLK_WDT_REG             (0x020C)
#define CLK_DISP_SCLK_REG       (0x0220)
#define CLK_DISP_PIXCLK_REG     (0x0224)
#define CLK_DMA_REG             (0x0400)
#define CLK_DCE_REG             (0x0414)
#define CLK_USB_DEV_REG         (0x041C)
#define CLK_USB_HOST_REG        (0x0420)
#define CLK_USB_PHY_REG         (0x0430)
#define CLK_CANFD0_REG          (0x0450)
#define CLK_XSPI_REG            (0x045C)
#define CLK_QSPI0_REG           (0x0460)
#define CLK_QSPI1_REG           (0x0464)
#define CLK_QSPI2_REG           (0x0468)
#define CLK_QSPI3_REG           (0x046C)
#define CLK_QSPI4_REG           (0x0470)
#define CLK_QSPI5_REG           (0x0474)
#define CLK_SDMC0_REG           (0x0480)
#define CLK_SDMC1_REG           (0x0484)
#define CLK_SYSCFG_REG          (0x0800)
#define CLK_SPIENC_REG          (0x0810)
#define CLK_PWMCS_REG           (0x0814)
#define CLK_MTOP_REG            (0x081C)
#define CLK_I2S_REG             (0x0820)
#define CLK_AUDIO_REG           (0x0830)
#define CLK_GPIO_REG            (0x083C)
#define CLK_UART0_REG           (0x0840)
#define CLK_UART1_REG           (0x0844)
#define CLK_UART2_REG           (0x0848)
#define CLK_UART3_REG           (0x084C)
#define CLK_UART4_REG           (0x0850)
#define CLK_LCD_REG             (0x0880)
#define CLK_MIPI_DSI_REG        (0x0888)
#define CLK_DVP_REG             (0x0890)
#define CLK_DE_REG              (0x08C0)
#define CLK_GE_REG              (0x08C4)
#define CLK_VE_REG              (0x08C8)
#define CLK_SID_REG             (0x0904)
#define CLK_RTC_REG             (0x0908)
#define CLK_GTC_REG             (0x090C)
#define CLK_I2C0_REG            (0x0960)
#define CLK_I2C1_REG            (0x0964)
#define CLK_XPWM0_REG           (0x0990)
#define CLK_XPWM1_REG           (0x0994)
#define CLK_XPWM2_REG           (0x0998)
#define CLK_XPWM3_REG           (0x099C)
#define CLK_ADCIM_REG           (0x09A0)
#define CLK_GPAI_REG            (0x09A4)
#define CLK_RTP_REG             (0x09A8)
#define CLK_TSEN_REG            (0x09AC)
#define CLK_CIR_REG             (0x09B0)
#define CLK_XPWM4_REG           (0x09C0)
#define CLK_XPWM5_REG           (0x09C4)
#define CLK_XPWM6_REG           (0x09C8)
#define CLK_XPWM7_REG           (0x09CC)
#define CLK_WR_CFG_REG          (0x0FE8)
#define CLK_WR_DIS_STS_REG      (0x0FEC)
#define CLK_VER_REG             (0x0FFC)

/* PLL_xxx_GEN register fields */

#define PLL_LOCK_BIT            (17)
#define PLL_EN_BIT              (16)
#define PLL_FACTORN_BIT         (8)
#define PLL_FACTORN_MASK        (0xff)
#define PLL_FACTORN_MIN         (14)
#define PLL_FACTORN_MAX         (199)

#define PLL_FACTORM_BIT         (4)
#define PLL_FACTORM_MASK        (0x3)
#define PLL_FACTORM_MIN         (0)
#define PLL_FACTORM_MAX         (3)
#define PLL_FACTORM_EN_BIT      (19)

#define PLL_FACTORP_BIT         (0)
#define PLL_FACTORP_MASK        (0x1)
#define PLL_FACTORP_MIN         (0)
#define PLL_FACTORP_MAX         (1)

#define PLL_DITHER_EN_BIT       (24)
#define PLL_FRAC_EN_BIT         (20)
#define PLL_FRAC_DIV_BIT        (0)
#define PLL_FRAC_DIV_MASK       (0x1ffff)

#define PLL_OUT_MUX             (20)
#define PLL_OUT_SYS             (18)

#define PLL_SDM_AMP_BIT         (0)
#define PLL_SDM_FREQ_BIT        (17)
#define PLL_SDM_STEP_BIT        (20)
#define PLL_SDM_MODE_BIT        (29)
#define PLL_SDM_EN_BIT          (31)

#define PLL_VCO_MIN             (768000000)
#define PLL_VCO_MAX             (1560000000)
#define PLL_SDM_AMP_MAX         (0x20000)
#define PLL_SDM_SPREAD_PPM      (10000)
#define PLL_SDM_SPREAD_FREQ     (33000)

#define CLK_NULL_REG            (0xFFFF)

/* BUS CLX_xxx register fields */

#define BUS_CLK_SEL (8)

/* APB0 CLX_xxx register fields */

#define MOD_RSTN   (24)
#define MOD_BUS_EN (20)
#define MOD_CLK_EN (16)

#ifdef __cplusplus
}
#endif

#endif /* __ARTINCHIP_AIC_CLK_ID_H__ */
