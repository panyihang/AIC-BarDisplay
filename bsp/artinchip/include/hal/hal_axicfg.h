/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef _AIC_HAL_AXICFG_
#define _AIC_HAL_AXICFG_
#include <aic_common.h>
#include <stdbool.h>
#include <rtconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_AXICFG_CPU_P_NUM 0
#define HAL_AXICFG_AHB_P_NUM 1
#define HAL_AXICFG_DE_P_NUM  2
#define HAL_AXICFG_GE_P_NUM  3
#define HAL_AXICFG_VE_P_NUM  4
#define HAL_AXICFG_DVP_P_NUM 5
#define HAL_AXICFG_CE_P_NUM  6

#define HAL_AXICFG_W_READ  0
#define HAL_AXICFG_W_WRITE 1

#define HAL_AXICFG_EXT_QOS 0 //QOS set by main device
#define HAL_AXICFG_INT_QOS 1 //QOS set by QOS_VAL reg

#define HAL_AXICFG_BASE 0x184FE000

#ifdef AIC_CHIP_D12P
typedef enum hal_axicfg_port_e {
    HAL_AXICFG_PORT_CPU = 0,
    HAL_AXICFG_PORT_AHB = 1,
    HAL_AXICFG_PORT_AHB1 = 2,
    HAL_AXICFG_PORT_DE = 3,
    HAL_AXICFG_PORT_GE = 4,
    HAL_AXICFG_PORT_VE = 5,
    HAL_AXICFG_PORT_DVP = 6,
    HAL_AXICFG_PORT_MAX
} hal_axicfg_port_t;
#else
typedef enum hal_axicfg_port_e {
    HAL_AXICFG_PORT_CPU = 0,
    HAL_AXICFG_PORT_AHB = 1,
    HAL_AXICFG_PORT_DE = 2,
    HAL_AXICFG_PORT_GE = 3,
    HAL_AXICFG_PORT_VE = 4,
    HAL_AXICFG_PORT_DVP = 5,
    HAL_AXICFG_PORT_MDI = 5,
    HAL_AXICFG_PORT_CE = 6,
    HAL_AXICFG_PORT_MAX
} hal_axicfg_port_t;
#endif
#ifndef AIC_AXICFG_PORT_CPU_EN
#define AXICFG_CPU_EN            0
#define AIC_AXICFG_PORT_CPU_PRIO 0
#else
#define AXICFG_CPU_EN 1
#endif

#ifndef AIC_AXICFG_PORT_AHB_EN
#define AXICFG_AHB_EN            0
#define AIC_AXICFG_PORT_AHB_PRIO 0
#else
#define AXICFG_AHB_EN 1
#endif

#ifndef AIC_AXICFG_PORT_AHB1_EN
#define AXICFG_AHB1_EN            0
#define AIC_AXICFG_PORT_AHB1_PRIO 0
#else
#define AXICFG_AHB1_EN 1
#endif

#ifndef AIC_AXICFG_PORT_DE_EN
#define AXICFG_DE_EN            0
#define AIC_AXICFG_PORT_DE_PRIO 0
#else
#define AXICFG_DE_EN 1
#endif

#ifndef AIC_AXICFG_PORT_GE_EN
#define AXICFG_GE_EN            0
#define AIC_AXICFG_PORT_GE_PRIO 0
#else
#define AXICFG_GE_EN 1
#endif

#ifndef AIC_AXICFG_PORT_VE_EN
#define AXICFG_VE_EN            0
#define AIC_AXICFG_PORT_VE_PRIO 0
#else
#define AXICFG_VE_EN 1
#endif

#ifndef AIC_AXICFG_PORT_DVP_EN
#define AXICFG_DVP_EN            0
#define AIC_AXICFG_PORT_DVP_PRIO 0
#else
#define AXICFG_DVP_EN 1
#endif

#ifndef AIC_AXICFG_PORT_CE_EN
#define AXICFG_CE_EN            0
#define AIC_AXICFG_PORT_CE_PRIO 0
#else
#define AXICFG_CE_EN 1
#endif

struct hal_axicfg_table {
    u8 enable;
    u8 priority;
};

// read: mode = 0; write: mode = 1;
int hal_axicfg_module_wr_init(u8 mode, hal_axicfg_port_t device_p, u8 priority);
int hal_axicfg_module_init(hal_axicfg_port_t device_p, u8 priority);

#ifdef __cplusplus
}
#endif

#endif
