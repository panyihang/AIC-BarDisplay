/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <driver.h>
#include <heap.h>
#include <board.h>
#include <uart.h>
#include <console.h>
#include <aic_osal.h>
#include <boot_param.h>
#include <hal_efuse.h>
#include <hal_syscfg.h>
#include <aic_hal.h>
#include <aic_time.h>
#include <boot_time.h>
#include <aic_clk_id.h>
#include <hal_syscfg.h>
#include <ram_param.h>
#include <hal_axicfg.h>
#ifdef AICUPG_LOG_BUFFER_SUPPORT
#include <log_buf.h>
#endif

extern size_t __heap_start;
extern size_t __heap_end;

extern int bl_upgmode_detect(enum boot_device bd);
extern void aic_board_pinmux_init(void);
extern void aic_board_sysclk_init(void);
extern void stdio_set_uart(int id);
extern int drv_dma_init(void);

size_t config_ram_size = 0;

#define MAX_HEAP_SIZE_IN_BOOT 0x800000

#ifdef AIC_BOOTLOADER_AXICFG_SUPPORT
struct hal_axicfg_table axi_cfg_table[HAL_AXICFG_PORT_MAX] = {
    { .enable = AXICFG_CPU_EN, .priority = (u8)AIC_AXICFG_PORT_CPU_PRIO },
    { .enable = AXICFG_AHB_EN, .priority = (u8)AIC_AXICFG_PORT_AHB_PRIO },
#ifdef AIC_CHIP_D12P
    { .enable = AXICFG_AHB1_EN, .priority = (u8)AIC_AXICFG_PORT_AHB1_PRIO },
#endif
    { .enable = AXICFG_DE_EN, .priority = (u8)AIC_AXICFG_PORT_DE_PRIO },
    { .enable = AXICFG_GE_EN, .priority = (u8)AIC_AXICFG_PORT_GE_PRIO },
    { .enable = AXICFG_VE_EN, .priority = (u8)AIC_AXICFG_PORT_VE_PRIO },
    { .enable = AXICFG_DVP_EN, .priority = (u8)AIC_AXICFG_PORT_DVP_PRIO },
#ifndef AIC_CHIP_D12P
    { .enable = AXICFG_CE_EN, .priority = (u8)AIC_AXICFG_PORT_CE_PRIO },
#endif
};
#endif

static void aic_board_heap_init(enum boot_device bd)
{
    size_t real_ram_size = 0;

#if AIC_PSRAM_SIZE
    config_ram_size = AIC_PSRAM_SIZE;
#elif AIC_DRAM_TOTAL_SIZE
    config_ram_size = AIC_DRAM_TOTAL_SIZE;
#elif AIC_SRAM_TOTAL_SIZE
    config_ram_size = AIC_SRAM_TOTAL_SIZE;
#elif AIC_SRAM_SIZE
    config_ram_size = AIC_SRAM_SIZE;
#endif

    real_ram_size = aic_get_ram_size();
    if (config_ram_size != real_ram_size)
        pr_warn("config ram size(0x%x) is not equal real ram size(0x%x)\n", (u32)config_ram_size, (u32)real_ram_size);

    heap_init();
}

static int board_init(enum boot_device bd)
{
    int brate;
    int cons_uart;

    /* target/<chip>/<board>/board.c */
    aic_board_sysclk_init();
    aic_board_pinmux_init();
    boot_time_trace("Clock and pinmux done");


#ifdef AIC_BOOTLOADER_AXICFG_SUPPORT
    for (int i = 0; i < HAL_AXICFG_PORT_MAX; i++) {
        if (axi_cfg_table[i].enable) {
            hal_axicfg_module_init(i, axi_cfg_table[i].priority);
        }
    }
#endif

    aic_board_heap_init(bd);
    boot_time_trace("Heap init done");

    cons_uart = AIC_BOOTLOADER_CONSOLE_UART;
    brate = uart_get_cur_baudrate(cons_uart);
    uart_init(cons_uart);
    uart_reset(cons_uart);
    if (brate > 0)
        uart_config_update(cons_uart, brate);
    stdio_set_uart(cons_uart);
    boot_time_trace("Console UART ready");

    return 0;
}

void show_banner(void)
{
    printf("\ntinySPL [Built on %s %s]\n", __DATE__, __TIME__);
}

int main(void)
{
    enum boot_device bd;
    int ctrlc = -1, cont_boot = 1, time_start;

    boot_time_trace("Enter main");

#ifdef AIC_SID_DRV
    hal_efuse_init();
#endif
#ifdef AIC_SYSCFG_DRV
    hal_syscfg_probe();
#endif
#ifdef AIC_DMA_DRV
    drv_dma_init();
#endif
#ifdef AICUPG_LOG_BUFFER_SUPPORT
    log_buf_init();
#endif

    aicos_local_irq_enable();

    bd = aic_get_boot_device();
    board_init(bd);

    show_banner();
    boot_time_trace("Banner shown");

    console_init();
    console_set_usrname("aic");

    time_start = aic_get_time_ms();
    while (aic_get_time_ms() - time_start < AIC_CONSOLE_CTRLC_TMO) {
        ctrlc = console_get_ctrlc();
        if (ctrlc > 0)
            break;
    }
    if (ctrlc < 0) {
#ifdef AICUPG_SUPPORT
        cont_boot = bl_upgmode_detect(bd);
#endif
        /*
         * Set bootcmd string to console, bootloader will check and run bootcmd
         * first in console_loop
         *
         * For the detail of boot command, please reference to cmd/
         */
        if (cont_boot) {
            switch (bd) {
                case BD_SDMC0:
                case BD_SDMC1:
                    console_set_bootcmd("mmc_boot");
                    break;
                case BD_SPINOR:
#ifdef AIC_BOOTLOADER_CMD_XIP_BOOT
                    console_set_bootcmd("xip_boot");
#elif defined(AIC_BOOTLOADER_CMD_NOR_BOOT)
                    console_set_bootcmd("nor_boot");
#endif
                    break;
                case BD_SPINAND:
                    console_set_bootcmd("nand_boot");
                    break;
                default:
                    break;
            }
        }
    }
    /*
     * In the loop:
     *  1. Bootloader first will try to run the "bootcmd", to boot application
     *  2. If bootloader failure to boot application, it will stay in console
     *     loop, user/developer can run built-in commands to do some jobs.
     */
    int ret = 0;
    while (1) {
        ret = console_loop();
        if (ret == CONSOLE_QUIT) {
            /* Can do some process here */
        }
    }

    return 0;
}
