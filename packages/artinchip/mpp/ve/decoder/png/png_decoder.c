/*
* Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
*
*  SPDX-License-Identifier: Apache-2.0
*  author: <qi.xu@artinchip.com>
*  Desc: png decode
*
*/

#define LOG_TAG "png_decoder"

#include <aic_core.h>
#include <stdlib.h>
#include <inttypes.h>
#include "mpp_codec.h"
#include "bytestream.h"
#include "png_decoder.h"
#include "png_hal.h"
#include "ve.h"
#include "mpp_mem.h"
#include "mpp_log.h"


#define SHOW_TAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))

static int png_get_nb_channels(int color_type)
{
    int channels;
    channels = 1;
    if ((color_type & (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)) ==
        PNG_COLOR_MASK_COLOR)
    channels = 3;
    if (color_type & PNG_COLOR_MASK_ALPHA)
        channels++;
    return channels;
}

static int alloc_phy_buffer(struct png_dec_ctx *s, int width, int height)
{
    size_t bitstream_size;

    // already alloc buffer
    if(s->idat_mpp_buf)
        return 0;

    if (width <= 0 || height <= 0)
        return -1;

    //int channels = png_get_nb_channels(s->color_type);

    logi("alloc mpp buffer, width %d, height %d, channel: %d, bitstream size %d",
        width, height, png_get_nb_channels(s->color_type), s->packet_size);

    bitstream_size = s->packet_size;
    if (s->color_type == PNG_COLOR_TYPE_PALETTE) {
        s->palette_mpp_buf = ve_buffer_alloc(s->ve_buf_handle, PALLETE_SIZE, ALLOC_NEED_VIR_ADDR);
        if(s->palette_mpp_buf == NULL) {
            loge("alloc palette buffer failed");
            return -1;
        }
    }

#if defined(AIC_CHIP_D12X)
    s->lz77_mpp_buf = (struct ve_buffer *)mpp_alloc(sizeof(struct ve_buffer));
    s->lz77_mpp_buf->phy_addr = 0x30040000;
    aicos_dcache_clean_invalid_range((unsigned long *)(ulong)0x30040000, LZ77_WINDOW_SIZE);
#else
    s->lz77_mpp_buf = ve_buffer_alloc(s->ve_buf_handle, LZ77_WINDOW_SIZE, 0);
#endif

    s->filter_mpp_buf = ve_buffer_alloc(s->ve_buf_handle, width * 4, 0);
    s->idat_mpp_buf = ve_buffer_alloc(s->ve_buf_handle, bitstream_size, ALLOC_NEED_VIR_ADDR);

    if (!s->idat_mpp_buf || !s->lz77_mpp_buf || !s->filter_mpp_buf) {
        loge("alloc mpp buffer failed!");
        return -1;
    }

    return 0;
}

static int free_phy_buffer(struct png_dec_ctx *s) {
    if (s->idat_mpp_buf) {
        ve_buffer_free(s->ve_buf_handle, s->idat_mpp_buf);
        s->idat_mpp_buf = NULL;
    }

    if (s->palette_mpp_buf) {
        ve_buffer_free(s->ve_buf_handle, s->palette_mpp_buf);
        s->palette_mpp_buf = NULL;
    }

#if defined(AIC_CHIP_D12X)
    if (s->lz77_mpp_buf) {
        mpp_free(s->lz77_mpp_buf);
        s->lz77_mpp_buf = NULL;
    }
#else
    if (s->lz77_mpp_buf) {
        ve_buffer_free(s->ve_buf_handle, s->lz77_mpp_buf);
        s->lz77_mpp_buf = NULL;
    }
#endif

    if (s->filter_mpp_buf) {
        ve_buffer_free(s->ve_buf_handle, s->filter_mpp_buf);
        s->filter_mpp_buf = NULL;
    }

    return 0;
}

static int png_decode_idat(struct png_dec_ctx *s, int length)
{
    int left_bytes = bytestream2_get_bytes_left(&s->gb);
    int avail = length < left_bytes ? length : left_bytes;
    const unsigned char *next = s->gb.buffer;

    if (s->decode_next_dat) {
        s->fdat_buf = (unsigned char *)s->gb.buffer;
        s->idat_data_size = avail;
    } else {
        if (s->idat_mpp_buf) {
            memcpy(s->idat_mpp_buf->vir_addr + s->idat_data_size, next, avail);
            ve_buffer_sync(s->idat_mpp_buf, CACHE_CLEAN);

            s->idat_data_size += avail;
            logd("idat_mpp_buf %p, length: %d", s->idat_mpp_buf->vir_addr, length);
        }
    }


    bytestream2_skip(&s->gb, length);

    return 0;
}

static int decode_ihdr_chunk(struct png_dec_ctx *s, uint32_t length)
{
    int ret;
    if (length != 13)
        return -1;

    if (s->state & PNG_IDAT) {
        loge("IHDR after IDAT");
        return -1;
    }

    if (s->state & PNG_IHDR) {
        loge("Multiple IHDR");
        return -1;
    }

    s->width  = s->cur_w = bytestream2_get_be32(&s->gb);
    s->height = s->cur_h = bytestream2_get_be32(&s->gb);

    if (s->width < 1 || s->height < 1) {
        loge("size too small: width %d, height %d", s->width, s->height);
        return -1;
    } else if (s->width * s->height > PNG_MAX_RAWSIZE) {
        loge("size too large: width %d, height %d", s->width, s->height);
        return -1;
    }

    s->bit_depth        = bytestream2_get_byte(&s->gb);
    if (s->bit_depth != 8) {
        loge("not support bit depth %d", s->bit_depth);
        goto error;
    }

    s->color_type       = bytestream2_get_byte(&s->gb);
    s->compression_type = bytestream2_get_byte(&s->gb);
    if (s->compression_type) {
        loge("Invalid compression method %d", s->compression_type);
        goto error;
    }

    s->filter_type      = bytestream2_get_byte(&s->gb);
    if (bytestream2_get_byte(&s->gb)) {
        loge("not support interleace type");
        return -1;
    }
    bytestream2_skip(&s->gb, 4); /* crc */
    s->state |= PNG_IHDR;

    ret = alloc_phy_buffer(s, s->width, s->height);
    if (ret < 0)
        return -1;

    logd("width=%d height=%d depth=%d color_type=%d compression_type=%d filter_type=%d\n",
        s->width, s->height, s->bit_depth, s->color_type,
        s->compression_type, s->filter_type);

    return 0;
error:
    s->cur_w = s->cur_h = s->width = s->height = 0;
    s->bit_depth = 8;
    return -1;
}

static int decode_idat_chunk(struct png_dec_ctx *s, uint32_t length)
{
    int ret;

    if (!s->decode_next_dat) {
        if (!(s->state & PNG_IHDR)) {
            loge("IDAT without IHDR");
            return -1;
        }
    }

    if (!(s->state & PNG_IDAT)) {
        s->channels       = png_get_nb_channels(s->color_type);
        s->bits_per_pixel = s->bit_depth * s->channels;

        if ((s->bit_depth == 2 || s->bit_depth == 4 || s->bit_depth == 8) &&
            s->color_type == PNG_COLOR_TYPE_RGB) {
            logd("rgb24");
        } else if ((s->bit_depth == 2 || s->bit_depth == 4 || s->bit_depth == 8) &&
            s->color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
            logd("rgb32");
        } else if ((s->bit_depth == 2 || s->bit_depth == 4 || s->bit_depth == 8) &&
            s->color_type == PNG_COLOR_TYPE_GRAY) {
            logd("gray8, not support");
        } else if ((s->bits_per_pixel == 1 || s->bits_per_pixel == 2 ||
            s->bits_per_pixel == 4 || s->bits_per_pixel == 8) &&
            s->color_type == PNG_COLOR_TYPE_PALETTE) {
            logd("pal8");
        } else {
            loge("not support, color format(%d), bitdepth(%d)", s->color_type, s->bit_depth);
            return -1;
        }
    }

    s->state |= PNG_IDAT;

    ret = png_decode_idat(s, length);
    if (ret < 0)
        return ret;

    bytestream2_skip(&s->gb, 4); /* crc */

    return 0;
}

static int decode_plte_chunk(struct png_dec_ctx *s, uint32_t length)
{
    int n, i, r, g, b;

    if ((length % 3) != 0 || length > 256 * 3)
        return -1;
    /* read the palette */
    n = length / 3;
    for (i = 0; i < n; i++) {
        r = bytestream2_get_byte(&s->gb);
        g = bytestream2_get_byte(&s->gb);
        b = bytestream2_get_byte(&s->gb);
        s->palette[i] = (0xFFU << 24) | (r << 16) | (g << 8) | b;
    }

    for (; i < 256; i++)
        s->palette[i] = (0xFFU << 24);

    s->state |= PNG_PLTE;

    bytestream2_skip(&s->gb, 4);     /* crc */

    return 0;
}

static int decode_trns_chunk(struct png_dec_ctx *s, uint32_t length)
{
    int i;

    if (!(s->state & PNG_IHDR)) {
        loge("trns before IHDR");
        return -1;
    }

    if (s->state & PNG_IDAT) {
        loge("trns after IDAT");
        return -1;
    }

    if (s->color_type == PNG_COLOR_TYPE_PALETTE) {
        if (length > 256 || !(s->state & PNG_PLTE))
            return -1;

        for (i = 0; i < length; i++) {
            unsigned v = bytestream2_get_byte(&s->gb);
            s->palette[i] = (s->palette[i] & 0x00ffffff) | (v << 24);
        }
    } else {
        return -1;
    }

    bytestream2_skip(&s->gb, 4); /* crc */

    return 0;
}

static int decode_fctl_chunk(struct png_dec_ctx *s, uint32_t length)
{
    uint32_t sequence_number;
    int cur_w, cur_h, x_offset, y_offset, dispose_op, blend_op;

    if (length != 26) {
        loge("wrong length %d != 26", (int)length);
        return -1;
    }

    if (!s->decode_next_dat) {
        if (!(s->state & PNG_IHDR)) {
            loge("fctl before IHDR\n");
            return -1;
        }
    }

    s->last_dispose_op = s->dispose_op;

    sequence_number = bytestream2_get_be32(&s->gb);
    cur_w           = bytestream2_get_be32(&s->gb);
    cur_h           = bytestream2_get_be32(&s->gb);
    x_offset        = bytestream2_get_be32(&s->gb);
    y_offset        = bytestream2_get_be32(&s->gb);
    bytestream2_skip(&s->gb, 4); /* delay_num (2), delay_den (2) */
    dispose_op      = bytestream2_get_byte(&s->gb);
    blend_op        = bytestream2_get_byte(&s->gb);
    bytestream2_skip(&s->gb, 4); /* crc */

    logd("fctl: sequence_number:%u, cur_w:%d, cur_h:%d, x_off:%d, y_off:%d\n",
        (unsigned int)sequence_number, cur_w, cur_h, x_offset, y_offset);
    if (blend_op != APNG_BLEND_OP_OVER && blend_op != APNG_BLEND_OP_SOURCE) {
        loge("Invalid blend_op %d\n", blend_op);
        return -1;
    }

    if (sequence_number == 0 && dispose_op == APNG_DISPOSE_OP_PREVIOUS) {
        // No previous frame to revert to for the first frame
        // Spec says to just treat it as a APNG_DISPOSE_OP_BACKGROUND
        dispose_op = APNG_DISPOSE_OP_BACKGROUND;
    }

    s->cur_w      = cur_w;
    s->cur_h      = cur_h;
    s->x_offset   = x_offset;
    s->y_offset   = y_offset;
    s->dispose_op = dispose_op;
    s->blend_op   = blend_op;

    return 0;
}

#ifdef AIC_VE_DRV_V10
static int memset_last_row_data(struct png_dec_ctx *s)
{
    // if filter type of first line > 1.
    // memset the last row buffer
    int stride = s->curr_frame->mpp_frame.buf.stride[0];
    int len = (s->height == 1) ? stride : stride * 2;

    unsigned char* addr = (unsigned char*)(unsigned long)s->curr_frame->mpp_frame.buf.phy_addr[0] + stride * s->height - len;
    memset(addr, 0, len);
    aicos_dcache_clean_invalid_range(addr, len);
    // ----- end ----------

    return 0;
}
#endif

static int decode_get_frame(struct png_dec_ctx *s)
{
    if(s->pix_fmt == MPP_FMT_ABGR_8888 || s->pix_fmt == MPP_FMT_ARGB_8888 ||
       s->pix_fmt == MPP_FMT_BGRA_8888 || s->pix_fmt == MPP_FMT_RGBA_8888)
        s->stride = ALIGN_8B(s->width*4);
    else if(s->pix_fmt == MPP_FMT_BGR_888 || s->pix_fmt == MPP_FMT_RGB_888)
        s->stride = ALIGN_8B(s->width*3);
    else if(s->pix_fmt == MPP_FMT_BGR_565 || s->pix_fmt == MPP_FMT_RGB_565)
        s->stride = ALIGN_8B(s->width*2);

    if(s->decoder.fm == NULL) {
        struct frame_manager_init_cfg cfg = {0};
        cfg.frame_count = s->extra_frame_num + 1;
        cfg.height = s->height;
        cfg.width = s->width;
        cfg.stride = s->stride;
        cfg.height_align = s->height;
        cfg.pixel_format = s->pix_fmt;
        cfg.allocator = s->decoder.allocator;
        s->decoder.fm = fm_create(&cfg);
    }

    s->curr_frame = fm_decoder_get_frame(s->decoder.fm);
    if(s->curr_frame == NULL) {
        pm_reclaim_ready_packet(s->decoder.pm, s->curr_packet);
        return DEC_NO_EMPTY_FRAME;
    }
    return 0;
}

static int decode_frame_common(struct png_dec_ctx *s)
{
    uint32_t tag, length;
    int decode_next_dat = 0;
    int ret;
    s->idat_data_size = 0;
    s->fdat_buf = NULL;

    for (;;) {
        length = bytestream2_get_bytes_left(&s->gb);

        logd("decode frame left %"PRIu32, length);
        if (length <= 0) {
            goto exit_loop;
        }

        length = bytestream2_get_be32(&s->gb);
        if (length > 0x7fffffff || length > bytestream2_get_bytes_left(&s->gb)) {
            loge("chunk too big, length: %"PRIu32, length);
            return -1;
        }

        logd("decode chunk length %"PRIu32, length);

        tag = bytestream2_get_le32(&s->gb);

        switch (tag) {
        case SHOW_TAG('I', 'H', 'D', 'R'):
            if ((ret = decode_ihdr_chunk(s, length)) < 0)
                return ret;
            break;
        case SHOW_TAG('f', 'c', 'T', 'L'):
            logd("find fcTL.\n");
            if ((ret = decode_fctl_chunk(s, length)) < 0)
                return ret;
            s->decode_next_dat = decode_next_dat = 1;
            logd("decode_next_dat %d.\n", s->decode_next_dat);
            break;
        case SHOW_TAG('f', 'd', 'A', 'T'):
            if (!decode_next_dat) {
                return -1;
            }
            bytestream2_get_be32(&s->gb);
            length -= 4;
            /* fallthrough */
        case SHOW_TAG('I', 'D', 'A', 'T'):
            if ((ret = decode_idat_chunk(s, length)) < 0)
                return ret;
            break;
        case SHOW_TAG('P', 'L', 'T', 'E'):
            if (decode_plte_chunk(s, length) < 0)
                goto skip_tag;
            break;
        case SHOW_TAG('t', 'R', 'N', 'S'):
            if (decode_trns_chunk(s, length) < 0)
                goto skip_tag;
            break;
        case SHOW_TAG('I', 'E', 'N', 'D'):
            logi("========== IEND ===========");
            if (!(s->state & PNG_ALLIMAGE))
                logi("IEND without all image");
            if (!(s->state & (PNG_ALLIMAGE|PNG_IDAT))) {
                return -1;
            }
            bytestream2_skip(&s->gb, 4); /* crc */
            goto exit_loop;
        default:
            /* skip tag */
        skip_tag:
            bytestream2_skip(&s->gb, length + 4);
            break;
        }
    }
exit_loop:

    ret = decode_get_frame(s);
    if (ret) {
        return ret;
    }
#ifdef AIC_VE_DRV_V10
    if (!s->decode_next_dat)
        memset_last_row_data(s);
#endif


    //hw only decode deflate bitstream, skip zlib header and tailer
    if (s->decode_next_dat) {
        if (s->fdat_buf == NULL || s->idat_data_size <= 0) {
            loge("apng fdat buf or size is illegal.");
            return -1;
        }
        s->vbv_offset = (s->fdat_buf - s->curr_packet->data) + 2;
        png_hardware_decode(s, s->fdat_buf + 2, s->idat_data_size - 6);
    } else {
        logd("png_hardware_decode vir:%p phy: %lx %d", s->idat_mpp_buf->vir_addr,
            s->idat_mpp_buf->phy_addr,s->idat_mpp_buf->size);
        s->vbv_offset = 2;
        png_hardware_decode(s, s->idat_mpp_buf->vir_addr + 2, s->idat_data_size - 6);
    }

    if(s->curr_packet->flag & PACKET_FLAG_EOS)
        s->curr_frame->mpp_frame.flags |= FRAME_FLAG_EOS;
    s->curr_frame->mpp_frame.buf.crop_en = 1;
    s->curr_frame->mpp_frame.buf.crop.x = 0;
    s->curr_frame->mpp_frame.buf.crop.y = 0;
    s->curr_frame->mpp_frame.buf.crop.width = s->width;
    s->curr_frame->mpp_frame.buf.crop.height = s->height;

    fm_decoder_frame_to_render(s->decoder.fm, s->curr_frame, 1);
    fm_decoder_put_frame(s->decoder.fm, s->curr_frame);

    return 0;
}

int __png_decode_frame(struct mpp_decoder *ctx)
{
    struct png_dec_ctx *s = (struct png_dec_ctx*)ctx;

    long long unsigned int sig;
    int ret;

    s->curr_packet = pm_dequeue_ready_packet(s->decoder.pm);
    uint8_t *buf     = s->curr_packet->data;
    int buf_size     = s->curr_packet->size;

    s->packet_size = buf_size;

    bytestream2_init(&s->gb, buf, buf_size);
    logd("decode_next_dat  %d", s->decode_next_dat);
    /*filter apng check signature when not process the first frame*/
    if (!s->decode_next_dat) {
        /* check signature */
        sig = bytestream2_get_be64(&s->gb);
        if (sig != PNGSIG && sig != MNGSIG) {
            loge("Invalid PNG signature 0x%08llx.\n", sig);
            return -1;
        }
    }

    s->state = 0;

    if ((ret = decode_frame_common(s)) < 0)
        return ret;

    logd("decode_frame_common ret %d", ret);

    pm_enqueue_empty_packet(s->decoder.pm, s->curr_packet);

    return 0;
}

static int __png_decode_init(struct mpp_decoder *ctx, struct decode_config *config)
{
    struct png_dec_ctx *s;

    logd("png_decode_init");

    if (ctx == NULL || config == NULL)
        return -1;

    s = (struct png_dec_ctx*)ctx;

    if(config->pix_fmt != MPP_FMT_ARGB_8888 && config->pix_fmt != MPP_FMT_ABGR_8888
      && config->pix_fmt != MPP_FMT_RGBA_8888 && config->pix_fmt != MPP_FMT_BGRA_8888
      && config->pix_fmt != MPP_FMT_RGB_888 && config->pix_fmt != MPP_FMT_BGR_888
      && config->pix_fmt != MPP_FMT_RGB_565 && config->pix_fmt != MPP_FMT_BGR_565) {
        logw("output pixel format not support, use RGB888");
        s->pix_fmt = MPP_FMT_RGB_888;
    } else {
        s->pix_fmt = config->pix_fmt;
    }

    s->ve_buf_handle = ve_buffer_allocator_create(VE_BUFFER_TYPE_DMA);
    s->bitstream_buffer_size = config->bitstream_buffer_size;

    s->ve_fd = ve_open_device();
    s->regs_base = ve_get_reg_base();

    s->vbv_offset = 0;
    s->decode_next_dat = 0;

    s->reg_list = mpp_alloc(sizeof(struct png_register_list));
    if(!s->reg_list)
        return -1;
    memset(s->reg_list, 0, sizeof(struct png_register_list));

    struct packet_manager_init_cfg cfg;
    cfg.ve_buf_handle = s->ve_buf_handle;
    cfg.buffer_size = config->bitstream_buffer_size;
    cfg.packet_count = config->packet_count;
    cfg.pkt_allocator = s->decoder.pkt_allocator;
    s->decoder.pm = pm_create(&cfg);
    s->extra_frame_num = config->extra_frame_num;
    if (!s->decoder.pm) {
       loge("Insufficient CMA memory resources \n");
       return DEC_ERR_NULL_PTR;
    }
    return 0;
}

static int __png_decode_destroy(struct mpp_decoder *ctx)
{
    struct png_dec_ctx *s = (struct png_dec_ctx*)ctx;

    free_phy_buffer(s);

    if (s->reg_list)
        mpp_free(s->reg_list);

    if(s->decoder.pm)
        pm_destroy(s->decoder.pm);

    if(s->decoder.fm)
        fm_destory(s->decoder.fm);

    if(s->ve_buf_handle)
        ve_buffer_allocator_destroy(s->ve_buf_handle);

    if(s->ve_fd > 0)
        ve_close_device();

    mpp_free(s);
    return 0;
}

int __png_decode_control(struct mpp_decoder *ctx, int cmd, void *param)
{
    // TODO
    return 0;
}

int __png_decode_reset(struct mpp_decoder *ctx)
{
    // TODO
    return 0;
}

struct dec_ops png_decoder = {
    .name           = "png",
    .init           = __png_decode_init,
    .destory        = __png_decode_destroy,
    .decode         = __png_decode_frame,
    .control        = __png_decode_control,
    .reset          = __png_decode_reset,
};

struct mpp_decoder* create_png_decoder()
{
    struct png_dec_ctx *s = (struct png_dec_ctx*)mpp_alloc(sizeof(struct png_dec_ctx));
    if(s == NULL)
        return NULL;
    memset(s, 0, sizeof(struct png_dec_ctx));

    s->decoder.ops = &png_decoder;

    return &s->decoder;
}
