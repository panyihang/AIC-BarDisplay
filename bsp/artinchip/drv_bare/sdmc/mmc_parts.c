/*
 * Copyright (c) 2023-2024, Artinchip Technology Co., Ltd
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
#include <private_param.h>
#include <partition_table.h>
#include <boot_param.h>
#include <aic_image.h>
#include <aic_partition.h>
#include "sdmc.h"

#define MAX_PART_NAME    32
#define GPT_HEADER_SIZE  (34 * 512)

#ifdef IMAGE_CFG_JSON_PARTS_GPT
#define MMC_GPT_PARTS IMAGE_CFG_JSON_PARTS_GPT
#else
#define MMC_GPT_PARTS ""
#endif

struct aic_partition *mmc_new_partition(char *s, u64 start)
{
    return aic_part_gpt_parse(s);
}

void mmc_free_partition(struct aic_partition *part)
{
    aic_part_free(part);
}

char *aic_mmc_get_partition_string(int mmc_id)
{
    char *parts = NULL;

#ifdef AIC_BOOTLOADER
    void *res_addr;
    res_addr = aic_get_boot_resource();
    parts = private_get_partition_string(res_addr);
    if (parts == NULL)
        parts = MMC_GPT_PARTS;
    if (parts)
        parts = strdup(parts);
#else
    uint8_t head_buf[512], *res;
    struct aic_image_header head;
    struct aic_sdmc *sdmc = NULL;
    u64 blkstart, blkcnt;
    u32 ret;

    sdmc = find_mmc_dev_by_index(mmc_id);
    if (!sdmc) {
        parts = MMC_GPT_PARTS;
        return parts;
    }

    /* First partition start from blk#34 */
    blkstart = GPT_HEADER_SIZE / 512;
    ret = mmc_bread(sdmc, blkstart, 1, (void *)head_buf);
    if (ret <= 0) {
        pr_err("Failed to read aic image head.\n");
        return NULL;
    }

    memcpy(&head, head_buf, sizeof(head));
    if (head.magic != AIC_IMAGE_MAGIC) {
        pr_err("aic image head verify failure.");
        return NULL;
    }
    if (head.private_data_offset) {
        blkstart += (head.private_data_offset + 511) / 512;
        blkcnt = (head.private_data_len + 511) / 512;
        res = malloc(blkcnt * 512);
        if (res == NULL) {
            pr_err("Failed to malloc resource buffer.\n");
            return NULL;
        }
        ret = mmc_bread(sdmc, blkstart, blkcnt, (void *)res);
        if (ret <= 0) {
            pr_err("Failed to read aic image head.");
            return NULL;
        }
        parts = private_get_partition_string(res);
        if (parts == NULL)
            parts = MMC_GPT_PARTS;
        if (parts)
            parts = strdup(parts);
        free(res);
    }
#endif
    return parts;
}

struct aic_partition *mmc_create_gpt_part2(int mmc_id)
{
    struct aic_partition *parts = NULL;
    char *partstr;

    partstr = aic_mmc_get_partition_string(mmc_id);
    parts = aic_part_gpt_parse(partstr);
    if (!parts)
        return NULL;

    if (parts->start != GPT_HEADER_SIZE) {
        pr_err("First partition start offset is not correct\n");
        goto out;
    }

    return parts;

out:
    if (parts)
        aic_part_free(parts);

    return NULL;
}

/* Legacy API, only for eMMC */
struct aic_partition *mmc_create_gpt_part(void)
{
    return mmc_create_gpt_part2(0);
}
