/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"

const struct aic_reset_signal aic_reset_signals[RESET_NUMBER] = {
    [RESET_WDT]         = { CLK_WDT_REG, BIT(24), AIC_RESET_AUTH_REQUEST },
    [RESET_DMA]         = { CLK_DMA_REG, BIT(24) },
    [RESET_DCE]         = { CLK_DCE_REG, BIT(24) },
    [RESET_USBD]        = { CLK_USB_DEV_REG, BIT(24) },
    [RESET_USBH0]       = { CLK_USB_HOST_REG, BIT(24) },
    [RESET_USBPHY0]     = { CLK_USB_PHY_REG, BIT(24) },
    [RESET_CANFD0]      = { CLK_CANFD0_REG, BIT(24) },
    [RESET_XSPI]        = { CLK_XSPI_REG, BIT(24) },
    [RESET_QSPI0]       = { CLK_QSPI0_REG, BIT(24) },
    [RESET_QSPI1]       = { CLK_QSPI1_REG, BIT(24) },
    [RESET_QSPI2]       = { CLK_QSPI0_REG, BIT(24) },
    [RESET_QSPI3]       = { CLK_QSPI1_REG, BIT(24) },
    [RESET_QSPI4]       = { CLK_QSPI0_REG, BIT(24) },
    [RESET_QSPI5]       = { CLK_QSPI1_REG, BIT(24) },
    [RESET_SDMC0]       = { CLK_SDMC0_REG, BIT(24) },
    [RESET_SDMC1]       = { CLK_SDMC1_REG, BIT(24) },
    [RESET_SPIENC]      = { CLK_SPIENC_REG, BIT(24) },
    [RESET_PWMCS]       = { CLK_PWMCS_REG, BIT(24) },
    [RESET_MTOP]        = { CLK_MTOP_REG, BIT(24) },
    [RESET_I2S]         = { CLK_I2S_REG, BIT(24) },
    [RESET_AUDIO]       = { CLK_AUDIO_REG, BIT(24) },
    [RESET_GPIO]        = { CLK_GPIO_REG, BIT(24) },
    [RESET_UART0]       = { CLK_UART0_REG, BIT(24) },
    [RESET_UART1]       = { CLK_UART1_REG, BIT(24) },
    [RESET_UART2]       = { CLK_UART2_REG, BIT(24) },
    [RESET_UART3]       = { CLK_UART3_REG, BIT(24) },
    [RESET_UART4]       = { CLK_UART4_REG, BIT(24) },
    [RESET_RGB]         = { CLK_LCD_REG, BIT(24) },
    [RESET_MIPIDSI]     = { CLK_MIPI_DSI_REG, BIT(24) },
    [RESET_DVP]         = { CLK_DVP_REG, BIT(24) },
    [RESET_DE]          = { CLK_DE_REG, BIT(24) },
    [RESET_GE]          = { CLK_GE_REG, BIT(24) },
    [RESET_VE]          = { CLK_VE_REG, BIT(24) },
    [RESET_GTC]         = { CLK_GTC_REG, BIT(24) },
    [RESET_I2C0]        = { CLK_I2C0_REG, BIT(24) },
    [RESET_I2C1]        = { CLK_I2C1_REG, BIT(24) },
    [RESET_XPWM0]       = { CLK_XPWM0_REG, BIT(24) },
    [RESET_XPWM1]       = { CLK_XPWM1_REG, BIT(24) },
    [RESET_XPWM2]       = { CLK_XPWM2_REG, BIT(24) },
    [RESET_XPWM3]       = { CLK_XPWM3_REG, BIT(24) },
    [RESET_ADCIM]       = { CLK_ADCIM_REG, BIT(24) },
    [RESET_GPAI]        = { CLK_GPAI_REG, BIT(24) },
    [RESET_RTP]         = { CLK_RTP_REG, BIT(24) },
    [RESET_TSEN]        = { CLK_TSEN_REG, BIT(24) },
    [RESET_CIR]         = { CLK_CIR_REG, BIT(24) },
    [RESET_XPWM4]       = { CLK_XPWM4_REG, BIT(24) },
    [RESET_XPWM5]       = { CLK_XPWM5_REG, BIT(24) },
    [RESET_XPWM6]       = { CLK_XPWM6_REG, BIT(24) },
    [RESET_XPWM7]       = { CLK_XPWM7_REG, BIT(24) },
};
