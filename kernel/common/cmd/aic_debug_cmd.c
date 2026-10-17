/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#ifdef RT_USING_FINSH
#include <rthw.h>
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#include <finsh.h>

static int cmd_pmem(int argc, char **argv)
{
    int32_t i;
    int32_t nunits = 16;
    int32_t width  = 4;
    uintptr_t addr = 0;

    switch (argc) {
    case 4:
        width = strtoul(argv[3], NULL, 0);
    case 3:
        nunits = strtoul(argv[2], NULL, 0);
        nunits = nunits > 0x100000 ? 0x100000 : nunits;
    case 2:
        addr = strtoul(argv[1], NULL, 0);
        break;
    default:
        rt_kprintf("p <addr> <nunits> <width>\n");
        rt_kprintf("addr  : address to display\n");
        rt_kprintf("nunits: number of units to display (default is 16)\n");
        rt_kprintf("width : width of unit, 1/2/4 (default is 4)\n");
        return 0;
    }

    switch (width) {
    case 1:
        for (i = 0; i < nunits; i++) {
            if (i % 16 == 0) {
                rt_kprintf("%08p:", addr);
            }
            rt_kprintf(" %02x", *(unsigned char *)addr);
            addr += 1;
            if (i % 16 == 15) {
                rt_kprintf("\n");
            }
        }
        if (i % 16 != 0)
            rt_kprintf("\n");
        break;
    case 2:
        for (i = 0; i < nunits; i++) {
            if (i % 8 == 0) {
                rt_kprintf("%08p:", addr);
            }
            rt_kprintf(" %04x", *(unsigned short *)addr);
            addr += 2;
            if (i % 8 == 7) {
                rt_kprintf("\n");
            }
        }
        if (i % 8 != 0)
            rt_kprintf("\n");
        break;
    default:
        for (i = 0; i < nunits; i++) {
            if (i % 4 == 0) {
                rt_kprintf("%08p:", addr);
            }
            rt_kprintf(" %08x", *(unsigned int *)addr);
            addr += 4;
            if (i % 4 == 3) {
                rt_kprintf("\n");
            }
        }
        if (i % 4 != 0)
            rt_kprintf("\n");
        break;
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_pmem, p, print memory);

static int cmd_mmem(int argc, char **argv)
{
    void *addr  = NULL;
    int32_t  width = 4;
    uint32_t value = 0;
    uint32_t old_value;
    uint32_t new_value;

    switch (argc) {
        case 4:
            width = strtoul(argv[3], NULL, 0);
        case 3:
            value = strtoul(argv[2], NULL, 0);
        case 2:
            addr = (void *)strtoul(argv[1], NULL, 0);
            break;
        default:
            rt_kprintf("m <addr> <value> <width>\n");
            rt_kprintf("addr  : address to modify\n");
            rt_kprintf("value : new value (default is 0)\n");
            rt_kprintf("width : width of unit, 1/2/4 (default is 4)\n");
            return 0;
    }

    switch (width) {
        case 1:
            old_value = (uint32_t)(*(uint8_t volatile *)addr);
            *(uint8_t volatile *)addr = (uint8_t)value;
            new_value = (uint32_t)(*(uint8_t volatile *)addr);
            break;
        case 2:
            old_value = (uint32_t)(*(unsigned short volatile *)addr);
            *(unsigned short volatile *)addr = (unsigned short)value;
            new_value = (uint32_t)(*(unsigned short volatile *)addr);
            break;
        case 4:
        default:
            old_value = *(uint32_t volatile *)addr;
            *(uint32_t volatile *)addr = (uint32_t)value;
            new_value = *(uint32_t volatile *)addr;
            break;
    }
    rt_kprintf("value on %p change from 0x%x to 0x%x.\n", addr, old_value, new_value);

    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_mmem, m, modify memory);

static int cmd_func(int argc, char **argv)
{
    uint32_t idx, ret;
    uint32_t para[8] = {0};
    typedef uint32_t (*func_ptr_t)(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                                   uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8);
    func_ptr_t func_ptr;

    if (argc == 1) {
        rt_kprintf("f <func> <para0> <para1> ... \n");
        rt_kprintf("func  : address of function\n");
        rt_kprintf("paraN : parameter of function\n");
        return 0;
    }

    argc = argc > 10 ? 10 : argc;

    func_ptr = (func_ptr_t)strtoul(argv[1], NULL, 0);
    for (idx = 2 ; idx < argc ; idx++) {
        para[idx - 2] = strtoul(argv[idx], NULL, 0);
    }

    rt_kprintf("function %p runing...\n", func_ptr);
    ret = func_ptr(para[0], para[1], para[2], para[3], para[4], para[5], para[6], para[7]);
    rt_kprintf("function %p return 0x%x.\n", func_ptr, ret);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_func, f, run a function);

#ifdef QEMU_RUN
static int cmd_qemu_exit(int argc, char **argv)
{
    *(int *)0x10002000 = 88;

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_qemu_exit, exit, exit qemu);
#endif /* QEMU_RUN */
#endif /* RT_USING_FINSH */
