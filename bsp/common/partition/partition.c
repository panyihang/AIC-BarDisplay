/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mingfeng.Li <mingfeng.li@artinchip.com>
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_partition.h>

static struct aic_partition *aic_part_parse(char *parts, u32 start)
{
    struct aic_partition *part;
    int cnt = 0;
    char *p;

    part = malloc(sizeof(struct aic_partition));
    if (!part)
        return NULL;

    memset(part, 0, sizeof(struct aic_partition));

    part->start = start;
    p = parts;
    if (*p == '-') {
        /* All remain space */
        part->size = 0;
        p++;
    } else {
        part->size = strtoull(p, &p, 0);
        if ((*p == 'k') || (*p == 'K')) {
            part->size *= 1024;
            p++;
        } else if ((*p == 'm') || (*p == 'M')) {
            part->size *= (1024 * 1024);
            p++;
        } else if ((*p == 'g') || (*p == 'G')) {
            part->size *= (1024 * 1024 * 1024);
            p++;
        }
    }
    if (*p == '@') {
        p++;
        /* Don't care offset here, just skip it */
        part->start = strtoull(p, &p, 0);
    }
    if (*p != '(') {
        printf("%s: Partition name should be next of size.\n", __FUNCTION__);
        printf("%s\n", p);
        goto err;
    }
    p++;

    cnt = 0;
    while (*p != ')') {
        if (cnt >= MAX_PARTITION_NAME)
            break;
        part->name[cnt++] = *p++;
    }
    p++;

    /* Skip characters until '\0', ',', ';' */
    while ((*p != '\0') && (*p != ';') && (*p != ','))
        p++;

    if (*p == ',') {
        p++;
        part->next = aic_part_parse(p, part->start + part->size);
    }

    return part;
err:
    if (part)
        free(part);
    return NULL;
}

static char *get_part_start(char *parts)
{
    char *p;

    p = parts + strlen(parts);

    /* Skip = : */
    while (p != parts) {
        if (*p != ':' && *p != '=') {
            p--;
        } else {
            p++;
            break;
        }
    }
    return p;
}

struct aic_partition *aic_part_mtd_parse(char *parts)
{
    char *p;

    p = get_part_start(parts);

    return aic_part_parse(p, 0);
}

struct aic_partition *aic_part_gpt_parse(char *parts)
{
    char *p;

    p = get_part_start(parts);
    return aic_part_parse(p, 0x4400);
}

struct aic_partition *aic_part_get_byname(struct aic_partition *head,
                                          const char *name)
{
    struct aic_partition *part = head;
    if (!part) {
        printf(
            "%s: local_aic is invalid, please use aic_parts_parse init the partition table\n",
            __FUNCTION__);
        return NULL;
    }
    while (part) {
        if (strncmp(name, part->name, strlen(name)) == 0) {
            return part;
        }
        part = part->next;
    }
    return NULL;
}

void aic_part_free(struct aic_partition *head)
{
    struct aic_partition *p, *n;

    p = head;
    n = p;
    while (p) {
        n = p->next;
        free(p);
        p = n;
    }
}

void aic_part_dump(struct aic_partition *head)
{
    struct aic_partition *p, *n;

    p = head;
    n = p;
    while (p) {
        n = p->next;
        printf("%s:\n", p->name);
        printf("\tstart 0x%lx\n", (unsigned long)p->start);
        printf("\tsize  0x%lx\n", (unsigned long)p->size);
        p = n;
    }
}
