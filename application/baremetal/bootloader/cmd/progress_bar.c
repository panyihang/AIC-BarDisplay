/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_common.h>
#include <aic_errno.h>

#include <string.h>
#include <console.h>
#include <artinchip_fb.h>
#include <video_font_data.h>

#ifdef AIC_BOOTLOADER_SPI_LCD
#include "spi_lcd.h"
#endif

#undef ALIGN_DOWM
#define ALIGN_DOWM(x, align)        ((x) & ~(align - 1))

#define BAR_BACKGROUND_COLOR        0x00, 0xA2, 0xE9
#define BAR_FILL_COLOR              0x18, 0xD4, 0x0A
#define CONSOLE_COLOR_FG            CONSOLE_LIGHT_GRAY
#define CONSOLE_COLOR_BG            CONSOLE_BLACK

#define WIDTH_SPLIT_NUMERATOR       5
#define WIDTH_SPLIT_DENOMINATOR     6

#define BAR_HEIGHT                  32

#define SPLIT_WIDTH(w)              \
    ((w) * WIDTH_SPLIT_NUMERATOR / WIDTH_SPLIT_DENOMINATOR)

#define PROGRESS_BAR_HELP                                       \
    "display progress bar:\n"                                   \
    "  progress_bar  <value>\n"                                 \
    "    value: precentage of progress bar\n"                   \
    "  e.g.: \n"                                                \
    "    progress_bar 0\n"                                      \
    "    progress_bar 99\n"                                     \
    "    progress_bar 100\n"

static void progress_bar_help(void)
{
    puts(PROGRESS_BAR_HELP);
}

#if defined(AIC_FB_ROTATE_EN) && !defined(AIC_GE_NORMAL)
#error progress bar rotation need enable GE normal mode
#endif

#ifdef AIC_BOOTLOADER_CMD_FB_CONSOLE
static u32 colour_fg = 0;
static u32 colour_bg = 0;
/*
 * The 16 colors supported by the console
 */
enum color_idx {
    CONSOLE_BLACK = 0,
    CONSOLE_RED,
    CONSOLE_GREEN,
    CONSOLE_BROWN,
    CONSOLE_BLUE,
    CONSOLE_MAGENTA,
    CONSOLE_CYAN,
    CONSOLE_LIGHT_GRAY,
    CONSOLE_GRAY,
    CONSOLE_LIGHT_RED,
    CONSOLE_LIGTH_GREEN,
    CONSOLE_YELLOW,
    CONSOLE_LIGHT_BLUE,
    CONSOLE_LIGHT_MAGENTA,
    CONSOLE_LIGHT_CYAN,
    CONSOLE_WHITE,

    CONSOLE_COLOR_COUNT
};

/*
 * Structure to describe a console color
 */
struct console_rgb {
    u32 r;
    u32 g;
    u32 b;
};
static const struct console_rgb colors[CONSOLE_COLOR_COUNT] = {
    { 0x00, 0x00, 0x00 },  /* black */
    { 0xc0, 0x00, 0x00 },  /* red */
    { 0x00, 0xc0, 0x00 },  /* green */
    { 0xc0, 0x60, 0x00 },  /* brown */
    { 0x00, 0x00, 0xc0 },  /* blue */
    { 0xc0, 0x00, 0xc0 },  /* magenta */
    { 0x00, 0xc0, 0xc0 },  /* cyan */
    { 0xc0, 0xc0, 0xc0 },  /* light gray */
    { 0x80, 0x80, 0x80 },  /* gray */
    { 0xff, 0x00, 0x00 },  /* bright red */
    { 0x00, 0xff, 0x00 },  /* bright green */
    { 0xff, 0xff, 0x00 },  /* yellow */
    { 0x00, 0x00, 0xff },  /* bright blue */
    { 0xff, 0x00, 0xff },  /* bright magenta */
    { 0x00, 0xff, 0xff },  /* bright cyan */
    { 0xff, 0xff, 0xff },  /* white */
};

static u32 console_color(struct aicfb_screeninfo *info, unsigned int idx)
{
    switch (info->format) {
    case MPP_FMT_RGB_565:
            return ((colors[idx].r >> 3) << 11) |
                   ((colors[idx].g >> 2) <<  5) |
                   ((colors[idx].b >> 3) <<  0);
        break;
    case MPP_FMT_ARGB_8888:
            return (colors[idx].r << 16) |
                   (colors[idx].g <<  8) |
                   (colors[idx].b <<  0);
        break;
    default:
        break;
    }

    /*
     * For unknown bit arrangements just support
     * black and white.
     */
    if (idx)
        return 0xffffff; /* white */

    return 0x000000; /* black */
}
#endif

void aicfb_draw_rect(struct aicfb_screeninfo *info,
                    unsigned int x, unsigned y,
                    unsigned int width, unsigned int height,
                    u8 red, u8 green, u8 blue)
{
#ifdef AIC_BOOTLOADER_SPI_LCD
    spi_lcd_fill_rect(info, x, y, width, height, red, green, blue);
    return;
#endif

#ifndef AIC_BOOTLOADER_CMD_ONLY_FB_CONSOLE
    int pbytes = info->bits_per_pixel / 8;
    unsigned char *fb;
    int i, j;

    fb = (unsigned char *)(info->framebuffer + y * info->stride + x * pbytes);

    switch (info->format) {
    case MPP_FMT_ARGB_8888:
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; j++) {
                *(fb++) = blue;
                *(fb++) = green;
                *(fb++) = red;
                *(fb++) = 0xFF;
            }

            fb += info->stride - width * pbytes;
        }
        break;
    case MPP_FMT_RGB_565:
        for (i = 0; i < height; ++i) {
            for (j = 0; j < width; j++) {
                *(uint16_t *)fb = ((red >> 3) << 11)
                        | ((green >> 2) << 5)
                        | (blue >> 3);
                fb += sizeof(uint16_t) / sizeof(*fb);
            }

            fb += info->stride - width * pbytes;
        }
        break;
    default:
        printf("%s: unsupported fb format %d", __func__, info->format);
        return;
    };

    aicos_dcache_clean_range((unsigned long *)info->framebuffer, info->smem_len);
#endif
}

#ifdef AIC_BOOTLOADER_CMD_FB_CONSOLE
static void aicfb_console_putc(struct aicfb_screeninfo *info,
        unsigned int x, unsigned int y, char ch)
{
    unsigned long dcache_size, dcache_start;
    int pbytes = info->bits_per_pixel / 8;
    int i, row;
    void *line;

    line = (unsigned char *)(info->framebuffer + y * info->stride + x * pbytes);
    dcache_start = ALIGN_DOWM((unsigned long)line, ARCH_DMA_MINALIGN);

    for (row = 0; row < VIDEO_FONT_HEIGHT; row++) {
        unsigned int idx = (ch - 32) * VIDEO_FONT_HEIGHT + row;
        uint32_t bits = video_fontdata[idx];

        switch (info->format) {
        case MPP_FMT_RGB_565:
        {
            uint16_t *dst = line;

            for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
                *dst++ = (bits & 0x80000000) ? colour_fg : colour_bg;
                bits <<= 1;
            }
            break;
        }
        case MPP_FMT_ARGB_8888:
        {
            uint32_t *dst = line;

            for (i = 0; i < VIDEO_FONT_WIDTH; i++) {
                *dst++ = (bits & 0x80000000) ? colour_fg : colour_bg;
                bits <<= 1;
            }
            break;
        }
        default:
            printf("%s: unsupported fb format %d", __func__, info->format);
            return;
        };
        line += info->stride;
    }

    dcache_size = ALIGN_UP((unsigned long)line - dcache_start,
            ARCH_DMA_MINALIGN);
    aicos_dcache_clean_range((unsigned long *)dcache_start, dcache_size);
}
#endif

static void aicfb_console_put_string(struct aicfb_screeninfo *info,
        unsigned int x, unsigned int y, const char *str)
{
#ifdef AIC_BOOTLOADER_CMD_FB_CONSOLE
    const char *s;
    int i;

    for (s = str, i = 0; *s; s++, i++)
        aicfb_console_putc(info, x + (i * VIDEO_FONT_WIDTH), y, *s);
#endif /* AIC_BOOTLOADER_CMD_FB_CONSOLE */

#ifdef AIC_BOOTLOADER_SPI_LCD
    if (str && *str) {
        spi_lcd_flush_region(x, y, strlen(str) * VIDEO_FONT_WIDTH,
                             VIDEO_FONT_HEIGHT, info->framebuffer);
    }
#endif
}

static void console_set_default_colors(struct aicfb_screeninfo *info)
{
#ifdef AIC_BOOTLOADER_CMD_FB_CONSOLE
    int fore, back;

    fore = CONSOLE_COLOR_FG;
    back = CONSOLE_COLOR_BG;

    colour_fg = console_color(info, fore);
    colour_bg = console_color(info, back);
#endif
}

static void progress_bar_flush_black(struct aicfb_screeninfo *info)
{
#ifndef AIC_BOOTLOADER_CMD_ONLY_FB_CONSOLE
    memset(info->framebuffer, 0x00, info->smem_len);
    aicos_dcache_clean_range((unsigned long *)info->framebuffer, info->smem_len);
#endif

#ifdef AIC_BOOTLOADER_SPI_LCD
    spi_lcd_set_window(0, 0, info->width - 1, info->height - 1);
    spi_lcd_flush(info->framebuffer, info->smem_len);
#else
#ifndef AIC_BOOTLOADER_CMD_ONLY_FB_CONSOLE
    aicfb_draw_rect(info, 0, 0, info->width, info->height, 0, 0, 0);
#endif
#if defined(AIC_FB_ROTATE_EN) && !defined(AIC_BOOTLOADER_SPI_LCD)
    {
        int index = 0;

        aicfb_ioctl(AICFB_PAN_DISPLAY, &index);
    }
#endif
#endif
}

void aicfb_draw_bar(unsigned int value)
{
    struct aicfb_screeninfo info;
    unsigned int bar_x, bar_y, width, height;
    unsigned int console_x, console_y;
    static bool power_on = false;
#ifdef AIC_FB_ROTATE_EN
    int index = 0;
#endif
    char str[5];

#ifdef AIC_BOOTLOADER_SPI_LCD
    spi_lcd_init();
    spi_lcd_get_info(&info);
#else
    aicfb_probe();

    if (aicfb_ioctl(AICFB_GET_SCREENINFO, &info) < 0)
        return;
#endif

    if (!power_on) {
        console_set_default_colors(&info);
        progress_bar_flush_black(&info);
#ifdef AIC_BOOTLOADER_SPI_LCD
        spi_lcd_backlight_on();
#else
        aicfb_ioctl(AICFB_POWERON, 0);
#endif
        power_on = true;
    }

    width     = SPLIT_WIDTH(info.width);
    height    = BAR_HEIGHT;
    bar_x     = (info.width - width) / 2;
#ifdef AIC_BOOTLOADER_SPI_LCD
    bar_y     = (info.height - BAR_HEIGHT - VIDEO_FONT_HEIGHT - 5) / 2;
#else
    bar_y     = 0;
#endif
    console_x = info.width / 2;
    console_y = bar_y + BAR_HEIGHT + 5;

    if (value == 0) {
#if defined(AIC_FB_ROTATE_EN) && defined(AIC_GE_NORMAL)
        hal_ge_init();
#endif
        console_x -= VIDEO_FONT_WIDTH;
        aicfb_draw_rect(&info, bar_x, bar_y, width, height, BAR_BACKGROUND_COLOR);
        aicfb_console_put_string(&info, console_x, console_y, "0%");
#if defined(AIC_FB_ROTATE_EN) && !defined(AIC_BOOTLOADER_SPI_LCD)
        aicfb_ioctl(AICFB_PAN_DISPLAY, &index);
#endif
        return;
    }

    if (value < 100)
        width = width * value / 100;

    aicfb_draw_rect(&info, bar_x, bar_y, width, height, BAR_FILL_COLOR);

    if (value == 100) {
        console_x -= VIDEO_FONT_WIDTH * 2;
        aicfb_console_put_string(&info, console_x, console_y, "100%");
#if defined(AIC_FB_ROTATE_EN) && !defined(AIC_BOOTLOADER_SPI_LCD)
        aicfb_ioctl(AICFB_PAN_DISPLAY, &index);
#endif
        return;
    }

    if (value < 10)
        console_x -= VIDEO_FONT_WIDTH;
    else
        console_x -= VIDEO_FONT_HEIGHT + (VIDEO_FONT_WIDTH >> 1);

    snprintf(str, sizeof(str), "%d%%", value);
    aicfb_console_put_string(&info, console_x, console_y, str);
#if defined(AIC_FB_ROTATE_EN) && !defined(AIC_BOOTLOADER_SPI_LCD)
    aicfb_ioctl(AICFB_PAN_DISPLAY, &index);
#endif
}

static int do_progress_bar(int argc, char *argv[])
{
    unsigned int intf;

    if (argc != 2)
        goto help;

    intf = strtol(argv[1], NULL, 0);
    aicfb_draw_bar(intf);
    return 0;

help:
    progress_bar_help();
    return 0;
}

CONSOLE_CMD(progress_bar, do_progress_bar, "Display progress bar.");
