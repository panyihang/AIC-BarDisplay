/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_AIC_CLK_ID_H__
#define __ARTINCHIP_AIC_CLK_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

enum clk_id {
    /* Fixed rate clock */
    CLK_DUMMY,
    CLK_OSC24M,
    CLK_OSC32K,
    /* PLL clock */
    CLK_PLL_INT0,
    CLK_PLL_INT1,
    CLK_PLL_FRA0,
    CLK_PLL_FRA2,
    /* fixed factor clock */
    CLK_AXI_AHB_SRC1,
    CLK_APB0_SRC1,
    CLK_CPU_SRC1,
    /* system clock */
    CLK_AXI0,
    CLK_AHB0,
    CLK_APB0,
    CLK_APB1,
    CLK_CPU,
    /* Peripheral clock */
    CLK_WDT,
    CLK_DMA,
    CLK_CE,
    CLK_USBD,
    CLK_USBH0,
    CLK_USB_PHY0,
    CLK_GMAC0,
    CLK_XSPI,
    CLK_QSPI0,
    CLK_QSPI1,
    CLK_QSPI2,
    CLK_QSPI3,
    CLK_SDMC0,
    CLK_SDMC1,
    CLK_PBUS,
    CLK_SYSCFG,
    CLK_SPIENC,
    CLK_MTOP,
    CLK_I2S0,
    CLK_AUDIO_SCLK,
    CLK_CODEC,
    CLK_GPIO,
    CLK_UART0,
    CLK_UART1,
    CLK_UART2,
    CLK_UART3,
    CLK_UART4,
    CLK_UART5,
    CLK_UART6,
    CLK_UART7,
    CLK_RGB,
    CLK_LVDS,
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
    CLK_I2C2,
    CLK_CAN0,
    CLK_CAN1,
    CLK_PWM,
    CLK_ADCIM,
    CLK_GPAI,
    CLK_RTP,
    CLK_TSEN,
    CLK_CIR,
    CLK_PSADC,
    CLK_CMP,
    CLK_PWMCS,
    CLK_PWMCS_SDFM,
    /* Display clock */
    CLK_PIX,
    CLK_SCLK,
    /* Output clock */
    CLK_OUT0,
    CLK_OUT1,
    CLK_OUT2,
    CLK_OUT3,
    AIC_CLK_NUM,
};

/* frequence */

#define CLOCK1_FREQ 48000000
#define CLOCK2_FREQ 60000000
#define CLOCK3_FREQ 120000000
#define CLOCK4_FREQ 62500000
#define CLOCK5_FREQ 48000000
#define CLOCK6_FREQ 60000000
#define CLOCK_120M  120000000
#define CLOCK_100M  100000000
#define CLOCK_72M   72000000
#define CLOCK_60M   60000000
#define CLOCK_50M   50000000
#define CLOCK_36M   36000000
#define CLOCK_30M   30000000
#define CLOCK_AUDIO 24576000
#define CLOCK_24M   24000000
#define CLOCK_12M   12000000
#define CLOCK_4M    4000000
#define CLOCK_1M    1000000
#define CLOCK_32K   32768

/*--- ArtInChip CMU register offsets ---*/

#define PLL_INT0_GEN_REG        (0x0000)
#define PLL_INT1_GEN_REG        (0x0004)
#define PLL_FRA0_GEN_REG        (0x0020)
#define PLL_FRA2_GEN_REG        (0x0028)
#define PLL_INT0_CFG_REG        (0x0040)
#define PLL_INT1_CFG_REG        (0x0044)
#define PLL_FRA0_CFG_REG        (0x0060)
#define PLL_FRA2_CFG_REG        (0x0068)
#define PLL_FRA0_SDM_REG        (0x0080)
#define PLL_FRA2_SDM_REG        (0x0088)
#define PLL_COM_REG             (0x00A0)
#define PLL_IN_REG              (0x00A4)
#define CLK_OUT0_REG            (0x00E0)
#define CLK_OUT1_REG            (0x00E4)
#define CLK_OUT2_REG            (0x00E8)
#define CLK_OUT3_REG            (0x00EC)
#define CLK_AXI_AHB_REG         (0x0100)
#define CLK_APB0_REG            (0x0120)
#define CLK_APB1_REG            (0x0124)
#define CLK_CPU_REG             (0x0200)
#define CLK_DM_REG              (0x0204)
#define CLK_WDT_REG             (0x020C)
#define CLK_DISP_REG            (0x0220)
#define CLK_AUDIO_REG           (0x0230)
#define CLK_PWMCS_SDFM_REG      (0x0240)
#define CLK_DMA_REG             (0x0410)
#define CLK_CE_REG              (0x0418)
#define CLK_USBD_REG            (0x041C)
#define CLK_USBH0_REG           (0x0420)
#define CLK_USB_PHY0_REG        (0x0430)
#define CLK_GMAC0_REG           (0x0440)
#define CLK_XSPI_REG            (0x045C)
#define CLK_QSPI0_REG           (0x0460)
#define CLK_QSPI1_REG           (0x0464)
#define CLK_QSPI2_REG           (0x0468)
#define CLK_QSPI3_REG           (0x046C)
#define CLK_SDMC0_REG           (0x0470)
#define CLK_SDMC1_REG           (0x0474)
#define CLK_CORDIC_REG          (0x0490)
#define CLK_HCL_REG             (0x0490)
#define CLK_PBUS_REG            (0x04A0)
#define CLK_SYSCFG_REG          (0x0800)
#define CLK_SPIENC_REG          (0x0810)
#define CLK_PWMCS_REG           (0x0814)
#define CLK_PSADC_REG           (0x0818)
#define CLK_MTOP_REG            (0x081C)
#define CLK_I2S0_REG            (0x0820)
#define CLK_CODEC_REG           (0x0830)
#define CLK_GPIO_REG            (0x083C)
#define CLK_UART0_REG           (0x0840)
#define CLK_UART1_REG           (0x0844)
#define CLK_UART2_REG           (0x0848)
#define CLK_UART3_REG           (0x084C)
#define CLK_UART4_REG           (0x0850)
#define CLK_UART5_REG           (0x0854)
#define CLK_UART6_REG           (0x0858)
#define CLK_UART7_REG           (0x085C)
#define CLK_TA_IF_REG           (0x0870)
#define CLK_EDT_REG             (0x0874)
#define CLK_BISS_IF_REG         (0x0878)
#define CLK_SDFM_REG            (0x087C)
#define CLK_RGB_REG             (0x0880)
#define CLK_LVDS_REG            (0x0884)
#define CLK_MIPID_REG           (0x0888)
#define CLK_DVP_REG             (0x0890)
#define CLK_DE_REG              (0x08C0)
#define CLK_GE_REG              (0x08C4)
#define CLK_VE_REG              (0x08C8)
#define CLK_SID_REG             (0x0904)
#define CLK_RTC_REG             (0x0908)
#define CLK_GTC_REG             (0x090C)
#define CLK_I2C0_REG            (0x0960)
#define CLK_I2C1_REG            (0x0964)
#define CLK_I2C2_REG            (0x0968)
#define CLK_CAN0_REG            (0x0980)
#define CLK_CAN1_REG            (0x0984)
#define CLK_PWM_REG             (0x0990)
#define CLK_ADCIM_REG           (0x09A0)
#define CLK_GPAI_REG            (0x09A4)
#define CLK_RTP_REG             (0x09A8)
#define CLK_TSEN_REG            (0x09AC)
#define CLK_CIR_REG             (0x09B0)
#define CLK_CMP_REG             (0x09E4)
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

/* BUS CLX_xxx register fields */

#define BUS_CLK_SEL (8)

/* APB0 CLX_xxx register fields */

#define MOD_RSTN   (13)
#define MOD_BUS_EN (12)
#define MOD_CLK_EN (8)

#ifdef __cplusplus
}
#endif

#endif /* __ARTINCHIP_AIC_CLK_ID_H__ */
