/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef USBD_AICUPG_H
#define USBD_AICUPG_H

#include "data_trans_layer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct usbd_interface *usbd_aicupg_init_intf(struct usbd_interface *intf,
        uint8_t out_ep, uint8_t in_ep);

#ifdef __cplusplus
}
#endif

#endif /* USBD_AICUPG_H */
