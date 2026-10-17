/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ardon <haidong.pan@artinchip.com>
 */

#ifndef _AIC_HAL_GE_H_
#define _AIC_HAL_GE_H_

#include <strings.h>
#include "aic_list.h"

#define GE_CMA MEM_CMA
#define GE_DEFAULT MEM_DEFAULT

struct aic_ge_client {
    struct list_head list;
    struct list_head buf_list;
    int id;
    int batch_num;
};

int hal_ge_init(void);
struct aic_ge_client *hal_ge_open(void);
int hal_ge_close(struct aic_ge_client *clt);
int hal_ge_control(struct aic_ge_client *clt, int cmd, void *arg);
int hal_ge_write(struct aic_ge_client *clt, const char *buff, size_t count);
int hal_ge_deinit(void);

#endif // end of _AIC_HAL_GE_H_
