/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_iopoll.h"
#include "aic_hal_clk.h"

#include "hal_adcim.h"
#ifdef AIC_SYSCFG_DRV
#include "hal_syscfg.h"
#endif

/* Register of ADCIM */
#define ADCIM_MCSR       0x000
#define ADCIM_CALCSR     0x004
#define ADCIM_FIFOSTS    0x008
#define ADCIM_VERSION    0xFFC

#ifdef FPGA_BOARD_ARTINCHIP
#define ADCIM_DEFAULT_FREQ      24000000
#else
#define ADCIM_DEFAULT_FREQ      48000000
#endif

#define ADCIM_MCSR_BUSY                 BIT(16)
#define ADCIM_MCSR_SEMFLAG_SHIFT        8
#define ADCIM_MCSR_SEMFLAG_MASK         GENMASK(15, 8)
#define ADCIM_MCSR_CHN_MASK             GENMASK(3, 0)
#define ADCIM_CALCSR_CALVAL_UPD         BIT(31)
#define ADCIM_CALCSR_CALVAL_SHIFT       16
#define ADCIM_CALCSR_CALVAL_MASK        GENMASK(27, 16)
#define ADCIM_CALCSR_ADC_ACQ_SHIFT      8
#define ADCIM_CALCSR_ADC_ACQ_MASK       GENMASK(15, 8)
#ifdef AIC_ADCIM_DELTA_ADC
#define ADCIM_CALCSR_ADC_CAL_SEL        BIT(4)
#endif
#define ADCIM_CALCSR_DCAL_MASK          BIT(1)
#define ADCIM_CALCSR_CAL_ENABLE         BIT(0)
#define ADCIM_FIFOSTS_ADC_ARBITER_IDLE  BIT(6)
#define ADCIM_FIFOSTS_FIFO_ERR          BIT(5)
#define ADCIM_FIFOSTS_CTR_MASK          GENMASK(4, 0)
#define ADCIM_CAL_ADC_STANDARD_VAL      0x800
#define AIC_ADC_MAX_VAL                 0xFFF
#define AIC_VOLTAGE_ACCURACY            10000
#define ADCIM_CALCSR_NUM                6
#define ADCIM_ADC_ACC_BIT               12
#define ADCIM_ADC_ACC_RANGE             ((1 << ADCIM_ADC_ACC_BIT) - 1)

#ifdef AIC_CHIP_D12X
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x8
#elif defined(AIC_CHIP_D13X)
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x28
#elif defined(AIC_CHIP_D21X)
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x32
#elif defined(AIC_CHIP_D12P)
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x1E
#else
#define ADCIM_CAL_ADC_OFFSET_MISMATCH   0x0
#endif

static int g_adcim_cal_param = 0;

static inline void adcim_writel(u32 val, int reg)
{
    writel(val, ADCIM_BASE + reg);
}

static inline u32 adcim_readl(int reg)
{
    return readl(ADCIM_BASE + reg);
}

int hal_adcim_calibration_set(unsigned int val)
{
    int cal;

    if (val > 4095) {
        pr_err("The calibration value %d is too big\n", val);
        return -EINVAL;
    }

    cal = adcim_readl(ADCIM_CALCSR);
    cal = (cal & ~ADCIM_CALCSR_CALVAL_MASK)
        | (val << ADCIM_CALCSR_CALVAL_SHIFT);
    cal = cal | ADCIM_CALCSR_CALVAL_UPD;
    adcim_writel(cal, ADCIM_CALCSR);

    return 0;
}

/*
 * The calibration value is taken six times and the average value is obtained
 * after removing the maximum and minimum values, in order to ensure the
 * stability of calibration
 */
u32 hal_adcim_auto_calibration(void)
{
    u32 flag = 1;
    u32 data = 0;
    int max = 0;
    int min = 0;
    u32 cal_array[ADCIM_CALCSR_NUM] = {0};

    for (int i = 0; i < ADCIM_CALCSR_NUM; i++) {
#ifdef AIC_ADCIM_DELTA_ADC
        adcim_writel(0x08002f01, ADCIM_CALCSR);//auto cal
#else
        adcim_writel(0x08002f03, ADCIM_CALCSR);//auto cal
#endif
        if (readl_poll_timeout(ADCIM_BASE + ADCIM_CALCSR, flag,
                           (flag & 0x1) == 0, 1000) < 0) {
            pr_err("Failed to read ADCIM Calibration\n");
            return 0;
        }

        cal_array[i] = (adcim_readl(ADCIM_CALCSR) >> 16) & 0xfff;

        if (cal_array[i] > max)
            max = cal_array[i];

        if (i == 0) {
            min = cal_array[0];
        } else if (cal_array[i] < min) {
            min = cal_array[i];
        }

        data += cal_array[i];
        pr_debug("[%d]cal_data %d\n", i, cal_array[i]);
    }

    data = (data - min - max) / (ADCIM_CALCSR_NUM - 2);

    pr_debug("max %d min %d, latest_data %d\n", max, min, data);
    return data;
}

int hal_adcim_adc2voltage(u16 *val, u32 cal_data, int scale, float def_voltage)
{
    int cal_voltage;
    int st_voltage = 0;
    int cal_val = 0;

#ifdef AIC_SYSCFG_DRV
    st_voltage = hal_syscfg_read_ldo_cfg();
#endif
    if (!st_voltage) {
        pr_debug("Failed to get standard voltage\n");
        st_voltage = (int)(def_voltage * AIC_VOLTAGE_ACCURACY);
    }

    cal_val = *val + ADCIM_CAL_ADC_STANDARD_VAL - cal_data + ADCIM_CAL_ADC_OFFSET_MISMATCH;
    if (cal_val >= 0) {
        *val = cal_val;
        cal_voltage = *val * st_voltage / AIC_ADC_MAX_VAL;
    } else {
        pr_err("Out of the input voltage range - %d \n", cal_val);
        *val = 0;
        cal_voltage = 0;
    }

    return cal_voltage;
}

u32 hal_adcim_adc2caled(u32 adc_data)
{
    int caled_adc_val = 0;

    caled_adc_val = adc_data + ADCIM_CAL_ADC_STANDARD_VAL - g_adcim_cal_param + ADCIM_CAL_ADC_OFFSET_MISMATCH;

    if (caled_adc_val > ADCIM_ADC_ACC_RANGE)
        caled_adc_val = ADCIM_ADC_ACC_RANGE;
    if (caled_adc_val < 0)
        caled_adc_val = 0;
    pr_debug("Calibration param: %d\n", g_adcim_cal_param);
    return caled_adc_val;
}

u32 hal_adcim_version(void)
{
#ifdef AIC_ADCIM_DRV_V11
    return 0x101;
#else
    return adcim_readl(ADCIM_VERSION);
#endif
}

void adcim_status_show(void)
{
    int mcsr = adcim_readl(ADCIM_MCSR);
    int fifo = adcim_readl(ADCIM_FIFOSTS);

    printf("In ADCIM V%s:\n"
           "Busy state: %d\n"
           "Semflag: %d\n"
           "Current Channel: %d\n"
           "ADC Arbiter Idel: %d\n"
           "FIFO Error: %d\n"
           "FIFO Counter: %d\n",
           EXPAND_BCD_VER(hal_adcim_version()),
           (mcsr & ADCIM_MCSR_BUSY) ? 1 : 0,
           (mcsr & ADCIM_MCSR_SEMFLAG_MASK)
            >> ADCIM_MCSR_SEMFLAG_SHIFT,
           mcsr & ADCIM_MCSR_CHN_MASK,
           fifo & ADCIM_FIFOSTS_ADC_ARBITER_IDLE ? 1 : 0,
           fifo & ADCIM_FIFOSTS_FIFO_ERR ? 1 : 0,
           fifo & ADCIM_FIFOSTS_CTR_MASK);

#ifdef AIC_ADCIM_DM_DRV
    hal_adcdm_status_show();
#endif
}

void adcim_calibration_show(void)
{
    int cal;

    cal = adcim_readl(ADCIM_CALCSR);

    pr_info("Calibration Enable: %d\n Current value: %d\nADC ACQ: %d\n",
           (cal & ADCIM_CALCSR_CAL_ENABLE) ? 0 : 1,
           (cal & ADCIM_CALCSR_CALVAL_MASK) >> ADCIM_CALCSR_CALVAL_SHIFT,
           (cal & ADCIM_CALCSR_ADC_ACQ_MASK) >> ADCIM_CALCSR_ADC_ACQ_SHIFT);
}

#ifdef AIC_ADCIM_DELTA_ADC
void hal_adcim_set_cal_enable(void)
{
    int val;

    //Init delta ADC
    adcim_writel(0xf002a600, ADCIM_MCSR);
    val = adcim_readl(ADCIM_CALCSR);
    val = val | ADCIM_CALCSR_ADC_ACQ_MASK;
    val = val | ADCIM_CALCSR_ADC_CAL_SEL;

    adcim_writel(val, ADCIM_CALCSR);
}
#else
void hal_adcim_set_dcalmask(void)
{
    int val;

    val = adcim_readl(ADCIM_CALCSR);
    val = val | ADCIM_CALCSR_DCAL_MASK | ADCIM_CALCSR_ADC_ACQ_MASK;
    adcim_writel(val, ADCIM_CALCSR);
}
#endif

int hal_adcim_init(void)
{
    int ret = 0;

#ifndef AIC_ADCIM_DRV_V10
    ret = hal_clk_set_freq(CLK_ADCIM, ADCIM_DEFAULT_FREQ);
    if (ret < 0) {
        pr_err("Failed to set ADCIM clk\n");
        return -1;
    }
#endif

    ret = hal_clk_enable_deassertrst(CLK_ADCIM);
    if (ret < 0) {
        pr_err("ADCIM clock/reset enable failed!\n");
        return -1;
    }

#ifdef AIC_ADCIM_DELTA_ADC
    hal_adcim_set_cal_enable();
#else
    hal_adcim_set_dcalmask();
#endif

    g_adcim_cal_param = hal_adcim_auto_calibration();

    return ret;
}

int hal_adcim_deinit(void)
{
    int ret = 0;

    ret = hal_clk_disable_assertrst(CLK_ADCIM);
    if (ret < 0) {
        pr_err("ADCIM clock/reset disable failed!");
        return -1;
    }

    return ret;
}

s32 hal_adcim_probe(void)
{
    s32 ret = 0;
    static s32 inited = 0;

    if (inited) {
        pr_debug("ADCIM is already inited\n");
        return 0;
    }

    ret = hal_adcim_init();
    if (ret < 0) {
        pr_err("ADCIM init failed!\n");
        return -1;
    }

    inited = 1;
    return 0;
}
