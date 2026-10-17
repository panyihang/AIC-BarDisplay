/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc: mpp ringbuf interface
 */

#ifndef MPP_RINGBUF_H
#define MPP_RINGBUF_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif


typedef void *mpp_ringbuf_t;

mpp_ringbuf_t mpp_ringbuffer_create(int length);
void mpp_ringbuffer_destroy(mpp_ringbuf_t rb);
void mpp_ringbuffer_reset(mpp_ringbuf_t rb);
int mpp_ringbuffer_put(mpp_ringbuf_t rb, const unsigned char *ptr, int length);
int mpp_ringbuffer_get(mpp_ringbuf_t rb, unsigned char *ptr, int length);
int mpp_ringbuffer_data_len(mpp_ringbuf_t rb);
int mpp_ringbuffer_space_len(mpp_ringbuf_t rb);

#ifdef __cplusplus
}
#endif

#endif /* MPP_RINGBUF_H */
