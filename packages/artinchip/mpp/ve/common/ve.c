/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  author: qi.xu@artinchip.com
 *  Desc: ve module
 */

#define LOG_TAG "ve"

#include <stdlib.h>
#include <string.h>

#include "aic_core.h"
#include "ve.h"
#include "mpp_log.h"
#include "aic_drv_ve.h"

#define VE_DEV        "/dev/aic_ve"
#define VE_REG_BASE  0x18C00000

// 2s
#define VE_TIMEOUT    (2000)

struct ve_device_info {
    unsigned long reg_base;
    //int ve_fd;
    struct aic_ve_client* client;
    int reg_size;
};

//static pthread_mutex_t     g_ve_mutex = PTHREAD_MUTEX_INITIALIZER;
aicos_mutex_t g_ve_mutex =NULL;

int             g_ve_ref = 0;
struct ve_device_info     g_ve_info;

int ve_open_device(void)
{
     if (g_ve_mutex == NULL) {
        g_ve_mutex = aicos_mutex_create();
    }

    aicos_mutex_take(g_ve_mutex, AICOS_WAIT_FOREVER);

     if (g_ve_ref == 0) {
        // 1. open /dev/aic_ve
        //g_ve_info.ve_fd = open(VE_DEV, O_RDWR);
        g_ve_info.client = drv_ve_open();
         if (g_ve_ref < 0) {
            loge("open %s failed!", VE_DEV);
            //pthread_mutex_unlock(&g_ve_mutex);
            aicos_mutex_give(g_ve_mutex);
            return -1;
        }

        // 3. map register space to virtual space
        g_ve_info.reg_base = VE_REG_BASE;
        //logd("g_reg_base: %08x", g_ve_info.reg_base);
    }

    g_ve_ref ++;

    aicos_mutex_give(g_ve_mutex);

    return 0;
}

void ve_close_device()
{
    aicos_mutex_take(g_ve_mutex, AICOS_WAIT_FOREVER);
     if (g_ve_ref == 0) {
        logd("ve has been closed");
        aicos_mutex_give(g_ve_mutex);
        return;
    }
    g_ve_ref --;

     if (g_ve_ref == 0) {
        drv_ve_close(g_ve_info.client);
    }

    aicos_mutex_give(g_ve_mutex);
}

unsigned long ve_get_reg_base()
{
    return g_ve_info.reg_base;
}

int ve_reset()
{
    int ret;

    aicos_mutex_take(g_ve_mutex, AICOS_WAIT_FOREVER);

    ret = drv_ve_control(g_ve_info.client, IOC_VE_RESET, NULL);

    aicos_mutex_give(g_ve_mutex);
    return ret;
}

int ve_wait(unsigned int *reg_status)
{
    int ret;

    aicos_mutex_take(g_ve_mutex, AICOS_WAIT_FOREVER);

    struct wait_info info;
    info.wait_time = VE_TIMEOUT;

    ret = drv_ve_control(g_ve_info.client, IOC_VE_WAIT, &info);
    *reg_status = info.reg_status;

    aicos_mutex_give(g_ve_mutex);
    return ret;
}

int ve_get_client()
{
    int ret;

    ret = drv_ve_control(g_ve_info.client, IOC_VE_GET_CLIENT, NULL);

    return ret;
}

int ve_put_client()
{
    int ret;

    ret = drv_ve_control(g_ve_info.client, IOC_VE_PUT_CLIENT, NULL);

    return ret;
}

