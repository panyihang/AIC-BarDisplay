/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_HAL_QSPI_
#define _AIC_HAL_QSPI_
#include <aic_common.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_QSPI_INVALID             (0xFFFFFFFF)
#define HAL_QSPI_BUS_WIDTH_SINGLE    1
#define HAL_QSPI_BUS_WIDTH_DUAL      2
#define HAL_QSPI_BUS_WIDTH_QUAD      4
#define HAL_QSPI_MAX_FREQ_HZ         200000000
#define HAL_QSPI_FPGA_MAX_FREQ_HZ    24000000
#define HAL_QSPI_INPUT_MIN_FREQ_HZ   32000000
#define HAL_QSPI_MIN_FREQ_HZ         100000
#define HAL_QSPI_HZ_PER_MHZ          1000000
#define HAL_QSPI_CPOL_ACTIVE_HIGH    0
#define HAL_QSPI_CPOL_ACTIVE_LOW     1
#define HAL_QSPI_CPHA_FIRST_EDGE     0
#define HAL_QSPI_CPHA_SECOND_EDGE    1
#define HAL_QSPI_CS_POL_VALID_HIGH   0
#define HAL_QSPI_CS_POL_VALID_LOW    1

#define HAL_QSPI_RX_FIFO             0
#define HAL_QSPI_TX_FIFO             1

#define HAL_QSPI_WAIT_DELAY_US       1
#define HAL_QSPI_WAIT_PER_CYCLE      HAL_QSPI_WAIT_DELAY_US
#define HAL_QSPI_DMA_DEV_BURST_ONE   1
#define HAL_QSPI_DMA_DEV_MAXBURST    8
#define HAL_QSPI_DMA_MEM_MAXBURST    16
#define HAL_QSPI_DMA_4BYTES_LINE     4

#define HAL_QSPI_FIFO_DEPTH          64
#define HAL_QSPI_FIFO_DEPTH_V3LITE   32

struct qspi_master_state;
struct qspi_slave_state;
typedef struct qspi_master_state qspi_master_handle;
typedef struct qspi_slave_state qspi_slave_handle;

typedef void (*qspi_master_async_cb)(qspi_master_handle *h, void *priv);
typedef void (*qspi_slave_async_cb)(qspi_slave_handle *h, void *priv);

struct qspi_master_config {
    u32 idx;
    u32 clk_in_hz;
    u32 clk_id;
    bool bit_mode;
    bool wire3_en;
    bool lsb_en;
    bool cs_auto;
    u8 cs_polarity;
    u8 cpol;
    u8 cpha;
    u32 rx_dlymode;
    u32 tx_dlymode;
    u32 max_hz;
    bool slave_en;
    u32 fifo_depth;
    u32 dev_max_burst;
    bool idma_en;
};

struct qspi_master_dma_config {
    u32 port_id;
    u32 mem_bus_width;
    u32 mem_max_burst;
    u32 dev_bus_width;
    u32 dev_max_burst;
};

struct spi_display_init_config {
    u32 dma_en;
    u32 spi_mode;
    u32 disp_on;
};

struct spi_display_param {
    u32 mtw;
    u32 vsw_en;
    u32 vbp_en;
    u32 vfp_en;

    u32 len_stride;
    u32 len;

    u32 hvbp;
    u32 hvld;
    u32 hvfp;

    u32 cmd_vsw;
    u32 cmd_vbp;
    u32 cmd_vld;
    u32 cmd_vfp;

    u32 count;
};

struct spi_display_data {
    void* tx_buf;
    u32 en;
    u32 frm_cnt;
};

struct qspi_slave_config {
    u32 idx;
    u32 clk_in_hz;
    u32 clk_id;
    bool bit_mode;
    bool wire3_en;
    bool lsb_en;
    bool cs_auto;
    u8 cs_polarity;
    u8 cpol;
    u8 cpha;
    u32 rx_dlymode;
    u32 tx_dlymode;
    u32 max_hz;
    bool slave_en;
};

struct qspi_slave_idma_config {
    u32 tx_bus_width;
    u32 tx_max_burst;
    u32 rx_bus_width;
    u32 rx_max_burst;
};

struct qspi_transfer {
    u8 *tx_data;
    u8 *rx_data;
    u32 data_len;
};

#define HAL_QSPI_STATUS_OK           (0)
#define HAL_QSPI_STATUS_IN_PROGRESS  (0x1UL << 0)
#define HAL_QSPI_STATUS_RX_UNDER_RUN (0x1UL << 1)
#define HAL_QSPI_STATUS_RX_OVER_FLOW (0x1UL << 2)
#define HAL_QSPI_STATUS_TX_UNDER_RUN (0x1UL << 3)
#define HAL_QSPI_STATUS_TX_OVER_FLOW (0x1UL << 4)

#define HAL_QSPI_STATUS_RX_READY     (0x1UL << 5)
#define HAL_QSPI_STATUS_CS_INVALID   (0x1UL << 6)
#define HAL_QSPI_STATUS_TRAN_DONE    (0x1UL << 31)
/*
 * HAL QSPI internal state, HAL user should not modify it directly
 */
struct qspi_master_state {
    u32 idx;
    u32 fifo_depth;
    u32 dev_max_burst;
    bool idma_en;
    qspi_master_async_cb cb;
    void *cb_priv;
    volatile u32 status;
    u32 clk_id;
    u32 bus_hz;
    u32 bus_width;
    struct qspi_master_dma_config dma_cfg;
    void * volatile dma_tx;
    void * volatile dma_rx;
    u8 * volatile async_tx; /* Used in Async Non-DMA mode */
    u8 * volatile async_rx; /* Used in Async Non-DMA mode */
    volatile u32 async_tx_remain; /* Used in Async Non-DMA mode */
    volatile u32 async_rx_remain; /* Used in Async Non-DMA mode */
    volatile u32 work_mode;
    volatile u32 done_mask;
    bool bit_mode;
};

struct qspi_slave_state {
    u32 idx;
    qspi_slave_async_cb cb;
    void *cb_priv;
    volatile u32 status;
    u32 clk_id;
    u32 bus_hz;
    u32 bus_width;
    u8 * volatile async_tx; /* Used in Async Non-DMA mode */
    u8 * volatile async_rx; /* Used in Async Non-DMA mode */
    u32 async_tx_wcnt; /* Used in Async mode */
    volatile u32 async_tx_remain; /* Used in Async mode */
    volatile u32 async_rx_remain; /* Used in Async mode */
    u32 async_tx_count; /* Used in Async mode */
    u32 async_rx_count; /* Used in Async mode */
    volatile u32 work_mode;
    volatile u32 done_mask;
    bool bit_mode;
};

struct qspi_bm_transfer {
    u8 *tx_data;
    u8 *rx_data;
    u32 rx_bits_len;
    u32 tx_bits_len;
};

#if defined(AIC_QSPI_DRV_V11) || defined(AIC_QSPI_DRV_V34)
/*
 * SPI NOR device's wrap bits setting value.
 * Should check SPI NOR datasheet to get these values.
 */
struct qspi_xip_wrap_bits {
    /*
    * When config SPI controller select to use fixed wrap length, should
    * set the selected wrap bits value to it.
    */
    u8 fixed_len;
    /*
    * Wrap bits value to disable wrap length
    */
    u8 disable;
    /*
    * Wrap bits value to select wrap length 64bytes
    */
    u8 auto_wl64;
    /*
    * Wrap bits value to select wrap length 32bytes
    */
    u8 auto_wl32;
    /*
    * Wrap bits value to select wrap length 16bytes
    */
    u8 auto_wl16;
    /*
    * Wrap bits value to select wrap length 8bytes
    */
    u8 auto_wl8;
};
#define HAL_XIP_BURST_WRAPPED_DISABLED          0x00
#define HAL_XIP_BURST_WRAPPED_WITH_FIXED_LEN    0x01
#define HAL_XIP_BURST_WRAPPED_WITH_AUTO_SEL_LEN 0x02
/*
 * When Cache enabled, should setup read burst
 */
struct qspi_xip_burst_cfg {
    u8 cmd_set_burst;   /* CMD to set burst with wrap or linear, 77h or C0h */
    u8 cmd_dummy_byte;  /* Dummy byte fot set burst command */
    u8 cmd_bits_width;  /* When CMD is C0h, bits width is needed to set */
    u8 wrap_en;         /* 00: disable, 01 wrap with fixed len, 02 wrap with auto len */
    struct qspi_xip_wrap_bits wrap;
};

struct qspi_xip_read_cmd_mode_byte {
    /*
    * Mode byte code value to set read command bypass mode,
    * User should check SPI NOR datasheet and set it.
    */
    u8 bypass;
    /*
    * Mode byte code value to set read command normal mode,
    * User should check SPI NOR datasheet and set it.
    */
    u8 normal;
};

struct qspi_xip_read_cfg {
    /*
    * QIO or QPI read command value
    */
    u8 read_cmd;
    /*
    * Read command's dummy byte.
    * User should check SPI NOR datasheet and set it.
    */
    u8 dummy_byte;
    u8 addr_mode; /* 0: 3byte address, 1: 4byte address */
    /*
    * When CPU cache is disabled, user can select to enable read command bypass mode
    */
    u8 read_cmd_bypass_en;
    struct qspi_xip_read_cmd_mode_byte mode;
};

int hal_qspi_master_set_qio_mode(qspi_master_handle *h);
void hal_qspi_master_set_cs_owner(qspi_master_handle *h, u8 soft_hw);
void hal_qspi_master_set_xip_burst_cfg(qspi_master_handle *h, struct qspi_xip_burst_cfg *cfg);
void hal_qspi_master_set_xip_read_cfg(qspi_master_handle *h, struct qspi_xip_read_cfg *cfg);
void hal_qspi_master_xip_enable(qspi_master_handle *h, bool enable);

int hal_qspi_slave_init(qspi_slave_handle *h, struct qspi_slave_config *cfg);
int hal_qspi_slave_deinit(qspi_slave_handle *h);
int hal_qspi_slave_set_bus_width(qspi_slave_handle *h, u32 bus_width);
int hal_qspi_slave_idma_config(qspi_slave_handle *h, struct qspi_slave_idma_config *cfg);
int hal_qspi_slave_register_cb(qspi_slave_handle *h, qspi_slave_async_cb cb, void *priv);
int hal_qspi_slave_get_status(qspi_slave_handle *h);
void hal_qspi_slave_irq_handler(qspi_slave_handle *h);
int hal_qspi_slave_transfer_async(qspi_slave_handle *h, struct qspi_transfer *t);
int hal_qspi_slave_transfer_abort(qspi_slave_handle *h);
int hal_qspi_slave_transfer_count(qspi_slave_handle *h);
void hal_qspi_slave_fifo_reset(qspi_slave_handle *h, u32 fifo);

void hal_qspi_show_ists(u32 id, u32 sts);

#endif //AIC_QSPI_DRV_V11

int hal_qspi_master_init(qspi_master_handle *h, struct qspi_master_config *cfg);
int hal_qspi_master_deinit(qspi_master_handle *h);
int hal_qspi_master_set_cs(qspi_master_handle *h, u32 cs_num, bool enable);
int hal_qspi_master_set_bus_freq(qspi_master_handle *h, u32 bus_hz);
int hal_qspi_master_set_bus_width(qspi_master_handle *h, u32 bus_width);
int hal_qspi_master_transfer_sync(qspi_master_handle *h, struct qspi_transfer *t);
int hal_qspi_master_dma_config(qspi_master_handle *h, struct qspi_master_dma_config *cfg);
int hal_qspi_master_register_cb(qspi_master_handle *h, qspi_master_async_cb cb, void *priv);
int hal_qspi_master_transfer_async(qspi_master_handle *h, struct qspi_transfer *t);
int hal_qspi_master_get_status(qspi_master_handle *h);
void hal_qspi_master_irq_handler(qspi_master_handle *h);
void hal_qspi_master_fifo_reset(qspi_master_handle *h, u32 fifo);
void hal_qspi_fifo_reset(u32 base, u32 fifo);

int hal_spi_master_transfer_sync(qspi_master_handle *h, struct qspi_transfer *t);
int hal_spi_master_transfer_async(qspi_master_handle *h, struct qspi_transfer *t);

int hal_qspi_master_transfer_bit_mode(qspi_master_handle *h, struct qspi_bm_transfer *t);

/* set delay mode for RX sampling */
void hal_qspi_master_set_rxdelay_mode(qspi_master_handle *h, u32 delay_mode);

/**
 * spi display
 */
#if defined(AIC_QSPI_DRV_V11) || defined(AIC_QSPI_DRV_V12)
void hal_spi_disp_init(qspi_master_handle *h, struct spi_display_init_config *disp_config);
void hal_spi_disp_set_mode(qspi_master_handle *h, struct spi_display_param *disp_param);
void hal_spi_disp_restart(qspi_master_handle *h, struct spi_display_data *disp_data);
#endif

#ifdef __cplusplus
}
#endif

#endif
