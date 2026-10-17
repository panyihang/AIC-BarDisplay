/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <efuse.h>
#include <console.h>

static void cmd_efuse_help(void)
{
    printf("efuse command usage:\n");
    printf("  efuse help                     : Get this help.\n");
    printf("  efuse dump     offset len      : Dump data from eFuse offset.\n");
    printf("  efuse read     addr offset len : Read eFuse data to RAM addr.\n");
#ifdef EFUSE_WRITE_SUPPORT
    printf("  efuse write    addr offset len : Write data to eFuse from RAM addr.\n");
    printf("  efuse writehex offset data     : Write data to eFuse from input hex string.\n");
    printf("  efuse writestr offset data     : Write data to eFuse from input string.\n");
#endif
    printf("  efuse authenticate sjtag key   : Authenticate secure jtag from hex string key.\n");
    printf("  efuse authenticate szone key   : Authenticate secure zone from hex string key.\n");
}

static void cmd_efuse_read(int argc, char **argv)
{
    ulong addr, offset, len;
    int ret;

    if (argc != 4) {
        printf("Invalid parameter.\n");
        return;
    }
    addr = strtoul(argv[1], NULL, 16);
    offset = strtoul(argv[2], NULL, 16);
    len = strtoul(argv[3], NULL, 16);

    ret = efuse_read(offset, (void *)addr, len);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return;
    }
    printf("Read efuse done.\n");
}

static void cmd_efuse_dump(int argc, char **argv)
{
    ulong offset, len;
    int i, j, ret;
    u8 data[512], c;

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    len = strtoul(argv[2], NULL, 16);

    ret = efuse_read(offset, (void *)data, len);
    if (ret <= 0) {
        printf("Read efuse error.\n");
        return;
    }
    for (i = 0; i < len; i += 16) {
        printf("0x%08lx: ", i + offset);
        for (j = i; j < i + 16; j++) {
            if (j < len)
                printf("%02X ", data[j]);
            else
                printf("   ");
        }
        printf("\t|");
        for (j = i; (j < len) && (j < i + 16); j++) {
            c = data[j] >= 32 && data[j] < 127 ? data[j] : '.';
            printf("%c", c);
        }
        printf("|\n");
    }
    printf("\n");
}

#ifdef EFUSE_WRITE_SUPPORT
static void cmd_efuse_write(int argc, char **argv)
{
    ulong addr, offset, len;
    int ret;

    if (argc != 4) {
        printf("Invalid parameter.\n");
        return;
    }
    addr = strtoul(argv[1], NULL, 16);
    offset = strtoul(argv[2], NULL, 16);
    len = strtoul(argv[3], NULL, 16);

    ret = efuse_program(offset, (const void *)addr, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}

static void cmd_efuse_writehex(int argc, char **argv)
{
    ulong offset, len;
    int ret, i, j;
    char *data, byte[3] = { 0x00, 0x00, 0x00 };
    u8 buf[512];

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    data = argv[2];
    len = strlen(data) / 2;

    /* hex string to hex value */
    for (i = 0, j = 0; i < strlen(data) - 1; i += 2, j += 1) {
        byte[0] = data[i];
        byte[1] = data[i + 1];
        buf[j] = strtol(byte, NULL, 16);
    }

    ret = efuse_program(offset, (const void *)buf, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}

static void cmd_efuse_writestr(int argc, char **argv)
{
    ulong offset, len;
    int ret;
    char *data;

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    offset = strtoul(argv[1], NULL, 16);
    data = argv[2];
    len = strlen(data);

    ret = efuse_program(offset, (const void *)data, len);
    if (ret <= 0) {
        printf("Program 0x%lx failed.\n", offset);
        return;
    }

    printf("Program efuse done.\n");
}
#endif

static void cmd_efuse_authenticate(int argc, char **argv)
{
    ulong len;
    int i, j;
    char *data, byte[3] = { 0x00, 0x00, 0x00 };
    u8 key[512];

    if (argc != 3) {
        printf("Invalid parameter.\n");
        return;
    }
    data = argv[2];
    len = strlen(data) / 2;

    /* hex string to hex value */
    for (i = 0, j = 0; i < strlen(data) - 1; i += 2, j += 1) {
        byte[0] = data[i];
        byte[1] = data[i + 1];
        key[j] = strtol(byte, NULL, 16);
    }

    if (!strcmp(argv[1], "sjtag")) {
        sjtag_auth((u32 *)key, len / 4);
    } else if (!strcmp(argv[1], "szone")) {
        szone_auth((u32 *)key, len / 4);
    }
}

static int cmd_efuse_do(int argc, char **argv)
{
    if (argc < 2) {
        cmd_efuse_help();
        return -1;
    }
    if (!strcmp(argv[1], "read")) {
        cmd_efuse_read(argc - 1, &argv[1]);
        return 0;
    }
    if (!strcmp(argv[1], "dump")) {
        cmd_efuse_dump(argc - 1, &argv[1]);
        return 0;
    }
#ifdef EFUSE_WRITE_SUPPORT
    if (!strcmp(argv[1], "write")) {
        efuse_write_enable();
        cmd_efuse_write(argc - 1, &argv[1]);
        efuse_write_disable();
        return 0;
    }
    if (!strcmp(argv[1], "writehex")) {
        efuse_write_enable();
        cmd_efuse_writehex(argc - 1, &argv[1]);
        efuse_write_disable();
        return 0;
    }
    if (!strcmp(argv[1], "writestr")) {
        efuse_write_enable();
        cmd_efuse_writestr(argc - 1, &argv[1]);
        efuse_write_disable();
        return 0;
    }
#endif
    if (!strcmp(argv[1], "authenticate")) {
        cmd_efuse_authenticate(argc - 1, &argv[1]);
        return 0;
    }
    cmd_efuse_help();

    return 0;
}
CONSOLE_CMD(efuse, cmd_efuse_do, "eFuse test example");
