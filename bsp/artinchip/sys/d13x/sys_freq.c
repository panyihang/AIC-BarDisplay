/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <aic_core.h>
#include <sys_freq.h>

int g_system_clock = IHS_VALUE;

int32_t drv_get_cpu_freq(int32_t idx)
{
    return g_system_clock;
}

int32_t drv_get_usi_freq(int32_t idx)
{
    return g_system_clock;
}

int32_t drv_get_usart_freq(int32_t idx)
{
#ifdef QEMU_RUN
    return g_system_clock;
#else
    return CLOCK_24M;
#endif

}

int32_t drv_get_pwm_freq(int32_t idx)
{
    return g_system_clock;
}

int32_t drv_get_i2s_freq(int32_t idx)
{
    return g_system_clock;
}

int32_t drv_get_sys_freq(void)
{
#ifdef QEMU_RUN
    return g_system_clock;
#else
    return CLOCK_4M;
#endif
}

int32_t drv_get_rtc_freq(int32_t idx)
{
    return g_system_clock;
}

int32_t drv_get_apb_freq(void)
{
    return g_system_clock;
}

int32_t drv_get_timer_freq(int32_t idx)
{
    return g_system_clock;
}
