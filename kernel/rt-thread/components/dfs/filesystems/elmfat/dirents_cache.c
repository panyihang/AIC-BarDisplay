/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_log.h>

#include "ff.h"
#include "dirents_cache.h"

/** @brief Maximum number of cached directory entry sectors */
#define MAX_CACHE_DIRENT_COUNT 90
#define DIRENT_CACHE_DEBUG

#define AIC_FAT_HEAD_MAGIC 0x46434941
struct aicfat_head_info {
    uint32_t magic;
    uint32_t cksum32;
    uint32_t length;
    uint32_t version;
    uint64_t image_size;
    uint64_t used_size;
    uint32_t dirent_sect_id;  /* Sector id */
    uint32_t dirent_data_len; /* Byte len */
};

struct dirent_cache_node {
    unsigned char *data;      /**< Sector data buffer */
    unsigned int index;
    struct dirent_cache_node *lru_prev;
    struct dirent_cache_node *lru_next;
#ifdef DIRENT_CACHE_DEBUG
    unsigned int cur_cache_reuse_cnt;
    unsigned int cache_buff_read_cnt;
    unsigned int disk_read_cnt;
#endif
};

/**
 * @brief Information about a cached directory entry sector
 */
struct dirent_sector_info {
    struct dirent_cache_node *node;
    unsigned int sect_id;     /**< Sector ID */
};

/**
 * @brief Private structure to manage directory entry cache sectors
 */
struct dirent_cache_priv {
    struct dirent_sector_info *sectors; /**< Array of cached sectors */
    struct dirent_cache_node *lru_head;
    struct dirent_cache_node *lru_tail;
    unsigned int id_count;                 /**< Number of valid entries in the array */
    unsigned int lru_count;
    unsigned int sector_size;           /**< Size of each sector in bytes */
};

/**
 * @brief Binary search for a sector ID within the sorted sector list
 *
 * This function performs a recursive binary search on the sector list to find
 * the index of the specified sector ID.
 *
 * @param[in] cache     Pointer to the private cache structure
 * @param[in] left      Left boundary of the current search range
 * @param[in] right     Right boundary of the current search range
 * @param[in] sect_id   Target sector ID to search for
 *
 * @return Index of the sector if found, otherwise -1
 */
static int dirent_sector_binary_search(struct dirent_cache_priv *cache, unsigned int left,
                                       unsigned int right, unsigned int sect_id)
{
    unsigned int mid;

    if (left > right)
        return -1;

    mid = left + (right - left) / 2;
    if (cache->sectors[mid].sect_id == sect_id)
        return (int)mid;

    if (cache->sectors[mid].sect_id < sect_id) {
        return dirent_sector_binary_search(cache, mid + 1, right, sect_id);
    } else {
        if (mid == 0)
            return -1;
        return dirent_sector_binary_search(cache, left, mid - 1, sect_id);
    }
    return -1;
}

#ifdef DIRENT_CACHE_DEBUG
struct dirent_cache_priv *g_dbg_dirent_cache = NULL;
void f_dirent_cache_set_debug_info(struct dirent_cache_priv *priv)
{
    g_dbg_dirent_cache = priv;
}

void f_dirent_cache_show_debug_info(void)
{
    if (g_dbg_dirent_cache == NULL)
        return;
    printf("cache sector count: %d\n", g_dbg_dirent_cache->lru_count);
    struct dirent_cache_node *node = g_dbg_dirent_cache->lru_head;
    while (node) {
        printf(
            "index %d: cur sect id %d, disk read cnt %d, cur cache used %d, buff total used %d\n",
            node->index, g_dbg_dirent_cache->sectors[node->index].sect_id, node->disk_read_cnt,
            node->cur_cache_reuse_cnt, node->cache_buff_read_cnt);
        node = node->lru_next;
    }
}
#endif

dirent_cache_t f_dirent_cache_init(unsigned int *id_list, unsigned int id_cnt,
                                   unsigned int sector_size)
{
    struct dirent_cache_priv *priv = NULL;
    unsigned int slist_len;

    priv = malloc(sizeof(struct dirent_cache_priv));
    if (!priv)
        return NULL;
    memset(priv, 0, sizeof(struct dirent_cache_priv));

    slist_len = id_cnt * sizeof(struct dirent_sector_info);
    priv->sectors = malloc(slist_len);
    if (!priv->sectors) {
        free(priv);
        return NULL;
    }
    memset(priv->sectors, 0, slist_len);
    priv->sector_size = sector_size;
    for (int i = 0; i < id_cnt; i++) {
        priv->sectors[i].sect_id = id_list[i];
    }
    priv->id_count = id_cnt;

#ifdef DIRENT_CACHE_DEBUG
    f_dirent_cache_set_debug_info(priv);
#endif
    return priv;
}

/**
 * @brief Free the directory entry cache handle
 *
 * This function releases all memory allocated for the cache including individual sector buffers.
 * Typically called during filesystem unmounting.
 *
 * @param[in] cache Handle to the directory entry cache to be freed
 */
void f_dirent_cache_free(dirent_cache_t cache)
{
    struct dirent_cache_priv *priv = (struct dirent_cache_priv *)cache;
    struct dirent_cache_node *dnode;

    if (priv == NULL)
        return;

    for (int i = 0; i < priv->id_count && priv->sectors != NULL; i++) {
        dnode = priv->sectors[i].node;
        if (dnode) {
            if (dnode->data)
                free(dnode->data);
            free(dnode);
            priv->sectors[i].node = NULL;
        }
    }
    priv->lru_head = NULL;
    priv->lru_tail = NULL;
    priv->lru_count = 0;
    if (priv->sectors) {
        free(priv->sectors);
        priv->sectors = NULL;
    }
    free(priv);
}

struct dirent_cache_node *dirent_lru_node_get_oldest(struct dirent_cache_priv *priv)
{
    if (priv->lru_count < MAX_CACHE_DIRENT_COUNT) {
        struct dirent_cache_node *dnode;
        /* Not reach the limitation, return new node. */
        dnode = (void *)malloc(sizeof(struct dirent_cache_node));
        memset(dnode, 0, sizeof(struct dirent_cache_node));
        dnode->data = (void *)malloc(priv->sector_size);
        dnode->lru_prev = NULL;
        dnode->lru_next = NULL;
        priv->lru_count++;
        return dnode;
    } else {
        /* Pop the tail node */
        struct dirent_cache_node *oldest, *prev;
        oldest = priv->lru_tail;
        if (!oldest) {
            pr_err("error, tail is null\n");
            return NULL;
        }
        prev = oldest->lru_prev;
        if (prev == NULL) {
            pr_err("error, tail prev is null\n");
            return NULL;
        }
        priv->lru_tail = prev;
        prev->lru_next = NULL;
        oldest->lru_prev = NULL;
        oldest->lru_next = NULL;
        priv->sectors[oldest->index].node = NULL;
        return oldest;
    }

    return NULL;
}

void dirent_lru_node_set_data(struct dirent_cache_priv *priv, struct dirent_cache_node *dnode,
                              unsigned char *buff, int index)
{
    memcpy(dnode->data, buff, priv->sector_size);
    dnode->index = index;
    priv->sectors[index].node = dnode;
    dnode->disk_read_cnt++;
    dnode->cur_cache_reuse_cnt = 0;
}

void dirent_lru_node_get_data(struct dirent_cache_priv *priv, struct dirent_cache_node *dnode,
                              unsigned char *buff)
{
    memcpy(buff, dnode->data, priv->sector_size);
    dnode->cur_cache_reuse_cnt++;
    dnode->cache_buff_read_cnt++;
}

int dirent_lru_node_set_latest(struct dirent_cache_priv *priv, struct dirent_cache_node *dnode)
{
    if (dnode->lru_prev) {
        pr_err("prev is not null\n");
    }
    if (dnode->lru_next) {
        pr_err("next is not null\n");
    }

    if (priv->lru_head == NULL) {
        /* First node */
        dnode->lru_prev = NULL;
        dnode->lru_next = NULL;
        priv->lru_head = dnode;
        priv->lru_tail = dnode;
    } else {
        dnode->lru_prev = NULL;
        dnode->lru_next = priv->lru_head;
        priv->lru_head->lru_prev = dnode;
        priv->lru_head = dnode;
    }
    return 0;
}

int dirent_lru_node_move_to_latest(struct dirent_cache_priv *priv, struct dirent_cache_node *dnode)
{
    struct dirent_cache_node *prev, *next;

    if (priv->lru_head == dnode)
        return 0;

    if (priv->lru_head == NULL) {
        pr_err("lru head is null\n");
    }

    prev = dnode->lru_prev;
    next = dnode->lru_next;
    if (prev == NULL) {
        pr_err("dnode prev is null\n");
    }
    prev->lru_next = next;
    if (next) {
        next->lru_prev = prev;
    }

    dnode->lru_prev = NULL;
    dnode->lru_next = priv->lru_head;
    priv->lru_head->lru_prev = dnode;
    priv->lru_head = dnode;
    if (dnode == priv->lru_tail) {
        priv->lru_tail = prev;
    }
    return 0;
}

/**
 * @brief Read data from the directory entry cache
 *
 * Checks if the requested sector has been cached. If so, returns the cached data.
 * Otherwise, indicates that the data needs to be loaded from storage.
 *
 * @param[in]  cache    Directory entry cache handle
 * @param[in]  sector   Logical block address of the sector to read
 * @param[out] buff     Buffer to copy the sector data into
 * @param[in]  count    Number of sectors to read (must be 1 currently)
 *
 * @return Positive value indicating number of sectors read if successful,
 *         DIRENT_DATA_MISSED if some data is missing,
 *         DIRENT_DATA_NOT_FOUND if the sector is not tracked,
 *         DIRENT_DATA_ERROR if an invalid parameter was passed
 */
int f_dirent_cache_read(dirent_cache_t cache, unsigned long sector, unsigned char *buff,
                        unsigned long count)
{
    struct dirent_cache_priv *priv = (struct dirent_cache_priv *)cache;
    int start = 0;

    if (priv == NULL)
        return DIRENT_DATA_ERROR;

    if (count > 1) {
        /* Referencing elmfat behavior: only one sector can be read per call */
        return DIRENT_DATA_NOT_FOUND;
    }
    if (priv->id_count == 0)
        return DIRENT_DATA_NOT_FOUND;

    start = dirent_sector_binary_search(priv, 0, priv->id_count - 1, (unsigned int)sector);

    if (start < 0)
        return DIRENT_DATA_NOT_FOUND;

    struct dirent_cache_node *dnode = priv->sectors[start].node;

    if (dnode == NULL) {
        return DIRENT_DATA_MISSED;
    }
    dirent_lru_node_get_data(priv, dnode, buff);
    dirent_lru_node_move_to_latest(priv, dnode);
    return count;
}

/**
 * @brief Set/copy sector data into the directory entry cache
 *
 * Stores the provided sector data into the corresponding slot in the cache.
 *
 * @param[in] cache     Directory entry cache handle
 * @param[in] sector    Logical block address of the sector being set
 * @param[in] buff      Buffer containing the sector data
 * @param[in] count     Number of sectors to set (must be 1 currently)
 *
 * @return Count of successfully set sectors, or -1 on error
 */
int f_dirent_cache_set(dirent_cache_t cache, unsigned long sector, unsigned char *buff,
                       unsigned long count)
{
    struct dirent_cache_priv *priv = (struct dirent_cache_priv *)cache;
    int index = 0;

    if (priv == NULL)
        return -1;

    if (count > 1) {
        /* Referencing elmfat behavior: only one sector can be set per call */
        return 0;
    }

    index = dirent_sector_binary_search(priv, 0, priv->id_count - 1, (unsigned int)sector);
    if (index < 0)
        return 0;

    struct dirent_cache_node *dnode;

    dnode = dirent_lru_node_get_oldest(priv);
    if (!dnode) {
        /* No buffer */
        return 0;
    }

    dirent_lru_node_set_data(priv, dnode, buff, index);
    dirent_lru_node_set_latest(priv, dnode);

    return count;
}

static uint32_t image_calc_checksum(uint8_t *buf, uint32_t size)
{
    uint32_t i, val, sum, rest, cnt;
    uint8_t *p;
    uint32_t *p32, *pe32;

    p = buf;
    i = 0;
    sum = 0;
    cnt = size >> 2;

    if ((unsigned long)buf & 0x3) {
        for (i = 0; i < cnt; i++) {
            p = &buf[i * 4];
            val = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
            sum += val;
        }
    } else {
        p32 = (uint32_t *)buf;
        pe32 = p32 + cnt;
        while (p32 < pe32) {
            sum += *p32;
            p32++;
        }
    }

    /* Calculate not 32 bit aligned part */
    rest = size - (cnt << 2);
    p = &buf[cnt * 4];
    val = 0;
    for (i = 0; i < rest; i++)
        val += (p[i] << (i * 8));
    sum += val;

    return sum;
}

int f_dirent_cache_get_sector_info(unsigned char *boot_sect, unsigned int *start_sect,
                                   unsigned int *bytelen)
{
    struct aicfat_head_info head;
    uint32_t cksum = 0;

    if (boot_sect == NULL) {
        return -1;
    }

    memcpy(&head, boot_sect + 0x80, sizeof(head));
    if (head.magic != AIC_FAT_HEAD_MAGIC) {
        return -1;
    }

    cksum = image_calc_checksum((void *)&head, sizeof(head));
    if (cksum != 0xFFFFFFFF) {
        return -1;
    }

    if (head.version <= 0x100)
        return -1;
    if (head.dirent_data_len == 0)
        return -1;

    *start_sect = head.dirent_sect_id;
    *bytelen = head.dirent_data_len;

    return 0;
}

