/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DIRENTS_CACHE_H__
#define __DIRENTS_CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Dirent cache state: data is not cached in memory yet. */
#define DIRENT_DATA_MISSED    0
/* Dirent cache state: program error. */
#define DIRENT_DATA_ERROR     -1
/* Dirent cache state: sector not found in cache list. */
#define DIRENT_DATA_NOT_FOUND -2

typedef void *dirent_cache_t;

dirent_cache_t f_dirent_cache_init(unsigned int *id_list, unsigned int id_cnt,
                                   unsigned int sector_size);
dirent_cache_t f_dirent_cache_create(char *drive_root, unsigned int sector_size);
void f_dirent_cache_free(dirent_cache_t cache);
int f_dirent_cache_read(dirent_cache_t cache, unsigned long sector, unsigned char *buff,
                        unsigned long count);
int f_dirent_cache_set(dirent_cache_t cache, unsigned long sector, unsigned char *buff,
                       unsigned long count);
int f_dirent_cache_get_sector_info(unsigned char *boot_sect, unsigned int *start_sect,
                                   unsigned int *bytelen);

#ifdef __cplusplus
}
#endif

#endif /* __DIRENTS_CACHE_H__ */
