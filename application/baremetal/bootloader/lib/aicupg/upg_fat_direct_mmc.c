/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <rtconfig.h>
#if defined(AICUPG_MMC_ARTINCHIP)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <upg_internal.h>
#include <aic_core.h>
#include <aicupg.h>
#include <aic_utils.h>
#include <sparse_format.h>
#include <upg_fat_direct.h>
#include <dfs_file.h>

static int fat_direct_mmc_write_raw(struct aic_sdmc *host, char *fpath,
                                    u32 doffset)
{
    struct dfs_fd fd;
    u8 *buf = NULL;
    int ret = 0, endflag = 0;
    u32 soffset, start, count;
    ulong dolen, actread, writecnt;

    printf("Programming %s to 0x%x\n", fpath, doffset);
    printf("\t");

    if (dfs_file_open(&fd, fpath, O_RDONLY) < 0) {
        printf("Open %s failed.\n", fpath);
        return -1;
    }

    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    soffset = 0;
    writecnt = 0;

    while (1) {
        actread = 0;
        dolen = MAX_WRITE_SIZE;
        if (dfs_file_lseek(&fd, soffset) != soffset) {
            printf("Error:seek file %s failed!\n", fpath);
            ret = 0;
            break;
        }

        actread = dfs_file_read(&fd, buf, dolen);
        if (actread <= 0) {
            pr_err("Error:read file %s failed!\n", fpath);
            ret = 0;
            break;
        }
        pr_debug("Going to read %ld from offset 0x%x, got 0x%ld\n", dolen,
                 soffset, actread);
        if (actread != dolen)
            endflag = 1;
        soffset += actread;

        writecnt += actread;
        printf("\r\t wrote count 0x%lx", writecnt);
        start = doffset / MMC_BLOCK_SIZE;
        count = actread / MMC_BLOCK_SIZE;
        if (actread % MMC_BLOCK_SIZE)
            count++;
        if (mmc_bwrite(host, start, count, buf) != count) {
            pr_err("Write failed, block %d\n", start);
            ret = -1;
            break;
        }
        doffset += actread;
        if (endflag)
            break;
    }

    if (buf)
        aicupg_free_align(buf);
    dfs_file_close(&fd);
    printf("\n");
    return ret;
}

static int fat_direct_mmc_write_sparse(struct aic_sdmc *host, char *fpath,
                                       u32 doffset)
{
    struct dfs_fd fd;
    u32 soffset, i, chunk, fill_val, chunk_blkcnt, doblk, blk_start, blks;
    ulong dolen, actread;
    u8 *buf = NULL, *p;
	u64 chunk_data_sz;
	u32 *fill_buf = NULL;
    int ret = 0;

    printf("Programming %s to 0x%x\n", fpath, doffset);
    printf("\t");

    if (dfs_file_open(&fd, fpath, O_RDONLY) < 0) {
        printf("Open %s failed.\n", fpath);
        ret = -1;
        goto out;
    }

    buf = aicupg_malloc_align(MAX_WRITE_SIZE, FRAME_LIST_SIZE);
    soffset = 0;

    dolen = MAX_WRITE_SIZE;
    if (dfs_file_lseek(&fd, soffset) != soffset) {
        printf("Error:seek file %s failed!\n", fpath);
        ret = -1;
        goto out;
    }

    actread = dfs_file_read(&fd, buf, dolen);
    if (actread <= 0) {
        pr_err("Error:read file %s failed!\n", fpath);
        ret = -1;
        goto out;
    }

    sparse_header_t sheader;
    chunk_header_t *cheader;

    if (!is_sparse_image(buf)) {
        printf("Not sparse image.\n");
        ret = -1;
        goto out;
    }
    memcpy(&sheader, buf, sizeof(sparse_header_t));
    pr_info("=== Sparse Image Header ===\n");
    pr_info("magic: 0x%x\n", sheader.magic);
    pr_info("major_version: 0x%x\n", sheader.major_version);
    pr_info("minor_version: 0x%x\n", sheader.minor_version);
    pr_info("file_hdr_sz: %d\n", sheader.file_hdr_sz);
    pr_info("chunk_hdr_sz: %d\n", sheader.chunk_hdr_sz);
    pr_info("blk_sz: %d\n", sheader.blk_sz);
    pr_info("total_blks: %d\n", sheader.total_blks);
    pr_info("total_chunks: %d\n", sheader.total_chunks);

    soffset += sheader.file_hdr_sz;
    actread = 0;
    for (chunk = 0; chunk < sheader.total_chunks; chunk++) {

        printf("\r\t wrote chunk %d", chunk);
        p = buf;

        /* There are 4 bytes data for FILL/CRC32, read 4bytes more here first */
        dolen = sheader.chunk_hdr_sz + 4;
        if (dfs_file_lseek(&fd, soffset) != soffset) {
            printf("Error:seek file %s failed!\n", fpath);
            ret = -1;
            goto out;
        }

        actread = dfs_file_read(&fd, p, dolen);
        if (actread <= 0) {
            pr_err("Error:read file %s failed!\n", fpath);
            ret = -1;
            goto out;
        }

        /* Read and skip over chunk header */
        cheader = (chunk_header_t *)p;

        if (cheader->chunk_type != CHUNK_TYPE_RAW) {
            pr_debug("=== Chunk Header ===\n");
            pr_debug("chunk_type: 0x%x\n", cheader->chunk_type);
            pr_debug("chunk_data_sz: 0x%x\n", cheader->chunk_sz);
            pr_debug("total_size: 0x%x\n", cheader->total_sz);
        }

        soffset += sheader.chunk_hdr_sz;
        p += sheader.chunk_hdr_sz;

        /* Data size in dst storage */
        chunk_data_sz = ((u64)sheader.blk_sz) * cheader->chunk_sz;
        chunk_blkcnt = DIV_ROUND_UP(chunk_data_sz, MMC_BLOCK_SIZE);

        switch (cheader->chunk_type) {
            case CHUNK_TYPE_RAW:
                if (cheader->total_sz != (sheader.chunk_hdr_sz + chunk_data_sz)) {
                    pr_err("Bogus chunk size for chunk type Raw\n");
                    ret = -1;
                    goto out;
                }

                /* Read raw chunk data */
                if (chunk_data_sz > MAX_WRITE_SIZE) {
                    aicupg_free_align(buf);
                    buf = aicupg_malloc_align(chunk_data_sz, FRAME_LIST_SIZE);
                    p = buf;
                }

                dolen = chunk_data_sz;
                if (dfs_file_lseek(&fd, soffset) != soffset) {
                    printf("Error:seek file %s failed!\n", fpath);
                    ret = -1;
                    goto out;
                }

                actread = dfs_file_read(&fd, p, dolen);
                if (actread <= 0) {
                    pr_err("Error:read file %s failed!\n", fpath);
                    ret = -1;
                    goto out;
                }
                soffset += actread;

                /* Write to dst storage */
                blk_start = doffset / MMC_BLOCK_SIZE;
                blks = mmc_bwrite(host, blk_start, chunk_blkcnt, p);
                if (blks < chunk_blkcnt) {
                    pr_err("Write failed, block %u[%u]\n", blk_start, blks);
                    ret = -1;
                    goto out;
                }
                doffset += blks * MMC_BLOCK_SIZE;
                break;

            case CHUNK_TYPE_FILL:
                if (cheader->total_sz != (sheader.chunk_hdr_sz + sizeof(u32))) {
                    pr_err("Bogus chunk size for chunk type FILL\n");
                    ret = -1;
                    goto out;
                }

                fill_val = *(u32 *)p;
                /* Step forward in sparse image */
                soffset += 4;

                /* Reuse work buffer */
                fill_buf = (void *)buf;

                for (i = 0; i < (MAX_WRITE_SIZE / sizeof(fill_val)); i++)
                    fill_buf[i] = fill_val;

                while (chunk_blkcnt) {
                    doblk = chunk_blkcnt;
                    if (doblk > MAX_WRITE_SIZE / MMC_BLOCK_SIZE)
                        doblk = MAX_WRITE_SIZE / MMC_BLOCK_SIZE;

                    blk_start = doffset / MMC_BLOCK_SIZE;
                    blks = mmc_bwrite(host, blk_start, doblk, (void *)fill_buf);
                    doffset += blks * MMC_BLOCK_SIZE;
                    chunk_blkcnt -= doblk;
                }

                break;

            case CHUNK_TYPE_DONT_CARE:
                doffset += chunk_blkcnt * MMC_BLOCK_SIZE;
                break;

            case CHUNK_TYPE_CRC32:
                if (cheader->total_sz != sheader.chunk_hdr_sz) {
                    pr_err("Bogus chunk size for chunk type Dont Care\n");
                    ret = -1;
                    goto out;
                }
                /* Step forward in sparse image */
                soffset += 4;
                break;

            default:
                printf("Unknown chunk type: %x\n", cheader->chunk_type);
        }
    }

out:
    if (buf)
        aicupg_free_align(buf);
    dfs_file_close(&fd);
    printf("\n");
    return ret;
}

int fat_direct_write_mmc(u32 sdmc_id, char *fpath, u32 doffset, char *attr)
{
    static struct aic_sdmc *sdmc[MAX_INTERFACE_NUM] = { 0 };
    struct aic_sdmc *host;
    int ret = 0;

    if (sdmc_id >= MAX_INTERFACE_NUM) {
        pr_err("%s, invalid parameter, sdmc id = %d\n", __func__, sdmc_id);
        return -1;
    }
    if (sdmc[sdmc_id] == NULL) {
        mmc_init(sdmc_id);
        sdmc[sdmc_id] = find_mmc_dev_by_index(sdmc_id);
        if (sdmc[sdmc_id] == NULL) {
            pr_err("find mmc dev failed.\n");
            return -1;
        }
    }
    host = sdmc[sdmc_id];

    if (attr && (strcmp(attr, "sparse") == 0)) {
        ret = fat_direct_mmc_write_sparse(host, fpath, doffset);
    } else {
        ret = fat_direct_mmc_write_raw(host, fpath, doffset);
    }
    return ret;
}
#endif
