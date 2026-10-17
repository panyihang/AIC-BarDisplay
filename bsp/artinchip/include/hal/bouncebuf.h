/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_BOUNCEBUF_H__
#define __INCLUDE_BOUNCEBUF_H__

#define GEN_BB_READ     (1 << 0)
#define GEN_BB_WRITE    (1 << 1)
#define GEN_BB_RW       (GEN_BB_READ | GEN_BB_WRITE)

struct bounce_buffer {
    void *user_buffer;
    void *bounce_buffer;
    size_t len;
    size_t len_aligned;
    unsigned int flags;
};

int bounce_buffer_start(struct bounce_buffer *state, void *data,
        size_t len, unsigned int flags);

int bounce_buffer_start_extalign(struct bounce_buffer *state, void *data,
                size_t len, unsigned int flags,
                size_t alignment,
                int (*addr_is_aligned)(struct bounce_buffer *state));

int bounce_buffer_stop(struct bounce_buffer *state);

#endif
