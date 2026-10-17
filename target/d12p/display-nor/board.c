/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: weilin.peng@artinchip.com
 */
#include <aic_core.h>
#include <aic_hal.h>
#include <ram_param.h>
#include <rtconfig.h>
#include "board.h"

extern void aic_board_pinmux_init(void);
extern void aic_board_sysclk_init(void);

#if defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B) || \
    defined(AIC_PANEL_DSI_ICNL9707_TXW935003B)
static void aic_board_panel_gpio_init(void)
{
    static const char *gpios[] = {
#ifdef AIC_PANEL_DSI_FL7707_AV092K5U_B
        AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO,
        AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO,
#elif defined(AIC_PANEL_DSI_FL7707_BOE1126_B)
        AIC_PANEL_FL7707_BOE1126_B_PEN_GPIO,
        AIC_PANEL_FL7707_BOE1126_B_RESET_GPIO,
#else
        AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO,
        AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO,
#endif
#ifdef AIC_PANEL_ENABLE_GPIO
        AIC_PANEL_ENABLE_GPIO,
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(gpios); i++) {
        long pin = hal_gpio_name2pin(gpios[i]);
        unsigned int group, offset;

        if (pin < 0)
            continue;
        group = GPIO_GROUP(pin);
        offset = GPIO_GROUP_PIN(pin);
        hal_gpio_set_func(group, offset, 1);
        hal_gpio_set_bias_pull(group, offset, PIN_PULL_DIS);
        hal_gpio_direction_output(group, offset);
        hal_gpio_clr_output(group, offset);
    }
}
#endif

#if defined(KERNEL_RTTHREAD)
#include <aic_drv.h>
#include <rthw.h>
#include <rtthread.h>

extern size_t __heap_start;
extern size_t __heap_end;

#ifdef RT_USING_MEMHEAP
extern size_t __psram_cma_heap_start;
extern size_t __psram_cma_heap_end;

struct aic_memheap
{
    aic_mem_region_t    type;
    char *              name;
    void *              begin_addr;
    void *              end_addr;
    struct rt_memheap   heap;
    struct rt_mutex     lock;
};

struct aic_memheap aic_memheaps[] = {
    // 32K sram only used by png decoder after system startup
#ifndef AIC_CHIP_D12P
    #ifdef AIC_SRAM_SW_EN
        {MEM_SRAM_SW, "heap_sram_sw", (void *)&__sram_sw_heap_start, (void *)&__sram_sw_heap_end},
    #endif
    #ifdef AIC_SRAM_CMA_EN
        {MEM_SRAM_CMA, "heap_sram_cma", (void *)&__sram_cma_heap_start, (void *)&__sram_cma_heap_end},
    #endif
#endif
#ifdef AIC_PSRAM_CMA_EN
    {MEM_CMA, "heap_cma", (void *)&__cma_heap_start, (void *)&__cma_heap_end},
#endif
};

void aic_memheap_init(void)
{
    rt_ubase_t begin_align;
    rt_ubase_t end_align;
    int i = 0;

    for (i=0; i<sizeof(aic_memheaps)/sizeof(struct aic_memheap); i++) {
        begin_align = RT_ALIGN((rt_ubase_t)aic_memheaps[i].begin_addr, RT_ALIGN_SIZE);
        end_align   = RT_ALIGN_DOWN((rt_ubase_t)aic_memheaps[i].end_addr, RT_ALIGN_SIZE);
        RT_ASSERT(end_align > begin_align);

        rt_memheap_init(&aic_memheaps[i].heap, aic_memheaps[i].name,
                        (void *)begin_align, end_align - begin_align);
        rt_mutex_init(&aic_memheaps[i].lock, aic_memheaps[i].name, RT_IPC_FLAG_PRIO);
    }
}

void *aic_memheap_malloc(int type, size_t size)
{
    void *ptr;
    int i = 0;

    for (i=0; i<sizeof(aic_memheaps)/sizeof(struct aic_memheap); i++) {
        if (aic_memheaps[i].type == type)
            break;
    }
    if (i >= sizeof(aic_memheaps)/sizeof(struct aic_memheap))
        return NULL;

    /* Enter critical zone */
    rt_mutex_take(&aic_memheaps[i].lock, RT_WAITING_FOREVER);
    /* allocate memory block from system heap */
    ptr = rt_memheap_alloc(&aic_memheaps[i].heap, size);
    /* Exit critical zone */
    rt_mutex_release(&aic_memheaps[i].lock);

    return ptr;
}

void aic_memheap_free(int type, void *rmem)
{
    int i = 0;

    if (rmem == RT_NULL)
        return;

    for (i=0; i<sizeof(aic_memheaps)/sizeof(struct aic_memheap); i++) {
        if (aic_memheaps[i].type == type)
            break;
    }
    if (i >= sizeof(aic_memheaps)/sizeof(struct aic_memheap))
        return;

    /* Enter critical zone */
    rt_mutex_take(&aic_memheaps[i].lock, RT_WAITING_FOREVER);
    rt_memheap_free(rmem);
    /* Exit critical zone */
    rt_mutex_release(&aic_memheaps[i].lock);
}

u32 aic_get_sys_heap_size(void)
{
#if AIC_PSRAM_SIZE
#ifdef AIC_PSRAM_SW_EN
    if (aic_get_ram_size() > AIC_PSRAM_SIZE)
        return aic_get_ram_size() - AIC_PSRAM_SIZE;
#endif
#endif
    return 0;
}
#endif

/**
 * This function will initial smart-evb board.
 */
void rt_hw_board_init(void)
{
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)&__heap_start, (void *)((u8 *)&__heap_end + aic_get_sys_heap_size()));
#if (!defined(QEMU_RUN) && defined(RT_USING_MEMHEAP))
    aic_memheap_init();
#endif
#endif

    aic_board_sysclk_init();
    aic_board_pinmux_init();

#if defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B) || \
    defined(AIC_PANEL_DSI_ICNL9707_TXW935003B)
    aic_board_panel_gpio_init();
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}

#elif defined(KERNEL_FREERTOS)
#elif defined(KERNEL_BAREMETAL)
#include <aic_tlsf.h>

void aic_hw_board_init(void)
{
#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_init();
#endif

    aic_board_sysclk_init();
    aic_board_pinmux_init();

#if defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B) || \
    defined(AIC_PANEL_DSI_ICNL9707_TXW935003B)
    aic_board_panel_gpio_init();
#endif
}
#endif

#ifdef RT_USING_DFS_MNTTABLE
#include <dfs_fs.h>
/*@}*/
#ifdef RT_USING_DFS_ROMFS
#include "dfs_romfs.h"
static const struct romfs_dirent _mountpoint_root[] =
{
    {ROMFS_DIRENT_DIR, "ram", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "data", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "rodata", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "sdcard", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "udisk", RT_NULL, 0},
};
const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_mountpoint_root, ARRAY_SIZE(_mountpoint_root)
};
#endif
const struct dfs_mount_tbl mount_table[] = {
#ifdef RT_USING_DFS_ROMFS
    {RT_NULL, "/", "rom", 0, &romfs_root, 0},
#endif
#ifdef LPKG_RAMDISK_TYPE_INITDATA
    {"ramdisk0", "/ram", "elm", 0, 0, 0},
#endif
#ifndef AIC_AB_SYSTEM_INTERFACE
#if (defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_0) || defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_1))
    {"blk_rodata", "/rodata", "elm", MS_RDONLY, 0, DFS_MOUNT_PRIO_EARLY},
#endif
#if (defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_1))
    {"blk_data", "/data", "elm", 0, 0, 1},
#endif
#endif
#ifdef LPKG_USING_LITTLEFS
    {"data", "/data", "lfs", 0, 0, 0},
#endif
#ifdef LPKG_USING_DFS_UFFS
    {"data", "/data", "uffs", 0, 0, 1},
#endif
#ifdef AIC_USING_SDMC1
    {"sd1", "/sdcard", "elm", 0, 0, 0},
#endif
#if (defined(AIC_USING_USB0_HOST) || defined(AIC_USING_USB1_HOST))
    {"udisk", "/udisk", "elm", 0, 0, 0xFF},
#endif
    {0}
};
#endif

void show_board_version(void)
{
    printf("Board: %s\n\n", PRJ_BOARD);
}
