/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-06-23     armink       the first version
 * 2019-08-22     MurphyZhao   adapt to none rt-thread case
 */

#include <fal.h>

#ifdef RT_VER_NUM
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <aic_common.h>

/* ========================== block device ======================== */
struct fal_blk_device {
    struct rt_device                parent;
    struct rt_device_blk_geometry   geometry;
    const struct fal_partition     *fal_part;
};

/* RT-Thread device interface */
#if RTTHREAD_VERSION >= 30000
static rt_err_t blk_dev_control(rt_device_t dev, int cmd, void *args)
#else
static rt_err_t blk_dev_control(rt_device_t dev, rt_uint8_t cmd, void *args)
#endif
{
    struct fal_blk_device *part = (struct fal_blk_device*) dev;

    assert(part != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *) args;
        if (geometry == RT_NULL)
        {
            return -RT_ERROR;
        }

        memcpy(geometry, &part->geometry, sizeof(struct rt_device_blk_geometry));
    }
    else if (cmd == RT_DEVICE_CTRL_BLK_ERASE)
    {
        rt_uint32_t *addrs = (rt_uint32_t *) args, start_addr = addrs[0], end_addr = addrs[1], phy_start_addr;
        rt_size_t phy_size;

        log_d("start_addr = %d end_addr = %d.\n", start_addr, end_addr);

        if (addrs == RT_NULL || start_addr > end_addr)
        {
            return -RT_ERROR;
        }

        if (end_addr == start_addr)
        {
            end_addr++;
        }

        phy_start_addr = start_addr * part->geometry.bytes_per_sector;
        phy_size = (end_addr - start_addr) * part->geometry.bytes_per_sector;

        if (fal_partition_erase(part->fal_part, phy_start_addr, phy_size) < 0)
        {
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_size_t blk_dev_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_blk_device *part = (struct fal_blk_device*) dev;

    assert(part != RT_NULL);

    ret = fal_partition_read(part->fal_part, pos * part->geometry.block_size, buffer, size * part->geometry.block_size);

    if (ret != (int)(size * part->geometry.block_size))
    {
        ret = 0;
    }
    else
    {
        ret = size;
    }

    return ret;
}

static rt_size_t blk_dev_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t sectors)
{
    (void)dev;
    (void)pos;
    (void)buffer;
    log_e("This config only supports read!\n");
    return sectors;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops blk_dev_ops = {
    RT_NULL,
    RT_NULL,
    RT_NULL,
    blk_dev_read,
    blk_dev_write,
    blk_dev_control
};
#endif

/**
 * create RT-Thread block device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created block device
 *            NULL: created failed
 */
struct rt_device *fal_blk_device_create(const char *parition_name)
{
    struct fal_blk_device *blk_dev;
    const struct fal_partition *fal_part = fal_partition_find(parition_name);
    const struct fal_flash_dev *fal_flash = NULL;
    char str[32] = {0};

    if (!fal_part)
    {
        log_e("Error: the partition name (%s) is not found.", parition_name);
        return NULL;
    }

    if ((fal_flash = fal_flash_device_find(fal_part->flash_name)) == NULL)
    {
        log_e("Error: the flash device name (%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    blk_dev = (struct fal_blk_device*) rt_malloc(sizeof(struct fal_blk_device));
    if (blk_dev)
    {
        blk_dev->fal_part = fal_part;

        /*To solve the problem of space waste when the sector is set to 4096*/
        /*bytes_per_sector and block_size must set to 512,
          the fatfs partition info can be recognized normally*/
        blk_dev->geometry.bytes_per_sector = 512;
        blk_dev->geometry.block_size = blk_dev->geometry.bytes_per_sector;
        blk_dev->geometry.sector_count = fal_part->len / blk_dev->geometry.bytes_per_sector;

        /* register device */
        blk_dev->parent.type = RT_Device_Class_Block;

#ifdef RT_USING_DEVICE_OPS
        blk_dev->parent.ops  = &blk_dev_ops;
#else
        blk_dev->parent.init = NULL;
        blk_dev->parent.open = NULL;
        blk_dev->parent.close = NULL;
        blk_dev->parent.read = blk_dev_read;
        blk_dev->parent.write = blk_dev_write;
        blk_dev->parent.control = blk_dev_control;
#endif

        /* no private */
        blk_dev->parent.user_data = RT_NULL;

        rt_sprintf(str, "blk_%s", fal_part->name);
        log_d("The FAL block device (%s) created successfully", str);

        rt_device_register(RT_DEVICE(blk_dev), str, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
    }
    else
    {
        log_e("Error: no memory for create FAL block device");
    }

    return RT_DEVICE(blk_dev);
}

/* ========================== MTD nor device ======================== */
#if defined(RT_USING_MTD_NOR)

struct fal_mtd_nor_device
{
    struct rt_mtd_nor_device       parent;
    const struct fal_partition     *fal_part;
};

static rt_size_t mtd_nor_dev_read(struct rt_mtd_nor_device* device, rt_off_t offset, rt_uint8_t* data, rt_uint32_t length)
{
    int ret = 0;
    struct fal_mtd_nor_device *part = (struct fal_mtd_nor_device*) device;

    assert(part != RT_NULL);

    ret = fal_partition_read(part->fal_part, offset, data, length);

    if (ret != (int)length)
    {
        ret = 0;
    }
    else
    {
        ret = length;
    }

    return ret;
}

static rt_size_t mtd_nor_dev_write(struct rt_mtd_nor_device* device, rt_off_t offset, const rt_uint8_t* data, rt_uint32_t length)
{
    int ret = 0;
    struct fal_mtd_nor_device *part;

    part = (struct fal_mtd_nor_device*) device;
    assert(part != RT_NULL);

    ret = fal_partition_write(part->fal_part, offset, data, length);

    if (ret != (int) length)
    {
        ret = 0;
    }
    else
    {
        ret = length;
    }

    return ret;
}

static rt_err_t mtd_nor_dev_erase(struct rt_mtd_nor_device* device, rt_off_t offset, rt_uint32_t length)
{
    int ret = 0;
    struct fal_mtd_nor_device *part;

    part = (struct fal_mtd_nor_device*) device;
    assert(part != RT_NULL);

    ret = fal_partition_erase(part->fal_part, offset, length);

    if ((rt_uint32_t)ret != length || ret < 0)
    {
        return -RT_ERROR;
    }
    else
    {
        return RT_EOK;
    }
}

static const struct rt_mtd_nor_driver_ops _ops =
{
    RT_NULL,
    mtd_nor_dev_read,
    mtd_nor_dev_write,
    mtd_nor_dev_erase,
};

/**
 * create RT-Thread MTD NOR device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created MTD NOR device
 *            NULL: created failed
 */
struct rt_device *fal_mtd_nor_device_create(const char *parition_name)
{
    struct fal_mtd_nor_device *mtd_nor_dev;
    const struct fal_partition *fal_part = fal_partition_find(parition_name);
    const struct fal_flash_dev *fal_flash = NULL;

    if (!fal_part)
    {
        log_e("Error: the partition name (%s) is not found.", parition_name);
        return NULL;
    }

    if ((fal_flash = fal_flash_device_find(fal_part->flash_name)) == NULL)
    {
        log_e("Error: the flash device name (%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    mtd_nor_dev = (struct fal_mtd_nor_device*) rt_malloc(sizeof(struct fal_mtd_nor_device));
    if (mtd_nor_dev)
    {
        mtd_nor_dev->fal_part = fal_part;

        mtd_nor_dev->parent.block_start = 0;
        mtd_nor_dev->parent.block_end = fal_part->len / fal_flash->blk_size;
        mtd_nor_dev->parent.block_size = fal_flash->blk_size;

        /* set ops */
        mtd_nor_dev->parent.ops = &_ops;

        log_d("The FAL MTD NOR device (%s) created successfully", fal_part->name);
        rt_mtd_nor_register_device(fal_part->name, &mtd_nor_dev->parent);
    }
    else
    {
        log_e("Error: no memory for create FAL MTD NOR device");
    }

    return RT_DEVICE(&mtd_nor_dev->parent);
}

#endif /* defined(RT_USING_MTD_NOR) */


/* ========================== char device ======================== */
struct fal_char_device
{
    struct rt_device                parent;
    const struct fal_partition     *fal_part;
};

/* RT-Thread device interface */
static rt_size_t char_dev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) dev;

    assert(part != RT_NULL);

    if (pos + size > part->fal_part->len)
        size = part->fal_part->len - pos;

    ret = fal_partition_read(part->fal_part, pos, buffer, size);

    if (ret != (int)(size))
        ret = 0;

    return ret;
}

static rt_size_t char_dev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_char_device *part;

    part = (struct fal_char_device *) dev;
    assert(part != RT_NULL);

    if (pos == 0)
    {
        fal_partition_erase_all(part->fal_part);
    }
    else if (pos + size > part->fal_part->len)
    {
        size = part->fal_part->len - pos;
    }

    ret = fal_partition_write(part->fal_part, pos, buffer, size);

    if (ret != (int) size)
        ret = 0;

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops char_dev_ops = {
    RT_NULL,
    RT_NULL,
    RT_NULL,
    char_dev_read,
    char_dev_write,
    RT_NULL
};
#endif

#ifdef RT_USING_POSIX
#include <dfs_posix.h>

/* RT-Thread device filesystem interface */
static int char_dev_fopen(struct dfs_fd *fd)
{
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    switch (fd->flags & O_ACCMODE)
    {
    case O_RDONLY:
        break;
    case O_WRONLY:
    case O_RDWR:
        /* erase partition when device file open */
        fal_partition_erase_all(part->fal_part);
        break;
    default:
        break;
    }
    fd->pos = 0;

    return RT_EOK;
}

static int char_dev_fread(struct dfs_fd *fd, void *buf, size_t count)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    if (fd->pos + count > part->fal_part->len)
        count = part->fal_part->len - fd->pos;

    ret = fal_partition_read(part->fal_part, fd->pos, buf, count);

    if (ret != (int)(count))
        return 0;

    fd->pos += ret;

    return ret;
}

static int char_dev_fwrite(struct dfs_fd *fd, const void *buf, size_t count)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    if (fd->pos + count > part->fal_part->len)
        count = part->fal_part->len - fd->pos;

    ret = fal_partition_write(part->fal_part, fd->pos, buf, count);

    if (ret != (int) count)
        return 0;

    fd->pos += ret;

    return ret;
}

static const struct dfs_file_ops char_dev_fops =
{
    char_dev_fopen,
    RT_NULL,
    RT_NULL,
    char_dev_fread,
    char_dev_fwrite,
    RT_NULL, /* flush */
    RT_NULL, /* lseek */
    RT_NULL, /* getdents */
    RT_NULL,
};
#endif /* defined(RT_USING_POSIX) */

/**
 * create RT-Thread char device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created char device
 *            NULL: created failed
 */
struct rt_device *fal_char_device_create(const char *parition_name)
{
    struct fal_char_device *char_dev;
    const struct fal_partition *fal_part = fal_partition_find(parition_name);

    if (!fal_part)
    {
        log_e("Error: the partition name (%s) is not found.", parition_name);
        return NULL;
    }

    if ((fal_flash_device_find(fal_part->flash_name)) == NULL)
    {
        log_e("Error: the flash device name (%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    char_dev = (struct fal_char_device *) rt_malloc(sizeof(struct fal_char_device));
    if (char_dev)
    {
        char_dev->fal_part = fal_part;

        /* register device */
        char_dev->parent.type = RT_Device_Class_Char;

#ifdef RT_USING_DEVICE_OPS
        char_dev->parent.ops  = &char_dev_ops;
#else
        char_dev->parent.init = NULL;
        char_dev->parent.open = NULL;
        char_dev->parent.close = NULL;
        char_dev->parent.read = char_dev_read;
        char_dev->parent.write = char_dev_write;
        char_dev->parent.control = NULL;
        /* no private */
        char_dev->parent.user_data = NULL;
#endif

        rt_device_register(RT_DEVICE(char_dev), fal_part->name, RT_DEVICE_FLAG_RDWR);
        log_d("The FAL char device (%s) created successfully", fal_part->name);

#ifdef RT_USING_POSIX
        /* set fops */
        char_dev->parent.fops = &char_dev_fops;
#endif

    }
    else
    {
        log_e("Error: no memory for create FAL char device");
    }

    return RT_DEVICE(char_dev);
}
#endif /* RT_VER_NUM */
