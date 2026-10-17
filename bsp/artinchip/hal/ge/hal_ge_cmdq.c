/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: ardon <haidong.pan@artinchip.com>
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "aic_core.h"
#include "aic_hal.h"
#include "aic_log.h"
#include "mpp_types.h"
#include "aic_drv_ge.h"

#include <sys/time.h>

#include "hal_ge_hw.h"
#include "hal_ge_reg.h"

#define AIC_GE_NAME "aic-ge"

#define MAX_WIDTH 4096
#define MAX_HEIGHT 4096

#define GE_TIMEOUT  4 * 1000

#define MAX_BATCH_NUM 8
/* Users can dynamically configure the memory size of cmdq buffer in Kconfig */
#define CMD_BUF_SIZE  AIC_GE_CMDQ_BUF_LENGTH

#define END_ADDR(start, size) ((start) + (size) - 1)
#define ALIGN_128B(x) ALIGN_UP(x, 128)
#define ALIGN_CACHE(x) ALIGN_UP(x, CACHE_LINE_SIZE)

#define BATCH_EVENT         0x01
#define HW_RUNNING_EVENT    0x02
#define BATCH_NUM_EVENT     0x04

#ifdef AIC_GE_DRV_V11
#define GE_CLOCK   (150000000)
#else
#define GE_CLOCK   (200000000)
#endif

#define WARN_ON(condition)  ({                                      \
    int __ret_warn_on = !!(condition);                              \
    unlikely(__ret_warn_on);                                        \
})

typedef unsigned int    atomic_t;

struct aic_ge_batch {
    struct list_head list;
    int offset;
    int length;
    int client_id;
};

struct aic_ge_data {
    struct device        *dev;
    struct reset_control *reset;
    struct clk           *mclk;
    int                  irq;
    aicos_mutex_t        lock;
    unsigned long        h_lock;
    aicos_event_t        wait_event;
    u32                  status;
    atomic_t             cur_id;
    bool                 hw_running;
    bool                 batch_full;
    struct list_head     free;
    struct list_head     ready;
    struct aic_ge_batch  *cur_batch;
    u8                   *cmd_ptr;
    uintptr_t            cmd_phys;
    u8                   *base_ptr;
    uintptr_t            base_phys;
    u8                   *dither_line_ptr;
    uintptr_t            dither_line_phys;
    u32                  total_size;
    int                  empty_size;
    int                  write_offset;
    struct list_head     client_list;
    enum ge_mode         ge_mode;
};

static struct aic_ge_data *g_data;
static int ge_clk_enable(struct aic_ge_data *data);

#ifdef CTRL_GE_CLK_IN_FRAME
static int ge_clk_disable(struct aic_ge_data *data);
#endif

atomic_t atomic_inc(atomic_t *target)
{
    atomic_t ret;

    ret = *target;
    (*target)++;

    return ret;
}

static int ge_clk_enable(struct aic_ge_data *data)
{
    int ret;

    hal_reset_assert(RESET_GE);
    ret = hal_reset_deassert(RESET_GE);
    if (ret) {
        hal_log_err("Couldn't deassert\n");
        return ret;
    }

    ret = hal_clk_set_freq(CLK_GE, GE_CLOCK);
    if (ret < 0) {
        hal_log_err("GE set clk failed\n");
        return -1;
    }

    ret = hal_clk_enable(CLK_GE);
    if (ret < 0) {
        hal_log_err("GE bus clk enable failed!\n");
        return -1;
    }

    return 0;
}

#ifndef CTRL_GE_CLK_IN_FRAME
static void ge_power_on(struct aic_ge_data *data)
{
    aicos_mutex_take(data->lock, AICOS_WAIT_FOREVER);
    ge_clk_enable(data);
    aicos_mutex_give(data->lock);
}
#else
static int ge_clk_disable(struct aic_ge_data *data)
{
    hal_clk_disable(CLK_GE);
    hal_reset_assert(RESET_GE);
    return 0;
}
#endif

static int ge_wait_empty_buffer(struct aic_ge_data *data, int req_size)
{
    int ret = 0;
    uint32_t recved;

    hal_log_debug("%s\n", __func__);

    while (req_size > data->empty_size ||
            data->batch_full) {
        ret = aicos_event_recv(data->wait_event,
                                BATCH_EVENT,
                                &recved,
                                GE_TIMEOUT);
        if (ret < 0)
            break;
    }

    return ret;
}

static int ge_wait_idle(struct aic_ge_data *data)
{
    int ret = 0;
    uint32_t recved;

    hal_log_debug("%s\n", __func__);

    while (data->hw_running) {
        ret = aicos_event_recv(data->wait_event,
                                HW_RUNNING_EVENT,
                                &recved,
                                GE_TIMEOUT);
        if (ret < 0)
            break;
    }

    hal_log_debug("%s\n", __func__);

    return ret;
}

static int ge_client_sync(struct aic_ge_data *data,
                struct aic_ge_client *client)
{
    int ret = 0;
    uint32_t recved;

    hal_log_debug("%s\n", __func__);

    while (client->batch_num) {
        ret = aicos_event_recv(data->wait_event,
                                BATCH_NUM_EVENT,
                                &recved,
                                GE_TIMEOUT);
        if (ret < 0) {
            break;
        }
    }

    hal_log_debug("%s\n", __func__);

    return ret;
}

static void run_hw(struct aic_ge_data *data)
{
    struct aic_ge_batch *batch;

    hal_log_debug("%s\n", __func__);

    WARN_ON(list_empty(&data->ready));
    WARN_ON(data->cur_batch);

    /* dequeue batch from ready list */
    batch = list_first_entry(&data->ready,
                  struct aic_ge_batch, list);

    list_del(&batch->list);

#ifdef CTRL_GE_CLK_IN_FRAME
    ge_clk_enable(data);
#endif

    /* config cmd queue ring buffer */
    writel(data->cmd_phys, GE_BASE + CMD_BUF_START_ADDR);
    writel(END_ADDR(data->cmd_phys, data->total_size),
           GE_BASE + CMD_BUF_END_ADDR);
    writel(batch->offset, GE_BASE + CMD_BUF_ADDR_OFFSET);
    writel(batch->length, GE_BASE + CMD_BUF_VALID_LENGTH);

    /* enable interrupt*/
    writel(GE_CTRL_FINISH_IRQ_EN | GE_CTRL_HW_ERR_IRQ_EN, GE_BASE + GE_CTRL);
#ifdef AIC_GE_DITHER
    writel(data->dither_line_phys, GE_BASE + DITHER_BGN_ADDR);
#endif
    /* run cmd queue */
    writel(GE_SW_RESET | GE_CMD_EN | GE_START_EN, GE_BASE + GE_START);
    data->hw_running = true;
    data->cur_batch = batch;
}

static int ge_free_batch_buffer(struct aic_ge_data *data)
{
    struct aic_ge_batch *batch;

    batch = list_first_entry(&data->free,
                 struct aic_ge_batch, list);
    list_del(&batch->list);
    aicos_free(GE_DEFAULT, batch);

    return 0;
}

static int ge_alloc_batch_buffer(struct aic_ge_data *data)
{
    struct aic_ge_batch *batch;

    batch = aicos_malloc(GE_DEFAULT, sizeof(struct aic_ge_batch));
    if(!batch)
        return -1;
    memset(batch, 0, sizeof(struct aic_ge_batch));
    list_add_tail(&batch->list, &data->free);

    return 0;
}

static void ge_reset(struct aic_ge_data *data)
{
    hal_reset_assert(RESET_GE);
    hal_reset_deassert(RESET_GE);
}

static irqreturn_t aic_ge_handler(int flag, void *ctx)
{
    u32 status;
    struct aic_ge_client *cur_client, *node;
    struct aic_ge_data *data = g_data;

    /* read interrupt status */
    status = readl(GE_BASE + GE_STATUS);

    /* clear interrupt status */
    writel(status, GE_BASE + GE_STATUS);

    /* disable interrupt*/
    writel(0, GE_BASE + GE_CTRL);

    WARN_ON(!status);

    if (status & GE_CTRL_HW_ERR_IRQ_EN) {
        hal_log_err("ge error status:%08x\n", status);
        ge_reset(data);
    }

    data->empty_size += ALIGN_CACHE(data->cur_batch->length);

    list_add_tail(&data->cur_batch->list, &data->free);
    data->batch_full = false;
    aicos_event_send(data->wait_event, BATCH_EVENT);

    list_for_each_entry_safe(cur_client, node, &data->client_list, list) {
         if (cur_client->id == data->cur_batch->client_id) {
            cur_client->batch_num--;
            if (cur_client->batch_num == 0) {
                aicos_event_send(data->wait_event, BATCH_NUM_EVENT);
            }
            break;
         }
    }

    data->cur_batch = NULL;

#ifdef CTRL_GE_CLK_IN_FRAME
    ge_clk_disable(data);
#endif

    if (!list_empty(&data->ready)) {
        run_hw(data);
    } else {
        /*idle*/
        data->hw_running = false;
        aicos_event_send(data->wait_event, HW_RUNNING_EVENT);
    }

    return 0;
}

int hal_ge_init(void)
{
    struct aic_ge_data *data;
    int i;
    s32 ret = 0;

    hal_log_info("cmd queue hal, cmdq buffer size = %d\n", CMD_BUF_SIZE);
    data = aicos_malloc(GE_DEFAULT, sizeof(struct aic_ge_data));
    if(!data)
        return -1;

    memset(data, 0, sizeof(struct aic_ge_data));

    data->wait_event = aicos_event_create();
    aicos_request_irq(GE_IRQn, aic_ge_handler, 0, NULL, (void *)data);

    INIT_LIST_HEAD(&data->free);
    INIT_LIST_HEAD(&data->ready);
    INIT_LIST_HEAD(&data->client_list);

    for (i = 0; i < MAX_BATCH_NUM; i++) {
        ret = ge_alloc_batch_buffer(data);
        if(ret < 0) {
            hal_log_err("failed to alloc batch buffer: %d\n", i);
            return ret;
        }
    }

    data->base_ptr = (u8 *)aicos_malloc(GE_CMA, CMD_BUF_SIZE);
    if (data->base_ptr == NULL) {
        hal_log_err("aicos_malloc data->base_ptr fail\n");
        return -1;
    }

    memset(data->base_ptr, 0, ALIGN_128B(CMD_BUF_SIZE));
    data->base_phys = (uintptr_t)data->base_ptr;

    if (!data->base_ptr)
        return -1;

    /* 128 bytes aligned */
    if (data->base_phys & 127) {
        data->cmd_ptr = (u8 *)ALIGN_128B((unsigned long)data->base_ptr);
        data->cmd_phys = ALIGN_128B(data->base_phys);
        data->total_size = ALIGN_128B(CMD_BUF_SIZE) - 128;
        data->empty_size = data->total_size;
    } else {
        data->cmd_ptr = data->base_ptr;
        data->cmd_phys = data->base_phys;
        data->total_size = ALIGN_128B(CMD_BUF_SIZE);
        data->empty_size = data->total_size;
    }
#ifdef AIC_GE_DITHER
    int dither_line_len = (MAX_WIDTH * 4) + 128 + CACHE_LINE_SIZE;
    data->dither_line_ptr = (u8 *)aicos_malloc(GE_CMA,  dither_line_len);
    if (!data->dither_line_ptr) {
        hal_log_err("failed to malloc dither line buffer\n");
        return -1;
    }
    data->dither_line_phys = ALIGN_128B((uintptr_t)data->dither_line_ptr);

    int dither_line_size_align = ALIGN_UP((MAX_WIDTH * 4), CACHE_LINE_SIZE);
    aicos_dcache_clean_invalid_range((unsigned long *)(data->dither_line_phys), dither_line_size_align);
#endif
    data->ge_mode = GE_MODE_CMDQ;
    data->lock = aicos_mutex_create();
    g_data = data;

#ifndef CTRL_GE_CLK_IN_FRAME
    ge_power_on(data);
#endif

    hal_log_debug("%s() end\n", __func__);

    return 0;
}

struct aic_ge_client *hal_ge_open(void)
{
    struct aic_ge_client *client = NULL;
    struct aic_ge_data *data = g_data;

    client = aicos_malloc(GE_DEFAULT, sizeof(struct aic_ge_client));
    if (!client)
        return NULL;

    memset(client, 0, sizeof(struct aic_ge_client));

    INIT_LIST_HEAD(&client->buf_list);

    client->id = atomic_inc(&data->cur_id);

    aicos_local_irq_save(&data->h_lock);
    list_add_tail(&client->list, &data->client_list);
    aicos_local_irq_restore(data->h_lock);
    return client;
}

int hal_ge_close(struct aic_ge_client *clt)
{
    struct aic_ge_data *data = g_data;
    struct aic_ge_client *client = clt;

    if (!client)
        return -1;

    aicos_local_irq_save(&data->h_lock);
    list_del(&client->list);
    aicos_local_irq_restore(data->h_lock);
    aicos_free(GE_DEFAULT, client);

    return 0;
}

int hal_ge_write(struct aic_ge_client *clt, const char *buff, size_t count)
{
    int write_offset;
    int aligned_count;
    struct aic_ge_client *client = clt;
    struct aic_ge_data *data = g_data;
    struct aic_ge_batch *batch;

    aligned_count = ALIGN_CACHE(count);
    if (aligned_count > (int)data->total_size)
        return -1;

    aicos_mutex_take(data->lock, AICOS_WAIT_FOREVER);

    /* wait for enough buffer */
    if (ge_wait_empty_buffer(data, aligned_count) < 0) {
        hal_log_err("failed to get enough buffer\n");
        return -1;
    }

    aicos_local_irq_save(&data->h_lock);
    /* dequeue free batch */
    batch = list_first_entry(&data->free,
                 struct aic_ge_batch, list);
    list_del(&batch->list);

    if (list_empty(&data->free))
         data->batch_full = true;

    write_offset = data->write_offset;
    data->write_offset += aligned_count;
    data->empty_size -= aligned_count;

    if (write_offset + aligned_count >= data->total_size)
         data->write_offset -= data->total_size;

    batch->client_id = client->id;
    batch->length = count;
    batch->offset = write_offset;
    client->batch_num++;
    aicos_local_irq_restore(data->h_lock);

    /* copy cmd from user space to ring buffer */
    if (write_offset + aligned_count <= data->total_size) {
        memcpy(data->cmd_ptr + write_offset, buff, count);
        aicos_dcache_clean_invalid_range((unsigned long *)(data->cmd_ptr + write_offset),
                                         ALIGN_CACHE(count));
    } else {
        int len0;
        int len1;

        len1 = write_offset + count - data->total_size;
        len0 = count - len1;

        memcpy(data->cmd_ptr + write_offset, buff, len0);
        memcpy(data->cmd_ptr, buff + len0, len1);
        aicos_dcache_clean_invalid_range((unsigned long *)(data->cmd_ptr + write_offset),
                                         ALIGN_CACHE(len0));
        aicos_dcache_clean_invalid_range((unsigned long *)data->cmd_ptr,
                                         ALIGN_CACHE(len1));
    }

    /* enqueue ready batch */
    aicos_local_irq_save(&data->h_lock);
    list_add_tail(&batch->list, &data->ready);

    if (!data->hw_running) {
        run_hw(data);
    }
    aicos_local_irq_restore(data->h_lock);
    aicos_mutex_give(data->lock);

    return 0;
}

int hal_ge_control(struct aic_ge_client *clt, int cmd, void *arg)
{
    int ret = 0;
    u32 version;
    struct aic_ge_data *data = g_data;
    struct aic_ge_client *client = clt;

    switch (cmd) {
    case IOC_GE_VERSION:
        version = readl(GE_BASE + GE_VERSION_ID);
        memcpy(arg, &version, sizeof(u32));
        break;
    case IOC_GE_MODE:
        memcpy(arg, &data->ge_mode, sizeof(u32));
        break;
    case IOC_GE_SYNC:
        ret = ge_client_sync(data, client);
        break;
    case IOC_GE_CMD_BUF_SIZE:
        memcpy(arg, &data->total_size, sizeof(u32));
        break;
    default:
        hal_log_warn("Invailid ioctl: %08x\n", cmd);
        ret = -1;
        break;
    }

    return ret;
}

int hal_ge_deinit(void)
{
    hal_log_debug("%s\n", __func__);

    struct aic_ge_data *data = g_data;
    int i = 0, ret = 0;
    ge_wait_idle(data);
    aicos_mutex_delete(data->lock);
    aicos_event_delete(data->wait_event);
    aicos_free(GE_CMA, data->base_ptr);
    for (i = 0; i < MAX_BATCH_NUM; i++) {
        ret = ge_free_batch_buffer(data);
        if (ret < 0) {
            hal_log_err("failed to free batch buffer: %d\n", i);
            return ret;
        }
    }
    aicos_free(GE_DEFAULT, data);
#ifdef AIC_GE_DITHER
    aicos_free(GE_CMA, data->dither_line_ptr);
#endif
    return 0;
}
