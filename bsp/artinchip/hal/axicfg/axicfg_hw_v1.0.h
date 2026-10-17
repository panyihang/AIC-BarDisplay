/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef _AIC_HAL_AXICFG_REGS_H_
#define _AIC_HAL_AXICFG_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AXICFG_INVALID_BASE (0xFFFFFFFF)

#define AXICFG_REG_CFG0(base, mode_w, device_p) \
    (volatile void *)((base) + 0x00UL + (mode_w)*0x10UL + (device_p)*0x20UL)
#define AXICFG_REG_CFG1(base, mode_w, device_p) \
    (volatile void *)((base) + 0x04UL + (mode_w)*0x10UL + (device_p)*0x20UL)

#define CFG0_BIT_REULATOR_EN_OFS (0)
#define CFG0_BIT_REULATOR_EN_MSK (0x1 << CFG0_BIT_REULATOR_EN_OFS)
#define CFG0_BIT_REULATOR_EN_VAl(v) \
    (((v) << CFG0_BIT_REULATOR_EN_OFS) & CFG0_BIT_REULATOR_EN_MSK)
#define CFG0_BIT_SLV_READY_OFS (1)
#define CFG0_BIT_SLV_READY_MSK (0x1 << CFG0_BIT_SLV_READY_OFS)
#define CFG0_BIT_SLV_READY_VAl(v) \
    (((v) << CFG0_BIT_SLV_READY_OFS) & CFG0_BIT_SLV_READY_MSK)
#define CFG0_BIT_BURST_LIMIT_OFS (8)
#define CFG0_BIT_BURST_LIMIT_MSK (0xFF << CFG0_BIT_BURST_LIMIT_OFS)
#define CFG0_BIT_BURST_LIMIT_VAl(v) \
    (((v) << CFG0_BIT_BURST_LIMIT_OFS) & CFG0_BIT_BURST_LIMIT_MSK)
#define CFG0_BIT_QOS_SEL_OFS (16)
#define CFG0_BIT_QOS_SEL_MSK (0x1 << CFG0_BIT_QOS_SEL_OFS)
#define CFG0_BIT_QOS_SEL_VAl(v) \
    (((v) << CFG0_BIT_QOS_SEL_OFS) & CFG0_BIT_QOS_SEL_MSK)
#define CFG0_BIT_QOS_VAL_OFS (24)
#define CFG0_BIT_QOS_VAL_MSK (0xF << CFG0_BIT_QOS_VAL_OFS)
#define CFG0_BIT_QOS_VAL_VAl(v) \
    (((v) << CFG0_BIT_QOS_VAL_OFS) & CFG0_BIT_QOS_VAL_MSK)

#define CFG1_BIT_BASIC_RATE_OFS (0)
#define CFG1_BIT_BASIC_RATE_MSK (0xFFF << CFG1_BIT_BASIC_RATE_OFS)
#define CFG1_BIT_BASIC_RATE_VAl(v) \
    (((v) << CFG1_BIT_BASIC_RATE_OFS) & CFG1_BIT_BASIC_RATE_MSK)
#define CFG1_BIT_BURST_RATE_OFS (16)
#define CFG1_BIT_BURST_RATE_MSK (0xFFF << CFG1_BIT_BURST_RATE_OFS)
#define CFG1_BIT_BURST_RATE_VAl(v) \
    (((v) << CFG1_BIT_BURST_RATE_OFS) & CFG1_BIT_BURST_RATE_MSK)

static inline void axicfg_hw_cfg0_set_reulator_en(u32 base, u8 mode_w,
                                                  u8 device_p, bool value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG0(base, mode_w, device_p));

    val &= ~(CFG0_BIT_REULATOR_EN_MSK);
    val |= CFG0_BIT_REULATOR_EN_VAl(value);

    writel(val, AXICFG_REG_CFG0(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg0_set_slv_ready(u32 base, u8 mode_w,
                                                u8 device_p, bool value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG0(base, mode_w, device_p));

    val &= ~(CFG0_BIT_SLV_READY_MSK);
    val |= CFG0_BIT_SLV_READY_VAl(value);

    writel(val, AXICFG_REG_CFG0(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg0_set_burst_limit(u32 base, u8 mode_w,
                                                  u8 device_p, u8 value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG0(base, mode_w, device_p));

    val &= ~(CFG0_BIT_BURST_LIMIT_MSK);
    val |= CFG0_BIT_BURST_LIMIT_VAl(value);

    writel(val, AXICFG_REG_CFG0(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg0_set_qos_sel(u32 base, u8 mode_w, u8 device_p,
                                              u8 value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG0(base, mode_w, device_p));

    val &= ~(CFG0_BIT_QOS_SEL_MSK);
    val |= CFG0_BIT_QOS_SEL_VAl(value);

    writel(val, AXICFG_REG_CFG0(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg0_set_qos_val(u32 base, u8 mode_w, u8 device_p,
                                              u8 value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG0(base, mode_w, device_p));

    val &= ~(CFG0_BIT_QOS_VAL_MSK);
    val |= CFG0_BIT_QOS_VAL_VAl(value);

    writel(val, AXICFG_REG_CFG0(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg1_set_basic_rate(u32 base, u8 mode_w,
                                                 u8 device_p, u8 value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG1(base, mode_w, device_p));

    val &= ~(CFG1_BIT_BASIC_RATE_MSK);
    val |= CFG1_BIT_BASIC_RATE_VAl(value);

    writel(val, AXICFG_REG_CFG1(base, mode_w, device_p));
}

static inline void axicfg_hw_cfg1_set_burst_rate(u32 base, u8 mode_w,
                                                 u8 device_p, u32 value)
{
    hal_log_debug("hal_axicfg %s:%d\n", __FUNCTION__, __LINE__);

    u32 val = readl(AXICFG_REG_CFG1(base, mode_w, device_p));

    val &= ~(CFG1_BIT_BURST_RATE_MSK);
    val |= CFG1_BIT_BURST_RATE_VAl(value);

    writel(val, AXICFG_REG_CFG1(base, mode_w, device_p));
}

#ifdef __cplusplus
}
#endif

#endif
