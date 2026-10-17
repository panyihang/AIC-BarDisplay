/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtconfig.h>

#if defined(KERNEL_RTTHREAD) && defined(AIC_BACKTRACE_DEBUG)
#include <rtthread.h>
#endif

#include <csi_core.h>

void (*trap_c_callback)(void);

void trap_c(uint32_t *regs)
{
    int i;
    uint32_t vec = 0;

    vec = __get_MCAUSE() & 0x3FF;

    printf("CPU Exception: NO.%ld", vec);
    printf("\n");

    for (i = 0; i < 31; i++)
    {
        printf("x%d: %08lx\t", i + 1, regs[i]);

        if ((i % 4) == 3)
        {
            printf("\n");
        }
    }

    printf("\n");
    printf("mcause : 0x%08lx\n", __get_MCAUSE());
    printf("mtval  : 0x%08lx\n", __get_MTVAL());
    printf("mepc   : 0x%08lx\n", regs[31]);
    printf("mstatus: 0x%08lx\n", regs[32]);
    printf("\n");

    if (trap_c_callback) {
        trap_c_callback();
    }
#ifndef AIC_BACKTRACE_DEBUG
    while (1) {};
#endif
}

#if defined(KERNEL_RTTHREAD) && defined(AIC_BACKTRACE_DEBUG)

#define CMB_CALL_STACK_MAX_DEPTH 32
extern size_t __stext;
extern size_t __etext;
extern rt_ubase_t g_base_irqstack;
extern rt_ubase_t g_top_irqstack;

void print_stack(uint32_t *stack_point,uint32_t*stack_point1,uint32_t epc)
{
    int i = 0;
    uint32_t sp = (uint32_t)stack_point;
    uint32_t pc;
    uint32_t base_irqstack = (uint32_t)&g_base_irqstack;
    uint32_t top_irqstack = (uint32_t)&g_top_irqstack;
    uint32_t stack_addr = (uint32_t) rt_thread_self()->stack_addr;
    uint32_t stack_size =  rt_thread_self()->stack_size;

    printf("__stext:%p __etext:%p,stack:\n", &__stext, &__etext);
    if (sp >= base_irqstack && sp <= top_irqstack) {
        //printf("[%s:%d]base_irqstack::0x%08lx top_irqstack::0x%08lx\n",__FUNCTION__,__LINE__, base_irqstack, top_irqstack);
        printf("base_irqstack::0x%08lx top_irqstack::0x%08lx\n", base_irqstack, top_irqstack);
        for (; sp < top_irqstack; sp += sizeof(uint32_t)) {
            pc  = *((uint32_t *)sp);
            printf("0x%08lx ", pc);
            if ((i % 4) == 3) {
                printf("\n");
            }
            i++;
        }
        sp = (uint32_t)stack_point1;
        printf("\n");
    }

    //printf("[%s:%d]stack_addr:0x%08lx stack_addr_end:0x%08lx\n",__FUNCTION__,__LINE__, stack_addr, stack_addr + stack_size);
    printf("stack_addr:0x%08lx stack_addr_end:0x%08lx\n",stack_addr, stack_addr + stack_size);
    for (i = 0; sp < stack_addr + stack_size; sp += sizeof(uint32_t)) {
        pc  = *((uint32_t *)sp);
        printf("0x%08lx ", pc);
        if ((i % 4) == 3) {
            printf("\n");
        }
        i++;
    }
    printf("\n\n");
}

void print_back_trace(int32_t n,rt_ubase_t *regs)
{
    int i = 0;
    printf("back_trace:\n");
    for(i = 0; i < n; i++) {
        printf("0x%08lx\n", regs[i]);
    }
}

void backtrace_call_stack(rt_ubase_t *stack_point,rt_ubase_t*stack_point1,rt_ubase_t epc)
{
    int depth = 0;
    rt_ubase_t sp = (rt_ubase_t)stack_point;
    rt_ubase_t pc;
    size_t code_start_addr = (size_t)&__stext;
    size_t code_end_addr = (size_t)&__etext;
    rt_ubase_t base_irqstack = (rt_ubase_t)&g_base_irqstack;
    rt_ubase_t top_irqstack = (rt_ubase_t)&g_top_irqstack;
    rt_ubase_t stack_addr = (rt_ubase_t) rt_thread_self()->stack_addr;
    rt_ubase_t stack_size =  rt_thread_self()->stack_size;

    printf("[%s:%d]__stext:%p __etext:%p\r\n",__FUNCTION__,__LINE__, &__stext, &__etext);

    printf("0x%08lx\n", epc);

    if (sp >= base_irqstack && sp <= top_irqstack) {
        for (; sp < top_irqstack; sp += sizeof(rt_ubase_t)) {
            pc  = *((rt_ubase_t *)sp);
            if((pc >= code_start_addr) && (pc <= code_end_addr) && (depth < CMB_CALL_STACK_MAX_DEPTH)) {
                printf("%08lx\n", pc);
                depth++;
            }
        }
        sp = (rt_ubase_t)stack_point1;
    }

    depth = 0;
    for (; sp < stack_addr + stack_size; sp += sizeof(rt_ubase_t)) {
        pc  = *((rt_ubase_t *)sp);
        if((pc >= code_start_addr) && (pc <= code_end_addr) && (depth < CMB_CALL_STACK_MAX_DEPTH)) {
            printf("0x%08lx\n", pc);
             depth++;
        }
    }
}
#endif  // defined(KERNEL_RTTHREAD) && defined(AIC_BACKTRACE_DEBUG)
