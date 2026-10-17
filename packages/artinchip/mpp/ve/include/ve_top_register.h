/*
* Copyright (C) 2020-2025 Artinchip Technology Co. Ltd
*
* SPDX-License-Identifier: Apache-2.0
*  author: <qi.xu@artinchip.com>
*  Desc: the  ve top register
*/

#ifndef VE_TOP_REGISTER_H
#define VE_TOP_REGISTER_H

#include "aic_io.h"
#include "mpp_log.h"

#define	read_reg_u32(offset)	readl(offset)
#define	write_reg_u32(offset,v)	writel(v, offset)

#ifdef AIC_VE_DRV_V10
    #define PNG_REG_OFFSET_ADDR	0xC00
    #define JPG_REG_OFFSET_ADDR	0x2000
    #define PIC_INFO_START_REG	0x1400
    #define PIC_INFO_END_REG	0x167C
#else
    #define PNG_REG_OFFSET_ADDR	0x100
    #define JPG_REG_OFFSET_ADDR	0x200
    #define PIC_INFO_START_REG	0x600
#endif

// [0]: clock enable
#define VE_CLK_REG    0x00

struct reg_ve_rst
{
	unsigned rst_avc :2;    // [0]: avc module reset
	unsigned rst_jpg : 2;
	unsigned rst_png : 2;
	unsigned r0 : 2;
	unsigned rst_pic : 2;
	unsigned r1 : 6;
	unsigned status : 16; // (ReadOnly) reset status, 0:reset finish
};
#define RST_PIC_MODULE		(0xFFFFFCFF)
#define RST_AVC_MODULE		(0xFFFFFFFC)
#define RST_JPG_MODULE		(0xFFFFFFF3)
#define RST_PNG_MODULE		(0xFFFFFFCF)
#define RST_AVC_PIC_MODULE	(RST_AVC_MODULE & RST_PIC_MODULE)
#define RST_JPG_PIC_MODULE	(RST_JPG_MODULE & RST_PIC_MODULE)
#define RST_PNG_PIC_MODULE	(RST_PNG_MODULE & RST_PIC_MODULE)
#define VE_RST_REG    0x04

// [0]: ve init
#define VE_INIT_REG    0x08

// [0]: ve interrupt enable
#define VE_IRQ_REG    0x0C

#define VE_AVC_EN_REG    0x10
#define VE_JPG_EN_REG    0x14
#define VE_PNG_EN_REG    0x18

/****************************** FRAME INFO REG *****************************************************/
// 0x1400-0x167c is used for config pic info, every pic info need 5 registers, max number picture is 18
// 1) frame format
// 2) frame size
// 3) y  addr
// 4) cb addr
// 5) cr addr
#define FRAME_FORMAT_REG(n)	(PIC_INFO_START_REG + 20*(n))
#define FRAME_SIZE_REG(n)	(PIC_INFO_START_REG + 20*(n) + 4)
#define FRAME_YADDR_REG(n)	(PIC_INFO_START_REG + 20*(n) + 8)
#define FRAME_CBADDR_REG(n)	(PIC_INFO_START_REG + 20*(n) + 12)
#define FRAME_CRADDR_REG(n)	(PIC_INFO_START_REG + 20*(n) + 16)

#define PIC_INFO_WRITE_END_REG    (PIC_INFO_START_REG+0x1FC)

struct frame_format_reg
{
	unsigned stride : 16;           //[15:0], stride 16bytes align
	unsigned cbcr_interleaved : 1;  // uv interleave
	unsigned color_mode : 3;  // 0-YUV420; 1-YUV422; 2-YUV224; 3-YUV444; 4-YUV400
	unsigned r2 : 12;
};

struct frame_size_reg
{
	unsigned pic_ysize : 16;
	unsigned pic_xsize : 16;
};

#endif
