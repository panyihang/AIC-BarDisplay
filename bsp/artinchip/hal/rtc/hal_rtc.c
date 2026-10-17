/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_hal_clk.h"

#include "hal_rtc.h"

#define AIC_RTC_NAME                    "aic-rtc"

#define RTC_REG_CTL                     (0x0000)
#define RTC_REG_INIT                    (0x0004)
#define RTC_REG_AON_IRQ_EN              (0x0008)
#define RTC_REG_AON_IRQ_STA             (0x000C)
#ifdef AIC_RTC_DRV_V121
#define RTC_REG_IO1_CFG                 (0x0010)
#define RTC_SLEEP_CFG                   (0x0014)
#endif
#define RTC_REG_TIME0                   (0x0020)
#define RTC_REG_TIME1                   (0x0024)
#define RTC_REG_TIME2                   (0x0028)
#define RTC_REG_TIME3                   (0x002C)
#define RTC_REG_ALARM0                  (0x0030)
#define RTC_REG_ALARM1                  (0x0034)
#define RTC_REG_ALARM2                  (0x0038)
#define RTC_REG_ALARM3                  (0x003C)
#define RTC_REG_CALI0                   (0x0040)
#define RTC_REG_CALI1                   (0x0044)
#define RTC_REG_ANALOG0                 (0x0050)
#define RTC_REG_ANALOG1                 (0x0054)
#define RTC_REG_ANALOG2                 (0x0058)
#define RTC_REG_ANALOG3                 (0x005C)
#define RTC_REG_WR_EN                   (0x00FC)
#ifdef AIC_RTC_DRV_V121
#define RTC_REG_WAKEUP_ADDR0            (0x0110)
#define RTC_REG_WAKEUP_ADDR1            (0x0114)
#define RTC_REG_WAKEUP_ADDR2            (0x0118)
#define RTC_REG_WAKEUP_ADDR3            (0x011C)
#define RTC_REG_SYSBAK                  (0x0120)
#define RTC_REG_WAKEUP_STATUS           (0x0130)
#define RTC_REG_BOOT_INFO               (0x0134)
#else
#define RTC_REG_SYSBAK                  (0x0100)
#endif
#define RTC_REG_TCNT                    (0x0800)
#define RTC_REG_32K_DET                 (0x0804)
#ifdef AIC_RTC_DRV_V121
#define RTC_REG_IRQ_EN                  (0x0808)
#define RTC_REG_IRQ_STA                 (0x080C)
#endif
#define RTC_REG_VER                     (0x08FC)

#ifdef AIC_RTC_DRV_V121
#define RTC_IO1_CFG                     BIT(7)
#define RTC_IO0_CFG                     BIT(6)
#define RTC_CTL_IO0_SEL_SHIFT           4
#define RTC_CTL_IO0_SEL_MASK            GENMASK(5, 4)
#define RTC_CTL_IO0_SEL(x)              (((x) & 0x3) << RTC_CTL_IO0_SEL_SHIFT)
#endif
#define RTC_CTL_IO_OUTPUT_ENABLE        1
#define RTC_CTL_IO_ALARM_OUTPUT         2
#define RTC_CTL_IO_32K_CLK_OUTPUT       3
#define RTC_CTL_IO_SEL_SHIFT            4
#define RTC_CTL_IO_SEL_MASK             GENMASK(5, 4)
#define RTC_CTL_IO_SEL(x)               (((x) & 0x3) << RTC_CTL_IO_SEL_SHIFT)
#define RTC_CTL_ALARM_EN                BIT(2)
#define RTC_CTL_TCNT_EN                 BIT(0)

#ifdef AIC_RTC_DRV_V121
#define RTC_IO1_RISE_IRQ_EN             BIT(7)
#define RTC_IO1_FALL_IRQ_EN             BIT(6)
#endif
#define RTC_IRQ_32K_ERR_EN              BIT(2)
#define RTC_IRQ_ALARM_EN                BIT(0)

#ifdef AIC_RTC_DRV_V121
#define RTC_IO1_TRIG_SEL_SHIFT          2
#define RTC_IO1_TRIG_SEL_MASK           GENMASK(3, 2)
#define RTC_IO1_TRIG_SEL(x)             (((x) & 0x3) << RTC_IO1_TRIG_SEL_SHIFT)
#define RTC_IO1_TRG_OUT_EN              BIT(1)
#define RTC_IO1_TRG_EN                  BIT(0)
#endif

#ifdef AIC_RTC_DRV_V121
#define RTC_IO1_RISE_IRQ_STA            BIT(7)
#define RTC_IO1_FALL_IRQ_STA            BIT(6)
#endif
#define RTC_IRQ_STA_32K_ERR             BIT(2)
#define RTC_IRQ_STA_ALARM_IO            BIT(1)
#define RTC_IRQ_STA_ALARM               BIT(0)

#define RTC_CAL1_FAST_DIR               BIT(7)

#define RTC_ANA0_RC1M_ISEL              BIT(7)
#define RTC_ANA0_RC1M_EN                BIT(6)
#define RTC_ANA0_LDO18_BYPASS           BIT(4)
#define RTC_ANA0_LDO18_VOL_MASK         GENMASK(3, 1)
#define RTC_ANA0_LDO18_VOL_SHIFT        (1)
#define RTC_ANA0_LDO18_EN               BIT(0)

#define RTC_ANA0_LDO18_VOL_120          7

#define RTC_ANA1_PD_CUR_SEL_MASK        GENMASK(6, 5)
#define RTC_ANA1_PD_CUR_SEL_SHIFT       (5)
#define RTC_ANA1_PD_CUR_EN              BIT(4)
#define RTC_ANA1_LDO11_VOL_MASK         GENMASK(3, 1)
#define RTC_ANA1_LDO11_VOL_SHIFT        (1)
#define RTC_ANA1_LDO11_LPEN             BIT(0)

#define RTC_ANA1_PD_CUR_SEL_025         0
#define RTC_ANA1_PD_CUR_SEL_050         1
#define RTC_ANA1_PD_CUR_SEL_075         2
#define RTC_ANA1_PD_CUR_SEL_100         3

#define RTC_ANA1_LDO11_VOL_090          4
#define RTC_ANA1_LDO11_VOL_080          6

#define RTC_ANA2_XTAL32K_DRV_MASK       GENMASK(3, 0)

#ifdef AIC_RTC_DRV_V20
#define RTC_ANA2_XTAL32K_EN             BIT(0)
#endif

#define RTC_ANA3_LDO12_XTAL32K_SW       BIT(1)
#define RTC_ANA3_XTAL32K_EN             BIT(0)

#define RTC_32K_DET_EN                  BIT(0)

#define RTC_WR_EN_KEY                   0xAC

#define RTC_DRV_TIMEOUT                 msecs_to_jiffies(2000)
#define RTC_32K_FREQ                    (32 * 1024 * 100)

#define RTC_WRITE_ENABLE    writeb(RTC_WR_EN_KEY, RTC_BASE + RTC_REG_WR_EN)
#define RTC_WRITE_DISABLE   writeb(0, RTC_BASE + RTC_REG_WR_EN)
#define RTC_WRITEB(val, reg) \
        do { \
                RTC_WRITE_ENABLE; \
                writeb((val) & 0xFF, RTC_BASE + (reg)); \
                RTC_WRITE_DISABLE; \
        } while (0)
#define RTC_WRITEL(val, reg) \
        do { \
                RTC_WRITE_ENABLE; \
                writeb((val) & 0xFF, RTC_BASE + (reg)); \
                writeb(((val) >> 8) & 0xFF, RTC_BASE + (reg) + 0x4); \
                writeb(((val) >> 16) & 0xFF, RTC_BASE + (reg) + 0x8); \
                writeb(((val) >> 24) & 0xFF, RTC_BASE + (reg) + 0xC); \
                RTC_WRITE_DISABLE; \
        } while (0)
#define RTC_READB(reg)  readb(RTC_BASE + reg)
#define RTC_READL(reg)  (readb(RTC_BASE + reg) \
                        | (readb((RTC_BASE + reg) + 0x4) << 8) \
                        | (readb((RTC_BASE + reg) + 0x8) << 16) \
                        | (readb((RTC_BASE + reg) + 0xC) << 24))
#define RTC_READ_CNT()  readl(RTC_BASE + RTC_REG_TCNT)

#define RTC_REBOOT_REASON_MASK          GENMASK(7, 4)
#define RTC_REBOOT_REASON_SHIFT         4

struct aic_rtc_dev {
    rtc_callback_t callback;
#ifdef AIC_RTC_DRV_V121
    rtc_callback_t io1back;
#endif
    u32 clk_rate;
    u32 clk_drv;
    u32 alarm_io;
    u32 cal_fast;
    s32 cal_val;
    s32 inited;
};

static struct aic_rtc_dev aich_rtc = {0};

#if 0
static void aic_rtc_set_32k_drv(u8 drv)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_ANALOG2);
    val &= ~RTC_ANA2_XTAL32K_DRV_MASK;
    val |= drv;
    RTC_WRITEB(val, RTC_REG_ANALOG2);
}
#endif

/* calibrate: (100 * 1024 * 1024 + cval)/( freq / 32) = 1024
 * freq = 100 * actual OSC frequency
 */
void hal_rtc_cali(s32 clk_rate)
{
    /* calibrate: (1024*1024+cval)/(freq/32) = 1024 */
    s64 cval = 0;

    if (clk_rate == RTC_32K_FREQ)
        return; /* It's perfect, need not calibrate */

    cval = 100 * 1024 * 1024 - clk_rate * 32;
    if (clk_rate > RTC_32K_FREQ)
        cval -= 50;
    else
        cval += 50;

    cval /= 100;
    hal_log_debug("RTC Calibration %d(clk %d)\n", cval, clk_rate);
    if (cval > 0)
        cval |= RTC_CAL1_FAST_DIR << 8;
    else
        cval = -cval;

    RTC_WRITEB((cval >> 8) & 0xFF, RTC_REG_CALI1);
    RTC_WRITEB(cval & 0xFF, RTC_REG_CALI0);
}

void hal_rtc_enable(u32 enable)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    if (enable)
        val |= RTC_CTL_TCNT_EN;
    else
        val &= ~RTC_CTL_TCNT_EN;
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_read_time(u32 *sec)
{
    *sec = RTC_READ_CNT();
}

void hal_rtc_set_time(u32 sec)
{
#if defined(AIC_RTC_DRV_V12) || defined(AIC_RTC_DRV_V121)
    //must clear TCNT_INIT before setting time in RTC V1.2
    RTC_WRITEB(0, RTC_REG_INIT);
#endif
    hal_rtc_enable(0);
    RTC_WRITEL(sec, RTC_REG_TIME0);
    RTC_WRITEB(1, RTC_REG_INIT);
    hal_rtc_enable(1);
}

s32 hal_rtc_read_alarm(u32 *sec)
{
    /* get alarm target time */
    *sec = RTC_READL(RTC_REG_ALARM0);

    return RTC_READB(RTC_REG_AON_IRQ_EN) & RTC_IRQ_ALARM_EN;
}

static int hal_rtc_alarm_irq_enable(u32 enabled)
{
    u32 val = 0;

    val = RTC_READB(RTC_REG_AON_IRQ_EN);
    val &=~(RTC_IRQ_32K_ERR_EN | RTC_IRQ_ALARM_EN);
    if (enabled)
        val |= (RTC_IRQ_32K_ERR_EN | RTC_IRQ_ALARM_EN);

    RTC_WRITEB(val, RTC_REG_AON_IRQ_EN);
    return 0;
}

void hal_rtc_alarm_io_output(void)
{
    u32 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    val |= RTC_CTL_IO_SEL(RTC_CTL_IO_ALARM_OUTPUT);
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_32k_clk_output(void)
{
    u32 val = 0;

    val = RTC_READB(RTC_REG_CTL);
    val |= RTC_CTL_IO_SEL(RTC_CTL_IO_32K_CLK_OUTPUT);
    RTC_WRITEB(val, RTC_REG_CTL);
}

void hal_rtc_set_alarm(u32 sec)
{
    u32 value;

    /* First, disable alarm */
    value = RTC_READB(RTC_REG_CTL);
    value &= ~RTC_CTL_ALARM_EN;
    RTC_WRITEB(value, RTC_REG_CTL);

    /* set alarm time value */
    RTC_WRITEL(sec, RTC_REG_ALARM0);

    /* Then, enable alarm */
    value |= RTC_CTL_ALARM_EN;
    RTC_WRITEB(value, RTC_REG_CTL);

    /* enable alarm irq */
    hal_rtc_alarm_irq_enable(1);
}

#ifdef AIC_RTC_DRV_V121
void hal_rtc_io_cfg(u32 sel)
{
    u32 val;

    val = RTC_READB(RTC_REG_CTL);
    val &=~RTC_CTL_IO0_SEL_MASK;
    val |= RTC_CTL_IO0_SEL(sel) | (RTC_IO0_CFG | RTC_IO1_CFG);
    RTC_WRITEL(val, RTC_REG_CTL);
}

void hal_rtc_io1_cfg(u32 trg_en, u32 out_en, u32 trg_sel)
{
    u32 val;

    val = RTC_READB(RTC_REG_IO1_CFG);
    val &= ~(RTC_IO1_TRG_EN | RTC_IO1_TRG_OUT_EN);
    if (trg_en)
        val |= RTC_IO1_TRG_EN;
    if (out_en)
        val |= RTC_IO1_TRG_OUT_EN;
    RTC_WRITEL(val, RTC_REG_IO1_CFG);

    val = RTC_READB(RTC_REG_IO1_CFG);
    val &=~RTC_IO1_TRIG_SEL_MASK;
    val |= RTC_IO1_TRIG_SEL(trg_sel);
    RTC_WRITEL(val, RTC_REG_IO1_CFG);
}

void hal_rtc_io1_irq_en(u32 irq_en)
{
    u32 val;

    val = RTC_READB(RTC_REG_AON_IRQ_EN);
    val &= ~RTC_IO1_RISE_IRQ_EN;
    if (irq_en)
        val |= RTC_IO1_RISE_IRQ_EN;
    RTC_WRITEB(val, RTC_REG_AON_IRQ_EN);
}

s32 hal_rtc_io1_register_callback(rtc_callback_t callback)
{
    if (callback == NULL) {
        hal_log_err("Invalid callback function!\n");
        return -1;
    }

    aich_rtc.io1back = callback;
    return 0;
}

void hal_rtc_set_wakeup_addr(u32 addr)
{
    RTC_WRITEL(addr, RTC_REG_WAKEUP_ADDR0);
}

void hal_rtc_set_boot_info(u8 info)
{
    RTC_WRITEL(info, RTC_REG_BOOT_INFO);
}
#endif

static void hal_rtc_low_power(void)
{
#ifdef AIC_RTC_LOW_POWER
    u8 val = 0;

    val |= RTC_ANA0_RC1M_EN | RTC_ANA0_LDO18_EN;
    val |= RTC_ANA0_LDO18_VOL_120 << RTC_ANA0_LDO18_VOL_SHIFT;
    RTC_WRITEB(val, RTC_REG_ANALOG0);

    val = RTC_ANA1_PD_CUR_SEL_075 << RTC_ANA1_PD_CUR_SEL_SHIFT;
    val |= RTC_ANA1_LDO11_VOL_090 << RTC_ANA1_LDO11_VOL_SHIFT;
    val |= RTC_ANA1_LDO11_LPEN;
    RTC_WRITEB(val, RTC_REG_ANALOG1);

#if defined(AIC_RTC_DRV_V11) || defined(AIC_RTC_DRV_V12) || defined(AIC_RTC_DRV_V121)
    val = RTC_ANA3_LDO12_XTAL32K_SW | RTC_ANA3_XTAL32K_EN;
    RTC_WRITEB(val, RTC_REG_ANALOG3);

    val = RTC_ANA0_RC1M_EN;
    val |= RTC_ANA0_LDO18_VOL_120 << RTC_ANA0_LDO18_VOL_SHIFT;
    RTC_WRITEB(val, RTC_REG_ANALOG0);
#endif
#endif
}

s32 hal_rtc_register_callback(rtc_callback_t callback)
{
    if (callback == NULL) {
        hal_log_err("Invalid callback function!\n");
        return -1;
    }

    aich_rtc.callback = callback;
    return 0;
}

irqreturn_t hal_rtc_irq(int irq, void *arg)
{
    u8 val = 0;

    val = RTC_READB(RTC_REG_AON_IRQ_STA);
    RTC_WRITEB(val, RTC_REG_AON_IRQ_STA);
    hal_log_debug("IRQ status %#x\n", val);

    if (val & RTC_IRQ_STA_32K_ERR)
        hal_log_err("The 32K clk is not fast enough\n");

    if (val & RTC_IRQ_STA_ALARM) {
        if (aich_rtc.callback)
            aich_rtc.callback();
    }

#ifdef AIC_RTC_DRV_V121
    if (val & (RTC_IO1_FALL_IRQ_STA | RTC_IO1_RISE_IRQ_STA)) {
        if (aich_rtc.io1back)
            aich_rtc.io1back();
    }
#endif
    return IRQ_HANDLED;
}

s32 hal_rtc_init(void)
{
    s32 ret = 0;
#if defined(AIC_RTC_DRV_V10) || defined(AIC_RTC_DRV_V20) || defined(AIC_RTC_DRV_V12) || defined(AIC_RTC_DRV_V121)
    u8 val = 0;
#endif

    if (aich_rtc.inited) {
        hal_log_info("RTC is already inited.\n");
        return 0;
    }
#ifdef CLK_RTC
    ret = hal_clk_enable(CLK_RTC);
    if (ret < 0) {
        hal_log_err("RTC clk enable failed!\n");
        return -1;
    }
#endif
    /* Check & clean poweroff alarm status */
    ret = RTC_READB(RTC_REG_AON_IRQ_STA);
    if (ret) {
        hal_log_debug("IRQ_STA is %#x\n", ret);
        RTC_WRITEB(ret, RTC_REG_AON_IRQ_STA);
        if (ret & RTC_IRQ_STA_ALARM_IO)
            hal_log_info("Powered by RTC alarm.\n");
    }

#if defined(AIC_RTC_DRV_V10)
    val = RTC_READB(RTC_REG_ANALOG1);
    val |= RTC_ANA1_LDO11_LPEN;
    RTC_WRITEB(val, RTC_REG_ANALOG1);
#elif defined(AIC_RTC_DRV_V20)
    val = RTC_READB(RTC_REG_ANALOG2);
    val |= RTC_ANA2_XTAL32K_EN;
    RTC_WRITEB(val, RTC_REG_ANALOG2);
#elif defined(AIC_RTC_DRV_V12) || defined(AIC_RTC_DRV_V121)
    val = RTC_READB(RTC_REG_ANALOG3);
    val |= RTC_ANA3_XTAL32K_EN;
    RTC_WRITEB(val, RTC_REG_ANALOG3);
#endif

    aich_rtc.inited = 1;
    hal_rtc_enable(1);
    hal_rtc_low_power();
    return 0;
}

s32 hal_rtc_deinit(void)
{
    if (!aich_rtc.inited) {
        hal_log_warn("RTC is not inited.\n");
        return -1;
    }
#ifdef CLK_RTC
    hal_clk_disable(CLK_RTC);
#endif
    aich_rtc.inited = 0;
    return 0;
}
