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
#include <aic_errno.h>

#define RUN_HELP                   \
    "run command:\n"               \
    "  run <cmd> [cmd args]\n"     \
    "    cmd: command name\n"      \
    "    cmd args: args for cmd\n" \
    "  e.g.: \n"                   \
    "    run md 0x40000000 64\n"   \
    "    run \"md w 0x40000000 64\"\n"

static void run_cmd_help(void)
{
    puts(RUN_HELP);
}

static int do_cmd_run(int argc, char *argv[])
{
    int i, total_len, ret;
    char *buf = NULL;

    total_len = 0;

    if (argc <= 1) {
        run_cmd_help();
        goto help;
    }

    for (i = 1; i < argc; i++) {
        total_len += strlen(argv[i]);
        total_len += 1;
    }

    buf = malloc(total_len);
    if (buf == NULL)
        return -ENOMEM;

    for (i = 1; i < argc; i++) {
        strcat(buf, argv[i]);
        strcat(buf, " ");
    }

    ret = console_run_cmd(buf);

    if (buf)
        free(buf);

    return ret;

help:
    run_cmd_help();
    return 0;
}

CONSOLE_CMD(run, do_cmd_run, "Run command string.");
