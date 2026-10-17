/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <cpuport.h>
#include <aic_soc.h>

.section .vectors, "aw", @progbits
    .align  6
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    .long   Default_Handler      /* 0 */
    .long   Default_Handler      /* 1 */
    .long   Default_Handler      /* 2 */
    .long   PendSV_Handler       /* 3 */
    .long   Default_Handler      /* 4 */
    .long   Default_Handler      /* 5 */
    .long   Default_Handler      /* 6 */
    .long   Default_IRQHandler   /* 7 */
    .long   Default_Handler      /* 8 */
    .long   Default_Handler      /* 9 */
    .long   Default_Handler      /* 10 */
    .long   Default_Handler      /* 11 */
    .long   Default_Handler      /* 12 */
    .long   Default_Handler      /* 13 */
    .long   Default_Handler      /* 14 */
    .long   Default_Handler      /* 15 */

    /* External interrupts */
    .long   Default_IRQHandler   /* 16 */
    .long   Default_IRQHandler   /* 17 */
    .long   Default_IRQHandler   /* 18 */
    .long   Default_IRQHandler   /* 19 */
    .long   Default_IRQHandler   /* 20 */
    .long   Default_IRQHandler   /* 21 */
    .long   Default_IRQHandler   /* 22 */
    .long   Default_IRQHandler   /* 23 */
    .long   Default_IRQHandler   /* 24 */
    .long   Default_IRQHandler   /* 25 */
    .long   Default_IRQHandler   /* 26 */
    .long   Default_IRQHandler   /* 27 */
    .long   Default_IRQHandler   /* 28 */
    .long   Default_IRQHandler   /* 29 */
    .long   Default_IRQHandler   /* 30 */
    .long   Default_IRQHandler   /* 31 */
    .long   Default_IRQHandler   /* 32 */
    .long   Default_IRQHandler   /* 33 */
    .long   Default_IRQHandler   /* 34 */
    .long   Default_IRQHandler   /* 35 */
    .long   Default_IRQHandler   /* 36 */
    .long   Default_IRQHandler   /* 37 */
    .long   Default_IRQHandler   /* 38 */
    .long   Default_IRQHandler   /* 39 */
    .long   Default_IRQHandler   /* 40 */
    .long   Default_IRQHandler   /* 41 */
    .long   Default_IRQHandler   /* 42 */
    .long   Default_IRQHandler   /* 43 */
    .long   Default_IRQHandler   /* 44 */
    .long   Default_IRQHandler   /* 45 */
    .long   Default_IRQHandler   /* 46 */
    .long   Default_IRQHandler   /* 47 */
    .long   Default_IRQHandler   /* 48 */
    .long   Default_IRQHandler   /* 49 */
    .long   Default_IRQHandler   /* 50 */
    .long   Default_IRQHandler   /* 51 */
    .long   Default_IRQHandler   /* 52 */
    .long   Default_IRQHandler   /* 53 */
    .long   Default_IRQHandler   /* 54 */
    .long   Default_IRQHandler   /* 55 */
    .long   Default_IRQHandler   /* 56 */
    .long   Default_IRQHandler   /* 57 */
    .long   Default_IRQHandler   /* 58 */
    .long   Default_IRQHandler   /* 59 */
    .long   Default_IRQHandler   /* 60 */
    .long   Default_IRQHandler   /* 61 */
    .long   Default_IRQHandler   /* 62 */
    .long   Default_IRQHandler   /* 63 */
    .long   Default_IRQHandler   /* 64 */
    .long   Default_IRQHandler   /* 65 */
    .long   Default_IRQHandler   /* 66 */
    .long   Default_IRQHandler   /* 67 */
    .long   Default_IRQHandler   /* 68 */
    .long   Default_IRQHandler   /* 69 */
    .long   Default_IRQHandler   /* 70 */
    .long   Default_IRQHandler   /* 71 */
    .long   Default_IRQHandler   /* 72 */
    .long   Default_IRQHandler   /* 73 */
    .long   Default_IRQHandler   /* 74 */
    .long   Default_IRQHandler   /* 75 */
    .long   Default_IRQHandler   /* 76 */
    .long   Default_IRQHandler   /* 77 */
    .long   Default_IRQHandler   /* 78 */
    .long   Default_IRQHandler   /* 79 */
    .long   Default_IRQHandler   /* 80 */
    .long   Default_IRQHandler   /* 81 */
    .long   Default_IRQHandler   /* 82 */
    .long   Default_IRQHandler   /* 83 */
    .long   Default_IRQHandler   /* 84 */
    .long   Default_IRQHandler   /* 85 */
    .long   Default_IRQHandler   /* 86 */
    .long   Default_IRQHandler   /* 87 */
    .long   Default_IRQHandler   /* 88 */
    .long   Default_IRQHandler   /* 89 */
    .long   Default_IRQHandler   /* 90 */
    .long   Default_IRQHandler   /* 91 */
    .long   Default_IRQHandler   /* 92 */
    .long   Default_IRQHandler   /* 93 */
    .long   Default_IRQHandler   /* 94 */
    .long   Default_IRQHandler   /* 95 */
    .long   Default_IRQHandler   /* 96 */
    .long   Default_IRQHandler   /* 97 */
    .long   Default_IRQHandler   /* 98 */
    .long   Default_IRQHandler   /* 99 */
    .size __Vectors,.-__Vectors

    .text
    .align  2
_start:
    .text
    .align  2
    .globl  Reset_Handler
    .globl  save_boot_params_ret
    .type   save_boot_params_ret,%function
    .type   Reset_Handler, %function
Reset_Handler:
.option push
.option norelax
    j       save_boot_params
save_boot_params_ret:
    la      gp, __global_pointer$
.option pop
    la      a0, Default_Handler
    ori     a0, a0, 3
    csrw    mtvec, a0
    la      a0, __Vectors
    csrw    mtvt, a0

    la      sp, g_top_irqstack
#ifdef CONFIG_THEAD_EXT_SPSWAPEN
    csrw    mscratch, sp
#endif
#ifdef KERNEL_BAREMETAL
    la      sp, g_top_normalstack
#endif

    la      a5, icache_enable
    jalr    a5
    la      a5, dcache_enable
    jalr    a5

#ifdef AIC_EARLY_DEBUG
    la      a5, early_debug_init
    jalr    a5
#endif

#ifndef QEMU_RUN

#ifdef AIC_TCM_EN
e907_tcm_init:
    /* setup tcm start address */
    la      t0, __itcm_start            // 0x30040000UL
    ori     t0, t0, 0x1
    csrs    mitcmcr, t0
    la      t0, __dtcm_start            // 0x30060000UL
    ori     t0, t0, 0x1
    csrs    mdtcmcr, t0
    fence

    /* enable syscfg clk */
    li      a0, (CMU_BASE + 0x800)      // 0x18020800UL
    li      t0, 0x00001000UL
    sw      t0, (a0)

    /* enable tcm */
    li      a0, (SYSCFG_BASE + 0x160)   // 0x18000160UL
    lw      t0, (a0)
    andi    t0, t0, ~0x00000002UL
    sw      t0, (a0)
    ori     t0, t0, 0x00000001UL
    sw      t0, (a0)
#endif /* AIC_TCM_EN */

#ifdef AIC_SRAM1_EN
    /* enable sram_s1 */
    li      a0, (SYSCFG_BASE + 0x160)   // 0x18000160UL
    lw      t0, (a0)
    andi    t0, t0, ~0x00000070UL
    ori     t0, t0, AIC_SRAM_S1_REG_SIZE
    sw      t0, (a0)
#endif /* AIC_SRAM1_EN */

#ifdef PSRAM_UNCACHED_EN
    /* set psram uncacheable */
    li      a0, 0x2FFFF034UL            // SYSMAP_ADDR6_ATTR
    li      t0, 0x00000004UL
    sw      t0, (a0)
#endif /* PSRAM_UNCACHED_EN */

#endif /* QEMU_RUN */

    /* Load data section */
#if defined(QEMU_RUN)
    la      a0, __erodata
    la      a1, __data_start__
    la      a2, __data_end__
    bgeu    a1, a2, 2f
1:
    lw      t0, (a0)
    sw      t0, (a1)
    addi    a0, a0, 4
    addi    a1, a1, 4
    bltu    a1, a2, 1b
2:
    la      t0, dcache_clean
    jalr    t0
#endif

    /* Clear bss section */
    la      a0, __bss_start__
    la      a1, __bss_end__
    bgeu    a0, a1, 2f
1:
    sw      zero, (a0)
    addi    a0, a0, 4
    bltu    a0, a1, 1b
2:

    /* Reloc private params */
    la      a5, reloc_private_params
    jalr    a5

#ifndef __NO_SYSTEM_INIT
    la      a5, SystemInit
    jalr    a5
#endif

#if (defined(KERNEL_RTTHREAD) || defined(KERNEL_FREERTOS))
    la      a5, entry
    jalr    a5
#else
    la      a5, main
    jalr    a5
#endif

    .size   Reset_Handler, . - Reset_Handler

__exit:
    j      __exit

