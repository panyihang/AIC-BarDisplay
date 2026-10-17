/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <aic_common.h>
#include <aic_image.h>
#include <boot_param.h>
#include <private_param.h>
#include <partition_table.h>
#include <aic_utils.h>
#include <mtd.h>
#include <aic_log.h>
#include <aic_osal.h>

char *aic_spinor_get_partition_string(struct mtd_dev *mtd)
{
    char *parts = NULL;

#ifdef AIC_BOOTLOADER
    void *res_addr;
    res_addr = aic_get_boot_resource();
    parts = private_get_partition_string(res_addr);
    if (parts == NULL)
        parts = IMAGE_CFG_JSON_PARTS_MTD;
    if (parts)
        parts = strdup(parts);
#else
    uint8_t head_buf[256], *res;
    struct aic_image_header head;
    int err;

    err = mtd->ops.read(mtd, 0, head_buf, 256);
    if (err) {
        pr_err("Failed to read aic image head.\n");
        return NULL;
    }

    memcpy(&head, head_buf, sizeof(head));
    if (head.magic != AIC_IMAGE_MAGIC) {
        pr_err("aic image head verify failure.");
        return NULL;
    }
    if (head.private_data_offset) {
        res = malloc(head.private_data_len);
        if (res == NULL) {
            pr_err("Failed to malloc resource buffer.\n");
            return NULL;
        }
        err = mtd->ops.read(mtd, head.private_data_offset, res,
                            head.private_data_len);
        if (err) {
            pr_err("Failed to read aic image head.");
            return NULL;
        }
        parts = private_get_partition_string(res);
        if (parts == NULL)
            parts = IMAGE_CFG_JSON_PARTS_MTD;
        if (parts)
            parts = strdup(parts);
        free(res);
    }
#endif
    return parts;
}
