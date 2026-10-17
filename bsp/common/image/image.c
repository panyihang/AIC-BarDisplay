/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_image.h>

int image_verify_magic(u8 *fw_base, u32 magic)
{
    u32 fw_magic;

    memcpy(&fw_magic, fw_base, 4);
    if (fw_magic == magic)
        return 0;

    return (-1);
}

u32 image_calc_checksum(u8 *buf, u32 size)
{
    u32 i, val, sum, rest, cnt;
    u8 *p;
    u32 *p32;

    p = buf;
    i = 0;
    sum = 0;
    cnt = size >> 2;

    if ((unsigned long)buf & 0x3) {
        for (i = 0; i < cnt; i++) {
            p = &buf[i * 4];
            val = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
            sum += val;
        }
    } else {
        p32 = (u32 *)buf;
        for (i = 0; i < cnt; i++) {
            sum += *p32;
            p32++;
        }
    }

    /* Calculate not 32 bit aligned part */
    rest = size - (cnt << 2);
    p = &buf[cnt * 4];
    val = 0;
    for (i = 0; i < rest; i++)
        val += (p[i] << (i * 8));
    sum += val;

    return sum;
}

/*
 * This function is used to verify the firmware checksum value.
 *
 * When generating firmware checksum, algorithm is like this:
 * 1. Set firmware header's checksum field to 0
 * 2. Then calculate firmware's 32-bit add sum value
 * 3. checksum value is the invert of 32-bit add sum value
 *
 * When verify checksum
 * 1. Just calcuate 32-bit add sum value
 * 2. The value + 1 should be 0
 *
 * How to process non 32-bit aligned bytes: padding 0 bytes to make it aligned.
 */
int image_verify_checksum(u8 *fw_base, u32 size)
{
    u32 sum;

    sum = image_calc_checksum(fw_base, size);
    if ((sum + 1) != 0) {
        printf("sum is 0x%X\n", sum);
        return (-1);
    }
    return 0;
}

int image_authentication(u8 *fw_base)
{
    return 0;
}

void *image_get_entry_point(u8 *fw_base)
{
    void *ep = NULL;
    struct image_header fw;

    memcpy(&fw, fw_base, sizeof(struct image_header));
    ep = (void *)(unsigned long)fw.entry_point;

    return ep;
}

void *image_get_load_address(u8 *fw_base)
{
    void *la = NULL;
    struct image_header fw;

    memcpy(&fw, fw_base, sizeof(struct image_header));
    la = (void *)(unsigned long)fw.load_address;

    return la;
}
