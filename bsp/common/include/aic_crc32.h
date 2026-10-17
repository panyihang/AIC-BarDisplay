/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __AIC_CRC32_H__
#define __AIC_CRC32_H__

#ifdef __cplusplus
extern "C" {
#endif

uint32_t env_crc32(uint32_t crc, const uint8_t *buf, uint32_t len);
uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_CRC32_H__ */
