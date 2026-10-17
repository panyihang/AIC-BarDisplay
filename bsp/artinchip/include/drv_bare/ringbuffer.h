/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <aic_core.h>
#include <aic_common.h>

typedef struct ringbuffer {
    uint8_t *buffer;
    uint32_t size;
    uint32_t write;
    uint32_t read;
    uint32_t data_len;
} ringbuf_t;

void ringbuf_reset(ringbuf_t *fifo);
uint32_t ringbuf_len(ringbuf_t *fifo);
uint32_t ringbuf_avail(ringbuf_t *fifo);
bool ringbuf_is_empty(ringbuf_t *fifo);
bool ringbuf_is_full(ringbuf_t *fifo);

void ringbuf_init(ringbuf_t *fifo, uint8_t *buf, uint32_t size);

/*write to ringbuffer*/
uint32_t ringbuf_in(ringbuf_t *fifo, const void *in, uint32_t len);

/*read to ringbuffer*/
uint32_t ringbuf_out(ringbuf_t *fifo, void *out, uint32_t len);

/*move to another ringbuffer*/
uint32_t ringbuf_move(ringbuf_t *fifo_in, ringbuf_t *fifo_out);

#ifdef __cplusplus
}
#endif

#endif /* _RING_BUFFER_H_ */
