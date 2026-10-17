/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "aic_hal.h"
#include "usb_otg.h"
#include "usbd_core.h"
#include "usbh_core.h"
#if defined(KERNEL_RTTHREAD)
#include <rtdevice.h>
#include <rtthread.h>
#endif

struct usb_otg g_usb_otg;

static int usb_otg_get_id(void);
static void usb_otg_sw_mode(unsigned int mode);

#if defined(KERNEL_RTTHREAD)
static void usb_otg_irq(void *data)
#else
static void usb_otg_irq(int irq_num, void *data)
#endif
{
    struct usb_otg *d = &g_usb_otg;

    usb_osal_sem_give(d->waitsem);
}

#if defined(KERNEL_RTTHREAD)
static void usbh_otg_thread(void *argument)
{
    struct usb_otg *d = &g_usb_otg;
    int mode = 0;
    int ret = 0;

    while (1) {
        ret = usb_osal_sem_take(d->waitsem, USB_OSAL_WAITING_FOREVER);
        if (ret < 0) {
            continue;
        }

        mode = usb_otg_get_id();
        if (mode < 0) {
            USB_LOG_ERR("get mode err.\n");
            break;
        }

        usb_otg_sw_mode(mode);
    }
}
#elif defined(KERNEL_BAREMETAL)
void usbh_otg_thread_poll(void *argument)
{
    struct usb_otg *d = &g_usb_otg;
    int mode = 0;
    int ret = 0;

    ret = usb_osal_sem_take(d->waitsem, 0);
    if (ret < 0) {
        return;
    }

    mode = usb_otg_get_id();
    if (mode < 0) {
        USB_LOG_ERR("get mode err.\n");
        return;
    }

    usb_otg_sw_mode(mode);
}
#endif

static int usb_otg_id_detect_en(void)
{
    struct usb_otg *d = &g_usb_otg;
    int pin = d->id_gpio;

    if (pin < 0) {
        USB_LOG_ERR("get id gpio err.\n");
        return -1;
    }

    d->waitsem = usb_osal_sem_create(0);
#if defined(KERNEL_RTTHREAD)
    d->otg_thread = usb_osal_thread_create("usbh_enum", CONFIG_USBHOST_PSC_STACKSIZE, CONFIG_USBHOST_PSC_PRIO + 1, usbh_otg_thread, NULL);
    if (d->otg_thread == NULL) {
        USB_LOG_ERR("Failed to create hub thread\r\n");
        usb_osal_sem_delete(d->waitsem);
        return -1;
    }

    rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING_FALLING, usb_otg_irq, RT_NULL);
    rt_pin_irq_enable(pin, 1);
#elif defined(KERNEL_BAREMETAL)
    unsigned int g, p;
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    aicos_request_irq(AIC_GPIO_TO_IRQ(pin), (irq_handler_t)usb_otg_irq, 0, "pin", NULL);
    hal_gpio_set_irq_mode(g, p, PIN_IRQ_MODE_EDGE_BOTH);
    hal_gpio_enable_irq(g, p);
#endif

    return 0;
}

static void usb_otg_id_detect_dis(void)
{
    struct usb_otg *d = &g_usb_otg;
    int pin = d->id_gpio;

    if (pin < 0) {
        USB_LOG_ERR("get id gpio err.\n");
        return;
    }

#if defined(KERNEL_RTTHREAD)
    rt_pin_irq_enable(pin, 0);
    rt_pin_detach_irq(pin);
    if (d->otg_thread)
        usb_osal_thread_delete(d->otg_thread);
#elif defined(KERNEL_BAREMETAL)
    unsigned int g, p;
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    aicos_irq_disable(AIC_GPIO_TO_IRQ(pin));
    hal_gpio_disable_irq(g, p);
#endif
    if (d->waitsem)
        usb_osal_sem_delete(d->waitsem);
}

static void usb_otg_set_vbus(unsigned char on)
{
#ifdef LPKG_CHERRYUSB_VBUSEN_GPIO
    struct usb_otg *d = &g_usb_otg;
    int pin = d->vbus_en_gpio;
    unsigned int g, p;

    if (pin < 0) {
        USB_LOG_ERR("get vbus-en gpio err.\n");
        return;
    }

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    if (on ^ d->vbus_en_gpio_polarity)
        hal_gpio_set_value(g, p, 0);
    else
        hal_gpio_set_value(g, p, 1);
#endif
}


static void usb_otg_set_dpsw_host(unsigned char on)
{
#ifdef LPKG_CHERRYUSB_DPSW_GPIO
    struct usb_otg *d = &g_usb_otg;
    int pin = d->dp_sw_gpio;
    unsigned int g, p;

    if (pin < 0) {
        USB_LOG_ERR("get dp-sw gpio err.\n");
        return;
    }

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    if (on ^ d->dp_sw_gpio_polarity)
        hal_gpio_set_value(g, p, 0);
    else
        hal_gpio_set_value(g, p, 1);
#endif
}

static int usb_otg_get_id(void)
{
    struct usb_otg *d = &g_usb_otg;
    int pin = d->id_gpio;
    unsigned int g, p, val;

    if (pin < 0) {
        USB_LOG_ERR("get dp-sw gpio err.\n");
        return -1;
    }

    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);

    hal_gpio_get_value(g, p, &val);

    if (val ^ d->id_gpio_polarity)
        return OTG_MODE_DEVICE;
    else
        return OTG_MODE_HOST;
}

int usb_otg_register_host(int index, struct usbh_bus *host)
{
    struct usb_otg *d = &g_usb_otg;

    if (index >= MAX_USB_HOST_BUS) {
        USB_LOG_ERR("get id gpio err.\n");
        return -1;
    }

    d->host[index] = host;

    if (d->mode != OTG_MODE_HOST)
        return 1;

    return 0;
}

int usb_otg_register_device(void *device)
{
    static unsigned char init = 0;
    struct usb_otg *d = &g_usb_otg;

    if (init)
        return 0;
    else
        init = 1;

    if (d->auto_flg)
        usb_otg_id_detect_en();

    d->device = device;

    if (d->mode != OTG_MODE_DEVICE)
        return 1;

    return 0;
}

static void usb_otg_start_host(unsigned char on)
{
    struct usb_otg *d = &g_usb_otg;
    int i = 0;

    for (i=0; i< MAX_USB_HOST_BUS; i++) {
        if ((d->host[i] == NULL)/* || (on == d->host_on[i])*/)
            continue;

        if (on) {
#ifdef LPKG_CHERRYUSB_HOST
            usbh_initialize(d->host[i]);
#endif
        } else {
#ifdef LPKG_CHERRYUSB_HOST
            usbh_deinitialize(d->host[i]);
#endif
        }

        d->host_on[i] = on;
    }
}

static void usb_otg_start_device(unsigned char on)
{
    struct usb_otg *d = &g_usb_otg;

    /*if (d->device_on == on)
        return;*/

    if (on) {
#ifdef LPKG_CHERRYUSB_DEVICE
        usbd_controller_init(0);
#endif
    } else {
#ifdef LPKG_CHERRYUSB_DEVICE
        usbd_controller_deinit(0);
#endif
    }

    d->device_on = on;
}

static void usb_otg_sw_gpio(unsigned int mode)
{
    struct usb_otg *d = &g_usb_otg;

    if (OTG_MODE_HOST == mode) {
        usb_otg_set_vbus(1);
        usb_otg_set_dpsw_host(1);
    } else {
        usb_otg_set_vbus(0);
        usb_otg_set_dpsw_host(0);
    }

    d->mode = mode;
}

static void usb_otg_sw_mode(unsigned int mode)
{
    struct usb_otg *d = &g_usb_otg;

    if (mode == d->mode)
        return;

    if (d->auto_flg)
        printf("Usb otg auto mode ");
    else
        printf("Usb otg manual mode ");

    if (OTG_MODE_HOST == mode) {
        usb_otg_start_device(0);
        usb_otg_sw_gpio(mode);
        usb_otg_start_host(1);
        printf("(switch usb0 to host).\n");
    } else {
        usb_otg_start_host(0);
        usb_otg_sw_gpio(mode);
        usb_otg_start_device(1);
        printf("(switch usb0 to device).\n");
    }

    d->mode = mode;
}

void usb_otg_set_mode(unsigned int auto_flg, unsigned int mode)
{
    struct usb_otg *d = &g_usb_otg;
    int id_mode = 0;

    if ((auto_flg == d->auto_flg) && (mode == d->mode))
        return;

    if (auto_flg != d->auto_flg) {
        if (auto_flg) {
            if (d->id_gpio < 0) {
                USB_LOG_ERR("Auto mode get id gpio err.\n");
                return;
            }
            id_mode = usb_otg_get_id();
            usb_otg_sw_mode(id_mode);
            usb_otg_id_detect_en();
        } else {
            usb_otg_id_detect_dis();
            usb_otg_sw_mode(mode);
        }
        d->auto_flg = !!auto_flg;
    } else {
        if ((!auto_flg) && (mode != d->mode)) {
            usb_otg_sw_mode(mode);
        }
    }
}

int usb_otg_get_mode(unsigned int *auto_flg, unsigned int *mode)
{
    *auto_flg = g_usb_otg.auto_flg;
    *mode = g_usb_otg.mode;

    return 0;
}

static int usb_otg_config_init(void)
{
    struct usb_otg *d = &g_usb_otg;
    int __attribute__((unused)) pin;
    unsigned int __attribute__((unused)) g, p;

    /* (1) Get id gpio */
    d->id_gpio = -1;
#if defined(LPKG_CHERRYUSB_ID_GPIO) && !defined(AIC_BOOTLOADER)
    pin = hal_gpio_name2pin(LPKG_CHERRYUSB_ID_GPIO_NAME);
    if (pin < 0) {
        USB_LOG_ERR("get id gpio err.\n");
        return -1;
    }
    d->id_gpio = pin;
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_bias_pull(g, p, PIN_PULL_UP);
    hal_gpio_set_debounce(g, p, 0xFFF);
    hal_gpio_direction_input(g, p);
    hal_gpio_set_irq_mode(g, p, PIN_IRQ_MODE_EDGE_BOTH);
    #ifdef LPKG_CHERRYUSB_ID_GPIO_ACT_LOW
    d->id_gpio_polarity = 0;
    #else
    d->id_gpio_polarity = 1;
    #endif
#endif

    /* (2) Get vbus-en gpio */
    d->vbus_en_gpio = -1;
#ifdef LPKG_CHERRYUSB_VBUSEN_GPIO
    pin = hal_gpio_name2pin(LPKG_CHERRYUSB_VBUSEN_GPIO_NAME);
    if (pin < 0) {
        USB_LOG_ERR("get vbus-en gpio err.\n");
        return -1;
    }
    d->vbus_en_gpio = pin;
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_direction_output(g, p);
    #ifdef LPKG_CHERRYUSB_VBUSEN_GPIO_ACT_LOW
    d->vbus_en_gpio_polarity = 0;
    #else
    d->vbus_en_gpio_polarity = 1;
    #endif
#endif

    /* (3) Get dp-sw gpio */
    d->dp_sw_gpio = -1;
#ifdef LPKG_CHERRYUSB_DPSW_GPIO
    pin = hal_gpio_name2pin(LPKG_CHERRYUSB_DPSW_GPIO_NAME);
    if (pin < 0) {
        USB_LOG_ERR("get dp-sw gpio err.\n");
        return -1;
    }
    d->dp_sw_gpio = pin;
    g = GPIO_GROUP(pin);
    p = GPIO_GROUP_PIN(pin);
    hal_gpio_set_func(g, p, 1);
    hal_gpio_set_drive_strength(g, p, 3);
    hal_gpio_direction_output(g, p);
    #ifdef LPKG_CHERRYUSB_DPSW_GPIO_ACT_LOW
    d->dp_sw_gpio_polarity = 0;
    #else
    d->dp_sw_gpio_polarity = 1;
    #endif
#endif

    /* (4) Get otg auto/manual mode */
#if defined(LPKG_CHERRYUSB_OTG_AUTO) && !defined(AIC_BOOTLOADER)
    d->auto_flg = 1;
    if (d->id_gpio < 0) {
        USB_LOG_ERR("get id gpio err.\n");
        return -1;
    }
    d->mode = usb_otg_get_id();
    usb_otg_sw_gpio(d->mode);
    //usb_otg_id_detect_en();
#else
    d->auto_flg = 0;
    #ifdef LPKG_CHERRYUSB_OTG_DEF_HOST
    d->mode = OTG_MODE_HOST;
    #else
    d->mode = OTG_MODE_DEVICE;
    #endif
    usb_otg_sw_gpio(d->mode);
#endif

    return 0;
}

int usb_otg_init(void)
{
    int ret = EOK;

    memset(&g_usb_otg, 0, sizeof(struct usb_otg));
    usb_otg_config_init();

    return ret;
}

#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static void cmd_set_otg_mode_usage(void)
{
    printf("Usage: set_otg_mode [auto/host/device]\n");
    printf("Example: set_otg_mode device\n");
}

static int cmd_set_otg_mode(int argc, char **argv)
{
    if (argc != 2) {
        cmd_set_otg_mode_usage();
        return -1;
    }
    if (strncmp(argv[1], "auto", 4) == 0) {
        usb_otg_set_mode(1, 0);
    } else if (strncmp(argv[1], "host", 4) == 0) {
        usb_otg_set_mode(0, OTG_MODE_HOST);
    } else if (strncmp(argv[1], "device", 5) == 0) {
        usb_otg_set_mode(0, OTG_MODE_DEVICE);
    } else {
        cmd_set_otg_mode_usage();
        return -1;
    }
    return 0;
}

static int cmd_get_otg_mode(int argc, char **argv)
{
    unsigned int auto_flg, mode;

    usb_otg_get_mode(&auto_flg, &mode);

    if (auto_flg)
        printf("Usb otg auto mode ");
    else
        printf("Usb otg manual mode ");

    if (mode == OTG_MODE_HOST)
        printf("(usb0 = host).\n");
    else
        printf("(usb0 = device).\n");

    return 0;
}

#if defined(RT_USING_FINSH)
MSH_CMD_EXPORT_ALIAS(cmd_set_otg_mode, set_otg_mode, Set usb otg mode);
MSH_CMD_EXPORT_ALIAS(cmd_get_otg_mode, get_otg_mode, Get usb otg mode);
#elif defined(AIC_CONSOLE_BARE_DRV)
#include <console.h>
CONSOLE_CMD(set_otg_mode, cmd_set_otg_mode, "Set usb otg mode");
CONSOLE_CMD(get_otg_mode, cmd_get_otg_mode, "Get usb otg mode");
#endif
#endif // defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)

#if defined(KERNEL_RTTHREAD)
INIT_BOARD_EXPORT(usb_otg_init);
#endif

