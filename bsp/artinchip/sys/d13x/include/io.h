/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IO_H__
#define IO_H__

#include "aic_common.h"

static inline void __raw_writeb(u8 val, volatile void *addr)
{
    asm volatile("sb %0, 0(%1)" : : "r"(val), "r"(addr));
}

static inline u8 __raw_readb(const volatile void *addr)
{
    u8 val;

    asm volatile("lb %0, 0(%1)" : "=r"(val) : "r"(addr));
    return val;
}

static inline void __raw_writel(u32 val, volatile void *addr)
{
    asm volatile("sw %0, 0(%1)" : : "r"(val), "r"(addr));
}

static inline u32 __raw_readl(const volatile void *addr)
{
    u32 val;

    asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr));
    return val;
}

#define mmiowb_set_pending() \
    do { \
    } while (0)
#define __io_br()       do { } while (0)
#define __io_ar(v)      __asm__ __volatile__("fence i,r" : : : "memory");
#define __io_bw()       __asm__ __volatile__("fence w,o" : : : "memory");
#define __io_aw()       mmiowb_set_pending()

#endif // AIC_CONFIG_H__
