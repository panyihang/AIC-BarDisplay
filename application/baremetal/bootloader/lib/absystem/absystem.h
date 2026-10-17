/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#ifndef __ABSYSTEM_H__
#define __ABSYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

int aic_ota_check(void);
int aic_get_os_to_startup(char *target_os);

#ifdef __cplusplus
}
#endif

#endif
