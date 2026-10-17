/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include "cpuport.h"

/* Compat RT-Thread 4.x uses direct members, 5.2.x nests them */
#if RT_VERSION_MAJOR >= 5
#define THREAD_NAME(t)   ((t)->parent.name)
#define THREAD_STAT(t)   ((t)->sched_thread_ctx.stat)
#else
#define THREAD_NAME(t)   ((t)->name)
#define THREAD_STAT(t)   ((t)->stat)
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>

#ifdef ARCH_RISCV64
#define PRINT_FRAME_REG(reg) rt_kprintf("%s : 0x%08x\t", #reg, (int)frame->reg);

struct thread_stack_frame
{
    rt_ubase_t mepc;        /* epc - epc    - program counter                     */
    rt_ubase_t ra;         /* x1  - ra     - return address for jumps            */
    rt_ubase_t mstatus;    /*              - machine status register             */
    rt_ubase_t gp;         /* x3  - gp     - global pointer                      */
    rt_ubase_t tp;         /* x4  - tp     - thread pointer                      */
    rt_ubase_t t0;         /* x5  - t0     - temporary register 0                */
    rt_ubase_t t1;         /* x6  - t1     - temporary register 1                */
    rt_ubase_t t2;         /* x7  - t2     - temporary register 2                */
    rt_ubase_t s0_fp;      /* x8  - s0/fp  - saved register 0 or frame pointer   */
    rt_ubase_t s1;         /* x9  - s1     - saved register 1                    */
    rt_ubase_t a0;         /* x10 - a0     - return value or function argument 0 */
    rt_ubase_t a1;         /* x11 - a1     - return value or function argument 1 */
    rt_ubase_t a2;         /* x12 - a2     - function argument 2                 */
    rt_ubase_t a3;         /* x13 - a3     - function argument 3                 */
    rt_ubase_t a4;         /* x14 - a4     - function argument 4                 */
    rt_ubase_t a5;         /* x15 - a5     - function argument 5                 */
    rt_ubase_t a6;         /* x16 - a6     - function argument 6                 */
    rt_ubase_t a7;         /* x17 - s7     - function argument 7                 */
    rt_ubase_t s2;         /* x18 - s2     - saved register 2                    */
    rt_ubase_t s3;         /* x19 - s3     - saved register 3                    */
    rt_ubase_t s4;         /* x20 - s4     - saved register 4                    */
    rt_ubase_t s5;         /* x21 - s5     - saved register 5                    */
    rt_ubase_t s6;         /* x22 - s6     - saved register 6                    */
    rt_ubase_t s7;         /* x23 - s7     - saved register 7                    */
    rt_ubase_t s8;         /* x24 - s8     - saved register 8                    */
    rt_ubase_t s9;         /* x25 - s9     - saved register 9                    */
    rt_ubase_t s10;        /* x26 - s10    - saved register 10                   */
    rt_ubase_t s11;        /* x27 - s11    - saved register 11                   */
    rt_ubase_t t3;         /* x28 - t3     - temporary register 3                */
    rt_ubase_t t4;         /* x29 - t4     - temporary register 4                */
    rt_ubase_t t5;         /* x30 - t5     - temporary register 5                */
    rt_ubase_t t6;         /* x31 - t6     - temporary register 6                */
#ifdef ARCH_RISCV_DSP
    rt_ubase_t vxsat;      /* P-ext vxsat reg */
#endif /* ARCH_RISCV_DSP */
#ifdef ARCH_RISCV_FPU
    rv_floatreg_t f0;      /* f0  */
    rv_floatreg_t f1;      /* f1  */
    rv_floatreg_t f2;      /* f2  */
    rv_floatreg_t f3;      /* f3  */
    rv_floatreg_t f4;      /* f4  */
    rv_floatreg_t f5;      /* f5  */
    rv_floatreg_t f6;      /* f6  */
    rv_floatreg_t f7;      /* f7  */
    rv_floatreg_t f8;      /* f8  */
    rv_floatreg_t f9;      /* f9  */
    rv_floatreg_t f10;     /* f10 */
    rv_floatreg_t f11;     /* f11 */
    rv_floatreg_t f12;     /* f12 */
    rv_floatreg_t f13;     /* f13 */
    rv_floatreg_t f14;     /* f14 */
    rv_floatreg_t f15;     /* f15 */
    rv_floatreg_t f16;     /* f16 */
    rv_floatreg_t f17;     /* f17 */
    rv_floatreg_t f18;     /* f18 */
    rv_floatreg_t f19;     /* f19 */
    rv_floatreg_t f20;     /* f20 */
    rv_floatreg_t f21;     /* f21 */
    rv_floatreg_t f22;     /* f22 */
    rv_floatreg_t f23;     /* f23 */
    rv_floatreg_t f24;     /* f24 */
    rv_floatreg_t f25;     /* f25 */
    rv_floatreg_t f26;     /* f26 */
    rv_floatreg_t f27;     /* f27 */
    rv_floatreg_t f28;     /* f28 */
    rv_floatreg_t f29;     /* f29 */
    rv_floatreg_t f30;     /* f30 */
    rv_floatreg_t f31;     /* f31 */
#endif /* ARCH_RISCV_FPU */
};

#else /* !ARCH_RISCV64 */

#define PRINT_FRAME_REG(reg) rt_kprintf("%s   : %08lx\t", #reg, frame->reg);

struct thread_stack_frame
{
    rt_ubase_t mepc;        /* epc - epc    - program counter                     */
    rt_ubase_t ra;         /* x1  - ra     - return address for jumps            */
    rt_ubase_t mstatus;    /*              - machine status register             */
    rt_ubase_t gp;         /* x3  - gp     - global pointer                      */
    rt_ubase_t tp;         /* x4  - tp     - thread pointer                      */
    rt_ubase_t t0;         /* x5  - t0     - temporary register 0                */
    rt_ubase_t t1;         /* x6  - t1     - temporary register 1                */
    rt_ubase_t t2;         /* x7  - t2     - temporary register 2                */
    rt_ubase_t s0_fp;      /* x8  - s0/fp  - saved register 0 or frame pointer   */
    rt_ubase_t s1;         /* x9  - s1     - saved register 1                    */
    rt_ubase_t a0;         /* x10 - a0     - return value or function argument 0 */
    rt_ubase_t a1;         /* x11 - a1     - return value or function argument 1 */
    rt_ubase_t a2;         /* x12 - a2     - function argument 2                 */
    rt_ubase_t a3;         /* x13 - a3     - function argument 3                 */
    rt_ubase_t a4;         /* x14 - a4     - function argument 4                 */
    rt_ubase_t a5;         /* x15 - a5     - function argument 5                 */
    rt_ubase_t a6;         /* x16 - a6     - function argument 6                 */
    rt_ubase_t a7;         /* x17 - s7     - function argument 7                 */
    rt_ubase_t s2;         /* x18 - s2     - saved register 2                    */
    rt_ubase_t s3;         /* x19 - s3     - saved register 3                    */
    rt_ubase_t s4;         /* x20 - s4     - saved register 4                    */
    rt_ubase_t s5;         /* x21 - s5     - saved register 5                    */
    rt_ubase_t s6;         /* x22 - s6     - saved register 6                    */
    rt_ubase_t s7;         /* x23 - s7     - saved register 7                    */
    rt_ubase_t s8;         /* x24 - s8     - saved register 8                    */
    rt_ubase_t s9;         /* x25 - s9     - saved register 9                    */
    rt_ubase_t s10;        /* x26 - s10    - saved register 10                   */
    rt_ubase_t s11;        /* x27 - s11    - saved register 11                   */
    rt_ubase_t t3;         /* x28 - t3     - temporary register 3                */
    rt_ubase_t t4;         /* x29 - t4     - temporary register 4                */
    rt_ubase_t t5;         /* x30 - t5     - temporary register 5                */
    rt_ubase_t t6;         /* x31 - t6     - temporary register 6                */
#ifdef ARCH_RISCV_DSP
    rt_ubase_t vxsat;      /* P-ext vxsat reg */
    rt_ubase_t reserve;    /* for 8 bytes align */
#endif /* ARCH_RISCV_DSP */
#ifdef ARCH_RISCV_FPU
    rv_floatreg_t f0;      /* f0  */
    rv_floatreg_t f1;      /* f1  */
    rv_floatreg_t f2;      /* f2  */
    rv_floatreg_t f3;      /* f3  */
    rv_floatreg_t f4;      /* f4  */
    rv_floatreg_t f5;      /* f5  */
    rv_floatreg_t f6;      /* f6  */
    rv_floatreg_t f7;      /* f7  */
    rv_floatreg_t f8;      /* f8  */
    rv_floatreg_t f9;      /* f9  */
    rv_floatreg_t f10;     /* f10 */
    rv_floatreg_t f11;     /* f11 */
    rv_floatreg_t f12;     /* f12 */
    rv_floatreg_t f13;     /* f13 */
    rv_floatreg_t f14;     /* f14 */
    rv_floatreg_t f15;     /* f15 */
    rv_floatreg_t f16;     /* f16 */
    rv_floatreg_t f17;     /* f17 */
    rv_floatreg_t f18;     /* f18 */
    rv_floatreg_t f19;     /* f19 */
    rv_floatreg_t f20;     /* f20 */
    rv_floatreg_t f21;     /* f21 */
    rv_floatreg_t f22;     /* f22 */
    rv_floatreg_t f23;     /* f23 */
    rv_floatreg_t f24;     /* f24 */
    rv_floatreg_t f25;     /* f25 */
    rv_floatreg_t f26;     /* f26 */
    rv_floatreg_t f27;     /* f27 */
    rv_floatreg_t f28;     /* f28 */
    rv_floatreg_t f29;     /* f29 */
    rv_floatreg_t f30;     /* f30 */
    rv_floatreg_t f31;     /* f31 */
#endif /* ARCH_RISCV_FPU */
};
#endif /* ARCH_RISCV64 */

static void print_reg_stack_frame(struct thread_stack_frame *frame)
{
    rt_kprintf("\n");
    rt_kprintf("CPU Exception: NO.0, Just for aligned. Ignore...\n");
    PRINT_FRAME_REG(tp);         /* x4  - tp     - thread pointer                      */
    PRINT_FRAME_REG(t0);         /* x5  - t0     - temporary register 0                */
    PRINT_FRAME_REG(t1);         /* x6  - t1     - temporary register 1                */
    PRINT_FRAME_REG(t2);         /* x7  - t2     - temporary register 2                */
    rt_kprintf("\n");
    PRINT_FRAME_REG(s0_fp);      /* x8  - s0/fp  - saved register 0 or frame pointer   */
    PRINT_FRAME_REG(s1);         /* x9  - s1     - saved register 1                    */
    PRINT_FRAME_REG(a0);         /* x10 - a0     - return value or function argument 0 */
    PRINT_FRAME_REG(a1);         /* x11 - a1     - return value or function argument 1 */
    rt_kprintf("\n");
    PRINT_FRAME_REG(a2);         /* x12 - a2     - function argument 2                 */
    PRINT_FRAME_REG(a3);         /* x13 - a3     - function argument 3                 */
    PRINT_FRAME_REG(a4);         /* x14 - a4     - function argument 4                 */
    PRINT_FRAME_REG(a5);         /* x15 - a5     - function argument 5                 */
    rt_kprintf("\n");
    PRINT_FRAME_REG(a6);         /* x16 - a6     - function argument 6                 */
    PRINT_FRAME_REG(a7);         /* x17 - s7     - function argument 7                 */
    PRINT_FRAME_REG(s2);         /* x18 - s2     - saved register 2                    */
    PRINT_FRAME_REG(s3);         /* x19 - s3     - saved register 3                    */
    rt_kprintf("\n");
    PRINT_FRAME_REG(s4);         /* x20 - s4     - saved register 4                    */
    PRINT_FRAME_REG(s5);         /* x21 - s5     - saved register 5                    */
    PRINT_FRAME_REG(s6);         /* x22 - s6     - saved register 6                    */
    PRINT_FRAME_REG(s7);         /* x23 - s7     - saved register 7                    */
    rt_kprintf("\n");
    PRINT_FRAME_REG(s8);         /* x24 - s8     - saved register 8                    */
    PRINT_FRAME_REG(s9);         /* x25 - s9     - saved register 9                    */
    PRINT_FRAME_REG(s10);        /* x26 - s10    - saved register 10                   */
    PRINT_FRAME_REG(s11);        /* x27 - s11    - saved register 11                   */
    rt_kprintf("\n");
    PRINT_FRAME_REG(t3);         /* x28 - t3     - temporary register 3                */
    PRINT_FRAME_REG(t4);         /* x29 - t4     - temporary register 4                */
    PRINT_FRAME_REG(t5);         /* x30 - t5     - temporary register 5                */
    PRINT_FRAME_REG(t6);         /* x31 - t6     - temporary register 6                */
    rt_kprintf("\n");
    PRINT_FRAME_REG(ra);         /* x1  - ra     - return address for jumps            */
    PRINT_FRAME_REG(gp);         /* x3  - gp     - global pointer                      */
    rt_kprintf("\n");
    PRINT_FRAME_REG(mepc);        /* epc - epc    - program counter                     */
    rt_kprintf("\n");
    PRINT_FRAME_REG(mstatus);    /*              - machine status register             */
    rt_kprintf("\n\n");

#if 0
#ifdef ARCH_RISCV_DSP
    PRINT_FRAME_REG(vxsat);      /* P-ext vxsat reg */
#endif
#ifdef ARCH_RISCV_FPU
    PRINT_FRAME_REG(f0);      /* f0  */
    PRINT_FRAME_REG(f1);      /* f1  */
    PRINT_FRAME_REG(f2);      /* f2  */
    PRINT_FRAME_REG(f3);      /* f3  */
    PRINT_FRAME_REG(f4);      /* f4  */
    PRINT_FRAME_REG(f5);      /* f5  */
    PRINT_FRAME_REG(f6);      /* f6  */
    PRINT_FRAME_REG(f7);      /* f7  */
    PRINT_FRAME_REG(f8);      /* f8  */
    PRINT_FRAME_REG(f9);      /* f9  */
    PRINT_FRAME_REG(f10);     /* f10 */
    PRINT_FRAME_REG(f11);     /* f11 */
    PRINT_FRAME_REG(f12);     /* f12 */
    PRINT_FRAME_REG(f13);     /* f13 */
    PRINT_FRAME_REG(f14);     /* f14 */
    PRINT_FRAME_REG(f15);     /* f15 */
    PRINT_FRAME_REG(f16);     /* f16 */
    PRINT_FRAME_REG(f17);     /* f17 */
    PRINT_FRAME_REG(f18);     /* f18 */
    PRINT_FRAME_REG(f19);     /* f19 */
    PRINT_FRAME_REG(f20);     /* f20 */
    PRINT_FRAME_REG(f21);     /* f21 */
    PRINT_FRAME_REG(f22);     /* f22 */
    PRINT_FRAME_REG(f23);     /* f23 */
    PRINT_FRAME_REG(f24);     /* f24 */
    PRINT_FRAME_REG(f25);     /* f25 */
    PRINT_FRAME_REG(f26);     /* f26 */
    PRINT_FRAME_REG(f27);     /* f27 */
    PRINT_FRAME_REG(f28);     /* f28 */
    PRINT_FRAME_REG(f29);     /* f29 */
    PRINT_FRAME_REG(f30);     /* f30 */
    PRINT_FRAME_REG(f31);     /* f31 */
#endif
#endif
}

#define LIST_FIND_OBJ_NR 8

typedef struct
{
    rt_list_t *list;
    rt_list_t **array;
    rt_uint8_t type;
    int nr;             /* input: max nr, can't be 0 */
    int nr_out;         /* out: got nr */
} list_get_next_t;

static void list_find_init(list_get_next_t *p, rt_uint8_t type, rt_list_t **array, int nr)
{
    struct rt_object_information *info;
    rt_list_t *list;

    info = rt_object_get_information((enum rt_object_class_type)type);
    list = &info->object_list;

    p->list = list;
    p->type = type;
    p->array = array;
    p->nr = nr;
    p->nr_out = 0;
}

static rt_list_t *list_get_next(rt_list_t *current, list_get_next_t *arg)
{
    int first_flag = 0;
    rt_base_t level;
    rt_list_t *node, *list;
    rt_list_t **array;
    int nr;

    arg->nr_out = 0;

    if (!arg->nr || !arg->type)
    {
        return (rt_list_t *)RT_NULL;
    }

    list = arg->list;

    if (!current) /* find first */
    {
        node = list;
        first_flag = 1;
    }
    else
    {
        node = current;
    }

    level = rt_hw_interrupt_disable();

    if (!first_flag)
    {
        struct rt_object *obj;
        /* The node in the list? */
        obj = rt_list_entry(node, struct rt_object, list);
        if ((obj->type & ~RT_Object_Class_Static) != arg->type)
        {
            rt_hw_interrupt_enable(level);
            return (rt_list_t *)RT_NULL;
        }
    }

    nr = 0;
    array = arg->array;
    while (1)
    {
        node = node->next;

        if (node == list)
        {
            node = (rt_list_t *)RT_NULL;
            break;
        }
        nr++;
        *array++ = node;
        if (nr == arg->nr)
        {
            break;
        }
    }

    rt_hw_interrupt_enable(level);
    arg->nr_out = nr;
    return node;
}

long trace_thread_stack(int argc, char *argv[])
{
    rt_list_t *next = (rt_list_t *)RT_NULL;
    rt_list_t *obj_list[LIST_FIND_OBJ_NR];
    struct thread_stack_frame *frame;
    list_get_next_t find_arg;
    rt_uint32_t *stack_end = NULL;
    rt_uint8_t stat = 0;
    rt_uint32_t *sp = NULL;
    rt_base_t level = 0;
    int i = 0, j = 0;

    if (argc != 2) {
        rt_kprintf("trace_thread_stack [thread_name]");
        return -1;
    }

    list_find_init(&find_arg, RT_Object_Class_Thread, obj_list, sizeof(obj_list) / sizeof(obj_list[0]));

    do {
        next = list_get_next(next, &find_arg);

        for (i = 0; i < find_arg.nr_out; i++)
        {
            struct rt_object *obj;
            struct rt_thread thread_info, *thread;

            obj = rt_list_entry(obj_list[i], struct rt_object, list);
            thread = (struct rt_thread *)obj;
            if (rt_strncmp(argv[1], THREAD_NAME(thread), (unsigned int)RT_NAME_MAX))
                continue;

            level = rt_hw_interrupt_disable();

            if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
            {
                rt_hw_interrupt_enable(level);
                continue;
            }
            /* copy info */
            rt_memcpy(&thread_info, obj, sizeof thread_info);
            rt_hw_interrupt_enable(level);

            stat = (THREAD_STAT(thread) & RT_THREAD_STAT_MASK);
            if (stat != RT_THREAD_SUSPEND) {
                rt_kprintf("thread: [%s] is not suspend, exit\n", THREAD_NAME(thread));
                continue;
            }

            rt_kprintf("-----------------[%s] thread stack trace-----------------\n", THREAD_NAME(thread));
            frame = thread->sp;
            print_reg_stack_frame(frame);

            sp = (rt_uint32_t *)(sizeof(struct thread_stack_frame) + (rt_ubase_t)frame);
            stack_end = (rt_uint32_t *)(thread->stack_size + (rt_ubase_t)thread->stack_addr);
            rt_kprintf("stack_addr:0x%08x stack_addr_end:0x%08x\n\nstack:\n", \
                        (rt_uint32_t)thread->sp, stack_end);
            for (; sp < stack_end; sp++) {
                if (!((rt_uint32_t)sp % 16))
                    rt_kprintf("\n");
                rt_kprintf("0x%08x ", *sp);
            }
            rt_kprintf("\n----------------------[%s] trace end----------------------\n", THREAD_NAME(thread));
        }

    } while ((next != (rt_list_t *)RT_NULL) && (j++ < RT_THREAD_PRIORITY_MAX));

    return 0;
}
MSH_CMD_EXPORT(trace_thread_stack, trace back thread stack);

#endif /* RT_USING_FINSH */

