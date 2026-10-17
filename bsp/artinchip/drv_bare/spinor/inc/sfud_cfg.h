/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the configure head file for this library.
 * Created on: 2016-04-23
 */

#ifndef _SFUD_CFG_H_
#define _SFUD_CFG_H_
#include <rtconfig.h>

#ifdef AIC_SPINOR_SFUD_DEBUG
#define SFUD_DEBUG_MODE
#define SFUD_INFO(fmt, ...)  printf(fmt"\n", ##__VA_ARGS__)
#define SFUD_DEBUG(fmt, ...)  printf(fmt"\n", ##__VA_ARGS__)
#else
#define SFUD_INFO(...)   do {} while (0)
#define SFUD_DEBUG(...)   do {} while (0)
#endif
#define SFUD_USING_SFDP

#define SFUD_USING_QSPI


#ifdef AIC_DISABLE_SFUD_WP_DEBUG
#define SFUD_WP_INFO(...)   do {} while (0)
#else
#define SFUD_WP_INFO(fmt, ...)  printf(fmt"\n", ##__VA_ARGS__)
#endif

/**
 * Using probe flash JEDEC ID then query defined supported flash chip information table. @see SFUD_FLASH_CHIP_TABLE
 */
#if defined(RT_SFUD_USING_FLASH_INFO_TABLE) || defined(BOOTLOADER_SFUD_USING_FLASH_INFO_TABLE)
#define SFUD_USING_FLASH_INFO_TABLE
#endif

#define SFUD_FLASH_DEVICE_TABLE {{0}}

#endif /* _SFUD_CFG_H_ */
