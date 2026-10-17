/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_gpio_id.h>

const int aic_gpio_groups_list[] = {
    PA_GROUP,
    PB_GROUP,
    PC_GROUP,
    PD_GROUP,
    PE_GROUP,
    PF_GROUP,
    PM_GROUP,
    PN_GROUP,
};

const int aic_gpio_group_size = sizeof(aic_gpio_groups_list) / sizeof(aic_gpio_groups_list[0]);
