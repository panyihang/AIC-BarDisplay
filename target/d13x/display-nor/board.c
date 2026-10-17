/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <ram_param.h>
#include <rtconfig.h>
#include "board.h"

extern void aic_board_pinmux_init(void);
extern void aic_board_sysclk_init(void);

#if defined(AIC_PANEL_DSI_ICNL9707_TXW935003B) || \
    defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B)
static void aic_board_panel_gpio_init(void)
{
    static const char *gpios[] = {
#ifdef AIC_PANEL_DSI_ICNL9707_TXW935003B
        AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO,
        AIC_PANEL_ENABLE_GPIO,
        AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO,
#elif defined(AIC_PANEL_DSI_FL7707_AV092K5U_B)
        AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO,
        AIC_PANEL_ENABLE_GPIO,
        AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO,
#else
        AIC_PANEL_FL7707_BOE1126_B_PEN_GPIO,
        AIC_PANEL_ENABLE_GPIO,
        AIC_PANEL_FL7707_BOE1126_B_RESET_GPIO,
#endif
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(gpios); i++) {
        long pin = hal_gpio_name2pin(gpios[i]);
        unsigned int group;
        unsigned int offset;

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
struct aic_memheap {
    aic_mem_region_t type;
    char *name;
    void *begin_addr;
    void *end_addr;
    struct rt_memheap heap;
    struct rt_mutex lock;
};

struct aic_memheap aic_memheaps[] = {
#ifdef AIC_TCM_EN
    {MEM_ITCM, "heap_itcm", (void *)&__itcm_heap_start,
     (void *)&__itcm_heap_end},
    {MEM_DTCM, "heap_dtcm", (void *)&__dtcm_heap_start,
     (void *)&__dtcm_heap_end},
#endif
#if defined(AIC_SRAM0_SW_EN) && !defined(AIC_DEFAULT_SYS_HEAP_SRAM)
    {MEM_SRAM0_SW, "heap_sram0_sw", (void *)&__sram_s0_sw_heap_start,
     (void *)&__sram_s0_sw_heap_end},
#endif
#ifdef AIC_SRAM1_SW_EN
    {MEM_SRAM1_SW, "heap_sram1_sw", (void *)&__sram_s1_sw_heap_start,
     (void *)&__sram_s1_sw_heap_end},
#endif
#if defined(AIC_PSRAM_SW_EN) && !defined(AIC_DEFAULT_SYS_HEAP_PSRAM)
    {MEM_PSRAM_SW, "heap_psram_sw", (void *)&__psram_sw_heap_start,
     (void *)&__psram_sw_heap_end},
#endif
#if defined(AIC_PSRAM_CMA_EN) || defined(AIC_SRAM1_CMA_EN)
    {MEM_CMA, "heap_cma", (void *)&__cma_heap_start,
     (void *)&__cma_heap_end},
#endif
};

void aic_memheap_init(void)
{
    rt_ubase_t begin_align;
    rt_ubase_t end_align;
    int i;

    for (i = 0; i < ARRAY_SIZE(aic_memheaps); i++) {
#if AIC_PSRAM_SIZE && defined(AIC_PSRAM_SW_EN) && \
    !defined(AIC_DEFAULT_SYS_HEAP_PSRAM)
        if (aic_memheaps[i].type == MEM_PSRAM_SW)
            aic_memheaps[i].end_addr += aic_get_ram_size() - AIC_PSRAM_SIZE;
#endif
        begin_align = RT_ALIGN((rt_ubase_t)aic_memheaps[i].begin_addr,
                               RT_ALIGN_SIZE);
        end_align = RT_ALIGN_DOWN((rt_ubase_t)aic_memheaps[i].end_addr,
                                  RT_ALIGN_SIZE);
        RT_ASSERT(end_align > begin_align);
        rt_memheap_init(&aic_memheaps[i].heap, aic_memheaps[i].name,
                        (void *)begin_align, end_align - begin_align);
        rt_mutex_init(&aic_memheaps[i].lock, aic_memheaps[i].name,
                      RT_IPC_FLAG_PRIO);
    }
}

void *aic_memheap_malloc(int type, size_t size)
{
    int i;
    void *ptr;

    for (i = 0; i < ARRAY_SIZE(aic_memheaps); i++) {
        if (aic_memheaps[i].type == type)
            break;
    }
    if (i == ARRAY_SIZE(aic_memheaps))
        return NULL;

    rt_mutex_take(&aic_memheaps[i].lock, RT_WAITING_FOREVER);
    ptr = rt_memheap_alloc(&aic_memheaps[i].heap, size);
    rt_mutex_release(&aic_memheaps[i].lock);
    return ptr;
}

void aic_memheap_free(int type, void *rmem)
{
    int i;

    if (!rmem)
        return;
    for (i = 0; i < ARRAY_SIZE(aic_memheaps); i++) {
        if (aic_memheaps[i].type == type)
            break;
    }
    if (i == ARRAY_SIZE(aic_memheaps))
        return;

    rt_mutex_take(&aic_memheaps[i].lock, RT_WAITING_FOREVER);
    rt_memheap_free(rmem);
    rt_mutex_release(&aic_memheaps[i].lock);
}

u32 aic_get_sys_heap_size(void)
{
#if AIC_PSRAM_SIZE && defined(AIC_PSRAM_SW_EN) && \
    defined(AIC_DEFAULT_SYS_HEAP_PSRAM)
    if (aic_get_ram_size() > AIC_PSRAM_SIZE)
        return aic_get_ram_size() - AIC_PSRAM_SIZE;
#endif
    return 0;
}
#endif

void rt_hw_board_init(void)
{
    aic_board_sysclk_init();
    aic_board_pinmux_init();

#if defined(AIC_PANEL_DSI_ICNL9707_TXW935003B) || \
    defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B)
    aic_board_panel_gpio_init();
#endif

#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)&__heap_start,
                        (void *)&__heap_end + aic_get_sys_heap_size());
#if !defined(QEMU_RUN) && defined(RT_USING_MEMHEAP)
    aic_memheap_init();
#endif
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}

#elif defined(KERNEL_BAREMETAL)
#include <aic_tlsf.h>

void aic_hw_board_init(void)
{
#ifdef TLSF_MEM_HEAP
    aic_tlsf_heap_init();
#endif
    aic_board_sysclk_init();
    aic_board_pinmux_init();
#if defined(AIC_PANEL_DSI_ICNL9707_TXW935003B) || \
    defined(AIC_PANEL_DSI_FL7707_AV092K5U_B) || \
    defined(AIC_PANEL_DSI_FL7707_BOE1126_B)
    aic_board_panel_gpio_init();
#endif
}
#endif

#ifdef RT_USING_DFS_MNTTABLE
#include <dfs_fs.h>
#ifdef RT_USING_DFS_ROMFS
#include "dfs_romfs.h"
static const struct romfs_dirent mountpoint_root[] = {
    {ROMFS_DIRENT_DIR, "ram", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "data", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "rodata", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "sdcard", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "udisk", RT_NULL, 0},
};
const struct romfs_dirent romfs_root = {
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)mountpoint_root,
    ARRAY_SIZE(mountpoint_root)
};
#endif

const struct dfs_mount_tbl mount_table[] = {
#ifdef RT_USING_DFS_ROMFS
    {RT_NULL, "/", "rom", 0, &romfs_root, DFS_MOUNT_PRIO_EARLY},
#endif
#if defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_0) || \
    defined(AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_1)
    {"blk_rodata", "/rodata", "elm", MS_RDONLY, 0, DFS_MOUNT_PRIO_EARLY},
#endif
#ifdef LPKG_USING_LITTLEFS
    {"data", "/data", "lfs", 0, 0, DFS_MOUNT_PRIO_EARLY},
#endif
    {0}
};
#endif

void show_board_version(void)
{
    printf("Board: %s\n\n", PRJ_BOARD);
}
