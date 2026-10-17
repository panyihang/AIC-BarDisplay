/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <malloc.h>
#include <aicupg.h>
#include <aic_core.h>
#include <aic_crc32.h>
#include "upg_internal.h"

struct artinchip_image_info {
    char magic[8];
    char platform[64];
    char product[64];
    char version[64];
    char media_type[64];
    u32 media_dev_id;
    u8 media_id[64]; /* NAND ID or MMC ID */
    u32 meta_offset; /* Meta Area start offset */
    u32 meta_size;   /* Meta Area size */
    u32 file_offset; /* File data Area start offset */
    u32 file_size;   /* File data Area size */
};

static struct artinchip_image_info image_info;
void ram_fwc_start(struct fwc_info *fwc)
{
    pr_debug("%s, FWC name %s\n", __func__, fwc->meta.name);
    if (strcmp(fwc->meta.name, "image.info") != 0) {
        fwc->priv = 0; /* Don't process it */
    } else {
        fwc->priv = &image_info;
        memset(&image_info, 0, sizeof(image_info));
    }
    fwc->block_size = 1;
}

/*
 * Get device type from image info by flash_index
 * - For multi-flash, media_type uses ";" as separator, e.g. "spi-nor;spi-nand"
 * - flash_index selects which token to return
 * - A stack-local copy is used because strtok_r modifies the string in-place
 */
static enum upg_dev_type get_device_type(struct artinchip_image_info *info, int flash_index)
{
    char media_type_copy[64];
    char *token;
    char *saveptr;
    enum upg_dev_type type = UPG_DEV_TYPE_RAM;
    int index = 0;

    pr_debug("%s, %s\n", __func__, info->media_type);

    /* Copy to avoid modifying the original info string */
    strncpy(media_type_copy, info->media_type, sizeof(media_type_copy) - 1);
    media_type_copy[sizeof(media_type_copy) - 1] = '\0';

    /* Split by ";" and walk to the flash_index-th token */
    token = strtok_r(media_type_copy, ";", &saveptr);
    while (token != NULL && index <= flash_index) {
        if (index == flash_index) {
            if (strcmp(token, "mmc") == 0)
                type = UPG_DEV_TYPE_MMC;
            else if (strcmp(token, "raw-nand") == 0)
                type = UPG_DEV_TYPE_RAW_NAND;
            else if (strcmp(token, "spi-nand") == 0)
                type = UPG_DEV_TYPE_SPI_NAND;
            else if (strcmp(token, "spi-nor") == 0)
                type = UPG_DEV_TYPE_SPI_NOR;
            break;
        }
        token = strtok_r(NULL, ";", &saveptr);
        index++;
    }

    return type;
}

enum upg_dev_type get_media_type(struct fwc_info *fwc)
{
    enum upg_dev_type type = UPG_DEV_TYPE_RAM;

    pr_debug("%s, %s\n", __func__, fwc->mpart.media.media_type);
    if (strcmp(fwc->mpart.media.media_type, "mmc") == 0)
        type = UPG_DEV_TYPE_MMC;
    else if (strcmp(fwc->mpart.media.media_type, "raw-nand") == 0)
        type = UPG_DEV_TYPE_RAW_NAND;
    else if (strcmp(fwc->mpart.media.media_type, "spi-nand") == 0)
        type = UPG_DEV_TYPE_SPI_NAND;
    else if (strcmp(fwc->mpart.media.media_type, "spi-nor") == 0)
        type = UPG_DEV_TYPE_SPI_NOR;

    return type;
}

/*
 * Process image info and prepare all flashes for upgrade
 * - For multi-flash, iterate all flashes parsed from media_type
 * - Each flash ID is extracted from media_dev_id (1 byte per flash)
 * - Call the corresponding prepare function for each flash
 */
static s32 image_info_proc(struct fwc_info *fwc,
                           struct artinchip_image_info *info)
{
    enum upg_dev_type type;
    s32 ret = 0;
    u8 dev_id;
    char *media_type_copy;
    char *token;
    char *saveptr;
    int flash_count = 0;
    int i;

    /* Count how many flashes are specified in media_type (separated by ";") */
    media_type_copy = strdup(info->media_type);
    if (!media_type_copy) {
        pr_err("Memory allocation failed\n");
        return -1;
    }

    token = strtok_r(media_type_copy, ";", &saveptr);
    while (token != NULL) {
        flash_count++;
        token = strtok_r(NULL, ";", &saveptr);
    }
    free(media_type_copy);

    /* Prepare each flash in order */
    for (i = 0; i < flash_count; i++) {
        type = get_device_type(info, i);
        /* Extract the i-th flash ID from media_dev_id (1 byte per flash) */
        dev_id = (info->media_dev_id >> (i * 8)) & 0xFF;

        pr_info("Write to: %s(%d), dev_id: %d\n",
                get_current_device_name(type), type, dev_id);

#if defined(AICUPG_MMC_ARTINCHIP)
        if (type == UPG_DEV_TYPE_MMC) {
            ret = mmc_fwc_prepare(fwc, dev_id);
            if (ret < 0) {
                pr_err("MMC prepare failed\n");
                return ret;
            }
        }
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        if (type == UPG_DEV_TYPE_SPI_NAND) {
            ret = nand_fwc_prepare(fwc, dev_id);
            if (ret < 0) {
                pr_err("NAND prepare failed\n");
                return ret;
            }
        }
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        if (type == UPG_DEV_TYPE_SPI_NOR) {
            ret = nor_fwc_prepare(fwc, dev_id);
            if (ret < 0) {
                pr_err("NOR prepare failed\n");
                return ret;
            }
        }
#endif
    }

    return 0;
}

/*
 * Current implementation, only one special RAM FWC need to be processed,
 *     image.info
 * This FWC is the same with fiwmware image header, and we can get information
 * about media type which firmware will be burn.
 *
 * This FWC will be sent before any FWC in "target" is coming.
 */
s32 ram_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct artinchip_image_info *info;

    fwc->trans_size += len;
    fwc->calc_partition_crc = crc32(fwc->calc_partition_crc, buf, len);

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

    if (fwc->priv == 0)
        return len;

    /* Only process image info data */
    if (len >= sizeof(*info)) {
        info = (struct artinchip_image_info *)fwc->priv;
        memcpy(info, buf, sizeof(*info));
        image_info_proc(fwc, info);
    }

    pr_debug("%s, l: %d\n", __func__, __LINE__);
    return len;
}

void ram_fwc_data_end(struct fwc_info *fwc)
{
    struct artinchip_image_info *info;
    enum upg_dev_type type;
    u8 dev_id;
    int flash_index;

    if (fwc->priv) {
        info = (struct artinchip_image_info *)fwc->priv;

        /* Use the first flash type and ID for following FWCs */
        flash_index = 0;
        type = get_device_type(info, flash_index);
        dev_id = (info->media_dev_id >> (flash_index * 8)) & 0xFF;

        set_current_device_type(type);
        set_current_device_id(dev_id);
    }
}
