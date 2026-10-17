/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <sys/time.h>
#include <string.h>

#include "drivers/sensor.h"

#define LOG_TAG             "TSEN"
#include "aic_core.h"
#include "hal_tsen.h"
#include "aic_hal_clk.h"

struct aic_tsen_dev {
    struct rt_sensor_device dev;
    u32 pclk_rate;
    struct aic_tsen_ch *ch;
};

// TODO: slope & shift should be read from eFuse, which was writen by CP tester
struct aic_tsen_ch aic_tsen_chs[] = {
#ifdef AIC_USING_TSEN_CPU
    {
        .id = 0,
        .available = 1,
        .name = "tsen_cpu",
        .mode = AIC_TSEN0_MODE,
        .soft_mode = AIC_TSEN0_SOFT_MODE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef FPGA_BOARD_ARTINCHIP
        .slope  = -1132,
        .offset = 2439001,
#endif
    },
#endif
#ifdef AIC_USING_TSEN_GPAI
    {
        .id = 1,
        .available = 1,
        .name = "tsen_gpai",
        .mode = AIC_TSEN1_MODE,
        .soft_mode = AIC_TSEN1_SOFT_MODE,
        .hta_enable = 0,
        .lta_enable = 0,
        .otp_enable = 0,
#ifndef FPGA_BOARD_ARTINCHIP
        .slope  = -1159,
        .offset = 2450566,
#endif
    }
#endif
};

static struct aic_tsen_dev g_aic_tsen[ARRAY_SIZE(aic_tsen_chs)];

static rt_size_t aic_tsen_fetch(struct rt_sensor_device *sensor,
                                void *buf, rt_size_t len)
{
    struct aic_tsen_ch *chan = &aic_tsen_chs[sensor->parent.device_id];
    struct rt_sensor_data *data = (struct rt_sensor_data *)buf;

    if (!chan)
        return -RT_EINVAL;

    if (!chan->available) {
        pr_warn("%s is unavailable!\n", chan->name);
        return -ENODEV;
    }
#ifdef AIC_PM_DRV_V15
    rt_pm_module_request(PM_SENSOR_ID, PM_SLEEP_MODE_NONE);
#endif

    pr_debug("The ch%d is obtaining data\n", chan->id);
#ifdef AIC_SID_DRV
    hal_tsen_curve_fitting(chan);
#else
    pr_debug("The eFuse is not enabled\n");
#endif
    data->type = RT_SENSOR_CLASS_TEMP;
    data->timestamp = rt_sensor_get_ts();
    if (chan->mode == AIC_TSEN_MODE_SINGLE)
        hal_tsen_get_temp(chan, (s32 *)&data->data.temp);
    else
        data->data.temp = hal_tsen_data2temp(chan);

#ifdef AIC_PM_DRV_V15
    rt_pm_module_release(PM_SENSOR_ID, PM_SLEEP_MODE_NONE);
#endif
    return 1;
}

static rt_err_t aic_tsen_control(struct rt_sensor_device *sensor,
                                 int cmd, void *args)
{
    struct aic_tsen_ch *data = (struct aic_tsen_ch *)args;
    rt_uint8_t device_id = sensor->parent.device_id;
    rt_uint32_t power_stat = 0;

    switch (cmd) {
        case RT_SENSOR_CTRL_GET_CH_INFO:
            *data = aic_tsen_chs[sensor->parent.device_id];
            break;
        case RT_SENSOR_CTRL_SET_POWER:
            data = &aic_tsen_chs[device_id];
            power_stat = (rt_uint32_t)(ptr_t)args;

            if (power_stat == RT_SENSOR_POWER_NORMAL) {
                hal_tsen_ch_init(data, data->pclk_rate);
            } else if (power_stat == RT_SENSOR_POWER_DOWN) {
                hal_tsen_ch_enable(data->id, 0);
            } else {
                LOG_D("Unsupported power state: 0x%x", power_stat);
                return -RT_ERROR;
            }
            break;
        default:
            LOG_D("Unsupported cmd: 0x%x", cmd);
            return -RT_ERROR;
            break;
    }
    return RT_EOK;
}

#ifdef RT_USING_PM
static int aic_tsen_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_clk_disable_assertrst(CLK_TSEN);
#else
        hal_clk_disable(CLK_TSEN);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void aic_tsen_resume(const struct rt_device *device, rt_uint8_t mode)
{
#ifdef AIC_PM_DRV_V15
    struct aic_tsen_ch *chan = NULL;
#endif

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        chan = &aic_tsen_chs[device->device_id];

        hal_tsen_clk_init();
        hal_tsen_enable(1);
        hal_tsen_ch_init(chan, chan->pclk_rate);
#else
        hal_clk_enable(CLK_TSEN);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_tsen_pm_ops =
{
    SET_DEVICE_PM_OPS(aic_tsen_suspend, aic_tsen_resume)
    NULL,
};
#endif

static struct rt_sensor_ops aic_sensor_ops = {
    .fetch_data = aic_tsen_fetch,
    .control = aic_tsen_control
};

static int drv_tsen_init(void)
{
    s32 ret = 0;
    struct rt_sensor_info *info;

    hal_tsen_clk_init();
    aicos_request_irq(TSEN_IRQn, hal_tsen_irq_handle, 0, NULL, NULL);
    hal_tsen_enable(1);
    hal_tsen_set_ch_num(ARRAY_SIZE(aic_tsen_chs));

    for (int i = 0; i < ARRAY_SIZE(aic_tsen_chs); i++) {
        hal_tsen_pclk_get(&aic_tsen_chs[i]);

        aic_tsen_chs[i].complete = aicos_sem_create(0);

        g_aic_tsen[i].ch = &aic_tsen_chs[i];
        info = &g_aic_tsen[i].dev.info;
        info->type       = RT_SENSOR_CLASS_TEMP;
        info->model      = "aic-tsen";
        info->unit       = RT_SENSOR_UNIT_DCELSIUS;
        info->range_max  = 125;
        info->range_min  = 0;
        info->period_min = 1000; // 1000ms
        g_aic_tsen[i].dev.parent.device_id = i;
        g_aic_tsen[i].dev.ops = &aic_sensor_ops;
        ret = rt_hw_sensor_register(&g_aic_tsen[i].dev, aic_tsen_chs[i].name,
                                    RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (ret != RT_EOK) {
            LOG_E("Failed to register %s. ret %d", aic_tsen_chs[i].name, ret);
            return -RT_ERROR;
        }
#ifdef RT_USING_PM
        rt_pm_device_register(&g_aic_tsen[i].dev.parent, &aic_tsen_pm_ops);
#endif
    }

    return 0;
}
INIT_DEVICE_EXPORT(drv_tsen_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_tsen_status(int argc, char **argv)
{
    for (int i = 0; i < ARRAY_SIZE(aic_tsen_chs); i++) {
        hal_tsen_status_show(&aic_tsen_chs[i]);
    }
}

MSH_CMD_EXPORT_ALIAS(cmd_tsen_status, tsen_status, Show the status of TSensor);

#endif
