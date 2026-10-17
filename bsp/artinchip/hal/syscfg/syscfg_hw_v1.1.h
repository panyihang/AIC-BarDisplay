/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
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

/* The register definition of SYSCFG V11 */
#define IRQ_CTL                 (SYSCFG_BASE + 0x008)
#define IRQ_STA                 (SYSCFG_BASE + 0x00C)
#define LDO25_CFG               (SYSCFG_BASE + 0x020)
#define LDO18_CFG               (SYSCFG_BASE + 0x024)
#define LDO1x_CFG               (SYSCFG_BASE + 0x028)
#define CMP_CFG                 (SYSCFG_BASE + 0x03C)
#define USB_REXT                (SYSCFG_BASE + 0x048)
#define PSEN_CFG                (SYSCFG_BASE + 0x0C0)
#define PSEN_CNT_VAL            (SYSCFG_BASE + 0x0C4)
#define SYS_SRAM_PAR            (SYSCFG_BASE + 0x100)
#define CPU_SRAM_PAR            (SYSCFG_BASE + 0x104)
#define USB_SRAM_PAR            (SYSCFG_BASE + 0x108)
#define VE_SRAM_PAR             (SYSCFG_BASE + 0x10C)
#define GE_SRAM_PAR             (SYSCFG_BASE + 0x110)
#define DE_SRAM_PAR             (SYSCFG_BASE + 0x114)
#define SRAM_CLK_CFG            (SYSCFG_BASE + 0x140)
#define SRAM_MAP_CFG            (SYSCFG_BASE + 0x160)
#define FLASH_CFG               (SYSCFG_BASE + 0x1F0)
#define ENCODER_CFG             (SYSCFG_BASE + 0x1F4)
#define USB0_CFG                (SYSCFG_BASE + 0x40C)
#define EMAC_CFG                (SYSCFG_BASE + 0x410)
#define DBG_CFG                 (SYSCFG_BASE + 0xF00)
#define DBG_DATA                (SYSCFG_BASE + 0xF08)
#define SYSCFG_VER              (SYSCFG_BASE + 0xFFC)

/* The field definition of IRQ_CTL */
#define IRQ_CTL_CMP_RST_EN                      BIT(31)
#define IRQ_CTL_CMP_IRQ_EN                      BIT(0)

/* The field definition of IRQ_STA */
#define IRQ_STA_CMP_IRQ_STA                     BIT(0)

/* The field definition of LDO25_CFG */
#define LDO25_CFG_ATB_ANA_EN                    BIT(27)
#define LDO25_CFG_ATB_ANA_SEL_MASK              GENMASK(26, 24)
#define LDO25_CFG_ATB_ANA_SEL_SHIFT             (24)
#define LDO25_CFG_ATB_BIAS_EN                   BIT(22)
#define LDO25_CFG_ATB_BIAS_SEL_MASK             GENMASK(21, 20)
#define LDO25_CFG_ATB_BIAS_SEL_SHIFT            (20)
#define LDO25_CFG_LVDS0_IBIAS_EN                BIT(18)
#define LDO25_CFG_XSPI_DLLC1_IBIAS_EN           BIT(17)
#define LDO25_CFG_XSPI_DLLC0_IBIAS_EN           BIT(16)
#define LDO25_CFG_BG_CTRL_MASK                  GENMASK(15, 8)
#define LDO25_CFG_BG_CTRL_SHIFT                 (8)
#define LDO25_CFG_LDO25_EN                      BIT(4)
#define LDO25_CFG_LDO25_VAL_MASK                GENMASK(2, 0)
#define LDO25_CFG_LDO25_VAL_SHIFT               (0)

/* The field definition of LDO18_CFG */
#define LDO18_CFG_ATB2_ANA_EN                   BIT(27)
#define LDO18_CFG_ATB2_ANA_SEL_MASK             GENMASK(25, 24)
#define LDO18_CFG_ATB2_ANA_SEL_SHIFT            (24)
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

/* The field definition of CMP_CFG */
#define CMP_CFG_CMP_DB_MASK                     GENMASK(31, 24)
#define CMP_CFG_CMP_DB_SHIFT                    (24)
#define CMP_CFG_CMP_MODE                        BIT(5)
#define CMP_CFG_CMP_EN                          BIT(4)
#define CMP_CFG_CMP_SEL_MASK                    GENMASK(2, 0)
#define CMP_CFG_CMP_SEL_SHIFT                   (0)

/* The field definition of USB_REXT */
#define USB_REXT_RES_CAL_EN                     BIT(8)
#define USB_REXT_RES_CAL_VAL_MASK               GENMASK(7, 0)
#define USB_REXT_RES_CAL_VAL_SHIFT              (0)
#define USB_REXT_RES_EN_VAL(v)                  (((v) << USB_REXT_RES_CAL_VAL_SHIFT) & USB_REXT_RES_CAL_VAL_MASK)

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
#define SYS_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define SYS_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of CPU_SRAM_PAR */
#define CPU_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define CPU_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of USB_SRAM_PAR */
#define USB_SRAM_PAR_SRAM_PAR_MASK              GENMASK(31, 0)
#define USB_SRAM_PAR_SRAM_PAR_SHIFT             (0)

/* The field definition of VE_SRAM_PAR */
#define VE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(31, 0)
#define VE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of GE_SRAM_PAR */
#define GE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(31, 0)
#define GE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of DE_SRAM_PAR */
#define DE_SRAM_PAR_SRAM_PAR_MASK               GENMASK(31, 0)
#define DE_SRAM_PAR_SRAM_PAR_SHIFT              (0)

/* The field definition of SRAM_CLK_CFG */
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_MASK       GENMASK(17, 0)
#define SRAM_CLK_CFG_SRAM_CLK_UNGATE_SHIFT      (0)

/* The field definition of SRAM_MAP_CFG */
#define SRAM_MAP_CFG_AXI_MAT_S0_CFG_MASK                GENMASK(15, 8)
#define SRAM_MAP_CFG_AXI_MAT_S0_CFG_SHIFT               (8)
#define SRAM_MAP_CFG_AXI_MAT_S1_SIZE_MASK               GENMASK(6, 4)
#define SRAM_MAP_CFG_AXI_MAT_S1_SIZE_SHIFT              (4)
#define SRAM_MAP_CFG_CPU_TCM_SRAM_ACLK_GATE             BIT(1)
#define SRAM_MAP_CFG_CPU_TCM_SRAM_CFG                   BIT(0)

/* The field definition of FLASH_CFG */
#define FLASH_CFG_FLASH_IOMAP_012_MASK          GENMASK(14, 12)
#define FLASH_CFG_FLASH_IOMAP_012_SHIFT         (12)
#define FLASH_CFG_FLASH_IOMAP_345_MASK          GENMASK(10, 8)
#define FLASH_CFG_FLASH_IOMAP_345_SHIFT         (8)
#define FLASH_CFG_FLASH_SRCSEL_MASK             GENMASK(1, 0)
#define FLASH_CFG_FLASH_SRCSEL_SHIFT            (0)

/* The field definition of ENCODER_CFG */
#define ENCODER_CFG_ENC1_SEL_MASK               GENMASK(17, 16)
#define ENCODER_CFG_ENC1_SEL_SHIFT              (16)
#define ENCODER_CFG_ENC0_SEL_MASK               GENMASK(1, 0)
#define ENCODER_CFG_ENC0_SEL_SHIFT              (0)

/* The field definition of USB0_CFG */
#define USB0_CFG_DRD_MODE                       BIT(0)

/* The field definition of EMAC_CFG */
#define EMAC_CFG_EMAC_REFCLK_INV                BIT(29)
#define EMAC_CFG_EMAC_REFDLY_SEL_MASK           GENMASK(28, 24)
#define EMAC_CFG_EMAC_REFDLY_SEL_SHIFT          (24)
#define EMAC_CFG_EMAC_TXCLK_INV                 BIT(17)
#define EMAC_CFG_EMAC_TXDLY_SEL_MASK            GENMASK(16, 12)
#define EMAC_CFG_EMAC_TXDLY_SEL_SHIFT           (12)
#define EMAC_CFG_EMAC_TXDLY_EN_VAL(v)          (((v) << EMAC_CFG_EMAC_TXDLY_SEL_SHIFT) & EMAC_CFG_EMAC_TXDLY_SEL_MASK)
#define EMAC_CFG_SW_TXCLK_DIV2_MASK             GENMASK(11, 8)
#define EMAC_CFG_SW_TXCLK_DIV2_SHIFT            (8)
#define EMAC_CFG_SW_TXCLK_DIV1_MASK             GENMASK(7, 4)
#define EMAC_CFG_SW_TXCLK_DIV1_SHIFT            (4)
#define EMAC_CFG_SW_TXCLK_DIV_EN                BIT(2)
#define EMAC_CFG_RMII_EXTCLK_SEL                BIT(1)

/* The field definition of DBG_CFG */
#define DBG_CFG_DBG_KEY_MASK                    GENMASK(31, 16)
#define DBG_CFG_DBG_KEY_SHIFT                   (16)
#define DBG_CFG_DBG_MODE_SEL_MASK               GENMASK(15, 14)
#define DBG_CFG_DBG_MODE_SEL_SHIFT              (14)
#define DBG_CFG_DBG_CLK_SEL_MASK                GENMASK(13, 8)
#define DBG_CFG_DBG_CLK_SEL_SHIFT               (8)
#define DBG_CFG_DBG_OUT_SEL_MASK                GENMASK(7, 0)
#define DBG_CFG_DBG_OUT_SEL_SHIFT               (0)

/* The field definition of DBG_DATA */
#define DBG_DATA_DBG_DATA_MASK                  GENMASK(31, 0)
#define DBG_DATA_DBG_DATA_SHIFT                 (0)

/* The field definition of SYSCFG_VER */
#define SYSCFG_VER_VERSION_MASK                 GENMASK(31, 0)
#define SYSCFG_VER_VERSION_SHIFT                (0)

#define IOMAP_EFUSE_WID                 9
#define EFUSE_DATA_IOMAP_POS            24

#define REFERENCE_VOLTAGE           24000
#define VOLTAGE_SPACING             1000

static inline u32 syscfg_hw_read_ldo_cfg(void)
{
    u32 ldo25_val;

    ldo25_val = readl(LDO25_CFG);
    ldo25_val &= LDO25_CFG_LDO25_VAL_MASK;

    return ldo25_val * VOLTAGE_SPACING + REFERENCE_VOLTAGE;
}

static inline void syscfg_hw_usb_init(u32 res_val)
{
#if defined(AIC_USING_USB0_HOST) || defined(AIC_USING_USB0_OTG) || defined(AICUPG_UDISK_ENABLE)
    u32 val;

    val = readl(USB_REXT);
    val &= ~USB_REXT_RES_CAL_VAL_MASK;
    val |= USB_REXT_RES_EN_VAL(res_val);
    val |= USB_REXT_RES_CAL_EN;
    writel(val, USB_REXT);
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

static inline void syscfg_hw_gmac_init(u32 gmac)
{
    u32 val;

    val = readl(EMAC_CFG);

    if (gmac == 0) {
        #ifdef AIC_DEV_GMAC0_PHY_EXTCLK
        val |= EMAC_CFG_RMII_EXTCLK_SEL;
        #endif
        #if AIC_DEV_GMAC0_TXDELAY
        val |= EMAC_CFG_EMAC_TXDLY_EN_VAL(AIC_DEV_GMAC0_TXDELAY);
        #endif
    }

    writel(val, EMAC_CFG);
}

static inline void syscfg_hw_ldo25_xspi_init(void)
{
    u32 val;
    val = readl(LDO25_CFG);

    // clear bit16, bit17
    val &= ~(LDO25_CFG_XSPI_DLLC0_IBIAS_EN | LDO25_CFG_XSPI_DLLC1_IBIAS_EN);
    // set bit16, bit17
    val |= (LDO25_CFG_XSPI_DLLC0_IBIAS_EN | LDO25_CFG_XSPI_DLLC1_IBIAS_EN);

    writel(val, LDO25_CFG);
}

enum syscfg_ldo18_cfg_ldo18_en_t {
    LDO18_EN_DISABLE = 0,
    LDO18_EN_ENABLE = 1,
};

#define LDO18_ENABLE_BIT4_VAL_STEP1   0x0
#define LDO18_ENABLE_BIT4_VAL_STEP2   0x10

[[maybe_unused]] static inline void syscfg_hw_ldo18_init(u8 status, u8 v_level)
{
    u32 val = 0;

    if (status == LDO18_EN_ENABLE) {
        /*SD suggest*/
        val |= (LDO18_ENABLE_BIT4_VAL_STEP1 | (v_level & LDO18_CFG_LDO18_VAL_MASK));
        writel(val, LDO18_CFG);
        aicos_udelay(1);

        val = 0;
        val |= (LDO18_ENABLE_BIT4_VAL_STEP2 | (v_level & LDO18_CFG_LDO18_VAL_MASK));
        writel(val, LDO18_CFG);
        aicos_udelay(10);
    } else {
        (void)v_level;
        val = readl(LDO18_CFG);
        val &= ~LDO18_CFG_LDO18_EN;
        writel(val, LDO18_CFG);
    }
}

#ifdef __cplusplus
}
#endif

#endif
