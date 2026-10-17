/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <console.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <aic_log.h>
#include <mmc.h>

static int do_mmc_dump(int argc, char **argv)
{
	unsigned long id, blkcnt, blkoffset;
    struct aic_sdmc *host;
	char *pe;
	u8 *p;
	int ret;

    if (argc != 4)
        return -1;

    id = strtoul(argv[1], &pe, 0);
    blkoffset = strtoul(argv[2], &pe, 0);
    blkcnt = strtoul(argv[3], &pe, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    p = malloc(512);

    for (int i = 0; i < blkcnt; i++) {
        mmc_bread(host, blkoffset, blkcnt, (void *)p);
        hexdump(p, blkcnt * 512, 1);
    }
    free(p);
    return 0;
}

static int do_mmc_read(int argc, char **argv)
{
	unsigned long id, addr, blkcnt, blkoffset;
    struct aic_sdmc *host;
	char *pe;
    void *p;
    int ret;

	if (argc < 5)
		return -1;

	id = strtoul(argv[1], &pe, 0);
	blkoffset = strtoul(argv[2], &pe, 0);
	blkcnt = strtoul(argv[3], &pe, 0);
	addr = strtoul(argv[4], &pe, 0);

    p = (void *)addr;
    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    mmc_bread(host, blkoffset, blkcnt, (void *)p);

    return 0;
}

static int do_mmc_erase(int argc, char **argv)
{
	unsigned long id, blkcnt, blkoffset;
    struct aic_sdmc *host;
	char *pe;
    int ret;

	if (argc < 4)
		return -1;

	id = strtoul(argv[1], &pe, 0);
	blkoffset = strtoul(argv[2], &pe, 0);
	blkcnt = strtoul(argv[3], &pe, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    mmc_berase(host, blkoffset, blkcnt);

    return 0;
}

static int do_mmc_write(int argc, char **argv)
{
	unsigned long id, addr, blkcnt, blkoffset;
    struct aic_sdmc *host;
	char *pe;
    void *p;
    int ret;

	if (argc < 5)
		return -1;

	id = strtoul(argv[1], &pe, 0);
	blkoffset = strtoul(argv[2], &pe, 0);
	blkcnt = strtoul(argv[3], &pe, 0);
	addr = strtoul(argv[4], &pe, 0);

    p = (void *)addr;
    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    mmc_bwrite(host, blkoffset, blkcnt, (void *)p);

    return 0;
}

static int do_mmc_scan(int argc, char **argv)
{
    unsigned long id, blkoffset;
    struct aic_sdmc *host;
    char *pe;
    int ret;
    u8 w_buf[512] = {0}, r_buf[512] = {0};
    u8 scan_res[4][32] = {0};
    int i, j, k;

    if (argc < 3)
        return -1;

    id = strtoul(argv[1], &pe, 0);
    blkoffset = strtoul(argv[2], &pe, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host== NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    /* set the write buf and write to the block */
    srand(aic_get_ticks());

    for (k = 0; k < 512; k++)
        w_buf[k] = rand() & 0xff;

    ret = mmc_bwrite(host, blkoffset, 1, w_buf);
    if (ret == 0) {
        printf("mmc_bwrite error!\n");
        return -1;
    }
    memset(scan_res, 1, sizeof(scan_res));

    /* scan the sample phase and chain */
    for (i = 0; i < 4; i++) {
        mmc_set_rx_phase(host, i);
        for (j = 0; j < 32; j++) {
            mmc_set_rx_delay(host, j);

            memset(r_buf, 0, sizeof(r_buf));

            ret = mmc_bread(host, blkoffset, 1, r_buf);
            if (ret == 0) {
                printf("mmc_bread error!\n");
                scan_res[i][j] = 0;
                continue;
            }
            for (k = 0; k < 512; k++) {
                if (w_buf[k] != r_buf[k]) {
                    printf("r_buf[%d]:expect 0x%02x, actual:0x%02x\n", k, w_buf[k], r_buf[k]);
                    scan_res[i][j] = 0;
                    break;
                }
            }
        }
    }

    /* print the result */
    printf("sample phase result:\n");
    printf("     ");
    for (i = 0; i < 32; i++) {
        printf("%-3d", i);
    }
    printf("\n    ----------------------------------------------------------------"
        "--------------------------------\n");
    for (i = 0; i < 4; i++) {
        printf("%-2d | ", i);
        for (j = 0; j < 32; j++) {
            printf("%02x ", scan_res[i][j]);
        }
        printf("\n");
    }

    for (k = 0; k < 512; k++)
        w_buf[k] = rand() & 0xff;

    memset(scan_res, 1, sizeof(scan_res));

    /* scan the driver phase and chain */
    for (i = 0; i < 4; i++) {
        mmc_set_tx_phase(host, i);
        for (j = 0; j < 32; j++) {
            mmc_set_tx_delay(host, j);

            ret = mmc_bwrite(host, blkoffset, 1, w_buf);
            if (ret == 0) {
                printf("mmc_bwrite error!\n");
                scan_res[i][j] = 0;
                continue;
            }

            memset(r_buf, 0, sizeof(r_buf));

            ret = mmc_bread(host, blkoffset, 1, r_buf);
            if (ret == 0) {
                scan_res[i][j] = 0;
                continue;
            }
            for (k = 0; k < 512; k++) {
                if (w_buf[k] != r_buf[k]) {
                    printf("r_buf[%d]:expect 0x%02x, actual:0x%02x\n", k, w_buf[k], r_buf[k]);
                    scan_res[i][j] = 0;
                    break;
                }
            }
        }
    }

    /* print the result */
    printf("driver phase result:\n");
    printf("     ");
    for (i = 0; i < 32; i++) {
        printf("%-3d", i);
    }
    printf("\n    ----------------------------------------------------------------"
        "--------------------------------\n");
    for (i = 0; i < 4; i++) {
        printf("%-2d | ", i);
        for (j = 0; j < 32; j++) {
            printf("%02x ", scan_res[i][j]);
        }
        printf("\n");
    }

    return 0;
}

static int confirm_key_prog(void)
{
    puts("Warning: Programming authentication key can be done only once !\n"
         "         Use this command only if you are sure of what you are doing,\n"
         "Really perform the key programming? <y/N> ");
    if (console_confirm_yesno())
        return -1;

    puts("Authentication key programming aborted\n");
    return 0;
}

static int do_mmc_rpmb_key(int argc, char *argv[])
{
    unsigned long id;
    void *key_addr;
    struct aic_sdmc *host = NULL;
    int ret;
    u32 original_part;

    if (argc < 3)
        return -1;

    id = strtoul(argv[1], NULL, 0);
    key_addr = (void *)strtoul(argv[2], NULL, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host == NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    if (!confirm_key_prog())
        return -1;

    original_part = host->dev->part_num;
    mmc_switch_part(host, MMC_PART_RPMB);
    ret = mmc_rpmb_set_key(host, key_addr);
    mmc_switch_part(host, original_part);
    if (ret) {
        printf("ERROR - Key already programmed ?\n");
        return -1;
    }

    return 0;
}

static int do_mmc_rpmb_dump(int argc, char *argv[])
{
    unsigned long id, blkcnt, blkoffset;
    struct aic_sdmc *host = NULL;
    void *key_addr = NULL;
    u8 *data;
    int n, ret;
    u32 original_part;

    if (argc < 4)
        return -1;

    id = strtoul(argv[1], NULL, 0);
    blkoffset = strtoul(argv[2], NULL, 0);
    blkcnt = strtoul(argv[3], NULL, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host == NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    if (argc == 5)
        key_addr = (void *)strtoul(argv[4], NULL, 0);

    data = aicos_malloc_align(0, 512 * blkcnt, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, 512 * blkcnt);

    printf("\nMMC RPMB dump: dev # %ld, block # %ld, count %ld ... ", id, blkoffset, blkcnt);
    original_part = host->dev->part_num;
    mmc_switch_part(host, MMC_PART_RPMB);
    n = mmc_rpmb_read(host, data, blkoffset, blkcnt, key_addr);
    mmc_switch_part(host, original_part);

    printf("%d RPMB blocks dump: %s\n", n, (n == blkcnt) ? "OK" : "ERROR");
    if (n != blkcnt) {
        aicos_free_align(0, data);
        return -1;
    }

    hexdump((void *)data, 512 * blkcnt, 1);
    hexdump((void *)host->cid, 16, 1);

    aicos_free_align(0, data);
    return 0;
}

static int do_mmc_rpmb_read(int argc, char *argv[])
{
    unsigned long id, blkcnt, blkoffset;
    struct aic_sdmc *host = NULL;
    void *addr = NULL, *key_addr = NULL;
    int n, ret;
    u32 original_part;

    if (argc < 5)
        return -1;

    id = strtoul(argv[1], NULL, 0);
    blkoffset = strtoul(argv[2], NULL, 0);
    blkcnt = strtoul(argv[3], NULL, 0);
    addr = (void *)strtoul(argv[4], NULL, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host == NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    if (argc == 6)
        key_addr = (void *)strtoul(argv[5], NULL, 0);

    printf("\nMMC RPMB read: dev # %ld, block # %ld, count %ld ... ", id, blkoffset, blkcnt);
    original_part = host->dev->part_num;
    mmc_switch_part(host, MMC_PART_RPMB);
    n = mmc_rpmb_read(host, addr, blkoffset, blkcnt, key_addr);
    mmc_switch_part(host, original_part);

    printf("%d RPMB blocks read: %s\n", n, (n == blkcnt) ? "OK" : "ERROR");
    if (n != blkcnt)
        return -1;
    return 0;
}

static int do_mmc_rpmb_write(int argc, char *argv[])
{
    unsigned long id, blkcnt, blkoffset;
    struct aic_sdmc *host = NULL;
    void *addr = NULL, *key_addr = NULL;
    int n, ret;
    u32 original_part;

    if (argc < 6)
        return -1;

    id = strtoul(argv[1], NULL, 0);
    blkoffset = strtoul(argv[2], NULL, 0);
    blkcnt = strtoul(argv[3], NULL, 0);
    addr = (void *)strtoul(argv[4], NULL, 0);
    key_addr = (void *)strtoul(argv[5], NULL, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host == NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    printf("\nMMC RPMB write: dev # %ld, block # %ld, count %ld ... ", id, blkoffset, blkcnt);
    original_part = host->dev->part_num;
    mmc_switch_part(host, MMC_PART_RPMB);
    n = mmc_rpmb_write(host, addr, blkoffset, blkcnt, key_addr);
    mmc_switch_part(host, original_part);

    printf("%d RPMB blocks written: %s\n", n, (n == blkcnt) ? "OK" : "ERROR");
    if (n != blkcnt)
        return -1;

    return 0;
}

static int do_mmc_rpmb_counter(int argc, char *argv[])
{
    unsigned long id, counter;
    struct aic_sdmc *host = NULL;
    int ret;
    u32 original_part;

    if (argc < 2)
        return -1;

    id = strtoul(argv[1], NULL, 0);

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %ld init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (host == NULL) {
        printf("can't find mmc device!");
        return -1;
    }

    original_part = host->dev->part_num;
    mmc_switch_part(host, MMC_PART_RPMB);
    ret = mmc_rpmb_get_counter(host, &counter);
    mmc_switch_part(host, original_part);
    if (ret)
        return -1;

    printf("RPMB Write counter= %lx\n", counter);

    return 0;
}

static void do_mmc_help(void)
{
    printf("ArtInChip MMC read/write command\n\n");
    printf("mmc help                                     : Show this help\n");
    printf("mmc dump  <id> <blkoffset> <blkcnt>          : Dump data in mmc <id>\n");
    printf("mmc erase <id> <blkoffset> <blkcnt>          : Erase data in mmc <id>\n");
    printf("mmc read  <id> <blkoffset> <blkcnt> <addr>   : Read data in mmc <id> to RAM address\n");
    printf("mmc write <id> <blkoffset> <blkcnt> <addr>   : Write data to mmc <id> from RAM address\n");
    printf("mmc scan  <id> <blkoffset>                   : scan the mmc phase and delay chain \n");
    printf("mmc rpmb dump    <id> <blkoffset> <blkcnt> [addr of auth-key]\n");
    printf("mmc rpmb read    <id> <blkoffset> <blkcnt> <addr> [addr of auth-key]\n");
    printf("mmc rpmb write   <id> <blkoffset> <blkcnt> <addr> <addr of auth-key>\n");
    printf("mmc rpmb key     <id> <addr of auth-key>\n");
    printf("mmc rpmb counter <id>\n");
    printf("Example:\n");
    printf("    mmc dump 0 0 1\n");
}

static int do_mmc_rpmb(int argc, char *argv[])
{
    if (argc < 3)
        goto help;

    if (!strcmp(argv[1], "dump")) {
        do_mmc_rpmb_dump(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "read")) {
        do_mmc_rpmb_read(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "write")) {
        do_mmc_rpmb_write(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "key")) {
        do_mmc_rpmb_key(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "counter")) {
        do_mmc_rpmb_counter(argc - 1, &argv[1]);
        return 0;
    }

help:
    do_mmc_help();

    return 0;
}

static int cmd_mmc_do(int argc, char **argv)
{
    if (argc <= 1)
        goto help;

    if (!strcmp(argv[1], "dump")) {
        do_mmc_dump(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "read")) {
        do_mmc_read(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "write")) {
        do_mmc_write(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "erase")) {
        do_mmc_erase(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "rpmb")) {
        do_mmc_rpmb(argc - 1, &argv[1]);
        return 0;
    } else if (!strcmp(argv[1], "scan")) {
        do_mmc_scan(argc - 1, &argv[1]);
        return 0;
    }

help:
    do_mmc_help();

    return 0;
}

CONSOLE_CMD(mmc, cmd_mmc_do, "ArtInChip MMC command");
