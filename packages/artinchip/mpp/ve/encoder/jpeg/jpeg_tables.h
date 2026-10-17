/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <qi.xu@artinchip.com>
 * Desc: jpeg tables
 */

#ifndef JPEG_TABLES_H
#define JPEG_TABLES_H

#include <stdint.h>

extern const unsigned char std_luma_quant_table[];
extern const unsigned char std_chroma_quant_table[];

extern const uint8_t bits_dc_luma[];
extern const uint8_t val_dc[];

extern const uint8_t bits_dc_chroma[];

extern const uint8_t bits_ac_luminance[];
extern const uint8_t val_ac_luminance[];

extern const uint8_t bits_ac_chroma[];
extern const uint8_t val_ac_chroma[];

extern uint8_t zigzag_dir[64];

void mjpeg_build_huffman_codes(uint8_t *huff_size, uint16_t *huff_code,
                                  const uint8_t *bits_table,
                                  const uint8_t *val_table);

#endif
