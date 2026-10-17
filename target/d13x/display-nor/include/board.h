/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_BOARD_H__
#define __AIC_BOARD_H__

#include <rtconfig.h>

#if defined(KERNEL_BAREMETAL)
void aic_hw_board_init(void);
#endif

#endif /* __AIC_BOARD_H__ */
