/*
 * Copyright (c) 2020-2025 ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
*/

#include <stddef.h>
#include "ve.h"
#include "ve_top_register.h"
#include "../png/png_hal.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "aic_core.h"
#include "mpp_zlib.h"

#ifdef AIC_VE_DRV_V10
#define USE_IRQ (0)
#else
#define USE_IRQ (1)
#endif

struct mpp_zlib_ctx {
    unsigned long lz77_addr;
    unsigned long reg_base;
};

static void config_ve_top(unsigned long reg_base, int use_irq)
{
    write_reg_u32(reg_base + VE_CLK_REG, 1);
    write_reg_u32(reg_base + VE_RST_REG, 0);

    while ((read_reg_u32(reg_base + VE_RST_REG) >> 16)) {
    }

    write_reg_u32(reg_base + VE_INIT_REG, 1);
    write_reg_u32(reg_base + VE_IRQ_REG, use_irq);
    write_reg_u32(reg_base + VE_PNG_EN_REG, 1);
}

int mpp_zlib_uncompressed(
        unsigned char *compressed_data,
        unsigned int compressed_len,
        unsigned char *uncompressed_data,
        unsigned int uncompressed_len)
{
    int ret = 0;
    int real_uncompress_len = -1;
    int offset = 0;
    unsigned int status;
    unsigned long reg_base;
    unsigned long lz77_buf;
    unsigned long lz77_buf_align_8;

    if (compressed_data  == NULL
        || compressed_len ==0
        || uncompressed_data == NULL
        || uncompressed_len == 0 ) {
            loge("param error!!!\n");
            return -1;
    }
    ret = ve_open_device();
    if (ret != 0) {
        loge("ve_open_device error!!!\n");
        return -1;
    }

    lz77_buf = (unsigned long)aicos_malloc(MEM_CMA, (32*1024+7));

    if (lz77_buf == 0) {
        loge("mpp_alloc error!!!\n");
        ve_close_device();
        return -1;
    }

    lz77_buf_align_8 = ((lz77_buf+7)&(~7));
    logd("lz77_buf:0x%lx,lz77_buf_align_8:0x%lx\n", lz77_buf, lz77_buf_align_8);

    offset = 2;
    logd("offset:%d\n",offset);
    reg_base = ve_get_reg_base();
    ve_get_client();
    config_ve_top(reg_base, USE_IRQ);
    write_reg_u32(reg_base+PNG_CTRL_REG, 0);
    write_reg_u32(reg_base+OUTPUT_BUFFER_ADDR_REG, (unsigned long)uncompressed_data);
    write_reg_u32(reg_base+OUTPUT_BUFFER_LENGTH_REG, (unsigned long)uncompressed_data + uncompressed_len -1);
    write_reg_u32(reg_base+INFLATE_WINDOW_BUFFER_ADDR_REG, lz77_buf_align_8);
    write_reg_u32(reg_base+INFLATE_INTERRUPT_REG, 15);
    write_reg_u32(reg_base+INFLATE_STATUS_REG, 15);
    write_reg_u32(reg_base+INFLATE_START_REG, 1);
    write_reg_u32(reg_base+INPUT_BS_START_ADDR_REG, (unsigned long)compressed_data);
    write_reg_u32(reg_base+INPUT_BS_END_ADDR_REG, (unsigned long)compressed_data+compressed_len-1);
    write_reg_u32(reg_base+INPUT_BS_OFFSET_REG, offset*8);
    write_reg_u32(reg_base+INPUT_BS_LENGTH_REG, (compressed_len-offset)*8);
    write_reg_u32(reg_base+INPUT_BS_DATA_VALID_REG, 0x80000003);

#if USE_IRQ
    if (ve_wait(&status) < 0) {
        loge("timeout");
        goto _exit;
    }
    if (status & PNG_ERROR) {
        loge("decode error, status: %08x", status);
        goto _exit;
    } else if (status & PNG_FINISH) {
        logd("decode ok, status: %08x", status);
    } else {
        loge("decode error, status: %08x", status);
        goto _exit;
    }
#else
    while ((read_reg_u32(reg_base + OUTPUT_COUNT_REG) != uncompressed_len)
        && ((read_reg_u32(reg_base + INFLATE_STATE_REG) & 0x7800) != 0x800)) {
            status = read_reg_u32(reg_base + INFLATE_STATUS_REG);
            if (status & PNG_ERROR) {
                goto _exit;
            }

            usleep(1000);
    }
#endif

    real_uncompress_len = read_reg_u32(reg_base+OUTPUT_COUNT_REG);
    logd("real_uncompress_len:%d\n",real_uncompress_len);

_exit:
    ve_put_client();
    ve_close_device();
    if (lz77_buf) {
        aicos_free(MEM_CMA, (void *)lz77_buf);
    }
    return real_uncompress_len;
}

static unsigned int read_le16(const unsigned char *p)
{
	return ((unsigned int) p[0])
		| ((unsigned int) p[1] << 8);
}

static int get_gzip_header(unsigned char *src)
{
	unsigned char flg;
	unsigned char *start;
	flg = src[3];
	start = src + 10;

	if(flg & 4) { // FEXTRA
		unsigned int xlen = read_le16(start);
		start += (xlen + 2);
	}

	if(flg & 8) { // FNAME
		do {
		} while(*start++);
	}

	if(flg & 16) { // FCOMMENT
		do {
		} while(*start++);
	}

	if(flg & 2) { // FHCRC
		start += 2;
	}

	return start - src;
}

int mpp_unzip_uncompressed(void *ctx, enum COMPRESS_TYPE type,
        unsigned char *src_buf, unsigned int src_buf_len,
        unsigned int data_offset, unsigned int data_len,
        unsigned char *dst, unsigned int dst_len,
        int first_part, int last_part)
{
    struct mpp_zlib_ctx *p = (struct mpp_zlib_ctx*)ctx;
    int offset = 0;
    unsigned int status;
    int real_uncompress_len;

    //printf("src_len: %d, first: %d, last: %d\n", src_len, first_part, last_part);
    if (first_part) {
        if (type == MPP_GZIP)
            offset = get_gzip_header(src_buf);
        else if (type == MPP_ZLIB)
            offset = 2;
        else
            loge("unkown type(%d), maybe error", type);
        write_reg_u32(p->reg_base+PNG_CTRL_REG, 0);
        write_reg_u32(p->reg_base+OUTPUT_BUFFER_ADDR_REG, (unsigned long)dst);
        write_reg_u32(p->reg_base+OUTPUT_BUFFER_LENGTH_REG, (unsigned long)dst + dst_len -1);
        write_reg_u32(p->reg_base+INFLATE_WINDOW_BUFFER_ADDR_REG, p->lz77_addr);
        write_reg_u32(p->reg_base+INFLATE_INTERRUPT_REG, 15);
        write_reg_u32(p->reg_base+INFLATE_STATUS_REG, 15);
        write_reg_u32(p->reg_base+INFLATE_START_REG, 1);
    }

    // enable irq
    write_reg_u32(p->reg_base + VE_IRQ_REG, 1);
    write_reg_u32(p->reg_base+INPUT_BS_START_ADDR_REG, (unsigned long)src_buf);
    write_reg_u32(p->reg_base+INPUT_BS_END_ADDR_REG, (unsigned long)src_buf+src_buf_len-1);
    write_reg_u32(p->reg_base+INPUT_BS_OFFSET_REG, (data_offset+offset)*8);
    write_reg_u32(p->reg_base+INPUT_BS_LENGTH_REG, (data_len-offset)*8);
    write_reg_u32(p->reg_base+INPUT_BS_DATA_VALID_REG, 0x80000000 | (last_part<<1) | first_part);

    if (ve_wait(&status) < 0) {
        loge("timeout");
        return -1;
    }

    if (status & PNG_ERROR) {
        loge("decode error, status: %08x", status);
        return -1;
    }

    if (last_part) {
        if (status & PNG_FINISH) {
            logi("finish");
        } else {
            loge("error, status: %08x", status);
            return -1;
        }
    } else {
        if (status & PNG_BITREQ) {
            logd("bit request: %08x", status);
        } else {
            loge("error, status: %08x", status);
            return -1;
        }
    }

    real_uncompress_len = read_reg_u32(p->reg_base+OUTPUT_COUNT_REG);
    return real_uncompress_len;
}

void mpp_unzip_destroy(void* ctx)
{
    struct mpp_zlib_ctx *p = (struct mpp_zlib_ctx*)ctx;

    ve_put_client();
    ve_close_device();
    aicos_free_align(MEM_CMA, (void*)p->lz77_addr);
    free(p);
}

void* mpp_unzip_create(void)
{
    int ret;
    struct mpp_zlib_ctx *p = (struct mpp_zlib_ctx*)malloc(sizeof(struct mpp_zlib_ctx));
    if (p == NULL) {
        return NULL;
    }
    memset(p, 0, sizeof(struct mpp_zlib_ctx));

    ret = ve_open_device();
    if (ret != 0) {
        loge("ve_open_device error!!!\n");
        goto err;
    }

    p->reg_base = ve_get_reg_base();
    ve_get_client();
    config_ve_top(p->reg_base, 1);

    p->lz77_addr = (unsigned long)aicos_malloc_align(MEM_CMA, 32*1024, CACHE_LINE_SIZE);
    if (p->lz77_addr == 0) {
        loge("lz77 buffer malloc failed");
        goto err;
    }
    aicos_dcache_clean_invalid_range((void*)p->lz77_addr, 32*1024);

    return (void*)p;

err:
    ve_close_device();
    if (p)
        free(p);

    return NULL;
}
