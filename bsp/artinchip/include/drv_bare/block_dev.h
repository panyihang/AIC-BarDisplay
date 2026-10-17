/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wudehuang <dehuang.wu@artinchip.com>
 */

#ifndef __AIC_BLOCK_DEV_H_
#define __AIC_BLOCK_DEV_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_list.h>

#ifdef __cplusplus
extern "C" {
#endif

struct block_dev;
struct block_drv_ops {
    u32 (*read)(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data);
    u32 (*write)(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data);
};

struct block_dev {
    struct list_head list;
    char *name;
    struct block_dev *parent;
    /* Partition Offset in parent device, it should be 0 if it is root device */
    u32 blk_offset;
    u32 blk_size;
    u32 blk_cnt;
    struct block_drv_ops ops;
    void *priv;
};

int block_add_device(struct block_dev *dev);
int block_del_device(struct block_dev *dev);
u32 block_get_device_count(void);
struct block_dev *block_get_device_by_id(u32 id);
struct block_dev *block_get_device(const char *name);
char *block_get_name_by_id(u32 id);
u32 block_read(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data);
u32 block_write(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_BLOCK_DEV_H_ */
