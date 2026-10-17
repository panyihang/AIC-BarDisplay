#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Project options */

#define PRJ_DEFCONFIG_FILENAME "d12p_display-nor_baremetal_bootloader_defconfig"

/* -- Important: If following options have been changed, you need save & rerun menuconfig before changing any other options. */

#define PRJ_CHIP "d12p"
#define PRJ_BOARD "display-nor"
#define PRJ_KERNEL "baremetal"
#define PRJ_APP "bootloader"
#define AIC_IMAGE_PRODUCT ""
#define PRJ_RV_TOOLCHAIN_V2_6_1
#define PRJ_TOOLCHAIN_VER "V2.6.1"
/* end of Project options */
#define PLATFORM_LUBANLITE
#define PKGS_DIR "tools/env/packages"

/* Chip options */

#define SOC_THEAD_SMART
#define PRJ_CUSTOM_LDS "application/baremetal/bootloader/ldscript/d12p_bootloader_gcc.ld"
#define AIC_CHIP_D12P
#define CACHE_LINE_SIZE 32
#define CPU_DCACHE_PREFETCH_EN
#define CPU_BASE 0x20000000
#define CPU_PSRAM_BASE 0x40000000
#define CPU_SRAM_BASE 0x30040000
#define AIC_CMU_DRV
#define AIC_CMU_DRV_V32
#define AIC_CMU_FRAMEWORK_V2
#define AIC_CMU_DRV_VER "32"
#define AIC_GPIO_DRV
#define AIC_GPIO_DRV_V12
#define AIC_GPIO_DRV_VER "12"
#define AIC_SYSCFG_DRV
#define AIC_SYSCFG_DRV_V16
#define AIC_SYSCFG_DRV_VER "16"
#define AIC_DMA_DRV
#define AIC_DMA_DRV_V30
#define AIC_DMA_DRV_VER "30"
#define AIC_DMA_CH_NUM 8
#define AIC_DMA_ALIGN_SIZE 4
#define AIC_UART_DRV
#define AIC_UART_DRV_V14
#define AIC_UART_DRV_VER "14"
#define AIC_UART_DEV_NUM 5
#define AIC_QSPI_DRV
#define AIC_QSPI_DRV_V34
#define AIC_QSPI_DRV_VER "34"
#define AIC_XSPI_DRV
#define AIC_XSPI_DRV_V11
#define AIC_USB_DEVICE_DRV
#define AIC_USB_DEVICE_DRV_V30
#define AIC_USB_DEVICE_DRV_VER "30"
#define AIC_USB_DEVICE_DEV_NUM 1
#define AIC_USB_HOST_EHCI_DRV
#define AIC_USB_HOST_EHCI_DRV_V10
#define AIC_USB_HOST_EHCI_DRV_VER "10"
#define AIC_USB_HOST_EHCI_DEV_NUM 2
#define AIC_AXICFG_DRV
#define AIC_AXICFG_DRV_V11
#define AIC_AXICFG_DRV_VER "11"
#define AIC_WRI_DRV
#define AIC_WRI_DRV_V16
#define AIC_WRI_DRV_VER "16"
#define AIC_RTC_DRV
#define AIC_RTC_DRV_V121
#define AIC_RTC_DRV_VER "121"
#define AIC_WDT_DRV
#define AIC_WDT_DRV_V10
#define AIC_WDT_DRV_VER "10"
#define AIC_SDMC_DRV
#define AIC_SDMC_DRV_V11
#define AIC_SDMC_DRV_VER "11"
#define AIC_DE_DRV
#define AIC_DE_DRV_V11
#define AIC_DE_DRV_VER "11"
#define AIC_DISP_HW_RGB
#define AIC_DISP_HW_MIPI_DBI
#define AIC_DISP_HW_MIPI_DSI
#define AIC_DISP_RGB_DRV_V13
#define AIC_DISP_MIPI_DBI_DRV_V13
#define AIC_DISP_MIPI_DSI_DRV_V12
#define AIC_SID_DRV
#define AIC_SID_DRV_V17
#define AIC_SID_DRV_VER "17"
/* end of Chip options */

/* Board options */

/* Peripheral Devices: */

/* Interface Related: */

#define AIC_USING_UART0

/* UART0 Parameter */

#define AIC_CLK_UART0_FREQ 48000000
#define AIC_DEV_UART0_BAUDRATE 115200
#define AIC_DEV_UART0_DATABITS 8
#define AIC_DEV_UART0_STOPBITS 1
#define AIC_DEV_UART0_PARITY 0
#define AIC_DEV_UART0_RS232
#define AIC_DEV_UART0_MODE_RS232
#define AIC_DEV_UART0_MODE 0
#define AIC_UART0_FLAG 259
#define AIC_DEV_UART0_RX_MODE_POLL
#define AIC_DEV_UART0_RX_MODE 0
/* end of UART0 Parameter */
#define AIC_USB_OTG_DRV
#define AIC_USING_USB0
#define AIC_USING_USB0_OTG

/* Usb parameter */

#define AIC_USB_RES_CAL 0x60
#define AIC_USB_PHY_TXREF 0x8
#define AIC_USB_TXPREEMPAMP 0
/* end of Usb parameter */

/* Using XPWM */

#define XPWM_FIFO_MAX 2
#define XPWM_FIFO_TH_DEFAULT_VALUE 3
/* end of Using XPWM */

/* Using QEP */

/* end of Using QEP */

/* Using EPWM */

/* end of Using EPWM */

/* Using CAP */

/* end of Using CAP */

/* Using HRTIMER */

/* end of Using HRTIMER */

/* Storage Related: */

#define AIC_USING_QSPI0

/* SPI0 Parameter */

#define AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ 200000000
#define AIC_QSPI0_BUS_WIDTH_4
#define AIC_QSPI0_BUS_WIDTH 4
#define AIC_DEV_QSPI0_DELAY_MODE 2
/* end of SPI0 Parameter */
#define AIC_QSPI0_DEVICE_SPINOR
#define AIC_QSPI0_DEVICE_SPINOR_FREQ 100000000
#define AIC_USING_SDMC1

/* SDMC1 Parameter */

#define AIC_SDMC1_BUSWIDTH4
#define AIC_SDMC1_DRV_PHASE 3
#define AIC_SDMC1_SMP_PHASE 0
#define AIC_SDMC1_CLK_FREQ 100000000
/* end of SDMC1 Parameter */

/* Analog Related: */


/* MutiMedia Related: */

#define AIC_USING_DE

/* Display Parameter */

#define AIC_DISPLAY_DRV
#define AIC_DISP_DE_DRV
#define AIC_DISP_MIPI_DSI_DRV
#define AIC_DISP_MIPI_DSI
#define AIC_DI_TYPE 3
#define AICFB_RGB565
#define AICFB_FORMAT 0x0e
#define AIC_DISP_BOOTUP_LOGO
#define AIC_DISP_COLOR_BLOCK
#define AICFB_ROTATE_0
#define AIC_FB_ROTATE_DEGREE 0

/* Display Panels */

#define AIC_PANEL_DSI_FL7707_AV092K5U_B
#define AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO "PD.16"
#define AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO "PD.17"
/* end of Display Panels */
#define AIC_GPIO_BACKLIGHT
#define AIC_PANEL_ENABLE_GPIO "PE.19"
/* end of Display Parameter */

/* System Related: */

#define AIC_USING_DMA
#define AIC_USING_WRI
#define AIC_USING_RTC

/* RTC Parameter */

#define AIC_RTC_CLK_RATE 3276800
/* end of RTC Parameter */
#define AIC_USING_WDT

/* Mem Options */

/* SRAM parameter */

#define AIC_SRAM_SIZE 0x5000
#define AIC_SRAM_CMA_EN
#define AIC_SRAM_SW_SIZE 0x0
/* end of SRAM parameter */

/* PSRAM parameter */

#define AIC_PSRAM_SIZE 0x800000
#define AIC_PSRAM_CMA_EN
#define AIC_PSRAM_SW_SIZE 0x300000
#define AIC_PSRAM_SW_EN
/* end of PSRAM parameter */
#define AIC_INTERRUPTSTACK_SIZE 8192

/* Support another Flash */

#define AIC_FLASH_NUM_ONE
/* end of Support another Flash */
/* end of Mem Options */

/* Clocks options */

#define AIC_CLK_PLL_INT0_FREQ 480000000
#define AIC_CLK_PLL_INT1_FREQ 1200000000
#define AIC_CLK_PLL_FRA0_FREQ 396000000
#define AIC_CLK_PLL_FRA1_FREQ 396000000
#define AIC_CLK_PLL_FRA2_FREQ 1188000000
#define AIC_CLK_CPU_FREQ 480000000
/* end of Clocks options */

/* Security Related: */

#define AIC_USING_SID

/* SID Parameter */

#define EFUSE_WRITE_SUPPORT
#define EFUSE_MAX_WORD 64
#define EFUSE_TIMING_VALUE 0x04021FF1
/* end of SID Parameter */
#define AIC_USING_SYSCFG

/* Syscfg Parameter */

#define AIC_SYSCFG_LDO1X_ENABLE
#define AIC_SYSCFG_LDO1X_VOL_VAL 6
/* end of Syscfg Parameter */
/* end of Board options */

/* Baremetal options */

#define KERNEL_BAREMETAL
#define DRIVER_HAL_EN
#define DRIVER_BARE_DRV_EN
#define AIC_NORMALSTACK_SIZE 8192
#define ARCH_RISCV
#define ARCH_RISCV_FPU
#define ARCH_RISCV_FPU_S
#define ARCH_RISCV32
/* end of Baremetal options */

/* Bootloader options */

#define AIC_BOOTLOADER

/* Memory */

#define AIC_BOOTLOADER_MEM_AUTO
#define AIC_BOOTLOADER_TEXT_SIZE 0x7FF00
#define AIC_BOOTLOADER_HEAP_SIZE 0xC0000
/* end of Memory */

/* Console */

#define AIC_BOOTLOADER_CONSOLE_UART 0
#define AIC_CONSOLE_CTRLC_TMO 30
/* end of Console */

/* Drivers */

#define AIC_BOOTLOADER_MMC_SUPPORT
#define AIC_BOOTLOADER_SPINOR_SUPPORT
#define AIC_BOOTLOADER_AXICFG_SUPPORT

/* AXICFG Priority setting */

#define AIC_AXICFG_PORT_DVP_EN
#define AIC_AXICFG_PORT_DVP_PRIO 10
/* end of AXICFG Priority setting */
/* end of Drivers */
#define AICUPG_SUPPORT
#define AICUPG_SINGLE_TRANS_BURN_CRC32_VERIFY
#define AICUPG_USB_ENABLE
#define AICUPG_SDCARD_ENABLE
#define AICUPG_SDCARD_CONTROLLER_ID 1
#define AICUPG_MMC_ARTINCHIP
#define AICUPG_NOR_ARTINCHIP

/* Commands */

#define AIC_BOOTLOADER_CMD_NOR_BOOT
#define AIC_BOOTLOADER_CMD_SPI_NOR
#define AIC_BOOTLOADER_CMD_MTD
#define AIC_BOOTLOADER_CMD_MEM
#define AIC_BOOTLOADER_CMD_PROGRESS_BAR
#define AIC_BOOTLOADER_CMD_FB_CONSOLE
/* end of Commands */

/* Debug */

/* end of Debug */
/* end of Bootloader options */

/* Local packages options */

/* Third-party packages options */

#define LPKG_USING_CHERRYUSB_V1_5_0
#define LPKG_USING_CHERRYUSB
#define LPKG_CHERRYUSB_OTG
#define LPKG_CHERRYUSB_OTG_AUTO
#define LPKG_CHERRYUSB_ID_GPIO
#define LPKG_CHERRYUSB_ID_GPIO_NAME ""
#define LPKG_CHERRYUSB_ID_GPIO_ACT_LOW
#define LPKG_CHERRYUSB_DEVICE
#define LPKG_CHERRYUSB_DEVICE_HS
#define LPKG_CHERRYUSB_DEVICE_AIC
#define LPKG_CHERRYUSB_DEVICE_AIC_DMA
#define LPKG_CHERRYUSB_DEVICE_SINGLE
#define LPKG_CHERRYUSB_DEVICE_CLASS_AIC_SINGLE
#define LPKG_CHERRYUSB_DEVICE_CLASS_AIC
#define LPKG_CHERRYUSB_DEVICE_AICUPG_FOR_USB

/* BenchMark Tests */

#define AIC_PRINT_FLOAT_CUSTOM
/* end of BenchMark Tests */
#define LPKG_USING_DFS
#define DFS_USING_POSIX
#define DFS_FILESYSTEMS_MAX 4
#define DFS_FILESYSTEM_TYPES_MAX 4
#define DFS_FD_MAX 16
#define LPKG_USING_DFS_ELMFAT

/* elm-chan's FatFs, Generic FAT Filesystem Module */

#define LPKG_DFS_ELM_MINIMIZE_0
#define RT_DFS_ELM_MINIMIZE 0
#define RT_DFS_ELM_CODE_PAGE 437
#define RT_DFS_ELM_WORD_ACCESS
#define LPKG_DFS_ELM_USE_LFN_3
#define RT_DFS_ELM_USE_LFN 3
#define LPKG_DFS_ELM_LFN_UNICODE_0
#define RT_DFS_ELM_LFN_UNICODE 0
#define RT_DFS_ELM_MAX_LFN 255
#define RT_DFS_ELM_DRIVES 2
#define RT_DFS_ELM_MAX_SECTOR_SIZE 512
#define RT_DFS_FAL_BLK_DEVICE_NANE "rodata"
#define RT_DFS_ELM_REENTRANT
#define RT_DFS_ELM_MUTEX_TIMEOUT 3000
/* end of elm-chan's FatFs, Generic FAT Filesystem Module */
#define LPKG_USING_MINIMP3
#define LPKG_USING_FDTLIB
#define LPKG_USING_FDTLIB_CRC32_VERIFY
/* end of Third-party packages options */

/* ArtInChip packages options */

#define LPKG_USING_ENV
/* end of ArtInChip packages options */
/* end of Local packages options */

/* Drivers options */

/* AIC Bare Driver */

#define AIC_MTD_BARE_DRV
#define AIC_CONSOLE_BARE_DRV
#define AIC_CONSOLE_SYSNAME "tinySPL"
#define AIC_PRINTF_BARE_DRV
#define AIC_USING_UMM_HEAP
#define AIC_UMM_HEAP_BARE_DRV
#define RING_BUFFER
/* end of AIC Bare Driver */
#define AIC_SPINOR_DRV

/* Peripheral */

#define LPKG_USING_SFUD
#define BOOTLOADER_SFUD_USING_FLASH_INFO_TABLE

/* External Audio Codec Support */

/* end of External Audio Codec Support */
/* end of Peripheral */

/* Drivers debug */

#define AIC_LOG_LEVEL_INFO
#define AIC_LOG_LEVEL 6
/* end of Drivers debug */

/* Drivers examples */

#define AIC_SPINOR_DRV_TEST
#define AIC_MMC_BARE_TEST
#define AIC_MTD_BARE_TEST
#define AIC_SID_BARE_TEST
/* end of Drivers examples */
/* end of Drivers options */

/* Automatically calc generated */

#define AIC_BOOTLOADER_HEAP_BASE (0x40740000)
#define AIC_BOOTLOADER_TEXT_BASE (0x406c0100)

#endif
