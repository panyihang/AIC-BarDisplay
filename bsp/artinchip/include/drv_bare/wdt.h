/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef __BL_WDT_H_
#define __BL_WDT_H_
#include <aic_common.h>

#ifdef __cplusplus
extern "C" {
#endif

int wdt_init(void);
int wdt_deinit(void);
int wdt_start(u32 tmo_ms);
void wdt_stop(void);
int wdt_expire_now(void);

#ifdef __cplusplus
}
#endif

#endif /* __BL_WDT_H_ */
