/* SPDX-License-Identifier: Apache-2.0 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <rtthread.h>
#include "usbdisp.h"

#define SETTINGS_MAGIC 0x55444953U
struct saved_settings { uint32_t magic, flags, check; };
static volatile int pending_command;
static volatile bool pending_value;

int udisp_settings_load(void)
{
    struct saved_settings s;
    FILE *f = fopen("/data/udisp.cfg", "rb");
    if (!f) return 0;
    int valid = fread(&s, 1, sizeof(s), f) == sizeof(s) &&
                s.magic == SETTINGS_MAGIC && s.check == (s.magic ^ s.flags);
    fclose(f);
    if (!valid) return -1;
    udisp_page_hidden = (s.flags & 1) != 0;
    udisp_hid_enabled = (s.flags & 2) != 0;
    return 0;
}

int udisp_settings_save(void)
{
    struct saved_settings s = {SETTINGS_MAGIC, 0, 0};
    FILE *f;
    int ret;
    s.flags = (udisp_page_hidden ? 1 : 0) | (udisp_hid_enabled ? 2 : 0);
    s.check = s.magic ^ s.flags;
    f = fopen("/data/udisp.tmp", "wb");
    if (!f) { printf("usbdisp: settings storage unavailable\n"); return -1; }
    ret = fwrite(&s, 1, sizeof(s), f) == sizeof(s) ? 0 : -1;
    if (fflush(f) || fsync(fileno(f))) ret = -1;
    if (fclose(f)) ret = -1;
    if (!ret) ret = rename("/data/udisp.tmp", "/data/udisp.cfg");
    if (ret) printf("usbdisp: settings save failed\n");
    return ret;
}

void udisp_settings_poll(void)
{
    int command;
    bool value;
    unsigned long flags;
    aicos_local_irq_save(&flags);
    command = pending_command;
    value = pending_value;
    pending_command = 0;
    aicos_local_irq_restore(flags);
    if (command) udisp_ui_command(command, value);
}

void udisp_perf_poll(void)
{
    static uint32_t last_ms;
    static struct udisp_stats last;
    uint32_t now = (uint32_t)aic_get_time_ms();
    uint32_t elapsed;
    struct udisp_stats cur;

    if (!last_ms) {
        last_ms = now;
        last = udisp_stats;
        return;
    }
    elapsed = now - last_ms;
    if (elapsed < 1000 || !udisp_stream_enabled)
        return;
    cur = udisp_stats;
    printf("usbdisp perf: rx=%u/s done=%u/s drop=%u/s decode=%u/s shown=%u/s "
           "decode_ms=%u present_ms=%u invalid=%u errors=%u display_errors=%u "
           "frame_id=%u addr=0x%08lx\n",
           (cur.received - last.received) * 1000U / elapsed,
           (cur.completed - last.completed) * 1000U / elapsed,
           (cur.dropped - last.dropped) * 1000U / elapsed,
           (cur.perf_decode_frames - last.perf_decode_frames) * 1000U / elapsed,
           (cur.shown - last.shown) * 1000U / elapsed,
           cur.perf_decode_frames == last.perf_decode_frames ? 0 :
           (cur.perf_decode_ms - last.perf_decode_ms) /
           (cur.perf_decode_frames - last.perf_decode_frames),
           cur.perf_decode_frames == last.perf_decode_frames ? 0 :
           (cur.perf_present_ms - last.perf_present_ms) /
           (cur.perf_decode_frames - last.perf_decode_frames),
           cur.invalid, cur.decode_errors, cur.display_errors,
           cur.last_frame_id, (unsigned long)cur.last_frame_addr);
    last_ms = now;
    last = cur;
}

static int cmd_udisp(int argc, char **argv)
{
    int command = 0;
    bool value = false;
    if (argc == 2 && !strcmp(argv[1], "settings")) command = 1;
    else if (argc == 2 && !strcmp(argv[1], "back")) command = 2;
    else if (argc == 3 && (!strcmp(argv[1], "page_hide") || !strcmp(argv[1], "hid"))) {
        if (strcmp(argv[2], "0") && strcmp(argv[2], "1")) return -1;
        command = !strcmp(argv[1], "page_hide") ? 3 : 4;
        value = atoi(argv[2]) != 0;
    } else if (argc == 2 && !strcmp(argv[1], "status")) {
        printf("host=%d stream=%d page_hide=%d hid=%d\n",
               udisp_host_alive(), udisp_stream_enabled, udisp_page_hidden, udisp_hid_enabled);
        printf("rx=%u complete=%u drop=%u invalid=%u decode_error=%u shown=%u display_error=%u frame_id=%u addr=0x%08lx\n",
               udisp_stats.received, udisp_stats.completed, udisp_stats.dropped,
               udisp_stats.invalid, udisp_stats.decode_errors, udisp_stats.shown,
               udisp_stats.display_errors, udisp_stats.last_frame_id,
               (unsigned long)udisp_stats.last_frame_addr);
        return 0;
    } else {
        printf("udisp settings|back|page_hide 0/1|hid 0/1|status\n");
        return -1;
    }
    unsigned long flags;
    aicos_local_irq_save(&flags);
    if (pending_command) { aicos_local_irq_restore(flags); return -1; }
    pending_value = value;
    pending_command = command;
    aicos_local_irq_restore(flags);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_udisp, udisp, USB display settings and status);
