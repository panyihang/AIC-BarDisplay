/*
 * SPDX-License-Identifier: Apache-2.0
 * LVGLMJPG format from the reference project's video_player.c.
 * Read each index entry on demand instead of caching the entire table.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "usbdisp.h"

struct video_header {
    char magic[8];
    uint16_t width, height, fps_num, fps_den;
    uint32_t frame_count, index_offset, frames_offset, max_frame_size, flags;
    uint8_t reserved[28];
} __attribute__((packed));

struct video_index { uint32_t offset, size; };
static FILE *file;
static struct video_header header;
static uint32_t frame_id, file_size;

void udisp_video_close(void)
{
    if (file) fclose(file);
    file = NULL;
}

int udisp_video_open(void)
{
    struct stat st;
    const char *path = "/rodata/video/video.bin";
    if (file) return 0;
    if (stat(path, &st) || st.st_size < sizeof(header)) return -1;
    file_size = st.st_size;
    file = fopen(path, "rb");
    if (!file) return -1;
    setvbuf(file, NULL, _IONBF, 0);
    if (fread(&header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header.magic, "LVGLMJPG", 8) || !header.width || !header.height ||
        !header.frame_count || !header.fps_num || !header.fps_den ||
        header.max_frame_size > UDISP_FRAME_LIMIT ||
        header.index_offset < sizeof(header) || header.frames_offset > file_size ||
        header.frames_offset < header.index_offset ||
        header.frame_count > (header.frames_offset - header.index_offset) / 8)
        goto fail;
    frame_id = 0;
    return 0;
fail:
    udisp_video_close();
    return -1;
}

uint32_t udisp_video_period(void)
{
    uint32_t period = file ? 1000U * header.fps_den / header.fps_num : 100;
    return period > 0 ? period : 1;
}

int udisp_video_read(uint8_t *data, uint32_t capacity, uint32_t *size)
{
    struct video_index entry;
    if (!file || fseek(file, header.index_offset + frame_id * 8, SEEK_SET) ||
        fread(&entry, 1, sizeof(entry), file) != sizeof(entry) ||
        !entry.size || entry.size > capacity || entry.offset < header.frames_offset ||
        entry.offset > file_size || entry.size > file_size - entry.offset ||
        fseek(file, entry.offset, SEEK_SET) ||
        fread(data, 1, entry.size, file) != entry.size) return -1;
    *size = entry.size;
    frame_id = (frame_id + 1) % header.frame_count;
    return 0;
}
