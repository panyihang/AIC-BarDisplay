/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <rtconfig.h>

#define LV_USE_LOG 1
#if LV_USE_LOG

    /*How important log should be added:
    *LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
    *LV_LOG_LEVEL_INFO        Log important events
    *LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
    *LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
    *LV_LOG_LEVEL_USER        Only logs added by the user
    *LV_LOG_LEVEL_NONE        Do not log anything*/
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /*1: Print the log with 'printf';
    *0: User need to register a callback with `lv_log_register_print_cb()`*/
    #define LV_LOG_PRINTF 1

    /*Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs*/
    /* Use LVGL's default trace flags for the selected warning log level. */

#endif  /*LV_USE_LOG*/

#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH          32
#endif

#define LV_USE_MEM_MONITOR 0
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif
#define LV_USE_SYSMON   1
#define LV_INDEV_DEF_READ_PERIOD 10

#if defined(KERNEL_BAREMETAL)|| defined(KERNEL_FREERTOS)
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB
#endif

#if defined(KERNEL_BAREMETAL) || defined(KERNEL_FREERTOS)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE <aic_common.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (aic_get_time_ms())    /*Expression evaluating to current system time in ms*/
#define LV_DISP_DEF_REFR_PERIOD 10
#endif

#if defined(LV_USE_STDLIB_MALLOC)
// #undef LV_USE_STDLIB_MALLOC
// #define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#endif
#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    /*Size of the memory available for `lv_malloc()` in bytes (>= 2kB)*/
    #define LV_MEM_SIZE (1024U * 1024U)          /*[bytes]*/

    /*Size of the memory expand for `lv_malloc()` in bytes*/
    #define LV_MEM_POOL_EXPAND_SIZE 0

    /*Set an address for the memory pool instead of allocating it as a normal array. Can be in external SRAM too.*/
    #define LV_MEM_ADR 0     /*0: unused*/
    /*Instead of an address give a memory allocator that will be called to get a memory pool for LVGL. E.g. my_malloc*/
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#endif  /*LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN*/

#define LV_USE_FS_POSIX 1
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER 'L'     /*Set an upper cased letter on which the drive will accessible (e.g. 'A')*/
    #define LV_FS_POSIX_PATH ""         /*Set the working directory. File/directory paths will be appended to it.*/
    #define LV_FS_POSIX_CACHE_SIZE 0    /*>0 to cache this number of bytes in lv_fs_read()*/
#endif

#if defined(AIC_LVGL_DEMO_BENCHMARK)
#define LV_USE_DEMO_BENCHMARK 1
#undef LV_FONT_MONTSERRAT_24
#define LV_FONT_MONTSERRAT_24 1
#undef LV_USE_DEMO_WIDGETS
#define LV_USE_DEMO_WIDGETS 1
#endif

#if defined(AIC_LVGL_DEMO_WIDGETS)
#define LV_USE_DEMO_WIDGETS 1
#endif

#define LV_USE_DEMO_MUSIC 0
#define LV_FONT_MONTSERRAT_12       1
#define LV_FONT_MONTSERRAT_16       1

/*FreeType library*/
#define LV_USE_FREETYPE 0
#if LV_USE_FREETYPE
    /*Let FreeType to use LVGL memory and file porting*/
    #define LV_FREETYPE_USE_LVGL_PORT 0

    /*Cache count of the glyphs in FreeType. It means the number of glyphs that can be cached.
     *The higher the value, the more memory will be used.*/
    #define LV_FREETYPE_CACHE_FT_GLYPH_CNT 256
#endif

#define LV_USE_GIF 1

#ifndef LV_CACHE_DEF_SIZE
#define LV_CACHE_DEF_SIZE 8 * 1024 * 1024
#endif

#ifndef LV_IMAGE_HEADER_CACHE_DEF_CNT
#define LV_IMAGE_HEADER_CACHE_DEF_CNT  20
#endif

#define LV_DEF_REFR_PERIOD 10

#define LV_USE_PARALLEL_DRAW_DEBUG 0

#define LV_DRAW_BUF_STRIDE_ALIGN 8

#define LV_DRAW_BUF_ALIGN CACHE_LINE_SIZE

#define LV_USE_MONKEY 1

#define LV_USE_PROFILER 0
#if LV_USE_PROFILER
#define LV_USE_PROFILER_BUILTIN 1
#define LV_PROFILER_BUILTIN_BUF_SIZE (256 * 1024)
#define LV_PROFILE_INIT_SLEEP_MS 3000
#define LV_PROFILE_START_TIME_MS 0
#define LV_PROFILE_END_TIME_MS   20000
#define LV_PROFILE_FILE "/udisk/profiler.log"
#endif

/* Keep the USB display application's LVGL footprint small. */
#if defined(APP_USB_DISP)
#undef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 32
#undef LV_USE_LOG
#define LV_USE_LOG 0
#undef LV_USE_SYSMON
#define LV_USE_SYSMON 0
#undef LV_USE_GIF
#define LV_USE_GIF 0
#undef LV_USE_MONKEY
#define LV_USE_MONKEY 0
#undef LV_CACHE_DEF_SIZE
#define LV_CACHE_DEF_SIZE 0
#undef LV_IMAGE_HEADER_CACHE_DEF_CNT
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 1
#define LV_USE_BAR 0
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_IMAGE 0
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0
#define LV_USE_FLEX 0
#define LV_USE_GRID 0
#define LV_USE_FS_STDIO 0
#undef LV_USE_FS_POSIX
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_LITTLEFS 0
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_TJPGD 0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_RLE 0
#define LV_USE_QRCODE 0
#define LV_USE_BARCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_TINY_TTF 0
#endif

// should at the end of lv_conf.h
#if defined(LV_USE_CONF_CUSTOM)
#include "lv_conf_custom.h"
#endif

#endif // LV_CONF_H
