/*
 * Copyright (c) 2020-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_VE_H_
#define _ARTINCHIP_HAL_VE_H_

#define IOC_VE_WAIT         0x01
#define IOC_VE_GET_CLIENT   0x02
#define IOC_VE_PUT_CLIENT   0x03
#define IOC_VE_GET_INFO     0x04
#define IOC_VE_RESET        0x05

struct wait_info {
        int wait_time;
        unsigned int reg_status;
};

struct aic_ve_client;

struct aic_ve_client *hal_ve_open(void);
int hal_ve_close(struct aic_ve_client *client);
int hal_ve_close_with_wait(void);
int hal_ve_control(struct aic_ve_client *client, int cmd, void *arg);
int hal_ve_probe(void);

#endif
