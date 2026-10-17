/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include "aic_osal.h"
#include "aic_common.h"

#ifdef AIC_CONSOLE_BARE_DRV
#include "console.h"
#endif


#if defined(RT_USING_FINSH) || defined(AIC_CONSOLE_BARE_DRV)
static char *show_size(u32 size)
{
    static char str[32] = "";

    memset(str, 0, 32);
    if (size >= 0x100000) {
        if ((size >> 10) % 0x400)
            snprintf(str, 32, "%d MB %d KB", size >> 20, (size >> 10) % 0x400);
        else
            snprintf(str, 32, "%d MB", size >> 20);
    } else if (size >= 0x400) {
        snprintf(str, 32, "%d KB", size >> 10);
    } else {
        snprintf(str, 32, "%d B", size);
    }
    return str;
}

#define PRINT_MEM_ITEM(name, s, e) \
do { \
    int size = (int)(ptr_t)&e - (int)(ptr_t)&s; \
    if (size) \
        printf("%-16s 0x%08X 0x%08X 0x%08X (%s)\n", name, \
               (int)(ptr_t)&s, (int)(ptr_t)&e, size, show_size(size)); \
    else \
        printf("%-16s %10d %10d %10d \n", name, 0, 0, 0); \
} while (0)

#define PRINT_MEM_ITEM_EX(name, s, e) \
do { \
    extern int s, e; \
    PRINT_MEM_ITEM(name, s, e); \
} while (0)

static int cmd_meminfo(int argc, char **argv)
{
    printf("--------------------------------------------------------------\n");
    printf("Name             Start      End        Size \n");
    printf("---------------- ---------- ---------- -----------------------\n");

#ifdef AIC_DRAM_TOTAL_SIZE
    PRINT_MEM_ITEM_EX("DRAM Total", __dram_start, __dram_end);
#endif
#ifdef AIC_DRAM_CMA_EN
    PRINT_MEM_ITEM("DRAM CMA Heap", __dram_cma_heap_start, __dram_cma_heap_end);
#endif

/* Ignore the baremetal mode,
   because the link script is so simple that some label are undefined */
#ifndef KERNEL_BAREMETAL

#ifdef AIC_TCM_EN
    PRINT_MEM_ITEM_EX("iTCM Total", __itcm_start, __itcm_end);
    PRINT_MEM_ITEM("iTCM Heap", __itcm_heap_start, __itcm_heap_end);
    PRINT_MEM_ITEM_EX("dTCM Total", __dtcm_start, __dtcm_end);
    PRINT_MEM_ITEM("dTCM Heap", __dtcm_heap_start, __dtcm_heap_end);
#endif

#ifdef AIC_SRAM_TOTAL_SIZE
    PRINT_MEM_ITEM_EX("SRAM S0 Total", __sram_s0_start, __sram_s0_end);
#ifdef AIC_SRAM1_SW_EN
    PRINT_MEM_ITEM_EX("SRAM S1 SW", __sram_s1_sw_start, __sram_s1_sw_end);
#endif
#ifdef AIC_SRAM1_CMA_EN
    PRINT_MEM_ITEM_EX("SRAM S1 CMA", __sram_s1_cma_start, __sram_s1_cma_end);
#endif
#endif

#ifdef AIC_SRAM_SIZE
#if (CONFIG_AIC_SRAM_SW_SIZE != 0)
    PRINT_MEM_ITEM("SRAM SW Heap", __sram_sw_heap_start, __sram_sw_heap_end);
#endif
#endif

#ifdef AIC_SRAM1_SW_EN
    PRINT_MEM_ITEM("SRAM1 SW Heap",
                   __sram_s1_sw_heap_start, __sram_s1_sw_heap_end);
#endif
#ifdef AIC_SRAM1_CMA_EN
    PRINT_MEM_ITEM("SRAM1 CMA Heap",
                   __sram_s1_cma_heap_start, __sram_s1_cma_heap_end);
#endif

#ifdef AIC_PSRAM_SIZE
    PRINT_MEM_ITEM_EX("PSRAM Total", __psram_start, __psram_end);
#endif
#ifdef AIC_PSRAM_SW_EN
    PRINT_MEM_ITEM_EX("PSRAM SW", __psram_sw_start, __psram_sw_end);
    PRINT_MEM_ITEM("PSRAM SW Heap",
                   __psram_sw_heap_start, __psram_sw_heap_end);
#endif
#ifdef AIC_PSRAM_CMA_EN
    PRINT_MEM_ITEM_EX("PSRAM CMA", __psram_cma_start, __psram_cma_end);
    PRINT_MEM_ITEM("PSRAM CMA Heap",
                   __psram_cma_heap_start, __psram_cma_heap_end);
#endif

#ifdef AIC_DRAM_CMA_EN
    PRINT_MEM_ITEM("CMA Heap", __cma_heap_start, __cma_heap_end);
#endif

#endif // end of ifndef KERNEL_BAREMETAL

    PRINT_MEM_ITEM("Heap", __heap_start, __heap_end);
    PRINT_MEM_ITEM_EX("Text section", __stext, __etext);
    PRINT_MEM_ITEM_EX("RO Data section", __srodata, __erodata);
    PRINT_MEM_ITEM_EX("Data section", __sdata, __edata);
    PRINT_MEM_ITEM_EX("BSS section", __sbss, __ebss);

    return 0;
}
#endif
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT_ALIAS(cmd_meminfo, meminfo, Show the memory information);
#endif
#ifdef AIC_CONSOLE_BARE_DRV
CONSOLE_CMD(meminfo, cmd_meminfo, "Show the memory information");
#endif
