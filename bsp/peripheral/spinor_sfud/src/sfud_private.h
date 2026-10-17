/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SFUD_PRIVATE_H_
#define _SFUD_PRIVATE_H_

#include "sfud_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int spi_nor_get_unique_id(sfud_flash *flash, uint8_t *data);
int spi_nor_sr_unlock_all(sfud_flash *flash);

#ifdef __cplusplus
}
#endif

#endif /* _SFUD_PRIVATE_H_ */
