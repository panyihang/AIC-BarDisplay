/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USB_OTG_H
#define USB_OTG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "usb_config.h"
#include "usb_util.h"
#include "usb_errno.h"
#include "usb_def.h"
#include "usb_list.h"
#include "usb_log.h"
#include "usb_dc.h"
#include "usb_osal.h"

#define MAX_USB_HOST_BUS        2

#define OTG_MODE_HOST           0
#define OTG_MODE_DEVICE         1
#define OTG_MODE_MSK            (OTG_MODE_HOST | OTG_MODE_DEVICE)

struct usb_otg {
    struct usbh_bus * host[MAX_USB_HOST_BUS];
    void * device;
    unsigned char host_on[MAX_USB_HOST_BUS];
    unsigned char device_on;
    int mode;
    int auto_flg;
    int id_gpio;
    int vbus_en_gpio;
    int dp_sw_gpio;
    unsigned char id_gpio_polarity;
    unsigned char vbus_en_gpio_polarity;
    unsigned char dp_sw_gpio_polarity;
    usb_osal_thread_t otg_thread;
    usb_osal_sem_t waitsem;
};

int usb_otg_register_host(int index, struct usbh_bus *host);
int usb_otg_register_device(void *device);
void usb_otg_set_mode(unsigned int auto_flg, unsigned int mode);
int usb_otg_get_mode(unsigned int *auto_flg, unsigned int *mode);
void usbh_otg_thread_poll(void *argument);
int usb_otg_init(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_OTG_H */
