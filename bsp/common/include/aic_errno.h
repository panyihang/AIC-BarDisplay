/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_ERRNO_H__
#define __AIC_ERRNO_H__

#define __LINUX_ERRNO_EXTENSIONS__

#include <errno.h>

#define EOK          0 /* Error State : OK */

/* User define err no */

#define ERRNO_DRV_START 0X80
/* driver General error codes */
typedef enum
{
    DRV_ERROR = ERRNO_DRV_START,   ///< Unspecified error
    DRV_ERROR_BUSY,                ///< Driver is busy
    DRV_ERROR_TIMEOUT,             ///< Timeout occurred
    DRV_ERROR_UNSUPPORTED,         ///< Operation not supported
    DRV_ERROR_PARAMETER,           ///< Parameter error
    DRV_ERROR_SPECIFIC             ///< Start of driver specific errors
} drv_err_e;

/** driver module id definition*/

#define AIC_DRV_ERRNO_BASE                  0x81000000
#define AIC_DRV_ERRNO_GPIO_BASE             0x81010000
#define AIC_DRV_ERRNO_USART_BASE            0x81020000
#define AIC_DRV_ERRNO_SPI_BASE              0x81030000
#define AIC_DRV_ERRNO_IIC_BASE              0x81040000
#define AIC_DRV_ERRNO_PWM_BASE              0x81050000
#define AIC_DRV_ERRNO_RTC_BASE              0x81060000
#define AIC_DRV_ERRNO_TIMER_BASE            0x81070000
#define AIC_DRV_ERRNO_WDT_BASE              0x81080000
#define AIC_DRV_ERRNO_AES_BASE              0x81090000
#define AIC_DRV_ERRNO_CRC_BASE              0x810A0000
#define AIC_DRV_ERRNO_RSA_BASE              0x810B0000
#define AIC_DRV_ERRNO_SHA_BASE              0x810C0000
#define AIC_DRV_ERRNO_TRNG_BASE             0x810D0000
#define AIC_DRV_ERRNO_EFLASH_BASE           0x810E0000
#define AIC_DRV_ERRNO_DMA_BASE              0x810F0000
#define AIC_DRV_ERRNO_NORFLASH_BASE         0x81100000
#define AIC_DRV_ERRNO_INTC_BASE             0x81110000
#define AIC_DRV_ERRNO_SPU_BASE              0x81120000
#define AIC_DRV_ERRNO_ADC_BASE              0x81130000
#define AIC_DRV_ERRNO_PMU_BASE              0x81140000
#define AIC_DRV_ERRNO_BMU_BASE              0x81150000
#define AIC_DRV_ERRNO_ETB_BASE              0x81160000
#define AIC_DRV_ERRNO_I2S_BASE              0x81170000
#define AIC_DRV_ERRNO_USI_BASE              0x81180000
#define AIC_DRV_ERRNO_SPIFLASH_BASE         0x81190000
#define AIC_DRV_ERRNO_ACMP_BASE             0x811A0000
#define AIC_DRV_ERRNO_MAILBOX_BASE          0x811B0000
#define AIC_DRV_ERRNO_EFUSEC_BASE           0x811C0000
#define AIC_DRV_ERRNO_CODEC_BASE            0x811D0000
#define AIC_DRV_ERRNO_DPU_BASE              0x811E0000
#define AIC_DRV_ERRNO_VPU_BASE              0x811F0000
#define AIC_DRV_ERRNO_CAMERA_BASE           0x81200000
#define AIC_DRV_ERRNO_MIPI_CSI_BASE         0x81210000
#define AIC_DRV_ERRNO_MIPI_CSI_READER_BASE  0x81220000
#define AIC_DRV_ERRNO_GMAC_BASE             0x81230000
#define AIC_DRV_ERRNO_ETHPHY_BASE           0x81240000
#define AIC_DRV_ERRNO_QW_EFUSE_BASE         0x81250000
#define AIC_DRV_ERRNO_RESET_BASE            0x81260000

#endif /* __AIC_ERRNO_H__ */
