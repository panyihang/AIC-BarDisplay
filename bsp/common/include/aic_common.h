/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_COMMON_H__
#define __AIC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtconfig.h>
#include <stddef.h>
#include <stdbool.h>
#include <aic_arch.h>

/* Luban-Lite version information */
#ifdef LUBAN_LITE_2_0
#define LL_VERSION      1
#define LL_SUBVERSION   3
#define LL_REVISION     2
#else
#define LL_VERSION      1
#define LL_SUBVERSION   3
#define LL_REVISION     2
#endif

typedef __signed__ char     s8;
typedef unsigned char       u8;

typedef __signed__ short    s16;
typedef unsigned short      u16;

typedef __signed__ int      s32;
typedef unsigned int        u32;

#ifdef ARCH_64BIT
typedef __signed__ long     s64;
typedef unsigned long       u64;
#define FMT_d64             "%ld"
#define FMT_x64             "%lx"
#define BITS_PER_LONG       64
#else
typedef __signed__ long long s64;
typedef unsigned long long   u64;
#define FMT_d64              "%lld"
#define FMT_x64              "%llx"
#define BITS_PER_LONG        32
#endif

typedef u32 dma_addr_t;
typedef u32 phy_addr_t;
typedef unsigned long ulong;
typedef unsigned long ptr_t;

#define U8_MAX  ((u8)~0U)
#define S8_MAX  ((s8)(U8_MAX >> 1))
#define S8_MIN  ((s8)(-S8_MAX - 1))
#define U16_MAX ((u16)~0U)
#define S16_MAX ((s16)(U16_MAX >> 1))
#define S16_MIN ((s16)(-S16_MAX - 1))
#define U32_MAX ((u32)~0U)
#define U32_MIN ((u32)0)
#define S32_MAX ((s32)(U32_MAX >> 1))
#define S32_MIN ((s32)(-S32_MAX - 1))
#define U64_MAX ((u64)~0ULL)
#define S64_MAX ((s64)(U64_MAX >> 1))
#define S64_MIN ((s64)(-S64_MAX - 1))

/* Character code support macros */
#define aic_in_range(c, lo, up)  ((u8)c >= lo && (u8)c <= up)
#define aic_isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#define aic_islower(c)  ((c) >= 'a' && (c) <= 'z')
#define aic_isdigit(c)  ((c) >= '0' && (c) <= '9')
#define aic_isxdigit(c) (aic_isdigit(c) || aic_in_range(c, 'a', 'f') \
                                        || aic_in_range(c, 'A', 'F'))
#define aic_isspace(c)  ((c) == ' ' || (c) == '\f' || (c) == '\n' \
                        || (c) == '\r' || (c) == '\t' || (c) == '\v')

#define aic_convert_tx_dlymode(c, d)  (((c > 4) ? 0 : c) | (d ? 1UL << 14 : 0))

#ifndef CHECK_PARAM
#define CHECK_PARAM(x, ret) \
    do { \
        if (!(x)) { \
            return ret; \
        } \
    } while (0)
#endif

#ifndef CHECK_PARAM_RET
#define CHECK_PARAM_RET(x) \
    do { \
        if (!(x)) { \
            return; \
        } \
    } while (0)
#endif

/* NOTE: pr_err() from aic_log.h is required */
#ifndef CHECK_RET_WITH_GOTO
#define CHECK_RET_WITH_GOTO(x, label) \
    do { \
        if (!(x)) { \
            pr_err("%s, %s()%d failed.", __FILE__, __FUNCTION__, __LINE__); \
            goto label; \
        } \
    } while (0)
#endif

#ifndef CHECK_RET_WITH_RET
#define CHECK_RET_WITH_RET(x, ret) \
    do { \
        if (!(x)) { \
            pr_err("%s, %s()%d failed.", __FILE__, __FUNCTION__, __LINE__); \
            return ret; \
        } \
    } while (0)
#endif

/* Exception Dealt With */
#define BUG() do {                                                            \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        while (1) {}                                                          \
    } while (0)

#if (!defined(likely))
#define likely(x)             __builtin_expect((long)!!(x), 1L)
#endif
#if (!defined(unlikely))
#define unlikely(x)           __builtin_expect((long)!!(x), 0L)
#endif

/* IOCTL interface commands */
#ifndef _IOWR
# define _IOWR(x, y, z)     (((x) << 8) | y)
#endif
#ifndef _IOW
# define _IOW(x, y, z)      (((x) << 8) | y)
#endif
#ifndef _IOR
# define _IOR(x, y, z)      (((x) << 8) | y)
#endif
#ifndef _IO
# define _IO(x, y)          (((x) << 8) | y)
#endif

#ifdef asm
#undef asm
#endif
#define asm __asm__

#define ARCH_DMA_MINALIGN   CACHE_LINE_SIZE

#if defined(KERNEL_RTTHREAD)
#define CONFIG_SYSTICK_HZ RT_TICK_PER_SECOND
#elif defined(KERNEL_FREERTOS)
#ifdef FREERTOS_TICK_RATE_HZ
#define CONFIG_SYSTICK_HZ FREERTOS_TICK_RATE_HZ
#else
#define CONFIG_SYSTICK_HZ 200
#endif
#elif defined(KERNEL_BAREMETAL)
#define CONFIG_SYSTICK_HZ 100
#elif defined(KERNEL_UCOS_II)
#define CONFIG_SYSTICK_HZ UCOS_TICKS_PER_SEC
#endif

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) \
    ({ \
        const __typeof__(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#ifndef ABS
#define ABS(x)  ((x) < 0 ? -(x) : (x))
#endif

#ifndef max
#define max(x, y) ({				\
	__typeof__(x) _max1 = (x);			\
	__typeof__(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })
#endif

#ifndef min
#define min(x, y) ({				\
	__typeof__(x) _min1 = (x);			\
	__typeof__(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })
#endif

#define do_div(n, base) \
    ({ \
        unsigned int __base = (base); \
        unsigned int __rem; \
        __rem = ((u64)(n)) % __base; \
        (n)   = ((u64)(n)) / __base; \
        __rem; \
    })

/*
 * Divide positive or negative dividend by positive or negative divisor
 * and round to closest integer. Result is undefined for negative
 * divisors if the dividend variable type is unsigned and for negative
 * dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor) \
    ({ \
        __typeof__(x) __x       = x; \
        __typeof__(divisor) __d = divisor; \
        (((__typeof__(x))-1) > 0 || ((__typeof__(divisor))-1) > 0 || \
         (((__x) > 0) == ((__d) > 0))) ? \
                (((__x) + ((__d) / 2)) / (__d)) : \
                (((__x) - ((__d) / 2)) / (__d)); \
    })
/*
 * Same as above but for u64 dividends. divisor must be a 32-bit
 * number.
 */
#define DIV_ROUND_CLOSEST_ULL(x, divisor) \
    ({ \
        __typeof__(divisor) __d     = divisor; \
        unsigned long long _tmp = (x) + (__d) / 2; \
        do_div(_tmp, __d); \
        _tmp; \
    })

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define ROUNDUP(a, b)      ((((a)-1) / (b) + 1) * (b))
#define ROUND(a, b)        (((a) + (b)-1) & ~((b)-1))
#define ROUNDDOWN(a, b)    ((a) & ~((b) - 1))

#define roundup(x, y) (                 \
{                           \
    const __typeof__(y) __y = y;            \
    (((x) + (__y - 1)) / __y) * __y;        \
}                           \
)
#define rounddown(x, y) (               \
{                           \
    __typeof__(x) __x = (x);                \
    __x - (__x % (y));              \
}                           \
)

/* NOTE: memset() from <string.h> is required */
#define EXPAND_BCD_VER(v)    ({ \
    static char ver[8]; \
    int i = 0; \
    memset(ver, 0, 8); \
    if (v & 0xF000) \
        ver[i++] = ((v >> 12) & 0xF) + 0x30; \
    ver[i++] = (((v) >> 8) & 0xF) + 0x30; \
    ver[i++] = '.'; \
    ver[i++] = (((v) >> 4) & 0xF) + 0x30; \
    ver[i]   = ((v) & 0xF) + 0x30; \
    ver; \
})

#ifdef ALIGN
#undef ALIGN
#endif
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN_UP(x,a)           __ALIGN_MASK((x),(__typeof__(x))(a)-1)
#define ALIGN_DOWN(x, a)        ((x) & (~((__typeof__(x))(a)-1)))

#define IS_ALIGNED(x, a) (((x) & ((__typeof__(x))(a)-1)) == 0)

#define PAD_COUNT(s, pad)   (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad)    (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)        \
    char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
              + (align - 1)];                   \
                                    \
    type *name = (type *)ALIGN_UP((uintptr_t)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)     \
    ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)     \
    ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)          \
    ALLOC_ALIGN_BUFFER(type, name, size, ARCH_DMA_MINALIGN)

#define BANNER1                                         "\n" \
    "     _         _   ___        ___ _     _           \n" \
    "    / \\   _ __| |_|_ _|_ __  / __| |__ (_)_ __     \n"
#define BANNER2                                              \
    "   / _ \\ | '__| __|| || '_ \\| |  | '_ \\| | '_ \\ \n" \
    "  / ___ \\| |  | |_ | || | | | |__| | | | | |_) |   \n"
#define BANNER3                                             \
    " /_/   \\_\\_|   \\__|___|_| |_|\\___|_| |_|_| .__/ \n"\
    "                                         |_|      \n\n"
#define BANNER          BANNER1 BANNER2 BANNER3

#define __va(x) ((void *)((unsigned long) (x)))
#define __pa(x) ((unsigned long) (x))

#ifndef __USED
#define __USED __attribute__((used))
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif
#ifndef __PACKED
#define __PACKED __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_UNION
#define __PACKED_UNION union __attribute__((packed, aligned(1)))
#endif
#ifndef __ALIGNED
#define __ALIGNED(x) __attribute__((aligned(x)))
#endif

#define  ATTRIBUTE_ISR __attribute__ ((interrupt ("machine")))

/* static define type: */

#ifdef AIC_TCM_EN
#define TCM_DATA_DEFINE __attribute__((section(".tcm_data")))
#define TCM_CODE_DEFINE __attribute__((section(".tcm_code")))
#else
#define TCM_DATA_DEFINE
#define TCM_CODE_DEFINE
#endif

#ifdef AIC_SRAM_CMA_EN
#define SRAM_CMA_DATA_DEFINE __attribute__((section(".sram_cma_data")))
#else
#define SRAM_CMA_DATA_DEFINE
#endif

#ifdef AIC_SRAM_SW_EN
#define SRAM_SW_DATA_DEFINE __attribute__((section(".sram_sw_data")))
#else
#define SRAM_SW_DATA_DEFINE
#endif

#ifdef AIC_SRAM1_CMA_EN
#define SRAM1_CMA_DATA_DEFINE __attribute__((section(".sram1_cma_data")))
#else
#define SRAM1_CMA_DATA_DEFINE
#endif

#ifdef AIC_SRAM1_SW_EN
#define SRAM1_SW_DATA_DEFINE __attribute__((section(".sram1_sw_data")))
#else
#define SRAM1_SW_DATA_DEFINE
#endif

#ifdef AIC_PSRAM_CMA_EN
#define PSRAM_CMA_DATA_DEFINE __attribute__((section(".psram_cma_data")))
#else
#define PSRAM_CMA_DATA_DEFINE
#endif

#ifdef AIC_PSRAM_SW_EN
#define PSRAM_SW_DATA_DEFINE __attribute__((section(".psram_sw_data")))
#else
#define PSRAM_SW_DATA_DEFINE
#endif

#ifdef AIC_DRAM_CMA_EN
#define DRAM_CMA_DATA_DEFINE __attribute__((section(".dram_cma_data")))
#else
#define DRAM_CMA_DATA_DEFINE
#endif

#ifdef FPGA_BOARD_ARTINCHIP
#define FPGA_EXT_DATA_DEFINE __attribute__((section(".fpga_ext_data")))
#endif

/* dynamic malloc type: */

typedef enum {
    MEM_DEFAULT = 0,
#ifdef AIC_TCM_EN
    MEM_ITCM,
    MEM_DTCM,
#endif
#ifdef AIC_SRAM_CMA_EN
    MEM_SRAM_CMA,
#endif
#ifdef AIC_SRAM_SW_EN
    MEM_SRAM_SW,
#endif
#if defined(AIC_SRAM0_SW_EN) && !defined(AIC_DEFAULT_SYS_HEAP_SRAM)
    MEM_SRAM0_SW,
#endif
#ifdef AIC_SRAM1_CMA_EN
    MEM_SRAM1_CMA,
#endif
#ifdef AIC_SRAM1_SW_EN
    MEM_SRAM1_SW,
#endif
#ifdef AIC_PSRAM_CMA_EN
    MEM_PSRAM_CMA,
#endif
#if defined(AIC_PSRAM_SW_EN) && !defined(AIC_DEFAULT_SYS_HEAP_PSRAM)
    MEM_PSRAM_SW,
#endif
#ifdef AIC_DRAM_CMA_EN
    MEM_DRAM_CMA,
#endif
#ifdef AIC_BOOTLOADER
    MEM_RESERVED,
#endif
    MAX_MEM_REGION,
} aic_mem_region_t;

typedef struct {
    char * name;
    aic_mem_region_t type;
    size_t start;
    size_t end;
} heap_def_t;

/* cma */
#ifdef AIC_BOOTLOADER
#define MEM_CMA MEM_RESERVED
#define CMA_DATA_DEFINE
#else
#if defined(AIC_DRAM_CMA_EN)
#define MEM_CMA         MEM_DRAM_CMA
#define CMA_DATA_DEFINE DRAM_CMA_DATA_DEFINE
#elif defined(AIC_PSRAM_CMA_EN)
#define MEM_CMA         MEM_PSRAM_CMA
#define CMA_DATA_DEFINE PSRAM_CMA_DATA_DEFINE
#elif defined(AIC_SRAM1_CMA_EN)
#define MEM_CMA         MEM_SRAM1_CMA
#define CMA_DATA_DEFINE SRAM1_CMA_DATA_DEFINE
#else
#define MEM_CMA MEM_DEFAULT
#define CMA_DATA_DEFINE
#endif
#endif

/* heap size define is ld.s */

extern size_t __heap_start;
extern size_t __heap_end;
extern size_t __cma_heap_start;
extern size_t __cma_heap_end;
#ifdef AIC_TCM_EN
extern size_t __itcm_heap_start;
extern size_t __itcm_heap_end;
extern size_t __dtcm_heap_start;
extern size_t __dtcm_heap_end;
#endif
#ifdef AIC_SRAM_CMA_EN
extern size_t __sram_cma_heap_start;
extern size_t __sram_cma_heap_end;
#endif
#ifdef AIC_SRAM_SW_EN
extern size_t __sram_sw_heap_start;
extern size_t __sram_sw_heap_end;
#endif
#ifdef AIC_SRAM0_SW_EN
extern size_t __sram_s0_sw_heap_start;
extern size_t __sram_s0_sw_heap_end;
#endif
#ifdef AIC_SRAM1_CMA_EN
extern size_t __sram_s1_cma_heap_start;
extern size_t __sram_s1_cma_heap_end;
#endif
#ifdef AIC_SRAM1_SW_EN
extern size_t __sram_s1_sw_heap_start;
extern size_t __sram_s1_sw_heap_end;
#endif
#ifdef AIC_PSRAM_CMA_EN
extern size_t __psram_cma_heap_start;
extern size_t __psram_cma_heap_end;
#endif
#ifdef AIC_PSRAM_SW_EN
extern size_t __psram_sw_heap_start;
extern size_t __psram_sw_heap_end;
#endif
#ifdef AIC_DRAM_CMA_EN
extern size_t __dram_cma_heap_start;
extern size_t __dram_cma_heap_end;
#endif

#include <aic_time.h>

#ifdef __cplusplus
}
#endif

#endif /* __AIC_COMMON_H__ */
