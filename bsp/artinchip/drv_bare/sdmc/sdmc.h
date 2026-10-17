/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_SDMC_H_
#define _AIC_SDMC_H_

#include <aic_core.h>
#include <hal_sdmc.h>
#include <mmc.h>

// #define SDMC_DUMP_CMD
// #define SDMC_RPMB_TRACE

void aic_sdmc_request(struct aic_sdmc *host, struct aic_sdmc_cmd *cmd,
                      struct aic_sdmc_data *data);
irqreturn_t aic_sdmc_irq(int irq, void *arg);
s32 aic_sdmc_clk_init(struct aic_sdmc *host);
int aic_sdmc_init(struct aic_sdmc *host);
void aic_sdmc_set_cfg(struct aic_sdmc *host);

#endif

