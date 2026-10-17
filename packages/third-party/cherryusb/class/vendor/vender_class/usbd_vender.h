/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _USBD_VENDER_H_
#define _USBD_VENDER_H_

#include "usb_vender.h"

#ifdef __cplusplus
extern "C" {
#endif

struct usbd_interface *usbd_vender_init_intf(uint8_t busid, struct usbd_interface *intf);

#ifdef __cplusplus
}
#endif

#endif /* _USBD_VENDER_H_ */
