/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_hal.h>
#include <mmc.h>
#include <block_dev.h>
#include <disk_part.h>

static unsigned long mmc_write(struct blk_desc *desc, u64 start, u64 blkcnt,
                               void *buffer)
{
    return mmc_bwrite(desc->priv, start, blkcnt, buffer);
}

static unsigned long mmc_read(struct blk_desc *desc, u64 start, u64 blkcnt,
                              const void *buffer)
{
    return mmc_bread(desc->priv, start, blkcnt, (void *)buffer);
}

static u32 mmc_block_write(struct block_dev *dev, u32 start, u32 blkcnt,
                           u8 *buffer)
{
    return mmc_bwrite(dev->priv, dev->blk_offset + start, blkcnt, buffer);
}

static u32 mmc_block_read(struct block_dev *dev, u32 start, u32 blkcnt,
                          u8 *buffer)
{
    return mmc_bread(dev->priv, dev->blk_offset + start, blkcnt, (void *)buffer);
}

static int mmc_block_add(struct aic_sdmc *host, struct aic_partition *parts)
{
    struct block_dev *card, *partdev;
    struct aic_partition *p;
    char devname[32];
    int pid = 0;

    card = malloc(sizeof(struct block_dev));
    if (!card) {
        pr_err("Out of memory.\n");
        return -1;
    }
    memset(card, 0, sizeof(*card));
    if (IS_SD(host))
        snprintf(devname, 32, "sd%d", host->index);
    else
        snprintf(devname, 32, "mmc%d", host->index);
    card->name = strdup(devname);
    card->blk_size = 512;
    card->blk_cnt = host->dev->card_capacity * 2;
    card->priv = host;
    card->ops.read = mmc_block_read;
    card->ops.write = mmc_block_write;
    block_add_device(card);
    p = parts;
    while (p) {
        if (IS_SD(host))
            snprintf(devname, 32, "sd%dp%d", host->index, pid);
        else
            snprintf(devname, 32, "mmc%dp%d", host->index, pid);
        pid++;
        partdev = malloc(sizeof(struct block_dev));
        if (!partdev) {
            pr_err("Out of memory.\n");
            return -1;
        }
        partdev->name = strdup(devname);
        partdev->blk_offset = p->start / card->blk_size;
        partdev->blk_size = card->blk_size;
        partdev->blk_cnt = p->size / card->blk_size;
        partdev->priv = host;
        partdev->ops.read = mmc_block_read;
        partdev->ops.write = mmc_block_write;
        partdev->parent = card;
        block_add_device(partdev);
        p = p->next;
    }

    return 0;
}

static void mmc_block_clear(struct aic_sdmc *host)
{
    int cnt;
    struct block_dev *dev;
    cnt = block_get_device_count();
    while (cnt) {
        dev = block_get_device_by_id(0);
        block_del_device(dev);
        cnt--;
    }
}

int mmc_block_init(struct aic_sdmc *host)
{
    struct aic_partition *parts;
    struct blk_desc dev_desc;
    struct disk_blk_ops ops;
    int ret;

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);

    dev_desc.blksz = 512;
    dev_desc.lba_count = host->dev->card_capacity * 2;
    dev_desc.priv = host;
    parts = aic_disk_get_parts(&dev_desc);

    ret = mmc_block_add(host, parts); 
    if (parts)
        aic_part_free(parts);
    return ret;
}

int mmc_block_deinit(struct aic_sdmc *host)
{
    mmc_block_clear(host);
    return 0;
}

int mmc_block_refresh(struct aic_sdmc *host)
{
    mmc_block_clear(host);
    mmc_block_init(host);
    return 0;
}
