/*
 * Copyright (c) 2022-2025, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usb_ehci_priv.h"
#if defined(LPKG_CHERRYUSB_HOST_OHCI)
#include "../ohci/usb_hc_ohci.h"
#endif
#ifdef RT_USING_PM
#include <rtdevice.h>

static int usbh_ehci_pm_init(struct usbh_bus *bus);
#endif

#define EHCI_TUNE_CERR    3 /* 0-3 qtd retries; 0 == don't stop */
#define EHCI_TUNE_RL_HS   4 /* nak throttle; see 4.9 */
#define EHCI_TUNE_RL_TT   0
#define EHCI_TUNE_MULT_HS 1 /* 1-3 transactions/uframe; 4.10.3 */
#define EHCI_TUNE_MULT_TT 1

struct ehci_hcd g_ehci_hcd[CONFIG_USBHOST_MAX_BUS];

USB_NOCACHE_RAM_SECTION struct ehci_qh_hw ehci_qh_pool[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_QH_NUM];
USB_NOCACHE_RAM_SECTION struct ehci_qtd_hw ehci_qtd_pool[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_QTD_NUM];

/* The head of the asynchronous queue */
USB_NOCACHE_RAM_SECTION struct ehci_qh_hw g_async_qh_head[CONFIG_USBHOST_MAX_BUS];
/* The head of the periodic queue */
USB_NOCACHE_RAM_SECTION struct ehci_qh_hw g_periodic_qh_head[CONFIG_USBHOST_MAX_BUS][EHCI_PERIOIDIC_QH_NUM];

/* The frame list */
USB_NOCACHE_RAM_SECTION uint32_t g_framelist[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_FRAME_LIST_SIZE] __attribute__((aligned(4096)));
#ifdef CONFIG_USB_DCACHE_ENABLE

#define EHCI_QH_HDR_ALIGN_SIZE  \
            ALIGN_UP(sizeof(struct ehci_qh), CONFIG_USB_ALIGN_SIZE)
#define EHCI_QTD_HDR_ALIGN_SIZE  \
            ALIGN_UP(sizeof(struct ehci_qtd), CONFIG_USB_ALIGN_SIZE)

#define CONFIG_USB_EHCI_BUF_NUM     3
static bool g_usb_ehci_buf_usbd[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_BUF_NUM];
USB_MEM_ALIGNX static uint8_t g_usb_ehci_buf[CONFIG_USBHOST_MAX_BUS][CONFIG_USB_EHCI_BUF_NUM][CONFIG_USB_EHCI_FRAME_LIST_SIZE];

void usb_dcache_clean(uintptr_t addr, uint32_t len);
void usb_dcache_invalidate(uintptr_t addr, uint32_t len);
void usb_dcache_clean_invalidate(uintptr_t addr, uint32_t len);

static int usb_ehci_buf_alloc(struct ehci_qtd_hw *qtd, uint32_t len)
{
    size_t flags;
    struct usbh_bus *bus = qtd->urb->hport->bus;

    if (len % CONFIG_USB_ALIGN_SIZE)
        qtd->align_buffer_len = ALIGN_UP(len, CONFIG_USB_ALIGN_SIZE);
    else
        qtd->align_buffer_len = len;

    if (qtd->align_buffer_len > CONFIG_USB_EHCI_FRAME_LIST_SIZE) {
        USB_LOG_DBG("Need alloc %d buf for cacheline algined\n",
                     (unsigned int)qtd->align_buffer_len);
        qtd->align_buffer = aicos_malloc_align(0, qtd->align_buffer_len,
                                               CONFIG_USB_ALIGN_SIZE);
        if (!qtd->align_buffer) {
            USB_LOG_ERR("alloc error.\r\n");
            return -5;
        }
    } else {
        for (int i = 0; i < CONFIG_USB_EHCI_BUF_NUM; i++) {
            if (!g_usb_ehci_buf_usbd[bus->hcd.hcd_id][i]) {
                flags = usb_osal_enter_critical_section();
                g_usb_ehci_buf_usbd[bus->hcd.hcd_id][i] = true;
                usb_osal_leave_critical_section(flags);
                qtd->align_buffer = g_usb_ehci_buf[bus->hcd.hcd_id][i];
                return 0;
            }
        }

        USB_LOG_ERR("USB EHCI alloc buffer failed. \n");
    }

    return 0;
}

static void usb_ehci_buf_free(struct ehci_qtd_hw *qtd)
{
    size_t flags;
    struct usbh_bus *bus = qtd->urb->hport->bus;

    if (!qtd->align_buffer)
        return;

    /* Whether the buf is allocated dynamically */
    for (int i = 0; i < CONFIG_USB_EHCI_BUF_NUM; i++) {
        if (qtd->align_buffer == g_usb_ehci_buf[bus->hcd.hcd_id][i]) {
            flags = usb_osal_enter_critical_section();
            g_usb_ehci_buf_usbd[bus->hcd.hcd_id][i] = false;
            usb_osal_leave_critical_section(flags);
            goto free;
        }
    }
    aicos_free_align(0, qtd->align_buffer);

free:
    qtd->align_buffer = NULL;
    qtd->align_buffer_len = 0;
}

static int usb_ehci_qtd_flush(struct ehci_qtd_hw *qtd)
{
    /* Flush the D-Cache, i.e., make the contents of the memory match the
    * contents of the D-Cache in the specified address range and invalidate
    * the D-Cache to force re-loading of the data from memory when next
    * accessed.
    */

    usb_dcache_clean((uintptr_t)&qtd->hw, EHCI_QTD_HDR_ALIGN_SIZE);

    return 0;
}

static int usb_ehci_qh_flush(struct ehci_qh_hw *qh)
{
    struct ehci_qtd_hw *qtd;

    qtd = EHCI_ADDR2QTD(qh->first_qtd);

    while (qtd) {
        usb_ehci_qtd_flush(qtd);
        qtd = EHCI_ADDR2QTD(qtd->hw.next_qtd);
    }

    usb_dcache_clean((uintptr_t)&qh->hw, EHCI_QH_HDR_ALIGN_SIZE);

    return 0;
}
#else
#define usb_dcache_clean(addr, len)
#define usb_dcache_invalidate(addr, len)
#define usb_dcache_clean_invalidate(addr, len)
#define usb_ehci_qtd_flush(qtd, bp, arg)
#define usb_ehci_qh_flush(qh)
#endif

static struct ehci_qh_hw *ehci_qh_alloc(struct usbh_bus *bus)
{
    struct ehci_qh_hw *qh;
    usb_osal_sem_t waitsem;
    size_t flags;

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QH_NUM; i++) {
        if (!g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[i]) {
            flags = usb_osal_enter_critical_section();
            g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[i] = true;
            usb_osal_leave_critical_section(flags);

            qh = &ehci_qh_pool[bus->hcd.hcd_id][i];
            waitsem = qh->waitsem;
            memset(qh, 0, sizeof(struct ehci_qh_hw));
            qh->hw.hlp = QTD_LIST_END;
            qh->hw.overlay.next_qtd = QTD_LIST_END;
            qh->hw.overlay.alt_next_qtd = QTD_LIST_END;
            qh->waitsem = waitsem;
            return qh;
        }
    }
    return NULL;
}

static void ehci_qh_free(struct usbh_bus *bus, struct ehci_qh_hw *qh)
{
    size_t flags;

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QH_NUM; i++) {
        if (&ehci_qh_pool[bus->hcd.hcd_id][i] == qh) {
            flags = usb_osal_enter_critical_section();
            g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[i] = false;
            usb_osal_leave_critical_section(flags);

            qh->urb = NULL;
            return;
        }
    }
}

static struct ehci_qtd_hw *ehci_qtd_alloc(struct usbh_bus *bus)
{
    struct ehci_qtd_hw *qtd;
    size_t flags;

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QTD_NUM; i++) {
        if (!g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[i]) {
            flags = usb_osal_enter_critical_section();
            g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[i] = true;
            usb_osal_leave_critical_section(flags);

            qtd = &ehci_qtd_pool[bus->hcd.hcd_id][i];
            memset(qtd, 0, sizeof(struct ehci_qtd_hw));
            qtd->hw.next_qtd = QTD_LIST_END;
            qtd->hw.alt_next_qtd = QTD_LIST_END;
            qtd->hw.token = QTD_TOKEN_STATUS_HALTED;
            return qtd;
        }
    }
    return NULL;
}

static void ehci_qtd_free(struct usbh_bus *bus, struct ehci_qtd_hw *qtd)
{
    size_t flags;

#ifdef CONFIG_USB_DCACHE_ENABLE
    usb_ehci_buf_free(qtd);
#endif

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QTD_NUM; i++) {
        if (&ehci_qtd_pool[bus->hcd.hcd_id][i] == qtd) {
            flags = usb_osal_enter_critical_section();
            g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[i] = false;
            usb_osal_leave_critical_section(flags);

            qtd->urb = NULL;
            return;
        }
    }
}

static inline void ehci_qh_add_head(struct ehci_qh_hw *head, struct ehci_qh_hw *n)
{
    n->hw.hlp = head->hw.hlp;
    usb_ehci_qh_flush(n);

    usb_dcache_invalidate((uintptr_t)&head->hw, EHCI_QH_HDR_ALIGN_SIZE);
    head->hw.hlp = QH_HLP_QH(n);
    usb_dcache_clean((uintptr_t)&head->hw, EHCI_QH_HDR_ALIGN_SIZE);
}

static inline void ehci_qh_remove(struct ehci_qh_hw *head, struct ehci_qh_hw *n)
{
    struct ehci_qh_hw *tmp = head;

    usb_dcache_invalidate((uintptr_t)&tmp->hw, EHCI_QH_HDR_ALIGN_SIZE);
    while (EHCI_ADDR2QH(tmp->hw.hlp) && EHCI_ADDR2QH(tmp->hw.hlp) != n) {
        tmp = EHCI_ADDR2QH(tmp->hw.hlp);
        usb_dcache_invalidate((uintptr_t)&tmp->hw, EHCI_QH_HDR_ALIGN_SIZE);
    }

    if (tmp) {
        tmp->hw.hlp = n->hw.hlp;
        usb_dcache_clean((uintptr_t)&tmp->hw, EHCI_QH_HDR_ALIGN_SIZE);
    }
}

static int ehci_caculate_smask(int binterval)
{
    int order, interval;

    interval = 1;
    while (binterval > 1) {
        interval *= 2;
        binterval--;
    }

    if (interval < 2) /* interval 1 */
        return 0xFF;
    if (interval < 4) /* interval 2 */
        return 0x55;
    if (interval < 8) /* interval 4 */
        return 0x22;
    for (order = 0; (interval > 1); order++) {
        interval >>= 1;
    }
    return (0x1 << (order % 8));
}

static struct ehci_qh_hw *ehci_get_periodic_qhead(struct usbh_bus *bus, uint8_t interval)
{
    interval /= 8;

    for (uint8_t i = 0; i < EHCI_PERIOIDIC_QH_NUM - 1; i++) {
        interval >>= 1;
        if (interval == 0) {
            return &g_periodic_qh_head[bus->hcd.hcd_id][i];
        }
    }
    return &g_periodic_qh_head[bus->hcd.hcd_id][EHCI_PERIOIDIC_QH_NUM - 1];
}

static void ehci_qh_fill(struct ehci_qh_hw *qh,
                         uint8_t dev_addr,
                         uint8_t ep_addr,
                         uint8_t ep_type,
                         uint16_t ep_mps,
                         uint8_t ep_mult,
                         uint8_t ep_interval,
                         uint8_t speed,
                         uint8_t hubaddr,
                         uint8_t hubport)
{
    uint32_t epchar = 0;
    uint32_t epcap = 0;

    /* QH endpoint characteristics:
     *
     * FIELD    DESCRIPTION
     * -------- -------------------------------
     * DEVADDR  Device address
     * I        Inactivate on Next Transaction
     * ENDPT    Endpoint number
     * EPS      Endpoint speed
     * DTC      Data toggle control
     * MAXPKT   Max packet size
     * C        Control endpoint
     * RL       NAK count reloaded
    */

    /* QH endpoint capabilities
     *
     * FIELD    DESCRIPTION
     * -------- -------------------------------
     * SSMASK   Interrupt Schedule Mask
     * SCMASK   Split Completion Mask
     * HUBADDR  Hub Address
     * PORT     Port number
     * MULT     High band width multiplier
     */

    epchar |= ((ep_addr & 0xf) << QH_EPCHAR_ENDPT_SHIFT);
    epchar |= (dev_addr << QH_EPCHAR_DEVADDR_SHIFT);
    epchar |= (ep_mps << QH_EPCHAR_MAXPKT_SHIFT);

    if (ep_type == USB_ENDPOINT_TYPE_CONTROL) {
        epchar |= QH_EPCHAR_DTC; /* toggle from qtd */
    }

    switch (speed) {
        case USB_SPEED_LOW:
            epchar |= QH_EPCHAR_EPS_LOW;
        case USB_SPEED_FULL:
            if (ep_type == USB_ENDPOINT_TYPE_CONTROL) {
                epchar |= QH_EPCHAR_C; /* for TT */
            }

            if (ep_type != USB_ENDPOINT_TYPE_INTERRUPT) {
                epchar |= (EHCI_TUNE_RL_TT << QH_EPCHAR_RL_SHIFT);
            }

            epcap |= QH_EPCAPS_MULT(EHCI_TUNE_MULT_TT);

            epcap |= QH_EPCAPS_HUBADDR(hubaddr);
            epcap |= QH_EPCAPS_PORT(hubport);

            if (ep_type == USB_ENDPOINT_TYPE_INTERRUPT) {
                epcap |= QH_EPCAPS_SSMASK(2);
                epcap |= QH_EPCAPS_SCMASK(0x78);
            }

            break;
        case USB_SPEED_HIGH:
            epchar |= QH_EPCHAR_EPS_HIGH;
            if (ep_type == USB_ENDPOINT_TYPE_CONTROL) {
                epchar |= (EHCI_TUNE_RL_HS << QH_EPCHAR_RL_SHIFT);

                epcap |= QH_EPCAPS_MULT(EHCI_TUNE_MULT_HS);
            } else if (ep_type == USB_ENDPOINT_TYPE_BULK) {
                epcap |= QH_EPCAPS_MULT(EHCI_TUNE_MULT_HS);
            } else {
                /* only for interrupt ep */
                epcap |= QH_EPCAPS_MULT(ep_mult);
                epcap |= ehci_caculate_smask(ep_interval);
            }
            break;

        default:
            break;
    }

    qh->hw.epchar = epchar;
    qh->hw.epcap = epcap;
}

static void ehci_qtd_bpl_fill(struct ehci_qtd_hw *qtd, uint32_t bufaddr, size_t buflen)
{
    uint32_t rest;

#ifdef CONFIG_USB_DCACHE_ENABLE
    if (((bufaddr % CONFIG_USB_ALIGN_SIZE) != 0) ||
        (((bufaddr + buflen) % CONFIG_USB_ALIGN_SIZE) != 0)) {
        int ret = usb_ehci_buf_alloc(qtd, buflen);
        if (ret)
            return;

        /* out direction */
        if (qtd->dir_in == 0)
            memcpy(qtd->align_buffer, (void *)(uintptr_t)bufaddr, buflen);

        qtd->buffer = (void *)(uintptr_t)bufaddr;
        qtd->buffer_len = buflen;
        bufaddr = (uint32_t)(uintptr_t)qtd->align_buffer;
        rest = qtd->align_buffer_len;
    } else {
        usb_ehci_buf_free(qtd);
        qtd->buffer = (void *)(uintptr_t)bufaddr;
        qtd->buffer_len = buflen;
        rest = buflen;
    }

    /* out direction */
    if (qtd->dir_in == 0)
        usb_dcache_clean((uintptr_t)bufaddr, rest);
    else
        usb_dcache_invalidate((uintptr_t)bufaddr, rest);
#endif

    qtd->hw.bpl[0] = bufaddr;
    rest = 0x1000 - (bufaddr & 0xfff);

    if (buflen < rest) {
        rest = buflen;
    } else {
        bufaddr += 0x1000;
        bufaddr &= ~0x0fff;

        for (int i = 1; rest < buflen && i < 5; i++) {
            qtd->hw.bpl[i] = bufaddr;
            bufaddr += 0x1000;

            if ((rest + 0x1000) < buflen) {
                rest += 0x1000;
            } else {
                rest = buflen;
            }
        }
    }
}

static void ehci_qtd_fill(struct ehci_qtd_hw *qtd, uint32_t bufaddr, size_t buflen, uint32_t token)
{
    /* qTD token
     *
     * FIELD    DESCRIPTION
     * -------- -------------------------------
     * STATUS   Status
     * PID      PID Code
     * CERR     Error Counter
     * CPAGE    Current Page
     * IOC      Interrupt on complete
     * NBYTES   Total Bytes to Transfer
     * TOGGLE   Data Toggle
     */

    qtd->hw.token = token;

    ehci_qtd_bpl_fill(qtd, bufaddr, buflen);
    qtd->total_len = buflen;
}

static struct ehci_qh_hw *ehci_control_urb_init(struct usbh_bus *bus, struct usbh_urb *urb, struct usb_setup_packet *setup, uint8_t *buffer, uint32_t buflen)
{
    struct ehci_qh_hw *qh = NULL;
    struct ehci_qtd_hw *qtd_setup = NULL;
    struct ehci_qtd_hw *qtd_data = NULL;
    struct ehci_qtd_hw *qtd_status = NULL;
    uint32_t token;
    size_t flags;

    qh = ehci_qh_alloc(bus);
    if (qh == NULL) {
        return NULL;
    }

    qtd_setup = ehci_qtd_alloc(bus);
    if (buflen > 0) {
        qtd_data = ehci_qtd_alloc(bus);
    }

    qtd_status = ehci_qtd_alloc(bus);
    if (qtd_status == NULL) {
        ehci_qh_free(bus, qh);
        if (qtd_setup) {
            ehci_qtd_free(bus, qtd_setup);
        }
        if (qtd_data) {
            ehci_qtd_free(bus, qtd_data);
        }
        return NULL;
    }

    ehci_qh_fill(qh,
                 urb->hport->dev_addr,
                 urb->ep->bEndpointAddress,
                 USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes),
                 USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize),
                 0,
                 0,
                 urb->hport->speed,
                 urb->hport->parent->hub_addr,
                 urb->hport->port);

    /* fill setup qtd */
    token = QTD_TOKEN_STATUS_ACTIVE |
            QTD_TOKEN_PID_SETUP |
            ((uint32_t)EHCI_TUNE_CERR << QTD_TOKEN_CERR_SHIFT) |
            ((uint32_t)8 << QTD_TOKEN_NBYTES_SHIFT);
    #ifdef CONFIG_USB_DCACHE_ENABLE
    qtd_setup->dir_in = 0;
    #endif
    qtd_setup->urb = urb;
    ehci_qtd_fill(qtd_setup, (uintptr_t)setup, 8, token);

    /* fill data qtd */
    if (setup->wLength > 0) {
        if ((setup->bmRequestType & 0x80) == 0x80) {
            token = QTD_TOKEN_PID_IN;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd_data->dir_in = 1;
            #endif
        } else {
            token = QTD_TOKEN_PID_OUT;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd_data->dir_in = 0;
            #endif
        }
        token |= QTD_TOKEN_STATUS_ACTIVE |
                 QTD_TOKEN_PID_OUT |
                 QTD_TOKEN_TOGGLE |
                 ((uint32_t)EHCI_TUNE_CERR << QTD_TOKEN_CERR_SHIFT) |
                 ((uint32_t)buflen << QTD_TOKEN_NBYTES_SHIFT);

        qtd_data->urb = urb;
        ehci_qtd_fill(qtd_data, (uintptr_t)buffer, buflen, token);
        qtd_setup->hw.next_qtd = EHCI_PTR2ADDR(qtd_data);
        qtd_data->hw.next_qtd = EHCI_PTR2ADDR(qtd_status);
    } else {
        qtd_setup->hw.next_qtd = EHCI_PTR2ADDR(qtd_status);
    }

    /* fill status qtd */
    if ((setup->bmRequestType & 0x80) == 0x80) {
        token = QTD_TOKEN_PID_OUT;
        #ifdef CONFIG_USB_DCACHE_ENABLE
        qtd_status->dir_in = 0;
        #endif
    } else {
        token = QTD_TOKEN_PID_IN;
        #ifdef CONFIG_USB_DCACHE_ENABLE
        qtd_status->dir_in = 1;
        #endif
    }
    token |= QTD_TOKEN_STATUS_ACTIVE |
             QTD_TOKEN_TOGGLE |
             QTD_TOKEN_IOC |
             ((uint32_t)EHCI_TUNE_CERR << QTD_TOKEN_CERR_SHIFT) |
             ((uint32_t)0 << QTD_TOKEN_NBYTES_SHIFT);

    qtd_status->urb = urb;
    ehci_qtd_fill(qtd_status, 0, 0, token);
    qtd_status->hw.next_qtd = QTD_LIST_END;

    /* update qh first qtd */
    qh->hw.curr_qtd = EHCI_PTR2ADDR(qtd_setup);
    qh->hw.overlay.next_qtd = EHCI_PTR2ADDR(qtd_setup);

    /* record qh first qtd */
    qh->first_qtd = EHCI_PTR2ADDR(qtd_setup);

    flags = usb_osal_enter_critical_section();

    qh->urb = urb;
    urb->hcpriv = qh;
    /* add qh into async list */
    ehci_qh_add_head(&g_async_qh_head[bus->hcd.hcd_id], qh);

    EHCI_HCOR->usbcmd |= EHCI_USBCMD_ASEN;

    usb_osal_leave_critical_section(flags);
    return qh;
}

static struct ehci_qh_hw *ehci_bulk_urb_init(struct usbh_bus *bus, struct usbh_urb *urb, uint8_t *buffer, uint32_t buflen)
{
    struct ehci_qh_hw *qh = NULL;
    struct ehci_qtd_hw *qtd = NULL;
    struct ehci_qtd_hw *first_qtd = NULL;
    struct ehci_qtd_hw *prev_qtd = NULL;
    uint32_t qtd_num = 0;
    uint32_t xfer_len = 0;
    uint32_t token;
    size_t flags;

    qh = ehci_qh_alloc(bus);
    if (qh == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QTD_NUM; i++) {
        if (!g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[i]) {
            qtd_num++;
        }
    }

    if (qtd_num < ((buflen + 0x3fff) / 0x4000)) {
        ehci_qh_free(bus, qh);
        return NULL;
    }

    ehci_qh_fill(qh,
                 urb->hport->dev_addr,
                 urb->ep->bEndpointAddress,
                 USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes),
                 USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize),
                 0,
                 0,
                 urb->hport->speed,
                 urb->hport->parent->hub_addr,
                 urb->hport->port);

    while (buflen >= 0) {
        qtd = ehci_qtd_alloc(bus);

        if (buflen > 0x4000) {
            xfer_len = 0x4000;
            buflen -= 0x4000;
        } else {
            xfer_len = buflen;
            buflen = 0;
        }

        if (urb->ep->bEndpointAddress & 0x80) {
            token = QTD_TOKEN_PID_IN;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd->dir_in = 1;
            #endif
        } else {
            token = QTD_TOKEN_PID_OUT;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd->dir_in = 0;
            #endif
        }

        token |= QTD_TOKEN_STATUS_ACTIVE |
                 ((uint32_t)EHCI_TUNE_CERR << QTD_TOKEN_CERR_SHIFT) |
                 ((uint32_t)xfer_len << QTD_TOKEN_NBYTES_SHIFT);

        if (buflen == 0) {
            token |= QTD_TOKEN_IOC;
        }

        qtd->urb = urb;
        ehci_qtd_fill(qtd, (uintptr_t)buffer, xfer_len, token);
        qtd->hw.next_qtd = QTD_LIST_END;
        buffer += xfer_len;

        if (prev_qtd) {
            prev_qtd->hw.next_qtd = EHCI_PTR2ADDR(qtd);
        } else {
            first_qtd = qtd;
        }
        prev_qtd = qtd;

        if (buflen == 0) {
            break;
        }
    }

    /* update qh first qtd */
    qh->hw.curr_qtd = EHCI_PTR2ADDR(first_qtd);
    qh->hw.overlay.next_qtd = EHCI_PTR2ADDR(first_qtd);

    /* update data toggle */
    if (urb->data_toggle) {
        qh->hw.overlay.token = QTD_TOKEN_TOGGLE;
    } else {
        qh->hw.overlay.token = 0;
    }

    /* record qh first qtd */
    qh->first_qtd = EHCI_PTR2ADDR(first_qtd);

    flags = usb_osal_enter_critical_section();

    qh->urb = urb;
    urb->hcpriv = qh;
    /* add qh into async list */
    ehci_qh_add_head(&g_async_qh_head[bus->hcd.hcd_id], qh);

    EHCI_HCOR->usbcmd |= EHCI_USBCMD_ASEN;

    usb_osal_leave_critical_section(flags);
    return qh;
}

static struct ehci_qh_hw *ehci_intr_urb_init(struct usbh_bus *bus, struct usbh_urb *urb, uint8_t *buffer, uint32_t buflen)
{
    struct ehci_qh_hw *qh = NULL;
    struct ehci_qtd_hw *qtd = NULL;
    struct ehci_qtd_hw *first_qtd = NULL;
    struct ehci_qtd_hw *prev_qtd = NULL;
    uint32_t qtd_num = 0;
    uint32_t xfer_len = 0;
    uint32_t token;
    size_t flags;

    qh = ehci_qh_alloc(bus);
    if (qh == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QTD_NUM; i++) {
        if (!g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[i]) {
            qtd_num++;
        }
    }

    if (qtd_num < ((buflen + 0x3fff) / 0x4000)) {
        ehci_qh_free(bus, qh);
        return NULL;
    }

    ehci_qh_fill(qh,
                 urb->hport->dev_addr,
                 urb->ep->bEndpointAddress,
                 USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes),
                 USB_GET_MAXPACKETSIZE(urb->ep->wMaxPacketSize),
                 USB_GET_MULT(urb->ep->wMaxPacketSize) + 1,
                 urb->ep->bInterval,
                 urb->hport->speed,
                 urb->hport->parent->hub_addr,
                 urb->hport->port);

    while (buflen >= 0) {
        qtd = ehci_qtd_alloc(bus);

        if (buflen > 0x4000) {
            xfer_len = 0x4000;
            buflen -= 0x4000;
        } else {
            xfer_len = buflen;
            buflen = 0;
        }

        if (urb->ep->bEndpointAddress & 0x80) {
            token = QTD_TOKEN_PID_IN;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd->dir_in = 1;
            #endif
        } else {
            token = QTD_TOKEN_PID_OUT;
            #ifdef CONFIG_USB_DCACHE_ENABLE
            qtd->dir_in = 0;
            #endif
        }

        token |= QTD_TOKEN_STATUS_ACTIVE |
                 ((uint32_t)EHCI_TUNE_CERR << QTD_TOKEN_CERR_SHIFT) |
                 ((uint32_t)xfer_len << QTD_TOKEN_NBYTES_SHIFT);

        if (buflen == 0) {
            token |= QTD_TOKEN_IOC;
        }

        qtd->urb = urb;
        ehci_qtd_fill(qtd, (uintptr_t)buffer, xfer_len, token);
        qtd->hw.next_qtd = QTD_LIST_END;
        buffer += xfer_len;

        if (prev_qtd) {
            prev_qtd->hw.next_qtd = EHCI_PTR2ADDR(qtd);
        } else {
            first_qtd = qtd;
        }
        prev_qtd = qtd;

        if (buflen == 0) {
            break;
        }
    }

    /* update qh first qtd */
    qh->hw.curr_qtd = EHCI_PTR2ADDR(first_qtd);
    qh->hw.overlay.next_qtd = EHCI_PTR2ADDR(first_qtd);

    /* update data toggle */
    if (urb->data_toggle) {
        qh->hw.overlay.token = QTD_TOKEN_TOGGLE;
    } else {
        qh->hw.overlay.token = 0;
    }

    /* record qh first qtd */
    qh->first_qtd = EHCI_PTR2ADDR(first_qtd);

    flags = usb_osal_enter_critical_section();

    qh->urb = urb;
    urb->hcpriv = qh;
    /* add qh into periodic list */
    if (urb->hport->speed == USB_SPEED_HIGH) {
        ehci_qh_add_head(ehci_get_periodic_qhead(bus, urb->ep->bInterval), qh);
    } else {
        ehci_qh_add_head(ehci_get_periodic_qhead(bus, urb->ep->bInterval * 8), qh);
    }

    EHCI_HCOR->usbcmd |= EHCI_USBCMD_PSEN;

    usb_osal_leave_critical_section(flags);
    return qh;
}

static void ehci_urb_waitup(struct usbh_bus *bus, struct usbh_urb *urb)
{
    struct ehci_qh_hw *qh;

    qh = (struct ehci_qh_hw *)urb->hcpriv;
    qh->urb = NULL;
    urb->hcpriv = NULL;

    qh->remove_in_iaad = 0;

    if (urb->timeout) {
        usb_osal_sem_give(qh->waitsem);
    } else {
        ehci_qh_free(bus, qh);
    }

    if (urb->complete) {
        if (urb->errorcode < 0) {
            urb->complete(urb->arg, urb->errorcode);
        } else {
            urb->complete(urb->arg, urb->actual_length);
        }
    }
}

static void ehci_qh_scan_qtds(struct usbh_bus *bus, struct ehci_qh_hw *qhead, struct ehci_qh_hw *qh)
{
    struct ehci_qtd_hw *qtd;

    ehci_qh_remove(qhead, qh);

    qtd = EHCI_ADDR2QTD(qh->first_qtd);

    while (qtd) {
        qtd->urb->actual_length += (qtd->total_len - ((qtd->hw.token & QTD_TOKEN_NBYTES_MASK) >> QTD_TOKEN_NBYTES_SHIFT));

        #ifdef CONFIG_USB_DCACHE_ENABLE
        if (qtd->dir_in == 1)
            aicos_dma_sync();
        if ((qtd->align_buffer) && (qtd->dir_in == 1)) {
            memcpy(qtd->buffer, qtd->align_buffer, qtd->buffer_len);
        }
        #endif

        ehci_qtd_free(bus, qtd);
        qh->first_qtd = qtd->hw.next_qtd;
        qtd = EHCI_ADDR2QTD(qh->first_qtd);
    }
}

static void ehci_check_qh(struct usbh_bus *bus, struct ehci_qh_hw *qhead, struct ehci_qh_hw *qh)
{
    struct usbh_urb *urb;
    struct ehci_qtd_hw *qtd;
    uint32_t token;

    usb_dcache_invalidate((uintptr_t)&qh->hw, EHCI_QH_HDR_ALIGN_SIZE);

    qtd = EHCI_ADDR2QTD(qh->first_qtd);

    if (qtd == NULL) {
        return;
    }

    while (qtd) {
        usb_dcache_invalidate((uintptr_t)&qtd->hw,
                                       EHCI_QTD_HDR_ALIGN_SIZE);

        token = qtd->hw.token;

        if (token & (QTD_TOKEN_STATUS_HALTED || QTD_TOKEN_STATUS_BABBLE)) {
            break;
        } else if (token & QTD_TOKEN_STATUS_ACTIVE) {
            return;
        }

        qtd = EHCI_ADDR2QTD(qtd->hw.next_qtd);
    }

    urb = qh->urb;

    if (token & QTD_TOKEN_STATUS_HALTED) {
        USB_LOG_ERR("QTD_TOKEN_STATUS_HALTED, token(0x%08x)\n", (unsigned int)token);
        urb->errorcode = -USB_ERR_STALL;
        urb->data_toggle = 0;
    } else if (token & QTD_TOKEN_STATUS_BABBLE) {
        USB_LOG_ERR("QTD_TOKEN_STATUS_BABBLE, token(0x%08x)\n", (unsigned int)token);
        urb->errorcode = -USB_ERR_BABBLE;
        urb->data_toggle = 0;
    } else {
        /* Data Buffer Error and XactErr should not interrupt transmit */
        if (token & QTD_TOKEN_STATUS_DBERR) {
            /* Report Data Buffer Error: non-fatal but useful */
            USB_LOG_DBG("QTD_TOKEN_STATUS_DBERR, token(0x%08x)\n", (unsigned int)token);
        } else if (token & QTD_TOKEN_STATUS_XACTERR) {
            /* Ignore the XactErr if token is not in STALL */
            USB_LOG_DBG("QTD_TOKEN_STATUS_XACTERR, token(0x%08x)\n", (unsigned int)token);
        }

        if (qh->hw.overlay.token & QTD_TOKEN_TOGGLE) {
            urb->data_toggle = true;
        } else {
            urb->data_toggle = false;
        }
        urb->errorcode = 0;
    }

    ehci_qh_scan_qtds(bus, qhead, qh);

    if (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) == USB_ENDPOINT_TYPE_INTERRUPT) {
        ehci_urb_waitup(bus, urb);
    } else {
        qh->remove_in_iaad = 1;

        EHCI_HCOR->usbcmd |= EHCI_USBCMD_IAAD;
    }
}

static void ehci_kill_qh(struct usbh_bus *bus, struct ehci_qh_hw *qhead, struct ehci_qh_hw *qh)
{
    struct ehci_qtd_hw *qtd;

    ehci_qh_remove(qhead, qh);

    qtd = EHCI_ADDR2QTD(qh->first_qtd);

    while (qtd) {
        ehci_qtd_free(bus, qtd);
        qh->first_qtd = qtd->hw.next_qtd;
        qtd = EHCI_ADDR2QTD(qh->first_qtd);
    }
}

static int usbh_reset_port(struct usbh_bus *bus, const uint8_t port)
{
    volatile uint32_t timeout = 0;
    uint32_t regval;

#if defined(CONFIG_USB_EHCI_HPMICRO) && CONFIG_USB_EHCI_HPMICRO
    if ((*(volatile uint32_t *)(bus->hcd.reg_base + 0x224) & 0xc0) == (2 << 6)) { /* Hardcode for hpm */
        EHCI_HCOR->portsc[port - 1] |= (1 << 29);
    } else {
        EHCI_HCOR->portsc[port - 1] &= ~(1 << 29);
    }
#endif
    regval = EHCI_HCOR->portsc[port - 1];
    regval &= ~EHCI_PORTSC_PE;
    regval |= EHCI_PORTSC_RESET;
    EHCI_HCOR->portsc[port - 1] = regval;
    usb_osal_msleep(55);

    EHCI_HCOR->portsc[port - 1] &= ~EHCI_PORTSC_RESET;
    do {
        usb_osal_msleep(1);
        timeout++;
        if (timeout > 100) {
            return -USB_ERR_TIMEOUT;
        }
    } while ((EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_RESET) != 0);

    return 0;
}

__WEAK void usb_hc_low_level_init(struct usbh_bus *bus)
{
}

__WEAK void usb_hc_low_level2_init(struct usbh_bus *bus)
{
}

__WEAK void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
}

int usb_hc_init(struct usbh_bus *bus)
{
    uint32_t interval;
    struct ehci_qh_hw *qh;

    volatile uint32_t timeout = 0;
    uint32_t regval;

    memset(&g_ehci_hcd[bus->hcd.hcd_id], 0, sizeof(struct ehci_hcd));
    memset(&ehci_qh_pool[bus->hcd.hcd_id], 0, sizeof(struct ehci_qh_hw) * CONFIG_USB_EHCI_QH_NUM);

    if (sizeof(struct ehci_qh_hw) % 32) {
        USB_LOG_ERR("struct ehci_qh_hw is not align 32\r\n");
        return -USB_ERR_INVAL;
    }
    if (sizeof(struct ehci_qtd_hw) % 32) {
        USB_LOG_ERR("struct ehci_qtd_hw is not align 32\r\n");
        return -USB_ERR_INVAL;
    }

    for (uint8_t index = 0; index < CONFIG_USB_EHCI_QH_NUM; index++) {
        qh = &ehci_qh_pool[bus->hcd.hcd_id][index];
        qh->waitsem = usb_osal_sem_create(0);
    }

    memset(&g_async_qh_head[bus->hcd.hcd_id], 0, sizeof(struct ehci_qh_hw));
    g_async_qh_head[bus->hcd.hcd_id].hw.hlp = QH_HLP_QH(&g_async_qh_head[bus->hcd.hcd_id]);
    g_async_qh_head[bus->hcd.hcd_id].hw.epchar = QH_EPCHAR_H;
    g_async_qh_head[bus->hcd.hcd_id].hw.overlay.next_qtd = QTD_LIST_END;
    g_async_qh_head[bus->hcd.hcd_id].hw.overlay.alt_next_qtd = QTD_LIST_END;
    g_async_qh_head[bus->hcd.hcd_id].hw.overlay.token = QTD_TOKEN_STATUS_HALTED;
    g_async_qh_head[bus->hcd.hcd_id].first_qtd = QTD_LIST_END;

    usb_dcache_clean((uintptr_t)&g_async_qh_head[bus->hcd.hcd_id].hw,
                          EHCI_QH_HDR_ALIGN_SIZE);

    memset(g_framelist[bus->hcd.hcd_id], 0, sizeof(uint32_t) * CONFIG_USB_EHCI_FRAME_LIST_SIZE);

    for (int i = EHCI_PERIOIDIC_QH_NUM - 1; i >= 0; i--) {
        memset(&g_periodic_qh_head[bus->hcd.hcd_id][i], 0, sizeof(struct ehci_qh_hw));
        g_periodic_qh_head[bus->hcd.hcd_id][i].hw.hlp = QH_HLP_END;
        g_periodic_qh_head[bus->hcd.hcd_id][i].hw.epchar = QH_EPCAPS_SSMASK(1);
        g_periodic_qh_head[bus->hcd.hcd_id][i].hw.overlay.next_qtd = QTD_LIST_END;
        g_periodic_qh_head[bus->hcd.hcd_id][i].hw.overlay.alt_next_qtd = QTD_LIST_END;
        g_periodic_qh_head[bus->hcd.hcd_id][i].hw.overlay.token = QTD_TOKEN_STATUS_HALTED;
        g_periodic_qh_head[bus->hcd.hcd_id][i].first_qtd = QTD_LIST_END;

        usb_dcache_clean((uintptr_t)&g_periodic_qh_head[bus->hcd.hcd_id][i].hw,
                              EHCI_QH_HDR_ALIGN_SIZE);

        interval = 1 << i;
        for (uint32_t j = interval - 1; j < CONFIG_USB_EHCI_FRAME_LIST_SIZE; j += interval) {
            if (g_framelist[bus->hcd.hcd_id][j] == 0) {
                g_framelist[bus->hcd.hcd_id][j] = QH_HLP_QH(&g_periodic_qh_head[bus->hcd.hcd_id][i]);
            } else {
                qh = EHCI_ADDR2QH(g_framelist[bus->hcd.hcd_id][j]);
                while (1) {
                    if (qh == &g_periodic_qh_head[bus->hcd.hcd_id][i]) {
                        break;
                    }
                    if (qh->hw.hlp == QH_HLP_END) {
                        qh->hw.hlp = QH_HLP_QH(&g_periodic_qh_head[bus->hcd.hcd_id][i]);
                        break;
                    }

                    qh = EHCI_ADDR2QH(qh->hw.hlp);
                }
            }
        }
    }

    usb_dcache_clean((uintptr_t)g_framelist[bus->hcd.hcd_id], CONFIG_USB_EHCI_FRAME_LIST_SIZE * sizeof(uint32_t));

    usb_hc_low_level_init(bus);

    USB_LOG_INFO("EHCI(%d) HCIVERSION:0x%04x\r\n", bus->hcd.hcd_id, (unsigned int)EHCI_HCCR->hciversion);
    USB_LOG_INFO("EHCI(%d) HCSPARAMS:0x%06x\r\n", bus->hcd.hcd_id, (unsigned int)EHCI_HCCR->hcsparams);
    USB_LOG_INFO("EHCI(%d) HCCPARAMS:0x%04x\r\n", bus->hcd.hcd_id, (unsigned int)EHCI_HCCR->hccparams);

    g_ehci_hcd[bus->hcd.hcd_id].ppc = (EHCI_HCCR->hcsparams & EHCI_HCSPARAMS_PPC) ? true : false;
    g_ehci_hcd[bus->hcd.hcd_id].n_ports = (EHCI_HCCR->hcsparams & EHCI_HCSPARAMS_NPORTS_MASK) >> EHCI_HCSPARAMS_NPORTS_SHIFT;
    g_ehci_hcd[bus->hcd.hcd_id].n_cc = (EHCI_HCCR->hcsparams & EHCI_HCSPARAMS_NCC_MASK) >> EHCI_HCSPARAMS_NCC_SHIFT;
    g_ehci_hcd[bus->hcd.hcd_id].n_pcc = (EHCI_HCCR->hcsparams & EHCI_HCSPARAMS_NPCC_MASK) >> EHCI_HCSPARAMS_NPCC_SHIFT;
    g_ehci_hcd[bus->hcd.hcd_id].has_tt = g_ehci_hcd[bus->hcd.hcd_id].n_cc ? false : true;
    g_ehci_hcd[bus->hcd.hcd_id].hcor_offset = EHCI_HCCR->caplength;

    USB_LOG_INFO("EHCI ppc:%u, n_ports:%u, n_cc:%u, n_pcc:%u\r\n",
                 g_ehci_hcd[bus->hcd.hcd_id].ppc,
                 g_ehci_hcd[bus->hcd.hcd_id].n_ports,
                 g_ehci_hcd[bus->hcd.hcd_id].n_cc,
                 g_ehci_hcd[bus->hcd.hcd_id].n_pcc);

    EHCI_HCOR->usbcmd &= ~EHCI_USBCMD_RUN;
    usb_osal_msleep(2);
    EHCI_HCOR->usbcmd |= EHCI_USBCMD_HCRESET;
    while (EHCI_HCOR->usbcmd & EHCI_USBCMD_HCRESET) {
        usb_osal_msleep(1);
        timeout++;
        if (timeout > 100) {
            return -USB_ERR_TIMEOUT;
        }
    }

    usb_hc_low_level2_init(bus);

    EHCI_HCOR->usbintr = 0;
    EHCI_HCOR->usbsts = EHCI_HCOR->usbsts;
#ifdef CONFIG_USB_EHCI_PRINT_HW_PARAM
    USB_LOG_INFO("EHCI HCIVERSION:%04x\r\n", (int)EHCI_HCCR->hciversion);
    USB_LOG_INFO("EHCI HCSPARAMS:%06x\r\n", (int)EHCI_HCCR->hcsparams);
    USB_LOG_INFO("EHCI HCCPARAMS:%04x\r\n", (int)EHCI_HCCR->hccparams);
#endif
    /* Set the Current Asynchronous List Address. */
    EHCI_HCOR->asynclistaddr = EHCI_PTR2ADDR(&g_async_qh_head[bus->hcd.hcd_id]);
    /* Set the Periodic Frame List Base Address. */
    EHCI_HCOR->periodiclistbase = EHCI_PTR2ADDR(g_framelist[bus->hcd.hcd_id]);

    regval = 0;
#if CONFIG_USB_EHCI_FRAME_LIST_SIZE == 1024
    regval |= EHCI_USBCMD_FLSIZE_1024;
#elif CONFIG_USB_EHCI_FRAME_LIST_SIZE == 512
    regval |= EHCI_USBCMD_FLSIZE_512;
#elif CONFIG_USB_EHCI_FRAME_LIST_SIZE == 256
    regval |= EHCI_USBCMD_FLSIZE_256;
#else
#error Unsupported frame size list size
#endif

    regval |= EHCI_USBCMD_ITHRE_1MF;
    regval |= EHCI_USBCMD_ASEN;
    regval |= EHCI_USBCMD_PSEN;
    regval |= EHCI_USBCMD_RUN;
    EHCI_HCOR->usbcmd = regval;

#ifdef CONFIG_USB_EHCI_CONFIGFLAG
    EHCI_HCOR->configflag = EHCI_CONFIGFLAG;
#endif
    /* Wait for the EHCI to run (no longer report halted) */
    timeout = 0;
    while (EHCI_HCOR->usbsts & EHCI_USBSTS_HALTED) {
        usb_osal_msleep(1);
        timeout++;
        if (timeout > 100) {
            return -USB_ERR_TIMEOUT;
        }
    }

    if (g_ehci_hcd[bus->hcd.hcd_id].ppc) {
        for (uint8_t port = 0; port < g_ehci_hcd[bus->hcd.hcd_id].n_ports; port++) {
            regval = EHCI_HCOR->portsc[port];
            regval |= EHCI_PORTSC_PP;
            regval &= ~(EHCI_PORTSC_CSC | EHCI_PORTSC_PEC | EHCI_PORTSC_OCC);
            EHCI_HCOR->portsc[port] = regval;
        }
    }

    if (g_ehci_hcd[bus->hcd.hcd_id].has_tt) {
#if defined(LPKG_CHERRYUSB_HOST_OHCI)
        USB_LOG_INFO("EHCI uses tt for ls/fs device, so cannot enable this macro\r\n");
        return -USB_ERR_INVAL;
#endif
    }

    if (g_ehci_hcd[bus->hcd.hcd_id].has_tt) {
        USB_LOG_INFO("EHCI uses tt for ls/fs device\r\n");
    } else {
#if defined(LPKG_CHERRYUSB_HOST_OHCI)
        USB_LOG_INFO("EHCI uses companion controller for ls/fs device\r\n");
        ohci_init(bus);
#else
        USB_LOG_WRN("Do not enable companion controller, you should use a hub to support ls/fs device\r\n");
#endif
    }

    /* Enable EHCI interrupts. */
    EHCI_HCOR->usbintr = EHCI_USBIE_INT | EHCI_USBIE_ERR | EHCI_USBIE_PCD | EHCI_USBIE_FATAL | EHCI_USBIE_IAA;
#ifdef RT_USING_PM
    usbh_ehci_pm_init(bus);
#endif
    return 0;
}

int usb_hc_deinit(struct usbh_bus *bus)
{
    struct ehci_qh_hw *qh;

    volatile uint32_t timeout = 0;
    uint32_t regval;

    EHCI_HCOR->usbintr = 0;

    regval = EHCI_HCOR->usbcmd;
    regval &= ~EHCI_USBCMD_ASEN;
    regval &= ~EHCI_USBCMD_PSEN;
    regval &= ~EHCI_USBCMD_RUN;
    EHCI_HCOR->usbcmd = regval;

    while ((EHCI_HCOR->usbsts & (EHCI_USBSTS_PSS | EHCI_USBSTS_ASS)) || ((EHCI_HCOR->usbsts & EHCI_USBSTS_HALTED) == 0)) {
        usb_osal_msleep(1);
        timeout++;
        if (timeout > 2000) {
            USB_LOG_WRN("Wait for the EHCI to stop timeout.\n");
        }
    }

    if (g_ehci_hcd[bus->hcd.hcd_id].ppc) {
        for (uint8_t port = 0; port < g_ehci_hcd[bus->hcd.hcd_id].n_ports; port++) {
            regval = EHCI_HCOR->portsc[port];
            regval &= ~EHCI_PORTSC_PP;
            EHCI_HCOR->portsc[port] = regval;
        }
    }

#ifdef CONFIG_USB_EHCI_CONFIGFLAG
    EHCI_HCOR->configflag = 0;
#endif

    EHCI_HCOR->usbsts = EHCI_HCOR->usbsts;
    EHCI_HCOR->usbcmd |= EHCI_USBCMD_HCRESET;

    for (uint8_t index = 0; index < CONFIG_USB_EHCI_QH_NUM; index++) {
        qh = &ehci_qh_pool[bus->hcd.hcd_id][index];
        usb_osal_sem_delete(qh->waitsem);
    }

#if defined(LPKG_CHERRYUSB_HOST_OHCI)
    ohci_deinit(bus);
#endif

    usb_hc_low_level_deinit(bus);

    return 0;
}

uint16_t usbh_get_frame_number(struct usbh_bus *bus)
{
#if defined(LPKG_CHERRYUSB_HOST_OHCI)
    if (EHCI_HCOR->portsc[0] & EHCI_PORTSC_OWNER) {
        return ohci_get_frame_number(bus);
    }
#endif

    return (((EHCI_HCOR->frindex & EHCI_FRINDEX_MASK) >> 3) & 0x3ff);
}

int usbh_roothub_control(struct usbh_bus *bus, struct usb_setup_packet *setup, uint8_t *buf)
{
    uint8_t nports;
    uint8_t port;
    uint32_t temp, status;

    nports = g_ehci_hcd[bus->hcd.hcd_id].n_ports;

    port = setup->wIndex;

    temp = EHCI_HCOR->portsc[port - 1];

#if defined(LPKG_CHERRYUSB_HOST_OHCI)
    if (temp & EHCI_PORTSC_OWNER) {
        return ohci_roothub_control(bus, setup, buf);
    }

    if ((temp & EHCI_PORTSC_LSTATUS_MASK) == EHCI_PORTSC_LSTATUS_KSTATE) {
        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_OWNER;

        while (!(EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_OWNER)) {
        }
        USB_LOG_INFO("Switch port %u to OHCI\r\n", port);
        return ohci_roothub_control(bus, setup, buf);
    }
#endif
    if (setup->bmRequestType & USB_REQUEST_RECIPIENT_DEVICE) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -USB_ERR_NOTSUPP;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                switch (setup->wValue) {
                    case HUB_FEATURE_HUB_C_LOCALPOWER:
                        break;
                    case HUB_FEATURE_HUB_C_OVERCURRENT:
                        break;
                    default:
                        return -USB_ERR_NOTSUPP;
                }
                break;
            case HUB_REQUEST_GET_DESCRIPTOR:
                break;
            case HUB_REQUEST_GET_STATUS:
                memset(buf, 0, 4);
                break;
            default:
                break;
        }
    } else if (setup->bmRequestType & USB_REQUEST_RECIPIENT_OTHER) {
        switch (setup->bRequest) {
            case HUB_REQUEST_CLEAR_FEATURE:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_ENABLE:
                        EHCI_HCOR->portsc[port - 1] &= ~EHCI_PORTSC_PE;
                        break;
                    case HUB_PORT_FEATURE_SUSPEND:
                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_RESUME;
                        usb_osal_msleep(20);
                        EHCI_HCOR->portsc[port - 1] &= ~EHCI_PORTSC_RESUME;
                        while (EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_RESUME) {
                        }

                        temp = EHCI_HCOR->usbcmd;
                        temp |= EHCI_USBCMD_ASEN;
                        temp |= EHCI_USBCMD_PSEN;
                        temp |= EHCI_USBCMD_RUN;
                        EHCI_HCOR->usbcmd = temp;

                        while ((EHCI_HCOR->usbcmd & EHCI_USBCMD_RUN) == 0) {
                        }

                    case HUB_PORT_FEATURE_C_SUSPEND:
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        EHCI_HCOR->portsc[port - 1] &= ~EHCI_PORTSC_PP;
                        break;
                    case HUB_PORT_FEATURE_C_CONNECTION:
                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_CSC;
                        break;
                    case HUB_PORT_FEATURE_C_ENABLE:
                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_PEC;
                        break;
                    case HUB_PORT_FEATURE_C_OVER_CURREN:
                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_OCC;
                        break;
                    case HUB_PORT_FEATURE_C_RESET:
                        break;
                    default:
                        return -USB_ERR_NOTSUPP;
                }
                break;
            case HUB_REQUEST_SET_FEATURE:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }

                switch (setup->wValue) {
                    case HUB_PORT_FEATURE_SUSPEND:
                        temp = EHCI_HCOR->usbcmd;
                        temp &= ~EHCI_USBCMD_ASEN;
                        temp &= ~EHCI_USBCMD_PSEN;
                        temp &= ~EHCI_USBCMD_RUN;
                        EHCI_HCOR->usbcmd = temp;

                        while (EHCI_HCOR->usbcmd & EHCI_USBCMD_RUN) {
                        }

                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_SUSPEND;
                        while ((EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_SUSPEND) == 0) {
                        }
                        break;
                    case HUB_PORT_FEATURE_POWER:
                        EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_PP;
                        break;
                    case HUB_PORT_FEATURE_RESET:
                        usbh_reset_port(bus, port);
#if defined(LPKG_CHERRYUSB_HOST_OHCI)
                        if (!(EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_PE)) {
                            EHCI_HCOR->portsc[port - 1] |= EHCI_PORTSC_OWNER;

                            while (!(EHCI_HCOR->portsc[port - 1] & EHCI_PORTSC_OWNER)) {
                            }

                            USB_LOG_INFO("reset Switch port %u to OHCI\r\n", port);
                            return -USB_ERR_NOTSUPP;
                        }
#endif
                        break;

                    default:
                        return -USB_ERR_NOTSUPP;
                }
                break;
            case HUB_REQUEST_GET_STATUS:
                if (!port || port > nports) {
                    return -USB_ERR_INVAL;
                }
                temp = EHCI_HCOR->portsc[port - 1];

                status = 0;
                if (temp & EHCI_PORTSC_CSC) {
                    status |= (1 << HUB_PORT_FEATURE_C_CONNECTION);
                }
                if (temp & EHCI_PORTSC_PEC) {
                    status |= (1 << HUB_PORT_FEATURE_C_ENABLE);
                }
                if (temp & EHCI_PORTSC_OCC) {
                    status |= (1 << HUB_PORT_FEATURE_C_OVER_CURREN);
                }

                if (temp & EHCI_PORTSC_CCS) {
                    status |= (1 << HUB_PORT_FEATURE_CONNECTION);
                }
                if (temp & EHCI_PORTSC_PE) {
                    status |= (1 << HUB_PORT_FEATURE_ENABLE);

                    if (usbh_get_port_speed(bus, port) == USB_SPEED_LOW) {
                        status |= (1 << HUB_PORT_FEATURE_LOWSPEED);
                    } else if (usbh_get_port_speed(bus, port) == USB_SPEED_HIGH) {
                        status |= (1 << HUB_PORT_FEATURE_HIGHSPEED);
                    }
                }
                if (temp & EHCI_PORTSC_SUSPEND) {
                    status |= (1 << HUB_PORT_FEATURE_SUSPEND);
                }
                if (temp & EHCI_PORTSC_OCA) {
                    status |= (1 << HUB_PORT_FEATURE_OVERCURRENT);
                }
                if (temp & EHCI_PORTSC_RESET) {
                    status |= (1 << HUB_PORT_FEATURE_RESET);
                }
                if (temp & EHCI_PORTSC_PP || !(EHCI_HCCR->hcsparams & EHCI_HCSPARAMS_PPC)) {
                    status |= (1 << HUB_PORT_FEATURE_POWER);
                }
                memcpy(buf, &status, 4);
                break;
            default:
                break;
        }
    }
    return 0;
}


int usbh_submit_urb(struct usbh_urb *urb)
{
    struct ehci_qh_hw *qh = NULL;
    size_t flags;
    int ret = 0;
    struct usbh_hub *hub;
    struct usbh_hubport *hport;
    struct usbh_bus *bus;

    if (!urb || !urb->hport || !urb->ep || !urb->hport->bus) {
        return -USB_ERR_INVAL;
    }

    bus = urb->hport->bus;

    /* find active hubport in roothub */
    hport = urb->hport;
    hub = urb->hport->parent;
    while (!hub->is_roothub) {
        hport = hub->parent;
        hub = hub->parent->parent;
    }

#if defined(LPKG_CHERRYUSB_HOST_OHCI)
    if (EHCI_HCOR->portsc[hport->port - 1] & EHCI_PORTSC_OWNER) {
        return ohci_submit_urb(urb);
    }
#endif

    if (!urb->hport->connected || !(EHCI_HCOR->portsc[hport->port - 1] & EHCI_PORTSC_CCS)) {
        return -USB_ERR_NOTCONN;
    }

    if (urb->errorcode == -USB_ERR_BUSY) {
        return -USB_ERR_BUSY;
    }

    flags = usb_osal_enter_critical_section();

    urb->hcpriv = NULL;
    urb->errorcode = -USB_ERR_BUSY;
    urb->actual_length = 0;

    usb_osal_leave_critical_section(flags);

    switch (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes)) {
        case USB_ENDPOINT_TYPE_CONTROL:
            qh = ehci_control_urb_init(bus, urb, urb->setup, urb->transfer_buffer, urb->transfer_buffer_length);
            if (qh == NULL) {
                return -USB_ERR_NOMEM;
            }
            urb->hcpriv = qh;
            break;
        case USB_ENDPOINT_TYPE_BULK:
            qh = ehci_bulk_urb_init(bus, urb, urb->transfer_buffer, urb->transfer_buffer_length);
            if (qh == NULL) {
                return -USB_ERR_NOMEM;
            }
            urb->hcpriv = qh;
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
            qh = ehci_intr_urb_init(bus, urb, urb->transfer_buffer, urb->transfer_buffer_length);
            if (qh == NULL) {
                return -USB_ERR_NOMEM;
            }
            urb->hcpriv = qh;
            break;
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
#ifdef CONFIG_USB_EHCI_ISO
            ret = ehci_iso_urb_init(bus, urb);
#endif
            break;
        default:
            break;
    }

    if (urb->timeout > 0) {
        /* wait until timeout or sem give */
        ret = usb_osal_sem_take(qh->waitsem, urb->timeout);
        if (ret < 0) {
            goto errout_timeout;
        }
        urb->timeout = 0;
        ret = urb->errorcode;
        /* we can free qh when waitsem is done */
        ehci_qh_free(bus, qh);
    }
    return ret;
errout_timeout:
    USB_LOG_ERR("urb submit timeout, ep:%d ep_type:%d dev_addr:0x%x\n\n",
            urb->ep->bEndpointAddress,
            USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes),
            urb->hport->dev_addr);
    urb->timeout = 0;
    usbh_kill_urb(urb);
    return ret;
}

int usbh_kill_urb(struct usbh_urb *urb)
{
    struct ehci_qh_hw *qh;
    struct usbh_bus *bus;
    size_t flags;
    bool remove_in_iaad = false;

    if (!urb || !urb->hport || !urb->hcpriv || !urb->hport->bus) {
        return -USB_ERR_INVAL;
    }

    bus = urb->hport->bus;

#if defined(LPKG_CHERRYUSB_HOST_OHCI)
    if (EHCI_HCOR->portsc[urb->hport->port - 1] & EHCI_PORTSC_OWNER) {
        return ohci_kill_urb(urb);
    }
#endif

    flags = usb_osal_enter_critical_section();

    EHCI_HCOR->usbcmd &= ~(EHCI_USBCMD_PSEN | EHCI_USBCMD_ASEN);

    if ((USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) == USB_ENDPOINT_TYPE_CONTROL) || (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) == USB_ENDPOINT_TYPE_BULK)) {
        qh = EHCI_ADDR2QH(g_async_qh_head[bus->hcd.hcd_id].hw.hlp);
        while ((qh != &g_async_qh_head[bus->hcd.hcd_id]) && qh) {
            if (qh->urb == urb) {
                remove_in_iaad = true;
                ehci_kill_qh(bus, &g_async_qh_head[bus->hcd.hcd_id], qh);
            }
            qh = EHCI_ADDR2QH(qh->hw.hlp);
        }
    } else if (USB_GET_ENDPOINT_TYPE(urb->ep->bmAttributes) == USB_ENDPOINT_TYPE_INTERRUPT) {
        qh = EHCI_ADDR2QH(g_periodic_qh_head[bus->hcd.hcd_id][EHCI_PERIOIDIC_QH_NUM - 1].hw.hlp);
        while (qh) {
            if (qh->urb == urb) {
                if (urb->hport->speed == USB_SPEED_HIGH) {
                    ehci_kill_qh(bus, ehci_get_periodic_qhead(bus, urb->ep->bInterval), qh);
                } else {
                    ehci_kill_qh(bus, ehci_get_periodic_qhead(bus, urb->ep->bInterval * 8), qh);
                }
            }
            qh = EHCI_ADDR2QH(qh->hw.hlp);
        }
    } else {
#ifdef CONFIG_USB_EHCI_ISO
        ehci_kill_iso_urb(bus, urb);
        EHCI_HCOR->usbcmd |= (EHCI_USBCMD_PSEN | EHCI_USBCMD_ASEN);
        usb_osal_leave_critical_section(flags);
        return 0;
#endif
    }

    EHCI_HCOR->usbcmd |= (EHCI_USBCMD_PSEN | EHCI_USBCMD_ASEN);

    qh = (struct ehci_qh_hw *)urb->hcpriv;
    urb->hcpriv = NULL;
    urb->errorcode = -USB_ERR_SHUTDOWN;
    qh->urb = NULL;

    if (urb->timeout) {
        usb_osal_sem_give(qh->waitsem);
    } else {
        ehci_qh_free(bus, qh);
    }

    if (remove_in_iaad) {
        volatile uint32_t timeout = 0;
        EHCI_HCOR->usbcmd |= EHCI_USBCMD_IAAD;
        while (!(EHCI_HCOR->usbsts & EHCI_USBSTS_IAA)) {
            usb_osal_msleep(1);
            timeout++;
            if (timeout > 100) {
                usb_osal_leave_critical_section(flags);
                return -USB_ERR_TIMEOUT;
            }
        }
        EHCI_HCOR->usbsts = EHCI_USBSTS_IAA;
    }

    usb_osal_leave_critical_section(flags);

    return 0;
}

static void ehci_scan_async_list(struct usbh_bus *bus)
{
    struct ehci_qh_hw *qh;

    qh = EHCI_ADDR2QH(g_async_qh_head[bus->hcd.hcd_id].hw.hlp);
    while ((qh != &g_async_qh_head[bus->hcd.hcd_id]) && qh) {
        if (qh->urb) {
            ehci_check_qh(bus, &g_async_qh_head[bus->hcd.hcd_id], qh);
        }
        qh = EHCI_ADDR2QH(qh->hw.hlp);
    }
}

static void ehci_scan_periodic_list(struct usbh_bus *bus)
{
    struct ehci_qh_hw *qh;

    qh = EHCI_ADDR2QH(g_periodic_qh_head[bus->hcd.hcd_id][EHCI_PERIOIDIC_QH_NUM - 1].hw.hlp);
    while (qh) {
        if (qh->urb) {
            if (qh->urb->hport->speed == USB_SPEED_HIGH) {
                ehci_check_qh(bus, ehci_get_periodic_qhead(bus, qh->urb->ep->bInterval), qh);
            } else {
                ehci_check_qh(bus, ehci_get_periodic_qhead(bus, qh->urb->ep->bInterval * 8), qh);
            }
        }
        qh = EHCI_ADDR2QH(qh->hw.hlp);
    }
}

void usbh_irq_handler(uint32_t irq_num, struct usbh_bus *bus)
{
    uint32_t usbsts;

    usbsts = EHCI_HCOR->usbsts & EHCI_HCOR->usbintr;
    EHCI_HCOR->usbsts = usbsts;

    if (usbsts & EHCI_USBSTS_INT) {
        ehci_scan_async_list(bus);
        ehci_scan_periodic_list(bus);
#ifdef CONFIG_USB_EHCI_ISO
        ehci_scan_isochronous_list(bus);
#endif
    }

    if (usbsts & EHCI_USBSTS_ERR) {
        ehci_scan_async_list(bus);
        ehci_scan_periodic_list(bus);
#ifdef CONFIG_USB_EHCI_ISO
        ehci_scan_isochronous_list(bus);
#endif
    }

    if (usbsts & EHCI_USBSTS_PCD) {
        for (int port = 0; port < g_ehci_hcd[bus->hcd.hcd_id].n_ports; port++) {
            uint32_t portsc = EHCI_HCOR->portsc[port];

            if (portsc & EHCI_PORTSC_CSC) {
                if ((portsc & EHCI_PORTSC_CCS) == EHCI_PORTSC_CCS) {
                } else {
                    for (uint8_t index = 0; index < CONFIG_USB_EHCI_QH_NUM; index++) {
                        g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[index] = false;
                    }
                    for (uint8_t index = 0; index < CONFIG_USB_EHCI_QTD_NUM; index++) {
                        g_ehci_hcd[bus->hcd.hcd_id].ehci_qtd_used[index] = false;
                    }
                    for (uint8_t index = 0; index < CONFIG_USB_EHCI_ITD_NUM; index++) {
                        g_ehci_hcd[bus->hcd.hcd_id].ehci_itd_used[index] = false;
                    }
                }

                bus->hcd.roothub.int_buffer[0] |= (1 << (port + 1));
                usbh_hub_thread_wakeup(&bus->hcd.roothub);
            }
        }
    }

    if (usbsts & EHCI_USBSTS_IAA) {
        for (uint8_t index = 0; index < CONFIG_USB_EHCI_QH_NUM; index++) {
            struct ehci_qh_hw *qh = &ehci_qh_pool[bus->hcd.hcd_id][index];
            if (g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[index] && qh->remove_in_iaad) {
                ehci_urb_waitup(bus, qh->urb);
            }
        }
    }

    if (usbsts & EHCI_USBSTS_FATAL) {
    }
}

#ifdef RT_USING_PM
static int ehci_is_busy(struct usbh_bus *bus)
{
    bool has_device = false;

    /* Quick check: if no device is connected at all, definitely not busy */
    for (uint8_t port = 0; port < g_ehci_hcd[bus->hcd.hcd_id].n_ports; port++) {
        if (EHCI_HCOR->portsc[port] & EHCI_PORTSC_CCS) {
            has_device = true;
            break;
        }
    }

    if (!has_device)
        return 0;

    /* Only block sleep for in-flight bulk/control transfers.
     * Interrupt transfers (HID) will be re-established on resume;
     * an idle U-disk has no QH allocated at all.
     */
    for (uint32_t i = 0; i < CONFIG_USB_EHCI_QH_NUM; i++) {
        if (!g_ehci_hcd[bus->hcd.hcd_id].ehci_qh_used[i])
            continue;

        struct ehci_qh_hw *qh = &ehci_qh_pool[bus->hcd.hcd_id][i];
        if (!qh->urb || !qh->urb->ep)
            continue;

        uint8_t ep_type = USB_GET_ENDPOINT_TYPE(qh->urb->ep->bmAttributes);
        if (ep_type == USB_ENDPOINT_TYPE_BULK ||
            ep_type == USB_ENDPOINT_TYPE_CONTROL)
            return 1;
    }

    return 0;
}

static int ehci_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct usbh_bus *bus = (struct usbh_bus *)device->user_data;

    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    case PM_SLEEP_MODE_SHUTDOWN:
        if (ehci_is_busy(bus))
            return -RT_EBUSY;
        usb_hc_deinit(bus);
        break;
    default:
        break;
    }

    return 0;
}

static void ehci_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct usbh_bus *bus = (struct usbh_bus *)device->user_data;

    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    case PM_SLEEP_MODE_SHUTDOWN:
        usb_hc_init(bus);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops ehci_pm_ops =
{
    SET_DEVICE_PM_OPS(ehci_suspend, ehci_resume)
    NULL,
};

static struct rt_device *usbh_dev[CONFIG_USBHOST_MAX_BUS];

static int usbh_ehci_pm_init(struct usbh_bus *bus)
{
    char str[32] = {0};

    if (usbh_dev[bus->busid])
        return 0;

    rt_snprintf(str, sizeof(str), "usbh%d", bus->busid);
    usbh_dev[bus->busid] = (struct rt_device *)rt_malloc(sizeof(struct rt_device));

    if (usbh_dev[bus->busid]) {
        rt_memset(usbh_dev[bus->busid], 0, sizeof(struct rt_device));
        usbh_dev[bus->busid]->user_data = bus;

        if (rt_device_register(usbh_dev[bus->busid], str, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
            USB_LOG_WRN("Failed to register usbh%d\n", bus->busid);
            rt_free(usbh_dev[bus->busid]);
            usbh_dev[bus->busid] = RT_NULL;
        } else {
            rt_pm_device_register(usbh_dev[bus->busid], &ehci_pm_ops);
        }
    }

    return 0;
}
#endif
