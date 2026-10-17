/* SPDX-License-Identifier: Apache-2.0 */
#include <stdio.h>
#include <string.h>
#include <rtthread.h>
#include <dfs_fs.h>
#include "usbdisp.h"

static void display_worker(void *arg)
{
    bool last_stream = false;
    uint32_t next_video = 0, size;
    (void)arg;
    while (1) {
        bool stream = udisp_stream_requested;
        if (!udisp_host_alive() && !udisp_stream_requested) {
            udisp_stream_requested = false;
            stream = false;
        }
        if (stream != last_stream) {
            udisp_stream_enabled = false;
            udisp_decoder_close();
            udisp_video_close();
            /* Preserve the remaining bytes of the dropped countdown frame
             * when entering stream mode, so JPEG data is never read as a header. */
            if (!stream) udisp_transport_reset();
            last_stream = stream;
            udisp_stream_enabled = stream;
            printf("usbdisp: stream %s\n", stream ? "start" : "stop");
            if (stream)
                udisp_ui_visibility(false);
            if (!stream) {
                udisp_video_open();
                next_video = 0;
            }
        }
        if (stream) {
            struct udisp_frame *frame;
            aicos_sem_take(udisp_ready, 30);
            frame = udisp_take_frame();
            if (frame) {
                if (udisp_decode(frame->data, frame->length, true)) udisp_ui_frame_ready();
                udisp_return_frame(frame);
            }
        } else {
            uint32_t now = aic_get_time_ms();
            if ((int32_t)(now - next_video) >= 0) {
                uint8_t *data = udisp_idle_buffer();
                if (udisp_video_open() == 0 &&
                    udisp_video_read(data, UDISP_FRAME_LIMIT, &size) == 0)
                    udisp_decode(data, size, false);
                if (!next_video) next_video = now;
                next_video += udisp_video_period();
                if ((int32_t)(now - next_video) > (int32_t)udisp_video_period())
                    next_video = now + udisp_video_period();
            }
            aicos_msleep(5);
        }
        udisp_perf_poll();
    }
}

int main(void)
{
    int retries;
    unsigned long cma_bytes;
    unsigned long nv12_bytes;
    for (retries = 0; retries < 200; retries++) {
        struct dfs_filesystem *fs = dfs_filesystem_lookup("/rodata");
        if (fs && fs->path && !strcmp(fs->path, "/rodata")) break;
        aicos_msleep(10);
    }
    if (retries == 200) { printf("usbdisp: assets mount failed\n"); return -1; }
    udisp_fb = mpp_fb_open();
    if (!udisp_fb || mpp_fb_ioctl(udisp_fb, AICFB_GET_SCREENINFO, &udisp_screen) < 0)
        return -1;
    cma_bytes = (unsigned long)&__cma_heap_end - (unsigned long)&__cma_heap_start;
    nv12_bytes = ((udisp_screen.width + 15U) & ~15U) *
                 ((udisp_screen.height + 15U) & ~15U) * 3U / 2U;
    udisp_settings_load();
    if (udisp_transport_start()) { printf("usbdisp: USB/buffer init failed\n"); return -1; }
    udisp_display_lock = aicos_mutex_create();
    if (!udisp_display_lock) {
        printf("usbdisp: display mutex allocation failed\n");
        return -1;
    }
    mpp_fb_ioctl(udisp_fb, AICFB_POWERON, NULL);
    udisp_ui_init();
    if (!aicos_thread_create("udispdec", 6144, 18, display_worker, NULL))
        return -1;
    printf("usbdisp: %ux%u, JPEG quality %d, max %d fps, native pixels\n",
           udisp_screen.width, udisp_screen.height, UDISP_JPEG_QUALITY, UDISP_MAX_FPS);
    printf("usbdisp mem: cma=%lu fb=%u rx=%u nv12x2=%lu budget=%lu\n",
           cma_bytes, udisp_screen.smem_len,
           UDISP_FRAME_COUNT * UDISP_FRAME_LIMIT, nv12_bytes * 2UL,
           (unsigned long)udisp_screen.smem_len +
           UDISP_FRAME_COUNT * UDISP_FRAME_LIMIT + nv12_bytes * 2UL);
    while (1) { udisp_ui_poll(); aicos_msleep(10); }
}
