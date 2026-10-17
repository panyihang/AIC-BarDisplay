/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include <aic_core.h>

/* ================ USB common Configuration ================ */

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)

#define usb_malloc(size) malloc(size)
#define usb_free(ptr)    free(ptr)

//#define CONFIG_USB_DBG_LEVEL USB_DBG_LOG
//#define CONFIG_USBDEV_SETUP_LOG_PRINT
#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* cache enable */
#define CONFIG_USB_DCACHE_ENABLE

/* attribute data into no cache ram */
#ifdef CONFIG_USB_DCACHE_ENABLE
#define USB_NOCACHE_RAM_SECTION
#define CONFIG_USB_ALIGN_SIZE CACHE_LINE_SIZE
#else

/* data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))
#endif

/* ================= USB Device Stack Configuration ================ */

/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 1024

/* Setup packet log for debug */
// #define CONFIG_USBDEV_SETUP_LOG_PRINT

/* Check if the input descriptor is correct */
// #define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
#ifdef LPKG_CHERRYUSB_DEVICE_TEST_MODE
#define CONFIG_USBDEV_TEST_MODE
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"
#endif

#ifdef USBDEV_MSC_THREAD
#define CONFIG_USBDEV_MSC_THREAD
#endif

#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO 4
#endif

#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE 2048
#endif

#ifdef USBDEV_MTP_THREAD
#define CONFIG_USBDEV_MTP_THREAD
#endif

#ifndef CONFIG_USBDEV_MTP_MAX_BUFSIZE
#define CONFIG_USBDEV_MTP_MAX_BUFSIZE 512
#endif

#ifndef CONFIG_USBDEV_MTP_GET_MAX_HANDLES
#define CONFIG_USBDEV_MTP_GET_MAX_HANDLES 512
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE 156
#endif

#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE 1536
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID 0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP

/* ================ USB HOST Stack Configuration ================== */

#if (defined(AIC_USING_USB0_HOST)|| defined(AIC_USING_USB0_OTG)) \
        && defined(AIC_USING_USB1_HOST)
#define CONFIG_USBHOST_MAX_BUS              2
#else
#define CONFIG_USBHOST_MAX_BUS              1
#endif

#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#ifdef KERNEL_RTTHREAD
#define CONFIG_USBHOST_MAX_EHPORTS          4
#else /* KERNEL_RTTHREAD */
#define CONFIG_USBHOST_MAX_EHPORTS          2
#endif /* KERNEL_RTTHREAD */
#ifdef LPKG_CHERRYUSB_HOST_UVC
#define CONFIG_USBHOST_MAX_INTERFACES       16
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 16
#else
#ifdef KERNEL_RTTHREAD
#define CONFIG_USBHOST_MAX_INTERFACES       8
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 8
#else /* KERNEL_RTTHREAD */
#define CONFIG_USBHOST_MAX_INTERFACES       4
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 4
#endif /* KERNEL_RTTHREAD */
#endif
#define CONFIG_USBHOST_MAX_ENDPOINTS        4

#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS 4
#define CONFIG_USBHOST_MAX_HID_CLASS     4
#define CONFIG_USBHOST_MAX_MSC_CLASS     2
#define CONFIG_USBHOST_MAX_AUDIO_CLASS   1
#define CONFIG_USBHOST_MAX_VIDEO_CLASS   1

#define CONFIG_USBHOST_DEV_NAMELEN 16

#ifndef CONFIG_USBHOST_PSC_PRIO
#define CONFIG_USBHOST_PSC_PRIO 0
#endif
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
#define CONFIG_USBHOST_PSC_STACKSIZE 4096
#endif

//#define CONFIG_USBHOST_GET_STRING_DESC

// #define CONFIG_USBHOST_MSOS_ENABLE
#define CONFIG_USBHOST_MSOS_VENDOR_CODE 0x00

/* Ep0 max transfer buffer */
#ifdef LPKG_CHERRYUSB_HOST_UVC
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 5120
#else
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 512
#endif

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#ifndef CONFIG_USBHOST_MSC_TIMEOUT
#define CONFIG_USBHOST_MSC_TIMEOUT 1500
#endif

/* ================ USB Device Port Configuration ================*/

#if defined(AIC_USING_USB0_DEVICE) || defined(AIC_USING_USB0_OTG)
/* AIC Device Controller Configuration */
#define CONFIG_USBDEV_MAX_BUS       1
#define CONFIG_USBDEV_DEF_BUS       0
#define CONFIG_USB_AIC_DC_PORT      1  /* 0 = FullSpeed, 1 = HighSpeed */
#define CONFIG_USB_AIC_DC_BASE      (USB_DEV_BASE)
#define CONFIG_USB_AIC_DC_CLK       (CLK_USBD)
#define CONFIG_USB_AIC_DC_RESET     (RESET_USBD)
#define CONFIG_USB_AIC_DC_PHY_CLK   (CLK_USB_PHY0)
#define CONFIG_USB_AIC_DC_PHY_RESET (RESET_USBPHY0)
#define CONFIG_USB_AIC_DC_IRQ_NUM   (USB_DEV_IRQn)
#ifndef LPKG_CHERRYUSB_DEVICE_AIC_CPU
#define CONFIG_USB_AIC_DMA_ENABLE
#endif
#endif
//#define USBD_IRQHandler USBD_IRQHandler
//#define USB_BASE (0x40080000UL)
//#define USB_NUM_BIDIR_ENDPOINTS 4

/* ================ USB Host Port Configuration ==================*/

#define CONFIG_USBHOST_PIPE_NUM 10

/* ================ EHCI Configuration ================ */

#define CONFIG_USB_EHCI_HCCR_OFFSET     (0x0)
#define CONFIG_USB_EHCI_HCOR_OFFSET     (0x10)
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE 1024
// #define CONFIG_USB_EHCI_INFO_ENABLE
// #define CONFIG_USB_EHCI_HCOR_RESERVED_DISABLE
#define CONFIG_USB_EHCI_CONFIGFLAG
#define CONFIG_USB_EHCI_PORT_POWER
#define CONFIG_USB_EHCI_ISO
// #define CONFIG_USB_EHCI_PRINT_HW_PARAM

/* ================ OHCI Configuration ================ */
#define CONFIG_USB_OHCI_HCOR_OFFSET (0x400)


/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE (2048)
#endif

/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE (2048)
#endif

/* This parameter affects usb performance, and depends on (TCP_WND)tcp eceive windows size,
 * you can change to 2K ~ 16K and must be larger than TCP RX windows size in order to avoid being overflow.
 */
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE (2048)
#endif
/* Because lwip do not support multi pbuf at a time, so increasing this variable has no performance improvement */
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE (2048)
#endif


#endif
