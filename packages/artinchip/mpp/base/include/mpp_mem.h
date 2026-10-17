/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <qi.xu@artinchip.com>
 * Desc: virtual memory allocator
 */

#ifndef MPP_MEM_H
#define MPP_MEM_H

#include <limits.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void *_mpp_alloc_(size_t len,const char *file,int line);

void show_mem_info_debug();

void mpp_free(void *ptr);

#define  mpp_alloc(len) _mpp_alloc_(len,__FILE__,__LINE__)

void *mpp_realloc(void *ptr,size_t size);

unsigned int mpp_phy_alloc(size_t size);
void mpp_phy_free(unsigned int addr);

#ifdef __cplusplus
}
#endif

#endif /* MEM_H */
