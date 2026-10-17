/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: jiji.chen <jiji.chen@artinchip.com>
 */

#ifndef _AIC_HAL_SYSCFG_REGS_H_
#define _AIC_HAL_SYSCFG_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The register definition of SYSCFG V12 */
#define LDO25_CFG                               (SYSCFG_BASE + 0x020)
#define LDO18_CFG                               (SYSCFG_BASE + 0x024)
#define LDO1x_CFG                               (SYSCFG_BASE + 0x028)
#define USB0_REXT                               (SYSCFG_BASE + 0x048)
#define ATB_CFG                                 (SYSCFG_BASE + 0x070)
#define PSEN_CFG                                (SYSCFG_BASE + 0x0C0)
#define PSEN_CNT_VAL                            (SYSCFG_BASE + 0x0C4)
#define SYS_SRAM_PAR                            (SYSCFG_BASE + 0x100)
#define CPU_SRAM_PAR                            (SYSCFG_BASE + 0x104)
#define VE_SRAM_PAR                             (SYSCFG_BASE + 0x10C)
#define GE_SRAM_PAR                             (SYSCFG_BASE + 0x110)
#define DE_SRAM_PAR                             (SYSCFG_BASE + 0x114)
#define SRAM_CLK_CFG                            (SYSCFG_BASE + 0x140)
#define VE_SRAM_MAP                             (SYSCFG_BASE + 0x164)
#define FLASH_CFG                               (SYSCFG_BASE + 0x1F0)
#define USB_PLL_COM                             (SYSCFG_BASE + 0x400)
#define USB0_CFG                                (SYSCFG_BASE + 0x40C)
#define SYSCFG_VER                              (SYSCFG_BASE + 0xFFC)

/* The field definition of LDO25_CFG */
#define LDO25_CFG_ATB_ANA_EN                    BIT(27)
#define LDO25_CFG_ATB_ANA_SEL_MASK              GENMASK(26, 24)
#define LDO25_CFG_ATB_ANA_SEL_SHIFT             (24)
#define LDO25_CFG_ATB_BIAS_SEL_MASK             GENMASK(21, 20)
#define LDO25_CFG_ATB_BIAS_SEL_SHIFT            (20)
#define LDO25_CFG_RESEVER_IBIAS_EN              BIT(18)
#define LDO25_CFG_XSPI_DLLC1_IBIAS_EN           BIT(17)
#define LDO25_CFG_XSPI_DLLC0_IBIAS_EN           BIT(16)
#define LDO25_CFG_BG_CTRL_MASK                  GENMASK(15, 8)
#define LDO25_CFG_BG_CTRL_SHIFT                 (8)
#define LDO25_CFG_LDO25_EN                      BIT(4)
#define LDO25_CFG_LDO25_VAL_MASK                GENMASK(2, 0)
#define LDO25_CFG_LDO25_VAL_SHIFT               (0)

/* The field definition of LDO18_CFG */
#define LDO18_CFG_LDO18_PD_FAST                 BIT(5)
#define LDO18_CFG_LDO18_EN                      BIT(4)
#define LDO18_CFG_LDO18_VAL_MASK                GENMASK(2, 0)
#define LDO18_CFG_LDO18_VAL_SHIFT               (0)

/* The field definition of LDO1x_CFG */
#define LDO1x_CFG_LDO1X_SOFT_EN                 BIT(6)
#define LDO1x_CFG_LDO1X_PD_FAST                 BIT(5)
#define LDO1x_CFG_LDO1X_EN                      BIT(4)
#define LDO1x_CFG_LDO1X_VAL_MASK                GENMASK(3, 0)
#define LDO1x_CFG_LDO1X_VAL_SHIFT               (0)

/* The field definition of ATB_CFG */
#define ATB_CFG_ATB3_SEL_MASK                   GENMASK(26, 25)
#define ATB_CFG_ATB3_SEL_SHIFT                  (25)
#define ATB_CFG_ATB3_OUT_EN                     BIT(24)
#define ATB_CFG_ATB2_SEL_MASK                   GENMASK(18, 17)
#define ATB_CFG_ATB2_SEL_SHIFT                  (17)
#define ATB_CFG_ATB2_OUT_EN                     BIT(16)
#define ATB_CFG_ATB1_SEL_MASK                   GENMASK(10, 9)
#define ATB_CFG_ATB1_SEL_SHIFT                  (9)
#define ATB_CFG_ATB1_OUT_EN                     BIT(8)
#define ATB_CFG_ATB0_SEL_MASK                   GENMASK(3, 1)
#define ATB_CFG_ATB0_SEL_SHIFT                  (1)
#define ATB_CFG_ATB0_OUT_EN                     BIT(0)

/* The field definition of PSEN_CFG */
#define PSEN_CFG_CNT_TIME_MASK                  GENMASK(31, 16)
#define PSEN_CFG_CNT_TIME_SHIFT                 (16)
#define PSEN_CFG_RO_SEL_MASK                    GENMASK(3, 1)
#define PSEN_CFG_RO_SEL_SHIFT                   (1)
#define PSEN_CFG_PSEN_START                     BIT(0)

/* The field definition of PSEN_CNT_VAL */
#define PSEN_CNT_VAL_CNT_VAL_MASK               GENMASK(15, 0)
#define PSEN_CNT_VAL_CNT_VAL_SHIFT              (0)

/* The field definition of SYS_SRAM_PAR */
#define SYS_SRAM_PAR_SRAM_PAR_MASK              GENMASK(23, 0)
#define SYS_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of CPU_SRAM_PAR */
#define CPU_SRAM_PAR_SRAM_PAR_MASK1              GENMASK(23, 16)
#define CPU_SRAM_PAR_SRAM_PAR_SHIFT1             (16)
#define CPU_SRAM_PAR_SRAM_PAR_MASK0              GENMASK(7, 0)
#define CPU_SRAM_PAR_SRAM_PAR_SHIFT0             (0)

/* The field definition of VE_SRAM_PAR */
#define VE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(15, 0)
#define VE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of GE_SRAM_PAR */
#define GE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(15, 0)
#define GE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of DE_SRAM_PAR */
#define DE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(15, 0)
#define DE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of SRAM_CLK_CFG */
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_MASK       GENMASK(18, 0)
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_SHIFT      (0)

/* The field definition of VE_SRAM_MAP */
#define VE_SRAM_MAP_SRAM                        BIT(0)

/* The field definition of FLASH_CFG */
#define FLASH_CFG_FLASH_IOMAP_012_MASK          GENMASK(14, 12)
#define FLASH_CFG_FLASH_IOMAP_012_SHIFT         (12)
#define FLASH_CFG_FLASH_IOMAP_345_MASK          GENMASK(10, 8)
#define FLASH_CFG_FLASH_IOMAP_345_SHIFT         (8)
#define FLASH_CFG_FLASH_SRCSEL_MASK             GENMASK(1, 0)
#define FLASH_CFG_FLASH_SRCSEL_SHIFT            (0)
#define FLASH_CFG_FLASH_SRCSEL_VAL(v)          (((v) << FLASH_CFG_FLASH_SRCSEL_SHIFT) & FLASH_CFG_FLASH_SRCSEL_MASK)

/* The field definition of USB_PLL */
#define USB_PLL_GEN_EN                          BIT(16)

/* The field definition of USB0_CFG */
#define USB0_CFG_DRD_MODE                       BIT(0)

/* The field definition of USB0_REXT */
#define USB0_REXT_RES_CAL_EN                    BIT(8)
#define USB0_REXT_RES_CAL_VAL_MASK              GENMASK(7, 0)
#define USB0_REXT_RES_CAL_VAL_SHIFT             (0)
#define USB0_REXT_RES_EN_VAL(v)                 (((v) << USB0_REXT_RES_CAL_VAL_SHIFT) & USB0_REXT_RES_CAL_VAL_MASK)


/* The field definition of SYSCFG_VER */
#define SYSCFG_VER_VERSION_MASK                 GENMASK(31, 0)
#define SYSCFG_VER_VERSION_SHIFT                (0)

#define IOMAP_EFUSE_WID                 7
#define EFUSE_DATA_IOMAP_POS            8

#define REFERENCE_VOLTAGE           24000
#define VOLTAGE_SPACING             1000

static inline void syscfg_hw_usb_init(u32 res_val)
{
    s32 __attribute__((unused)) val;

#if defined(AIC_USING_USB0_HOST) || defined(AIC_USING_USB0_OTG) || defined(AICUPG_UDISK_ENABLE)
    val = readl(USB0_REXT);
    val &= ~USB0_REXT_RES_CAL_VAL_MASK;
    val |= USB0_REXT_RES_EN_VAL(res_val);
    val |= USB0_REXT_RES_CAL_EN;
    writel(val, USB0_REXT);
#endif

}

static inline void syscfg_hw_usb_phy0_set_host(void)
{
    u32 val;

    val = readl(USB0_CFG);
    val &= ~USB0_CFG_DRD_MODE;
    writel(val, USB0_CFG);
}

static inline void syscfg_hw_usb_phy0_set_device(void)
{
    u32 val;

    val = readl(USB0_CFG);
    val |= USB0_CFG_DRD_MODE;
    writel(val, USB0_CFG);
}

static inline u32 syscfg_hw_read_ldo_cfg(void)
{
    u32 ldo25_val;

    ldo25_val = readl(LDO25_CFG);
    ldo25_val &= LDO25_CFG_LDO25_VAL_MASK;

    return ldo25_val * VOLTAGE_SPACING + REFERENCE_VOLTAGE;
}

static inline void syscfg_hw_ldo1x_enable(u8 v_level)
{
    u32 val = 0;

    /*SD suggest: set the LDO1X_CFG_LDO1X_PD_FAST_SHIFT to 0, other bit not use.*/
    val |= (v_level & LDO1x_CFG_LDO1X_VAL_MASK);
    writel(val, LDO1x_CFG);
}

static inline void syscfg_hw_ldo1x_disable()
{
    u32 val = 0;

    val = readl(LDO1x_CFG);
    val &= LDO1x_CFG_LDO1X_VAL_MASK;

    /*BIT4, BIT5, BIT6, DISABLE*/
    writel(val, LDO1x_CFG);
    val |= LDO1x_CFG_LDO1X_SOFT_EN;
    writel(val, LDO1x_CFG);
}

#if defined(AIC_SYSCFG_SIP_FLASH_ENABLE)
static inline void syscfg_hw_sip_flash_init(void)
{
    u32 val, ctrl_id;

    ctrl_id = 2 + AIC_SIP_FLASH_ACCESS_QSPI_ID;

#if defined(AIC_USING_SID)
u32 map;
    /* 1. Read eFuse to set SiP flash IO mapping */
    hal_efuse_clk_enable();
    hal_efuse_read(IOMAP_EFUSE_WID, &val);
    hal_efuse_clk_disable();
    map = (val >> EFUSE_DATA_IOMAP_POS) & 0xFF;

    /* 2. Set the SiP flash's access Controller */
    val = map << 8 | (ctrl_id & 0x3);
#else   /* AIC_USING_SID */
    val = readl(FLASH_CFG);
    val &= ~FLASH_CFG_FLASH_SRCSEL_MASK;
    val |= ctrl_id;
#endif

    writel(val, FLASH_CFG);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
