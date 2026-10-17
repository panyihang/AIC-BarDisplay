/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: zrq <zrq@artinchip.com>
 */

#ifndef __SDIO_MGR_H__
#define __SDIO_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEVICE_TYPE_SD_CARD = 0,
    DEVICE_TYPE_MMC_CARD,
    DEVICE_TYPE_SDIO_DEVICE,
    DEVICE_TYPE_MAX
} rt_sdio_mgr_type_t;

typedef enum {
    WAIT_MODE_ANY,
    WAIT_MODE_ALL
} rt_sdio_mgr_wait_mode_t;

/* sdio manager control command */
#define SDIO_MGR_CMD_WAIT_READY        (RT_DEVICE_CTRL_BASE(SDIO) + 0)
#define SDIO_MGR_CMD_IS_READY          (RT_DEVICE_CTRL_BASE(SDIO) + 1)
#define SDIO_MGR_CMD_WAIT_CUSTOM       (RT_DEVICE_CTRL_BASE(SDIO) + 2)

/* event define */
#define SDIO_EVENT_SD_CARD_READY      (1 << 0)
#define SDIO_EVENT_MMC_CARD_READY     (1 << 1)
#define SDIO_EVENT_SDIO_DEVICE_READY  (1 << 2)
#define SDIO_EVENT_ALL_READY          (SDIO_EVENT_SD_CARD_READY | \
                                SDIO_EVENT_MMC_CARD_READY | SDIO_EVENT_SDIO_DEVICE_READY)

struct rt_sdio_mgr_wait_params {
    rt_uint32_t wait_events;
    rt_sdio_mgr_wait_mode_t wait_mode;
    rt_int32_t timeout_ticks;
    rt_uint32_t *recv_events;
};

struct rt_sdio_mgr_device
{
    struct rt_device parent;
    struct rt_event init_event;
    rt_bool_t event_init_flag;
    rt_err_t init_result[DEVICE_TYPE_MAX];
    const char *device_names[DEVICE_TYPE_MAX];
};

void rt_sdio_mgr_notify_init_complete(rt_sdio_mgr_type_t type, rt_err_t result);

#ifdef __cplusplus
}
#endif

#endif /* __SDIO_MGR_H__ */
