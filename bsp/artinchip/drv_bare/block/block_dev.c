/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <block_dev.h>

static LIST_HEAD(block_dev_list);

int block_add_device(struct block_dev *block)
{
    struct block_dev *item;

    if (!block)
        return -EINVAL;

    item = block_get_device(block->name);
    if (item)
        return -EEXIST;

    list_add_tail(&block->list, &block_dev_list);
    return 0;
}

int block_del_device(struct block_dev *block)
{
    struct block_dev *item;

    if (!block)
        return -EINVAL;

    item = block_get_device(block->name);
    if (item != block)
        return -EEXIST;

    list_del(&block->list);
    return 0;
}

struct block_dev *block_get_device(const char *name)
{
    struct block_dev *item;
    struct list_head *pos;

    if (!name) {
        return NULL;
    }

    list_for_each(pos, &block_dev_list) {
        item = list_entry(pos, struct block_dev, list);
        if (!strcmp(item->name, name))
            return item;
    }
    return NULL;
}

u32 block_get_device_count(void)
{
    struct list_head *pos;
    u32 cnt;

    cnt = 0;

    list_for_each(pos, &block_dev_list) {
        cnt++;
    }
    return cnt;
}

struct block_dev *block_get_device_by_id(u32 id)
{
    struct block_dev *item;
    struct list_head *pos;
    u32 cnt;

    cnt = 0;
    item = NULL;

    list_for_each(pos, &block_dev_list) {
        item = list_entry(pos, struct block_dev, list);
        if (cnt == id)
            break;
        cnt++;
    }
    return item;
}

char *block_get_name_by_id(u32 id)
{
    struct block_dev *item;
    struct list_head *pos;
    u32 cnt;

    cnt = 0;
    item = NULL;

    list_for_each(pos, &block_dev_list) {
        item = list_entry(pos, struct block_dev, list);
        if (cnt == id)
            break;
        cnt++;
    }
    return item->name;
}

u32 block_read(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data)
{
    if (dev && dev->ops.read)
        return dev->ops.read(dev, start_blk, blkcnt, data);
    return 0;
}

u32 block_write(struct block_dev *dev, u32 start_blk, u32 blkcnt, u8 *data)
{
    if (dev && dev->ops.write)
        return dev->ops.write(dev, start_blk, blkcnt, data);
    return 0;
}
