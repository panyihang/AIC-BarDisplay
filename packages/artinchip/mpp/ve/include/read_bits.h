/*
 * Copyright (C) 2020-2025 Artinchip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  author: <qi.xu@artinchip.com>
 *  Desc: parse bitstream
 */

#ifndef READ_BITS_H
#define READ_BITS_H

#include <math.h>
#include <stdint.h>
#include "mpp_log.h"

#define MIN_CACHE_BITS 25
#define NEG_USR32(a, s) (((uint32_t)(a)) >> (32 - (s)))
#define BSWAP32(x) ((((x) << 8 & 0xff00) | ((x) >> 8 & 0x00ff)) << 16 | \
		((((x) >> 16) << 8 & 0xff00) | (((x) >> 16) >> 8 & 0x00ff)))

struct read_bit_context {
    const unsigned char *buffer, *buffer_end;
    int index;
    int size_in_bits;        // bit size in buffer
    const unsigned char *current_ptr;
    int bytes_remaining;
    uint32_t cache;
    int cache_bits;
    int emulation_prevention_state;
    int eptb_enable;
};

static inline int init_read_bits(struct read_bit_context* s, const unsigned char* buf, int bit_size, int eptb_enable)
{
    int ret = 0;
    if (bit_size < 0 || !buf) {
        bit_size = 0;
        buf = NULL;
        ret = -1;
    }

    int buffer_size = (bit_size + 7) >> 3;

    s->buffer = buf;
    s->size_in_bits = bit_size;
    s->buffer_end = buf + buffer_size;
    s->index = 0;
    s->current_ptr = buf;
    s->bytes_remaining = buffer_size;
    s->cache = 0;
    s->cache_bits = 0;
    s->emulation_prevention_state = 0;
    s->eptb_enable = eptb_enable;

    return ret;
}

static inline int get_next_byte(struct read_bit_context *s)
{
    while (s->bytes_remaining > 0) {
        unsigned char byte = *s->current_ptr++;
        s->bytes_remaining--;

        if (!s->eptb_enable)
                return byte;

        switch (s->emulation_prevention_state) {
            case 0:
                if (byte == 0x00) {
                    s->emulation_prevention_state = 1;
                }
                return byte;
            case 1:
                if (byte == 0x00) {
                    s->emulation_prevention_state = 2;
                } else {
                    s->emulation_prevention_state = 0;
                }
                return byte;
            case 2:
                if (byte == 0x03) {
                    s->emulation_prevention_state = 0;
                    s->index += 8;
                    continue;
                } else {
                    s->emulation_prevention_state = (byte == 0x00) ? 1 : 0;
                    return byte;
                }
            default:
                s->emulation_prevention_state = 0;
                return byte;
        }
    }
    return -1;
}

static inline void fill_cache(struct read_bit_context *s, int n)
{
    while (s->cache_bits < n && s->bytes_remaining > 0) {
        int byte = get_next_byte(s);
        if (byte == -1) break;

        s->cache = (s->cache << 8) | byte;
        s->cache_bits += 8;
    }
}

/**
* Read 1-25 bits.
* careful: we donot check the end of bitstream
*/
static inline unsigned int read_bits(struct read_bit_context *s, int n)
{
    if (n <= 0) return 0;

    fill_cache(s, n);

    if (s->cache_bits < n) {
        n = s->cache_bits;
        if (n == 0) return 0;
    }

    unsigned int result = (s->cache >> (s->cache_bits - n)) & ((1 << n) - 1);
    s->cache_bits -= n;
    s->index += n;

    return result;
}

static inline void skip_bits(struct read_bit_context *s, int n)
{
    if (n <= 0) return;

    fill_cache(s, n);

    if (s->cache_bits < n) {
        n = s->cache_bits;
    }

    s->cache_bits -= n;
    s->index += n;
}

/**
* Show 1-25 bits.
*/
static inline unsigned int show_bits(struct read_bit_context *s, int n)
{
    unsigned int re_index = (s)->index;
    unsigned int re_cache;
    re_cache = BSWAP32((*((const uint32_t*)((s)->buffer + (re_index >> 3))))) << (re_index & 7);

    unsigned int tmp = NEG_USR32(re_cache, n);

    return tmp;
}

/**
* get current bit offset.
*/
static inline int read_bits_count(struct read_bit_context *s)
{
    return s->index;
}

/**
* get bits left in stream.
*/
static inline int read_bits_left(struct read_bit_context *s)
{
    return s->size_in_bits - s->index;
}

/**
 * Read 0-32 bits.(big endian)
 */
static inline unsigned int read_bits_long(struct read_bit_context *s, int n)
{
    if (n <= 0) {
        return 0;
    } else if (n <= MIN_CACHE_BITS) {
        return read_bits(s, n);
    } else {
        unsigned ret = read_bits(s, 16) << (n - 16);
        return ret | read_bits(s, n - 16);
    }
}

/**
 * Show 0-32 bits.
 */
static inline unsigned int show_bits_long(struct read_bit_context *s, int n)
{
    if (n <= MIN_CACHE_BITS) {
        return show_bits(s, n);
    } else {
        struct read_bit_context gb = *s;
        return read_bits_long(&gb, n);
    }
}

/**
 * read ue(v) for avc.
 */
static inline int read_ue_golomb(struct read_bit_context *gb)
{
    int prefix_zero_cnt = 0;
    int val;
    int prefix = 0;
    int surfix = 0;
    int i=0;

    while(1)
    {
        val = read_bits(gb, 1);
        if(val == 0)
            prefix_zero_cnt++;
        else
            break;
    }
    prefix = (1 << prefix_zero_cnt) -1;
    for(i=0; i<prefix_zero_cnt; i++)
    {
        val = read_bits(gb, 1);
        surfix += val*(1 << (prefix_zero_cnt-i-1));
    }

    return prefix + surfix;
}

/**
 * read se(v) for avc.
 */
static inline int read_se_golomb(struct read_bit_context *gb)
{
    int uev = read_ue_golomb(gb);
    int sign = (uev & 1) ? 1: -1;
    int sev = sign * ((uev+1) >> 1);

    return sev;
}

#endif /* READ_BITS_H */
