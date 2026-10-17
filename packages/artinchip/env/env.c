/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aic_core.h>
#include <env.h>
#include <aic_crc32.h>
#include <aic_partition.h>
#include <disk_part.h>

#ifdef KERNEL_BAREMETAL
#include <mmc.h>
#endif

#ifndef KERNEL_BAREMETAL

#include <rtthread.h>
#include <rtdevice.h>

#ifdef AIC_SPINOR_DRV
#include <fal.h>
#endif

#else

#include <console.h>

#if (defined (AIC_SPINOR_DRV) || defined (AIC_SPINAND_DRV))
#include <mtd.h>
#endif

#endif

#define O_RDONLY (00000000)
#define O_WRONLY (00000001)
#define O_RDWR   (00000002)

#define CONFIG_EXTRA_ENV_SETTINGS      \
    "osAB_next=A\0"                    \
    "osAB_now=A\0"                     \
    "rodataAB_next=A\0"                \
    "rodataAB_now=A\0"                 \
    "dataAB_next=A\0"                  \
    "dataAB_now=A\0"                   \
    "upgrade_available=0\0"            \
    "bootlimit=5\0"                    \
    "bootcount=0\0"                    \
    "rodata_partname=blk_rodata\0"     \
    "rodata_partname_r=blk_rodata_r\0" \
    "data_partname=blk_data\0"         \
    "data_partname_r=blk_data_r\0"

static char default_environment[] = {
#ifdef CONFIG_EXTRA_ENV_SETTINGS
    CONFIG_EXTRA_ENV_SETTINGS
#endif
    "\0"
};

#ifdef AIC_SYS_REDUNDAND_ENVIRONMENT
static int have_redund_env = 1;
#else
static int have_redund_env = 0;
#endif

static unsigned char active_flag = 1;
/* obsolete_flag must be 0 to efficiently set it on NOR flash without erasing */
static unsigned char obsolete_flag = 0;
static unsigned long usable_envsize;

static int dev_current;

static int bd = 0; //boot device

struct env_image_single {
    uint32_t crc; /* CRC32 over data bytes    */
    char data[];
};

struct env_image_redundant {
    uint32_t crc;        /* CRC32 over data bytes    */
    unsigned char flags; /* active or obsolete */
    char data[];
};

enum flag_scheme {
    FLAG_NONE,
    FLAG_BOOLEAN,
    FLAG_INCREMENTAL,
};

struct environment {
    void *image;
    uint32_t *crc;
    unsigned char *flags;
    char *data;
    enum flag_scheme flag_scheme;
};

static struct environment environment = {
    .flag_scheme = FLAG_NONE,
};

static int flash_io(int mode);

#define SET_ENV_HELP                                   \
    "Usage: fw_setenv name value\n"                    \
    "Modify variables in U-Boot environment\n"         \
    "Need to run cmd fw_setbd in advance\n"            \
    "Examples:\n"                                      \
    "fw_setenv foo bar   set variable foo equal bar\n" \
    "fw_setenv foo       clear variable foo\n"

#ifndef KERNEL_BAREMETAL
static aicos_mutex_t lock_env = NULL;

static int env_lock(void)
{
    if (aicos_mutex_take(lock_env, AICOS_WAIT_FOREVER) == 0)
        return 0;
    else
        return -1;
}

static int env_unlock(void)
{
    if (lock_env != NULL) {
        aicos_mutex_give(lock_env);
        return 0;
    } else {
        return -1;
    }
}

static int env_lock_init(void)
{
    if (lock_env == NULL) {
        lock_env = aicos_mutex_create();
        if (lock_env == NULL)
            return -1;
    }

    return 0;
}
#endif

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */
static char *envmatch(char *s1, char *s2)
{
    if (s1 == NULL || s2 == NULL)
        return NULL;

    while (*s1 == *s2++)
        if (*s1++ == '=')
            return s2;
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return s2;
    return NULL;
}

/**
 * Search the environment for a variable.
 * Return the value, if found, or NULL, if not found.
 */
char *fw_getenv(char *name)
{
    char *env, *nxt;

    for (env = environment.data; *env; env = nxt + 1) {
        char *val;

        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[usable_envsize]) {
                pr_err("Environment not terminated\n");
                return NULL;
            }
        }
        val = envmatch(name, env);
        if (!val)
            continue;
        return val;
    }
    return NULL;
}

/*
 * Set/Clear a single variable in the environment.
 * This is called in sequence to update the environment
 * in RAM without updating the copy in flash after each set
 */
int fw_env_write(char *name, char *value)
{
    int len;
    char *env, *nxt;
    char *oldval = NULL;
    int deleting, creating, overwriting;

    /*
	 * search if variable with this name already exists
	 */
    for (nxt = env = environment.data; *env; env = nxt + 1) {
        for (nxt = env; *nxt; ++nxt) {
            if (nxt >= &environment.data[usable_envsize]) {
                pr_err("Environment not terminated\n");
                /* errno = EINVAL; */
                return -1;
            }
        }
        oldval = envmatch(name, env);
        if (oldval)
            break;
    }

    deleting = (oldval && !(value && strlen(value)));
    creating = (!oldval && (value && strlen(value)));
    overwriting = (oldval && (value && strlen(value)));

    if (deleting) {
        pr_info("Env: delting\n");
    } else if (overwriting) {
        pr_info("Env: overwriting\n");
    } else if (creating) {
        pr_info("Env: creating\n");
    } else {
        pr_info("Env: nothing\n");
        return 0;
    }

    if (deleting || overwriting) {
        if (*++nxt == '\0') {
            *env = '\0';
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0'))
                    break;
                ++env;
            }
        }
        *++env = '\0';
    }

    /* Delete only ? */
    if (!value || !strlen(value))
        return 0;

    /*
	 * Append new definition at the end
	 */
    for (env = environment.data; *env || *(env + 1); ++env)
        continue;

    if (env > environment.data)
        ++env;
    /*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > AIC_ENV_SIZE - (env-environment)
	 */
    len = strlen(name) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(value) + 1;

    if (len > (&environment.data[usable_envsize] - env)) {
        pr_err("Environment overflow, \"%s\" deleted\n", name);
        return -1;
    }

    while ((*env = *name++) != '\0')
        env++;
    *env = '=';
    while ((*++env = *value++) != '\0')
        continue;

    /* end is marked with double '\0' */
    *++env = '\0';

    return 0;
}

int fw_env_flush(void)
{
    uint32_t crc32;

    /*
	 * Update CRC
	 */

    crc32 = env_crc32(0, (uint8_t *)environment.data, usable_envsize);

    if (crc32 == *environment.crc)
        return 0;
    *environment.crc = crc32;

    /* write environment back to flash */
    if (flash_io(O_RDWR)) {
        pr_err("Can't write fw_env to flash\n");
        return -1;
    }

    return 0;
}

#ifdef AIC_SPINOR_DRV
#ifndef KERNEL_BAREMETAL
static int rtt_spinor_load_env_simple(void *buf, size_t size)
{
    const struct fal_partition *env_current;

    if (dev_current == 0) {
        env_current = fal_partition_find(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = fal_partition_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (fal_partition_read(env_current, 0, buf, size) != size) {
        pr_err("Fal read env fail\n");
        return -1;
    }

    return 0;
}

static int rtt_spinor_save_env_simple(void *buf, size_t size)
{
    const struct fal_partition *env_current;

    if (dev_current == 0) {
        env_current = fal_partition_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = fal_partition_find(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (fal_partition_erase(env_current, 0, AIC_ENV_SIZE) < 0) {
        pr_err("Fal erase env fail\n");
        return -1;
    }

    if (fal_partition_write(env_current, 0, buf, size) != size) {
        pr_err("Fal write env fail\n");
        return -1;
    }

    return 0;
}
#else
static int bar_spinor_load_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_read(env_current, 0, buf, size) < 0) {
        pr_err("Read env fail\n");
        return -1;
    }

    return 0;
}

static int bar_spinor_save_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (env_current == NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_erase(env_current, 0, AIC_ENV_SIZE)) {
        pr_err("Mtd erase env fail\n");
        return -1;
    }

    if (mtd_write(env_current, 0, buf, size)) {
        pr_err("Mtd write env fail\n");
        return -1;
    }

    return 0;
}
#endif
#endif

#ifdef AIC_SPINAND_DRV
#ifndef KERNEL_BAREMETAL
static int mtd_is_block_aligned(rt_off_t page, rt_uint32_t ppb)
{
    if (page & (ppb - 1))
        return 0;

    return 1;
}

static int rtt_spinand_load_env_simple(void *buf, size_t size)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev;
    rt_err_t ret;
    rt_off_t offset = 0;
    rt_uint32_t page_id = 0, blk = 0, remain = 0;

    if (dev_current == 0) {
        dev = rt_device_find(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        dev = rt_device_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    if (dev == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
            pr_err("Open MTD env failed.!\n");
            return ret;
    }

    mtd = (struct rt_mtd_nand_device *)dev;
    remain = size;

    while (remain) {
        page_id = offset / mtd->page_size;

        if (page_id > (mtd->block_total * mtd->pages_per_block)) {
            pr_err("All blocks in the part is bad!\n");
            ret = -RT_ERROR;
            goto rtt_spinand_load_env_simple_exit;
        }

        blk = page_id / mtd->pages_per_block;

        if (mtd_is_block_aligned(page_id, mtd->pages_per_block) &&
            rt_mtd_nand_check_block(mtd, blk) != RT_EOK) {
            pr_err("Block %u is bad, skip it.\n", blk);
            offset += mtd->pages_per_block * mtd->page_size;
            continue;
        }

        ret = rt_mtd_nand_read(mtd, page_id, buf, mtd->page_size, RT_NULL, 0);
        if (ret < 0) {
            pr_err("Failed to read page data from NAND.\n");
            ret = -RT_ERROR;
            goto rtt_spinand_load_env_simple_exit;
        }

        buf += mtd->page_size;
        offset += mtd->page_size;
        if (remain >= mtd->page_size)
            remain -= mtd->page_size;
        else
            remain = 0;
    }

rtt_spinand_load_env_simple_exit:
    rt_device_close(dev);

    return ret < 0 ? ret : 0;
}

static int rtt_spinand_save_env_simple(void *buf, size_t size)
{
    struct rt_mtd_nand_device *mtd;
    rt_device_t dev;
    rt_err_t ret = 0;
    rt_off_t offset = 0, bad_blk_offset = 0;
    rt_uint32_t page_id = 0, blk = 0, remain = 0, good_blk = 0;
    rt_uint8_t *buf_ptr = NULL;

    if (dev_current == 0) {
        dev = rt_device_find(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        dev = rt_device_find(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    if (dev == RT_NULL) {
        pr_err("Not found dev_current:%d\n", dev_current);
        return -RT_ERROR;
    }

    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    if (ret) {
            pr_err("Open MTD env failed.!\n");
            return ret;
    }

    mtd = (struct rt_mtd_nand_device *)dev;

write_start:
    offset = 0;
    remain = size;
    buf_ptr = buf;

    while (!good_blk) {
        page_id = offset / mtd->page_size;

        if (page_id > (mtd->block_total * mtd->pages_per_block)) {
            pr_err("All blocks in the part is bad!\n");
            ret = -RT_ERROR;
            goto rtt_spinand_save_env_simple;
        }

        blk = page_id / mtd->pages_per_block;

        if (rt_mtd_nand_check_block(mtd, blk) != RT_EOK) {
            pr_err("Block is bad, skip it.\n");
            bad_blk_offset += mtd->pages_per_block * mtd->page_size;
            offset += mtd->pages_per_block * mtd->page_size;
            continue;
        } else {
            if (rt_mtd_nand_erase_block(mtd, blk)) {
                /* write to next good block */
                ret = rt_mtd_nand_mark_badblock(mtd, blk);
                if (ret) {
                    pr_err("Failed to mark block %u bad.\n", blk);
                    ret = -RT_ERROR;
                    goto rtt_spinand_save_env_simple;
                }
                good_blk = false;
                goto write_start;
            }
            good_blk = true;
        }
    }

    while (remain) {
        page_id = offset / mtd->page_size;

        ret = rt_mtd_nand_write(mtd, page_id, buf_ptr, mtd->page_size, RT_NULL, 0);
        if (ret) {
            /* write to next good block */
            ret = rt_mtd_nand_mark_badblock(mtd, blk);
            if (ret) {
                pr_err("Failed to mark block %u bad.\n", blk);
                ret = -RT_ERROR;
                goto rtt_spinand_save_env_simple;
            }
            good_blk = false;
            goto write_start;
        }

        buf_ptr += mtd->page_size;
        offset += mtd->page_size;
        if (remain >= mtd->page_size)
            remain -= mtd->page_size;
        else
            remain = 0;
    }

rtt_spinand_save_env_simple:
    rt_device_close(dev);
    return ret;
}
#else
static int bar_spinand_load_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;
    unsigned long offset = 0;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_block_isbad(env_current, offset)) {
        pr_err("Block is bad, skip it.\n");
        offset += env_current->erasesize;
        if (offset >= env_current->size) {
            pr_err("All blocks in the part is bad!\n");
            return -1;
        }
    }

    if (mtd_read(env_current, offset, buf, size) < 0) {
        pr_err("Read env fail\n");
        return -1;
    }

    return 0;
}

static int bar_spinand_save_env_simple(void *buf, size_t size)
{
    struct mtd_dev *env_current;
    unsigned long offset = 0;
    unsigned write_len = 0;
    int err = 0;

    if (dev_current == 0) {
        env_current = mtd_get_device(AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        env_current = mtd_get_device(AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (mtd_block_isbad(env_current, offset)) {
        pr_err("Block is bad, skip it.\n");
        offset += env_current->erasesize;
        if (offset >= env_current->size) {
            pr_err("All blocks in the part is bad!\n");
            return -1;
        }
    }

    if (mtd_erase(env_current, offset, AIC_ENV_SIZE)) {
        pr_err("Mtd erase env fail\n");
        return -1;
    }

    while (write_len < size) {
        err = mtd_write_oob(env_current, offset + write_len, buf + write_len, env_current->writesize, NULL, 0);
        if (err) {
            printf("Write mtd at offset 0x%lx error, mark it.\n", offset + write_len);
            err = mtd_block_markbad(env_current, ALIGN_DOWN(offset + write_len, env_current->erasesize));
            if (err)
                printf("Mark block bad error.\n");

            return -1;
        }
        write_len += env_current->writesize;
    }

    return 0;
}
#endif
#endif

#ifdef AIC_SDMC_DRV
#ifndef KERNEL_BAREMETAL
static unsigned long mmc_write(struct blk_desc *block_dev, u64 start,
                               u64 blkcnt, void *buffer)
{
    return rt_device_write(block_dev->priv, start, (void *)buffer, blkcnt);
}

static unsigned long mmc_read(struct blk_desc *block_dev, u64 start, u64 blkcnt,
                              const void *buffer)
{
    return rt_device_read(block_dev->priv, start, (void *)buffer, blkcnt);
}

static int get_mmc_devname_by_partname(const char *partname, char *devname, int len)
{
    rt_device_t dev;
    rt_int32_t id = 0;
    struct aic_partition *parts, *part;
    struct disk_blk_ops ops;
    struct blk_desc dev_desc;
    char dname[8] = { 0 };

    id = (bd == BD_SDMC0) ? 0 : 1;

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);

    rt_snprintf(dname, sizeof(dname), "mmc%d", id);
    dev = rt_device_find(dname);
    if (dev == RT_NULL) {
        memset(dname, 0, sizeof(dname));
        rt_snprintf(dname, sizeof(dname), "sd%d", id);
        dev = rt_device_find(dname);
        if (dev == RT_NULL) {
            pr_err("Not found %s dev.\n", dname);
            return -1;
        }
    }

    rt_device_open(dev, RT_DEVICE_FLAG_RDWR);
    dev_desc.blksz = 512;
    dev_desc.lba_count = 0;
    dev_desc.priv = dev;
    parts = aic_disk_get_parts(&dev_desc);
    if (parts == RT_NULL) {
        pr_err("Not found dev %s partition info.\n", dname);
        return -1;
    }

    part = aic_part_get_byname(parts, partname);
    if (part == RT_NULL) {
        pr_err("Not found dev %s %s partition info.\n", dname, partname);
        aic_part_free(parts);
        rt_device_close(dev);
        return -1;
    }
    rt_snprintf(devname, len, "%sp%d", dname, part->index);

    aic_part_free(parts);
    rt_device_close(dev);

    return 0;
}

static int rtt_mmc_load_env_simple(void *buf, size_t size)
{
    rt_device_t env_current = RT_NULL;
    struct rt_device_blk_geometry get_data;
    size_t blkcnt = 0;
    char devname[8] = { 0 };

    if (dev_current == 0) {
        if (!get_mmc_devname_by_partname(AIC_ENV_PART_NAME, devname, 8))
            env_current = rt_device_find(devname);
    } else if (dev_current == 1) {
        if (!get_mmc_devname_by_partname(AIC_ENV_REDUNDAND_PART_NAME, devname, 8))
            env_current = rt_device_find(devname);
    }

    if (env_current == RT_NULL) {
        pr_err("Not found %s dev_current:%d\n", devname, dev_current);
        return -1;
    }

    rt_device_open(env_current, RT_DEVICE_FLAG_RDWR);

    rt_device_control(env_current, RT_DEVICE_CTRL_BLK_GETGEOME, (void *)&get_data);

    blkcnt = size / get_data.bytes_per_sector;

    if (rt_device_read(env_current, 0, buf, blkcnt) != blkcnt) {
        pr_err("mmc read env fail\n");
        goto rtt_mmc_load_env_simple_exit;
    }

rtt_mmc_load_env_simple_exit:
    rt_device_close(env_current);

    return 0;
}

static int rtt_mmc_save_env_simple(void *buf, size_t size)
{
    rt_device_t env_current = RT_NULL;
    struct rt_device_blk_geometry get_data;
    unsigned long long p[2] = {0};
    size_t blkcnt = 0;
    char devname[8] = { 0 };

    if (dev_current == 0) {
        if (!get_mmc_devname_by_partname(AIC_ENV_REDUNDAND_PART_NAME, devname, 8))
            env_current = rt_device_find(devname);
    } else if (dev_current == 1) {
        if (!get_mmc_devname_by_partname(AIC_ENV_PART_NAME, devname, 8))
            env_current = rt_device_find(devname);
    }

    if (env_current == RT_NULL) {
        pr_err("Not found %s dev_current:%d\n", devname, dev_current);
        return -1;
    }

    rt_device_open(env_current, RT_DEVICE_FLAG_RDWR);

    rt_device_control(env_current, RT_DEVICE_CTRL_BLK_GETGEOME, (void *)&get_data);

    p[0] = 0;//offset is 0
    p[1] = get_data.sector_count;

    if (rt_device_control(env_current, RT_DEVICE_CTRL_BLK_ERASE, (void *)p) < 0) {
        pr_err("Erase mmc partition failed!");
        goto rtt_mmc_save_env_simple_exit;
    }

    blkcnt = size / get_data.bytes_per_sector;

    if (rt_device_write(env_current, 0, (void *)buf, blkcnt) != blkcnt) {
        pr_err("mmc write env fail\n");
        goto rtt_mmc_save_env_simple_exit;
    }

rtt_mmc_save_env_simple_exit:
    rt_device_close(env_current);

    return 0;
}
#else
static int bar_mmc_load_env_simple(void *buf, size_t size)
{
    struct aic_sdmc *host = NULL;
    struct aic_partition *parts = NULL, *part = NULL;
    struct blk_desc dev_desc = {0};
    int id = 0;

    id = (bd == BD_SDMC0) ? 0 : 1;

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        pr_err("can't find mmc device!");
        return -1;
    }

    dev_desc.blksz = 512;
    dev_desc.lba_count = host->dev->card_capacity * 2;
    dev_desc.priv = host;
    parts = aic_disk_get_parts(&dev_desc);
    if (!parts) {
        pr_err("Not found SDMC%d partition info.\n", id);
        return -1;
    }

    if (dev_current == 0) {
        part = aic_part_get_byname(parts, AIC_ENV_PART_NAME);
    } else if (dev_current == 1) {
        part = aic_part_get_byname(parts, AIC_ENV_REDUNDAND_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (parts)
        aic_part_free(parts);

    mmc_bread(host, part->start / dev_desc.blksz, size / dev_desc.blksz, buf);

    return 0;
}

static int bar_mmc_save_env_simple(void *buf, size_t size)
{
    struct aic_sdmc *host = NULL;
    struct aic_partition *parts = NULL, *part = NULL;
    struct blk_desc dev_desc = {0};
    int id = 0;

    id = (bd == BD_SDMC0) ? 0 : 1;

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        pr_err("can't find mmc device!");
        return -1;
    }

    dev_desc.blksz = 512;
    dev_desc.lba_count = host->dev->card_capacity * 2;
    dev_desc.priv = host;
    parts = aic_disk_get_parts(&dev_desc);
    if (!parts) {
        pr_err("Not found SDMC%d partition info.\n", id);
        return -1;
    }

    if (dev_current == 0) {
        part = aic_part_get_byname(parts, AIC_ENV_REDUNDAND_PART_NAME);
    } else if (dev_current == 1) {
        part = aic_part_get_byname(parts, AIC_ENV_PART_NAME);
    } else {
        pr_err("Invalid dev_current:%d\n", dev_current);
        return -1;
    }

    if (parts)
        aic_part_free(parts);

    mmc_berase(host, part->start / dev_desc.blksz, part->size / dev_desc.blksz);

    mmc_bwrite(host, part->start / dev_desc.blksz, size / dev_desc.blksz, buf);

    return 0;
}
#endif
#endif

static int flash_env_read(void *buf, size_t size)
{
    int ret = 0;

    switch (bd) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinor_load_env_simple(buf, size);
#else
            ret = bar_spinor_load_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinand_load_env_simple(buf, size);
#else
            ret = bar_spinand_load_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SDMC_DRV
        case BD_SDMC0:
        case BD_SDMC1:
#ifndef KERNEL_BAREMETAL
            ret = rtt_mmc_load_env_simple(buf, size);
#else
            ret = bar_mmc_load_env_simple(buf, size);
#endif
            break;
#endif

        default:
            break;
    }

    if (ret)
        return ret;

#ifdef AIC_ENV_DEBUG
    int i = 0, j = 0;
    char *str;

    str = buf;

    for (i = 0; i < size; i++) {
        printf("0x%x ", str[i]);
        j++;
        if (j >= 16) {
            printf("\n");
            j = 0;
        }
    }
#endif

    return 0;
}

static int flash_env_write(void *buf, size_t size)
{
    int ret = 0;

    switch (environment.flag_scheme) {
        case FLAG_NONE:
            break;
        case FLAG_INCREMENTAL:
            (*environment.flags)++;
            break;
        case FLAG_BOOLEAN:
            *environment.flags = active_flag;
            break;
        default:
            pr_info("Unimplemented flash scheme %d\n", environment.flag_scheme);
            return -1;
    }

    switch (bd) {
#ifdef AIC_SPINOR_DRV
        case BD_SPINOR:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinor_save_env_simple(buf, size);
#else
            ret = bar_spinor_save_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SPINAND_DRV
        case BD_SPINAND:
#ifndef KERNEL_BAREMETAL
            ret = rtt_spinand_save_env_simple(buf, size);
#else
            ret = bar_spinand_save_env_simple(buf, size);
#endif
            break;
#endif
#ifdef AIC_SDMC_DRV
        case BD_SDMC0:
        case BD_SDMC1:
#ifndef KERNEL_BAREMETAL
            ret = rtt_mmc_save_env_simple(buf, size);
#else
            ret = bar_mmc_save_env_simple(buf, size);
#endif
            break;
#endif

        default:
            break;
    }

    if (ret)
        return ret;

    return 0;
}

static int flash_io(int mode)
{
    void *buf;
    size_t size;
    int ret = 0;

#ifndef KERNEL_BAREMETAL
    ret = env_lock();
    if (ret) {
        pr_err("Lock env failed\n");
        goto exit_flash_io;
    }
#endif

    if (mode == O_RDWR) {
        buf = environment.image;
        size = AIC_ENV_SIZE;
        ret = flash_env_write(buf, size);
    } else {
        buf = environment.image;
        size = AIC_ENV_SIZE;
        ret = flash_env_read(buf, size);
    }
    if (ret)
        return ret;

#ifndef KERNEL_BAREMETAL
    ret = env_unlock();
    if (ret) {
        ret = -1;
        pr_err("Unlock env failed\n");
    }

exit_flash_io:
#endif

    return ret;
}

/*
 * Prevent confusion if running from erased flash memory
 */
int fw_env_open(void)
{
    uint32_t crc0, crc0_ok;
    unsigned char flag0;
    void *addr0 = NULL;

    int crc1, crc1_ok;
    unsigned char flag1;
    void *addr1 = NULL;

    int ret;

    struct env_image_single *single;
    struct env_image_redundant *redundant;

    if (!bd) {
        bd = aic_get_boot_device();

        if ((bd <= BD_NONE) || (bd >= BD_SDFAT32)) {
            pr_err("Parameter bd is not right\n");
            return -1;
        }
    }

#ifndef KERNEL_BAREMETAL
    ret = env_lock_init();
    if (ret) {
        return -1;
    }
#endif

    addr0 = aicos_malloc_align(0, AIC_ENV_SIZE, CACHE_LINE_SIZE);
    if (addr0 == NULL) {
        pr_err("Not enough memory for environment (%d bytes)\n", AIC_ENV_SIZE);
        ret = -1;
        goto open_cleanup;
    }

    /* read environment from FLASH to local buffer */
    environment.image = addr0;

    if (have_redund_env) {
        redundant = addr0;
        environment.crc = &redundant->crc;
        environment.flags = &redundant->flags;
        environment.data = redundant->data;
    } else {
        single = addr0;
        environment.crc = &single->crc;
        environment.flags = NULL;
        environment.data = single->data;
    }

    usable_envsize = AIC_ENV_SIZE - sizeof(uint32_t);
    if (have_redund_env)
        usable_envsize -= sizeof(char);

    dev_current = 0;
    if (flash_io(O_RDONLY)) {
        ret = -EIO;
        goto open_cleanup;
    }
    crc0 = env_crc32(0, (uint8_t *)environment.data, usable_envsize);

#ifdef AIC_ENV_DEBUG
    printf("crc0 = 0x%x,environment.crc = 0x%x\n", (unsigned int)crc0,
           (unsigned int)*environment.crc);
#endif
    crc0_ok = (crc0 == *environment.crc);

    if (!have_redund_env) {
        if (!crc0_ok) {
            pr_err("Bad CRC, using default environment\n");
            memcpy(environment.data, default_environment,
                   sizeof(default_environment));
        }
    } else {
        flag0 = *environment.flags;

        dev_current = 1;
        addr1 = aicos_malloc_align(0, AIC_ENV_SIZE, CACHE_LINE_SIZE);
        if (addr1 == NULL) {
            pr_err("Not enough memory for environment (%d bytes)\n",
                   AIC_ENV_SIZE);
            ret = -ENOMEM;
            goto open_cleanup;
        }
        redundant = addr1;

        /*
		 * have to set environment.image for flash_read(), careful -
		 * other pointers in environment still point inside addr0
		 */
        environment.image = addr1;
        if (flash_io(O_RDONLY)) {
            ret = -EIO;
            goto open_cleanup;
        }

        /* Check flag scheme compatibility */
        /* uboot default is FLAG_INCREMENTAL */
        environment.flag_scheme = FLAG_INCREMENTAL,

        crc1 = env_crc32(0, (uint8_t *)redundant->data, usable_envsize);
#ifdef AIC_ENV_DEBUG
        printf("crc1 = 0x%x,redundant->crc = 0x%x\n", (unsigned int)crc1,
               (unsigned int)redundant->crc);
#endif
        crc1_ok = (crc1 == redundant->crc);
        flag1 = redundant->flags;

        if (crc0_ok && !crc1_ok) {
            dev_current = 0;
        } else if (!crc0_ok && crc1_ok) {
            dev_current = 1;
        } else if (!crc0_ok && !crc1_ok) {
            pr_err("Warning: Bad CRC, using default environment\n");
            memcpy(environment.data, default_environment,
                   sizeof(default_environment));
            dev_current = 0;
        } else {
            switch (environment.flag_scheme) {
                case FLAG_BOOLEAN:
                    if (flag0 == active_flag && flag1 == obsolete_flag) {
                        dev_current = 0;
                    } else if (flag0 == obsolete_flag && flag1 == active_flag) {
                        dev_current = 1;
                    } else if (flag0 == flag1) {
                        dev_current = 0;
                    } else if (flag0 == 0xFF) {
                        dev_current = 0;
                    } else if (flag1 == 0xFF) {
                        dev_current = 1;
                    } else {
                        dev_current = 0;
                    }
                    break;
                case FLAG_INCREMENTAL:
                    if (flag0 == 255 && flag1 == 0)
                        dev_current = 1;
                    else if ((flag1 == 255 && flag0 == 0) || flag0 >= flag1)
                        dev_current = 0;
                    else /* flag1 > flag0 */
                        dev_current = 1;
                    break;
                default:
                    pr_info("Unknown flag scheme %d\n",
                            environment.flag_scheme);
                    ret = -1;
                    goto open_cleanup;
            }
        }

        /*
		 * If we are reading, we don't need the flag and the CRC any
		 * more, if we are writing, we will re-calculate CRC and update
		 * flags before writing out
		 */
        if (dev_current) {
            environment.image = addr1;
            environment.crc = &redundant->crc;
            environment.flags = &redundant->flags;
            environment.data = redundant->data;
            aicos_free_align(0, addr0);
        } else {
            environment.image = addr0;
            /* Other pointers are already set */
            aicos_free_align(0, addr1);
        }
    }
    return 0;

open_cleanup:
    if (addr0) {
        aicos_free_align(0, addr0);
    }

    if (addr1) {
        aicos_free_align(0, addr1);
    }

    return ret;
}

/*
 * Simply free allocated buffer with environment
 */
int fw_env_close(void)
{
    int ret = 0;

    if (environment.image) {
        aicos_free_align(0, environment.image);
    }

    environment.image = NULL;
    return ret;
}

#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static int cmd_fw_printenv(int argc, char **argv)
{
    int i, ret = 0;

    if (fw_env_open()) {
        pr_err("Open env failed\n");
        return -1;
    }

    if (argc == 1) { /* Print all env variables  */
        char *env, *nxt;
        for (env = environment.data; *env; env = nxt + 1) {
            for (nxt = env; *nxt; ++nxt) {
                if (nxt >= &environment.data[usable_envsize]) {
                    pr_err("Environment not terminated\n");
                    return -1;
                }
            }

            printf("%s\n", env);
        }
        fw_env_close();
        return 0;
    }

    for (i = 1; i < argc; ++i) { /* print a subset of env variables */
        char *name = argv[i];
        char *val = NULL;

        val = fw_getenv(name);
        if (!val) {
            pr_err("\"%s\" not defined\n", name);
            ret = -1;
            continue;
        }

        pr_err("%s=%s\n", name, val);
    }

    fw_env_close();
    return ret;
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_fw_printenv, fw_printenv, Print env);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(fw_printenv, cmd_fw_printenv, "Print env");
#endif

static int cmd_fw_setenv(int argc, char **argv)
{
    int ret = 0;

    if ((argc != 3) && (argc != 2)) {
        printf(SET_ENV_HELP);
        return -1;
    }

    if (fw_env_open()) {
        ret = -1;
        goto fw_setenv_err;
    }

    ret = fw_env_write(argv[1], argv[2]);
    if (ret) {
        pr_err("Env write fail\n");
        goto fw_setenv_err;
    }

    fw_env_flush();

fw_setenv_err:
    fw_env_close();

    return ret;
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_fw_setenv, fw_setenv, Set env);
#elif defined(AIC_CONSOLE_BARE_DRV)
CONSOLE_CMD(fw_setenv, cmd_fw_setenv, "Set env");
#endif
#endif // defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
