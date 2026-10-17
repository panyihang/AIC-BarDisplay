/*
 * Copyright (c) 2022-2024, ArtInChip, Artinchip Technology Co., Ltd
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
    PU_GROUP,
};

const int aic_gpio_group_size = sizeof(aic_gpio_groups_list) / sizeof(aic_gpio_groups_list[0]);
