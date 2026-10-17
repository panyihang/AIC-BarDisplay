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
#include <mmc.h>
#include <disk_part.h>
#include <aic_utils.h>

static struct aic_sdmc *mmc_host = NULL;

#define PART_HELP                                                    \
    "Partition command:\n"                                      \
    "  part init <id>                                            \n" \
    "      <id> is mmc id."                                          \
    "  part list\n"                                                  \
    "  part dump\n"                                                  \
    "  part gptwrite [partition string]\n"                           \
    "  e.g.: \n"                                                     \
    "    part init 0\n"                                              \
    "    part list\n"                                                \
    "    part dump\n"                                                \
    "    part gptwrite 256k@0x4400(spl),8m(os),12m(rodata),35m(data)\n"

static void cmd_part_help(void)
{
    puts(PART_HELP);
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

static int cmd_part_init(int id)
{
    struct aic_sdmc *host = NULL;
    struct disk_blk_ops ops;
    int ret;

    ret = mmc_init(id);
    if (ret) {
        printf("sdmc %d init failed.\n", id);
        return ret;
    }

    host = find_mmc_dev_by_index(id);
    if (!host) {
        pr_err("find mmc dev failed.\n");
        return -1;
    }

    mmc_host = host;

    ops.blk_write = mmc_write;
    ops.blk_read = mmc_read;
    aic_disk_part_set_ops(&ops);

    printf("mmc controller id %d\n", id);
    printf("Capacity %d MB\n", mmc_host->dev->card_capacity >> 10);
    return 0;
}

static int cmd_part_list(void)
{
    struct blk_desc dev_desc;
    struct aic_partition *parts, *p;
    dev_desc.blksz = 512;
    dev_desc.lba_count = mmc_host->dev->card_capacity * 2;
    dev_desc.priv = mmc_host;

    parts = aic_disk_get_parts(&dev_desc);

    p = parts;
    while (p) {
        printf("Start: 0x%08llx; Size: 0x%08llx; %s\n", p->start, p->size, p->name);
        p = p->next;
    }
    if (parts)
        aic_part_free(parts);
    return 0;
}

static int cmd_part_dump_gpt(void)
{
    struct blk_desc dev_desc;
    dev_desc.blksz = 512;
    dev_desc.lba_count = mmc_host->dev->card_capacity * 2;
    dev_desc.priv = mmc_host;

    aic_disk_dump_parts(&dev_desc);
    return 0;
}

static int cmd_gpt_write(char *partstr)
{
    struct aic_partition *partition;
    struct blk_desc dev_desc;
    int ret;

    partition = aic_part_gpt_parse(partstr);

    if (partition == NULL)
        return -1;
    dev_desc.blksz = 512;
    dev_desc.lba_count = mmc_host->dev->card_capacity * 2;
    dev_desc.priv = mmc_host;

    ret = aic_disk_write_gpt(&dev_desc, partition);
    if (ret) {
        printf("Write PART table failed.\n");
    }
    aic_part_free(partition);

    return 0;
}

static int do_part(int argc, char *argv[])
{
    char *cmd = NULL, *part;
    unsigned long id;

    cmd = argv[1];
    if (argc == 2) {
        if (!strcmp(cmd, "list")) {
            cmd_part_list();
            return 0;
        }
        if (!strcmp(cmd, "dump")) {
            cmd_part_dump_gpt();
            return 0;
        }
    }
    if (argc >= 3) {
        if (!strcmp(cmd, "init")) {
            id = strtol(argv[2], NULL, 0);
            cmd_part_init(id);
            return 0;
        } else if (!strcmp(cmd, "gptwrite")) {
            part = argv[2];
            printf("Part: %s\n", part);
            cmd_gpt_write(part);
            return 0;
        }
    }

    cmd_part_help();

    return 0;
}

CONSOLE_CMD(part, do_part, "Partition util");
