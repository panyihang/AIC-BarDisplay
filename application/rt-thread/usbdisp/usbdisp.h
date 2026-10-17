/* SPDX-License-Identifier: Apache-2.0 */
#ifndef USB_DISP_APP_H
#define USB_DISP_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "aic_osal.h"
#include "mpp_fb.h"
#include "mpp_decoder.h"
#include "lvgl.h"
#include "artinchip_fb.h"

#define UDISP_FRAME_LIMIT 300000U
#define UDISP_JPEG_QUALITY FIRMWARE_JPEG_QUALITY
#define UDISP_MAX_FPS FIRMWARE_MAX_FPS
#define UDISP_PRESENT_PERIOD_MS 17
#define UDISP_FRAME_COUNT 2
#define UDISP_COUNTDOWN_MS 3000U
#define UDISP_UI_WIDTH 400
#define UDISP_UI_HEIGHT 480

struct udisp_header {
    uint16_t crc16;
    uint8_t type;
    uint8_t cmd;
    uint16_t x, y, width, height;
    uint32_t frame_id:10;
    uint32_t payload_total:22;
} __attribute__((packed));

typedef char udisp_header_must_be_16_bytes[(sizeof(struct udisp_header) == 16) ? 1 : -1];

struct udisp_frame {
    uint8_t *data;
    uint32_t length;
    uint32_t total;
    uint32_t sequence;
    uint8_t state;
};

struct udisp_stats {
    uint32_t received, completed, dropped, invalid, decode_errors, shown;
    uint32_t perf_decode_frames;
    uint32_t perf_decode_ms;
    uint32_t perf_present_ms;
    uint32_t display_errors;
    uint32_t last_frame_id;
    uint32_t last_frame_addr;
};

extern struct aicfb_screeninfo udisp_screen;
extern struct udisp_stats udisp_stats;
extern struct mpp_fb *udisp_fb;
extern aicos_mutex_t udisp_display_lock;
extern volatile bool udisp_stream_hint;
extern volatile bool udisp_stream_enabled;
extern volatile bool udisp_stream_requested;
extern volatile bool udisp_page_hidden;
extern volatile bool udisp_hid_enabled;
extern aicos_sem_t udisp_ready;

int udisp_transport_start(void);
bool udisp_host_alive(void);
void udisp_transport_reset(void);
struct udisp_frame *udisp_take_frame(void);
void udisp_return_frame(struct udisp_frame *frame);
uint8_t *udisp_idle_buffer(void);
int udisp_touch_report(const void *fingers, unsigned count);
void udisp_ui_init(void);
void udisp_ui_deinit(void);
void udisp_ui_poll(void);
void udisp_ui_visibility(bool enable);
void udisp_ui_frame_ready(void);
void udisp_perf_poll(void);
int udisp_video_open(void);
int udisp_video_read(uint8_t *data, uint32_t capacity, uint32_t *size);
uint32_t udisp_video_period(void);
void udisp_video_close(void);
bool udisp_decode(const uint8_t *data, uint32_t size, bool native_screen);
void udisp_decoder_close(void);
int udisp_decoder_open(void);
int udisp_settings_load(void);
int udisp_settings_save(void);
void udisp_settings_poll(void);
void udisp_ui_command(int command, bool value);
#endif
