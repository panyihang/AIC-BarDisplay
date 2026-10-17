/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_SOC_H_
#define _AIC_SOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IHS_VALUE
#define  IHS_VALUE                  (20000000)
#endif

#ifndef EHS_VALUE
#define  EHS_VALUE                  (20000000)
#endif

/* frequence */

#define CLOCK_120M                  120000000
#define CLOCK_100M                  100000000
#define CLOCK_72M                   72000000
#define CLOCK_60M                   60000000
#define CLOCK_50M                   50000000
#define CLOCK_36M                   36000000
#define CLOCK_30M                   30000000
#define CLOCK_AUDIO                 24576000
#define CLOCK_24M                   24000000
#define CLOCK_12M                   12000000
#define CLOCK_4M                    4000000
#define CLOCK_1M                    1000000
#define CLOCK_32K                   32768

#ifndef __ASSEMBLY__
/* -------------------------  Interrupt Number Definition  ------------------------ */
typedef enum IRQn {
    NMI_EXPn                        = -2,  /* NMI Exception */
    Supervisor_Software_IRQn        = 1U,
    Machine_Software_IRQn           = 3U,  /* Machine software interrupt */
    User_Timer_IRQn                 = 4,   /* User timer interrupt */
    Supervisor_Timer_IRQn           = 5U,  /* Supervisor timer interrupt */
    CORET_IRQn                      = 7U,  /* core Timer Interrupt */
    Supervisor_External_IRQn        = 9U,
    Machine_External_IRQn           = 11U, /* Machine external interrupt */

    DCE_IRQn                        = 31U,
    DMA_IRQn                        = 32U,
    USB_DEV_IRQn                    = 34U,
    USB_HOST0_EHCI_IRQn              = 35U,
    USB_HOST0_OHCI_IRQn              = 36U,
    SDMC0_IRQn                      = 38U,
    SDMC1_IRQn                      = 39U,
    SPI_ENC_IRQn                    = 41U,
    QSPI0_IRQn                      = 42U,
    QSPI1_IRQn                      = 43U,
    QSPI2_IRQn                      = 44U,
    QSPI3_IRQn                      = 45U,
    QSPI4_IRQn                      = 46U,
    QSPI5_IRQn                      = 47U,
    XSPI_IRQn                       = 49U,
    RTC_IRQn                        = 50U,
    MTOP_IRQn                       = 51U,
    I2S0_IRQn                       = 52U,
    AUDIO_IRQn                      = 54U,
    LCD_IRQn                        = 55U,
    MIPI_DSI_IRQn                   = 56U,
    DVP_IRQn                        = 57U,
    DE_IRQn                         = 59U,
    GE_IRQn                         = 60U,
    VE_IRQn                         = 61U,
    WDT_IRQn                        = 64U,
    GPIO_IRQn                       = 68U, /* 68~75 */
    UART0_IRQn                      = 78U,
    UART1_IRQn                      = 79U,
    UART2_IRQn                      = 80U,
    UART3_IRQn                      = 81U,
    UART4_IRQn                      = 82U,
    GPAI_IRQn                       = 92U,
    RTP_IRQn                        = 93U,
    TSEN_IRQn                       = 94U,
    CIR_IRQn                        = 95U,
    XPWM0_IRQn                      = 96U,
    XPWM1_IRQn                      = 97U,
    XPWM2_IRQn                      = 98U,
    XPWM3_IRQn                      = 99U,
    XPWM4_IRQn                      = 100U,
    XPWM5_IRQn                      = 101U,
    XPWM6_IRQn                      = 102U,
    XPWM7_IRQn                      = 103U,
    CANFD0_IRQn                     = 106U,
    EPWM0_IRQn                      = 108U,
    EPWM1_IRQn                      = 109U,
    EPWM2_IRQn                      = 110U,
    CAP0_IRQn                       = 113U,
    CAP1_IRQn                       = 114U,
    QEP0_IRQn                       = 119U,
    QEP1_IRQn                       = 120U,
    I2C0_IRQn                       = 126U,
    I2C1_IRQn                       = 127U,
    MAX_IRQn,
}
IRQn_Type;

#define UART_IRQn(id)    (UART0_IRQn + id)
#endif

/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */


/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */
#define BROM_BASE                   0x30000000UL /* - 0x30007FFF, 512KB	*/
#define SRAM_BASE                   0x30040000UL /* - 0x3004FFFF, 1MB	*/
#define PSRAM_BASE                  0x40000000UL /* - 0x5FFFFFFF, 512MB	*/
#define DMA_BASE                    0x10000000UL /* - 0x1000FFFF, 64KB	,64KB	*/
#define DCE_BASE                    0x10010000UL /* - 0x10010FFF,  4KB	,4KB	*/
#define USB_DEV_BASE                0x10200000UL /* - 0x1020FFFF, 64KB	,--	*/
#define USB_HOST0_BASE              0x10210000UL /* - 0x1021FFFF, 64KB	,--	*/
#define CANFD0_BASE                 0x102C0000UL /* - 0x102CFFFF, 64KB */
#define XSPI_BASE                   0x10300000UL /* - 0x10030FFF,  4KB	,4KB	*/
#define QSPI0_BASE                  0x10400000UL /* - 0x1040FFFF, 64KB	,--	*/
#define QSPI1_BASE                  0x10401000UL /* - 0x1041FFFF, 64KB	,256KB	*/
#define QSPI2_BASE                  0x10402000UL /* - 0x1042FFFF, 64KB	,256KB	*/
#define QSPI3_BASE                  0x10403000UL /* - 0x1043FFFF, 64KB	,256KB	*/
#define QSPI4_BASE                  0x10404000UL /* - 0x1044FFFF, 64KB	,256KB	*/
#define QSPI5_BASE                  0x10405000UL /* - 0x1045FFFF, 64KB	,256KB	*/
#define SPI_ENC_BASE                0x1040F000UL /* - 0x1040FFFF, 4KB */
#define SDMC0_BASE                  0x10440000UL /* - 0x1044FFFF, 64KB	,--	*/
#define SDMC1_BASE                  0x10450000UL /* - 0x1045FFFF, 64KB	,--	*/
#define AHBCFG_BASE                 0x104FE000UL /* - 0x104FEFFF,  4KB	,4KB	*/
#define GPIO_BASE                   0x10700000UL /* - 0x10700FFF, 4KB	,64KB	*/
#define PWMCS_BASE                  0x11040000UL /* - 0x1104FFFF, 64KB */
#define XPWM0_BASE                  0x110C0000UL /* - 0x110C00FF, 256B */
#define XPWM1_BASE                  0x110C0100UL /* - 0x110C01FF, 256B */
#define XPWM2_BASE                  0x110C0200UL /* - 0x110C02FF, 256B */
#define XPWM3_BASE                  0x110C0300UL /* - 0x110C03FF, 256B */
#define XPWM4_BASE                  0x110C0400UL /* - 0x110C04FF, 256B */
#define XPWM5_BASE                  0x110C0500UL /* - 0x110C05FF, 256B */
#define XPWM6_BASE                  0x110C0600UL /* - 0x110C06FF, 256B */
#define XPWM7_BASE                  0x110C0700UL /* - 0x110C07FF, 256B */
#define SYSCFG_BASE                 0x18000000UL /* - 0x18000FFF, 4KB	,64KB	*/
#define CMU_BASE                    0x18020000UL /* - 0x18020FFF, 4KB	,32KB	*/
#define ADCIM_BASE                  0x18400000UL /* - 0x18400FFF, 4KB	,--	*/
#define GPAI_BASE                   0x18401000UL /* - 0x18401FFF, 4KB	,--	*/
#define TSEN_BASE                   0x18402000UL /* - 0x18402FFF, 4KB   ,-- */
#define RTP_BASE                    0x18403000UL /* - 0x18403FFF, 4KB	,--	*/
#define AXICFG_BASE                 0x184FE000UL /* - 0x184FEFFF, 4KB	,--	*/
#define MTOP_BASE                   0x184FF000UL /* - 0x184FFFFF, 4KB	,--	*/
#define I2S0_BASE                   0x18600000UL /* - 0x18600FFF, 4KB */
#define AUDIO_BASE                  0x18610000UL /* - 0x18610FFF, 4KB	,64KB	*/
#define UART0_BASE                  0x18710000UL /* - 0x18710FFF, 4KB	,--	*/
#define UART1_BASE                  0x18711000UL /* - 0x18711FFF, 4KB	,--	*/
#define UART2_BASE                  0x18712000UL /* - 0x18712FFF, 4KB	,--	*/
#define UART3_BASE                  0x18713000UL /* - 0x18713FFF, 4KB	,--	*/
#define UART4_BASE                  0x18714000UL /* - 0x18714FFF, 4KB	,--	*/
#define UART_BASE(id)               (UART0_BASE + (id) * 0x1000UL)
#define I2C0_BASE                   0x18720000UL /* - 0x18720FFF, 4KB	,--	*/
#define I2C1_BASE                   0x18721000UL /* - 0x18721FFF, 4KB	,--	*/
#define LCD_BASE                    0x18800000UL /* - 0x18800FFF, 4KB	,64KB	*/
#define MIPI_DSI_BASE               0x18820000UL /* - 0x18820FFF, 4KB */
#define DVP_BASE                    0x18830000UL /* - 0x18830FFF, 4KB */
#define DE_BASE                     0x18A00000UL /* - 0x18AFFFFF, 1MB	,1MB	*/
#define GE_BASE                     0x18B00000UL /* - 0x18BFFFFF, 1MB	,1MB	*/
#define VE_BASE                     0x18C00000UL /* - 0x18CFFFFF, 1MB	,1MB	*/
#define WDT_BASE                    0x19000000UL /* - 0x19000FFF, 4KB	,64KB	*/
#define WRI_BASE                    0x1900F000UL /* - 0x1900FFFF, 4KB	,64KB	*/
#define SID_BASE                    0x19010000UL /* - 0x19010FFF, 4KB	,64KB	*/
#define RTC_BASE                    0x19030000UL /* - 0x19030FFF, 4KB */
#define GTC_BASE                    0x19050000UL /* - 0x19051FFF, 8KB	,64KB	*/
#define CIR_BASE                    0x19260000UL /* - 0x19260FFF, 4KB	,--	*/
#define FLASH_XIP_BASE              0x60000000UL

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */


#ifdef __cplusplus
}
#endif

#endif  /* _AIC_SOC_H_ */
