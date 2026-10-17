/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
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
extern struct aic_clk_ops aic_clk_auth_ops;

/* Fixed rate clocks */
FRCLK(CLK_OSC24M, "osc24m", CLOCK_24M);
FRCLK(CLK_OSC32K, "osc32k", CLOCK_32K);
FRCLK(CLK_APB1, "apb1", CLOCK_24M);

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

PLL_FRA(CLK_PLL_FRA1, "pll_fra1", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA1_GEN_REG, PLL_FRA1_CFG_REG, PLL_FRA1_SDM_REG,
        CLK_IGNORE_UNUSED);

#ifdef AIC_CLK_PLL_FRA2_SSC_DIS
PLL_FRA(CLK_PLL_FRA2, "pll_fra2", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA2_GEN_REG, PLL_FRA2_CFG_REG, PLL_FRA2_SDM_REG, 0);
#else
PLL_SDM(CLK_PLL_FRA2, "pll_fra2", CLK_OSC24M, PARENT("osc24m"),
        PLL_FRA2_GEN_REG, PLL_FRA2_CFG_REG, PLL_FRA2_SDM_REG, 0);
#endif

struct table_div no_div[] = {
    {
        .shift = -1,
        {
            .div = 1,
        }
    },
};

struct table_div bus_div[] = {
    {
        .shift = -1,
        {
            .div = 2,
        }
    },
};

struct table_div mod_div1[] = {
    {
        .shift = 0,
        {
            .width = 6,
        }
    },
};

struct table_div mod_div2[] = {
    {
        .shift = 0,
        {
            .width = 4,
        }
    },
};

struct table_div mod_div3[] = {
    {
        .shift = 0,
        {
            .width = 3,
        }
    },
};

struct table_div pixclk_div[] = {
    {
        .shift = 0,
        {
            .width = 8,
        }
    },
};

s8 no_gates[] = {-1};
s8 mod_gates[] = {16};
s8 pre_gates[] = {17};
s8 apb_gates[] = {20};
s8 ahb_gates[] = {21};
s8 mod_apb_gates[] = {16, 20};
s8 apb_axi_gates[] = {20, 22};
s8 ahb_axi_gates[] = {21, 22};
s8 pre_mod_gates[] = {17, 16};
s8 pre_mod_apb_gates[] = {17, 16, 20};
s8 pre_mod_ahb_gates[] = {17, 16, 21};
s8 pre_mod_apb_axi_gates[] = {17, 16, 20, 22};
s8 pre_mod_ahb_axi_gates[] = {17, 16, 21, 22};

/* Fixed parent clocks */
FPCLK(CLK_AXI0, "axi0", CLK_CPU, PARENT("cpu"), CLK_NULL_REG, no_gates, bus_div, 0);
FPCLK(CLK_AHB0, "ahb0", CLK_CPU, PARENT("cpu"), CLK_NULL_REG, no_gates, bus_div, 0);
FPCLK(CLK_AHB1, "ahb1", CLK_CPU, PARENT("cpu"), CLK_NULL_REG, no_gates, bus_div, 0);
FPCLK(CLK_APB0, "apb0", CLK_AHB0, PARENT("ahb0"), CLK_NULL_REG, no_gates, bus_div, 0);
FPCLK(CLK_SCLK, "disp_sclk", CLK_PLL_FRA2, PARENT("pll_fra2"), CLK_DISP_SCLK_REG,
      pre_gates, mod_div1, 0);
FPCLK(CLK_PIX, "disp_pixclk", CLK_SCLK, PARENT("disp_sclk"), CLK_DISP_PIXCLK_REG,
      no_gates, pixclk_div, 0);
FPCLK(CLK_DMA, "dma", CLK_AHB0, PARENT("ahb0"), CLK_DMA_REG, ahb_axi_gates, no_div, 0);
FPCLK(CLK_DCE, "dce", CLK_AHB0, PARENT("ahb0"), CLK_DCE_REG, ahb_gates, no_div, 0);
FPCLK(CLK_USBD, "usb_dev", CLK_AHB0, PARENT("ahb0"), CLK_USB_DEV_REG, ahb_gates, no_div, 0);
FPCLK(CLK_USBH0, "usb_host", CLK_AHB0, PARENT("ahb0"), CLK_USB_HOST_REG, ahb_gates, no_div, 0);
FPCLK(CLK_USB_PHY0, "usb_phy", CLK_OSC24M, PARENT("osc24m"), CLK_USB_PHY_REG, mod_gates, no_div, 0);
FPCLK(CLK_CANFD0, "canfd0", CLK_PLL_INT1, PARENT("pll_int1"), CLK_CANFD0_REG,
      pre_mod_ahb_gates, mod_div1, 0);
FPCLK(CLK_XSPI, "xspi", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_XSPI_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI0, "qspi0", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI0_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI1, "qspi1", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI1_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI2, "qspi2", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI2_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI3, "qspi3", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI3_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI4, "qspi4", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI4_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_QSPI5, "qspi5", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_QSPI5_REG,
      pre_mod_ahb_axi_gates, mod_div2, 0);
FPCLK(CLK_SDMC0, "sdmc0", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_SDMC0_REG,
      pre_mod_ahb_gates, mod_div2, 0);
FPCLK(CLK_SDMC1, "sdmc1", CLK_PLL_FRA0, PARENT("pll_fra0"), CLK_SDMC1_REG,
      pre_mod_ahb_gates, mod_div2, 0);
FPCLK(CLK_SYSCFG, "syscfg", CLK_OSC24M, PARENT("osc24m"), CLK_SYSCFG_REG,
      apb_gates, no_div, 0);
FPCLK(CLK_SPIENC, "spienc", CLK_AHB0, PARENT("ahb0"), CLK_SPIENC_REG,
      ahb_gates, no_div, 0);
FPCLK(CLK_PWMCS, "pwmcs", CLK_AHB1, PARENT("ahb1"), CLK_PWMCS_REG,
      ahb_gates, no_div, 0);
FPCLK(CLK_MTOP, "mtop", CLK_APB0, PARENT("apb0"), CLK_MTOP_REG,
      apb_axi_gates, no_div, 0);
FPCLK(CLK_I2S0, "i2s0", CLK_PLL_FRA1, PARENT("pll_fra1"), CLK_I2S_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_CODEC, "audio", CLK_PLL_FRA1, PARENT("pll_fra1"), CLK_AUDIO_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_GPIO, "gpio", CLK_AHB0, PARENT("ahb0"), CLK_GPIO_REG, ahb_gates, no_div, 0);
FPCLK(CLK_UART0, "uart0", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART0_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_UART1, "uart1", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART1_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_UART2, "uart2", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART2_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_UART3, "uart3", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART3_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_UART4, "uart4", CLK_PLL_INT1, PARENT("pll_int1"), CLK_UART4_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_RGB, "lcd", CLK_PLL_FRA2, PARENT("pll_fra2"), CLK_LCD_REG,
      pre_mod_apb_gates, mod_div3, 0);
FPCLK(CLK_MIPIDSI, "mipi_dsi", CLK_PLL_FRA2, PARENT("pll_fra2"), CLK_MIPI_DSI_REG,
      pre_mod_apb_gates, mod_div3, 0);
FPCLK(CLK_DVP, "dvp", CLK_PLL_INT1, PARENT("pll_int1"), CLK_DVP_REG,
      pre_mod_apb_axi_gates, mod_div2, 0);
FPCLK(CLK_DE, "de", CLK_PLL_INT1, PARENT("pll_int1"), CLK_DE_REG,
      pre_mod_apb_axi_gates, mod_div2, 0);
FPCLK(CLK_GE, "ge", CLK_PLL_INT1, PARENT("pll_int1"), CLK_GE_REG,
      pre_mod_apb_axi_gates, mod_div2, 0);
FPCLK(CLK_VE, "ve", CLK_PLL_INT1, PARENT("pll_int1"), CLK_VE_REG,
      pre_mod_apb_axi_gates, mod_div2, 0);
FPCLK(CLK_SID, "sid", CLK_OSC24M, PARENT("osc24m"), CLK_SID_REG, mod_apb_gates, no_div, 0);
FPCLK(CLK_RTC, "rtc", CLK_APB1, PARENT("apb1"), CLK_RTC_REG, apb_gates, no_div, 0);
FPCLK(CLK_GTC, "gtc", CLK_APB1, PARENT("apb1"), CLK_GTC_REG, apb_gates, no_div, 0);
FPCLK(CLK_I2C0, "i2c0", CLK_APB0, PARENT("apb0"), CLK_I2C0_REG, apb_gates, no_div, 0);
FPCLK(CLK_I2C1, "i2c1", CLK_APB0, PARENT("apb0"), CLK_I2C1_REG, apb_gates, no_div, 0);
FPCLK(CLK_XPWM0, "xpwm0", CLK_AHB1, PARENT("ahb1"), CLK_XPWM0_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM1, "xpwm1", CLK_AHB1, PARENT("ahb1"), CLK_XPWM1_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM2, "xpwm2", CLK_AHB1, PARENT("ahb1"), CLK_XPWM2_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM3, "xpwm3", CLK_AHB1, PARENT("ahb1"), CLK_XPWM3_REG, ahb_gates, no_div, 0);
FPCLK(CLK_ADCIM, "adcim", CLK_PLL_INT1, PARENT("pll_int1"), CLK_ADCIM_REG,
      pre_mod_apb_gates, mod_div1, 0);
FPCLK(CLK_GPAI, "gpai", CLK_AHB1, PARENT("ahb1"), CLK_GPAI_REG, apb_gates, no_div, 0);
FPCLK(CLK_RTP, "rtp", CLK_AHB1, PARENT("ahb1"), CLK_RTP_REG, apb_gates, no_div, 0);
FPCLK(CLK_TSEN, "tsen", CLK_AHB1, PARENT("ahb1"), CLK_TSEN_REG, apb_gates, no_div, 0);
FPCLK(CLK_CIR, "cir", CLK_APB1, PARENT("apb1"), CLK_CIR_REG, apb_gates, no_div, 0);
FPCLK(CLK_XPWM4, "xpwm4", CLK_AHB1, PARENT("ahb1"), CLK_XPWM4_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM5, "xpwm5", CLK_AHB1, PARENT("ahb1"), CLK_XPWM5_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM6, "xpwm6", CLK_AHB1, PARENT("ahb1"), CLK_XPWM6_REG, ahb_gates, no_div, 0);
FPCLK(CLK_XPWM7, "xpwm7", CLK_AHB1, PARENT("ahb1"), CLK_XPWM7_REG, ahb_gates, no_div, 0);

static const u8 cpu_src_sels[] = {
    CLK_OSC24M,
    CLK_PLL_INT0,
};

struct table_div cpu_div[] = {
    {
        .shift = -1,
        {
            .div = 1,
        }
    },
    {
        .shift = 0,
        {
            .width = 3,
        }
    },
};

static const u8 wdog_src_sels[] = {
    CLK_OSC24M,
    CLK_OSC32K,
};

struct table_div wdog_div[] = {
    {
        .shift = -1,
        {
            .div = 750,
        }
    },
    {
        .shift = -1,
        {
            .div = 1,
        }
    },
};

static const u8 out0_src_sels[] = {
    CLK_PLL_INT1,
    CLK_PLL_FRA1,
    CLK_PLL_FRA2,
};

struct table_div out0_div[] = {
    {
        .shift = 0,
        {
            .div = 8,
        }
    },
    {
        .shift = 0,
        {
            .div = 8,
        }
    },
    {
        .shift = 0,
        {
            .div = 8,
        }
    },
};

AUTHCLK(CLK_CPU, "cpu", cpu_src_sels, CLK_CPU_REG, CLK_WR_CFG_REG, 0xA1C, 20, 12, 16,
        mod_gates, 12, 2, cpu_div);
AUTHCLK(CLK_WDT, "wdog", wdog_src_sels, CLK_WDT_REG, CLK_WR_CFG_REG, 0xA1C, 20, 12, 16,
        mod_apb_gates, 12, 1, wdog_div);
AUTHCLK(CLK_OUT0, "out0", out0_src_sels, CLK_OUT0_REG, CLK_WR_CFG_REG, 0xA1C, 20, 12, 16,
        pre_mod_gates, 12, 2, out0_div);

/* Clock cfg array */
const struct aic_clk_comm_cfg *aic_clk_cfgs[AIC_CLK_NUM] = {
    /* Fixed rate clock */
    AIC_CLK_CFG(CLK_OSC24M),
    AIC_CLK_CFG(CLK_OSC32K),
    AIC_CLK_CFG(CLK_APB1),
    /* PLL clock */
    AIC_CLK_CFG(CLK_PLL_INT0),
    AIC_CLK_CFG(CLK_PLL_INT1),
    AIC_CLK_CFG(CLK_PLL_FRA0),
    AIC_CLK_CFG(CLK_PLL_FRA1),
    AIC_CLK_CFG(CLK_PLL_FRA2),
    /* system clock */
    AIC_CLK_CFG(CLK_AXI0),
    AIC_CLK_CFG(CLK_AHB0),
    AIC_CLK_CFG(CLK_AHB1),
    AIC_CLK_CFG(CLK_APB0),
    AIC_CLK_CFG(CLK_OUT0),
    /* Peripheral clock */
    AIC_CLK_CFG(CLK_SCLK),
    AIC_CLK_CFG(CLK_PIX),
    AIC_CLK_CFG(CLK_DMA),
    AIC_CLK_CFG(CLK_DCE),
    AIC_CLK_CFG(CLK_USBD),
    AIC_CLK_CFG(CLK_USBH0),
    AIC_CLK_CFG(CLK_USB_PHY0),
    AIC_CLK_CFG(CLK_CANFD0),
    AIC_CLK_CFG(CLK_XSPI),
    AIC_CLK_CFG(CLK_QSPI0),
    AIC_CLK_CFG(CLK_QSPI1),
    AIC_CLK_CFG(CLK_QSPI2),
    AIC_CLK_CFG(CLK_QSPI3),
    AIC_CLK_CFG(CLK_QSPI4),
    AIC_CLK_CFG(CLK_QSPI5),
    AIC_CLK_CFG(CLK_SDMC0),
    AIC_CLK_CFG(CLK_SDMC1),
    AIC_CLK_CFG(CLK_SYSCFG),
    AIC_CLK_CFG(CLK_SPIENC),
    AIC_CLK_CFG(CLK_PWMCS),
    AIC_CLK_CFG(CLK_MTOP),
    AIC_CLK_CFG(CLK_I2S0),
    AIC_CLK_CFG(CLK_CODEC),
    AIC_CLK_CFG(CLK_GPIO),
    AIC_CLK_CFG(CLK_UART0),
    AIC_CLK_CFG(CLK_UART1),
    AIC_CLK_CFG(CLK_UART2),
    AIC_CLK_CFG(CLK_UART3),
    AIC_CLK_CFG(CLK_UART4),
    AIC_CLK_CFG(CLK_RGB),
    AIC_CLK_CFG(CLK_MIPIDSI),
    AIC_CLK_CFG(CLK_DVP),
    AIC_CLK_CFG(CLK_DE),
    AIC_CLK_CFG(CLK_GE),
    AIC_CLK_CFG(CLK_VE),
    AIC_CLK_CFG(CLK_SID),
    AIC_CLK_CFG(CLK_CPU),
    AIC_CLK_CFG(CLK_WDT),
    AIC_CLK_CFG(CLK_RTC),
    AIC_CLK_CFG(CLK_GTC),
    AIC_CLK_CFG(CLK_I2C0),
    AIC_CLK_CFG(CLK_I2C1),
    AIC_CLK_CFG(CLK_XPWM0),
    AIC_CLK_CFG(CLK_XPWM1),
    AIC_CLK_CFG(CLK_XPWM2),
    AIC_CLK_CFG(CLK_XPWM3),
    AIC_CLK_CFG(CLK_ADCIM),
    AIC_CLK_CFG(CLK_GPAI),
    AIC_CLK_CFG(CLK_RTP),
    AIC_CLK_CFG(CLK_TSEN),
    AIC_CLK_CFG(CLK_CIR),
    AIC_CLK_CFG(CLK_XPWM4),
    AIC_CLK_CFG(CLK_XPWM5),
    AIC_CLK_CFG(CLK_XPWM6),
    AIC_CLK_CFG(CLK_XPWM7),
};
