/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <aic_crc32.h>
#include <aicupg.h>
#include <mtd.h>
#include <sfud.h>
#include "upg_internal.h"
#include <spienc.h>
#include <firmware_security.h>

#ifdef LPKG_USING_LEVELX
#include "lx_api.h"
UINT  aic_lx_nor_flash_driver_initialize(LX_NOR_FLASH *nor_flash);
#endif
#define MAX_DUPLICATED_PART 6
#define MAX_NOR_NAME        32

#define SECTOR_4K  4 * 1024
#define SECTOR_32K 32 * 1024
#define SECTOR_64K 64 * 1024

struct aicupg_nor_priv {
    struct mtd_dev *mtd[MAX_DUPLICATED_PART];
    unsigned long start_offset[MAX_DUPLICATED_PART];
    unsigned long erase_offset[MAX_DUPLICATED_PART];
#ifdef LPKG_USING_LEVELX
    LX_NOR_FLASH    *lx_nor_flash;
#endif
};

static s32 nor_fwc_get_mtd_partitions(struct fwc_info *fwc,
                                      struct aicupg_nor_priv *priv)
{
    char name[MAX_NOR_NAME], *p;
    int cnt = 0, idx = 0;

    p = fwc->meta.partition;
    while (*p) {
        if (cnt >= MAX_NOR_NAME) {
            pr_err("Partition name is too long.\n");
            return -1;
        }

        name[cnt] = *p;
        p++;
        cnt++;
        if (*p == ';' || *p == '\0') {
            name[cnt] = '\0';
            priv->mtd[idx] = mtd_get_device(name);
            if (!priv->mtd[idx]) {
                pr_err("MTD %s part not found.\n", name);
                return -1;
            }
#ifdef LPKG_USING_LEVELX
            if (strstr(fwc->meta.attr, "block")) {
                mtd_erase(priv->mtd[idx], 0, priv->mtd[idx]->size);
                lx_nor_flash_initialize();
                priv->lx_nor_flash = (LX_NOR_FLASH *)malloc(sizeof(LX_NOR_FLASH));
                memset(priv->lx_nor_flash, 0, sizeof(LX_NOR_FLASH));
                priv->lx_nor_flash->mtd_device = priv->mtd[idx];
                if(lx_nor_flash_open(priv->lx_nor_flash, name, aic_lx_nor_flash_driver_initialize))
                    pr_err("%s: %d lx_nor_flash_open failed\n", __func__, __LINE__);
            }
#endif
            idx++;
            cnt = 0;
        }
        if (*p == ';')
            p++;
        if (*p == '\0')
            break;
    }
    return 0;
}

int nor_is_exist(u32 id)
{
    struct mtd_dev *mtd;
    char name[10];

    memset(name, 0, 10);
    strcpy(name, "nor0");
    name[3] = '0' + id;
    mtd = mtd_get_device(name);
    if (mtd) {
        return 1;
    }

    return 0;
}

s32 nor_get_geometry(u32 id, struct storage_geometry *geo)
{
    struct mtd_dev *mtd;
    char name[10];

    if (geo == NULL)
        return -1;

    memset(name, 0, 10);
    strcpy(name, "nor0");
    name[3] = '0' + id;
    mtd = mtd_get_device(name);
    if (mtd) {
        geo->total_size = mtd->size;
        geo->write_size = mtd->writesize;
        geo->erase_size = mtd->erasesize;
        geo->oob_size = 0;
        return 0;
    }

    return -1;
}

s32 nor_storage_erase(u32 id, struct storage_erase *erase)
{
    struct mtd_dev *mtd;
    char name[10];
    int ret = 0;

    if (erase == NULL)
        return -1;

    memset(name, 0, 10);
    strcpy(name, "nor0");
    name[3] = '0' + id;
    mtd = mtd_get_device(name);
    if (mtd) {
        /* The start offset of flash */
        ret = mtd_erase(mtd, erase->start, erase->size);
        return ret;
    }

    return -1;
}

/*
 * storage device prepare,should probe spi norflash
 *  - probe spi device
 *  - set env for MTD partitions
 *  - probe MTD device
 */
extern sfud_flash *sfud_probe(u32 spi_bus);
s32 nor_fwc_prepare(struct fwc_info *fwc, u32 id)
{
    sfud_flash *flash;

    flash = sfud_probe(id);
    if (flash == NULL) {
        printf("Failed to probe spinor flash.\n");
        return -1;
    }

#ifdef AIC_SPIENC_BYPASS_IN_UPGMODE
        spienc_set_bypass(1);
#endif
    return 0;
}

/*
 * New FirmWare Component start, should prepare to burn FWC to NOR
 *  - Get FWC attributes
 *  - Parse MTD partitions  FWC going to upgrade
 */
void nor_fwc_start(struct fwc_info *fwc)
{
    struct aicupg_nor_priv *priv;
    int ret = 0;

    priv = malloc(sizeof(struct aicupg_nor_priv));
    if (!priv) {
        pr_err("Out of memory, malloc failed.\n");
        goto err;
    }
    memset(priv, 0, sizeof(struct aicupg_nor_priv));
    fwc->priv = priv;

    ret = nor_fwc_get_mtd_partitions(fwc, priv);
    if (ret) {
        pr_err("Get MTD partitions failed.\n");
        goto err;
    }

#ifdef AIC_USING_SPIENC
    if (strstr(fwc->meta.name, "target.spl"))
        spienc_select_tweak(AIC_SPIENC_HW_TWEAK);
#endif
    fwc->block_size = 2048;

#ifdef AICUPG_FIRMWARE_SECURITY
    firmware_security_init();
#endif

    return;
err:
    if (priv)
        free(priv);
}

s32 nor_fwc_mtd_erase(struct mtd_dev *mtd, struct aicupg_nor_priv *priv, unsigned long offset, s32 len, int i)
{
    unsigned long erase_offset;
    int ret = 0;

    erase_offset = priv->erase_offset[i];
    while ((offset + len) > erase_offset) {
        if ((offset + ROUNDUP(len, SECTOR_64K)) <= (mtd->size)) {
            ret = mtd_erase(mtd, erase_offset, ROUNDUP(len, SECTOR_64K));
            if (ret) {
                return ret;
            }
            priv->erase_offset[i] = erase_offset + ROUNDUP(len, SECTOR_64K);
        } else if ((offset + ROUNDUP(len, SECTOR_32K)) <= (mtd->size)) {
            ret = mtd_erase(mtd, erase_offset, ROUNDUP(len, SECTOR_32K));
            if (ret) {
                return ret;
            }
            priv->erase_offset[i] = erase_offset + ROUNDUP(len, SECTOR_32K);
        } else {
            ret = mtd_erase(mtd, erase_offset, ROUNDUP(len, SECTOR_4K));
            if (ret) {
                return ret;
            }
            priv->erase_offset[i] = erase_offset + ROUNDUP(len, SECTOR_4K);
        }
        erase_offset = priv->erase_offset[i];
    }

    return ret;
}

/*
 * New FirmWare Component write, should write data to NOR
 *  - Erase MTD partitions for prepare write
 *  - Write data to MTD partions
 */
static s32 nor_fwc_mtd_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nor_priv *priv;
    struct mtd_dev *mtd;
    unsigned long offset;
    int i, calc_len = 0, ret = 0;
    u8 __attribute__((unused)) *rdbuf = NULL;

    if ((fwc->meta.size - fwc->trans_size) < len)
        calc_len = fwc->meta.size - fwc->trans_size;
    else
        calc_len = len;

    fwc->calc_partition_crc = crc32(fwc->calc_partition_crc, buf, calc_len);

#ifdef AICUPG_FIRMWARE_SECURITY
    firmware_security_decrypt(buf, len);
#endif

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    rdbuf = aicupg_malloc_align(len, CACHE_LINE_SIZE);
    if (!rdbuf) {
        pr_err("Error: malloc buffer failed.\n");
        return 0;
    }
#endif

    priv = (struct aicupg_nor_priv *)fwc->priv;
    for (i = 0; i < MAX_DUPLICATED_PART; i++) {
        mtd = priv->mtd[i];
        if (!mtd)
            continue;

        offset = priv->start_offset[i];
        if ((offset + len) > (mtd->size)) {
            pr_err("Not enough space to write mtd %s\n", mtd->name);
            goto out;
        }

        /* erase 1 sector when offset+len more than erased address */
        ret = nor_fwc_mtd_erase(mtd, priv, offset, len, i);
        if (ret) {
            pr_err("MTD erase sector failed!\n");
            goto out;
        }

        ret = mtd_write(mtd, offset, buf, len);
        if (ret) {
            pr_err("Write mtd %s error.\n", mtd->name);
            goto out;
        }


#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
        // Read data to calc crc
        ret = mtd_read(mtd, offset, rdbuf, len);
        if (ret) {
            pr_err("Read mtd %s error.\n", mtd->name);
            goto out;
        }
#endif
        priv->start_offset[i] = offset + len;
    }

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    if (crc32(0, buf, calc_len) != crc32(0, rdbuf, calc_len)) {
        pr_err("calc_len:%d\n", calc_len);
        pr_err("crc err at trans len %u\n", fwc->trans_size);
    }
#endif

    if (rdbuf)
        aicupg_free_align(rdbuf);
    return len;

out:
    if (rdbuf)
        aicupg_free_align(rdbuf);
    return ret;
}

#ifdef LPKG_USING_LEVELX
s32 nor_fwc_levelx_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nor_priv *priv;
    struct mtd_dev *mtd;
    unsigned long offset;
    int i, calc_len = 0, ret = 0;
    u8 __attribute__((unused)) *rdbuf = NULL;
    u32 sec_cnt, sector;
    u8 *temp_ptr = NULL;
    LX_NOR_FLASH *lx_dev;

    if ((fwc->meta.size - fwc->trans_size) < len)
        calc_len = fwc->meta.size - fwc->trans_size;
    else
        calc_len = len;

    fwc->calc_partition_crc = crc32(fwc->calc_partition_crc, buf, calc_len);

#ifdef AICUPG_FIRMWARE_SECURITY
    firmware_security_decrypt(buf, len);
#endif

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    rdbuf = aicupg_malloc_align(len, CACHE_LINE_SIZE);
    if (!rdbuf) {
        pr_err("Error: malloc buffer failed.\n");
        return 0;
    }
#endif

    priv = (struct aicupg_nor_priv *)fwc->priv;
    lx_dev = priv->lx_nor_flash;
    for (i = 0; i < MAX_DUPLICATED_PART; i++) {
        mtd = priv->mtd[i];
        if (!mtd)
            continue;

        offset = priv->start_offset[i];
        if ((offset + len) > (mtd->size)) {
            pr_err("Not enough space to write mtd %s\n", mtd->name);
            goto out;
        }

        sec_cnt = len / 512;
        sector = offset / 512;
        temp_ptr = buf;

        while (sec_cnt) {
            ret =  lx_nor_flash_sector_write(lx_dev, sector, temp_ptr);
            if (ret) {
                pr_err("LeveX write sector %u failed!\n", sector);
                goto out;
            }

            sec_cnt--;
            sector++;
            temp_ptr += 512;
        }

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
        // Read data to calc crc
        sec_cnt = len / 512;
        sector = offset / 512;
        temp_ptr = rdbuf;
        while (sec_cnt) {
            ret =  lx_nor_flash_sector_read(lx_dev, sector, temp_ptr);
            if (ret) {
                pr_err("LeveX read sector %u failed!\n", sector);
                goto out;
            }

            sec_cnt--;
            sector++;
            temp_ptr += 512;
        }
#endif
        priv->start_offset[i] = offset + len;
    }

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    if (crc32(0, buf, calc_len) != crc32(0, rdbuf, calc_len)) {
        pr_err("calc_len:%d\n", calc_len);
        pr_err("crc err at trans len %u\n", fwc->trans_size);
    }
#endif

    if (rdbuf)
        aicupg_free_align(rdbuf);
    return len;

out:
    if (rdbuf)
        aicupg_free_align(rdbuf);
    return ret;
}
#endif

/*
 * New FirmWare Component write, should write data to NOR
 *  - Erase MTD partitions for prepare write
 *  - Write data to MTD partions
 */
s32 nor_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    if (aicupg_get_fwc_attr(fwc) & FWC_ATTR_DEV_BLOCK) {
#ifdef LPKG_USING_LEVELX
        nor_fwc_levelx_write(fwc, buf, len);
#endif
    } else if (aicupg_get_fwc_attr(fwc) & FWC_ATTR_DEV_MTD) {
        nor_fwc_mtd_write(fwc, buf, len);
    } else {
        /* Do nothing */
    }

    if (len < 0) {
        return -1;
    } else {
        fwc->trans_size += len;
    }

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    return len;
}

s32 nor_fwc_data_read(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_nor_priv *priv;
    struct mtd_dev *mtd;
    unsigned long offset;
    int ret;

    priv = (struct aicupg_nor_priv *)fwc->priv;
    mtd = priv->mtd[0];
    if (!mtd)
        return 0;

    offset = priv->start_offset[0];
    ret = mtd_read(mtd, offset, buf, len);
    if (ret) {
        pr_err("read mtd %s error.\n", mtd->name);
        return 0;
    }
    priv->start_offset[0] = offset + len;

    fwc->trans_size += len;
    fwc->calc_partition_crc = fwc->meta.crc;
    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    return len;
}

/*
 * New FirmWare Component end, should free memory
 *  - free the memory what to  deposit device information
 */
void nor_fwc_data_end(struct fwc_info *fwc)
{
    struct aicupg_nor_priv *priv;

    priv = (struct aicupg_nor_priv *)fwc->priv;
    if (!priv)
        return;

#ifdef AIC_USING_SPIENC
    if (strstr(fwc->meta.name, "target.spl"))
        spienc_select_tweak(AIC_SPIENC_USER_TWEAK);
#endif
    pr_debug("trans len all %d\n", fwc->trans_size);
    free(priv);
}
