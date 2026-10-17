/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_drv_irq.h>
#include <aic_hal.h>

extern void drv_irq_vectors_init(void);
extern void mm_heap_initialize(void);

int SystemCoreClock = IHS_VALUE;  /* System Core Clock Frequency      */
extern int __Vectors;

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = IHS_VALUE;
}

#ifndef QEMU_RUN
void aic_clk_lowpower(void)
{
    hal_clk_pll_lowpower();
    hal_clk_disable_assertrst(CLK_USB_PHY0);
    hal_clk_disable_assertrst(CLK_USBH0);
    hal_clk_disable_assertrst(CLK_USBD);
}

void aic_gtc_enable(void)
{
    /* enable gtc clk */
    *(volatile uint32_t *)(CMU_BASE+0x090c) = 0x3100;
    /* enable gtc */
    *(volatile uint32_t *)GTC_BASE = 0x0001;
}
#endif

void dcache_enable(void)
{
    aicos_dcache_enable();
}

void dcache_clean(void)
{
    aicos_dcache_clean();
}

void icache_enable(void)
{
    aicos_icache_enable();
}

static void _system_init_for_kernel(void)
{
    drv_irq_vectors_init();

#ifndef QEMU_RUN
    aic_gtc_enable();
    aic_clk_lowpower();
#endif
#ifndef KERNEL_BAREMETAL
    csi_coret_config(drv_get_sys_freq() / CONFIG_SYSTICK_HZ, CORET_IRQn);    //10ms
    drv_irq_enable(CORET_IRQn);
#endif
}

/**
  * @brief  initialize system map
  * @param  None
  * @return None
  */
void systemmap_config(void)
{
   csi_sysmap_config_region(0, 0x20000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
   csi_sysmap_config_region(1, 0x40000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
   csi_sysmap_config_region(2, 0x50000000, SYSMAP_SYSMAPCFG_SO_Msk);
   csi_sysmap_config_region(3, 0x50700000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
   csi_sysmap_config_region(4, 0x60000000, SYSMAP_SYSMAPCFG_SO_Msk);
   csi_sysmap_config_region(5, 0x80000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
   csi_sysmap_config_region(6, 0x90000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
   csi_sysmap_config_region(7, 0xf0000000, SYSMAP_SYSMAPCFG_SO_Msk);
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    int i;

#ifdef QEMU_RUN
    systemmap_config();
#endif

    /* enable mstatus FS */
#ifdef ARCH_RISCV_FPU
    uint32_t mstatus = __get_MSTATUS();
    mstatus |= (1 << 13);
    __set_MSTATUS(mstatus);
#endif

    /* enable mxstatus THEADISAEE */
    uint32_t mxstatus = __get_MXSTATUS();
    mxstatus |= (1 << 22);
    /* enable mxstatus MM */
    mxstatus |= (1 << 15);
    __set_MXSTATUS(mxstatus);

    /* enable mexstatus SPUSHEN/SPSWAPEN */
    uint32_t mexstatus = __get_MEXSTATUS();
#ifdef CONFIG_THEAD_EXT_SPUSHEN
    mexstatus |= (1 << 16); // SPUSHEN
#endif
#ifdef CONFIG_THEAD_EXT_SPSWAPEN
    mexstatus |= (1 << 17); // SPSWAPEN
#endif
    __set_MEXSTATUS(mexstatus);

    /* get interrupt level from info */
    CLIC->CLICCFG = (((CLIC->CLICINFO & CLIC_INFO_CLICINTCTLBITS_Msk) >> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);

    for (i = 0; i < MAX_IRQn; i++)
    {
        CLIC->CLICINT[i].IP = 0;
        CLIC->CLICINT[i].ATTR = 1; /* use vector interrupt */
    }

    /* tspend use positive interrupt */
    CLIC->CLICINT[Machine_Software_IRQn].ATTR = 0x3;

    csi_dcache_enable();
    csi_icache_enable();
    drv_irq_enable(Machine_Software_IRQn);

    _system_init_for_kernel();
}
