/*
 * SPDX-License-Identifier: Apache-2.0
 * UDISP receive policy ported from Espressif's app_vendor.c / usb_frame.c.
 * No allocation, blocking locks or JPEG work is performed in USB callbacks.
 */
#include <string.h>
#include <stdio.h>
#include <fal.h>
#include "usbdisp.h"
#include "usbd_core.h"
#include "usbd_vender.h"
#include "usbd_msc.h"
#include "usbd_hid.h"
#include "usb_osal.h"
#include "aic_core.h"
#include "aic_time.h"
#include "aic_osal.h"

#define RX_SIZE 512
#ifdef CONFIG_USB_HS
#define BULK_MPS 512
#else
#define BULK_MPS 64
#endif

static USB_MEM_ALIGNX uint8_t rx_buffer[RX_SIZE];
static struct udisp_frame frames[UDISP_FRAME_COUNT];
static struct udisp_frame *current;
static uint32_t skip_remaining, sequence;
static volatile bool configured;
static struct usbd_interface vendor, hid, msc;
static const struct fal_partition *assets;
static uint8_t descriptor[512];
static uint8_t hid_report[512];
static uint32_t hid_report_size;
static char capability[64];
static uint8_t ms_os_string[18] = {0x12, USB_DESCRIPTOR_TYPE_STRING,
    'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0,0x20};
static uint8_t ms_compat_id[40] = {
    40,0,0,0,0,1,4,0,1,0,0,0,0,0,0,0,
    0,1,'W','I','N','U','S','B',0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};
static struct usb_msosv1_descriptor ms_os_desc = {
    .string = ms_os_string, .vendor_code = 0x20, .compat_id = ms_compat_id,
};
static bool started;
static volatile uint32_t last_sof_ms;

static uint8_t *add_string(uint8_t *p, const char *s)
{
    unsigned n = strlen(s), i;
    *p++ = 2 + n * 2;
    *p++ = USB_DESCRIPTOR_TYPE_STRING;
    for (i = 0; i < n; i++) { *p++ = s[i]; *p++ = 0; }
    return p;
}

static void build_hid_report(void)
{
    /* Five fingers, ten bytes each, followed by contact count. */
    static const uint8_t head[] = {0x05,0x0d,0x09,0x04,0xa1,0x01,0x85,0x01};
    uint16_t width = udisp_screen.width, height = udisp_screen.height;
    const uint8_t finger[] = {
        0x09,0x42,0xa1,0x02,0x09,0x42,0x15,0,0x25,1,
        0x75,1,0x95,1,0x81,2,0x95,7,0x81,3,
        0x75,8,0x09,0x51,0x95,1,0x81,2,
        0x05,1,0x26,width & 0xff,width >> 8,0x75,16,0x55,0x0e,0x65,0x13,
        0x09,0x30,0x35,0,0x46,width & 0xff,width >> 8,0x81,2,
        0x26,height & 0xff,height >> 8,0x46,height & 0xff,height >> 8,0x09,0x31,0x81,2,
        0x05,0x0d,0x09,0x48,0x09,0x49,0x95,2,0x81,2,0xc0
    };
    static const uint8_t tail[] = {
        0x09,0x54,0x25,0x7f,0x95,1,0x75,8,0x81,2,
        0x85,2,0x09,0x55,0x95,1,0x25,16,0xb1,2,0xc0
    };
    uint8_t *p = hid_report;
    int i;
    memcpy(p, head, sizeof(head)); p += sizeof(head);
    for (i = 0; i < 5; i++) {
        memcpy(p, finger, sizeof(finger));
        p += sizeof(finger);
    }
    memcpy(p, tail, sizeof(tail)); p += sizeof(tail);
    hid_report_size = p - hid_report;
}

static void build_descriptors(void)
{
    const uint8_t base[] = {
        USB_DEVICE_DESCRIPTOR_INIT(USB_2_0,0,0,0,0x303a,0x2987,0x0101,1),
        USB_CONFIG_DESCRIPTOR_INIT(80,3,1,USB_CONFIG_BUS_POWERED,100),
        USB_INTERFACE_DESCRIPTOR_INIT(0,0,2,0xff,0,0,4),
        USB_ENDPOINT_DESCRIPTOR_INIT(0x01,USB_ENDPOINT_TYPE_BULK,BULK_MPS,0),
        USB_ENDPOINT_DESCRIPTOR_INIT(0x81,USB_ENDPOINT_TYPE_BULK,BULK_MPS,0),
        USB_INTERFACE_DESCRIPTOR_INIT(1,0,1,3,0,0,5),
        USB_HID_DESCRIPTOR_INIT(0x0111,0,1,0x22,hid_report_size),
        USB_ENDPOINT_DESCRIPTOR_INIT(0x82,USB_ENDPOINT_TYPE_INTERRUPT,64,10),
        USB_INTERFACE_DESCRIPTOR_INIT(2,0,2,8,6,0x50,6),
        USB_ENDPOINT_DESCRIPTOR_INIT(0x03,USB_ENDPOINT_TYPE_BULK,BULK_MPS,0),
        USB_ENDPOINT_DESCRIPTOR_INIT(0x83,USB_ENDPOINT_TYPE_BULK,BULK_MPS,0),
        USB_LANGID_INIT(1033)
    };
    uint8_t *p = descriptor;
    memcpy(p, base, sizeof(base)); p += sizeof(base);
    /* The open IDD interprets Bl in KiB, not bytes. Round down so the host
     * never encodes a frame larger than the allocated RX buffer. */
    snprintf(capability, sizeof(capability), "esp32p4udisp0_R%ux%u_Ejpg%d_Fps%d_Bl%u",
             udisp_screen.width, udisp_screen.height, UDISP_JPEG_QUALITY,
             UDISP_MAX_FPS, (unsigned)(UDISP_FRAME_LIMIT / 1024));
    p = add_string(p, "Espressif");
    p = add_string(p, capability);
    p = add_string(p, "012-2021");
    p = add_string(p, capability);
    p = add_string(p, "touch");
    p = add_string(p, "assets");
#ifdef CONFIG_USB_HS
    {
        const uint8_t qualifier[] = {10,6,0,2,0,0,0,64,1,0};
        memcpy(p, qualifier, sizeof(qualifier)); p += sizeof(qualifier);
    }
#endif
    *p = 0;
}

static struct usbd_interface *vendor_init(uint8_t busid, struct usbd_interface *intf)
{
    return usbd_vender_init_intf(busid, intf);
}

void udisp_transport_reset(void)
{
    size_t flags = usb_osal_enter_critical_section();
    int i;
    current = NULL;
    skip_remaining = 0;
    for (i = 0; i < UDISP_FRAME_COUNT; i++)
        if (frames[i].state != 3) frames[i].state = 0;
    udisp_stream_hint = false;
    usb_osal_leave_critical_section(flags);
}

static void receive(const uint8_t *data, uint32_t length)
{
    struct udisp_header h;
    uint32_t payload, copy;
    int i;
    if (!length) return;
    udisp_stats.received += length;
    if (skip_remaining) {
        skip_remaining -= length < skip_remaining ? length : skip_remaining;
        return;
    }
    if (!current) {
        if (length < sizeof(h)) { udisp_stats.invalid++; return; }
        memcpy(&h, data, sizeof(h));
        payload = h.payload_total;
        if (h.type == 0xff) return;
        if (h.type != 3 || h.x || h.y || h.width != udisp_screen.width ||
            h.height != udisp_screen.height || !payload) {
            udisp_stats.invalid++;
            if (payload <= UDISP_FRAME_LIMIT * 4U) skip_remaining = payload;
        } else {
            udisp_stream_hint = true;
            if (udisp_stream_enabled && payload <= UDISP_FRAME_LIMIT) {
                for (i = 0; i < UDISP_FRAME_COUNT; i++)
                    if (frames[i].state == 2) break;
                if (i == UDISP_FRAME_COUNT) {
                    for (i = 0; i < UDISP_FRAME_COUNT; i++)
                        if (frames[i].state == 0) { current = &frames[i]; break; }
                }
            }
            if (!current) { udisp_stats.dropped++; skip_remaining = payload; }
        }
        data += sizeof(h); length -= sizeof(h);
        if (!current) {
            skip_remaining -= length < skip_remaining ? length : skip_remaining;
            return;
        }
        current->length = 0;
        current->total = payload;
        current->state = 1;
        current->sequence = ++sequence;
    }
    copy = current->total - current->length;
    if (length > copy) {
        udisp_stats.invalid++;
        current->state = 0; current = NULL;
        return;
    }
    memcpy(current->data + current->length, data, length);
    current->length += length;
    if (current->length == current->total) {
        current->state = 2;
        current = NULL;
        udisp_stats.completed++;
        aicos_sem_give(udisp_ready);
    }
}

static void rx_callback(uint8_t busid, uint8_t ep, uint32_t size)
{
    receive(rx_buffer, size);
    usbd_ep_start_read(busid, ep, rx_buffer, sizeof(rx_buffer));
}

static void tx_callback(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)busid; (void)ep; (void)size;
}

static struct usbd_endpoint rx_ep = {.ep_addr=0x01,.ep_cb=rx_callback};
static struct usbd_endpoint tx_ep = {.ep_addr=0x81,.ep_cb=tx_callback};
static struct usbd_endpoint hid_ep = {.ep_addr=0x82,.ep_cb=tx_callback};
static USB_MEM_ALIGNX uint8_t touch_report[52];
static volatile bool touch_busy;

static void touch_done(uint8_t busid, uint8_t ep, uint32_t size)
{
    (void)busid; (void)ep; (void)size;
    touch_busy = false;
}

int udisp_touch_report(const void *fingers, unsigned count)
{
    /* Wire-compatible extension hook; there is no touch controller on these boards. */
    int ret;
    size_t flags;
    if (!fingers || count > 5 || !udisp_hid_enabled || !udisp_stream_enabled)
        return -1;
    flags = usb_osal_enter_critical_section();
    if (!configured || touch_busy) { usb_osal_leave_critical_section(flags); return -1; }
    memset(touch_report, 0, sizeof(touch_report));
    touch_report[0] = 1;
    memcpy(touch_report + 1, fingers, count * 10);
    touch_report[51] = count;
    touch_busy = true;
    ret = usbd_ep_start_write(0, 0x82, touch_report, sizeof(touch_report));
    if (ret < 0) touch_busy = false;
    usb_osal_leave_critical_section(flags);
    return ret;
}

void usbd_hid_get_report(uint8_t busid, uint8_t intf, uint8_t id,
                         uint8_t type, uint8_t **data, uint32_t *length)
{
    static uint8_t max_count = 5;
    (void)busid; (void)intf; (void)type;
    if (id == 2) { *data = &max_count; *length = 1; } else { *length = 0; }
}

static void event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
    case USBD_EVENT_SOF:
        last_sof_ms = aic_get_time_ms();
        break;
    case USBD_EVENT_CONFIGURED:
        configured = true;
        last_sof_ms = aic_get_time_ms();
        usbd_ep_start_read(busid, 0x01, rx_buffer, sizeof(rx_buffer));
        break;
    case USBD_EVENT_RESET:
    case USBD_EVENT_DISCONNECTED:
        configured = false;
        touch_busy = false;
        udisp_stream_requested = false;
        udisp_stream_enabled = false;
        udisp_transport_reset();
        break;
    default: break;
    }
}

static const char *string_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index == 0) return (const char *)"	";
    if (index == 1) return "Espressif";
    if (index == 2) return capability;
    if (index == 3) return "012-2021";
    if (index == 4) return capability;
    if (index == 5) return "touch";
    if (index == 6) return "assets";
    return NULL;
}

static const struct usb_descriptor advanced_desc = {
    .string_descriptor_callback = string_callback,
    .msosv1_descriptor = &ms_os_desc,
};

bool udisp_host_alive(void)
{
    return configured && (uint32_t)(aic_get_time_ms() - last_sof_ms) < 1500;
}

struct udisp_frame *udisp_take_frame(void)
{
    struct udisp_frame *f = NULL;
    int i;
    size_t flags = usb_osal_enter_critical_section();
    for (i = 0; i < UDISP_FRAME_COUNT; i++)
        if (frames[i].state == 2 && (!f || frames[i].sequence > f->sequence))
            f = &frames[i];
    for (i = 0; i < UDISP_FRAME_COUNT; i++)
        if (frames[i].state == 2) frames[i].state = (&frames[i] == f) ? 3 : 0;
    usb_osal_leave_critical_section(flags);
    return f;
}

void udisp_return_frame(struct udisp_frame *f)
{
    size_t flags = usb_osal_enter_critical_section();
    f->state = 0;
    usb_osal_leave_critical_section(flags);
}

uint8_t *udisp_idle_buffer(void) { return frames[0].data; }

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *count, uint32_t *size)
{
    (void)busid; (void)lun;
    *size = 512; *count = assets ? assets->len / 512 : 0;
}

bool usbd_msc_check_storage(void) { return assets != NULL; }

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector,
                         uint8_t *data, uint32_t length)
{
    uint32_t offset;
    (void)busid; (void)lun;
    if (!assets || sector > assets->len / 512) return -1;
    offset = sector * 512;
    if (length > assets->len - offset) return -1;
    return fal_partition_read(assets, offset, data, length) == length ? 0 : -1;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector,
                          uint8_t *data, uint32_t length)
{
    (void)busid; (void)lun; (void)sector; (void)data; (void)length;
    return -1;
}

int udisp_transport_start(void)
{
    int i;
    if (started) return 0;
    assets = fal_partition_find("rodata");
    if (!assets) return -1;
    for (i = 0; i < UDISP_FRAME_COUNT; i++) {
        frames[i].data = aicos_malloc_align(MEM_CMA, UDISP_FRAME_LIMIT, CACHE_LINE_SIZE);
        if (!frames[i].data) goto fail;
    }
    udisp_ready = aicos_sem_create(0);
    if (!udisp_ready) goto fail;
    build_hid_report();
    build_descriptors();
#ifdef CONFIG_USBDEV_ADVANCE_DESC
    usbd_desc_register(0, &advanced_desc);
#else
    usbd_desc_register(0, descriptor);
    usbd_msosv1_desc_register(0, &ms_os_desc);
#endif
    usbd_add_interface(0, vendor_init(0, &vendor));
    usbd_add_endpoint(0, &rx_ep); usbd_add_endpoint(0, &tx_ep);
    usbd_add_interface(0, usbd_hid_init_intf(0, &hid, hid_report, hid_report_size));
    hid_ep.ep_cb = touch_done;
    usbd_add_endpoint(0, &hid_ep);
    usbd_add_interface(0, usbd_msc_init_intf(0, &msc, 0x03, 0x83));
    usbd_msc_set_readonly(0, true);
    usbd_msc_thread_init(0);
    started = true;
    usbd_initialize(0, USB_DEV_BASE, event_handler);
    return 0;
fail:
    for (i = 0; i < UDISP_FRAME_COUNT; i++) {
        if (frames[i].data) aicos_free_align(MEM_CMA, frames[i].data);
        frames[i].data = NULL;
    }
    return -1;
}
