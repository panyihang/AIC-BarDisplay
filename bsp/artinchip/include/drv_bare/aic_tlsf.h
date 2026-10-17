/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_TLSF_H_
#define _ARTINCHIP_AIC_TLSF_H_

#include "tlsf.h"

void aic_tlsf_heap_test(void);
void aic_tlsf_heap_init(void);
void *aic_tlsf_malloc(u32 mem_type, u32 nbytes);
void aic_tlsf_free(u32 mem_type, void *ptr);
void *aic_tlsf_realloc(u32 mem_type, void *ptr, u32 nbytes);
void *aic_tlsf_calloc(u32 mem_type, u32 count, u32 size);
void *aic_tlsf_malloc_align(u32 mem_type, u32 size, u32 align);
void aic_tlsf_free_align(u32 mem_type, void *ptr);
void aic_tlsf_mem_info(u32 mem_type, u32 *total, u32 *used,
                            u32 *max_used, u32 *free, u32 *max_free);

#endif
