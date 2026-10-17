/*
 * Copyright (c) 2020-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
 */

#include <stdint.h>
#include <sys/time.h>
#include <string.h>

#include "aic_soc.h"
#include "aic_common.h"
#include "aic_log.h"
#include "aic_hal_clk.h"
#include "aic_osal.h"
#include "aic_io.h"
#include "aic_hal_reset.h"
#include "aic_hal_ve.h"

#define VE_CLOCK   (200000000)

#define VE_SRAM_MAP 0x164

#define VE_IRQ_REG 0x0C
#define AVC_EN_REG 0x10
#define JPG_EN_REG 0x14
#define PNG_EN_REG 0x18

#ifdef AIC_VE_DRV_V10
#define JPG_STATUS_REG  (0x2004)
#define PNG_STATUS_REG  (0xC04)
#define AVC_STATUS_REG  (0x128)
#else
#define JPG_STATUS_REG  (0x204)
#define PNG_STATUS_REG  (0x104)
#endif

#define AVC_CLEAR_IRQ 0x70000
#define JPG_CLEAR_IRQ 0xf
#define PNG_CLEAR_IRQ 0xf

#define VE_ENABLE_IRQ 1
#define VE_DISABLE_IRQ 0

#define FINISH_EVENT 1

#define VE_TIMEOUT  2000 // 2s
#define VE_CLOSE_TIMEOUT_MS 50

struct aic_ve_client {
    u32 status;                     // finish status
    aicos_event_t wait_event;
    aicos_mutex_t lock;
    u32 is_busy;
};

struct aic_ve_client client = {0};

static int hal_ve_clk_enable(void)
{
    int ret;

    ret = hal_clk_set_freq(CLK_VE, VE_CLOCK);
    if (ret < 0) {
        hal_log_err("VE set clk failed");
        return -1;
    }

    ret = hal_clk_enable(CLK_VE);
    if(ret < 0) {
        hal_log_err("VE clk enable failed");
        return -1;
    }


    hal_reset_assert(RESET_VE);
    ret = hal_reset_deassert(RESET_VE);
    if(ret < 0) {
        hal_log_err("VE clk reset deassert failed");
        return -1;
    }

    return 0;
}

static int hal_ve_clk_disable(void)
{
    hal_reset_assert(RESET_VE);
    hal_clk_disable(CLK_VE);

    return 0;
}

static void hal_ve_power_on()
{
    hal_ve_clk_enable();
}

static void hal_ve_power_off()
{
    hal_ve_clk_disable();
}

static irqreturn_t hal_ve_handle(int irq, void* ctx)
{
    u32 status;
    // Careful, the ctx is unused now !!!
    //struct aic_ve_client *client = (struct aic_ve_client*)ctx;

#ifdef AIC_VE_DRV_V10
    // h264
    if (readl(VE_BASE + AVC_EN_REG)) {
        status = readl(VE_BASE + AVC_STATUS_REG);
        client.status = status;
        writel(status | AVC_CLEAR_IRQ, VE_BASE + AVC_STATUS_REG);
    }
#endif
    // jpeg
    if (readl(VE_BASE + JPG_EN_REG)) {
        status = readl(VE_BASE + JPG_STATUS_REG);
        client.status = status;
        writel(status | JPG_CLEAR_IRQ, VE_BASE + JPG_STATUS_REG);
    }

    // png
    if (readl(VE_BASE + PNG_EN_REG)) {
        status = readl(VE_BASE + PNG_STATUS_REG);
        client.status = status;

        status |= PNG_CLEAR_IRQ;
        writel(status, VE_BASE + PNG_STATUS_REG);
    }

    writel(VE_DISABLE_IRQ, VE_BASE + VE_IRQ_REG);

    client.is_busy = 0;
    aicos_event_send(client.wait_event, FINISH_EVENT);
    return 0;
}

int hal_ve_probe(void)
{
    memset(&client, 0, sizeof(struct aic_ve_client));
    client.lock = aicos_mutex_create();
    if (client.lock == NULL) {
        hal_log_err("Create mutex failed");
        return -1;
    }
    client.wait_event = aicos_event_create();
    if (client.wait_event == NULL) {
        aicos_mutex_delete(client.lock);
        hal_log_err("Create wait_event failed");
        return -1;
    }
    aicos_request_irq(VE_IRQn, hal_ve_handle, 0, NULL, (void*)&client);
    aicos_irq_enable(VE_IRQn);
    pr_debug("++++ aich_ve_probe, client: %p", &client);

#if defined(AIC_VE_DRV_V40) || defined(AIC_VE_DRV_V31)
    // switch system sram to VE
    writel(1 | (0xa1c<<20), SYSCFG_BASE + VE_SRAM_MAP);
#endif
    return 0;
}

struct aic_ve_client *hal_ve_open(void)
{
    hal_ve_power_on();
    return &client;
}

int hal_ve_close(struct aic_ve_client *client)
{
    hal_ve_power_off();
    return 0;
}

int hal_ve_close_with_wait(void)
{
    int ret = 0;

    if(client.is_busy && aicos_event_recv(client.wait_event, FINISH_EVENT, NULL, VE_CLOSE_TIMEOUT_MS) < 0) {
        hal_log_err("VE wait irq failed");
        ret = -1;
    }

    hal_ve_power_off();
    return ret;
}

int hal_ve_control(struct aic_ve_client *client, int cmd, void *arg)
{
    switch(cmd) {
    case IOC_VE_WAIT: {
        struct wait_info *info = (struct wait_info*)arg;
        uint32_t recved;

        if(aicos_event_recv(client->wait_event, FINISH_EVENT, &recved, info->wait_time) < 0) {
            hal_log_info("VE wait irq failed");
            return -1;
        }

        info->reg_status = client->status;
        break;
    }
    case IOC_VE_GET_CLIENT:
        if(aicos_mutex_take(client->lock, VE_TIMEOUT) < 0) {
            hal_log_info("VE get client failed");
            return -1;
        }
        client->is_busy = 1;
        break;
    case IOC_VE_PUT_CLIENT:
        aicos_mutex_give(client->lock);
        break;
    case IOC_VE_RESET:
        hal_reset_assert(RESET_VE);
        hal_reset_deassert(RESET_VE);
        break;
    default:
        hal_log_err("unkown ve cmd: %d", cmd);
        return -1;
    }

    return 0;
}
