/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
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
          |  4M/8M   |
          |          |
          |          |
          +----------+
          |          |
          | FPGA Ext |
          |  PSRAM   |
          | 64M-psram|
          |     size |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
0x43FFFFFF+----------+
 */

#include "rtconfig.h"

MEMORY
{
    SRAM_SW     : ORIGIN = (AIC_BOOTLOADER_TEXT_BASE), LENGTH = AIC_BOOTLOADER_TEXT_SIZE
}

PROVIDE (__heap_start              = AIC_BOOTLOADER_HEAP_BASE);
PROVIDE (__heap_end                = AIC_BOOTLOADER_HEAP_BASE + AIC_BOOTLOADER_HEAP_SIZE);

REGION_ALIAS("REGION_TEXT"          , SRAM_SW);
REGION_ALIAS("REGION_RODATA"        , SRAM_SW);
REGION_ALIAS("REGION_DATA"          , SRAM_SW);
REGION_ALIAS("REGION_BSS"           , SRAM_SW);
REGION_ALIAS("REGION_SRAM_SW"       , SRAM_SW);

ENTRY(Reset_Handler)
SECTIONS
{
 .text : AT(ADDR(.text)){
  . = ALIGN(0x8) ;
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

  . = ALIGN(0x10) ;
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
  . = ALIGN(0x8) ;
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
  . = ALIGN(0x8) ;
  __ctor_start__ = .;
  KEEP (*(SORT(.ctors.*)))
  KEEP (*(.ctors))
  __ctor_end__ = .;
  KEEP (*(SORT(.dtors.*)))
  KEEP (*(.dtors))
  __dtor_end__ = .;

  /* usb host class */
  . = ALIGN(0x8) ;
  __usbh_class_info_start__ = .;
  KEEP(*(.usbh_class_info))
  __usbh_class_info_end__ = .;

  . = ALIGN(0x8) ;
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA

 .data : {
  . = ALIGN(0x8) ;
  __data_start__ = . ;
  __sdata = . ;
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
  . = ALIGN(8) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(8) ;
  __console_init_end = .;
  __global_pointer$ = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(.note.ABI-tag)
  __edata = .;
  __data_end__ = .;
  . = ALIGN(0x8) ;

 } > REGION_DATA AT > REGION_RODATA
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_DATA AT > REGION_RODATA
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __edata = .;
  __data_end__ = .;
 } > REGION_DATA AT > REGION_RODATA

 .bss : {
  . = ALIGN(0x8) ;
  __sbss = ALIGN(0x8) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x8) ;
  __ebss = . ;
  __end = . ;
  end = . ;
  __bss_end__ = .;
 } > REGION_BSS
}
