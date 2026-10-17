/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <malloc.h>
#include <aicupg.h>
#include <aic_core.h>
#include <partition_table.h>
#include "upg_internal.h"

/* Include necessary headers for device type detection */
#if defined(AICUPG_NAND_ARTINCHIP) || defined(AICUPG_NOR_ARTINCHIP)
#include <mtd.h>
#endif

#if defined(AICUPG_MMC_ARTINCHIP)
#include <disk_part.h>

/* MMC read/write wrapper functions for partition detection */
static unsigned long mmc_bread_wrapper(struct blk_desc *block_dev, u64 start, u64 blkcnt, const void *buffer)
{
    return mmc_bread(block_dev->priv, start, blkcnt, (void *)buffer);
}

static unsigned long mmc_bwrite_wrapper(struct blk_desc *block_dev, u64 start, u64 blkcnt, void *buffer)
{
    return mmc_bwrite(block_dev->priv, start, blkcnt, buffer);
}
#endif

/*
 * Check if a semicolon-separated media_type string contains the target type.
 * For multi-flash, media_type may be "spi-nor;spi-nand".
 * Returns 1 if found, 0 otherwise.
 */
static int __attribute__((unused)) media_type_match(const char *media_types, const char *target)
{
    char *copy = strdup(media_types);
    char *token;
    char *saveptr;
    int match = 0;

    if (!copy)
        return 0;

    token = strtok_r(copy, ";", &saveptr);
    while (token != NULL) {
        if (strcmp(token, target) == 0) {
            match = 1;
            break;
        }
        token = strtok_r(NULL, ";", &saveptr);
    }
    free(copy);

    return match;
}

#if defined(AICUPG_MMC_ARTINCHIP)
/**
 * Check if partition exists in MMC device
 *
 * @param part_name: Partition name to search for
 * @param dev_id: MMC device ID
 * @return: true if found, false otherwise
 */
static bool check_mmc_partition(const char *part_name, u8 dev_id)
{
    struct aic_sdmc *host;
    struct blk_desc dev_desc;
    struct disk_blk_ops ops;
    struct aic_partition *parts, *part;
    bool found = false;

    host = find_mmc_dev_by_index(dev_id);
    if (!host || !host->dev)
        return false;

    /* Setup block device descriptor */
    ops.blk_read = mmc_bread_wrapper;
    ops.blk_write = mmc_bwrite_wrapper;
    aic_disk_part_set_ops(&ops);
    dev_desc.blksz = 512;
    dev_desc.lba_count = host->dev->card_capacity * 2;
    dev_desc.priv = host;

    /* Get partition list and search */
    parts = aic_disk_get_parts(&dev_desc);
    if (!parts)
        return false;

    part = parts;
    while (part) {
        if (strcmp(part->name, part_name) == 0) {
            found = true;
            break;
        }
        part = part->next;
    }

    mmc_free_partition(parts);
    return found;
}
#endif

#if defined(AICUPG_NAND_ARTINCHIP) || defined(AICUPG_NOR_ARTINCHIP)
/**
 * Check if partition exists in MTD device and get device type
 *
 * @param part_name: Partition name to search for
 * @param type: Output parameter for device type
 * @return: true if found, false otherwise
 */
static bool check_mtd_partition(const char *part_name, enum upg_dev_type *type)
{
    struct mtd_dev *mtd;

    mtd = mtd_get_device(part_name);
    if (!mtd)
        return false;

    /* Determine device type by name or characteristics */
    if (strstr(mtd->name, "nand") != NULL) {
        *type = UPG_DEV_TYPE_SPI_NAND;
    } else if (strstr(mtd->name, "nor") != NULL) {
        *type = UPG_DEV_TYPE_SPI_NOR;
    } else {
        /* Use writesize and oobsize to determine */
        *type = (mtd->writesize < 1024 || mtd->oobsize == 0) ?
                UPG_DEV_TYPE_SPI_NOR : UPG_DEV_TYPE_SPI_NAND;
    }

    return true;
}
#endif

/**
 * Get device type and device ID by partition name
 *
 * @param part_name: Partition name to search for
 * @param dev_type: Output parameter for device type
 * @param dev_id: Output parameter for device ID
 * @return: 0 if found, -1 if not found
 */
int get_device_by_part_name(const char *part_name, enum upg_dev_type *dev_type, int *dev_id)
{
    char media_type_copy[64];
    char *token;
    char *saveptr;
    int flash_index = 0;
    const char *media_type;
    u32 media_dev_id;

    if (!part_name || !dev_type)
        return -1;

    /* Get media_type and media_dev_id through API */
    media_type = get_upg_media_type();
    media_dev_id = get_upg_media_dev_id();

    if (!media_type || !media_type[0])
        return -1;

    /* Initialize output parameters */
    *dev_type = UPG_DEV_TYPE_UNKNOWN;
    if (dev_id)
        *dev_id = -1;

    /* Copy media_type to avoid modifying the original */
    strncpy(media_type_copy, media_type, sizeof(media_type_copy) - 1);
    media_type_copy[sizeof(media_type_copy) - 1] = '\0';

    /* Parse media_type string (separated by ";") */
    token = strtok_r(media_type_copy, ";", &saveptr);
    while (token != NULL) {
        u8 current_dev_id = (media_dev_id >> (flash_index * 8)) & 0xFF;

        /* Check MMC devices */
        if (strcmp(token, "mmc") == 0) {
#if defined(AICUPG_MMC_ARTINCHIP)
            if (check_mmc_partition(part_name, current_dev_id)) {
                *dev_type = UPG_DEV_TYPE_MMC;
                if (dev_id)
                    *dev_id = current_dev_id;
                return 0;
            }
#endif
        } else if (strcmp(token, "spi-nand") == 0 || strcmp(token, "spi-nor") == 0) {
           /* Check MTD devices (SPI NAND/NOR) */
#if defined(AICUPG_NAND_ARTINCHIP) || defined(AICUPG_NOR_ARTINCHIP)
            enum upg_dev_type type;
            if (check_mtd_partition(part_name, &type)) {
                *dev_type = type;
                if (dev_id)
                    *dev_id = current_dev_id;
                return 0;
            }
#endif
        }

        token = strtok_r(NULL, ";", &saveptr);
        flash_index++;
    }

    return -1;
}

#ifdef IMAGE_CFG_JSON_PARTS_GPT
#define GPT_PART_TABLE IMAGE_CFG_JSON_PARTS_GPT
#else
#define GPT_PART_TABLE ""
#endif

/*
 * UPG_PROTO_CMD_SET_FWC_META:
 *   -> [CMD HEADER]
 *   -> [DATA from host]
 *   <- [RESP HEADER]
 *
 * In RAM device, it only receive and process one type of FWC:
 *   - Firmware information
 *   This Firmware Component describes which storage device type firmware will
 *   be written.
 */
static struct fwc_info *fwc_info_data;
static void set_fwc_meta_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s data len %d\n", __func__, cmd_data_len);
    if (cmd->cmd != UPG_PROTO_CMD_SET_FWC_META)
        return;

    cmd_state_init(cmd, CMD_STATE_START);
    if (fwc_info_data == NULL) {
        fwc_info_data = malloc(sizeof(struct fwc_info));
        if (fwc_info_data == NULL) {
            pr_debug("%s fwc_info_data = NULL\n", __func__);
            return;
        }
    }

    memset(fwc_info_data, 0, sizeof(struct fwc_info));
    fwc_info_data->burn_result = -1;
    fwc_info_data->run_result = -1;
    cmd->priv = fwc_info_data;
}

static s32 set_fwc_meta_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                             s32 len)
{
    struct fwc_info *fwc;
    s32 clen = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_DATA_IN);

    if (cmd->state == CMD_STATE_DATA_IN) {
        /* Data length should be FWC_META_DATA_LEN */
        if (len > FWC_META_DATA_LEN)
            len = FWC_META_DATA_LEN;
        memcpy(&fwc->meta, buf, len);
        clen = len;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 set_fwc_meta_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                             s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    s32 siz = 0;

    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void set_fwc_meta_cmd_end(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;

    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return;

    printf("Firmware Component:\n");
    printf("    name:      %s\n", fwc->meta.name);
    printf("    partition: %s\n", fwc->meta.partition);
    printf("    attr:      %s\n", fwc->meta.attr);
    if (cmd->state == CMD_STATE_END) {
        if (!memcmp(fwc->meta.name, "image.updater", 13)) {
            set_current_device_type(UPG_DEV_TYPE_RAM);
            set_current_device_id(0);
        } else {
            int dev_id;
            enum upg_dev_type dev_type;

            if (get_device_by_part_name(fwc->meta.partition, &dev_type, &dev_id) == 0) {
                set_current_device_type(dev_type);
                set_current_device_id(dev_id);
            }
        }

        fwc->start_us = aic_get_time_us();

        dev_type = get_current_device_type();
        printf("    Media:     %s(%d)\n", get_current_device_name(dev_type),
               dev_type);
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ram_fwc_start(fwc);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_start(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
}

/*
 * UPG_PROTO_CMD_GET_BLOCK_SIZE:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [BLOCK SIZE]
 */
static void get_block_size_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_BLOCK_SIZE)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_block_size_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                               s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_block_size_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                               s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->block_size;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void get_block_size_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_SEND_FWC_DATA:
 *   -> [CMD HEADER]
 *   -> [DATA PKT]
 *   -> [DATA PKT]
 *   -> ...
 *   <- [RESP HEADER]
 */
static void send_fwc_data_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_SEND_FWC_DATA) {
        pr_debug("cmd is not UPG_PROTO_CMD_SEND_FWC_DATA\n");
        return;
    }
    /* fwc meta data should be sent before SEND_FWC_DATA */
    if (fwc_info_data == NULL) {
        pr_debug("fwc_info_data is NULL\n");
        return;
    }

    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 send_fwc_data_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;
    s32 clen = 0, ret = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL) {
        pr_debug("fwc info is NULL\n");
        return 0;
    }

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_DATA_IN);

    if (cmd->state == CMD_STATE_DATA_IN) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ret = ram_fwc_data_write(fwc, buf, len);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_data_write(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_data_write(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_data_write(fwc, buf, len);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
                pr_err("Unknown device.\n");
            default:
                break;
        }

        clen = ret;
        if (fwc->trans_size >= fwc->meta.size) {
            fwc->burn_result = 0;
            fwc->run_result = 0;
            cmd_state_set_next(cmd, CMD_STATE_RESP);
        }
    }

    pr_debug("%s, l: %d\n", __func__, __LINE__);
    return clen;
}

static s32 send_fwc_data_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL) {
        pr_debug("fwc info is NULL\n");
        return 0;
    }

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    } else if (cmd->state == CMD_STATE_DATA_IN) {
        /* It must be something wrong. */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_FAIL, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void send_fwc_data_cmd_end(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;
    u64 total_len = 0;
    u32 start_us;

    fwc = (struct fwc_info *)cmd->priv;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ram_fwc_data_end(fwc);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_data_end(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }

    total_len = fwc->trans_size;
    start_us = aic_get_time_us() - fwc->start_us;
    pr_info("    Partition: %s programming done.\n", fwc->meta.partition);
    pr_info("    Used time: %u.%u sec, Speed: %lu.%lu KB/s.\n",
            start_us / 1000000, start_us / 1000 % 1000,
            (ulong)((total_len * 1000000) / start_us / 1024),
            (ulong)((total_len * 1000000) / start_us % 1024));

    pr_debug("%s, l: %d\n", __func__, __LINE__);
}

/*
 * UPG_PROTO_CMD_GET_FWC_CRC:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC CRC]
 */
static void get_fwc_crc_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_CRC)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_crc_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                            s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_crc_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                            s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->calc_partition_crc;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_crc_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_FWC_BURN_RESULT:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC BURN RESULT]
 */
static void get_fwc_burn_result_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_BURN_RESULT)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_burn_result_cmd_write_input_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_burn_result_cmd_read_output_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->burn_result ? 1 : 0;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_burn_result_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_FWC_RUN_RESULT:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC RUN RESULT]
 */
static void get_fwc_run_result_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_RUN_RESULT)
        return;
    /* fwc meta data should be sent before GET_FWC_CRC */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_run_result_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                                   s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_run_result_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                   s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->run_result ? 1 : 0;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_run_result_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_STORAGE_MEDIA:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [MEDIA LIST]
 */
static void get_storage_media_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_STORAGE_MEDIA)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 get_storage_media_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    pr_debug("%s\n", __func__);
    /* No input data for this command */
    return 0;
}

static s32 get_storage_media_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    struct resp_header resp;
    struct storage_media medias[5] = { 0 };
    u32 siz = 0, i = 0;

    pr_debug("%s\n", __func__);
#if defined(AICUPG_MMC_ARTINCHIP)
    if (mmc_is_exist(0)) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "mmc");
    }
    if (mmc_is_exist(1)) {
        medias[i].media_dev_id = 1;
        strcpy(medias[i++].media_type, "mmc");
    }
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
    if (nand_is_exist(0)) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "spi-nand");
    }
    if (nand_is_exist(1)) {
        medias[i].media_dev_id = 1;
        strcpy(medias[i++].media_type, "spi-nand");
    }
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
    if (nor_is_exist(0)) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "spi-nor");
    }
    if (nor_is_exist(1)) {
        medias[i].media_dev_id = 1;
        strcpy(medias[i++].media_type, "spi-nor");
    }
#endif

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        siz = sizeof(struct storage_media) * i;
        aicupg_gen_resp(&resp, cmd->cmd, 0, siz);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        memcpy(buf, &medias, sizeof(struct storage_media) * i);
        siz += sizeof(struct storage_media) * i;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_storage_media_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}
/*
 * UPG_PROTO_CMD_GET_PARTITION_TABLE:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [PARTITION TABLE]
 */
static void get_partition_table_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    static struct storage_media media = { 0 };

    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_PARTITION_TABLE)
        return;

    cmd->priv = &media;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 get_partition_table_cmd_write_input_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct storage_media *priv;
    u32 clen = 0, siz = 0;

    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);
    if (cmd->state == CMD_STATE_ARG) {
        siz = sizeof(struct storage_media);
        if (len < siz)
            return 0;
        memcpy(priv, buf, siz);
        clen += siz;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 get_partition_table_cmd_read_output_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct resp_header resp;
    struct storage_media *priv;
    char *part_table = "";
    u32 siz = 0;

    pr_debug("%s\n", __func__);
    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, PARTITION_TABLE_LEN);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    printf("media_type:%s\n", priv->media_type);
    if (cmd->state == CMD_STATE_DATA_OUT) {
        if (priv->media_type[0] == 0)
            return 0;
#if defined(AICUPG_MMC_ARTINCHIP)
        else if (!memcmp(priv->media_type, "mmc", 3))
            part_table = GPT_PART_TABLE;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        else if (!memcmp(priv->media_type, "spi-nand", 8))
            part_table = IMAGE_CFG_JSON_PARTS_MTD;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        else if (!memcmp(priv->media_type, "spi-nor", 7))
            part_table = IMAGE_CFG_JSON_PARTS_MTD;
#endif
        else
            return 0;
        printf("part table:%s\n", part_table);
        memcpy(buf, part_table, PARTITION_TABLE_LEN);
        siz += PARTITION_TABLE_LEN;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void get_partition_table_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_READ_FWC_DATA:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC PARTITION DATA]
 */
static void read_fwc_data_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_READ_FWC_DATA)
        return;

    if (fwc_info_data == NULL) {
        fwc_info_data = malloc(sizeof(struct fwc_info));
        if (fwc_info_data == NULL) {
            pr_debug("%s fwc_info_data = NULL\n", __func__);
            return;
        }
    }

    memset(fwc_info_data, 0, sizeof(struct fwc_info));
    cmd->priv = fwc_info_data;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 read_fwc_data_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    struct fwc_info *fwc;
    s32 clen = 0, siz = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        siz = sizeof(struct media_partition);
        if (len < siz)
            return 0;
        memcpy(&(fwc->mpart), buf, siz);
        clen += siz;
        cmd_state_set_next(cmd, CMD_STATE_RESP);

        strcpy(fwc->meta.partition, fwc->mpart.part.name);
        strcpy(fwc->meta.attr, fwc->mpart.attr);
        fwc->meta.offset = fwc->mpart.part.start;
        fwc->meta.size = fwc->mpart.part.size;
    }

    return clen;
}

static s32 read_fwc_data_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    enum upg_dev_type dev_type;
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, ret = 0, clen = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        siz = fwc->mpart.part.size;
        aicupg_gen_resp(&resp, cmd->cmd, 0, siz);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);

        dev_type = get_media_type(fwc);
        set_current_device_type(dev_type);
        pr_info("    Media: %s(%d)\n", get_current_device_name(dev_type),
                dev_type);

        switch (dev_type) {
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NAND prepare failed\n");
                mmc_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NAND prepare failed\n");
                nand_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NOR prepare failed\n");
                nor_fwc_start(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
    }

    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        dev_type = get_current_device_type();
        switch (dev_type) {
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_data_read(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_data_read(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_data_read(fwc, buf, len);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
                pr_err("Unknown device.\n");
            default:
                break;
        }
        clen = ret;
        if (fwc->trans_size >= fwc->mpart.part.size)
            cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return clen;
}

static void read_fwc_data_cmd_end(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;

    fwc = (struct fwc_info *)cmd->priv;
    (void)fwc;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_data_end(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
    pr_debug("%s, l: %d\n", __func__, __LINE__);
}

/*
 * UPG_PROTO_CMD_SET_UART_ARGS:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC PARTITION DATA]
 */
static void set_uart_args_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s data len %d\n", __func__, cmd_data_len);
    if (cmd->cmd != UPG_PROTO_CMD_SET_UART_ARGS)
        return;

    cmd->priv = 0;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 set_uart_args_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf, s32 len)
{
    s32 clen = 0;
    u32 baudrate = 0;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        /*
         * Enter recv argument state
         */
        if (len < 4)
            return 0;
        memcpy(&baudrate, buf, 4);
        clen = 4;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }
    cmd->priv = (void *)(uintptr_t)baudrate;

    return clen;
}

static s32 set_uart_args_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf, s32 len)
{
    struct resp_header resp;
    s32 siz = 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, (unsigned long)UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

extern void aic_uart_upg_baudrate_update(int baudrate);
static void set_uart_args_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
#ifdef AICUPG_UART_ENABLE
        int baudrate = (uintptr_t)cmd->priv;
        aic_uart_upg_baudrate_update(baudrate);
#endif
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
}

/*
 * UPG_PROTO_CMD_GET_STORAGE_GEOME:
 *   -> [CMD HEADER]
 *   -> [CMD ARGS]
 *   <- [RESP HEADER]
 *   <- [STORAGE GEOMETRY]
 */
static void get_storage_geome_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    struct storage_media *media_info;

    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_STORAGE_GEOME)
        return;
    media_info = malloc(sizeof(struct storage_media));
    if (!media_info)
        return;
    memset(media_info, 0, sizeof(*media_info));
    cmd->priv = media_info;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 get_storage_geome_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    struct storage_media *priv;
    u32 clen;

    clen = 0;
    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        /*
         * Enter recv argument state
         */
        if (len < sizeof(*priv))
            return 0;
        memcpy(priv, buf, sizeof(*priv));
        clen += sizeof(*priv);
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 get_storage_geome_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    struct storage_media *priv;
    struct resp_header resp;
    u32 siz = 0;
    u8 *p;

    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;
    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        siz = sizeof(struct storage_geometry);
        aicupg_gen_resp(&resp, cmd->cmd, 0, siz);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;
    if (cmd->state == CMD_STATE_DATA_OUT) {
        struct storage_geometry geome;

        memset(&geome, 0, sizeof(geome));
#if defined(AICUPG_MMC_ARTINCHIP)
        if (strncmp(priv->media_type, "mmc", 3) == 0)
            mmc_get_geometry(priv->media_dev_id, &geome);
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        if (strncmp(priv->media_type, "spi-nor", 7) == 0)
            nor_get_geometry(priv->media_dev_id, &geome);
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        if (strncmp(priv->media_type, "spi-nand", 8) == 0)
            nand_get_geometry(priv->media_dev_id, &geome);
#endif

        /* Enter read DATA state */
        p = (u8 *)&geome;
        memcpy(buf + siz, p, sizeof(geome));
        siz += sizeof(geome);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_storage_geome_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    free((void *)cmd->priv);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

struct storage_erase_args {
    struct storage_media media;
    u32 pad;
    struct storage_erase erase;
} __packed;

/*
 * UPG_PROTO_CMD_ERASE_STORAGE:
 *   -> [CMD HEADER]
 *   -> [CMD ARGS]
 *   <- [RESP]
 */
static void erase_storage_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    struct storage_erase_args *erase_info;

    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_ERASE_STORAGE)
        return;
    erase_info = malloc(sizeof(struct storage_erase_args));
    if (!erase_info)
        return;

    memset(erase_info, 0, sizeof(struct storage_erase_args));
    cmd->priv = erase_info;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 erase_storage_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf, s32 len)
{
    struct storage_erase_args *erase_info;
    u32 clen;

    clen = 0;
    erase_info = (struct storage_erase_args *)cmd->priv;
    if (erase_info == NULL)
        return 0;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        /*
         * Enter recv argument state
         */
        if (len < sizeof(*erase_info))
            return 0;

        memcpy(erase_info, buf, sizeof(*erase_info));
        clen += sizeof(*erase_info);
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 erase_storage_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    struct storage_erase_args *erase_info;
    struct resp_header resp;
    u32 siz = 0;
    s32 ret = -1;

    erase_info = (struct storage_erase_args *)cmd->priv;
    if (cmd->state == CMD_STATE_RESP) {
        // Call erase
#if defined(AICUPG_MMC_ARTINCHIP)
        if (strncmp(erase_info->media.media_type, "mmc", 3) == 0)
            ret = mmc_storage_erase(erase_info->media.media_dev_id, &erase_info->erase);
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        if (strncmp(erase_info->media.media_type, "spi-nor", 7) == 0)
            ret = nor_storage_erase(erase_info->media.media_dev_id, &erase_info->erase);
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        if (strncmp(erase_info->media.media_type, "spi-nand", 8) == 0)
            ret = nand_storage_erase(erase_info->media.media_dev_id, &erase_info->erase);
#endif
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        if (ret)
            aicupg_gen_resp(&resp, cmd->cmd, (unsigned long)UPG_RESP_FAIL, 0);
        else
            aicupg_gen_resp(&resp, cmd->cmd, (unsigned long)UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void erase_storage_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    free((void *)cmd->priv);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

static struct upg_cmd fwc_cmd_list[] = {
    {
        UPG_PROTO_CMD_SET_FWC_META,
        set_fwc_meta_cmd_start,
        set_fwc_meta_cmd_write_input_data,
        set_fwc_meta_cmd_read_output_data,
        set_fwc_meta_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_BLOCK_SIZE,
        get_block_size_cmd_start,
        get_block_size_cmd_write_input_data,
        get_block_size_cmd_read_output_data,
        get_block_size_cmd_end,
    },
    {
        UPG_PROTO_CMD_SEND_FWC_DATA,
        send_fwc_data_cmd_start,
        send_fwc_data_cmd_write_input_data,
        send_fwc_data_cmd_read_output_data,
        send_fwc_data_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_CRC,
        get_fwc_crc_cmd_start,
        get_fwc_crc_cmd_write_input_data,
        get_fwc_crc_cmd_read_output_data,
        get_fwc_crc_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_BURN_RESULT,
        get_fwc_burn_result_cmd_start,
        get_fwc_burn_result_cmd_write_input_data,
        get_fwc_burn_result_cmd_read_output_data,
        get_fwc_burn_result_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_RUN_RESULT,
        get_fwc_run_result_cmd_start,
        get_fwc_run_result_cmd_write_input_data,
        get_fwc_run_result_cmd_read_output_data,
        get_fwc_run_result_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_STORAGE_MEDIA,
        get_storage_media_cmd_start,
        get_storage_media_cmd_write_input_data,
        get_storage_media_cmd_read_output_data,
        get_storage_media_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_PARTITION_TABLE,
        get_partition_table_cmd_start,
        get_partition_table_cmd_write_input_data,
        get_partition_table_cmd_read_output_data,
        get_partition_table_cmd_end,
    },
    {
        UPG_PROTO_CMD_READ_FWC_DATA,
        read_fwc_data_cmd_start,
        read_fwc_data_cmd_write_input_data,
        read_fwc_data_cmd_read_output_data,
        read_fwc_data_cmd_end,
    },
    {
        UPG_PROTO_CMD_SET_UART_ARGS,
        set_uart_args_cmd_start,
        set_uart_args_cmd_write_input_data,
        set_uart_args_cmd_read_output_data,
        set_uart_args_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_STORAGE_GEOME,
        get_storage_geome_cmd_start,
        get_storage_geome_cmd_write_input_data,
        get_storage_geome_cmd_read_output_data,
        get_storage_geome_cmd_end,
    },
    {
        UPG_PROTO_CMD_ERASE_STORAGE,
        erase_storage_cmd_start,
        erase_storage_cmd_write_input_data,
        erase_storage_cmd_read_output_data,
        erase_storage_cmd_end,
    },
};

struct upg_cmd *find_fwc_command(struct cmd_header *h)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(fwc_cmd_list); i++) {
        if (fwc_cmd_list[i].cmd == (u32)h->command) {
            return &fwc_cmd_list[i];
        }
    }

    return NULL;
}
