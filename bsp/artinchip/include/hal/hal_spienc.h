/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#ifndef _AIC_HAL_SPIENC_H_
#define _AIC_HAL_SPIENC_H_

#include <aic_core.h>

#define AIC_SPIENC_USER_TWEAK 0
#define AIC_SPIENC_HW_TWEAK   1

#define AIC_SPIENC_BYPASS_ENABLE  1
#define AIC_SPIENC_BYPASS_DISABLE 0


#define AIC_SPIENC_BUS_MODE 0
#define AIC_SPIENC_IND_MODE 1

#define AIC_SPIENC_AES_128_CTR 0
#define AIC_SPIENC_HMAC_SHA256 1

#define AIC_SPIENC_KEY_SRC_EFUSE 0
#define AIC_SPIENC_KEY_SRC_USER  1

#define IOC_TYPE_SPIE 'E'
#define AIC_SPIENC_IOCTL_CRYPT_CFG \
    _IOW(IOC_TYPE_SPIE, 0x10, struct spienc_crypt_cfg)
#define AIC_SPIENC_IOCTL_START        _IOW(IOC_TYPE_SPIE, 0x11, u32)
#define AIC_SPIENC_IOCTL_STOP         _IOW(IOC_TYPE_SPIE, 0x12, u32)
#define AIC_SPIENC_IOCTL_CHECK_EMPTY  _IOW(IOC_TYPE_SPIE, 0x13, u32)
#define AIC_SPIENC_IOCTL_TWEAK_SELECT _IOW(IOC_TYPE_SPIE, 0x14, u32)

int hal_spienc_init(void);
#ifdef AIC_SPIENC_DRV_V20
void hal_spienc_set_work_mode(int mode);
void hal_spienc_select_alg(int algo);
void hal_spienc_select_alg_key(int key_src);
void hal_spienc_set_alg_key(u8 *key);
void hal_spienc_set_alg_iv(u8 *iv);
void hal_spienc_start_alg(u8 *data, u32 dlen, u8 *out, u32 olen);
#endif
void hal_spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen);
void hal_spienc_set_bypass(int status);
void hal_spienc_select_tweak(int select);
void hal_spienc_xip_enable(void);
void hal_spienc_xip_disable(void);
void hal_spienc_start(void);
void hal_spienc_stop(void);
int hal_spienc_check_empty(void);

#endif
