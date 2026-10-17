/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __DFS_FOR_BAREMETAL_H__
#define __DFS_FOR_BAREMETAL_H__

#include "rtconfig.h"
#include "aic_common.h"
#include "aic_log.h"

#define DFS_USING_WORKDIR               1
/* RT-Thread error code definitions */
#define RT_EOK                          0  /**< There is no error */
#define RT_ERROR                        1  /**< A generic error happens */
#define RT_ETIMEOUT                     2  /**< Timed out */
#define RT_EFULL                        3  /**< The resource is full */
#define RT_EEMPTY                       4  /**< The resource is empty */
#define RT_ENOMEM                       5  /**< No memory */
#define RT_ENOSYS                       6  /**< No system */
#define RT_EBUSY                        7  /**< Busy */
#define RT_EIO                          8  /**< IO error */
#define RT_EINTR                        9  /**< Interrupted system call */
#define RT_EINVAL                       10 /**< Invalid argument */
#define RT_TRUE                         1  /**< boolean true  */
#define RT_FALSE                        0  /**< boolean fails */

/* null pointer definition */
#define RT_NULL                         0

#define LOG_E(fmt, ...) \
    do { \
        pr_err(fmt"\n", ##__VA_ARGS__); \
    } while (0)
#define LOG_W(fmt, ...) \
    do { \
        pr_warn(fmt"\n", ##__VA_ARGS__); \
    } while (0)
#define LOG_D(fmt, ...) \
    do { \
        pr_debug(fmt"\n", ##__VA_ARGS__); \
    } while (0)

#define RT_ASSERT(EX) \
    do { \
        if (!(EX)) \
            pr_err("Assert failed!\n"); \
    } while (0)

#define RTM_EXPORT(f)
#define INIT_PREV_EXPORT(f)
#define INIT_COMPONENT_EXPORT(f)
#define FINSH_FUNCTION_EXPORT(f, d)

#define rt_malloc(s)            aicos_malloc(0, (s))
#define rt_malloc_align(s, c)   aicos_malloc_align(0, (s), (c))
#define rt_free(p)              aicos_free(0, (p))
#define rt_free_align(s)        aicos_free_align(0, (s))
#define rt_calloc               calloc
#define rt_realloc              realloc
#define rt_memset               memset
#define rt_kprintf              printf
#define rt_strdup               strdup
#define rt_strlen               strlen
#define rt_strncpy              strncpy
#define rt_snprintf             snprintf

typedef u8 rt_bool_t;
typedef u32 rt_size_t;
typedef u32 rt_uint32_t;
typedef s32 rt_int32_t;

/**
 * block device geometry structure
 */
struct rt_device_blk_geometry
{
    u32 sector_count;       /**< count of sectors */
    u32 bytes_per_sector;   /**< number of bytes per sector */
    u32 block_size;         /**< number of bytes to erase one block */
};

#endif
