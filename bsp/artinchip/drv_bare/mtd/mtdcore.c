/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <mtd.h>

static LIST_HEAD(mtd_dev_list);

int mtd_add_device(struct mtd_dev *mtd)
{
    struct mtd_dev *item;

    if (!mtd)
        return -EINVAL;

    item = mtd_get_device(mtd->name);
    if (item)
        return -EEXIST;

    list_add_tail(&mtd->list, &mtd_dev_list);
    return 0;
}

int mtd_del_device(struct mtd_dev *mtd)
{
    struct mtd_dev *item;

    if (!mtd)
        return -EINVAL;

    item = mtd_get_device(mtd->name);
    if (item != mtd)
        return -EEXIST;

    list_del(&mtd->list);
    return 0;
}

struct mtd_dev *mtd_get_device(const char *name)
{
    struct mtd_dev *item;
    struct list_head *pos;

    if (!name) {
        return NULL;
    }

    list_for_each(pos, &mtd_dev_list) {
        item = list_entry(pos, struct mtd_dev, list);
        if (!strcmp(item->name, name))
            return item;
    }
    return NULL;
}

u32 mtd_get_device_count(void)
{
    struct list_head *pos;
    u32 cnt;

    cnt = 0;

    list_for_each(pos, &mtd_dev_list) {
        cnt++;
    }
    return cnt;
}

struct mtd_dev *mtd_get_device_by_id(u32 id)
{
    struct mtd_dev *item;
    struct list_head *pos;
    u32 cnt;

    cnt = 0;
    item = NULL;

    list_for_each(pos, &mtd_dev_list) {
        item = list_entry(pos, struct mtd_dev, list);
        if (cnt == id)
            break;
        cnt++;
    }
    return item;
}

int mtd_read(struct mtd_dev *mtd, u32 offset, uint8_t *data, u32 len)
{
    if (mtd && mtd->ops.read)
        return mtd->ops.read(mtd, offset, data, len);
    return -1;
}

int mtd_contread(struct mtd_dev *mtd, u32 offset, uint8_t *data, u32 len)
{
    if (mtd && mtd->ops.cont_read)
        return mtd->ops.cont_read(mtd, offset, data, len);
    return -1;
}

int mtd_erase(struct mtd_dev *mtd, u32 offset, u32 len)
{
    if (mtd && mtd->ops.erase)
        return mtd->ops.erase(mtd, offset, len);
    return -1;
}

/*
 * Since NAND Flash returns errors every time a page is written, this function cannot accurately
 * return the true error page number. Therefore, we should use mtd_write_oob() to write a page.
 */
int mtd_write(struct mtd_dev *mtd, u32 offset, uint8_t *data, u32 len)
{
    if (mtd && mtd->ops.write)
        return mtd->ops.write(mtd, offset, data, len);
    return -1;
}

int mtd_read_oob(struct mtd_dev *mtd, u32 offset, uint8_t *data, u32 len, uint8_t *spare_data, u32 spare_len)
{
    if (mtd && mtd->ops.read_oob)
        return mtd->ops.read_oob(mtd, offset, data, len, spare_data, spare_len);
    return -1;
}

int mtd_write_oob(struct mtd_dev *mtd, u32 offset, uint8_t *data, u32 len, uint8_t *spare_data, u32 spare_len)
{
    if (mtd && mtd->ops.write_oob)
        return mtd->ops.write_oob(mtd, offset, data, len, spare_data, spare_len);
    return -1;
}

int mtd_block_isbad(struct mtd_dev *mtd, u32 offset)
{
    if (mtd && mtd->ops.block_isbad)
        return mtd->ops.block_isbad(mtd, offset);
    return -1;
}

int mtd_block_markbad(struct mtd_dev *mtd, u32 offset)
{
    if (mtd && mtd->ops.block_markbad)
        return mtd->ops.block_markbad(mtd, offset);
    return -1;
}

int mtd_map_oob_user_region(struct mtd_dev *mtd, u8 *oobbuf, u8 *buf, int start, int nbytes)
{
    if (mtd && mtd->ops.map_user)
        return mtd->ops.map_user(mtd, oobbuf, buf, start, nbytes);
    return -1;
}

int mtd_unmap_oob_user_region(struct mtd_dev *mtd, u8 *dst, u8* src, int start, int nbytes)
{
    if (mtd && mtd->ops.unmap_user)
        return mtd->ops.unmap_user(mtd, dst, src, start, nbytes);
    return -1;
}

u32 mtd_trans_lga_to_pha(struct mtd_dev *mtd, u32 offset)
{
    if (mtd && mtd->ops.la2pa)
        return mtd->ops.la2pa(mtd, offset);
    return offset;
}
