/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 *  SPDX-License-Identifier: Apache-2.0
 *  author: <qi.xu@artinchip.com>
 *  Desc: jpeg marker define
 */

#ifndef MJPEG_H
#define MJPEG_H

#define SOF0  0xc0      /* baseline */
#define SOF1  0xc1      /* extended sequential, huffman (not support)*/
#define SOF2  0xc2      /* progressive, huffman (not support)*/
#define SOF3  0xc3      /* lossless, huffman (not support)*/
#define SOF5  0xc5      /* differential sequential, huffman */
#define SOF6  0xc6      /* differential progressive, huffman */
#define SOF7  0xc7      /* differential lossless, huffman */
#define JPG   0xc8      /* reserved for JPEG extension */
#define SOF9  0xc9      /* extended sequential, arithmetic */
#define SOF10 0xca      /* progressive, arithmetic */
#define SOF11 0xcb      /* lossless, arithmetic */
#define SOF13 0xcd      /* differential sequential, arithmetic */
#define SOF14 0xce      /* differential progressive, arithmetic */
#define SOF15 0xcf      /* differential lossless, arithmetic */
#define DHT   0xc4      /* define huffman tables */

#define RST0  0xd0
#define RST7  0xd7
#define SOI   0xd8      /* start of image */
#define EOI   0xd9      /* end of image */
#define SOS   0xda      /* start of scan */
#define DQT   0xdb      /* define quantization tables */
#define DRI   0xdd      /* define restart interval */

#define APP0  0xe0
#define APP15 0xef

#define JPG0  0xf0      /* unsupport */
#define JPG13 0xfd      /* unsupport */

#define COM  0xfe

#endif /* MJPEG_H */
