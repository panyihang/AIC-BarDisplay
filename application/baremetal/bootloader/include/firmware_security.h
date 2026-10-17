/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __FIRMWARE_SECURITY_H_
#define __FIRMWARE_SECURITY_H_

#include <stdint.h>
#include <stddef.h>

int firmware_security_init(void);
void firmware_security_decrypt(uint8_t *buf, unsigned int length);

#endif // __FIRMWARE_SECURITY_H_
