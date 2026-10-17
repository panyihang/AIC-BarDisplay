/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <zrq@artinchip.com>
 */

#include <rtdevice.h>
#include <drivers/sdio_mgr.h>

#define DBG_TAG "sdio_mgr"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static struct rt_sdio_mgr_device sdio_mgr_dev;

static const rt_uint32_t sdio_mgr_event_map[DEVICE_TYPE_MAX] = {
    SDIO_EVENT_SD_CARD_READY,
    SDIO_EVENT_MMC_CARD_READY,
    SDIO_EVENT_SDIO_DEVICE_READY
};

static rt_bool_t check_events_is_ready(rt_uint32_t wait_events, rt_sdio_mgr_wait_mode_t wait_mode)
{
    int i;
    rt_bool_t is_ready;

    if (wait_mode == WAIT_MODE_ANY) {
        is_ready = RT_FALSE;
        for (i = 0; i < DEVICE_TYPE_MAX; i++) {
            if ((wait_events & sdio_mgr_event_map[i]) &&
                (sdio_mgr_dev.init_result[i] == RT_EOK)) {
                is_ready = RT_TRUE;
                break;
            }
        }
    } else {
        is_ready = RT_TRUE;
        for (i = 0; i < DEVICE_TYPE_MAX; i++) {
            if ((wait_events & sdio_mgr_event_map[i]) &&
                (sdio_mgr_dev.init_result[i] == -RT_ERROR)) {
                is_ready = RT_FALSE;
                break;
            }
        }
    }

    return is_ready;
}

static rt_uint32_t get_current_events(rt_uint32_t wait_events)
{
    rt_uint32_t current_events = 0;
    int i;

    for (i = 0; i < DEVICE_TYPE_MAX; i++) {
        if (sdio_mgr_dev.init_result[i] == RT_EOK)
            current_events |= sdio_mgr_event_map[i];
    }

    return current_events & wait_events;
}

static rt_err_t rt_sdio_mgr_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_sdio_mgr_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_sdio_mgr_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_sdio_mgr_control(rt_device_t dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);
    RT_ASSERT(args != RT_NULL);
    struct rt_sdio_mgr_device *mgr = RT_NULL;
    mgr = (struct rt_sdio_mgr_device *)dev;

    switch (cmd)
    {
    case SDIO_MGR_CMD_WAIT_READY:
    {
        struct rt_sdio_mgr_wait_params *params = (struct rt_sdio_mgr_wait_params *)args;

        rt_uint32_t recv_events;
        rt_uint8_t event_flags;

        if (check_events_is_ready(params->wait_events, params->wait_mode)) {
            recv_events = get_current_events(params->wait_events);
            if (params->recv_events != RT_NULL)
                *(params->recv_events) = recv_events;
            LOG_I("Events 0x%08x already ready.", recv_events);
            return RT_EOK;
        }

        event_flags = (params->wait_mode == WAIT_MODE_ALL) ? RT_EVENT_FLAG_AND : RT_EVENT_FLAG_OR;
        event_flags |= RT_EVENT_FLAG_CLEAR;

        LOG_D("Waiting for events: 0x%08X, mode: %s, timeout: %d ms",
                   params->wait_events,
                   params->wait_mode == WAIT_MODE_ALL ? "ALL" : "ANY",
                   params->timeout_ticks);

        rt_err_t result = rt_event_recv(&mgr->init_event, params->wait_events,
                                       event_flags, params->timeout_ticks, &recv_events);

        if (params->recv_events != RT_NULL)
            *(params->recv_events) = recv_events;

        if (result == RT_EOK)
            LOG_D("Wait successful, received events: 0x%08X", recv_events);
        else
            LOG_E("Wait error:%d", result);

        return result;
    }
    default:
        return -RT_EINVAL;
    }
}

void rt_sdio_mgr_notify_init_complete(rt_sdio_mgr_type_t type, rt_err_t result)
{
    if (type >= DEVICE_TYPE_MAX)
        return;

    if (result == RT_EOK) {
        LOG_I("%s initialization complete.", sdio_mgr_dev.device_names[type]);

        rt_uint32_t event = sdio_mgr_event_map[type];

        if (!sdio_mgr_dev.event_init_flag)
            return;

        rt_event_send(&sdio_mgr_dev.init_event, event);
    } else {
        LOG_E("%s initialization failed: %d.", sdio_mgr_dev.device_names[type], result);
    }
    sdio_mgr_dev.init_result[type] = result;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops sdio_mgr_device_ops =
{
    rt_sdio_mgr_init,
    rt_sdio_mgr_open,
    rt_sdio_mgr_close,
    RT_NULL,
    RT_NULL,
    rt_sdio_mgr_control
};
#endif /* RT_USING_DEVICE_OPS */

static int rt_sdio_mgr_device_register(void)
{
    rt_device_t device = &sdio_mgr_dev.parent;
    int i;

#ifdef RT_USING_DEVICE_OPS
    device->ops = &sdio_mgr_device_ops;
#else
    device->init = rt_sdio_mgr_init;
    device->open = rt_sdio_mgr_open;
    device->close = rt_sdio_mgr_close;
    device->read  = RT_NULL;
    device->write = RT_NULL;
    device->control = rt_sdio_mgr_control;
#endif /* RT_USING_DEVICE_OPS */

    rt_err_t result =  rt_event_init(&sdio_mgr_dev.init_event, "sdio_mgr_event", RT_IPC_FLAG_FIFO);
    if (result != RT_EOK) {
        rt_kprintf("sdio mgr event initialization failed! err:%d\n", result);
        return result;
    }
    sdio_mgr_dev.event_init_flag = RT_TRUE;

    for (i = 0; i < DEVICE_TYPE_MAX; i++)
        sdio_mgr_dev.init_result[i] = -RT_ERROR;

    sdio_mgr_dev.device_names[DEVICE_TYPE_SD_CARD] = "SD Card";
    sdio_mgr_dev.device_names[DEVICE_TYPE_MMC_CARD] = "MMC Card";
    sdio_mgr_dev.device_names[DEVICE_TYPE_SDIO_DEVICE] = "SDIO Device";

    return rt_device_register(device, "sdio_mgr", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}
INIT_DEVICE_EXPORT(rt_sdio_mgr_device_register);

