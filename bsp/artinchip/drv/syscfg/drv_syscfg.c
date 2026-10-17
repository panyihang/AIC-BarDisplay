/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "hal_syscfg.h"

int drv_syscfg_init(void)
{
    if (hal_syscfg_probe())
        return -RT_ERROR;
    else
        return RT_EOK;
}
INIT_BOARD_EXPORT(drv_syscfg_init);
