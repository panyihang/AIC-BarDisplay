/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>
#include <partition_table.h>

#define NOR_FLASH_DEV_NAME             "norflash0"

/* ===================== Flash device Configuration ========================= */
/* FAL_FLASH_DEV_TABLE is no longer required.
 * fal_flash.c will use fal_flash_table_ptr from spinor_fal.c by default.
 * If you need custom device table, define FAL_FLASH_DEV_TABLE here.
 */

#endif /* _FAL_CFG_H_ */
