/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_time.h>
#include <aic_errno.h>
#include <aic_utils.h>
#include <mtd.h>
#include <spienc.h>
#include <rtconfig.h>

#ifdef AIC_SPINAND_DRV
#define MTD_HELP                                     \
    "mtd read write command:\n"                      \
    "  mtd list\n"                                   \
    "  mtd bypass <status>\n"                        \
    "      status = 1: bypass spienc\n"              \
    "      status = 0: enable spienc\n"              \
    "  mtd dump  <part> <offset> <size>\n"           \
    "  mtd alloc  <size>\n"                          \
    "  mtd free\n"                                   \
    "  mtd read  <part> <addr> <offset> <size>\n"    \
    "  mtd erase <part> <offset> <size>\n"           \
    "  mtd write <part> <addr> <offset> <size>\n"    \
    "  mtd oobdump  <part> <offset>\n"               \
    "  mtd oobread  <part> <addr> <offset>\n"        \
    "  mtd oobwrite <part> <addr> <offset>\n"        \
    "  mtd contread <part> <addr> <offset> <size>\n" \
    "  mtd isbad <part> <block>\n"                     \
    "e.g.:\n"                                        \
    "  mtd alloc 4096\n"                             \
    "  # Alloc buffer: 0x3e146e0\n"                  \
    "  mtd_gendata l 0x3e146e0 0x100 0xA0a1a2a3\n"   \
    "  mtd erase nand0 0 0x1000\n"                   \
    "  mtd write nand0 0x3e146e0 0 0x1000\n"
#else
#define MTD_HELP                                     \
    "mtd read write command:\n"                      \
    "  mtd list\n"                                   \
    "  mtd bypass <status>\n"                        \
    "      status = 1: bypass spienc\n"              \
    "      status = 0: enable spienc\n"              \
    "  mtd dump  <part> <offset> <size>\n"           \
    "  mtd alloc  <size>\n"                          \
    "  mtd free\n"                                   \
    "  mtd read  <part> <addr> <offset> <size>\n"    \
    "  mtd erase <part> <offset> <size>\n"           \
    "  mtd write <part> <addr> <offset> <size>\n"    \
    "e.g.:\n"                                        \
    "  mtd alloc 4096\n"                             \
    "  # Alloc buffer: 0x3e146e0\n"                  \
    "  mtd_gendata l 0x3e146e0 0x100 0xA0a1a2a3\n"   \
    "  mtd erase nor0 0 0x1000\n"                    \
    "  mtd write nor0 0x3e146e0 0 0x1000\n"
#endif

static void mtd_help(void)
{
    puts(MTD_HELP);
}

static int do_mtd_list(int argc, char *argv[])
{
    u32 cnt, i;
    struct mtd_dev *mtd;

    if (mtd_probe()) {
        printf("Failed to probe mtd device.\n");
        return -1;
    }
    cnt = mtd_get_device_count();

    if (cnt == 0) {
        printf("No mtd devices.\n");
        return 0;
    }

    printf("MTD devices:\n");
    for (i = 0; i < cnt; i++) {
        mtd = mtd_get_device_by_id(i);
        printf("%-24s 0x%08lx ~ 0x%08lx\n", mtd->name, mtd->start,
               mtd->start + mtd->size);
    }
    return 0;
}

static int do_mtd_bypass(int argc, char *argv[])
{
#if defined(AIC_SPIENC_DRV)
    unsigned long status;

    status = strtol(argv[1], NULL, 0);
    spienc_set_bypass(status);
#endif

    return 0;
}

void *g_test_buf = NULL;
static int do_mtd_alloc_mem(int argc, char *argv[])
{
    u8 *buf;
    unsigned long size;

    size = strtoul(argv[1], NULL, 0);
    buf = aicos_malloc_align(0, size, CACHE_LINE_SIZE);

    g_test_buf = buf;
    printf("Alloc buffer: 0x%lx\n", (long)buf);
    return 0;
}

static int do_mtd_free_mem()
{
    if (g_test_buf)
        aicos_free_align(0, g_test_buf);
    return 0;
}

static int do_mtd_dump(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data = NULL;
    char *name;
    unsigned long offset, size;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtoul(argv[2], NULL, 0);
    size = strtoul(argv[3], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    err = mtd_read(mtd, offset, data, size);
    if (!err)
        hexdump((void *)data, size, 1);
    aicos_free_align(0, data);
    return err;
}

static int do_mtd_read(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u64 start_us;

    if (argc < 5) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);
    size = strtoul(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    start_us = aic_get_time_us();
    err = mtd_read(mtd, offset, data, size);
    show_speed("mtd_read speed", size, aic_get_time_us() - start_us);
    return err;
}

static int do_mtd_erase(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    char *name;
    unsigned long offset, size;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtoul(argv[2], NULL, 0);
    size = strtoul(argv[3], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_erase(mtd, offset, size);
    return err;
}

static int do_mtd_write(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u64 start_us;
    u32 write_len = 0;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);
    size = strtoul(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Source address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }

    start_us = aic_get_time_us();
    if (mtd->ops.write_oob) {
        while (write_len < size) {
            err = mtd_write_oob(mtd, offset + write_len, data + write_len,
                                mtd->writesize, NULL, 0);
            if (err) {
                printf("Write mtd at offset 0x%lx error, mark it.\n",
                       offset + write_len);
                err = mtd_block_markbad(mtd,
                               ALIGN_DOWN(offset + write_len, mtd->erasesize));
                if (err)
                    printf("Mark block bad error.\n");

                return err;
            }
            write_len += mtd->writesize;
        }
    } else {
        err = mtd_write(mtd, offset, data, size);
    }
    show_speed("mtd_write speed", size, aic_get_time_us() - start_us);

    return err;
}

#ifdef AIC_SPINAND_DRV
static int do_mtd_oobdump(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long offset, size;

    if (argc < 3) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtoul(argv[2], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }

    size = mtd->oobsize + mtd->writesize;

    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    if (data == NULL) {
        printf("Out of memory.\n");
        return -1;
    }

    memset(data, 0, size);

    err = mtd_read_oob(mtd, offset, data, mtd->writesize, data + mtd->writesize,
                       mtd->oobsize);
    if (!err)
        hexdump((void *)data, size, 1);
    else
        printf("mtd read oob err:%d.\n", err);
    aicos_free_align(0, data);
    return err;
}

#ifdef AIC_SPINAND_CONT_READ
static int do_mtd_contread(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset, size;
    u32 start_us;

    if (argc < 5) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);
    size = strtoul(argv[4], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    start_us = aic_get_time_us();
    err = mtd_contread(mtd, offset, data, size);
    show_speed("mtd_contread speed", size, aic_get_time_us() - start_us);
    if (!err)
        hexdump((void *)data, size, 1);
    return err;
}
#endif

static int do_mtd_oobread(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Dest address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_read_oob(mtd, offset, data, mtd->writesize, data + mtd->writesize,
                       mtd->oobsize);
    return err;
}

static int do_mtd_oobwrite(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    u8 *data;
    char *name;
    unsigned long addr, offset;

    if (argc < 4) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    addr = strtoul(argv[2], NULL, 0);
    offset = strtoul(argv[3], NULL, 0);

    data = (u8 *)addr;
    if (data == NULL) {
        printf("Source address is not correct.\n");
        return -1;
    }

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_write_oob(mtd, offset, data, mtd->writesize,
                        data + mtd->writesize, mtd->oobsize);
    return err;
}

static int do_mtd_block_isbad(int argc, char *argv[])
{
    int err;
    struct mtd_dev *mtd;
    char *name;
    unsigned long offset;

    if (argc < 3) {
        mtd_help();
        return -1;
    }

    name = argv[1];
    offset = strtoul(argv[2], NULL, 0);

    mtd = mtd_get_device(name);
    if (!mtd) {
        printf("Failed to get mtd %s\n", name);
        mtd_help();
        return -1;
    }
    err = mtd_block_isbad(mtd, offset);
    return err;
}
#endif

static int do_mtd(int argc, char *argv[])
{
    if (argc < 2) {
        mtd_help();
        return 0;
    }

    if (!strncmp(argv[1], "list", 4))
        return do_mtd_list(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "bypass", 6))
        return do_mtd_bypass(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "dump", 4))
        return do_mtd_dump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "alloc", 5))
        return do_mtd_alloc_mem(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "free", 4))
        return do_mtd_free_mem();
    else if (!strncmp(argv[1], "read", 4))
        return do_mtd_read(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "write", 5))
        return do_mtd_write(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "erase", 5))
        return do_mtd_erase(argc - 1, &argv[1]);
#ifdef AIC_SPINAND_DRV
    else if (!strncmp(argv[1], "oobdump", 7))
        return do_mtd_oobdump(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "oobread", 7))
        return do_mtd_oobread(argc - 1, &argv[1]);
    else if (!strncmp(argv[1], "oobwrite", 8))
        return do_mtd_oobwrite(argc - 1, &argv[1]);
#ifdef AIC_SPINAND_CONT_READ
    else if (!strncmp(argv[1], "contread", 8))
        return do_mtd_contread(argc - 1, &argv[1]);
#endif
    else if (!strncmp(argv[1], "isbad", 8))
        return do_mtd_block_isbad(argc - 1, &argv[1]);
#endif
    mtd_help();
    return 0;
}

#define GD_HELP                                                 \
    "Generate test data command:\n"                             \
    "  mtd_gendata [mode] <addr> <cnt> <value>\n"               \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    cnt:  count of data\n"                                 \
    "    value: value going to write\n"                         \
    "  e.g.: \n"                                                \
    "    mtd_gendata 0x40000000 64\n"                           \
    "    mtd_gendata 0x40000000 64 0xAA\n"                      \
    "    mtd_gendata w 0x40000000 64 0xA5\n"                    \
    "    mtd_gendata l 0x40000000 64 0x5A\n"

static void gen_mem_data_help(void)
{
    puts(GD_HELP);
}

static int gen_mem_data(int argc, char *argv[])
{
    unsigned long addr, val, cnt, i;
    unsigned char *p;
    int next, rand, groupsize;

    val = 0;
    if (argc < 3) {
        goto help;
    }

    rand = 0;
    next = 1;

    groupsize = 1;
    if (*argv[next] == 'b') {
        groupsize = 1;
        next++;
    } else if (*argv[next] == 'w') {
        groupsize = 2;
        next++;
    } else if (*argv[next] == 'l') {
        groupsize = 4;
        next++;
    }

    addr = strtoul(argv[next++], NULL, 0);
    cnt = strtoul(argv[next++], NULL, 0);
    if (next < argc)
        val = strtoul(argv[next++], NULL, 0);
    else
        rand = 1;

    printf("groupsize %d, addr 0x%lx, cnt %ld, val 0x%lx\n", groupsize, addr, cnt,
            val);
    p = (unsigned char *)addr;
    for (i = 0; i < cnt; i++) {
        if (rand) {
            val = i % (1 << (8 * groupsize));
        }
        memcpy(p, &val, groupsize);
        p += groupsize;
    }
    return 0;

help:
    gen_mem_data_help();
    return 0;
}

CONSOLE_CMD(mtd, do_mtd, "MTD R/W command.");
CONSOLE_CMD(mtd_gendata, gen_mem_data,   "Generate test data to memory");
