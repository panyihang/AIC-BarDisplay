/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <mtd.h>

#ifdef AIC_SPINAND_DRV
#include <spinand_port.h>
#endif

#ifdef AIC_SPINOR_DRV
#include <sfud.h>
extern sfud_flash *sfud_probe(u32 spi_bus);
#endif

static int qspi_active_id_list[] = {
#if defined(AIC_USING_QSPI0)
    0,
#endif
#if defined(AIC_USING_QSPI1)
    1,
#endif
#if defined(AIC_USING_QSPI2)
    2,
#endif
#if defined(AIC_USING_QSPI3)
    3,
#endif
#if defined(AIC_USING_QSPI4)
    4,
#endif
#if defined(AIC_USING_SE_SPI)
    5,
#endif
};

int mtd_probe(void)
{
    int i, id;

#ifdef AIC_SPINOR_DRV
    for (i = 0; i < ARRAY_SIZE(qspi_active_id_list); i++) {
        id = qspi_active_id_list[i];
        sfud_probe(id);
    }
#endif

#ifdef AIC_SPINAND_DRV
    for (i = 0; i < ARRAY_SIZE(qspi_active_id_list); i++) {
        id = qspi_active_id_list[i];
        spinand_probe(id);
    }
#endif

    return 0;
}
