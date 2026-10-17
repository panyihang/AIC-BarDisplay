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

#define MD_HELP                                                 \
    "memory display command:\n"                                 \
    "  md [mode] <addr> <count>\n"                              \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    count: display unit count\n"                           \
    "  e.g.: \n"                                                \
    "    md 0x40000000 64\n"                                    \
    "    md w 0x40000000 64\n"                                  \
    "    md l 0x40000000 64\n"

static void mem_display_help(void)
{
    puts(MD_HELP);
}

static int do_mem_display(int argc, char *argv[])
{
    int groupsize;
    unsigned long addr, cnt;

    cnt = 1;
    if (argc < 3) {
        goto help;
    }

    if (argc > 3) {
        if (*argv[1] == 'b')
            groupsize = 1;
        else if (*argv[1] == 'w')
            groupsize = 2;
        else if (*argv[1] == 'l')
            groupsize = 4;
        else
            goto help;
        addr = strtoul(argv[2], NULL, 0);
        cnt = strtoul(argv[3], NULL, 0);
    } else {
        groupsize = 1;
        addr = strtoul(argv[1], NULL, 0);
        cnt = strtoul(argv[2], NULL, 0);
    }

    hexdump((void *)addr, cnt * groupsize, groupsize);

    return 0;

help:
    mem_display_help();
    return 0;
}

#define MW_HELP                                                 \
    "memory write command:\n"                                   \
    "  mw [mode] <addr> <value>\n"                              \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    value: value going to write\n"                         \
    "  e.g.: \n"                                                \
    "    mw 0x40000000 64\n"                                    \
    "    mw w 0x40000000 64\n"                                  \
    "    mw l 0x40000000 64\n"

static void mem_write_help(void)
{
    puts(MW_HELP);
}

static int do_mem_write(int argc, char *argv[])
{
    unsigned long addr, val;
    unsigned char *p;
    int groupsize;

    val = 0;
    if (argc < 3) {
        goto help;
    }

    if (argc > 3) {
        if (*argv[1] == 'b')
            groupsize = 1;
        else if (*argv[1] == 'w')
            groupsize = 2;
        else if (*argv[1] == 'l')
            groupsize = 4;
        else
            goto help;
        addr = strtoul(argv[2], NULL, 0);
        val = strtoul(argv[3], NULL, 0);
    } else {
        groupsize = 1;
        addr = strtoul(argv[1], NULL, 0);
        val = strtoul(argv[2], NULL, 0);
    }

    p = (unsigned char *)addr;
    memcpy(p, &val, groupsize);
    return 0;

help:
    mem_write_help();
    return 0;
}

#define GD_HELP                                                 \
    "Generate test data command:\n"                             \
    "  gendata [mode] <addr> <cnt> <value>\n"                   \
    "    mode: should be \"b\" \"w\" \"l\", default is \"b\"\n" \
    "    addr: hex address string\n"                            \
    "    cnt:  count of data\n"                                 \
    "    value: value going to write\n"                         \
    "  e.g.: \n"                                                \
    "    gendata 0x40000000 64\n"                               \
    "    gendata 0x40000000 64 0xAA\n"                          \
    "    gendata w 0x40000000 64 0xA5\n"                        \
    "    gendata l 0x40000000 64 0x5A\n"

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

CONSOLE_CMD(md, do_mem_display, "Memory display");
CONSOLE_CMD(mw, do_mem_write,   "Memory write");
CONSOLE_CMD(gendata, gen_mem_data,   "Generate test data to memory");
