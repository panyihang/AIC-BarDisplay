/*
 * Copyright (c) 2020-2026 ArtInChip Technology Co., Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _PRIVATE_PARAM_H_
#define _PRIVATE_PARAM_H_
#include <aic_common.h>

#define DATA_SECT_TYPE_DRAM        0x41490001
#define DATA_SECT_TYPE_SYS_UART    0x41490002
#define DATA_SECT_TYPE_SYS_JTAG    0x41490003
#define DATA_SECT_TYPE_SYS_UPGMODE 0x41490004
#define DATA_SECT_TYPE_PARTITION   0x41490005
#define DATA_SECT_TYPE_SEC_KEYDATA 0x41490008
#define DATA_SECT_TYPE_END         0x4149FFFF

#define KEYDATA_VERSION_V1 1

struct private_dram_param {
	u32 type;
	u32 size;
	u32 freq;
	u32 zq;
	u32 odt;
	u32 para1;
	u32 para2;
	u32 mr0;
	u32 mr1;
	u32 mr2;
	u32 mr3;
	u32 mr4;
	u32 mr5;
	u32 mr6;
	u32 tpr0;
	u32 tpr1;
	u32 tpr2;
	u32 tpr3;
	u32 tpr4;
	u32 tpr5;
	u32 tpr6;
	u32 tpr7;
	u32 tpr8;
	u32 tpr9;
	u32 tpr10;
	u32 tpr11;
	u32 tpr12;
	u32 tpr13;
	u32 tpr14;
	u32 tpr15;
	u32 tpr16;
	u32 tpr17;
	u32 tpr18;
};

struct private_system_uart_param {
	u32 uart_id;
	u32 uart_tx_pin_cfg_reg;
	u32 uart_tx_pin_cfg_val;
	u32 uart_rx_pin_cfg_reg;
	u32 uart_rx_pin_cfg_val;
};

struct private_system_upgmode_pin_param {
	u32 upgmode_pin_cfg_reg;
	u32 upgmode_pin_cfg_val;
	u32 upgmode_pin_input_reg;
	u32 upgmode_pin_input_msk;
	u32 upgmode_pin_input_val;
	u32 upgmode_pin_pullup_dly;
};

struct private_system_jtag_param {
	u32 jtag_id;
	u32 jtag_do_pin_cfg_reg;
	u32 jtag_do_pin_cfg_val;
	u32 jtag_di_pin_cfg_reg;
	u32 jtag_di_pin_cfg_val;
	u32 jtag_ms_pin_cfg_reg;
	u32 jtag_ms_pin_cfg_val;
	u32 jtag_ck_pin_cfg_reg;
	u32 jtag_ck_pin_cfg_val;
};

struct private_system_jtag_head {
	u32 jtag_only;
	struct private_system_jtag_param param;
};

struct private_security_keydata_v1 {
	u32 version;
	u8 seed[64];
	u8 key_enc[64];
};

struct private_dram_param *private_get_ddr_init_param(void *res_addr, u32 type);
struct private_system_uart_param *private_get_system_uart_param(void *res_addr);
struct private_system_jtag_head *private_get_system_jtag_param(void *res_addr);
struct private_system_upgmode_pin_param *private_get_system_upgmode_pin_param(void *res_addr);
struct private_security_keydata_v1 *get_security_keydata(void *res_addr, u32 ver);
char *private_get_partition_string(void *res_addr);

#endif /* _PRIVATE_PARAM_H_ */
