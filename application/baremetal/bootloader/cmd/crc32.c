/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <console.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <aic_crc32.h>

#define CRC32_HELP                               \
    "Calculate CRC32 value of data in memory:\n" \
    "  crc32 <addr> <length>\n"                  \
    "    addr: hex address string\n"             \
    "    length: count of data\n"                \
    "  e.g.: \n"                                 \
    "    crc32 0x40000000 64\n"

static void do_mem_crc32_help(void)
{
    puts(CRC32_HELP);
}

static int do_mem_crc32(int argc, char *argv[])
{
    unsigned long addr, len;
    u32 crcval;

    if (argc < 3) {
        goto help;
    }

    addr = strtoul(argv[1], NULL, 0);
    len = strtoul(argv[2], NULL, 0);

    crcval = (u32)crc32(0, (void *)addr, len);
    printf("CRC32 result: 0x%x\n", crcval);
    return 0;

help:
    do_mem_crc32_help();
    return 0;
}

CONSOLE_CMD(crc32, do_mem_crc32, "Calculate CRC32 of data in memory");
