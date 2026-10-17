/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_IO_H__
#define __AIC_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <io.h>

#ifndef BIT
#define BIT(s) (1U << (s))
#define BIT_UL(s) (1UL << (s))
#endif

#define GENMASK(h, l)       (((~(0U)) - ((1U) << (l)) + 1) & \
                             (~(0U) >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK_UL(h, l)    (((~(0UL)) - ((1UL) << (l)) + 1) & \
                             (~(0UL) >> (BITS_PER_LONG - 1 - (h))))
#define uswap_16(x) \
    ((((x) & 0xff00) >> 8) | \
     (((x) & 0x00ff) << 8))
#define uswap_32(x) \
    ((((x) & 0xff000000) >> 24) | \
     (((x) & 0x00ff0000) >>  8) | \
     (((x) & 0x0000ff00) <<  8) | \
     (((x) & 0x000000ff) << 24))

#define cpu_to_le16(val) (val)
#define cpu_to_le32(val) (val)

#define le16_to_cpu(val) (val)
#define le32_to_cpu(val) (val)

#define cpu_to_be16(val) uswap_16(val)
#define cpu_to_be32(val) uswap_32(val)

#define be16_to_cpu(val) uswap_16(val)
#define be32_to_cpu(val) uswap_32(val)

#define readb_cpu(c)    ({ u8  __r = __raw_readb(c); __r; })
#define readl_cpu(c)    ({ u32 __r = le32_to_cpu(__raw_readl(c)); __r; })

#define writeb_cpu(v, c)    ((void)__raw_writeb((v), (c)))
#define writel_cpu(v, c)    ((void)__raw_writel((u32)cpu_to_le32(v), (c)))

#define readb(c) \
    ({ \
        u8  __v; \
        __io_br(); \
        __v = readb_cpu((volatile void *)c); \
        __io_ar(__v); __v; \
    })

#define readl(c) \
    ({ \
        u32 __v; \
        __io_br(); \
        __v = readl_cpu((volatile void *)c); \
        __io_ar(__v); \
        __v; \
    })

#define writeb(v, c) \
    ({ \
        __io_bw(); \
        writeb_cpu((v), (volatile void *)(c)); \
        __io_aw(); \
    })

#define writel(v, c) \
    ({ \
        __io_bw(); \
        writel_cpu((v), ((volatile void *)c)); \
        __io_aw(); \
    })

/* Some common bit operation of a (long type) register */
#define writel_clrbits(mask, addr)       writel(readl(addr) & ~(mask), addr)
#define writel_clrbit(bit, addr)         writel_clrbits(bit, addr)
#define writel_bits(val, mask, shift, addr) \
    ({ \
        if (val) \
            writel((readl(addr) & ~(mask)) | ((val) << (shift)), addr); \
        else \
            writel_clrbits(mask, addr); \
    })
#define writel_bit(bit, addr)           writel(readl(addr) | bit, addr)
#define readl_bits(mask, shift, addr)   ((readl(addr) & (mask)) >> (shift))
#define readl_bit(bit, addr)            ((readl(addr) & bit) ? 1 : 0)

/* Some common bit operation of a variable */
#define clrbits(mask, cur)              (cur &= ~(mask))
#define clrbit(bit, cur)                clrbits(bit, cur)
#define setbits(val, mask, shift, cur) \
    ({ \
        if (val) \
            (cur = (cur & ~(mask)) | ((val) << (shift))); \
        else \
            clrbits(mask, cur); \
    })
#define setbit(bit, cur)                (cur |= bit)
#define getbits(mask, shift, cur)       ((cur & (mask)) >> (shift))
#define getbit(bit, cur)                (cur & bit ? 1 : 0)

#define bitrev8(x)   \
({                  \
    u8 ___x = x;            \
    ___x = (___x >> 4) | (___x << 4);   \
    ___x = ((___x & (u8)0xCCU) >> 2) | ((___x & (u8)0x33U) << 2);   \
    ___x = ((___x & (u8)0xAAU) >> 1) | ((___x & (u8)0x55U) << 1);   \
    ___x;                               \
})

#ifdef __cplusplus
}
#endif

#endif /* __AIC_IO_H__ */
