/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "usbd_core.h"
#include "usbd_vender.h"

static int vender_class_interface_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_WRN("VENDER Class request: "
                 "bRequest 0x%02x\r\n",
                 setup->bRequest);

    switch (setup->bRequest) {
        default:
            USB_LOG_WRN("Unhandled VENDER Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static int vender_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_WRN("VENDER request: "
                 "bRequest 0x%02x\r\n",
                 setup->bRequest);

    switch (setup->bRequest) {
        default:
            USB_LOG_WRN("Unhandled VENDER Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static void vender_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:

            break;

        default:
            break;
    }
}

struct usbd_interface *usbd_vender_init_intf(uint8_t busid, struct usbd_interface *intf)
{
    intf->class_interface_handler = vender_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = vender_request_handler;
    intf->notify_handler = vender_notify_handler;

    return intf;
}
