/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_BOOT_TIME_H__
#define __AIC_BOOT_TIME_H__
#include <aic_common.h>

#ifdef __cplusplus
extern "C" {
#endif

void boot_time_trace(char *msg);
void boot_time_show(void);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_BOOT_TIME_H__ */
