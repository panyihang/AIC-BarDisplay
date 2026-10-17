/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"

extern struct aic_clk_ops aic_clk_fixed_rate_ops;
extern struct aic_clk_ops aic_clk_pll_ops;
extern struct aic_clk_ops aic_clk_fixed_parent_ops;
extern struct aic_clk_ops aic_clk_multi_parent_ops;
extern struct aic_clk_ops aic_clk_disp_ops;

/* Fixed rate clocks */
FRCLK(CLK_OSC24M, "osc24m", CLOCK_24M);
FRCLK(CLK_OSC32K, "osc32k", CLOCK_32K);

/* PLL clocks */

PLL_INT(CLK_PLL_INT0, "pll_int0", CLK_OSC24M, PARENT("osc24m"),
        PLL_INT0_GEN_REG, 0);
PLL_INT(CLK_PLL_INT1, "pll_int1", CLK_OSC24M, PARENT("osc24m"),
        PLL_INT1_GEN_REG, 0);
#ifdef AIC_CLK_PLL_FRA0_SSC_DIS
PLL_FRA(CLK_PLL_FRA0, "pll_fra0", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA0_GEN_REG, PLL_FRA0_CFG_REG, PLL_FRA0_SDM_REG,
        CLK_IGNORE_UNUSED);
#else
PLL_SDM(CLK_PLL_FRA0, "pll_fra0", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA0_GEN_REG, PLL_FRA0_CFG_REG, PLL_FRA0_SDM_REG,
        CLK_IGNORE_UNUSED);
#endif
#ifdef AIC_CLK_PLL_FRA2_SSC_DIS
PLL_FRA(CLK_PLL_FRA2, "pll_fra2", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA2_GEN_REG, PLL_FRA2_CFG_REG, PLL_FRA2_SDM_REG, 0);
#else
PLL_SDM(CLK_PLL_FRA2, "pll_fra2", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA2_GEN_REG, PLL_FRA2_CFG_REG, PLL_FRA2_SDM_REG, 0);
#endif

/* Fixed parent clocks */

FPCLK(CLK_CPU_SRC1, "cpu_src1", CLK_PLL_INT0, PARENT("pll_int0"), CLK_CPU_REG,
      -1, -1, 0, 5);
FPCLK(CLK_AXI_AHB_SRC1, "axi_ahb_src1", CLK_PLL_INT1, PARENT("pll_int1"),
      CLK_AXI_AHB_REG, -1, -1, 0, 5);
FPCLK(CLK_APB0_SRC1, "apb0_src1", CLK_PLL_INT1, PARENT("pll_int1"),
      CLK_APB0_REG, -1, -1, 0, 5);
FPCLK(CLK_APB1, "apb1", CLK_OSC24M, PARENT("osc24m"), CLK_APB1_REG, -1, -1, 0,
      0);
FPCLK(CLK_DMA, "dma", CLK_AHB0, PARENT("ahb0"), CLK_DMA_REG, 12, -1, 0, 0);
FPCLK(CLK_CE, "ce", CLK_PLL_INT1, PARENT("pll_int1"), CLK_CE_REG, 12, 8, 0, 5);
FPCLK(CLK_USBD, "usb_dev", CLK_AHB0, PARENT("ahb0"), CLK_USBD_REG, 12, -1, 0,
      0);
FPCLK(CLK_USBH0, "usb_host0", CLK_AHB0, PARENT("ahb0"), CLK_USBH0_REG, 12, -1,
      0, 0);
FPCLK(CLK_USB_PHY0, "usb_phy0", CLK_OSC24M, PARENT("osc24m"), CLK_USB_PHY0_REG,
      -1, 8, 0, 0);
FPCLK(CLK_GMAC0, "gmac0", CLK_PLL_INT1, PARENT("pll_int1"), CLK_GMAC0_REG, 12,
      8, 0, 5);
FPCLK(CLK_XSPI, "xspi", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_XSPI_REG, 12, 8,
      0, 5);
FPCLK(CLK_QSPI0, "qspi0", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI0_REG, 12,
      8, 0, 5);
FPCLK(CLK_QSPI1, "qspi1", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI1_REG, 12,
      8, 0, 5);
FPCLK(CLK_QSPI2, "qspi2", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI2_REG, 12,
      8, 0, 5);
FPCLK(CLK_QSPI3, "qspi3", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI3_REG, 12,
      8, 0, 5);
FPCLK(CLK_SDMC0, "sdmc0", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_SDMC0_REG, 12,
      8, 0, 5);
FPCLK(CLK_SDMC1, "sdmc1", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_SDMC1_REG, 12,
      8, 0, 5);
FPCLK(CLK_PBUS, "pbus", CLK_AHB0, PARENT("ahb0"), CLK_PBUS_REG, 12, -1, 0, 0);
FPCLK(CLK_SYSCFG, "syscfg", CLK_OSC24M, PARENT("osc24m"), CLK_SYSCFG_REG, 12,
      -1, 0, 0);
FPCLK(CLK_RTC, "rtc", CLK_OSC32K, PARENT("osc32k"), CLK_RTC_REG, 12, -1, 0, 0);
FPCLK_BASE(CLK_AUDIO_SCLK, "audio_sclk", CLK_PLL_INT1, PARENT("pll_int1"), CLK_AUDIO_REG, -1,
      -1, 0, 1, 49, 4);
FPCLK(CLK_CODEC, "codec", CLK_AUDIO_SCLK, PARENT("audio_sclk"), CLK_CODEC_REG, 12,
      8, 0, 0);
FPCLK(CLK_I2S0, "i2s0", CLK_AUDIO_SCLK, PARENT("audio_sclk"), CLK_I2S0_REG, 12, 8,
      0, 0);
FPCLK(CLK_DE, "de", CLK_PLL_INT1, PARENT("pll_int1"), CLK_DE_REG, 12, 8, 0, 5);
FPCLK(CLK_GE, "ge", CLK_PLL_INT1, PARENT("pll_int1"), CLK_GE_REG, 12, 8, 0, 5);
FPCLK(CLK_VE, "ve", CLK_PLL_INT1, PARENT("pll_int1"), CLK_VE_REG, 12, 8, 0, 5);
FPCLK(CLK_WDT, "wdt", CLK_OSC32K, PARENT("clk_32k"), CLK_WDT_REG, 12, 8, 0,
      0);
FPCLK(CLK_SID, "sid", CLK_OSC24M, PARENT("osc24m"), CLK_SID_REG, 12, 8, 0, 0);
FPCLK(CLK_GTC, "gtc", CLK_APB1, PARENT("apb1"), CLK_GTC_REG, 12, -1, 0, 0);
FPCLK(CLK_GPIO, "gpio", CLK_APB0, PARENT("apb0"), CLK_GPIO_REG, 12, -1, 0, 0);
FPCLK(CLK_UART0, "uart0", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART0_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART1, "uart1", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART1_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART2, "uart2", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART2_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART3, "uart3", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART3_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART4, "uart4", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART4_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART5, "uart5", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART5_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART6, "uart6", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART6_REG, 12,
      8, 0, 5);
FPCLK(CLK_UART7, "uart7", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART7_REG, 12,
      8, 0, 5);
FPCLK(CLK_I2C0, "i2c0", CLK_APB1, PARENT("apb1"), CLK_I2C0_REG, 12, -1, 0, 0);
FPCLK(CLK_I2C1, "i2c1", CLK_APB1, PARENT("apb1"), CLK_I2C1_REG, 12, -1, 0, 0);
FPCLK(CLK_I2C2, "i2c2", CLK_APB1, PARENT("apb1"), CLK_I2C2_REG, 12, -1, 0, 0);
FPCLK(CLK_CAN0, "can0", CLK_APB1, PARENT("apb1"), CLK_CAN0_REG, 12, -1, 0, 0);
FPCLK(CLK_CAN1, "can1", CLK_APB1, PARENT("apb1"), CLK_CAN1_REG, 12, -1, 0, 0);
FPCLK(CLK_PWM, "pwm", CLK_PLL_INT1, PARENT("pll_int1"), CLK_PWM_REG, 12, 8, 0,
      5);
FPCLK(CLK_ADCIM, "adcim", CLK_PLL_INT1, PARENT("pll_int1"), CLK_ADCIM_REG, 12,
      8, 0, 5);
FPCLK(CLK_GPAI, "gpai", CLK_APB1, PARENT("apb1"), CLK_GPAI_REG, 12, -1, 0, 0);
FPCLK(CLK_RTP, "rtp", CLK_APB1, PARENT("apb1"), CLK_RTP_REG, 12, -1, 0, 0);
FPCLK(CLK_TSEN, "tsen", CLK_APB1, PARENT("apb1"), CLK_TSEN_REG, 12, -1, 0, 0);
FPCLK(CLK_CIR, "cir", CLK_APB1, PARENT("apb1"), CLK_CIR_REG, 12, -1, 0, 0);
FPCLK(CLK_DVP, "dvp", CLK_PLL_INT1, PARENT("pll_int1"), CLK_DVP_REG, 12, 8, 0,
      5);
FPCLK(CLK_MTOP, "mtop", CLK_APB0, PARENT("apb0"), CLK_MTOP_REG, 12, -1, 0, 0);
FPCLK(CLK_SPIENC, "spienc", CLK_AHB0, PARENT("ahb0"), CLK_SPIENC_REG, 12, 8, 0,
      0);
FPCLK(CLK_RGB, "rgb", CLK_SCLK, PARENT("sclk"), CLK_RGB_REG, 12, 8, 0, 0);
FPCLK(CLK_LVDS, "lvds", CLK_SCLK, PARENT("sclk"), CLK_LVDS_REG, 12, 8, 0, 0);
FPCLK(CLK_MIPIDSI, "mipidsi", CLK_PLL_FRA2, PARENT("pll_fra2"), CLK_MIPID_REG, 12, 8, 0,
      0);
FPCLK(CLK_PSADC, "psadc", CLK_PLL_INT1, PARENT("pll_int1"), CLK_PSADC_REG, 12, 8, 0, 5);
FPCLK(CLK_PWMCS, "pwmcs", CLK_PLL_INT1, PARENT("pll_int1"), CLK_PWMCS_REG, 12, 8, 0, 0);
FPCLK(CLK_PWMCS_SDFM, "pwmcs_sdfm", CLK_PLL_INT1, PARENT("pll_int1"),
        CLK_PWMCS_SDFM_REG, -1, -1, 0, 5);

/* Multi parent clocks */

static const u8 axi_ahb_src_sels[] = {
    /* "osc24m", "ahb0_src1", */
    CLK_OSC24M,
    CLK_AXI_AHB_SRC1,
};

static const u8 apb0_src_sels[] = {
    /* "osc24m",
    "apb0_src1", */
    CLK_OSC24M,
    CLK_APB0_SRC1,
};

static const u8 cpu_src_sels[] = {
    /* "osc24m",
    "cpu_src1", */
    CLK_OSC24M,
    CLK_CPU_SRC1,
};

static const u8 outclk_src_sels[] = {
    /* "pll_int1",
    "pll_fra1",
    "pll_fra2", */
    CLK_PLL_INT1,
    0,
    CLK_PLL_FRA2,
};

MPCLK(CLK_CPU, "cpu", cpu_src_sels, CLK_CPU_REG, -1, 8, 1, 0, 0);
MPCLK(CLK_AXI0, "axi0", axi_ahb_src_sels, CLK_AXI_AHB_REG, -1, 8, 1, 0, 0);
MPCLK(CLK_AHB0, "ahb0", axi_ahb_src_sels, CLK_AXI_AHB_REG, -1, 8, 1, 0, 0);
MPCLK(CLK_APB0, "apb0", apb0_src_sels, CLK_APB0_REG, -1, 8, 1, 0, 0);
MPCLK(CLK_OUT0, "clk_out0", outclk_src_sels, CLK_OUT0_REG, 16, 12, 3, 0, 8);
MPCLK(CLK_OUT1, "clk_out1", outclk_src_sels, CLK_OUT1_REG, 16, 12, 3, 0, 8);
MPCLK(CLK_OUT2, "clk_out2", outclk_src_sels, CLK_OUT2_REG, 16, 12, 3, 0, 8);
MPCLK(CLK_OUT3, "clk_out3", outclk_src_sels, CLK_OUT3_REG, 16, 12, 3, 0, 8);

/* Disp clocks */

DISPCLK(CLK_PIX, "pixclk", CLK_SCLK, PARENT("sclk"), CLK_DISP_REG, 0, 0, 4, 5,
        10, 2, 12, 2);
DISPCLK(CLK_SCLK, "sclk", CLK_PLL_FRA2, PARENT("pll_fra2"), CLK_DISP_REG, 0, 3,
        0, 0, 0, 0, 0, 0);

/* Clock cfg array */

const struct aic_clk_comm_cfg *aic_clk_cfgs[AIC_CLK_NUM] = {
    /* Fixed rate clock */
    DUMMY_CFG(CLK_DUMMY),
    AIC_CLK_CFG(CLK_OSC24M),
    AIC_CLK_CFG(CLK_OSC32K),
    /* PLL clock */
    AIC_CLK_CFG(CLK_PLL_INT0),
    AIC_CLK_CFG(CLK_PLL_INT1),
    AIC_CLK_CFG(CLK_PLL_FRA0),
    AIC_CLK_CFG(CLK_PLL_FRA2),
    /* fixed factor clock */
    AIC_CLK_CFG(CLK_AXI_AHB_SRC1),
    AIC_CLK_CFG(CLK_APB0_SRC1),
    AIC_CLK_CFG(CLK_CPU_SRC1),
    /* system clock */
    AIC_CLK_CFG(CLK_AXI0),
    AIC_CLK_CFG(CLK_AHB0),
    AIC_CLK_CFG(CLK_APB0),
    AIC_CLK_CFG(CLK_APB1),
    AIC_CLK_CFG(CLK_CPU),
    /* Peripheral clock */
    AIC_CLK_CFG(CLK_DMA),
    AIC_CLK_CFG(CLK_CE),
    AIC_CLK_CFG(CLK_USBD),
    AIC_CLK_CFG(CLK_USBH0),
    AIC_CLK_CFG(CLK_USB_PHY0),
    AIC_CLK_CFG(CLK_GMAC0),
    AIC_CLK_CFG(CLK_XSPI),
    AIC_CLK_CFG(CLK_QSPI0),
    AIC_CLK_CFG(CLK_QSPI1),
    AIC_CLK_CFG(CLK_QSPI2),
    AIC_CLK_CFG(CLK_QSPI3),
    AIC_CLK_CFG(CLK_SDMC0),
    AIC_CLK_CFG(CLK_SDMC1),
    AIC_CLK_CFG(CLK_SYSCFG),
    AIC_CLK_CFG(CLK_RTC),
    AIC_CLK_CFG(CLK_SPIENC),
    AIC_CLK_CFG(CLK_I2S0),
    AIC_CLK_CFG(CLK_AUDIO_SCLK),
    AIC_CLK_CFG(CLK_CODEC),
    AIC_CLK_CFG(CLK_RGB),
    AIC_CLK_CFG(CLK_LVDS),
    AIC_CLK_CFG(CLK_MIPIDSI),
    AIC_CLK_CFG(CLK_DE),
    AIC_CLK_CFG(CLK_GE),
    AIC_CLK_CFG(CLK_VE),
    AIC_CLK_CFG(CLK_WDT),
    AIC_CLK_CFG(CLK_SID),
    AIC_CLK_CFG(CLK_GTC),
    AIC_CLK_CFG(CLK_GPIO),
    AIC_CLK_CFG(CLK_UART0),
    AIC_CLK_CFG(CLK_UART1),
    AIC_CLK_CFG(CLK_UART2),
    AIC_CLK_CFG(CLK_UART3),
    AIC_CLK_CFG(CLK_UART4),
    AIC_CLK_CFG(CLK_UART5),
    AIC_CLK_CFG(CLK_UART6),
    AIC_CLK_CFG(CLK_UART7),
    AIC_CLK_CFG(CLK_I2C0),
    AIC_CLK_CFG(CLK_I2C1),
    AIC_CLK_CFG(CLK_I2C2),
    AIC_CLK_CFG(CLK_CAN0),
    AIC_CLK_CFG(CLK_CAN1),
    AIC_CLK_CFG(CLK_PWM),
    AIC_CLK_CFG(CLK_ADCIM),
    AIC_CLK_CFG(CLK_GPAI),
    AIC_CLK_CFG(CLK_RTP),
    AIC_CLK_CFG(CLK_TSEN),
    AIC_CLK_CFG(CLK_CIR),
    AIC_CLK_CFG(CLK_DVP),
    AIC_CLK_CFG(CLK_PBUS),
    AIC_CLK_CFG(CLK_MTOP),
    AIC_CLK_CFG(CLK_PSADC),
    AIC_CLK_CFG(CLK_PWMCS),
    AIC_CLK_CFG(CLK_PWMCS_SDFM),
    /* Display clock */
    AIC_CLK_CFG(CLK_PIX),
    AIC_CLK_CFG(CLK_SCLK),
    /* Output clock */
    AIC_CLK_CFG(CLK_OUT0),
    AIC_CLK_CFG(CLK_OUT1),
    AIC_CLK_CFG(CLK_OUT2),
    AIC_CLK_CFG(CLK_OUT3),
};
