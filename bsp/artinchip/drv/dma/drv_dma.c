/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "aic_core.h"
#include "aic_hal_clk.h"
#include "aic_hal_reset.h"

#include "hal_dma.h"
#include "drv_dma.h"

#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#include <rtdevice.h>
#endif

#if defined(AIC_USING_DMA0)
#define CLK_DMA CLK_DMA0
#elif defined(AIC_USING_DMA1)
#define CLK_DMA CLK_DMA1
#elif defined(AIC_USING_DMA2)
#define CLK_DMA CLK_DMA2
#endif

void drv_dma_deinit(void)
{
    hal_reset_assert(RESET_DMA);
    hal_clk_disable(CLK_DMA);

#ifdef KERNEL_RTTHREAD
    struct rt_device *device;

    device = rt_device_find("aic_dma");
    if (device) {
        rt_device_unregister(device);
        rt_free(device);
    }
#endif
}

int drv_dma_init(void)
{
    s32 ret = 0;

    if (hal_clk_is_enabled(CLK_DMA))
        drv_dma_deinit();

#ifdef KERNEL_RTTHREAD
    struct rt_device *device = NULL;

    device = (struct rt_device *)rt_malloc(sizeof(struct rt_device));
    if (!device) {
        pr_err("Failed to alloc memory for device\n");
        return -1;
    }
    rt_memset(device, 0, sizeof(struct rt_device));
    ret = rt_device_register(device, "aic_dma", RT_DEVICE_FLAG_DEACTIVATE);
    if (ret != RT_EOK) {
        pr_err("Failed to register device\n");
        goto err;
    }
#endif

    ret = hal_clk_enable(CLK_DMA);
    if (ret < 0) {
        pr_err("DMA BUS clk enable failed!");
        goto err;
    }

    ret = hal_reset_deassert(RESET_DMA);
    if (ret < 0) {
        pr_err("DMA reset deassert failed!");
        goto err;
    }

    hal_dma_init();

#if defined(AIC_DMA_DRV_V21)
    aicos_request_irq(R_DMA_IRQn, hal_dma_irq, 0, NULL, NULL);
#else
    aicos_request_irq(DMA_IRQn, hal_dma_irq, 0, NULL, NULL);
#endif

    pr_info("ArtInChip DMA loaded\n");
    return 0;

err:
#ifdef KERNEL_RTTHREAD
    if (device) {
        rt_device_unregister(device);
        rt_free(device);
    }
#endif
    return -1;
}

#ifdef KERNEL_RTTHREAD
INIT_BOARD_EXPORT(drv_dma_init);
#endif

#ifdef AIC_USING_PM
static int aic_dma_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_STANDBY:
        case PM_SLEEP_MODE_DEEP:
            /* Consumers have already suspended (DMA is late
             * suspend), so any in-flight transfer should be the
             * last one from a consumer. Wait for it to complete.
             * On timeout, block sleep - the system will retry
             * later. */
            if (hal_wait_dma_chans_finish()) {
                pr_warn("DMA busy, block light sleep\n");
                return -EBUSY;
            }
#ifdef AIC_PM_DRV_V15
            hal_reset_assert(RESET_DMA);
            hal_clk_disable(CLK_DMA);
#else
            hal_clk_disable(CLK_DMA);
#endif
            break;
        default:
            break;
    }
    return 0;
}

static void aic_dma_resume(const struct rt_device *device, rt_uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_IDLE:
            break;
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_STANDBY:
        case PM_SLEEP_MODE_DEEP:
#ifdef AIC_PM_DRV_V15
            /* Full hardware reinit required.
             * DMA is early resume, so this runs before consumer
             * drivers resume - they will re-setup DMA transfers
             * during their own resume. */
            hal_clk_enable(CLK_DMA);
            hal_reset_deassert(RESET_DMA);
            hal_dma_init();
#else
            hal_clk_enable(CLK_DMA);
#endif
            break;
        default:
            break;
    }
}

static struct rt_device_pm_ops aic_dma_pm_ops = {
    SET_DEVICE_PM_OPS(NULL, NULL)
    SET_LATE_DEVICE_PM_OPS(aic_dma_suspend, aic_dma_resume)
    NULL
};

int drv_dma_pm_init(void)
{
    struct rt_device *device;

    device = rt_device_find("aic_dma");
    if (device) {
        rt_pm_device_register(device, &aic_dma_pm_ops);
    } else {
        pr_warn("Failed to get dma device\n");
        return -RT_ERROR;
    }
    return 0;
}

INIT_DEVICE_EXPORT(drv_dma_pm_init);
#endif

long drv_dma_ch_alloc(struct dma_chan *dma_ch, int8_t ch_id, int8_t ctrl_id)
{
    struct aic_dma_chan *chan;

    if (dma_ch == NULL)
        return -1;

    chan = hal_request_dma_chan();

    if (chan == NULL)
        return -1;

    *(struct aic_dma_chan **)dma_ch = chan;

    return 0;
}

void drv_dma_ch_free(struct dma_chan *dma_ch)
{
    struct aic_dma_chan *chan;

    if (dma_ch == NULL)
        return;
    chan = *(struct aic_dma_chan **)dma_ch;

    if (chan)
        hal_release_dma_chan(chan);
}

long drv_dma_ch_config(struct dma_chan *dma_ch, struct dma_slave_config *config)
{
    struct aic_dma_chan *chan;

    if (dma_ch == NULL)
        return -1;
    chan = *(struct aic_dma_chan **)dma_ch;

    if (chan == NULL)
        return -1;

    return hal_dma_chan_config(chan, config);
}

long drv_dma_ch_attach_callback(struct dma_chan *dma_ch, void *callback, void *arg)
{
    struct aic_dma_chan *chan;
    int ret;

    if (dma_ch == NULL)
        return -1;
    chan = *(struct aic_dma_chan **)dma_ch;
    if (chan == NULL)
        return -1;

    ret = hal_dma_chan_register_cb(chan, callback, arg);

    return ret;
}

void drv_dma_ch_detach_callback(struct dma_chan *dma_ch)
{
    struct aic_dma_chan *chan;

    if (dma_ch == NULL)
        return;
    chan = *(struct aic_dma_chan **)dma_ch;

    if (chan == NULL)
        return;

    chan->callback = NULL;
    chan->callback_param = NULL;
}

void drv_dma_ch_start(struct dma_chan *dma_ch, void *srcaddr, void *dstaddr, u32 length)
{
    struct aic_dma_chan *chan;
    int ret;

    if (dma_ch == NULL)
        return;

    chan = *(struct aic_dma_chan **)dma_ch;
    if (chan == NULL)
        return;

    if (DMA_MEM_TO_MEM == chan->cfg.direction) {
        ret =
            hal_dma_chan_prep_memcpy(chan, (unsigned long)dstaddr, (unsigned long)srcaddr, length);
    } else {
        ret = hal_dma_chan_prep_device(chan, (unsigned long)dstaddr, (unsigned long)srcaddr, length,
                                       chan->cfg.direction);
    }
    if (ret)
        return;

    hal_dma_chan_start(chan);
}

void drv_dma_ch_stop(struct dma_chan *dma_ch)
{
    struct aic_dma_chan *chan;

    if (dma_ch == NULL)
        return;

    chan = *(struct aic_dma_chan **)dma_ch;
    if (chan == NULL)
        return;

    hal_dma_chan_stop(chan);
}

#if defined(RT_USING_FINSH)
#include <finsh.h>

static void cmd_dma_dump(int argc, char **argv)
{
    u32 ch_nr = 0;

    if ((argc != 2) || !aic_isdigit(argv[1][0])) {
        pr_err("Invalid parameter\n");
        return;
    }

    ch_nr = atoi(argv[1]);
    if (ch_nr >= AIC_DMA_CH_NUM) {
        pr_err("Invalid channel No.%d\n", ch_nr);
        return;
    }

    hal_dma_chan_dump(ch_nr);
}
MSH_CMD_EXPORT_ALIAS(cmd_dma_dump, dma_dump, Dump DMA register.Argument : channel_num);

#endif
