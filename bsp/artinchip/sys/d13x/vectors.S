/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <cpuport.h>

/* Enable interrupts when returning from the handler */
#define MSTATUS_PRV1 0x3880

.section .bss

    .align  3
    .global g_base_irqstack
    .global g_top_irqstack
g_base_irqstack:
    .space AIC_INTERRUPTSTACK_SIZE
g_top_irqstack:

    .align 2
    .globl g_trap_sp
    .type  g_trap_sp, object
g_trap_sp:
    .long 0
    .size g_trap_sp, .-g_trap_sp

#ifdef KERNEL_BAREMETAL
    .align  3
    .global g_base_normalstack
    .global g_top_normalstack
g_base_normalstack:
    .space AIC_NORMALSTACK_SIZE
g_top_normalstack:
#endif

irq_nested_level:
    .long 0

.text

    .align  2
    .global Default_IRQHandler
    .weak   Default_IRQHandler
    .type   Default_IRQHandler, %function
Default_IRQHandler:
#ifdef __riscv_xthead
    ipush
#else
    .long   0x0040000b
#endif

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(_NO_OMIT_FRAME_POINT_)
    addi    sp,sp,-8
    STORE   s0,(sp)
    csrr    a0, mepc
    STORE   a0, 4(sp)
    addi    s0, sp, 8
#endif

#ifdef ARCH_RISCV_FPU
    addi    sp, sp, -(20 * FREGBYTES)

    FSTORE  ft0,   0 * FREGBYTES(sp)
    FSTORE  ft1,   1 * FREGBYTES(sp)
    FSTORE  ft2,   2 * FREGBYTES(sp)
    FSTORE  ft3,   3 * FREGBYTES(sp)
    FSTORE  ft4,   4 * FREGBYTES(sp)
    FSTORE  ft5,   5 * FREGBYTES(sp)
    FSTORE  ft6,   6 * FREGBYTES(sp)
    FSTORE  ft7,   7 * FREGBYTES(sp)
    FSTORE  fa0,   8 * FREGBYTES(sp)
    FSTORE  fa1,   9 * FREGBYTES(sp)
    FSTORE  fa2,  10 * FREGBYTES(sp)
    FSTORE  fa3,  11 * FREGBYTES(sp)
    FSTORE  fa4,  12 * FREGBYTES(sp)
    FSTORE  fa5,  13 * FREGBYTES(sp)
    FSTORE  fa6,  14 * FREGBYTES(sp)
    FSTORE  fa7,  15 * FREGBYTES(sp)
    FSTORE  ft8,  16 * FREGBYTES(sp)
    FSTORE  ft9,  17 * FREGBYTES(sp)
    FSTORE  ft10, 18 * FREGBYTES(sp)
    FSTORE  ft11, 19 * FREGBYTES(sp)
#endif

#if defined(KERNEL_RTTHREAD)
    la      t0, rt_interrupt_enter
    jalr    t0
#elif defined(KERNEL_FREERTOS)
    la      t0, aicos_irq_enter
    jalr    t0
#endif

    /* call isr */
    /* t1 = irq_num */
    csrr    t1, mcause
    andi    t1, t1, 0x3FF
    slli    t1, t1, 2
    /* irq_handler_func(): t2 = g_irqvector[irq_num] */
    la      t0, g_irqvector
    add     t0, t0, t1
    lw      t2, (t0)
    /* para1 : a0 = irq_num */
    mv      a0, t1
    srli    a0, a0, 2
    /* para2 : a1 = g_irqdata[irq_num] = irq_data */
    la      t0, g_irqdata
    add     t0, t0, t1
    lw      a1, (t0)
    /* g_irqcnt[irq_num]++ */
    la      t0, g_irqcnt
    add     a3, t0, t1
    lw      t0, (a3)
    addi    t0, t0, 1
    sw      t0, (a3)

    /* call irq_handler_func(irq_num, irq_data) */
    jalr    t2

#if defined(KERNEL_RTTHREAD)
    la      t2, rt_interrupt_leave
    jalr    t2
#elif defined(KERNEL_FREERTOS)
    la      t2, aicos_irq_exit
    jalr    t2
#endif

#ifdef ARCH_RISCV_FPU
    FLOAD   ft0,  0 * FREGBYTES(sp)
    FLOAD   ft1,  1 * FREGBYTES(sp)
    FLOAD   ft2,  2 * FREGBYTES(sp)
    FLOAD   ft3,  3 * FREGBYTES(sp)
    FLOAD   ft4,  4 * FREGBYTES(sp)
    FLOAD   ft5,  5 * FREGBYTES(sp)
    FLOAD   ft6,  6 * FREGBYTES(sp)
    FLOAD   ft7,  7 * FREGBYTES(sp)
    FLOAD   fa0,  8 * FREGBYTES(sp)
    FLOAD   fa1,  9 * FREGBYTES(sp)
    FLOAD   fa2, 10 * FREGBYTES(sp)
    FLOAD   fa3, 11 * FREGBYTES(sp)
    FLOAD   fa4, 12 * FREGBYTES(sp)
    FLOAD   fa5, 13 * FREGBYTES(sp)
    FLOAD   fa6, 14 * FREGBYTES(sp)
    FLOAD   fa7, 15 * FREGBYTES(sp)
    FLOAD   ft8, 16 * FREGBYTES(sp)
    FLOAD   ft9, 17 * FREGBYTES(sp)
    FLOAD   ft10,18 * FREGBYTES(sp)
    FLOAD   ft11,19 * FREGBYTES(sp)

    addi    sp, sp, (20 * FREGBYTES)
#endif

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(_NO_OMIT_FRAME_POINT_)
lw    s0, (sp)
addi  sp, sp, 8
#endif

#ifdef __riscv_xthead
    ipop
#else
    .long   0x0050000b
#endif

/******************************************************************************
 * Functions:
 *     void trap(void);
 * default exception handler
 ******************************************************************************/
    .align  2
    .global trap
    .type   trap, %function
trap:
    /* Check for interrupt */
    addi    sp, sp, -4
    sw      t0, 0x0(sp)
    csrr    t0, mcause
    blt     t0, x0, .Lirq
    addi    sp, sp, 4
#ifndef AIC_BACKTRACE_DEBUG
    la      t0, g_trap_sp
#else
    mv       t0, sp
#endif
    addi    t0, t0, -132

    sw      x1,    0(t0)
    sw      x2,    4(t0)
    sw      x3,    8(t0)
    sw      x4,   12(t0)
    sw      x6,   20(t0)
    sw      x7,   24(t0)
    sw      x8,   28(t0)
    sw      x9,   32(t0)
    sw      x10,  36(t0)
    sw      x11,  40(t0)
    sw      x12,  44(t0)
    sw      x13,  48(t0)
    sw      x14,  52(t0)
    sw      x15,  56(t0)
    sw      x16,  60(t0)
    sw      x17,  64(t0)
    sw      x18,  68(t0)
    sw      x19,  72(t0)
    sw      x20,  76(t0)
    sw      x21,  80(t0)
    sw      x22,  84(t0)
    sw      x23,  88(t0)
    sw      x24,  92(t0)
    sw      x25,  96(t0)
    sw      x26, 100(t0)
    sw      x27, 104(t0)
    sw      x28, 108(t0)
    sw      x29, 112(t0)
    sw      x30, 116(t0)
    sw      x31, 120(t0)

    csrr    a0, mepc
    sw      a0, 124(t0)
    csrr    a0, mstatus
    sw      a0, 128(t0)

    mv      a0, t0
    lw      t0, -4(sp)
    mv      sp, a0
    sw      t0, 16(sp)

    la      a5, trap_c
    jalr    a5

#if defined(_ENABLE_BACK_TRACE_STACK_) && defined(KERNEL_RTTHREAD)
    mv      a0, sp
    addi    a0, a0, 132
    csrr    a1, mscratch
    csrr    a2, mepc
    la      a5, print_stack
    jalr    a5
#define CALL_STACK_MAX_DEPTH 32
    la      a0, rt_current_thread
    beqz    a0, .
#ifdef _NO_OMIT_FRAME_POINT_
    mv      t0, sp
    la      t1, CALL_STACK_MAX_DEPTH
    slli    t1, t1, 2
    sub     t0, t0, t1
    csrr    a1, mepc
    STORE   a1, (t0)
    mv      a1, s0
    li      t1, 1
1:
    // check sram addr
    li      t2, 0x30000000
    la      t3, AIC_SRAM_TOTAL_SIZE
    add     t3, t2, t3

    blt     a1, t2, 2f
    bge     a1, t3, 2f

    addi    a2, a1, -(4)
    blt     a2, t2, 2f
    bge     a2, t3, 2f

    addi    a2, a1, -(8)
    blt     a2, t2, 2f
    bge     a2, t3, 2f

    j       3f
2:
    // check psram addr
    li      t2, 0x40000000
    la      t3, AIC_PSRAM_SIZE
    add     t3, t2, t3

    blt     a1, t2, 4f
    bge     a1, t3, 4f

    addi    a2, a1, -(4)
    blt     a2, t2, 4f
    bge     a2, t3, 4f

    addi    a2, a1, -(8)
    blt     a2, t2, 4f
    bge     a2, t3, 4f
3:
    // get ra
    LOAD    a2, -4(a1)
    slli    t2, t1, 2
    add     t2, t2, t0
    STORE   a2, (t2)
    // get next fp
    LOAD    a1, -8(a1)
    //inc call depth
    addi    t1, t1, 1
    // check end
    li  a2, 0xdeadbeef
    beq a1, a2, 4f
    la  a2, CALL_STACK_MAX_DEPTH
    blt t1, a2, 1b
4:
    mv      a0, t1
    mv      a1, t0
    mv      sp, t0
    la      a5, print_back_trace
    jalr    a5
#else
    mv      a0, sp
    addi    a0, a0, 132
    csrr    a1, mscratch
    csrr    a2, mepc
    la      a5, backtrace_call_stack
    jalr    a5
#endif
#endif


    j       .

.Lirq:
    lw      t0, 0x0(sp)
    addi    sp, sp, 4
    j       Default_IRQHandler

    .align  6
    .weak   Default_Handler
    .global Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    /* Check for nmi */
    addi    sp, sp, -8
    sw      t0, 0x0(sp)
    sw      t1, 0x4(sp)
    csrr    t0, mcause
    andi    t0, t0, 0x3FF
    li      t1, 24
    beq     t0, t1, .NMI_Handler
    lw      t0, 0x0(sp)
    lw      t1, 0x4(sp)
    addi    sp, sp, 8
    j       trap

.NMI_Handler:
    lw      t0, 0x0(sp)
    lw      t1, 0x4(sp)
    addi    sp, sp, 8

    addi    sp, sp, -64

    sw      ra,  0(sp)
    sw      t0,  4(sp)
    sw      t1,  8(sp)
    sw      t2, 12(sp)
    sw      a0, 16(sp)
    sw      a1, 20(sp)
    sw      a2, 24(sp)
    sw      a3, 28(sp)
    sw      a4, 32(sp)
    sw      a5, 36(sp)
    sw      a6, 40(sp)
    sw      a7, 44(sp)
    sw      t3, 48(sp)
    sw      t4, 52(sp)
    sw      t5, 56(sp)
    sw      t6, 60(sp)

#ifdef ARCH_RISCV_FPU
    addi    sp,    sp, -(20*FREGBYTES)
    FSTORE  ft0,   0 * FREGBYTES(sp)
    FSTORE  ft1,   1 * FREGBYTES(sp)
    FSTORE  ft2,   2 * FREGBYTES(sp)
    FSTORE  ft3,   3 * FREGBYTES(sp)
    FSTORE  ft4,   4 * FREGBYTES(sp)
    FSTORE  ft5,   5 * FREGBYTES(sp)
    FSTORE  ft6,   6 * FREGBYTES(sp)
    FSTORE  ft7,   7 * FREGBYTES(sp)
    FSTORE  fa0,   8 * FREGBYTES(sp)
    FSTORE  fa1,   9 * FREGBYTES(sp)
    FSTORE  fa2,  10 * FREGBYTES(sp)
    FSTORE  fa3,  11 * FREGBYTES(sp)
    FSTORE  fa4,  12 * FREGBYTES(sp)
    FSTORE  fa5,  13 * FREGBYTES(sp)
    FSTORE  fa6,  14 * FREGBYTES(sp)
    FSTORE  fa7,  15 * FREGBYTES(sp)
    FSTORE  ft8,  16 * FREGBYTES(sp)
    FSTORE  ft9,  17 * FREGBYTES(sp)
    FSTORE  ft10, 18 * FREGBYTES(sp)
    FSTORE  ft11, 19 * FREGBYTES(sp)
#endif

    la      t0, g_nmivector
    lw      t0, (t0)
    jalr    t0

#ifdef ARCH_RISCV_FPU
    FLOAD   ft0,   0 * FREGBYTES(sp)
    FLOAD   ft1,   1 * FREGBYTES(sp)
    FLOAD   ft2,   2 * FREGBYTES(sp)
    FLOAD   ft3,   3 * FREGBYTES(sp)
    FLOAD   ft4,   4 * FREGBYTES(sp)
    FLOAD   ft5,   5 * FREGBYTES(sp)
    FLOAD   ft6,   6 * FREGBYTES(sp)
    FLOAD   ft7,   7 * FREGBYTES(sp)
    FLOAD   fa0,   8 * FREGBYTES(sp)
    FLOAD   fa1,   9 * FREGBYTES(sp)
    FLOAD   fa2,  10 * FREGBYTES(sp)
    FLOAD   fa3,  11 * FREGBYTES(sp)
    FLOAD   fa4,  12 * FREGBYTES(sp)
    FLOAD   fa5,  13 * FREGBYTES(sp)
    FLOAD   fa6,  14 * FREGBYTES(sp)
    FLOAD   fa7,  15 * FREGBYTES(sp)
    FLOAD   ft8,  16 * FREGBYTES(sp)
    FLOAD   ft9,  17 * FREGBYTES(sp)
    FLOAD   ft10, 18 * FREGBYTES(sp)
    FLOAD   ft11, 19 * FREGBYTES(sp)

    addi    sp, sp, (20 * FREGBYTES)
#endif

    lw      ra,  0(sp)
    lw      t0,  4(sp)
    lw      t1,  8(sp)
    lw      t2, 12(sp)
    lw      a0, 16(sp)
    lw      a1, 20(sp)
    lw      a2, 24(sp)
    lw      a3, 28(sp)
    lw      a4, 32(sp)
    lw      a5, 36(sp)
    lw      a6, 40(sp)
    lw      a7, 44(sp)
    lw      t3, 48(sp)
    lw      t4, 52(sp)
    lw      t5, 56(sp)
    lw      t6, 60(sp)

    addi    sp, sp, 64
    mret


    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler handler_name
    .weak   \handler_name
    .globl  \handler_name
    .set    \handler_name, Default_Handler
    .endm

//    def_irq_handler PendSV_Handler
//    def_irq_handler SysTick_Handler

#ifdef KERNEL_BAREMETAL
    .align  6
    .weak   PendSV_Handler
    .global PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    j       trap

    .size   PendSV_Handler, . - PendSV_Handler
#endif
