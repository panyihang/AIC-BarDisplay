/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "hal_tsen.h"
#include "aic_hal_clk.h"
#include <string.h>
#ifdef AIC_SID_DRV
#include "hal_efuse.h"
#endif

#define AIC_TSEN_TIMEOUT    3000

#ifdef AIC_TSEN_DRV_V12
#define TSEN_SHARE_ACQ
#endif

/* Register definition of Thermal Sensor Controller */
#define TSEN_MCR        0x000
#define TSEN_INTR       0x004
#ifdef TSEN_SHARE_ACQ
#define TSEN_ACQ        0x010
#endif
#define TSENn_CFG(n)    (0x100 + (((n) & 0x3) << 5))
#define TSENn_ITV(n)    (0x100 + (((n) & 0x3) << 5) + 0x4)
#define TSENn_FIL(n)    (0x100 + (((n) & 0x3) << 5) + 0x8)
#define TSENn_DATA(n)   (0x100 + (((n) & 0x3) << 5) + 0xC)
#define TSENn_INT(n)    (0x100 + (((n) & 0x3) << 5) + 0x10)
#define TSENn_HTAV(n)   (0x100 + (((n) & 0x3) << 5) + 0x14)
#define TSENn_LTAV(n)   (0x100 + (((n) & 0x3) << 5) + 0x18)
#ifndef AIC_TSEN_DRV_V10
#define TSENn_OTPV(n)   (0x100 + (((n) & 0x3) << 5) + 0x1C)
#endif
#define TSEN_VERSION    0xFFC

#define TSEN_MCR_CTLX_EN(n)     (BIT(20) << (n))
#define TSEN_MCR_TSEN_EN(n)     (BIT(16) << n)

#ifndef TSEN_SHARE_ACQ
#define TSEN_MCR_ACQ_SHIFT      8
#define TSEN_MCR_ACQ_MASK       GENMASK(15, 8)
#endif
#define TSEN_MCR_MAX_SHIFT      4
#define TSEN_MCR_MAX_MASK       GENMASK(5, 4)
#define TSEN_MCR_EN             BIT(0)

#define TSEN_INTR_CH_INT_FLAG(n)    (BIT(16) << (n))
#define TSEN_INTR_CH_INT_EN(n)      (BIT(0) << (n))

#define TSENn_CFG_DIFF_MODE_SELECT  BIT(31)
#define TSENn_CFG_INVERTED_SELECT   BIT(27)
#ifndef TSEN_SHARE_ACQ
#define TSENn_CFG_ADC_ACQ_VAL       0xff
#define TSENn_CFG_ADC_ACQ_SHIFT     8
#define TSENn_CFG_ADC_ACQ_MASK      GENMASK(15, 8)
#endif
#define TSENn_CFG_HIGH_ADC_PRIORITY BIT(4)
#define TSENn_CFG_PERIOD_SAMPLE_EN  BIT(1)
#define TSENn_CFG_SINGLE_SAMPLE_EN  BIT(0)

#define TSENn_ITV_SHIFT         16

#define TSENn_FIL_2_POINTS      1
#define TSENn_FIL_4_POINTS      2
#define TSENn_FIL_8_POINTS      3

#define TSENn_INT_OTP_FLAG          BIT(28)
#define TSENn_INT_LTA_RM_FLAG       BIT(27)
#define TSENn_INT_LTA_VALID_FLAG    BIT(26)
#define TSENn_INT_HTA_RM_FLAG       BIT(25)
#define TSENn_INT_HTA_VALID_FLAG    BIT(24)
#define TSENn_INT_DAT_OVW_FLAG      BIT(17)
#define TSENn_INT_DAT_RDY_FLAG      BIT(16)
#define TSENn_INT_OTP_RESET         (0xA << 12)
#define TSENn_INT_OTP_IE            (0x5 << 12)
#define TSENn_INT_LTA_RM_IE         BIT(11)
#define TSENn_INT_LTA_VALID_IE      BIT(10)
#define TSENn_INT_HTA_RM_IE         BIT(9)
#define TSENn_INT_HTA_VALID_IE      BIT(8)
#define TSENn_INT_DAT_OVW_IE        BIT(1)
#define TSENn_INT_DATA_RDY_IE       BIT(0)

#define TSENn_HLTA_EN               BIT(31)
#define TSENn_HLTA_RM_THD_SHIFT     16
#define TSENn_HLTA_RM_THD_MASK      GENMASK(27, 16)
#define TSENn_HLTA_THD_MASK         GENMASK(11, 0)
#define TSENn_HTA_RM_THD(t)         ((t) - 3)
#define TSENn_LTA_RM_THD(t)         ((t) + 3)

#define TSENn_OTPV_EN               BIT(31)
#define TSENn_OTPV_VAL_MASK         GENMASK(11, 0)

#define THERMAL_CORE_TEMP_AMPN_SCALE   10 // in RT-Thread sensor framework
#define TSEN_CALIB_ACCURACY_SCALE      1000 // = 10000 / 10

#define TSEN_VOLTAGE_SCALE_UNIT         2.14285
#define TSEN_TRIM_VOLTAGE_BOUNDARY_VAL  0x80

#if defined(AIC_SID_DRV_V10)
#define TSEN_CP_VERSION_OFFSET          0x1C
#define TSEN_LDO30_BG_CTRL_OFFSET       0x28
#define TSEN_THS0_ADC_VAL_LOW_OFFSET    0x2c
#define TSEN_THS_ENV_TEMP_LOW_OFFSET    0x24
#define TSEN_THS0_ADC_VAL_HIGH_OFFSET   0x20
#define TSEN_THS0_ADC_VAL_HIGH_MASK     0xfff
#define TSEN_THS0_ADC_VAL_LOW_MASK      0xfff
#define TSEN_THS1_ADC_VAL_LOW_MASK      GENMASK(23, 12)
#define TSEN_THS1_ADC_VAL_LOW_SHIFT     12
#define TSEN_LDO30_BG_CTRL_MASK         0xff
#define TSEN_THS_ENV_TEMP_HIGH_MASK     0xff
#define TSEN_THS_ENV_TEMP_LOW_MASK      0Xf
#define TSEN_CP_VERSION_MASK            0x3f
#define TSEN_THS_ENV_TEMP_LOW_SHIFT     16
#define TSEN_THS_ENV_TEMP_HIGH_SHIFT    24
#define TSEN_CP_VERSION_SHIFT           20
#define TSEN_SINGLE_POINT_CALI_K_CPU    -1132
#define TSEN_SINGLE_POINT_CALI_K_GPAI   -1159
#elif defined(AIC_SID_DRV_V11)
#define TSEN_CP_VERSION_OFFSET          0x1C
#define TSEN_CP_VERSION_MASK            0x3f
#define TSEN_CP_VERSION_SHIFT           20
#define TSEN_THS0_ADC_VAL_LOW_OFFSET    0x2c
#define TSEN_THS_ENV_TEMP_LOW_OFFSET    0x2c
#define TSEN_THS0_ADC_VAL_HIGH_OFFSET   0x34
#define TSEN_THS0_ADC_VAL_LOW_MASK      0xfff
#define TSEN_THS0_ADC_VAL_HIGH_MASK     0xfff
#define TSEN_THS1_ADC_VAL_LOW_MASK      GENMASK(23, 12)
#define TSEN_THS1_ADC_VAL_LOW_SHIFT     12
#define TSEN_THS_ENV_TEMP_LOW_MASK      0Xff
#define TSEN_THS_ENV_TEMP_HIGH_MASK     0xff
#define TSEN_THS_ENV_TEMP_LOW_SHIFT     24
#define TSEN_THS_ENV_TEMP_HIGH_SHIFT    24
#define TSEN_SINGLE_POINT_CALI_K_CPU    940
#define TSEN_SINGLE_POINT_CALI_K_GPAI   940
#elif defined(AIC_SID_DRV_V12)
#define TSEN_CP_VERSION_OFFSET          0x1C
#define TSEN_CP_VERSION_MASK            0x3f
#define TSEN_CP_VERSION_SHIFT           20
#define TSEN_THS0_ADC_VAL_LOW_OFFSET    0x0c
#define TSEN_THS_ENV_TEMP_LOW_OFFSET    0x0c
#define TSEN_THS0_ADC_VAL_LOW_MASK      0xfff
#define TSEN_THS_ENV_TEMP_LOW_MASK      0Xff
#define TSEN_THS_ENV_TEMP_LOW_SHIFT     24
#define TSEN_SINGLE_POINT_CALI_K_CPU    950
#elif defined(AIC_SID_DRV_V17)
#define TSEN_CP_VERSION_OFFSET          0x08
#define TSEN_CP_VERSION_MASK            0x3f
#define TSEN_CP_VERSION_SHIFT           16
#define TSEN_THS0_ADC_VAL_LOW_OFFSET    0x0c
#define TSEN_THS_ENV_TEMP_LOW_OFFSET    0x0c
#define TSEN_THS0_ADC_VAL_LOW_MASK      0xfff
#define TSEN_THS_ENV_TEMP_LOW_MASK      0xf
#define TSEN_THS_ENV_TEMP_LOW_SHIFT     24
#define TSEN_SINGLE_POINT_CALI_K_CPU    993
#endif

#define TSEN_ORIGIN_STANDARD_VOLTAGE    3000 // = 3 * 1000
#define TSEN_EFUSE_STANDARD_LENGTH      4

#define TSEN_ENV_TEMP_LOW_BASE          25
#define TSEN_ENV_TEMP_HIGH_BASE         65
#define TSEN_ENV_TEMP_LOW_SIGN_MASK     BIT(3)
#define TSEN_ENV_TEMP_HIGH_SIGN_MASK    BIT(7)

#define TSEN_CP_VERSION_DIFF_TYPE       0xA
static u32 aic_tsen_ch_num = 0; // the number of available sensor
extern struct aic_tsen_ch aic_tsen_chs[AIC_TSEN_CH_NUM];

static inline void tsen_writel(u32 val, int reg)
{
    writel(val, TSEN_BASE + reg);
}

static inline u32 tsen_readl(int reg)
{
    return readl(TSEN_BASE + reg);
}

/* Temperature = ADC data * slope + offset.
 * 1. Temperature accuracy adopts 1000.
 * 2. Slope and Offset accuracy adopts 10000.
 */
s32 hal_tsen_data2temp(struct aic_tsen_ch *chan)
{
    int temp = 0;

    if (chan->latest_data == 4095 || chan->latest_data == 0)
        return 0;

    if (!chan->slope)
        return chan->latest_data;

    temp = chan->slope * chan->latest_data;
    temp += chan->offset;

    if ((temp % TSEN_CALIB_ACCURACY_SCALE) < (TSEN_CALIB_ACCURACY_SCALE / 2))
        temp = temp / TSEN_CALIB_ACCURACY_SCALE;
    else
        temp = temp / TSEN_CALIB_ACCURACY_SCALE + 1;

    hal_log_debug("%s temperature: %d -> %d.%d\n", chan->name,
                  chan->latest_data,
                  temp / THERMAL_CORE_TEMP_AMPN_SCALE,
                  temp % THERMAL_CORE_TEMP_AMPN_SCALE);
    return temp;
}

/* ADC data = (Temperature - offset) / slope */
u16 hal_tsen_temp2data(struct aic_tsen_ch *chan, s32 temp)
{
    int data = temp * TSEN_CALIB_ACCURACY_SCALE - chan->offset;

    if (!chan->slope)
        return (u16)temp;

    data /= chan->slope;
    hal_log_debug("%s data: %d -> %d\n", chan->name, temp, data);
    return (u16)data;
}

void hal_tsen_set_ch_num(u32 num)
{
    aic_tsen_ch_num = num;
}

struct aic_tsen_ch *hal_tsen_get_valid_ch(u32 ch)
{
    s32 i;

    if (ch >= AIC_TSEN_CH_NUM) {
        pr_err("Invalid channel %d\n", ch);
        return NULL;
    }

    for (i = 0; i < aic_tsen_ch_num; i++) {
        if (!aic_tsen_chs[i].available)
            continue;

        if (aic_tsen_chs[i].id == ch)
            return &aic_tsen_chs[i];
    }

    pr_warn("Ch%d is unavailable!\n", ch);
    return NULL;
}

#ifdef AIC_SID_DRV
int hal_tsen_efuse_read(u32 addr, u32 *data, u32 size)
{
    u32 wid, wval, rest, cnt;
    int ret;
    int length = TSEN_EFUSE_STANDARD_LENGTH;

    if (hal_efuse_clk_enable()) {
        return -1;
    }

    rest = size;
    while (rest > 0) {
        wid = addr >> 2;
        ret = hal_efuse_read(wid, &wval);
        if (ret)
            break;
        *data = wval;
        cnt = rest;
        if (addr % length) {
            if (rest > (length - (addr % length)))
                cnt = (length - (addr % length));
        } else {
            if (rest > length)
                cnt = length;
        }
        addr += cnt;
        rest -= cnt;
    }

    hal_efuse_clk_disable();

    return (int)(size - rest);
}
#endif

/* The temperature obtained from nvmem contains sign bits.
 * For this purpose, this function converts data through sign bit mask
 */
int hal_tsen_env_temp_cali(u8 sign_mask, u8 val)
{
    if (val & sign_mask)
        return -(val & (sign_mask - 1));
    else
        return val & (sign_mask - 1);

}

#ifdef AIC_SID_DRV
void hal_tsen_single_point_cali(struct aic_tsen_ch *chan)
{
    u32 ths0_adc_val = 0;
    u32 ths0_adc_val_low;
    u32 ths_env_temp_low = 0;
    int env_temp_low = TSEN_ENV_TEMP_LOW_BASE;
    int length = TSEN_EFUSE_STANDARD_LENGTH;
    int cali_scale;

    cali_scale = THERMAL_CORE_TEMP_AMPN_SCALE * TSEN_CALIB_ACCURACY_SCALE;

    /* The variable ths0_adc_val is the data result of the entire row for
     * ths0_adc_val in the SID. */
    hal_tsen_efuse_read(TSEN_THS0_ADC_VAL_LOW_OFFSET, &ths0_adc_val, length);
    ths0_adc_val_low = ths0_adc_val & TSEN_THS0_ADC_VAL_LOW_MASK;

#ifdef AIC_USING_TSEN_GPAI
    u32 ths1_adc_val_low = (ths0_adc_val & TSEN_THS1_ADC_VAL_LOW_MASK) >> TSEN_THS1_ADC_VAL_LOW_SHIFT;
#endif

    hal_tsen_efuse_read(TSEN_THS_ENV_TEMP_LOW_OFFSET, &ths_env_temp_low,
                        length);
    ths_env_temp_low = (ths_env_temp_low >> TSEN_THS_ENV_TEMP_LOW_SHIFT) & TSEN_THS_ENV_TEMP_LOW_MASK;
    env_temp_low += hal_tsen_env_temp_cali(TSEN_ENV_TEMP_LOW_SIGN_MASK,
                                           ths_env_temp_low);
    pr_debug("env_temp_low:%d, ths0_adc_val_low:%d\n", env_temp_low, ths0_adc_val_low);

    switch (chan->id) {
    case AIC_TSEN_SENSOR_CPU:
        chan->slope = TSEN_SINGLE_POINT_CALI_K_CPU;
        chan->offset = env_temp_low * cali_scale - chan->slope * ths0_adc_val_low;
        break;
#ifdef AIC_USING_TSEN_GPAI
    case AIC_TSEN_SENSOR_GPAI:
        chan->slope = TSEN_SINGLE_POINT_CALI_K_GPAI;
        chan->offset = env_temp_low * cali_scale - chan->slope * ths1_adc_val_low;
        break;
#endif
    default:
        pr_err("The macro definition for the offset value of the corresponding sensor is missing");
        return;
    }

    return;
}

#ifdef AIC_TSEN_DRV_V10
void hal_tsen_single_point_cali_for_correct(struct aic_tsen_ch *chan)
{
    u32 ldo30_bg_ctrl = 0;
    u32 ths0_adc_val = 0;
    u32 ths0_adc_val_low;
    u32 ths_env_temp_low = 0;
    int env_temp_low = TSEN_ENV_TEMP_LOW_BASE;
    int standard_vol = TSEN_ORIGIN_STANDARD_VOLTAGE;
    int vol_scale_unit = TSEN_VOLTAGE_SCALE_UNIT;
    int length = TSEN_EFUSE_STANDARD_LENGTH;
    int origin_voltage;
    int origin_adc;
    int cali_scale;

    cali_scale = THERMAL_CORE_TEMP_AMPN_SCALE * TSEN_CALIB_ACCURACY_SCALE;

    hal_tsen_efuse_read(TSEN_LDO30_BG_CTRL_OFFSET, &ldo30_bg_ctrl, length);
    ldo30_bg_ctrl = ldo30_bg_ctrl & TSEN_LDO30_BG_CTRL_MASK;
    hal_tsen_efuse_read(TSEN_THS0_ADC_VAL_LOW_OFFSET, &ths0_adc_val, length);
    ths0_adc_val_low = ths0_adc_val & TSEN_THS0_ADC_VAL_LOW_MASK;

#ifdef AIC_USING_TSEN_GPAI
    u32 ths1_adc_val_low = (ths0_adc_val & TSEN_THS1_ADC_VAL_LOW_MASK) >> TSEN_THS1_ADC_VAL_LOW_SHIFT;
#endif

    if (ldo30_bg_ctrl > TSEN_TRIM_VOLTAGE_BOUNDARY_VAL) {
        origin_voltage = standard_vol - (255 - ldo30_bg_ctrl) * vol_scale_unit;
    } else {
        origin_voltage = standard_vol + ldo30_bg_ctrl * vol_scale_unit;
    }

    hal_tsen_efuse_read(TSEN_THS_ENV_TEMP_LOW_OFFSET, &ths_env_temp_low,
                        length);
    ths_env_temp_low = (ths_env_temp_low >> TSEN_THS_ENV_TEMP_LOW_SHIFT) & TSEN_THS_ENV_TEMP_LOW_MASK;
    env_temp_low += hal_tsen_env_temp_cali(TSEN_ENV_TEMP_LOW_SIGN_MASK,
                                           ths_env_temp_low);

    switch (chan->id) {
    case AIC_TSEN_SENSOR_CPU:
        chan->slope = TSEN_SINGLE_POINT_CALI_K_CPU;
        origin_adc = origin_voltage * ths0_adc_val_low / standard_vol;
        break;

#ifdef AIC_USING_TSEN_GPAI
    case AIC_TSEN_SENSOR_GPAI:
        chan->slope = TSEN_SINGLE_POINT_CALI_K_GPAI;
        origin_adc = origin_voltage * ths1_adc_val_low / standard_vol;
        break;
#endif
    default:
        pr_err("The macro definition for the offset value of the corresponding sensor is missing");
        return;
    }
    chan->offset = env_temp_low * cali_scale - chan->slope * origin_adc;
    hal_log_debug("k:%d, b:%d\n", chan->slope, chan->offset);

    return;
}
#endif

/* For temperature's slope and offset calculated by y=kx+b equation.
 * THS0_ADC_VAL as y, THS_ENV_TEMP as x.
 */
#if !defined(AIC_SID_DRV_V12) && !defined(AIC_SID_DRV_V17)
static void hal_tsen_get_cali_param(int y1, int y2, int x1, int x2,
                   struct aic_tsen_ch *chan)
{
    int slope, offset;
    int calib_scale;

    calib_scale = THERMAL_CORE_TEMP_AMPN_SCALE * TSEN_CALIB_ACCURACY_SCALE;
    if ((x2 - x1) != 0) {
        slope = (y1 - y2) * calib_scale / (x2 - x1);
        offset = y1 * calib_scale - slope * x1;
        chan->slope = slope;
        chan->offset = offset;
    }
}

void hal_tsen_double_point_cali(struct aic_tsen_ch *chan)
{
    int env_temp_low = TSEN_ENV_TEMP_LOW_BASE;
    int env_temp_high = TSEN_ENV_TEMP_HIGH_BASE;
    int length = TSEN_EFUSE_STANDARD_LENGTH;
    u32 ths_env_temp_low = 0;
    u8 ths_env_temp_high = 0;
    u32 ths0_adc_val_low = 0;
    u32 ths_temp_high = 0;
    u16 ths0_adc_val_high;

    hal_tsen_efuse_read(TSEN_THS_ENV_TEMP_LOW_OFFSET, &ths_env_temp_low,
                        length);
    ths_env_temp_low = (ths_env_temp_low >> TSEN_THS_ENV_TEMP_LOW_SHIFT) & TSEN_THS_ENV_TEMP_LOW_MASK;

    hal_tsen_efuse_read(TSEN_THS0_ADC_VAL_LOW_OFFSET, &ths0_adc_val_low,
                        length);
    ths0_adc_val_low = ths0_adc_val_low & TSEN_THS0_ADC_VAL_LOW_MASK;

    hal_tsen_efuse_read(TSEN_THS0_ADC_VAL_HIGH_OFFSET, &ths_temp_high, length);
    ths_env_temp_high = (ths_temp_high >> TSEN_THS_ENV_TEMP_HIGH_SHIFT) & TSEN_THS_ENV_TEMP_HIGH_MASK;
    ths0_adc_val_high = ths_temp_high & TSEN_THS0_ADC_VAL_HIGH_MASK;


    env_temp_low += hal_tsen_env_temp_cali(TSEN_ENV_TEMP_LOW_SIGN_MASK,
                                           ths_env_temp_low);
    env_temp_high += hal_tsen_env_temp_cali(TSEN_ENV_TEMP_HIGH_SIGN_MASK,
                                            ths_env_temp_high);

    hal_tsen_get_cali_param(ths0_adc_val_low, ths0_adc_val_high,
                            ths_env_temp_low, ths_env_temp_high, chan);
}
#endif

void hal_tsen_curve_fitting(struct aic_tsen_ch *chan)
{
    int cp_version;
    u32 data = 0;

    hal_tsen_efuse_read(TSEN_CP_VERSION_OFFSET, &data,
                        TSEN_EFUSE_STANDARD_LENGTH);
    cp_version = data >> TSEN_CP_VERSION_SHIFT & TSEN_CP_VERSION_MASK;
    if (!cp_version)
        return;

    hal_log_debug("CP version:%d\n", cp_version);

#ifdef AIC_TSEN_DRV_V10
    if (cp_version >= TSEN_CP_VERSION_DIFF_TYPE) {
        hal_tsen_single_point_cali(chan);
    } else {
        hal_tsen_single_point_cali_for_correct(chan);
    }
#else
    hal_tsen_single_point_cali(chan);
#endif

    return;
}
#endif // end of AIC_SID_DRV

u32 tsen_sec2itv(u32 pclk, u32 sec)
{
    u32 tmp = pclk >> 16;

    tmp *= sec;
    return tmp;
}

void hal_tsen_enable(int enable)
{
    int mcr;

    mcr = tsen_readl(TSEN_MCR);
    if (enable)
        mcr |= TSEN_MCR_EN;
    else
        mcr &= ~TSEN_MCR_EN;

#ifdef TSEN_SHARE_ACQ
    if (enable)
        tsen_writel(0x30199, TSEN_ACQ);
#else
    mcr |= TSEN_MCR_ACQ_MASK;
#endif
    tsen_writel(mcr, TSEN_MCR);
}

void hal_tsen_ch_enable(u32 ch, int enable)
{
    int mcr;

    mcr = tsen_readl(TSEN_MCR);
    if (enable) {
        mcr |= TSEN_MCR_TSEN_EN(ch);
        mcr |= TSEN_MCR_CTLX_EN(ch);
    } else {
        mcr &= ~TSEN_MCR_TSEN_EN(ch);
        mcr &= ~TSEN_MCR_CTLX_EN(ch);
    }

    tsen_writel(mcr, TSEN_MCR);
}

static void tsen_int_enable(u32 ch, u32 enable, u32 detail)
{
    u32 val = 0;

    val = tsen_readl(TSEN_INTR);
    if (enable) {
        val |= TSEN_INTR_CH_INT_EN(ch);
        tsen_writel(detail, TSENn_INT(ch));
    } else {
        val &= ~TSEN_INTR_CH_INT_EN(ch);
        tsen_writel(0, TSENn_INT(ch));
    }
    tsen_writel(val, TSEN_INTR);
}

static void tsen_ch_acq_set(u32 ch)
{
#ifndef TSEN_SHARE_ACQ
    u32 val = 0;

    val = tsen_readl(TSENn_CFG(ch));
    val &= ~TSENn_CFG_ADC_ACQ_MASK;
    val |= TSENn_CFG_ADC_ACQ_VAL << TSENn_CFG_ADC_ACQ_SHIFT;
    tsen_writel(val, TSENn_CFG(ch));
#endif
}

static void tsen_single_smp(u32 ch)
{
    tsen_writel(TSENn_CFG_SINGLE_SAMPLE_EN | tsen_readl(TSENn_CFG(ch)),
           TSENn_CFG(ch));
}

/* Only in period mode, HTA, LTA and OTP are available */
static void tsen_setup_mode(struct aic_tsen_ch *chan, u32 pclk)
{
    u32 val, detail = TSENn_INT_DATA_RDY_IE;
    u32 cfg_val = 0;

    val = 0;
    if (chan->hta_enable) {
        detail |= TSENn_INT_HTA_RM_IE | TSENn_INT_HTA_VALID_IE;
        val = TSENn_HLTA_EN
            | ((chan->hta_rm_thd << TSENn_HLTA_RM_THD_SHIFT)
            & TSENn_HLTA_RM_THD_MASK)
            | (chan->hta_thd & TSENn_HLTA_THD_MASK);
    }
    tsen_writel(val, TSENn_HTAV(chan->id));

    val = 0;
    if (chan->lta_enable) {
        detail |= TSENn_INT_LTA_RM_IE | TSENn_INT_LTA_VALID_IE;
        val = TSENn_HLTA_EN
            | ((chan->lta_rm_thd << TSENn_HLTA_RM_THD_SHIFT)
            & TSENn_HLTA_RM_THD_MASK)
            | (chan->lta_thd & TSENn_HLTA_THD_MASK);
    }
    tsen_writel(val, TSENn_LTAV(chan->id));

#ifndef AIC_TSEN_DRV_V10
    val = 0;
    if (chan->otp_enable) {
        detail |= TSENn_INT_OTP_RESET;
        val = TSENn_OTPV_EN | (chan->otp_thd & TSENn_OTPV_VAL_MASK);
    }
    tsen_writel(val, TSENn_OTPV(chan->id));
#endif
    tsen_int_enable(chan->id, 1, detail);

    tsen_writel(TSENn_FIL_8_POINTS, TSENn_FIL(chan->id));

    tsen_ch_acq_set(chan->id);

    val = 0;
    cfg_val = tsen_readl(TSENn_CFG(chan->id));
    cfg_val &= ~(TSENn_CFG_PERIOD_SAMPLE_EN | TSENn_CFG_SINGLE_SAMPLE_EN);
    if (chan->mode == AIC_TSEN_MODE_PERIOD) {
        val = tsen_sec2itv(pclk, chan->smp_period);
        cfg_val |= TSENn_CFG_PERIOD_SAMPLE_EN;
    }
    tsen_writel(val << TSENn_ITV_SHIFT | 0xFFFF, TSENn_ITV(chan->id));
    tsen_writel(cfg_val, TSENn_CFG(chan->id));

    if (chan->mode == AIC_TSEN_MODE_PERIOD)
        hal_tsen_ch_enable(chan->id, 1);
}

static void tsen_diff_mode(u32 ch, u32 diff, u32 inverted)
{
    u32 val = tsen_readl(TSENn_CFG(ch));

    if (diff)
        val |= TSENn_CFG_DIFF_MODE_SELECT;
    else
        val &= ~TSENn_CFG_DIFF_MODE_SELECT;

    if (inverted)
        val |= TSENn_CFG_INVERTED_SELECT;
    else
        val &= ~TSENn_CFG_INVERTED_SELECT;

    tsen_writel(val, TSENn_CFG(ch));
}

int hal_tsen_ch_init(struct aic_tsen_ch *chan, u32 pclk)
{
    tsen_setup_mode(chan, pclk);

#ifdef AIC_ADCIM_DM_DRV
    chan->inverted = 1;
#endif

    if (chan->diff_mode || chan->inverted)
        tsen_diff_mode(chan->id, chan->diff_mode, chan->inverted);

    /* For single mode, should init the channel in .get_temp() */
    return 0;
}

int hal_tsen_get_temp(struct aic_tsen_ch *chan, s32 *val)
{
    int ret = 0;

    if (!chan->available) {
        hal_log_err("%s is unavailable!\n", chan->name);
        return -ENODATA;
    }

    if (chan->mode == AIC_TSEN_MODE_SINGLE) {
        tsen_single_smp(chan->id);
        hal_tsen_ch_enable(chan->id, 1);
    }

    aicos_sem_reset(chan->complete, 0);
    ret = aicos_sem_take(chan->complete, AIC_TSEN_TIMEOUT);
    if (ret < 0) {
        hal_log_err("%s read timeout!\n", chan->name);
        hal_tsen_ch_enable(chan->id, 0);
        return -ETIMEDOUT;
    }
    if (chan->mode == AIC_TSEN_MODE_SINGLE)
        hal_tsen_ch_enable(chan->id, 0);

    if (val)
        *val = hal_tsen_data2temp(chan);

    return 0;
}

void hal_tsen_status_show(struct aic_tsen_ch *chan)
{
    int mcr = tsen_readl(TSEN_MCR);
    int version = tsen_readl(TSEN_VERSION);

    printf("In Thermal Sensor V%s:\n"
               "Ch Name          Mode Diff Inv Enable Value  LTA  HTA  OTP\n"
               "%d  %-13s %4s %4d %3d %6d %5d %4d %4d %4d\n",
               EXPAND_BCD_VER(version),
               chan->id, chan->name, chan->mode ? "P" : "S",
               chan->diff_mode, chan->inverted,
               mcr & TSEN_MCR_TSEN_EN(chan->id) ? 1 : 0,
               chan->latest_data, chan->lta_thd, chan->hta_thd, chan->otp_thd);
}

irqreturn_t hal_tsen_irq_handle(int irq, void *arg)
{
    int i, status, detail = 0;
    struct aic_tsen_ch *chan = NULL;

    status = tsen_readl(TSEN_INTR);
    hal_log_debug("Module IRQ status: %#x\n", status);
    for (i = 0; i < AIC_TSEN_CH_NUM; i++) {
        if (!(status & TSEN_INTR_CH_INT_FLAG(i)))
            continue;

        chan = hal_tsen_get_valid_ch(i);
        if (!chan)
            continue;

        detail = tsen_readl(TSENn_INT(i));
        hal_log_debug("ch%d IRQ status: %#x\n", i, detail);
        if (detail & TSENn_INT_DAT_RDY_FLAG) {
            chan->latest_data = tsen_readl(TSENn_DATA(i));
            hal_log_debug("ch%d data %d\n", i, chan->latest_data);

            aicos_sem_give(chan->complete);
        }
        tsen_writel(detail, TSENn_INT(i));

        if (detail & TSENn_INT_LTA_VALID_FLAG)
            hal_log_warn("LTA: ch%d %d(%d)!\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
        if (detail & TSENn_INT_LTA_RM_FLAG)
            hal_log_warn("LTA removed: ch%d %d(%d)\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
        if (detail & TSENn_INT_HTA_VALID_FLAG)
            hal_log_warn("HTA: ch%d %d(%d)!\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
        if (detail & TSENn_INT_HTA_RM_FLAG)
            hal_log_warn("HTA removed: ch%d %d(%d)\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
        if (detail & TSENn_INT_OTP_FLAG)
            hal_log_warn("OTP: ch%d %d(%d)!\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
        if (detail & TSENn_INT_DAT_OVW_FLAG)
            hal_log_warn("Data Over Write: ch%d %d(%d)!\n", i,
                 hal_tsen_data2temp(chan), chan->latest_data);
    }

    hal_log_debug("IRQ status %#x, detail %#x\n", status, detail);

    return IRQ_HANDLED;
}

s32 hal_tsen_clk_init(void)
{
    s32 ret = 0;

    ret = hal_clk_enable(CLK_TSEN);
    if (ret < 0) {
        pr_err("TSensor clk enable failed!\n");
        return -1;
    }

    ret = hal_clk_enable_deassertrst(CLK_TSEN);
    if (ret < 0) {
        pr_err("TSensor reset deassert failed!\n");
        return -1;
    }

    return ret;
}

void hal_tsen_pclk_get(struct aic_tsen_ch *chan)
{
    chan->pclk_rate = hal_clk_get_freq(hal_clk_get_parent(CLK_TSEN));
}
