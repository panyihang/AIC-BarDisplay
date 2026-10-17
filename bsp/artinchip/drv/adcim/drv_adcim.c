/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aic_core.h"
#include "hal_adcim.h"
#include "aic_hal_clk.h"

static struct rt_device g_adcim_dev = {0};
int drv_adcim_init(void)
{
    if (hal_adcim_probe())
        return -RT_ERROR;

    if (rt_device_register(&g_adcim_dev, "adcim", RT_DEVICE_FLAG_RDONLY))
        return -RT_ERROR;
    return RT_EOK;
}
INIT_BOARD_EXPORT(drv_adcim_init);

#ifdef RT_USING_PM
static int drv_adcim_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_adcim_deinit();
#else
        hal_clk_disable(CLK_ADCIM);
#endif
        break;
    default:
        break;
    }

    return 0;
}

static void drv_adcim_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
#ifdef AIC_PM_DRV_V15
        hal_adcim_init();
#else
        hal_clk_enable(CLK_ADCIM);
#endif
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops g_adcim_pm_ops =
{
    SET_DEVICE_PM_OPS(drv_adcim_suspend, drv_adcim_resume)
    NULL,
};

static int drv_adcim_pm_init(void)
{
    rt_pm_device_register(&g_adcim_dev, &g_adcim_pm_ops);
    return RT_EOK;
}
INIT_DEVICE_EXPORT(drv_adcim_pm_init);
#endif
