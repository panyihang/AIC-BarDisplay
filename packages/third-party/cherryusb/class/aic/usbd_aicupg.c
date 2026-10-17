/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include "usbd_core.h"
#include "usbd_aicupg.h"
#if defined(CONFIG_USBDEV_AICUPG_THREAD)
#include "usb_osal.h"
#endif
#include "aicupg.h"
#include <aic_utils.h>

#define BULK_OUT_EP_IDX 0
#define BULK_IN_EP_IDX  1

/* Describe EndPoints configuration */
static struct usbd_endpoint aicupg_ep_data[2];

#define AIC_BULK_OUT_EP aicupg_ep_data[BULK_OUT_EP_IDX].ep_addr
#define AIC_BULK_IN_EP  aicupg_ep_data[BULK_IN_EP_IDX].ep_addr

#define TRANS_DATA_BUFF_MAX_SIZE (64 * 1024)

static struct aicupg_trans_info trans_info;

static uint32_t aicupg_ep_start_read(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    usbd_ep_start_read(0, AIC_BULK_OUT_EP, data, data_len);
    return data_len;
}

static uint32_t aicupg_ep_start_write(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    usbd_ep_start_write(0, AIC_BULK_IN_EP, data, data_len);
    return data_len;
}

static int aicupg_class_interface_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    USB_LOG_DBG("AICUPG Class request: bRequest 0x%02x\r\n", setup->bRequest);

    switch (setup->bRequest) {
        default:
            USB_LOG_DBG("Unhandled AICUPG Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static void aicupg_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_CONFIGURED:
            USB_LOG_DBG("Start reading aic cbw\r\n");
            if (!trans_info.trans_pkt_buf) {
                trans_info.trans_pkt_siz = TRANS_DATA_BUFF_MAX_SIZE;
                trans_info.trans_pkt_buf = aicupg_malloc_align(trans_info.trans_pkt_siz, CACHE_LINE_SIZE);
                if (!trans_info.trans_pkt_buf) {
                    pr_err("malloc trans pkt buf(%u) failed.\n", (u32)trans_info.trans_pkt_siz);
                    return;
                }
            }
            trans_info.stage = AICUPG_READ_CBW;
            trans_info.recv((uint8_t *)trans_info.cbw, USB_SIZEOF_AIC_CBW);
            break;

        default:
            break;
    }
}

void aicupg_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_DBG("actual out len:%d\r\n", nbytes);

    data_trans_layer_out_proc(&trans_info);
}

void aicupg_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
    data_trans_layer_in_proc(&trans_info);
}

struct usbd_interface *usbd_aicupg_init_intf(struct usbd_interface *intf,
        uint8_t out_ep, uint8_t in_ep)
{
    intf->class_interface_handler = aicupg_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = aicupg_notify_handler;

    aicupg_ep_data[BULK_OUT_EP_IDX].ep_addr = out_ep;
    aicupg_ep_data[BULK_OUT_EP_IDX].ep_cb = aicupg_bulk_out;
    aicupg_ep_data[BULK_IN_EP_IDX].ep_addr = in_ep;
    aicupg_ep_data[BULK_IN_EP_IDX].ep_cb = aicupg_bulk_in;

    usbd_add_endpoint(0, &aicupg_ep_data[BULK_OUT_EP_IDX]);
    usbd_add_endpoint(0, &aicupg_ep_data[BULK_IN_EP_IDX]);

    memset((uint8_t *)&trans_info, 0, sizeof(struct aicupg_trans_info));

    trans_info.send = aicupg_ep_start_write;
    trans_info.recv = aicupg_ep_start_read;

    trans_info.cbw = aicupg_malloc_align(sizeof(struct aic_cbw), CACHE_LINE_SIZE);
    trans_info.csw = aicupg_malloc_align(sizeof(struct aic_csw), CACHE_LINE_SIZE);

    return intf;
}
