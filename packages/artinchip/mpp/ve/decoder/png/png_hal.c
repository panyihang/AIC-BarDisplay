/*
* Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
*
*  SPDX-License-Identifier: Apache-2.0
*  author: <qi.xu@artinchip.com>
*  Desc: png register configure
*
*/

#define LOG_TAG "png_hal"

#include <aic_core.h>
#include "png_hal.h"
#include "ve.h"
#include "ve_top_register.h"
#include "mpp_log.h"

#ifdef DEBUG_REGS
static void dump_regs(struct png_dec_ctx *s)
{
	u32 i = 0;

	for(i = 0; i< 0xff; i+=4) {
		logi("addr: %08x, val: %08x", i, read_reg_u32(s->regs_base + PNG_REG_OFFSET_ADDR + i));
	}
}
#endif

void config_ve_top_reg(struct png_dec_ctx *s)
{
	int i;
	u32 val;

	write_reg_u32(s->regs_base + VE_CLK_REG, 1);

	// [9:8]: pic module reset; [5:4]: png module reset
	write_reg_u32(s->regs_base + VE_RST_REG, RST_PNG_PIC_MODULE);

	for (i = 0; i < 10; i++) {
		val = read_reg_u32(s->regs_base + VE_RST_REG);
		if ((val>>16) == 0)
			break;
	}

	write_reg_u32(s->regs_base + VE_INIT_REG, 1);
	write_reg_u32(s->regs_base + VE_IRQ_REG, 1);
	write_reg_u32(s->regs_base + VE_PNG_EN_REG, 1);
}

void png_reset(struct png_dec_ctx *s)
{
	write_reg_u32(s->regs_base + INFLATE_RESET_REG, 1);
}

static int set_bitstream_and_wait(struct png_dec_ctx *s, unsigned char *data, int length)
{
	int offset = s->vbv_offset;
	int is_first = 1;
	int is_last = 1;
	unsigned int status = 0;

	struct png_register_list *reg_list = (struct png_register_list *)s->reg_list;
	u32 *pval;
	u32 val;

	logd("config bit stream");
	if (s->decode_next_dat) {
		val = s->curr_packet->phy_base + s->curr_packet->phy_offset;
		logd("apng: start_addr:%08x, offset:%d", val, offset);
	} else {
		val = s->idat_mpp_buf->phy_addr;
	}
	write_reg_u32(s->regs_base + INPUT_BS_START_ADDR_REG, val);

	if (s->decode_next_dat) {
		val = s->curr_packet->phy_base + s->curr_packet->phy_offset + s->curr_packet->size - 1;
		logd("apng: end_addr:%08x", val);
	} else {
		val = s->idat_mpp_buf->phy_addr + s->idat_mpp_buf->size - 1;
	}

	write_reg_u32(s->regs_base + INPUT_BS_END_ADDR_REG, val);

	write_reg_u32(s->regs_base + INPUT_BS_OFFSET_REG, offset * 8);
	write_reg_u32(s->regs_base + INPUT_BS_LENGTH_REG, length * 8);

	pval = (u32 *)&reg_list->_48_inflate_valid;
	reg_list->_48_inflate_valid.first = is_first;
	reg_list->_48_inflate_valid.last = is_last;
	reg_list->_48_inflate_valid.valid = 1;
	write_reg_u32(s->regs_base + INPUT_BS_DATA_VALID_REG, *pval);

	// wait IRQ
	if(ve_wait(&status) < 0) {
		aicos_msleep(10);
		ve_reset();
		loge("png timeout");
		return -1;
	}

	if(status & PNG_ERROR) {
		loge("png decode error, status: %08x", status);
		aicos_msleep(10);
		ve_reset();
		return -1;
	} else if(status & PNG_FINISH) {
		return 0;
	} else if(status & PNG_BITREQ) {
		loge("png bit request, not support now");
		// TODO
		return -1;
	}

	return 0;
}

static void png_set_clip_info(struct png_dec_ctx *s)
{
    if(s->decoder.crop_en == 0)
        return;

#ifndef AIC_VE_DRV_V10
    write_reg_u32(s->regs_base + PNG_CLIP_BASE_REG, s->decoder.crop_x << 16 | s->decoder.crop_y);
    if (s->decoder.crop_width > 0 && s->decoder.crop_height > 0) {
        write_reg_u32(s->regs_base + PNG_CLIP_SIZE_REG,
                      (1U << 31) | (s->decoder.crop_width << 16) | s->decoder.crop_height);
    }
#endif
}

int png_hardware_decode(struct png_dec_ctx *s, unsigned char *buf, int length)
{
    struct png_register_list *reg_list = (struct png_register_list *)s->reg_list;
    u32 *pval;
    u32 val;

    memset(reg_list, 0, sizeof(struct png_register_list));
    s->hw_size = s->stride * s->height;

    ve_get_client();

    // 1. reset ve
    config_ve_top_reg(s);
    png_reset(s);

    // 2.set png info
    pval = (u32 *)&reg_list->_10_png_ctrl;
    reg_list->_10_png_ctrl.bit_depth = s->bit_depth;
    reg_list->_10_png_ctrl.color_type = s->color_type;
    reg_list->_10_png_ctrl.dec_type = 1;
    write_reg_u32(s->regs_base + PNG_CTRL_REG, *pval);

    // 3. set picture size
    pval = (u32 *)&reg_list->_14_png_size;
    if (s->decode_next_dat) {
        reg_list->_14_png_size.width = s->cur_w;
        reg_list->_14_png_size.height = s->cur_h;
    } else {
        reg_list->_14_png_size.width = s->width;
        reg_list->_14_png_size.height = s->height;
    }
    write_reg_u32(s->regs_base + PNG_SIZE_REG, *pval);

    write_reg_u32(s->regs_base + PNG_STRIDE_REG, s->curr_frame->mpp_frame.buf.stride[0]);

    int format = 0;
    if(s->pix_fmt == MPP_FMT_RGBA_8888)
        format = RGBA8888;
    else if(s->pix_fmt == MPP_FMT_BGRA_8888)
        format = BGRA8888;
    else if(s->pix_fmt == MPP_FMT_ABGR_8888)
        format = ABGR8888;
    else if(s->pix_fmt == MPP_FMT_ARGB_8888)
        format = ARGB8888;
    else if(s->pix_fmt == MPP_FMT_BGR_888)
        format = BGR888;
    else if(s->pix_fmt == MPP_FMT_RGB_888)
        format = RGB888;
    else if(s->pix_fmt == MPP_FMT_BGR_565)
        format = BGR565;
    else if(s->pix_fmt == MPP_FMT_RGB_565)
        format = RGB565;

    write_reg_u32(s->regs_base + PNG_FORMAT_REG, format);

    // 4. set output buffer
    int channel = (s->pix_fmt == MPP_FMT_BGR_888 || s->pix_fmt == MPP_FMT_RGB_888) ? 3 : 4;
    int stride = s->curr_frame->mpp_frame.buf.stride[0];
    int out_offset = 0;

    logd("decode_next_dat %d, x_offset %d, y_offset %d\n",
        s->decode_next_dat, s->x_offset, s->y_offset);

    if (s->decode_next_dat) {
        out_offset = s->y_offset * stride + s->x_offset * channel;
    } else {
        out_offset = s->decoder.output_y * stride + s->decoder.output_x * channel;
    }

    val = s->curr_frame->phy_addr[0] + out_offset;
    write_reg_u32(s->regs_base + OUTPUT_BUFFER_ADDR_REG, val);

#ifdef AIC_VE_DRV_V10
    write_reg_u32(s->regs_base + OUTPUT_BUFFER_LENGTH_REG, s->height * stride);
#else
    if (s->decode_next_dat) {
        val = s->curr_frame->phy_addr[0] + (s->y_offset + s->cur_h) * stride;
    } else {
        val = s->curr_frame->phy_addr[0] + (s->decoder.output_y + s->height) * stride;
    }
    write_reg_u32(s->regs_base + OUTPUT_BUFFER_LENGTH_REG, val);
#endif

    // 5. set LZ77 buffer 32K
    val = s->lz77_mpp_buf->phy_addr;
    write_reg_u32(s->regs_base + INFLATE_WINDOW_BUFFER_ADDR_REG, val);

    // PNG filter line buffer address
    val = s->filter_mpp_buf->phy_addr;
    write_reg_u32(s->regs_base + PNG_FILTER_LINE_BUF_ADDR_REG, val);

    // 6. set memory register for palette
    if (s->color_type == PNG_COLOR_TYPE_PALETTE) {
        unsigned char* palette_buf = (unsigned char*)&s->palette;
        memcpy(s->palette_mpp_buf->vir_addr, palette_buf, 256 * 4);
        ve_buffer_sync(s->palette_mpp_buf, CACHE_CLEAN);

        val = s->palette_mpp_buf->phy_addr;
        write_reg_u32(s->regs_base + PNG_PNG_PALETTE_ADDR_REG, val);
    }

    // 7. set clip info
    png_set_clip_info(s);

    // 8. decode start
    logd("config start");
    write_reg_u32(s->regs_base + INFLATE_INTERRUPT_REG, 15);
    write_reg_u32(s->regs_base + INFLATE_STATUS_REG, 15);
    write_reg_u32(s->regs_base + INFLATE_START_REG, 7);

    // 9.set bitstream
    if (set_bitstream_and_wait(s, buf, length)) {
        ve_put_client();
        return -1;
    }

    val = read_reg_u32(s->regs_base + PNG_COUNT_REG);
    logi("png clock: %d", val);

    // disable png module
    write_reg_u32(s->regs_base + VE_PNG_EN_REG, 0);
    ve_put_client();

    return 0;
}
