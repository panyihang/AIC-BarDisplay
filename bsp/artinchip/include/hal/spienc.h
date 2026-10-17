/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Hao Xiong <hao.xiong@artinchip.com>
 */

#ifndef _AIC_SPIENC_H_
#define _AIC_SPIENC_H_

#include <hal_spienc.h>

static inline int spienc_init(void)
{
    return hal_spienc_init();
}

#ifdef AIC_SPIENC_DRV_V20
static inline void spienc_set_work_mode(int mode)
{
    hal_spienc_set_work_mode(mode);
}

static inline void spienc_select_alg(int algo)
{
    hal_spienc_select_alg(algo);
}
static inline void spienc_select_alg_key(int key_src)
{
    hal_spienc_select_alg_key(key_src);
}

static inline void spienc_set_alg_key(u8 *key)
{
    hal_spienc_set_alg_key(key);
}

static inline void spienc_set_alg_iv(u8 *iv)
{
    hal_spienc_set_alg_iv(iv);
}

static inline void spienc_start_alg(u8 *data, u32 dlen, u8 *out, u32 olen)
{
    hal_spienc_start_alg(data, dlen, out, olen);
}
#endif

static inline void spienc_set_cfg(u32 spi_bus, u32 addr, u32 cpos, u32 clen)
{
    hal_spienc_set_cfg(spi_bus, addr, cpos, clen);
}

static inline void spienc_set_bypass(int status)
{
    hal_spienc_set_bypass(status);
}

static inline void spienc_select_tweak(int select)
{
    hal_spienc_select_tweak(select);
}

static inline void spienc_xip_enable(void)
{
    hal_spienc_xip_enable();
}

static inline void spienc_xip_disable(void)
{
    hal_spienc_xip_disable();
}

static inline void spienc_start(void)
{
    hal_spienc_start();
}

static inline void spienc_stop(void)
{
    hal_spienc_stop();
}

static inline int spienc_check_empty(void)
{
    return hal_spienc_check_empty();
}

#endif
