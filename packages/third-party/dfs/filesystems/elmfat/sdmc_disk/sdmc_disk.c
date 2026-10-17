/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dfs_bare.h>

#include "aic_osal.h"
#include "sdmc_disk.h"
#include "block_dev.h"
#include "aic_utils.h"
#include "mmc.h"

DRESULT sdmc_disk_write(void *hdisk, const uint8_t *buf, uint32_t sector, rt_size_t cnt)
{
    struct block_dev *dev = hdisk;
    rt_size_t size = 0;

    if (!dev)
        return RES_NOTRDY;

    size = block_write(dev, sector, cnt, (u8 *)buf);
    if (size != cnt) {
        pr_err("write() return %d/%d\n", size, cnt);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT sdmc_disk_read(void *hdisk, uint8_t *buf, uint32_t sector, rt_size_t cnt)
{
    struct block_dev *dev = hdisk;
    rt_size_t size = 0;

    if (!dev)
        return RES_NOTRDY;

    size = block_read(dev, sector, cnt, buf);
    if (size != cnt) {
        pr_err("read() return %d/%d\n", size, cnt);
        return RES_ERROR;
    }

    return RES_OK;
}

DRESULT sdmc_disk_ioctl(void *hdisk, uint8_t command, void *buf)
{
    struct block_dev *dev = hdisk;
    DRESULT result = RES_OK;

    if (!dev)
        return RES_NOTRDY;

    switch (command) {
    case GET_SECTOR_COUNT:
        if (buf) {
            *(uint32_t *)buf = dev->blk_cnt;
        } else {
            result = RES_PARERR;
        }

        break;

    case GET_SECTOR_SIZE:
        if (buf) {
            *(uint32_t *)buf = dev->blk_size;
        } else {
            result = RES_PARERR;
        }

        break;

    case GET_BLOCK_SIZE: /* Get erase block size in unit of sectors (DWORD) */
        if (buf) {
            *(uint32_t *)buf = dev->blk_size / dev->blk_size;
        } else {
            result = RES_PARERR;
        }

        break;

    case CTRL_SYNC:
        result = RES_OK;
        break;

    default:
        result = RES_PARERR;
        break;
    }

    return result;
}

DSTATUS sdmc_disk_status(void *hdisk)
{
    return RES_OK;
}

void *sdmc_disk_initialize(const char *device_name)
{
    struct block_dev *dev;

    dev = block_get_device(device_name);

    return dev;
}
