/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SFUD_QE_H_
#define _SFUD_QE_H_

#include "sfud_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int spi_nor_sr1_bit6_quad_enable(sfud_flash *flash);
int spi_nor_sr2_bit1_quad_enable(sfud_flash *flash);
int spi_nor_quad_enable(sfud_flash *flash);

#ifdef __cplusplus
}
#endif

#endif /* _SFUD_QE_H_ */
