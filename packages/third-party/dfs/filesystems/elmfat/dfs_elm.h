/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-02-06     Bernard      Add elm_init function declaration
 */

#ifndef __DFS_ELM_H__
#define __DFS_ELM_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE     64
#endif

#define DEVICE_TYPE_SDMC_DISK        ((const void *)0)      /* eMMC/SD */
#define DEVICE_TYPE_SPINAND_DISK     ((const void *)1)      /* SPINAND */
#define DEVICE_TYPE_USB_DISK         ((const void *)2)      /* USB pendrive */
#define DEVICE_TYPE_SPINOR_DISK      ((const void *)3)      /* SPINOR */
#define DEVICE_TYPE_RAM_DISK         ((const void *)4)      /* RAM */
#define DTL(x)                       (long)(x)

struct elm_dev_info {
    char *dev_name;
    long dev_type;
    void *priv;
};

int elm_init(void);

#ifdef __cplusplus
}
#endif

#endif
