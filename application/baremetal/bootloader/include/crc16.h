/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wudehuang <dehuang.wu@artinchip.com>
 */

#ifndef __BL_CRC16_H_
#define __BL_CRC16_H_

#include <stdint.h>

u16 crc16_ccitt(u16 cksum, const unsigned char *buf, int len);

#endif /* __BL_CRC16_H_ */
