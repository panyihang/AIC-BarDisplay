/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <aic_utils.h>
#if defined(AICUPG_NOR_ARTINCHIP)
#include <sfud.h>
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
#include <spinand_port.h>
#endif
#if defined(AICUPG_MMC_ARTINCHIP)
#include <mmc.h>
#endif

#define AIC_IMAGE_MAGIC 0x20434941
#define AIC_PAGE_TABLE  0x50434941
#define AIC_SPL_SIZE    (128 * 1024)
#define AICUPG_ERASE_HELP                                               \
    "ArtInChip data erase command for upgdrading mode:\n"               \
    "erase [boot]\n"                                                    \
    "    Erase boot program in flash/mmc\n"                             \
    "erase <offset> <length>\n"                                         \
    "    Erase data in flash/mmc\n"

#define OP_STORAGE_SPINOR  1
#define OP_STORAGE_SPINAND 2
#define OP_STORAGE_MMC     3

__USED static void aicupg_erase_help(void)
{
    puts(AICUPG_ERASE_HELP);
}

#if defined(AICUPG_NOR_ARTINCHIP)
extern sfud_flash *sfud_probe(u32 spi_bus);
static int spinor_erase_boot(sfud_flash *flash)
{
    u8 data[256];
    u32 head;
    int i;

    /* Two image backup */
    for (i = 0; i < 2; i++) {
        if (sfud_read(flash, i * AIC_SPL_SIZE, 256, data)) {
            printf("Failed to read data from spinor.\n");
            return -1;
        }
        memcpy(&head, data, 4);
        if (head == AIC_IMAGE_MAGIC) {
            if (sfud_erase(flash, i * AIC_SPL_SIZE, AIC_SPL_SIZE)) {
                printf("Failed to erase spinor.\n");
                return -1;
            }
        }
    }
    return 0;
}
#endif

static int do_spinor_erase(int argc, char *argv[])
{
    int err = -1;
#if defined(AICUPG_NOR_ARTINCHIP)
    sfud_flash *flash;
    unsigned long offset, length;

    flash = sfud_probe(0);
    if (!flash) {
        printf("Failed to init spinor\n");
        return -1;
    }

    if ((argc == 2) && (!strcmp(argv[1], "boot")))
        return spinor_erase_boot(flash);

    /* erase <offset> <length> */
    if (argc != 3) {
        aicupg_erase_help();
        return -1;
    }

    offset = strtol(argv[1], NULL, 0);
    length = strtol(argv[2], NULL, 0);

    if (offset % 0x1000) {
        printf("The start offset is not alignment with 4KB.\n");
        return -1;
    }
    if (length % 0x1000) {
        printf("The length is not alignment with 4KB.\n");
        return -1;
    }

    err = sfud_erase(flash, offset, length);
    if (err) {
        printf("Failed to erase spinor.\n");
        return -1;
    }

#endif
    return err;
}

#if defined(AICUPG_NAND_ARTINCHIP)
#define SPL_CANDIDATE_BLOCK_NUM 18
extern int nand_spl_get_candidate_blocks(u32 *blks, u32 size);
static int spinand_erase_boot(struct aic_spinand *flash)
{
    u32 spl_blocks[SPL_CANDIDATE_BLOCK_NUM];
    u32 offset, blk_size, head;
    int ret, i;
    u8 *data;

    data = aicos_malloc_align(0, 4096, CACHE_LINE_SIZE);
    if (!data) {
        printf("Failed to malloc buffer.\n");
        return -1;
    }
    nand_spl_get_candidate_blocks(spl_blocks, SPL_CANDIDATE_BLOCK_NUM);

    blk_size = flash->info->page_size * flash->info->pages_per_eraseblock;
    for (i = 0; i < SPL_CANDIDATE_BLOCK_NUM; i++) {
        offset = spl_blocks[i] * blk_size;
        ret = spinand_read(flash, data, offset, flash->info->page_size);
        if (ret) {
            printf("Failed to read data from spinand.\n");
            return -1;
        }
        memcpy(&head, data, 4);
        if (head == AIC_PAGE_TABLE) {
            ret = spinand_erase(flash, offset, blk_size);
            if (ret) {
                printf("Failed to erase spinand.\n");
                return -1;
            }
        }
    }

    aicos_free_align(0, data);
    return ret;
}
#endif

static int do_spinand_erase(int argc, char *argv[])
{
    int ret = -1;
#if defined(AICUPG_NAND_ARTINCHIP)
    unsigned long offset, length;
    struct aic_spinand *flash;
    u32 blk_size;

    flash = spinand_probe(0);
    if (!flash) {
        printf("Failed to init spinand\n");
        return -1;
    }

    if ((argc == 2) && (!strcmp(argv[1], "boot")))
        return spinand_erase_boot(flash);

    /* erase <offset> <length> */
    if (argc != 3) {
        aicupg_erase_help();
        return -1;
    }

    offset = strtol(argv[1], NULL, 0);
    length = strtol(argv[2], NULL, 0);

    blk_size = flash->info->page_size * flash->info->pages_per_eraseblock;
    if (offset % blk_size) {
        printf("The start offset is not alignment with block size.\n");
        return -1;
    }
    if (length % blk_size) {
        printf("The length is not alignment with block size.\n");
        return -1;
    }

    ret = spinand_erase(flash, offset, length);
    if (ret) {
        printf("Failed to erase spinand.\n");
        return -1;
    }
#endif
    return ret;
}

#if defined(AICUPG_MMC_ARTINCHIP)
static int mmc_erase_boot(struct aic_sdmc *host)
{
    u32 blkoffset, blkcnt, head;
    u8 *data = NULL;

    data = aicos_malloc_align(0, 512, CACHE_LINE_SIZE);
    if (!data) {
        printf("Failed to malloc buffer.\n");
        return -1;
    }
    memset(data, 0, 512);
    blkcnt = (AIC_SPL_SIZE / 512);
    for (int i = 0; i < 2; i++) {
        blkoffset = 34 + i * blkcnt;
        mmc_bread(host, blkoffset, 1, (void *)data);
        memcpy(&head, data, 4);
        if (head == AIC_IMAGE_MAGIC) {
            memset(data, 0, 512);
            mmc_bwrite(host, blkoffset, 1, (void *)data);
        }
    }

    aicos_free_align(0, data);
    return 0;
}
#endif

static int do_mmc_erase(int argc, char *argv[])
{
    int ret = -1;
#if defined(AICUPG_MMC_ARTINCHIP)
    unsigned long offset, length, grp_size;
    u32 blkoffset, blkcnt;
    struct aic_sdmc *host;

    ret = mmc_init(0);
    if (ret) {
        printf("sdmc init failed.\n");
        return ret;
    }
    host = find_mmc_dev_by_index(0);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    if ((argc == 2) && (!strcmp(argv[1], "boot")))
        return mmc_erase_boot(host);

    /* erase <offset> <length> */
    if (argc != 3) {
        aicupg_erase_help();
        return -1;
    }

    offset = strtol(argv[1], NULL, 0);
    length = strtol(argv[2], NULL, 0);
    grp_size = host->dev->erase_grp_size * 512;
    if (offset % grp_size) {
        printf("The start offset is not alignment with group size %lu.\n", grp_size);
        return -1;
    }
    if (length % grp_size) {
        printf("The length is not alignment with group size: %lu.\n", grp_size);
        return -1;
    }
    blkoffset = offset / 512;
    blkcnt = length / 512;

    mmc_berase(host, blkoffset, blkcnt);
    ret = 0;
#endif
    return ret;
}

static int do_aicupg_erase(int argc, char *argv[])
{
    int ret = -1, target;

    target = 0;
#if defined(AICUPG_MMC_ARTINCHIP)
    /* When Flash and eMMC/SD are all enabled, the MMC priority is low */
    target = OP_STORAGE_MMC;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
    target = OP_STORAGE_SPINOR;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
    target = OP_STORAGE_SPINAND;
#endif

    if (target == OP_STORAGE_SPINOR)
        ret = do_spinor_erase(argc, argv);
    if (target == OP_STORAGE_SPINAND)
        ret = do_spinand_erase(argc, argv);
    if (target == OP_STORAGE_MMC)
        ret = do_mmc_erase(argc, argv);

    return ret;
}

CONSOLE_CMD(erase, do_aicupg_erase, "Erase command in upgrading mode.");
