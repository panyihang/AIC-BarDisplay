/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_RESET_ID_H__
#define __AIC_RESET_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

enum reset_id {
    RESET_WDT,
    RESET_DMA,
    RESET_DCE,
    RESET_USBD,
    RESET_USBH0,
    RESET_USBPHY0,
    RESET_CANFD0,
    RESET_XSPI,
    RESET_QSPI0,
    RESET_QSPI1,
    RESET_QSPI2,
    RESET_QSPI3,
    RESET_QSPI4,
    RESET_QSPI5,
    RESET_SDMC0,
    RESET_SDMC1,
    RESET_SPIENC,
    RESET_PWMCS,
    RESET_MTOP,
    RESET_I2S,
    RESET_AUDIO,
    RESET_GPIO,
    RESET_UART0,
    RESET_UART1,
    RESET_UART2,
    RESET_UART3,
    RESET_UART4,
    RESET_RGB,
    RESET_MIPIDSI,
    RESET_DVP,
    RESET_DE,
    RESET_GE,
    RESET_VE,
    RESET_GTC,
    RESET_I2C0,
    RESET_I2C1,
    RESET_XPWM0,
    RESET_XPWM1,
    RESET_XPWM2,
    RESET_XPWM3,
    RESET_ADCIM,
    RESET_GPAI,
    RESET_RTP,
    RESET_TSEN,
    RESET_CIR,
    RESET_XPWM4,
    RESET_XPWM5,
    RESET_XPWM6,
    RESET_XPWM7,
    RESET_NUMBER,
};

#ifdef __cplusplus
}
#endif

#endif /* __AIC_RESET_ID_H__ */
