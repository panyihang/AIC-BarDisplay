/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: mingfeng.li <mingfeng.li@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <mtd.h>

static struct nftl_volume *nftl_new_volume(char *s)
{
    struct nftl_volume *vol = NULL;
    int cnt = 0;
    char *p;

    vol = malloc(sizeof(struct nftl_volume));
    if (!vol)
        return NULL;

    memset(vol, 0, sizeof(struct nftl_volume));

    p = s;
    if (*p == '-') {
        /* All remain space */
        vol->size = 0;
        p++;
    } else {
        vol->size = strtoull(p, &p, 0);
        if ((*p == 'k') || (*p == 'K')) {
            vol->size *= 1024;
            p++;
        } else if ((*p == 'm') || (*p == 'M')) {
            vol->size *= (1024 * 1024);
            p++;
        } else if ((*p == 'g') || (*p == 'G')) {
            vol->size *= (1024 * 1024 * 1024);
            p++;
        }
    }
    if (*p == '@') {
        p++;
        /* Don't care offset here, just skip it */
        strtoull(p, &p, 0);
    }
    if (*p != '(') {
        printf("Volume name should be next of size.\n");
        goto err;
    }
    p++;

    cnt = 0;
    while (*p != ')') {
        if (cnt >= MAX_NAND_NAME)
            break;
        vol->name[cnt++] = *p++;
    }
    p++;

    vol->vol_type = 1; /* Default is dynamic */
    if (*p == 's')
        vol->vol_type = 0; /* Static volume */

    /* Skip characters until '\0', ',', ';' */
    while ((*p != '\0') && (*p != ';') && (*p != ','))
        p++;

    if (*p == ',') {
        p++;
        vol->next = nftl_new_volume(p);
    }

    return vol;
err:
    if (vol)
        free(vol);
    return NULL;
}

static void nftl_free_volume(struct nftl_volume *vol)
{
    if (!vol)
        return;

    nftl_free_volume(vol->next);
    free(vol);
}

void free_nftl_list(struct nftl_mtd *nftl)
{
    if (!nftl)
        return;

    free_nftl_list(nftl->next);
    nftl_free_volume(nftl->vols);
    free(nftl);
}

struct nftl_mtd *build_nftl_list(char *nftlvols)
{
    struct nftl_mtd *head, *cur, *nxt;
    char *p;
    int cnt;

    head = NULL;
    cur = NULL;
    p = nftlvols;

next:
    nxt = malloc(sizeof(struct nftl_mtd));
    if (!nxt)
        goto err;
    memset(nxt, 0, sizeof(struct nftl_mtd));

    if (cur)
        cur->next = nxt;
    cur = nxt;
    if (!head)
        head = cur;

    cnt = 0;
    while (*p != '\0' && *p != ':') {
        if (cnt >= MAX_NAND_NAME)
            break;
        cur->name[cnt++] = *p++;
    }

    if (*p != ':') {
        printf("nftlvols_nand is invalid\n");
        goto err;
    }
    p++;

    cur->vols = nftl_new_volume(p);
    if (!cur->vols) {
        printf("Build nftl volume list failed.\n");
        goto err;
    }

    /* Check if there is other nftl device */
    while (*p != '\0' && *p != ';')
        p++;

    if (*p == ';') {
        p++;
        goto next;
    }

    return head;
err:
    if (head)
        free_nftl_list(head);
    return NULL;
}

u8 partition_nftl_is_exist(char *mtd_name, struct nftl_mtd *nftl_list)
{
    struct nftl_mtd *current_list;

    if (nftl_list == NULL) {
        return 0;
    }
    current_list = nftl_list;

    while (current_list) {
        if (strcmp(mtd_name, current_list->name) == 0)
            return 1;
        current_list = current_list->next;
    }
    pr_debug("NFTL Partition %s is not exist.....\n", mtd_name);
    return 0;
}

static struct mtd_partition *_part_parse(char *parts, u32 start)
{
    struct mtd_partition *part;
    int cnt = 0;
    char *p;

    part = malloc(sizeof(struct mtd_partition));
    if (!part)
        return NULL;

    memset(part, 0, sizeof(struct mtd_partition));

    part->start = start;
    p = parts;
    if (*p == '-') {
        /* All remain space */
        part->size = 0;
        p++;
    } else {
        part->size = strtoul(p, &p, 0);
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
        part->start = strtoul(p, &p, 0);
    }
    if (*p != '(') {
        printf("Partition name should be next of size.\n");
        printf("%s\n", p);
        goto err;
    }
    p++;

    cnt = 0;
    while (*p != ')') {
        if (cnt >= MAX_MTD_NAME)
            break;
        part->name[cnt++] = *p++;
    }
    p++;

    /* Skip characters until '\0', ',', ';' */
    while ((*p != '\0') && (*p != ';') && (*p != ','))
        p++;

    if (*p == ',') {
        p++;
        part->next = _part_parse(p, part->start + part->size);
    }

    return part;
err:
    if (part)
        free(part);
    return NULL;
}

struct mtd_partition *mtd_parts_parse(char *parts, u32 spi_bus)
{
    char *p;
    char prefix[16];

    p = parts;

    if (!p)
        return NULL;

    /* Build the prefix string to search for, e.g. "spi0.0:" or "spi1.0:" */
    snprintf(prefix, sizeof(prefix), "spi%u.0:", spi_bus);

    /* Find the prefix in the partition string */
    p = strstr(parts, prefix);
    if (p == NULL) {
        printf("mtdparts: spi_bus %u not found in: %s\n", spi_bus, parts);
        return NULL;
    }

    /* Skip the prefix to get to the partition list */
    p += strlen(prefix);

    return _part_parse(p, 0);
}

void mtd_parts_free(struct mtd_partition *head)
{
    struct mtd_partition *p, *n;

    p = head;
    n = p;
    while (p) {
        n = p->next;
        free(p);
        p = n;
    }
}

u8 partition_levelx_is_exist(char *mtd_name, char *levelx_partstr)
{
    if (mtd_name == NULL || levelx_partstr == NULL) {
        return 0;
    }

    size_t pattern_len = strlen(mtd_name) + 3;
    char *pattern = (char *)malloc(pattern_len);
    if (pattern == NULL) {
        return 0;
    }
    snprintf(pattern, pattern_len, "(%s)", mtd_name);

    if (strstr(levelx_partstr, pattern) != NULL) {
        free(pattern);
        return 1;
    } else {
        free(pattern);
        return 0;
    }
}
