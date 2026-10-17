/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety     first version
 */

#include <rtthread.h>
#include <dfs_fs.h>
#include <disk_part.h>

#include <drivers/mmcsd_core.h>

#define DBG_TAG               "SDIO"
#ifdef RT_SDIO_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif /* RT_SDIO_DEBUG */
#include <rtdbg.h>

#ifdef AIC_AB_SYSTEM_INTERFACE
#include <absystem_os.h>
#include <boot_param.h>
#endif

#define BLK_MIN(a, b) ((a) < (b) ? (a) : (b))

struct mmcsd_blk_device
{
    struct rt_mmcsd_card *card;
    rt_list_t list;
    struct rt_device dev;
    struct dfs_partition part;
    struct rt_device_blk_geometry geometry;
    rt_size_t max_req_size;
};

#ifndef RT_MMCSD_MAX_PARTITION
#define RT_MMCSD_MAX_PARTITION 16
#endif

#if defined(RT_USING_DFS_MNTTABLE) && defined(AIC_AB_SYSTEM_INTERFACE)
extern const struct dfs_mount_tbl mount_table[];

static const struct dfs_mount_tbl *mmcsd_find_ab_mount_entry(const char *logical_name,
                                                             const char *path,
                                                             const char *part_name,
                                                             const char *dev_name)
{
    int index = 0;

    while (mount_table[index].path != RT_NULL)
    {
        const struct dfs_mount_tbl *entry = &mount_table[index];

        if ((entry->device_name != RT_NULL) &&
            !strncmp(entry->path, path, PARTITION_NAME_MAX) &&
            (!strncmp(entry->device_name, logical_name, PARTITION_NAME_MAX) ||
             !strncmp(entry->device_name, part_name, PARTITION_NAME_MAX) ||
             !strncmp(entry->device_name, dev_name, PARTITION_NAME_MAX)))
        {
            return entry;
        }

        index++;
    }

    return RT_NULL;
}

static int mmcsd_mount_ab_partition(struct mmcsd_blk_device *blk_dev,
                                    const char *logical_name,
                                    const char *path,
                                    const char *default_fs,
                                    unsigned long default_rwflag,
                                    const void *default_data)
{
    const struct dfs_mount_tbl *entry;
    const char *fs_type = default_fs;
    unsigned long rwflag = default_rwflag;
    const void *data = default_data;

    entry = mmcsd_find_ab_mount_entry(logical_name, path, blk_dev->part.name,
                                      blk_dev->dev.parent.name);
    if (entry != RT_NULL)
    {
        fs_type = entry->filesystemtype;
        rwflag = entry->rwflag;
        data = entry->data;
    }

    if (dfs_mount(blk_dev->dev.parent.name, path, fs_type, rwflag, data) == 0)
    {
        LOG_I("mount fs[%s] device[%s] to %s ok.\n",
              fs_type, blk_dev->dev.parent.name, path);
        return RT_EOK;
    }

    LOG_E("mount fs[%s] device[%s] to %s failed.\n",
          fs_type, blk_dev->dev.parent.name, path);
    return -RT_ERROR;
}
#endif

rt_int32_t mmcsd_num_wr_blocks(struct rt_mmcsd_card *card)
{
    rt_int32_t err;
    rt_uint32_t blocks;

    struct rt_mmcsd_req req;
    struct rt_mmcsd_cmd cmd;
    struct rt_mmcsd_data data;
    rt_uint32_t timeout_us;

    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = APP_CMD;
    cmd.arg = card->rca << 16;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err)
        return -RT_ERROR;
    if (!controller_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
        return -RT_ERROR;

    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_APP_SEND_NUM_WR_BLKS;
    cmd.arg = 0;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    rt_memset(&data, 0, sizeof(struct rt_mmcsd_data));

    data.timeout_ns = card->tacc_ns * 100;
    data.timeout_clks = card->tacc_clks * 100;

    timeout_us = data.timeout_ns / 1000;
    timeout_us += data.timeout_clks * 1000 /
        (card->host->io_cfg.clock / 1000);

    if (timeout_us > 100000)
    {
        data.timeout_ns = 100000000;
        data.timeout_clks = 0;
    }

    data.blksize = 4;
    data.blks = 1;
    data.flags = DATA_DIR_READ;
    data.buf = &blocks;

    rt_memset(&req, 0, sizeof(struct rt_mmcsd_req));

    req.cmd = &cmd;
    req.data = &data;

    mmcsd_send_request(card->host, &req);

    if (cmd.err || data.err)
        return -RT_ERROR;

    return blocks;
}

static rt_err_t rt_mmcsd_req_blk(struct rt_mmcsd_card *card,
                                 rt_uint32_t           sector,
                                 void                 *buf,
                                 rt_size_t             blks,
                                 rt_uint8_t            dir)
{
    struct rt_mmcsd_cmd  cmd, stop;
    struct rt_mmcsd_data  data;
    struct rt_mmcsd_req  req;
    struct rt_mmcsd_host *host = card->host;
    rt_uint32_t r_cmd, w_cmd;

    mmcsd_host_lock(host);
    rt_memset(&req, 0, sizeof(struct rt_mmcsd_req));
    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    rt_memset(&stop, 0, sizeof(struct rt_mmcsd_cmd));
    rt_memset(&data, 0, sizeof(struct rt_mmcsd_data));
    req.cmd = &cmd;
    req.data = &data;

    cmd.arg = sector;
    if (!(card->flags & CARD_FLAG_SDHC))
    {
        cmd.arg <<= 9;
    }
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

    data.blksize = SECTOR_SIZE;
    data.blks  = blks;

    if (blks > 1)
    {
        if (!controller_is_spi(card->host) || !dir)
        {
            req.stop = &stop;
            stop.cmd_code = STOP_TRANSMISSION;
            stop.arg = 0;
            stop.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;
        }
        r_cmd = READ_MULTIPLE_BLOCK;
        w_cmd = WRITE_MULTIPLE_BLOCK;
    }
    else
    {
        req.stop = RT_NULL;
        r_cmd = READ_SINGLE_BLOCK;
        w_cmd = WRITE_BLOCK;
    }

    if (!dir)
    {
        cmd.cmd_code = r_cmd;
        data.flags |= DATA_DIR_READ;
    }
    else
    {
        cmd.cmd_code = w_cmd;
        data.flags |= DATA_DIR_WRITE;
    }

    mmcsd_set_data_timeout(&data, card);
    data.buf = buf;
    mmcsd_send_request(host, &req);

    if (!controller_is_spi(card->host) && dir != 0)
    {
        do
        {
            rt_int32_t err;

            cmd.cmd_code = SEND_STATUS;
            cmd.arg = card->rca << 16;
            cmd.flags = RESP_R1 | CMD_AC;
            err = mmcsd_send_cmd(card->host, &cmd, 5);
            if (err)
            {
                LOG_E("error %d requesting status", err);
                break;
            }
            /*
             * Some cards mishandle the status bits,
             * so make sure to check both the busy
             * indication and the card state.
             */
         } while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
            (R1_CURRENT_STATE(cmd.resp[0]) == 7));
    }

    mmcsd_host_unlock(host);

    if (cmd.err || data.err || stop.err)
    {
        LOG_E("mmcsd request blocks error");
        LOG_E("%d,%d,%d, 0x%08x,0x%08x",
                   cmd.err, data.err, stop.err, data.flags, sector);

        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t rt_mmcsd_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_mmcsd_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_mmcsd_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t mmcsd_erase(struct mmcsd_blk_device *blk_dev, u64 start,
                                 u64 blkcnt)
{
    rt_err_t err;
    u64 start_temp = 0, end_temp = 0;
    struct rt_mmcsd_cmd  cmd;
    struct dfs_partition *part = &blk_dev->part;
    struct rt_mmcsd_card *card = blk_dev->card;

    start = start + part->offset;

    if (!(card->flags & CARD_FLAG_SDHC))
    {
        start_temp = start << 9;
        end_temp = (start + blkcnt - 1) << 9;
    }
    else
    {
        start_temp = start;
        end_temp = start + blkcnt - 1;
    }

    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    if (card->card_type == CARD_TYPE_MMC)
        cmd.cmd_code = ERASE_GROUP_START;
    else
        cmd.cmd_code = ERASE_GROUP_START - 3;
    cmd.arg = start_temp;
    cmd.flags = RESP_R1;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err) {
        LOG_E("mmcsd_erase send ERASE_GROUP_START ERROR!");
        return -RT_ERROR;
    }

    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    if (card->card_type == CARD_TYPE_MMC)
        cmd.cmd_code = ERASE_GROUP_END;
    else
        cmd.cmd_code = ERASE_GROUP_END - 3;
    cmd.arg = end_temp;
    cmd.flags = RESP_R1;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err) {
        LOG_E("mmcsd_erase send ERASE_GROUP_END ERROR!");
        return -RT_ERROR;
    }

    rt_memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = ERASE;
    cmd.arg = 0;
    cmd.flags = RESP_R1B;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err) {
        LOG_E("mmcsd_erase send ERASE ERROR!");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t rt_mmcsd_control(rt_device_t dev, int cmd, void *args)
{
    struct mmcsd_blk_device *blk_dev = (struct mmcsd_blk_device *)dev->user_data;
    u64 *p = (u64 *)args;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_BLK_GETGEOME:
        rt_memcpy(args, &blk_dev->geometry, sizeof(struct rt_device_blk_geometry));
        break;
    case RT_DEVICE_CTRL_BLK_ERASE:
        mmcsd_erase(blk_dev, p[0], p[1]);
        break;
    default:
        break;
    }
    return RT_EOK;
}

static rt_size_t rt_mmcsd_read(rt_device_t dev,
                               rt_off_t    pos,
                               void       *buffer,
                               rt_size_t   size)
{
    rt_err_t err = 0;
    rt_size_t offset = 0;
    rt_size_t req_size = 0;
    rt_size_t remain_size = size;
    void *rd_ptr = (void *)buffer;
    struct mmcsd_blk_device *blk_dev = (struct mmcsd_blk_device *)dev->user_data;
    struct dfs_partition *part = &blk_dev->part;

    if (dev == RT_NULL)
    {
        rt_set_errno(-EINVAL);
        return 0;
    }

    rt_sem_take(part->lock, RT_WAITING_FOREVER);
    while (remain_size)
    {
        req_size = (remain_size > blk_dev->max_req_size) ? blk_dev->max_req_size : remain_size;
        err = rt_mmcsd_req_blk(blk_dev->card, part->offset + pos + offset, rd_ptr, req_size, 0);
        if (err)
            break;
        offset += req_size;
        rd_ptr = (void *)((rt_uint8_t *)rd_ptr + (req_size << 9));
        remain_size -= req_size;
    }
    rt_sem_release(part->lock);

    /* the length of reading must align to SECTOR SIZE */
    if (err)
    {
        rt_set_errno(-EIO);
        return 0;
    }
    return size - remain_size;
}

static rt_size_t rt_mmcsd_write(rt_device_t dev,
                                rt_off_t    pos,
                                const void *buffer,
                                rt_size_t   size)
{
    rt_err_t err = 0;
    rt_size_t offset = 0;
    rt_size_t req_size = 0;
    rt_size_t remain_size = size;
    void *wr_ptr = (void *)buffer;
    struct mmcsd_blk_device *blk_dev = (struct mmcsd_blk_device *)dev->user_data;
    struct dfs_partition *part = &blk_dev->part;

    if (dev == RT_NULL)
    {
        rt_set_errno(-EINVAL);
        return 0;
    }

    rt_sem_take(part->lock, RT_WAITING_FOREVER);
    while (remain_size)
    {
        req_size = (remain_size > blk_dev->max_req_size) ? blk_dev->max_req_size : remain_size;
        err = rt_mmcsd_req_blk(blk_dev->card, part->offset + pos + offset, wr_ptr, req_size, 1);
        if (err)
            break;
        offset += req_size;
        wr_ptr = (void *)((rt_uint8_t *)wr_ptr + (req_size << 9));
        remain_size -= req_size;
    }
    rt_sem_release(part->lock);

    /* the length of reading must align to SECTOR SIZE */
    if (err)
    {
        rt_set_errno(-EIO);

        return 0;
    }
    return size - remain_size;
}

static rt_int32_t mmcsd_set_blksize(struct rt_mmcsd_card *card)
{
    struct rt_mmcsd_cmd cmd;
    int err;

    /* Block-addressed cards ignore MMC_SET_BLOCKLEN. */
    if (card->flags & CARD_FLAG_SDHC)
        return 0;

    mmcsd_host_lock(card->host);
    cmd.cmd_code = SET_BLOCKLEN;
    cmd.arg = 512;
    cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;
    err = mmcsd_send_cmd(card->host, &cmd, 5);
    mmcsd_host_unlock(card->host);

    if (err)
    {
        LOG_E("MMCSD: unable to set block size to %d: %d", cmd.arg, err);

        return -RT_ERROR;
    }

    return 0;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops mmcsd_blk_ops =
{
    rt_mmcsd_init,
    rt_mmcsd_open,
    rt_mmcsd_close,
    rt_mmcsd_read,
    rt_mmcsd_write,
    rt_mmcsd_control
};
#endif


static struct mmcsd_blk_device * rt_mmcsd_create_blkdev(struct rt_mmcsd_card *card, const char* dname, struct dfs_partition* psPart)
{
    struct mmcsd_blk_device *blk_dev;
    char sname[12];

    blk_dev = rt_calloc(1, sizeof(struct mmcsd_blk_device));
    if (!blk_dev)
    {
        LOG_E("mmcsd:malloc memory failed!");
        return RT_NULL;
    }

    if (psPart != RT_NULL)
    {
        rt_memcpy(&blk_dev->part, psPart, sizeof(struct dfs_partition));
        blk_dev->geometry.sector_count = psPart->size;
    }
    else
    {
        blk_dev->part.offset = 0;
        blk_dev->part.size   = 0;
        blk_dev->geometry.sector_count = card->card_capacity * (1024 / 512);
    }

    blk_dev->geometry.bytes_per_sector = 1<<9;
    blk_dev->geometry.block_size = card->card_blksize;

    rt_snprintf(sname, sizeof(sname), "sem_%s", dname);
    blk_dev->part.lock = rt_sem_create(sname, 1, RT_IPC_FLAG_FIFO);
    if (blk_dev->part.lock == RT_NULL)
    {
        LOG_E("mmcsd:create semaphore failed!");
        rt_free(blk_dev);
        return RT_NULL;
    }

    blk_dev->max_req_size = BLK_MIN((card->host->max_dma_segs *
                                     card->host->max_seg_size) >> 9,
                                    (card->host->max_blk_count *
                                     card->host->max_blk_size) >> 9);

    /* register mmcsd device */
    blk_dev->dev.type = RT_Device_Class_Block;
#ifdef RT_USING_DEVICE_OPS
    blk_dev->dev.ops  = &mmcsd_blk_ops;
#else
    blk_dev->dev.init = rt_mmcsd_init;
    blk_dev->dev.open = rt_mmcsd_open;
    blk_dev->dev.close = rt_mmcsd_close;
    blk_dev->dev.read = rt_mmcsd_read;
    blk_dev->dev.write = rt_mmcsd_write;
    blk_dev->dev.control = rt_mmcsd_control;
#endif
    blk_dev->dev.user_data = blk_dev;

    blk_dev->card = card;

    rt_device_register(&blk_dev->dev, dname,
        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

    /* Insert to list. */
    rt_list_insert_after(&card->blk_devices, &blk_dev->list);

#ifdef RT_USING_DFS_MNTTABLE
    LOG_D("Try to mount %s\n", blk_dev->dev.parent.name);
#ifdef AIC_AB_SYSTEM_INTERFACE
    rt_int32_t id = 0;
    char ro_target[32] = { 0 };
    char rw_target[32] = { 0 };

    id = (aic_get_boot_device() == BD_SDMC0) ? 0 : 1;
    /* Check if the current device is a boot device */
    if (blk_dev->card->host->id == id) {
        /* Check if the current device partition is rodata */
        if (!strncmp("rodata", blk_dev->part.name, 6)) {
            aic_ota_status_update();
            /* Try to get the rodata device name or partition name from env */
            aic_get_rodata_to_mount(ro_target);
            if (!strncmp(ro_target, blk_dev->part.name, sizeof(ro_target)) ||
                !strncmp(ro_target, blk_dev->dev.parent.name, sizeof(ro_target))) {
                /* Try to mount the rodata device */
                mmcsd_mount_ab_partition(blk_dev, "rodata", "/rodata", "elm", MS_RDONLY, 0);
            }
        }
        /* Check if the current device partition is data */
        if (!strncmp("data", blk_dev->part.name, 4)) {
            aic_ota_status_update();
            /* Try to get the data device name or partition name from env */
            aic_get_data_to_mount(rw_target);
            if (!strncmp(rw_target, blk_dev->part.name, sizeof(rw_target)) ||
                !strncmp(rw_target, blk_dev->dev.parent.name, sizeof(rw_target))) {
                /* Try to mount the data device */
                mmcsd_mount_ab_partition(blk_dev, "data", "/data", "elm", 0, 0);
            }
        }
    }
#endif

    /* try to mount file system on this block device */
    dfs_mount_device(&(blk_dev->dev));
#endif  /* RT_USING_DFS_MNTTABLE */

    return blk_dev;
}

static unsigned long mmcsd_write(struct blk_desc *blk_dev, u64 start,
                                 u64 blkcnt, void *buffer)
{
    int err;

    err = rt_mmcsd_req_blk(blk_dev->priv, start, buffer, blkcnt, 1);
    if (err == RT_EOK)
        return blkcnt;
    return 0;
}

static unsigned long mmcsd_read(struct blk_desc *blk_dev, u64 start, u64 blkcnt,
                                const void *buffer)
{
    int err;

    err = rt_mmcsd_req_blk(blk_dev->priv, start, (void *)buffer, blkcnt, 0);
    if (err == RT_EOK)
        return blkcnt;
    return 0;
}

rt_int32_t rt_mmcsd_blk_probe(struct rt_mmcsd_card *card)
{
    rt_int32_t err = 0;
    rt_uint8_t status;
    rt_uint8_t *sector;

    if (!(card->host->flags & MMCSD_SUP_HIGHSPEED_DDR)) {
        err = mmcsd_set_blksize(card);
        if(err)
        {
            return err;
        }
    }

    LOG_D("probe mmcsd block device!");

    /* get the first sector to read partition table */
    sector = (rt_uint8_t *)rt_malloc(SECTOR_SIZE);
    if (sector == RT_NULL)
    {
        LOG_E("allocate partition sector buffer failed!");

        return -RT_ENOMEM;
    }

    status = rt_mmcsd_req_blk(card, 0, sector, 1, 0);
    if (status == RT_EOK)
    {
        rt_uint8_t i;
        char dname[8];
        struct dfs_partition part;
        struct mmcsd_blk_device *blk_dev = RT_NULL;
        rt_int32_t host_id = card->host->id;

        /* Initial blk_device link-list. */
        rt_list_init(&card->blk_devices);

        /* Always create the super node, given name is with allocated host id. */
        if (card->card_type == CARD_TYPE_MMC)
            rt_snprintf(dname, sizeof(dname), "mmc%d", host_id);
        else
            rt_snprintf(dname, sizeof(dname), "sd%d", host_id);
        blk_dev = rt_mmcsd_create_blkdev(card, (const char*)dname, NULL);
        if ( blk_dev == RT_NULL )
        {
            err = -RT_ENOMEM;
            goto exit_rt_mmcsd_blk_probe;
        }

        struct aic_partition *parts, *p;
        struct blk_desc dev_desc;
        struct disk_blk_ops ops;

        ops.blk_write = mmcsd_write;
        ops.blk_read = mmcsd_read;
        aic_disk_part_set_ops(&ops);
        dev_desc.blksz = card->card_blksize;
        dev_desc.lba_count = card->card_capacity * (1024 / 512);
        dev_desc.priv = card;
        parts = aic_disk_get_parts(&dev_desc);
        p = parts;
        i = 0;
        while (p) {
                /* Given name is with allocated host id and its partition index. */
                if (card->card_type == CARD_TYPE_MMC)
                    rt_snprintf(dname, sizeof(dname), "mmc%dp%d", host_id, i);
                else
                    rt_snprintf(dname, sizeof(dname), "sd%dp%d", host_id, i);
                memset(&part, 0, sizeof(part));
                part.type = 0;
                memcpy(part.name, p->name, strlen(p->name));
                part.offset = p->start / card->card_blksize;
                part.size = p->size / card->card_blksize;
                blk_dev = rt_mmcsd_create_blkdev(card, (const char*)dname, &part);
                if ( blk_dev == RT_NULL )
                {
                    err = -RT_ENOMEM;
                    aic_part_free(parts);
                    goto exit_rt_mmcsd_blk_probe;
                }
                p = p->next;
                i++;
        }
        if (parts)
                aic_part_free(parts);
    }
    else
    {
        LOG_E("read mmcsd first sector failed");
        err = -RT_ERROR;
    }

exit_rt_mmcsd_blk_probe:

    /* release sector buffer */
    rt_free(sector);

    if (err)
        rt_mmcsd_blk_remove(card);

    return err;
}

void rt_mmcsd_blk_remove(struct rt_mmcsd_card *card)
{
    rt_list_t *l, *n;
    struct mmcsd_blk_device *blk_dev;

    for (l = (&card->blk_devices)->next, n = l->next; l && l != &card->blk_devices; l = n, n=n->next)
    {
        blk_dev = (struct mmcsd_blk_device *)rt_list_entry(l, struct mmcsd_blk_device, list);
        if (blk_dev->card == card)
        {
            /* unmount file system */
            const char * mounted_path = dfs_filesystem_get_mounted_path(&(blk_dev->dev));
            if (mounted_path)
            {
                  dfs_unmount(mounted_path);
                  LOG_D("unmount file system %s for device %s.\r\n", mounted_path, blk_dev->dev.parent.name);
            }
            rt_sem_delete(blk_dev->part.lock);
            rt_device_unregister(&blk_dev->dev);
            rt_list_remove(&blk_dev->list);
            rt_free(blk_dev);
        }
    }
}
