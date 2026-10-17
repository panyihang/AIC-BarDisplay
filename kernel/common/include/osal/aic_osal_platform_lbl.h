/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_
#define _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>
#include <csi_core.h>
#include <aic_drv_irq.h>
//#include <aic_tlsf.h>

//--------------------------------------------------------------------+
// Irq API
//--------------------------------------------------------------------+

static inline void aicos_local_irq_save(unsigned long *state)
{
    *state = csi_irq_save();
}

static inline void aicos_local_irq_restore(unsigned long state)
{
    csi_irq_restore(state);
}

extern unsigned long g_aicos_irq_state;

static inline void aicos_local_irq_disable(void)
{
    __disable_irq();
}

static inline void aicos_local_irq_enable(void)
{
    __enable_irq();
}

enum irqreturn
{
    IRQ_NONE        = (0 << 0),
    IRQ_HANDLED     = (1 << 0),
    IRQ_WAKE_THREAD = (1 << 1),
};
typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);
typedef irqreturn_t (*pin_irq_handler_t)(void *);

static inline int aicos_request_irq(unsigned int irq, irq_handler_t handler, unsigned int flags,
                                 const char *name, void *data)
{
    drv_irq_register(irq, handler, data);
    drv_irq_enable(irq);

    return 0;
}

static inline void aicos_irq_enable(unsigned int irq)
{
    drv_irq_enable(irq);
}

static inline void aicos_irq_disable(unsigned int irq)
{
    drv_irq_disable(irq);
}

static inline void aicos_irq_set_prio(unsigned int irq, unsigned int priority)
{
    if (irq >= MAX_IRQn)
        return;

#if __riscv_xlen == 64
    csi_plic_set_prio(PLIC_BASE, irq, priority);
#elif __riscv_xlen == 32
    csi_vic_set_prio(irq, priority);
#else
#endif
}

static inline unsigned int aicos_irq_get_prio(unsigned int irq)
{
    if (irq >= MAX_IRQn)
        return -1;

#if __riscv_xlen == 64
    return csi_plic_get_prio(PLIC_BASE, irq);
#elif __riscv_xlen == 32
    return csi_vic_get_prio(irq);
#else
    return 0;
#endif
}

static inline void aicos_irq_set_threshold(unsigned int threshold)
{
#if __riscv_xlen == 64
    csi_plic_set_threshold(PLIC_BASE, threshold);
#elif __riscv_xlen == 32
    csi_vic_set_threshold(threshold);
#else
#endif
}

static inline unsigned int aicos_irq_get_threshold(void)
{
#if __riscv_xlen == 64
    return csi_plic_get_threshold(PLIC_BASE);
#elif __riscv_xlen == 32
    return csi_vic_get_threshold();
#else
    return 0;
#endif
}

//--------------------------------------------------------------------+
// Cache API
//--------------------------------------------------------------------+

static inline void aicos_icache_enable(void)
{
    csi_icache_enable();
}

static inline void aicos_icache_disable(void)
{
    csi_icache_disable();
}

static inline void aicos_icache_invalid(void)
{
    csi_icache_invalid();
}

static inline void aicos_dcache_enable(void)
{
    csi_dcache_enable();
}

static inline void aicos_dcache_disable(void)
{
    csi_dcache_disable();
}

static inline void aicos_dcache_invalid(void)
{
    csi_dcache_invalid();
}

static inline void aicos_dcache_clean(void)
{
    csi_dcache_clean();
}

static inline void aicos_dcache_clean_invalid(void)
{
    csi_dcache_clean_invalid();
}

static inline void aicos_dcache_invalid_range(void *addr, u32 size)
{
    csi_dcache_invalid_range((phy_addr_t)(ptr_t)addr, size);
}

static inline void aicos_dcache_clean_range(void *addr, u32 size)
{
    csi_dcache_clean_range((phy_addr_t)(ptr_t)addr, size);
}

static inline void aicos_dcache_clean_invalid_range(void *addr, u32 size)
{
    csi_dcache_clean_invalid_range((phy_addr_t)(ptr_t)addr, size);
}

//--------------------------------------------------------------------+
// DMA API
//--------------------------------------------------------------------+

extern unsigned char g_dma_w_sync_buffer[CACHE_LINE_SIZE];

static inline void aicos_dma_sync(void)
{
#ifdef AIC_CHIP_D21X
    asm volatile("sw t0, (%0)" : : "r"(g_dma_w_sync_buffer));
    csi_dcache_clean_range((phy_addr_t)(ptr_t)g_dma_w_sync_buffer, CACHE_LINE_SIZE);
#endif
}

//--------------------------------------------------------------------+
// Delay API
//--------------------------------------------------------------------+
extern void aic_udelay(u32 us);

static inline void aicos_mdelay(unsigned long msecs)
{
    aic_udelay(msecs * 1000);
}

static inline void aicos_udelay(unsigned long usecs)
{
    aic_udelay(usecs);
}

#ifdef __cplusplus
}
#endif

#endif /* _ARTINCHIP_AIC_OSAL_PLATFORM_LBL_H_ */
