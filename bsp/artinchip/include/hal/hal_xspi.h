/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef _AIC_HAL_XSPI_
#define _AIC_HAL_XSPI_
#include <aic_common.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_XSPI_MAX_FREQ_HZ         445500000
#define HAL_XSPI_MIN_FREQ_HZ         49500000

#define BOUNDARY_2K 0
#define BOUNDARY_1K 1

struct hal_xspi_state;
typedef struct hal_xspi_state hal_xspi_handle;
typedef void (*hal_xspi_async_cb)(hal_xspi_handle *h, void *priv);
typedef struct hal_xspi_proto_cfg hal_xspi_proto_cfg_t;

struct hal_xspi_config {
    u32 idx;
    u32 clk_in_hz;
    u32 clk_id;
    u32 cs0_port;
    u32 cs1_port;
    bool bit_mode;
    bool wire3_en;
    bool lsb_en;
    bool cs_auto;
    u8 cs_polarity;
    u8 cpol;
    u8 cpha;
};

struct hal_xspi_dma_config {
    u32 port_id;
    u32 tx_bus_width;
    u32 tx_max_burst;
    u32 rx_bus_width;
    u32 rx_max_burst;
};

struct hal_xspi_transfer {
    u8 *tx_data;
    u8 *rx_data;
    u32 data_len;
};

struct hal_xspi_proto_cfg {
    u8 mode;
    u8 clk_mode;
    u8 parallel_mode;

    u8 wr_cmd_clk_mode;
    u8 wr_cmd_lines;
    u8 wr_cmd_val;

    u8 rd_cmd_clk_mode;
    u8 rd_cmd_lines;
    u8 rd_cmd_val;

    u8 addr_clk_mode;
    u8 addr_lines;
    u8 addr_width;

    u8 wr_dummy;
    u8 rd_dummy;

    u8 wr_cnt_lines;
    u32 wr_cnt;

    u8 rd_cnt_lines;
    u32 rd_cnt;

};

/*
 * HAL XSPI internal state, HAL user should not modify it directly
 */
struct hal_xspi_state {
    u32 idx;
    hal_xspi_async_cb cb;
    void *cb_priv;
    u32 status;
    u32 clk_id;
    u32 bus_hz;
    u32 bus_width;
    struct hal_xspi_dma_config dma_cfg;
    void *dma_tx;
    void *dma_rx;
    u8 *async_tx; /* Used in Async Non-DMA mode */
    u8 *async_rx; /* Used in Async Non-DMA mode */
    u32 async_tx_remain; /* Used in Async Non-DMA mode */
    u32 async_rx_remain; /* Used in Async Non-DMA mode */
    u32 work_mode;
    u32 done_mask;
};




int hal_xspi_init(hal_xspi_handle *h, struct hal_xspi_config *cfg);
int hal_xspi_reset_clk(hal_xspi_handle *h, u32 reset_clock);
int hal_xspi_set_cmd_width(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines);
int hal_xspi_set_cmd_width(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines);
int hal_xspi_set_cmd(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 cmd);
int hal_xspi_set_addr_width(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u8 bw_3_4_bytes);
int hal_xspi_set_addr(hal_xspi_handle *h, u8 addr);
int hal_xspi_set_dummy(hal_xspi_handle *h, u8 lines, u8 dummy);
int hal_xspi_set_write_cnt(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u32 count);
int hal_xspi_set_read_cnt(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u32 count);
int hal_xspi_start_transfer(hal_xspi_handle *h);
int hal_xspi_transfer_cpu_sync(hal_xspi_handle *h, struct hal_xspi_transfer *t);
int hal_xspi_xip_cfg(hal_xspi_handle *h, hal_xspi_proto_cfg_t xip_proto_cfg);
int hal_xspi_xip_enable(hal_xspi_handle *h);
int hal_xspi_dll_training(hal_xspi_handle *h, u8 sel, u8 reg_icp, void *psram_buf, u32 len);
int hal_xspi_set_dll_ctl(hal_xspi_handle *h, u8 sel, u8 reg_icp, u8 phase_sel);
int hal_xspi_set_phase_sel(hal_xspi_handle *h, u8 sel, u8 phase_sel);

int hal_xspi_set_cs(hal_xspi_handle *h, u8 sel);
int hal_xspi_set_boudary(hal_xspi_handle *h, u8 by);
int hal_xspi_set_parallel_mode(hal_xspi_handle *h, u8 mode);
int hal_xspi_set_timeout(hal_xspi_handle *h, u32 timeout);

#ifdef __cplusplus
}
#endif

#endif
