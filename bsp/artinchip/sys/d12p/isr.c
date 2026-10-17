/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#endif
#include <aic_core.h>

extern void systick_handler(void);
extern void xPortSysTickHandler(void);
extern void OSTimeTick(void);

void SysTick_Handler(int irq_num, void *data)
{
    csi_coret_config(drv_get_sys_freq() / CONFIG_SYSTICK_HZ, CORET_IRQn);

#if defined(KERNEL_RHINO)
    systick_handler();
#elif defined(KERNEL_FREERTOS)
    xPortSysTickHandler();
#elif defined(KERNEL_RTTHREAD)
    rt_tick_increase();
#elif defined(KERNEL_UCOS)
    OSTimeTick();
#elif defined(KERNEL_BAREMETAL)
#endif
}

void TIM4_NMIHandler(int irq_num, void *data)
{

}
