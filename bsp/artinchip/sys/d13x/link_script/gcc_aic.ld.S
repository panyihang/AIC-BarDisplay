/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 ******************************************************************************
                d13x Memory Layout
 ******************************************************************************

           配置(1)        配置(2)       配置(3)
          SRAM_S1 OFF   SRAM_S1 OFF    SRAM_S1 ON
              TCM ON        TCM OFF
0x30040000+----------+  +----------+ +----------+
          |   ITCM   |  |          | |          |
          |   128K   |  |          | |   TCM    |
0x30060000+----------+  |          | |    +     |
          |   DTCM   |  |          | | SRAM_S0  |
          |   128k   |  |  SRAM_S0 | |          |
0x30080000+----------+  |    1M    | |1M-sram_s1|
          |          |  |          | |   size   |
          |          |  |          | |          |
          |          |  |          | |          |
          |  SRAM_S0 |  |          | +----------+
          |   768K   |  |          |
          |          |  |          |
          |          |  |          |
          |          |  |          |
          |          |  |          |
0x3013FFFF+----------+  +----------+

0x40000000 ------------------------> +----------+
- sram_s1 size                       |  SRAM_S1 |
                                     |          |
                                     |128K/256K/|
                                     |384K/512K/|
                                     |640K/768K |
0x3FFFFFFF ------------------------> +----------+

0x40000000+----------+
          |          |
          |  PSRAM   |
          |4M/8M/32M |
          |          |
          |          |
0x43FFFFFF+----------+
 */

#include "rtconfig.h"

#ifndef AIC_BOOTLOADER_RESERVE_SIZE
    #define AIC_BOOTLOADER_RESERVE_SIZE 0x40000
#endif
#if (AIC_BOOTLOADER_RESERVE_SIZE > AIC_SRAM_S1_SIZE)
    #define AIC_SRAM_RESERVE_SIZE AIC_BOOTLOADER_RESERVE_SIZE
#else
    #define AIC_SRAM_RESERVE_SIZE AIC_SRAM_S1_SIZE
#endif

#ifdef AIC_USING_PM
    #define AIC_PM_SRAM_RESERVE_SIZE 0x200
#else
    #define AIC_PM_SRAM_RESERVE_SIZE 0
#endif

MEMORY
{
    BROM        : ORIGIN = 0x30000000                                             , LENGTH = 256K
#ifdef AIC_TCM_EN
    ITCM        : ORIGIN = 0x30040000                                             , LENGTH = AIC_ITCM_SIZE
    DTCM        : ORIGIN = 0x30040000 + AIC_ITCM_SIZE                             , LENGTH = AIC_DTCM_SIZE
    SRAM_S0     : ORIGIN = 0x30040000 + AIC_ITCM_SIZE + AIC_DTCM_SIZE             , LENGTH = AIC_SRAM_TOTAL_SIZE - AIC_SRAM_RESERVE_SIZE - AIC_ITCM_SIZE - AIC_DTCM_SIZE
#else
    SRAM_S0     : ORIGIN = 0x30040000 + AIC_XIP_RAMCODE_SIZE                      , LENGTH = AIC_SRAM_TOTAL_SIZE - AIC_SRAM_RESERVE_SIZE - AIC_XIP_RAMCODE_SIZE
#endif
    SRAM_S1_CMA : ORIGIN = 0x40000000 - AIC_SRAM_S1_SIZE                          , LENGTH = AIC_SRAM_S1_SIZE - AIC_SRAM1_SW_SIZE
    SRAM_S1_SW  : ORIGIN = 0x40000000 - AIC_SRAM1_SW_SIZE                         , LENGTH = AIC_SRAM1_SW_SIZE
    PSRAM_CMA   : ORIGIN = 0x40000000                                             , LENGTH = AIC_PSRAM_SIZE - AIC_PSRAM_SW_SIZE
    PSRAM_SW    : ORIGIN = 0x40000000 + AIC_PSRAM_SIZE - AIC_PSRAM_SW_SIZE        , LENGTH = AIC_PSRAM_SW_SIZE
#ifdef AIC_XIP
    FLASH_XIP   : ORIGIN = 0x60000000 + AIC_XIP_FW_OFFSET                         , LENGTH = 512M - AIC_XIP_FW_OFFSET
#endif
}

#ifdef AIC_TCM_EN
PROVIDE (__itcm_start               = 0x30040000);
PROVIDE (__itcm_end                 = __itcm_start + AIC_ITCM_SIZE);
PROVIDE (__dtcm_start               = __itcm_end);
PROVIDE (__dtcm_end                 = __itcm_end + AIC_DTCM_SIZE);
PROVIDE (__sram_s0_start            = __dtcm_end);
PROVIDE (__sram_s0_end              = __dtcm_end + AIC_SRAM_TOTAL_SIZE - AIC_SRAM_S1_SIZE - AIC_ITCM_SIZE - AIC_DTCM_SIZE);
#else
PROVIDE (__sram_s0_start            = 0x30040000);
PROVIDE (__sram_s0_end              = __sram_s0_start + AIC_SRAM_TOTAL_SIZE - AIC_SRAM_S1_SIZE);
#endif

PROVIDE (__sram_pm_start            = __sram_s0_pm_heap_start);
PROVIDE (__sram_pm_end              = __sram_s0_pm_heap_end);

PROVIDE (__sram_s1_cma_start        = __sram_s1_cma_heap_start);
PROVIDE (__sram_s1_cma_end          = 0x40000000 - AIC_SRAM1_SW_SIZE);
PROVIDE (__sram_s1_sw_start         = 0x40000000 - AIC_SRAM1_SW_SIZE);
PROVIDE (__sram_s1_sw_end           = 0x40000000);


PROVIDE (__psram_cma_start          = __psram_cma_heap_start);
PROVIDE (__psram_cma_end            = 0x40000000 + AIC_PSRAM_SIZE - AIC_PSRAM_SW_SIZE);
PROVIDE (__psram_sw_start           = __psram_sw_heap_start);
PROVIDE (__psram_sw_end             = 0x40000000 + AIC_PSRAM_SIZE);

PROVIDE (__psram_start              = 0x40000000);
PROVIDE (__psram_end                = 0x40000000 + AIC_PSRAM_SIZE);
PROVIDE (__dtb_pos_f                = __psram_end - 0x40000);

#ifdef AIC_TCM_EN
PROVIDE (__itcm_heap_end            = __itcm_end);
PROVIDE (__dtcm_heap_end            = __dtcm_end);
#endif
PROVIDE (__sram_s0_sw_heap_end      = __sram_s0_end);
PROVIDE (__sram_s1_cma_heap_end     = __sram_s1_cma_end);
PROVIDE (__sram_s1_sw_heap_end      = __sram_s1_sw_end);
PROVIDE (__psram_cma_heap_end       = __psram_cma_end);
PROVIDE (__psram_sw_heap_end        = __psram_sw_end);


PROVIDE (__min_heap_size            = 0x200);
#ifdef AIC_DEFAULT_SYS_HEAP_SRAM
PROVIDE (__heap_start               = __sram_s0_sw_heap_start);
PROVIDE (__heap_end                 = __sram_s0_sw_heap_end);
#elif defined AIC_DEFAULT_SYS_HEAP_PSRAM
PROVIDE (__heap_start               = __psram_sw_heap_start);
PROVIDE (__heap_end                 = __psram_sw_heap_end);
#else
PROVIDE (__heap_start               = __sram_s0_sw_heap_start);
PROVIDE (__heap_end                 = __sram_s0_sw_heap_end);
#endif

#ifdef AIC_PSRAM_CMA_EN
PROVIDE (__cma_heap_start           = __psram_cma_heap_start);
PROVIDE (__cma_heap_end             = __psram_cma_heap_end);
#else
PROVIDE (__cma_heap_start           = __sram_s1_cma_heap_start);
PROVIDE (__cma_heap_end             = __sram_s1_cma_heap_end);
#endif

#ifdef AIC_TCM_EN
REGION_ALIAS("REGION_ITCM"          , ITCM);
REGION_ALIAS("REGION_DTCM"          , DTCM);
#endif
REGION_ALIAS("REGION_SRAM_S0_SW"    , SRAM_S0);
REGION_ALIAS("REGION_SRAM_S1_CMA"   , SRAM_S1_CMA);
REGION_ALIAS("REGION_SRAM_S1_SW"    , SRAM_S1_SW);
REGION_ALIAS("REGION_PSRAM_CMA"     , PSRAM_CMA);
REGION_ALIAS("REGION_PSRAM_SW"      , PSRAM_SW);

#ifdef AIC_SEC_TEXT_SRAM_S0
REGION_ALIAS("REGION_TEXT"          , SRAM_S0);
#elif defined AIC_SEC_TEXT_SRAM_S1
REGION_ALIAS("REGION_TEXT"          , SRAM_S1_CMA);
#elif defined AIC_SEC_TEXT_PSRAM
REGION_ALIAS("REGION_TEXT"          , PSRAM_CMA);
#elif defined AIC_SEC_TEXT_XIP
REGION_ALIAS("REGION_TEXT"          , FLASH_XIP);
#else
REGION_ALIAS("REGION_TEXT"          , SRAM_S0);
#endif

#ifdef AIC_SEC_RODATA_SRAM_S0
REGION_ALIAS("REGION_RODATA"        , SRAM_S0);
#elif defined AIC_SEC_RODATA_SRAM_S1
REGION_ALIAS("REGION_RODATA"        , SRAM_S1_CMA);
#elif defined AIC_SEC_RODATA_PSRAM
REGION_ALIAS("REGION_RODATA"        , PSRAM_CMA);
#elif defined AIC_SEC_RODATA_XIP
REGION_ALIAS("REGION_RODATA"        , FLASH_XIP);
#else
REGION_ALIAS("REGION_RODATA"        , SRAM_S0);
#endif

#ifdef AIC_SEC_DATA_SRAM_S0
REGION_ALIAS("REGION_DATA"          , SRAM_S0);
#elif defined AIC_SEC_DATA_SRAM_S1
REGION_ALIAS("REGION_DATA"          , SRAM_S1_CMA);
#elif defined AIC_SEC_DATA_PSRAM
REGION_ALIAS("REGION_DATA"          , PSRAM_CMA);
#else
REGION_ALIAS("REGION_DATA"          , SRAM_S0);
#endif

#ifdef AIC_SEC_BSS_SRAM_S0
REGION_ALIAS("REGION_BSS"           , SRAM_S0);
#elif defined AIC_SEC_BSS_SRAM_S1
REGION_ALIAS("REGION_BSS"           , SRAM_S1_CMA);
#elif defined AIC_SEC_BSS_PSRAM
REGION_ALIAS("REGION_BSS"           , PSRAM_CMA);
#else
REGION_ALIAS("REGION_BSS"           , PSRAM_CMA);
#endif

ENTRY(Reset_Handler)
SECTIONS
{
#ifdef AIC_TCM_EN
 .itcm : {
  . = ALIGN(0x4) ;
  __itcm_code_start = .;
  *(.tcm_code)
  . = ALIGN(0x4) ;
  __itcm_code_end = .;
  __itcm_heap_start = .;
 } > REGION_ITCM AT > REGION_ITCM

 .dtcm : {
  . = ALIGN(0x4) ;
  __dtcm_data_start = .;
  *(.tcm_data)
  . = ALIGN(0x4) ;
  __dtcm_data_end = .;
  __dtcm_heap_start = .;
 } > REGION_DTCM
#endif

 .text : AT(ADDR(.text)){
  . = ALIGN(0x4) ;
  __stext = . ;
  KEEP(*startup_gcc.o(*.text*))
  *(.text)
  *(.text*)
  *(.text.*)
  *(.gnu.warning)
  *(.stub)
  *(.gnu.linkonce.t*)
  *(.glue_7t)
  *(.glue_7)
  *(.jcr)
  *(.init)
  *(.fini)
  . = ALIGN (4) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)

  /* section information for tiny console shell */
  . = ALIGN(4) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(4) ;
  __console_init_end = .;

  /* section information for finsh shell */
  . = ALIGN(4);
  __fsymtab_start = .;
  KEEP(*(FSymTab))
  __fsymtab_end = .;
  . = ALIGN(4);
  __vsymtab_start = .;
  KEEP(*(VSymTab))
  __vsymtab_end = .;
  . = ALIGN(4);

  /* section information for initial. */
  . = ALIGN(4);
  __rt_init_start = .;
  KEEP(*(SORT(.rti_fn*)))
  __rt_init_end = .;
  . = ALIGN(4);
  . = ALIGN(0x10) ;

#ifdef RT_USING_MODULE
  /* section information for modules */
  . = ALIGN(4);
  __rtmsymtab_start = .;
  KEEP(*(RTMSymTab))
  __rtmsymtab_end = .;
#endif

  __etext = . ;
 } > REGION_TEXT

 .eh_frame_hdr : {
  *(.eh_frame_hdr)
 } > REGION_TEXT
 .eh_frame : ONLY_IF_RO {
  KEEP (*(.eh_frame))
 } > REGION_TEXT
 .gcc_except_table : ONLY_IF_RO {
  *(.gcc_except_table .gcc_except_table.*)
 } > REGION_TEXT

 .rodata :{
  . = ALIGN(0x4) ;
  __srodata = .;
  *(.rdata)
  *(.rdata*)
  *(.rdata1)
  *(.rdata.*)
  *(.rodata)
  *(.rodata1)
  *(.rodata*)
  *(.rodata.*)
  *(.srodata*)
  *(.rodata.str1.4)
  . = ALIGN(0x4) ;
    PROVIDE(__ctors_start__ = .);
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array))
  PROVIDE(__ctors_end__ = .);
  PROVIDE(__dtors_start__ = .);
  KEEP(*(SORT(.dtors.*)))
  KEEP(*(.dtors))
  PROVIDE(__dtors_end__ = .);

  /* usb host class */
  . = ALIGN(0x8) ;
  __usbh_class_info_start__ = .;
  KEEP(*(.usbh_class_info))
  __usbh_class_info_end__ = .;

#ifdef LPKG_USING_ZEPHYR_BLUETOOTH
  /* zephyr bluetooth bt_l2cap_fixed_chan */
  . = ALIGN(0x8) ;
 _bt_l2cap_fixed_chan_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_l2cap_fixed_chan.static.*)));
 _bt_l2cap_fixed_chan_list_end = .;

  /* zephyr bluetooth bt_l2cap_br_fixed_chan */
  . = ALIGN(0x8) ;
 _bt_l2cap_br_fixed_chan_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_l2cap_br_fixed_chan.static.*)));
 _bt_l2cap_br_fixed_chan_list_end = .;

  /* zephyr bluetooth bt_conn_cb */
  . = ALIGN(0x8) ;
 _bt_conn_cb_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_conn_cb.static.*)));
 _bt_conn_cb_list_end = .;

  /* zephyr bluetooth bt_sco_conn_cb */
  . = ALIGN(0x8) ;
 _bt_sco_conn_cb_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_sco_conn_cb.static.*)));
 _bt_sco_conn_cb_list_end = .;

  /* zephyr bluetooth bt_sco_hci_cb */
  . = ALIGN(0x8) ;
 _bt_sco_hci_cb_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_sco_hci_cb.static.*)));
 _bt_sco_hci_cb_list_end = .;

  /* zephyr bluetooth bt_gatt_service_static */
  . = ALIGN(0x8) ;
 _bt_gatt_service_static_list_start = .;
 KEEP(*(SORT_BY_NAME(._bt_gatt_service_static.static.*)));
 _bt_gatt_service_static_list_end = .;

  /* zephyr bluetooth net_buf_pool */
  . = ALIGN(0x8) ;
 _net_buf_pool_list_start = .;
 KEEP(*(SORT_BY_NAME(._net_buf_pool.static.*)));
 _net_buf_pool_list_end = .;

  /* zephyr bluetooth k_mem_slab */
  . = ALIGN(0x8) ;
 _k_mem_slab_list_start = .;
 KEEP(*(SORT_BY_NAME(._k_mem_slab.static.*)));
 _k_mem_slab_list_end = .;
#endif


#ifdef AIC_XIP
  . = ALIGN(0x40) ;
#else
  . = ALIGN(0x4) ;
#endif
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA

#ifdef AIC_XIP
 .ram.code : {
  . = ALIGN(0x8) ;
  __ram_code_start__ = .;
  __sdata = . ;
  __data_start__ = . ;
  *(.ram.code*)
  __ram_code_end__ = .;
  . = ALIGN(0x8) ;
 } > REGION_DATA AT > REGION_DATA

 .data : {
  . = ALIGN(0x4) ;
#else
 .data : {
  . = ALIGN(0x4) ;
  __data_start__ = . ;
  __sdata = . ;
#endif
  data_start = . ;
  KEEP(*startup_gcc.o(*.vectors*))
  *(.got.plt)
  *(.got)
  *(.gnu.linkonce.r*)
  *(.data)
  *(.data*)
  *(.data1)
  *(.data.*)
  *(.gnu.linkonce.d*)
  *(.data1)
  *(.gcc_except_table)
  *(.gcc_except_table*)
  __start_init_call = .;
  *(.initcall.init)
  __stop_init_call = .;
  __start_cmd = .;
  *(.bootloaddata.cmd)
  . = ALIGN(4) ;
  __stop_cmd = .;
  __global_pointer$ = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(__libc_atexit)
  *(__libc_subinit)
  *(__libc_subfreeres)
  *(.note.ABI-tag)
  __edata = .;
  __data_end__ = .;
  . = ALIGN(0x4) ;

 } > REGION_DATA AT > REGION_DATA
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_DATA AT > REGION_DATA
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __data_end__ = .;
 } > REGION_DATA AT > REGION_DATA

 .bss : {
  . = ALIGN(0x4) ;
  __sbss = ALIGN(0x4) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x4) ;
  __ebss = . ;
  __end = . ;
  end = . ;
  __bss_end__ = .;
 } > REGION_BSS

#ifdef AIC_PM_SRAM_RESERVE_SIZE
 ._sram_s0_pm_heap : {
  . = ALIGN(0x4) ;
  __sram_s0_pm_heap_start = .;
  . += AIC_PM_SRAM_RESERVE_SIZE;
  . = ALIGN(0x4) ;
  __sram_s0_pm_heap_end = .;
 } > REGION_SRAM_S0_SW AT > REGION_SRAM_S0_SW
#endif

 ._sram_s0_sw_heap : {
  . = ALIGN(0x4) ;
  __sram_s0_sw_heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x4) ;
 } > REGION_SRAM_S0_SW AT > REGION_SRAM_S0_SW

 .sram_s1_cma : {
  . = ALIGN(0x4) ;
  __sram_s1_cma_data_start = .;
  *(.sram1_cma_data)
  . = ALIGN(0x4) ;
  __sram_s1_cma_data_end = .;
  __sram_s1_cma_heap_start = .;
 } > REGION_SRAM_S1_CMA AT > REGION_SRAM_S1_CMA

 .sram_s1_sw : {
  . = ALIGN(0x4) ;
  __sram_s1_sw_data_start = .;
  *(.sram1_sw_data)
  . = ALIGN(0x4) ;
  __sram_s1_sw_data_end = .;
  __sram_s1_sw_heap_start = .;
 } > REGION_SRAM_S1_SW AT > REGION_SRAM_S1_SW

 .psram_cma : {
  . = ALIGN(0x4) ;
  __psram_cma_data_start = .;
  *(.psram_cma_data)
  . = ALIGN(0x4) ;
  __psram_cma_data_end = .;
  __psram_cma_heap_start = .;
 } > REGION_PSRAM_CMA AT > REGION_PSRAM_CMA

 .psram_sw : {
  . = ALIGN(0x4) ;
  __psram_sw_data_start = .;
  *(.psram_sw_data)
  . = ALIGN(0x4) ;
  __psram_sw_data_end = .;
  __psram_sw_heap_start = .;
 } > REGION_PSRAM_SW AT > REGION_PSRAM_SW
}
