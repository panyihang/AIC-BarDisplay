/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Xiong Hao <hao.xiong@artinchip.com>
 */

#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_utils.h>
#include <aic_crc32.h>
#include <aicupg.h>
#include <sparse_format.h>
#include <partition_table.h>
#include <disk_part.h>
#include "upg_internal.h"

#define MMC_BLOCK_SIZE   512
#define SPARSE_FILLBUF_SIZE (1024 * 1024)

struct aicupg_mmc_priv {
    struct aic_sdmc *host;
    struct aic_partition *parts;
    sparse_header_t sparse_header;
    chunk_header_t chunk_header;
    unsigned char remain_data[MMC_BLOCK_SIZE];
    unsigned int remain_len;
    ulong blkstart;
    int cur_chunk;
    int cur_chunk_remain_data_sz;
    int cur_chunk_burned_data_sz;
    int is_sparse;
};

int mmc_is_exist(u32 id)
{
    struct aic_sdmc *host;

    host = find_mmc_dev_by_index(id);
    if (host != NULL) {
        return 1;
    }
    return 0;
}

s32 mmc_get_geometry(u32 id, struct storage_geometry *geo)
{
    struct aic_sdmc *host;
    if (geo == NULL)
        return -1;

    host = find_mmc_dev_by_index(id);
    if (host != NULL) {
        geo->total_size = host->dev->card_capacity * 2;
        geo->write_size = 512;
        geo->erase_size = host->dev->erase_grp_size;
        geo->oob_size = 0;
        return 0;
    }
    return -1;
}

s32 mmc_storage_erase(u32 id, struct storage_erase *erase)
{
    struct aic_sdmc *host;
    u32 start, cnt, ret = 0;

    if (erase == NULL)
        return -1;

    host = find_mmc_dev_by_index(id);
    if (host != NULL) {
        /* The start sector and sector count of MMC */
        start = erase->start;
        cnt = erase->size;
        ret = mmc_berase(host, start, cnt);
        if (ret != cnt)
            return -1;
        return 0;
    }

    return -1;
}


static unsigned long mmc_write(struct blk_desc *block_dev, u64 start,
                               u64 blkcnt, void *buffer)
{
    return mmc_bwrite(block_dev->priv, start, blkcnt, buffer);
}

static unsigned long mmc_read(struct blk_desc *block_dev, u64 start, u64 blkcnt,
                              const void *buffer)
{
    return mmc_bread(block_dev->priv, start, blkcnt, (void *)buffer);
}

s32 mmc_fwc_prepare(struct fwc_info *fwc, u32 mmc_id)
{
    int ret = 0;
    struct aic_sdmc *host;
    struct disk_blk_ops ops;
    struct blk_desc dev_desc;
    struct aic_partition *parts;

    ret = mmc_init(mmc_id);
    if (ret) {
        pr_err("sdmc %d init failed.\n", mmc_id);
        return ret;
    }

    /* Create GPT partition table */
    parts = mmc_create_gpt_part2(mmc_id);
    if (!parts) {
        pr_err("sdmc %d create gpt part failed.\n", mmc_id);
        return -1;
    }

    /* Get host for dev_desc */
    host = find_mmc_dev_by_index(mmc_id);
    if (!host) {
        pr_err("find mmc dev %d failed.\n", mmc_id);
        mmc_free_partition(parts);
        return -1;
    }

    /* Setup block device operations */
    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);
    dev_desc.blksz = MMC_BLOCK_SIZE;
    dev_desc.lba_count = host->dev->card_capacity * 2;
    dev_desc.priv = host;

    /* Write GPT partition table to disk */
    ret = aic_disk_write_gpt(&dev_desc, parts);
    if (ret) {
        pr_err("Write PART table failed.\n");
        mmc_free_partition(parts);
        return ret;
    }

    /* Refresh MMC block device */
    mmc_block_refresh(host);

    /* Free the temporary partition table */
    mmc_free_partition(parts);

    return 0;
}

struct aic_partition *mmc_fwc_get_part_by_name(struct fwc_info *fwc, char *name)
{
    struct aicupg_mmc_priv *priv;
    struct aic_partition *parts = NULL;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    parts = priv->parts;
    while (parts) {
        if (!strcmp(parts->name, name))
            return parts;

        parts = parts->next;
    }

    return NULL;
}

void mmc_fwc_start(struct fwc_info *fwc)
{
    struct aicupg_mmc_priv *priv;
    int mmc_id = 0;

    mmc_id = get_current_device_id();

    priv = malloc(sizeof(struct aicupg_mmc_priv));
    if (!priv) {
        pr_err("malloc mmc priv failed.\n");
        goto out;
    }
    memset(priv, 0, sizeof(struct aicupg_mmc_priv));

    priv->host = find_mmc_dev_by_index(mmc_id);
    if (priv->host == NULL) {
        pr_err("find mmc dev failed.\n");
        goto out;
    }

    /* Create partition table and assign to priv->parts */
    priv->parts = mmc_create_gpt_part2(mmc_id);
    if (!priv->parts) {
        pr_err("sdmc %d create gpt part failed.\n", mmc_id);
        goto out;
    }
    fwc->priv = priv;
    fwc->block_size = MMC_BLOCK_SIZE;

    return;
out:
    if (priv->parts)
        mmc_free_partition(priv->parts);

    if (priv)
        free(priv);
}

s32 mmc_fwc_sparse_fill(struct aicupg_mmc_priv *priv, struct aic_partition *parts, u64 chunk_blkcnt, uint32_t fill_val)
{
    u32 blks, remain_blks, redund_blks, erase_group;
    u32 *fill_buf, fill_buf_num_blks, fill_blks = 0;
    int i, j;

    fill_buf = (u32 *)aicupg_malloc_align(ROUNDUP(SPARSE_FILLBUF_SIZE, CACHE_LINE_SIZE), CACHE_LINE_SIZE);
    if (!fill_buf) {
        pr_err("Malloc failed for: CHUNK_TYPE_FILL\n");
        return 0;
    }

    for (i = 0; i < (SPARSE_FILLBUF_SIZE / sizeof(fill_val)); i++)
        fill_buf[i] = fill_val;

    // When using 0 fill, it is faster to use erase than write
    if (chunk_blkcnt >= 0x400 && fill_val == 0x0) { // 512K
        // 1. Fill part start blocks to align by group. 1 group = 1024 blocks
        remain_blks = ROUNDUP((parts->start / MMC_BLOCK_SIZE) + priv->blkstart, 0x400) - ((parts->start / MMC_BLOCK_SIZE) + priv->blkstart);
        blks = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, remain_blks, (u8 *)fill_buf);
        if (blks < remain_blks) { /* blks might be > j (eg. NAND bad-blocks) */
            pr_err("Write failed, block %llu[%d]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, remain_blks);
            goto out;
        }
        priv->blkstart += blks;
        fill_blks += blks;

        // 2. Erase by group for faster speed,
        erase_group = (chunk_blkcnt - remain_blks) / 0x400;
        blks = mmc_berase(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, erase_group * 0x400);
        if (blks != (erase_group * 0x400)) { /* blks might be > j (eg. NAND bad-blocks) */
            pr_err("Erase failed, block %llu[%d]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, erase_group * 0x400);
            goto out;
        }
        priv->blkstart += blks;
        fill_blks += blks;

        // 3. Fill of remaining blocks
        redund_blks = chunk_blkcnt - remain_blks - (erase_group * 0x400);
        blks = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, redund_blks, (u8 *)fill_buf);
        if (blks < redund_blks) { /* blks might be > j (eg. NAND bad-blocks) */
            pr_err("Write failed, block %llu[%d]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, redund_blks);
            goto out;
        }
        priv->blkstart += blks;
        fill_blks += blks;
    } else {
        fill_buf_num_blks = SPARSE_FILLBUF_SIZE / MMC_BLOCK_SIZE;
        for (i = 0; i < chunk_blkcnt;) {
            j = chunk_blkcnt - i;
            if (j > fill_buf_num_blks)
                j = fill_buf_num_blks;

            blks = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, j, (u8 *)fill_buf);
            if (blks < j) { /* blks might be > j (eg. NAND bad-blocks) */
                pr_err("Write failed, block %llu[%d]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, j);
                goto out;
            }
            priv->blkstart += blks;
            fill_blks += blks;
            i += j;
        }
    }

out:
    aicupg_free_align(fill_buf);
    return fill_blks;
}

s32 mmc_fwc_sparse_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;
    struct aic_partition *parts;
    sparse_header_t *sheader;
    chunk_header_t *cheader;
    u8 *wbuf, *p;
    s32 clen = 0, remain, total_len;
    u32 chunk;
    u64 chunk_data_sz, chunk_blkcnt, remain_blkcnt;
    u32 total_blocks = 0, blks;
    u32 fill_val;

    wbuf = malloc(ROUNDUP(len + MMC_BLOCK_SIZE, fwc->block_size));
    if (!wbuf) {
        pr_err("malloc failed.\n");
        return 0;
    }
    p = wbuf;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!priv) {
        pr_err("MMC FWC get priv failed.\n");
        goto out;
    }

    if (priv->remain_len > 0) {
        memcpy(wbuf, priv->remain_data, priv->remain_len);
        memcpy(wbuf + priv->remain_len, buf, len);
    } else {
        memcpy(wbuf, buf, len);
    }

    total_len = (priv->remain_len + len);
    remain = total_len;

    parts = mmc_fwc_get_part_by_name(fwc, fwc->meta.partition);
    if (!parts) {
        pr_err("not find %s part info.\n", fwc->meta.partition);
        goto out;
    }

    sheader = &(priv->sparse_header);
    if (is_sparse_image(wbuf)) {
        priv->is_sparse = 1;
        memcpy(sheader, wbuf, sizeof(sparse_header_t));
        wbuf += sheader->file_hdr_sz;
        clen += sheader->file_hdr_sz;
        remain -= sheader->file_hdr_sz;
        pr_info("=== Sparse Image Header ===\n");
        pr_info("magic: 0x%x\n", sheader->magic);
        pr_info("major_version: 0x%x\n", sheader->major_version);
        pr_info("minor_version: 0x%x\n", sheader->minor_version);
        pr_info("file_hdr_sz: %d\n", sheader->file_hdr_sz);
        pr_info("chunk_hdr_sz: %d\n", sheader->chunk_hdr_sz);
        pr_info("blk_sz: %d\n", sheader->blk_sz);
        pr_info("total_blks: %d\n", sheader->total_blks);
        pr_info("total_chunks: %d\n", sheader->total_chunks);
    }

    pr_debug("Flashing Sparse Image\n");

    /* Start processing chunks */
    for (chunk = priv->cur_chunk; chunk < sheader->total_chunks; chunk++) {
        /* Read and skip over chunk header */
        cheader = (chunk_header_t *)wbuf;

        if (cheader->chunk_type != CHUNK_TYPE_RAW &&
            cheader->chunk_type != CHUNK_TYPE_FILL &&
            cheader->chunk_type != CHUNK_TYPE_DONT_CARE &&
            cheader->chunk_type != CHUNK_TYPE_CRC32 &&
            priv->cur_chunk_remain_data_sz) {
            cheader = &(priv->chunk_header);
            chunk_data_sz = priv->cur_chunk_remain_data_sz;
        } else {
            wbuf += sheader->chunk_hdr_sz;
            clen += sheader->chunk_hdr_sz;
            remain -= sheader->chunk_hdr_sz;
            memcpy(&(priv->chunk_header), cheader, sizeof(chunk_header_t));
            chunk_data_sz = ((u64)sheader->blk_sz) * cheader->chunk_sz;
            priv->cur_chunk_remain_data_sz = chunk_data_sz;
            priv->cur_chunk_burned_data_sz = 0;
            pr_debug("=== Chunk Header ===\n");
            pr_debug("chunk_type: 0x%x\n", cheader->chunk_type);
            pr_debug("chunk_size: 0x%x\n", cheader->chunk_sz);
            pr_debug("total_size: 0x%x\n", cheader->total_sz);
            pr_debug("=== Chunk DEBUG ===\n");
            pr_debug("chunk_id: %u\t", chunk);
            pr_debug("chunk_offset: %u\t", fwc->trans_size + clen);
            pr_debug("chunk_number: %u\n", cheader->total_sz - sheader->chunk_hdr_sz);
        }

        chunk_blkcnt = DIV_ROUND_UP(chunk_data_sz, MMC_BLOCK_SIZE);
        if (priv->blkstart + chunk_blkcnt > (parts->size / MMC_BLOCK_SIZE)) {
            pr_err("Request would exceed partition size!\n");
            goto out;
        }

        remain_blkcnt = remain / MMC_BLOCK_SIZE;
        switch (cheader->chunk_type) {
            case CHUNK_TYPE_RAW:
                if (cheader->total_sz != (sheader->chunk_hdr_sz + chunk_data_sz + priv->cur_chunk_burned_data_sz)) {
                    pr_err("Bogus chunk size for chunk type Raw\n");
                    goto out;
                }

                if (remain_blkcnt > chunk_blkcnt && (remain - chunk_data_sz) >= 16) {
                    blks = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, chunk_blkcnt, wbuf);
                    if (blks < chunk_blkcnt) { /* blks might be > blkcnt (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%u]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, blks);
                        goto out;
                    }
                    remain = remain - chunk_data_sz;
                    priv->cur_chunk_remain_data_sz = 0;
                    priv->cur_chunk_burned_data_sz += chunk_data_sz;
                } else {
                    blks = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, remain_blkcnt, wbuf);
                    if (blks < remain_blkcnt) { /* blks might be > blkcnt (eg. NAND bad-blocks) */
                        pr_err("Write failed, block %llu[%u]\n", (parts->start / MMC_BLOCK_SIZE) + priv->blkstart, blks);
                        goto out;
                    }
                    priv->cur_chunk_remain_data_sz -= remain_blkcnt * MMC_BLOCK_SIZE;
                    priv->cur_chunk_burned_data_sz += remain_blkcnt * MMC_BLOCK_SIZE;
                    remain = remain % MMC_BLOCK_SIZE;
                }

                priv->blkstart += blks;
                total_blocks += blks;
                wbuf += blks * MMC_BLOCK_SIZE;
                clen += blks * MMC_BLOCK_SIZE;

                break;

            case CHUNK_TYPE_FILL:
                if (cheader->total_sz != (sheader->chunk_hdr_sz + sizeof(uint32_t))) {
                    pr_err("Bogus chunk size for chunk type FILL\n");
                    goto out;
                }

                fill_val = *(uint32_t *)wbuf;
                wbuf = (u8 *)wbuf + sizeof(u32);
                clen += sizeof(uint32_t);
                remain -= sizeof(uint32_t);

                pr_debug("FILL with \t 0x%08x\n", fill_val);

                blks = mmc_fwc_sparse_fill(priv, parts, chunk_blkcnt, fill_val);
                if (blks != chunk_blkcnt) {
                    pr_err("CHUNK_TYPE_FILL FILL failed.\n");
                    goto out;
                }

                priv->cur_chunk_remain_data_sz -= blks * MMC_BLOCK_SIZE;
                total_blocks += DIV_ROUND_UP(chunk_data_sz, sheader->blk_sz);

                break;

            case CHUNK_TYPE_DONT_CARE:
                priv->blkstart += chunk_blkcnt;
                total_blocks += cheader->chunk_sz;
                break;

            case CHUNK_TYPE_CRC32:
                if (cheader->total_sz != sheader->chunk_hdr_sz) {
                    pr_err("Bogus chunk size for chunk type Dont Care\n");
                    goto out;
                }
                total_blocks += cheader->chunk_sz;
                wbuf += chunk_data_sz;
                clen += chunk_data_sz;
                remain -= chunk_data_sz;
                break;

            default:
                printf("Unknown chunk type: %x\n", cheader->chunk_type);
                cheader = &(priv->chunk_header);
        }

        if ((priv->cur_chunk_remain_data_sz > 0 && (remain > 0 && remain < MMC_BLOCK_SIZE)) || remain < sizeof(chunk_header_t))
            break;
    }

    priv->remain_len = remain;
    priv->cur_chunk = chunk;
    if (priv->remain_len) {
        memcpy(priv->remain_data, wbuf, priv->remain_len);
    }

    fwc->calc_partition_crc = fwc->meta.crc;
    fwc->trans_size += clen;

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

out:
    free(p);
    return len;
}

s32 mmc_fwc_raw_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;
    struct aic_partition *parts = NULL;
    u64 blkstart, blkcnt;
    u8 __attribute__((unused)) *rdbuf = NULL;
    long n;
    u32 clen = 0, calc_len = 0;

    if ((fwc->meta.size - fwc->trans_size) < len)
        calc_len = fwc->meta.size - fwc->trans_size;
    else
        calc_len = len;

    fwc->calc_partition_crc = crc32(fwc->calc_partition_crc, buf, calc_len);

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    rdbuf = aicupg_malloc_align(len, CACHE_LINE_SIZE);
    if (!rdbuf) {
        pr_err("Error: malloc buffer failed.\n");
        return 0;
    }
#endif

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!priv) {
        pr_err("MMC FWC get priv failed.\n");
        goto out;
    }

    parts = mmc_fwc_get_part_by_name(fwc, fwc->meta.partition);
    if (!parts)
        pr_err("not find %s part info.\n", fwc->meta.partition);

    blkstart = fwc->trans_size / MMC_BLOCK_SIZE;
    blkcnt = len / MMC_BLOCK_SIZE;
    if (len % MMC_BLOCK_SIZE)
        blkcnt++;

    if ((blkstart + blkcnt) > (parts->size / MMC_BLOCK_SIZE)) {
        pr_err("Data size exceed the partition size.\n");
        goto out;
    }

    clen = len;
    n = mmc_bwrite(priv->host, (parts->start / MMC_BLOCK_SIZE) + blkstart, blkcnt, buf);
    if (n != blkcnt) {
        pr_err("Error, write to partition %s failed.\n", fwc->meta.partition);
        clen = n * MMC_BLOCK_SIZE;
        fwc->trans_size += clen;
    }

#ifdef AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
    // Read data to calc crc
    n = mmc_bread(priv->host, (parts->start / MMC_BLOCK_SIZE) + blkstart, blkcnt, rdbuf);
    if (n != blkcnt) {
        pr_err("Error, read from  partition %s failed.\n", fwc->meta.partition);
        clen = n * MMC_BLOCK_SIZE;
        fwc->trans_size += clen;
    }

    if (crc32(0, buf, calc_len) != crc32(0, rdbuf, calc_len)) {
        pr_err("calc_len:%d\n", calc_len);
        pr_err("crc err at trans len %u\n", fwc->trans_size);
    }
#endif

    fwc->trans_size += clen;

    pr_debug("%s, data len %d, trans len %d\n", __func__, len, fwc->trans_size);

out:
    if (rdbuf)
        aicupg_free_align(rdbuf);
    return clen;
}

s32 mmc_fwc_data_write(struct fwc_info *fwc, u8 *buf, s32 len)
{
    struct aicupg_mmc_priv *priv;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (!is_sparse_image(buf) && !priv->is_sparse) {
        pr_debug("Not a sparse image\n");
        return mmc_fwc_raw_write(fwc, buf, len);
    } else {
        pr_debug("A sparse image\n");
        return mmc_fwc_sparse_write(fwc, buf, len);
    }
}

s32 mmc_fwc_data_read(struct fwc_info *fwc, u8 *buf, s32 len)
{
    return 0;
}

void mmc_fwc_data_end(struct fwc_info *fwc)
{
    struct aicupg_mmc_priv *priv;

    priv = (struct aicupg_mmc_priv *)fwc->priv;
    if (priv->parts)
        mmc_free_partition(priv->parts);

    if (fwc->priv) {
        free(priv);
        fwc->priv = 0;
    }
}
