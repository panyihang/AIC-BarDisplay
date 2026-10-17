/*
* Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
*
* SPDX-License-Identifier: Apache-2.0
*
* author: jun.ma@artinchip.com
* Desc: packet allocator
*/

#ifndef PACKET_ALLOCATOR_H
#define PACKET_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpp_dec_type.h"

struct packet_allocator {
    struct pkt_alloc_ops *ops;
};

struct pkt_alloc_ops {
    int (*allocator_init)(struct packet_allocator *p);
    int (*allocator_deinit)(struct packet_allocator *p);
    int (*alloc)(struct packet_allocator *p,struct mpp_packet *packet);
    int (*free)(struct packet_allocator *p,struct mpp_packet *packet);
};

#ifdef __cplusplus
}
#endif

#endif
