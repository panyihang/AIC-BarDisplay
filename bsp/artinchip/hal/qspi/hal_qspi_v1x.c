/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <hal_dma.h>
#include <hal_qspi.h>
#include "qspi_internal.h"

#if defined(AIC_QSPI_DRV_V11) || defined(AIC_QSPI_DRV_V12)
#include "qspi_hw_v1.1.h"
#else
#include "qspi_hw_v1.0.h"
#endif

void hal_qspi_master_bit_mode_init(u32 base)
{
#if AIC_SUPPORT_SPI_X_WIRE_IN_BIT_MODE  == 4
    qspi_hw_set_work_mode(base, BMTC_BIT_WM_BIT_STD);
#else
    qspi_hw_set_work_mode(base, BMTC_BIT_WM_BIT_3WIRE);
#endif

    /* set chip select number */
    qspi_hw_bit_mode_set_cs_pol(base, false);
    qspi_hw_bit_mode_set_ss_owner(base, true);
}

int hal_qspi_master_transfer_bit_mode(qspi_master_handle *h, struct qspi_bm_transfer *t)
{
    struct qspi_master_state *qspi;
    u32 base;
    int ret = 0;

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;

    if (!t->rx_bits_len && !t->tx_bits_len)
        return -EINVAL;

    qspi_hw_reset_fifo(base);
    if (t->tx_data && t->rx_data) {
        ret = qspi_hw_bit_mode_send_then_recv(base, t->tx_data, t->tx_bits_len,
                                              t->rx_data, t->rx_bits_len);
    } else {
        if (t->tx_data) {
            ret = qspi_hw_bit_mode_write(base, t->tx_data, t->tx_bits_len);
        }
        if (t->rx_data) {
            ret = qspi_hw_bit_mode_read(base, t->rx_data, t->rx_bits_len);
        }
    }

    return ret; /* In progress */
}

int hal_qspi_master_init(qspi_master_handle *h, struct qspi_master_config *cfg)
{
    struct qspi_master_state *qspi;
    u32 base, sclk, tmp_sclk, cur_clk;
    int ret;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cfg, -EINVAL);

    qspi = (struct qspi_master_state *)h;

    base = qspi_hw_index_to_base(cfg->idx);
    if (base == QSPI_INVALID_BASE) {
        hal_log_err("invalid spi controller index %d\n", cfg->idx);
        return -ENODEV;
    }

    sclk = cfg->clk_in_hz;
    if (sclk > HAL_QSPI_MAX_FREQ_HZ)
        sclk = HAL_QSPI_MAX_FREQ_HZ;
    else if (sclk < HAL_QSPI_INPUT_MIN_FREQ_HZ)
        sclk = HAL_QSPI_INPUT_MIN_FREQ_HZ;
    qspi->idx = cfg->idx;

    tmp_sclk = sclk + HAL_QSPI_HZ_PER_MHZ;
    do {
        tmp_sclk -= HAL_QSPI_HZ_PER_MHZ;
        hal_clk_set_freq(cfg->clk_id, tmp_sclk);
        cur_clk = hal_clk_get_freq(cfg->clk_id);
    } while (cur_clk > sclk);

    show_freq("freq (input)", qspi->idx, cur_clk);
    ret = hal_clk_enable(cfg->clk_id);
    if (ret < 0) {
        hal_log_err("QSPI %d clk enable failed!\n", cfg->idx);
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(cfg->clk_id);
    if (ret < 0) {
        hal_log_err("QSPI %d reset deassert failed!\n", cfg->idx);
        return -EFAULT;
    }

    qspi_hw_init_default(base);
    qspi_hw_set_ctrl_mode(base, QSPI_CTRL_MODE_MASTER);
    qspi_hw_set_tx_delay_mode(base, true);
    qspi_hw_set_rx_delay_mode(base, cfg->rx_dlymode);
    qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    qspi_hw_clear_interrupt_status(base, ICR_BIT_ALL_MSK);
    qspi_hw_set_cpol(base, cfg->cpol);
    qspi_hw_set_cpha(base, cfg->cpha);
    qspi_hw_set_lsb_en(base, cfg->lsb_en);
    qspi_hw_set_3wire_en(base, cfg->wire3_en);
    qspi_hw_set_cs_polarity(base, cfg->cs_polarity);
    if (cfg->cs_polarity == QSPI_CS_POL_VALID_LOW)
        qspi_hw_set_cs_level(base, QSPI_CS_LEVEL_HIGH);
    else
        qspi_hw_set_cs_level(base, QSPI_CS_LEVEL_LOW);
    if (cfg->cs_auto)
        qspi_hw_set_cs_owner(base, QSPI_CS_CTL_BY_HW);
    else
        qspi_hw_set_cs_owner(base, QSPI_CS_CTL_BY_SW);
    qspi_hw_drop_invalid_data(base, QSPI_DROP_INVALID_DATA);
    qspi_hw_reset_fifo(base);
    qspi_hw_set_fifo_watermark(base, QSPI_TX_WATERMARK, QSPI_RX_WATERMARK);

    qspi->clk_id = cfg->clk_id;
    qspi->cb = NULL;
    qspi->cb_priv = NULL;

    if (h->bit_mode) {
        hal_qspi_master_bit_mode_init(base);
    }

    return 0;
}

void hal_qspi_fifo_reset(u32 base, u32 fifo)
{
    if (fifo == HAL_QSPI_RX_FIFO) {
        qspi_hw_reset_rx_fifo(base);
    }
    if (fifo == HAL_QSPI_TX_FIFO) {
        qspi_hw_reset_tx_fifo(base);
    }
}

void hal_qspi_master_fifo_reset(qspi_master_handle *h, u32 fifo)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);
    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    hal_qspi_fifo_reset(base, fifo);
}

int hal_qspi_master_deinit(qspi_master_handle *h)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_master_state *)h;
#ifdef AIC_DMA_DRV
    if (qspi->dma_rx) {
        hal_release_dma_chan(qspi->dma_rx);
        qspi->dma_rx = NULL;
    }
    if (qspi->dma_tx) {
        hal_release_dma_chan(qspi->dma_tx);
        qspi->dma_tx = NULL;
    }
#endif
    qspi->cb = NULL;
    qspi->cb_priv = NULL;
    qspi->async_tx = NULL;
    qspi->async_rx = NULL;
    qspi->async_tx_remain = 0;
    qspi->async_rx_remain = 0;
    return 0;
}

int hal_qspi_master_set_cs(qspi_master_handle *h, u32 cs_num, bool enable)
{
    struct qspi_master_state *qspi;
    u32 base, pol, level;

    CHECK_PARAM(h, -EINVAL);
    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    if (h->bit_mode) {
        pol = qspi_hw_bit_mode_get_cs_pol(base);

        if (enable)
            level = (pol == QSPI_CS_POL_VALID_LOW) ? QSPI_CS_LEVEL_LOW :
                                                     QSPI_CS_LEVEL_HIGH;
        else
            level = (pol == QSPI_CS_POL_VALID_LOW) ? QSPI_CS_LEVEL_HIGH :
                                                     QSPI_CS_LEVEL_LOW;

        qspi_hw_bit_mode_set_cs_num(cs_num, base);
        qspi_hw_bit_mode_set_cs_level(base, level);
        return 0;
    }

    pol = qspi_hw_get_cs_polarity(base);
    if (enable)
        level = (pol == QSPI_CS_POL_VALID_LOW) ? QSPI_CS_LEVEL_LOW :
                                                 QSPI_CS_LEVEL_HIGH;
    else
        level = (pol == QSPI_CS_POL_VALID_LOW) ? QSPI_CS_LEVEL_HIGH :
                                                 QSPI_CS_LEVEL_LOW;
    qspi_hw_select_cs_num(base, cs_num);
    qspi_hw_set_cs_level(base, level);

    return 0;
}

int hal_qspi_master_set_bus_freq(qspi_master_handle *h, u32 bus_hz)
{
    u32 base, sclk, divider, div, cal_clk = 0;
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    sclk = hal_clk_get_freq(qspi->clk_id);

    if (bus_hz > HAL_QSPI_MAX_FREQ_HZ)
        bus_hz = HAL_QSPI_MAX_FREQ_HZ;
    else if (bus_hz < HAL_QSPI_MIN_FREQ_HZ)
        bus_hz = HAL_QSPI_MIN_FREQ_HZ;

    if (h->bit_mode) {
        div = qspi_hw_bit_mode_set_clk(bus_hz, sclk, base);
        cal_clk = sclk / (2 * div);
    } else {
        divider = qspi_master_get_best_div_param(sclk, bus_hz, &div);
        if (divider == 1) {
            cal_clk = sclk/(2*(div + 1));
        } else if (divider == 0) {
            cal_clk = sclk >> div;
        }
        qspi_hw_set_clk_div(base, divider, div);
    }

    show_freq("freq ( bus )", qspi->idx, cal_clk);
    qspi->bus_hz = bus_hz;

    return 0;
}

int hal_qspi_master_set_bus_width(qspi_master_handle *h, u32 bus_width)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_bus_width(base, bus_width);
    qspi->bus_width = bus_width;
    if (qspi->bus_width == 0)
        qspi->bus_width = QSPI_BUS_WIDTH_SINGLE;

    return 0;
}

int qspi_wait_transfer_done(u32 base, u32 tmo)
{
    u32 cnt = 0;

    while (qspi_hw_check_transfer_done(base) == false) {
        aic_udelay(HAL_QSPI_WAIT_DELAY_US);
        cnt++;
        if (cnt > tmo) {
            qspi_reg_dump(base);
            return -ETIMEDOUT;
        }
    }
    return 0;
}

static int qspi_fifo_write_read(u32 base, u8 *tx, u8 *rx, u32 len, u32 tmo)
{
    u32 free_len, dolen, cnt;

    while (len) {
        free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
        cnt = 0;
        while (free_len <= (QSPI_FIFO_DEPTH >> 3)) {
            aic_udelay(HAL_QSPI_WAIT_DELAY_US);
            free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
            cnt++;
            if (cnt > tmo)
                return -ETIMEDOUT;
            continue;
        }
        dolen = min(free_len, len);
        qspi_hw_write_fifo(base, tx, dolen);
        cnt = 0;
        while (qspi_hw_get_rx_fifo_cnt(base) != dolen) {
            aic_udelay(HAL_QSPI_WAIT_DELAY_US);
            cnt++;
            if (cnt > tmo)
                return -ETIMEDOUT;
        }
        qspi_hw_read_fifo(base, rx, dolen);
        tx += dolen;
        rx += dolen;
        len -= dolen;
    }

    /* Data are written to FIFO, waiting all data are sent out */
    cnt = 0;
    while (qspi_hw_get_tx_fifo_cnt(base)) {
        aic_udelay(HAL_QSPI_WAIT_DELAY_US);
        cnt++;
        if (cnt > tmo)
            return -ETIMEDOUT;
    }
    return 0;
}

static int qspi_fifo_write_data(u32 base, u8 *data, u32 len, u32 tmo)
{
    u32 dolen, free_len, cnt;

    while (len) {
        free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
        if (free_len <= (QSPI_FIFO_DEPTH >> 3)) {
            cnt = 0;
            while (free_len <= (QSPI_FIFO_DEPTH >> 3)) {
                aic_udelay(HAL_QSPI_WAIT_DELAY_US);
                free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
                cnt++;
                if (cnt > tmo)
                    return -ETIMEDOUT;
            }
        }
        dolen = min(free_len, len);
        qspi_hw_write_fifo(base, data, dolen);
        data += dolen;
        len -= dolen;
    }

    /* Data are written to FIFO, waiting all data are sent out */
    cnt = 0;
    while (qspi_hw_get_tx_fifo_cnt(base)) {
        aic_udelay(HAL_QSPI_WAIT_DELAY_US);
        cnt++;
        if (cnt > tmo)
            return -ETIMEDOUT;
    }
    return 0;
}

int qspi_fifo_read_data(u32 base, u8 *data, u32 len, u32 tmo)
{
    u32 dolen, cnt = 0;

    while (len) {
        dolen = qspi_hw_get_rx_fifo_cnt(base);
        if (dolen == 0) {
            aic_udelay(HAL_QSPI_WAIT_DELAY_US);
            cnt++;
            if (cnt > tmo)
                return -ETIMEDOUT;
            continue;
        } else if (dolen > len) {
            dolen = len;
        }
        qspi_hw_read_fifo(base, data, dolen);
        data += dolen;
        len -= dolen;
        aic_udelay(HAL_QSPI_WAIT_DELAY_US);
    }

    return 0;
}

static void qspi_check_status(u32 base)
{
    u32 sts, tx_fifo_cnt, rx_fifo_cnt;

    qspi_hw_get_interrupt_status(base, &sts);
    if (sts & ISTS_BIT_ERRS) {
        hal_log_err("Error, status: 0x%x.\n", sts);
        qspi_reg_dump(base);
    }

    tx_fifo_cnt = qspi_hw_get_tx_fifo_cnt(base);
    rx_fifo_cnt = qspi_hw_get_rx_fifo_cnt(base);

    if (tx_fifo_cnt || rx_fifo_cnt) {
        hal_log_err("Error, TX FIFO: %u, RX FIFO: %u.\n", tx_fifo_cnt, rx_fifo_cnt);
        qspi_reg_dump(base);
        qspi_hw_reset_fifo(base);
    }

    qspi_hw_clear_interrupt_status(base, sts);
}

static int qspi_master_transfer_cpu_sync(qspi_master_handle *h,
                                         struct qspi_transfer *t)
{
    u32 base, tmo_cnt, txlen, tx_1line_cnt, rxlen;
    struct qspi_master_state *qspi;
    int ret = 0;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    tmo_cnt = qspi_calc_timeout(qspi->bus_hz, t->data_len);
    /* CPU mode, spend more time */
    tmo_cnt *= 10;
    qspi_hw_reset_fifo(base);

    if (t->tx_data && t->rx_data) {
        if (qspi_hw_get_bus_width(base) != QSPI_BUS_WIDTH_SINGLE) {
            hal_log_err("Full duplex mode did not support.\n");
            ret = -1;
            goto out;
        }
        qspi->work_mode = QSPI_WORK_MODE_SYNC_DUPLEX_CPU;
        qspi_hw_set_transfer_cnt(base, t->data_len, t->data_len, 0, 0);
        qspi_hw_drop_invalid_data(base, QSPI_RECV_ALL_INPUT_DATA);
        qspi_hw_start_transfer(base);
        ret = qspi_fifo_write_read(base, t->tx_data, t->rx_data, t->data_len, tmo_cnt);
        if (ret < 0) {
            hal_log_err("read write fifo failure.\n");
            goto out;
        }
    } else if (t->tx_data) {
        txlen = t->data_len;
        tx_1line_cnt = 0;
        if (qspi_hw_get_bus_width(base) == QSPI_BUS_WIDTH_SINGLE)
            tx_1line_cnt = txlen;
        qspi->work_mode = QSPI_WORK_MODE_SYNC_TX_CPU;
        qspi_hw_set_transfer_cnt(base, txlen, tx_1line_cnt, 0, 0);
        qspi_hw_start_transfer(base);
        ret = qspi_fifo_write_data(base, t->tx_data, txlen, tmo_cnt);
        if (ret < 0) {
            hal_log_err("TX write fifo failure.\n");
            goto out;
        }
    } else if (t->rx_data) {
        rxlen = t->data_len;
        qspi->work_mode = QSPI_WORK_MODE_SYNC_RX_CPU;
        qspi_hw_set_transfer_cnt(base, 0, 0, rxlen, 0);
        qspi_hw_start_transfer(base);
        ret = qspi_fifo_read_data(base, t->rx_data, rxlen, tmo_cnt);
        if (ret < 0) {
            hal_log_err("RX read fifo failure: rxlen %d, tmo_cnt %d.\n", rxlen,
                        tmo_cnt);
            goto out;
        }
    }
    ret = qspi_wait_transfer_done(base, tmo_cnt);
    if (ret < 0) {
        hal_log_err("Wait transfer done timeout.\n");
        goto out;
    }
out:
    qspi_hw_drop_invalid_data(base, QSPI_DROP_INVALID_DATA);
    qspi_check_status(base);

    return ret;
}

#ifdef AIC_DMA_DRV
static const u32 dynamic_dma_table[] = {
#ifdef AIC_QSPI1_DYNAMIC_DMA
    1,
#endif
#ifdef AIC_QSPI2_DYNAMIC_DMA
    2,
#endif
#ifdef AIC_QSPI3_DYNAMIC_DMA
    3,
#endif
};
static bool qspi_master_dynamic_dma(struct qspi_master_state *qspi)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(dynamic_dma_table); i++) {
        if (dynamic_dma_table[i] == qspi->idx)
            return true;
    }
    return false;
}

int hal_qspi_master_dma_config(qspi_master_handle *h,
                               struct qspi_master_dma_config *cfg)
{
    struct aic_dma_chan *rx_chan, *tx_chan;
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cfg, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    if (qspi->dma_rx || qspi->dma_tx) {
        hal_log_err("DMA already init for QSPI.\n");
        return -EINVAL;
    }

    if (qspi_master_dynamic_dma(qspi))
        goto dma_dynamic;

    rx_chan = hal_request_dma_chan();
    if (!rx_chan) {
        hal_log_err("Request dma chan error.\n");
        goto err;
    }

    tx_chan = hal_request_dma_chan();
    if (!tx_chan) {
        hal_log_err("Request dma chan error.\n");
        hal_release_dma_chan(rx_chan);
        rx_chan = NULL;
        goto err;
    }

    qspi->dma_rx = rx_chan;
    qspi->dma_tx = tx_chan;

dma_dynamic:
    qspi->dma_cfg = *cfg;
    qspi->dma_cfg.dev_max_burst = HAL_QSPI_DMA_DEV_MAXBURST;
    qspi->dma_cfg.dev_bus_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    qspi->dma_cfg.mem_max_burst = HAL_QSPI_DMA_MEM_MAXBURST;
    qspi->dma_cfg.mem_bus_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

    return 0;
err:
    if (rx_chan)
        hal_release_dma_chan(rx_chan);
    return -ENODEV;
}

static int qspi_master_can_dma(struct qspi_master_state *qspi,
                               struct qspi_transfer *t)
{
    if (t->data_len <= QSPI_FIFO_DEPTH)
        return 0;
    if (t->data_len % AIC_DMA_ALIGN_SIZE)
        return 0;
    if (t->tx_data) {
        /* Meet DMA's address align requirement. */
        if (((unsigned long)t->tx_data) & (AIC_DMA_ALIGN_SIZE - 1))
            return 0;
    }
    if (t->rx_data) {
        /* Meet DMA's address align requirement. */
        if (((unsigned long)t->rx_data) & (AIC_DMA_ALIGN_SIZE - 1))
            return 0;
    }

    /* Request DMA channel while using it. */
    if (qspi_master_dynamic_dma(qspi)) {
        if (t->tx_data) {
            qspi->dma_tx = hal_request_dma_chan();
            if (qspi->dma_tx == NULL) {
                hal_log_err("TX request dma chan failed.\n");
                return 0;
            }
        }
        if (t->rx_data) {
            qspi->dma_rx = hal_request_dma_chan();
            if (qspi->dma_rx == NULL) {
                hal_log_err("RX request dma chan failed.\n");
                if (qspi->dma_tx) {
                    hal_release_dma_chan(qspi->dma_tx);
                    qspi->dma_tx = NULL;
                }
                return 0;
            }
        }
    }

    return 1;
}

static int qspi_master_wait_dma_done(struct aic_dma_chan *ch, u32 tmo)
{
    u32 left, cnt = 0;

    while (1) {
        if ((hal_dma_chan_tx_status(ch, &left) == DMA_COMPLETE) && (left == 0))
            break;

        aic_udelay(HAL_QSPI_WAIT_DELAY_US);
        cnt++;
        if (cnt > tmo) {
            hal_dma_chan_dump(ch->ch_nr);
            return -ETIMEDOUT;
        }
    }
    return 0;
}

static int qspi_txrx_dma_sync(qspi_master_handle *h,
                                         struct qspi_transfer *t, u32 tmo_cnt)
{
    struct qspi_master_state *qspi;
    struct dma_slave_config dmacfg;
    u32 base;
    int ret;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    if (qspi_hw_get_bus_width(base) != QSPI_BUS_WIDTH_SINGLE) {
            hal_log_err("Full duplex mode did not support.\n");
            return -1;
    }
    qspi->work_mode = QSPI_WORK_MODE_SYNC_DUPLEX_DMA;
    qspi_hw_tx_dma_enable(base);
    qspi_hw_rx_dma_enable(base);
    qspi_hw_set_transfer_cnt(base, t->data_len, t->data_len, 0, 0);
    qspi_hw_drop_invalid_data(base, QSPI_RECV_ALL_INPUT_DATA);

    /* config tx DMA channel */
    dmacfg.direction = DMA_MEM_TO_DEV;
    dmacfg.slave_id = qspi->dma_cfg.port_id;
    dmacfg.src_addr = (unsigned long)t->tx_data;
    dmacfg.dst_addr = (unsigned long)QSPI_REG_TXD(base);

    dmacfg.src_addr_width = qspi->dma_cfg.mem_bus_width;
    dmacfg.src_maxburst = qspi->dma_cfg.mem_max_burst;
    if (!(t->data_len % HAL_QSPI_DMA_4BYTES_LINE))
        dmacfg.dst_addr_width = qspi->dma_cfg.dev_bus_width;
    else
        dmacfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    dmacfg.dst_maxburst = qspi->dma_cfg.dev_max_burst;

    ret = hal_dma_chan_config(qspi->dma_tx, &dmacfg);
    if (ret < 0) {
        hal_log_err("TX dma chan config failure.\n");
        ret = -1;
        goto out_err;
    }
    ret = hal_dma_chan_prep_device(qspi->dma_tx, PTR2U32(QSPI_REG_TXD(base)),
                                    PTR2U32(t->tx_data), t->data_len,
                                    DMA_MEM_TO_DEV);
    if (ret < 0) {
        hal_log_err("TX dma chan prepare failure.\n");
        ret = -1;
        goto out_err;
    }

    ret = hal_dma_chan_start(qspi->dma_tx);
    if (ret < 0) {
        hal_log_err("TX dma chan start failure.\n");
        ret = -1;
        goto out_err;
    }

    /* config rx DMA channel */
    dmacfg.direction = DMA_DEV_TO_MEM;
    dmacfg.slave_id = qspi->dma_cfg.port_id;
    dmacfg.src_addr = (unsigned long)QSPI_REG_RXD(base);
    dmacfg.dst_addr = (unsigned long)t->rx_data;

    if (!(t->data_len % HAL_QSPI_DMA_4BYTES_LINE))
        dmacfg.src_addr_width = qspi->dma_cfg.dev_bus_width;
    else
        dmacfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
    dmacfg.src_maxburst = qspi->dma_cfg.dev_max_burst;
    dmacfg.dst_addr_width = qspi->dma_cfg.mem_bus_width;
    dmacfg.dst_maxburst = qspi->dma_cfg.mem_max_burst;

    ret = hal_dma_chan_config(qspi->dma_rx, &dmacfg);
    if (ret < 0) {
        hal_log_err("RX dma chan config failure.\n");
        ret = -1;
        goto out_err;
    }
    ret = hal_dma_chan_prep_device(qspi->dma_rx, PTR2U32(t->rx_data),
                                    PTR2U32(QSPI_REG_RXD(base)), t->data_len,
                                    DMA_DEV_TO_MEM);
    if (ret < 0) {
        hal_log_err("RX dma chan prepare failure.\n");
        ret = -1;
        goto out_err;
    }
    ret = hal_dma_chan_start(qspi->dma_rx);
    if (ret < 0) {
        hal_log_err("RX dma chan start failure.\n");
        ret = -1;
        goto out_err;
    }

    qspi_hw_start_transfer(base);
    ret = qspi_wait_transfer_done(base, tmo_cnt);
    if (ret < 0) {
        hal_log_err("Wait transfer done timeout.\n");
    }
    ret = qspi_master_wait_dma_done(qspi->dma_rx, tmo_cnt);
    if (ret < 0) {
        hal_log_err("RX wait dma done timeout.\n");
    }

    qspi_hw_drop_invalid_data(base, QSPI_DROP_INVALID_DATA);
    qspi_hw_rx_dma_disable(base);
    qspi_hw_tx_dma_disable(base);
    hal_dma_chan_stop(qspi->dma_rx);
    hal_dma_chan_stop(qspi->dma_tx);
    aicos_dcache_invalid_range((void *)(unsigned long)t->rx_data, t->data_len);
    if (qspi_master_dynamic_dma(qspi)) {
        hal_release_dma_chan(qspi->dma_tx);
        qspi->dma_tx = NULL;
        hal_release_dma_chan(qspi->dma_rx);
        qspi->dma_rx = NULL;
    }

    return ret;
out_err:
    qspi_hw_tx_dma_disable(base);
    qspi_hw_rx_dma_disable(base);
    if (qspi_master_dynamic_dma(qspi)) {
        if (qspi->dma_tx) {
            hal_release_dma_chan(qspi->dma_tx);
            qspi->dma_tx = NULL;
        }
        if (qspi->dma_rx) {
            hal_release_dma_chan(qspi->dma_rx);
            qspi->dma_rx = NULL;
        }
    }
    return ret;
}

static int qspi_master_transfer_dma_sync(qspi_master_handle *h,
                                         struct qspi_transfer *t)
{
    u32 base, tmo_cnt, txlen, tx_1line_cnt, rxlen;
    struct qspi_master_state *qspi;
    struct aic_dma_chan *dma_rx, *dma_tx;
    struct dma_slave_config dmacfg;
    int ret = 0;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    memset(&dmacfg, 0, sizeof(dmacfg));

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    tmo_cnt = qspi_calc_timeout(qspi->bus_hz, t->data_len);
    qspi_hw_reset_fifo(base);

    if (t->tx_data && t->rx_data) {
        ret = qspi_txrx_dma_sync(h, t, tmo_cnt);
    } else if (t->tx_data) {
        txlen = t->data_len;
        tx_1line_cnt = 0;
        if (qspi_hw_get_bus_width(base) == QSPI_BUS_WIDTH_SINGLE)
            tx_1line_cnt = txlen;
        qspi->work_mode = QSPI_WORK_MODE_SYNC_TX_CPU;
        qspi_hw_tx_dma_enable(base);
        qspi_hw_set_transfer_cnt(base, txlen, tx_1line_cnt, 0, 0);
        dma_tx = qspi->dma_tx;

        dmacfg.direction = DMA_MEM_TO_DEV;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)t->tx_data;
        dmacfg.dst_addr = (unsigned long)QSPI_REG_TXD(base);

        dmacfg.src_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.src_maxburst = qspi->dma_cfg.mem_max_burst;
        if (!(txlen % HAL_QSPI_DMA_4BYTES_LINE))
            dmacfg.dst_addr_width = qspi->dma_cfg.dev_bus_width;
        else
            dmacfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        dmacfg.dst_maxburst = qspi->dma_cfg.dev_max_burst;

        ret = hal_dma_chan_config(dma_tx, &dmacfg);
        if (ret < 0) {
            hal_log_err("TX dma chan config failure.\n");
            goto out;
        }
        ret = hal_dma_chan_prep_device(dma_tx, PTR2U32(QSPI_REG_TXD(base)),
                                       PTR2U32(t->tx_data), txlen,
                                       DMA_MEM_TO_DEV);
        if (ret < 0) {
            hal_log_err("TX dma chan prepare failure.\n");
            goto out;
        }
        ret = hal_dma_chan_start(dma_tx);
        if (ret < 0) {
            hal_log_err("TX dma chan start failure.\n");
            goto out;
        }
        qspi_hw_start_transfer(base);
        ret = qspi_master_wait_dma_done(dma_tx, tmo_cnt);
        if (ret < 0) {
            hal_log_err("TX wait dma done timeout.\n");
            goto tx_stop;
        }
        ret = qspi_wait_transfer_done(base, tmo_cnt);
        if (ret < 0) {
            hal_log_err("TX wait transfer done timeout.\n");
            goto tx_stop;
        }
    tx_stop:
        qspi_hw_tx_dma_disable(base);
        hal_dma_chan_stop(dma_tx);
        if (qspi_master_dynamic_dma(qspi)) {
            hal_release_dma_chan(dma_tx);
            qspi->dma_tx = NULL;
        }
    } else if (t->rx_data) {
        rxlen = t->data_len;
        qspi->work_mode = QSPI_WORK_MODE_SYNC_RX_CPU;
        qspi_hw_rx_dma_enable(base);
        qspi_hw_set_transfer_cnt(base, 0, 0, rxlen, 0);
        dma_rx = qspi->dma_rx;

        dmacfg.direction = DMA_DEV_TO_MEM;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)QSPI_REG_RXD(base);
        dmacfg.dst_addr = (unsigned long)t->rx_data;

        if (!(rxlen % HAL_QSPI_DMA_4BYTES_LINE))
            dmacfg.src_addr_width = qspi->dma_cfg.dev_bus_width;
        else
            dmacfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        dmacfg.src_maxburst = qspi->dma_cfg.dev_max_burst;
        dmacfg.dst_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.dst_maxburst = qspi->dma_cfg.mem_max_burst;

        ret = hal_dma_chan_config(dma_rx, &dmacfg);
        if (ret < 0) {
            hal_log_err("RX dma chan config failure.\n");
            goto out;
        }
        ret = hal_dma_chan_prep_device(dma_rx, PTR2U32(t->rx_data),
                                       PTR2U32(QSPI_REG_RXD(base)), rxlen,
                                       DMA_DEV_TO_MEM);
        if (ret < 0) {
            hal_log_err("RX dma chan prepare failure.\n");
            goto out;
        }
        ret = hal_dma_chan_start(dma_rx);
        if (ret < 0) {
            hal_log_err("RX dma chan start failure.\n");
            goto out;
        }
        qspi_hw_start_transfer(base);
        ret = qspi_wait_transfer_done(base, tmo_cnt);
        if (ret < 0) {
            hal_log_err("RX wait transfer done timeout.\n");
            goto rx_stop;
        }
        ret = qspi_master_wait_dma_done(dma_rx, tmo_cnt);
        if (ret < 0) {
            hal_log_err("RX wait dma done timeout.\n");
            goto rx_stop;
        }
    rx_stop:
        qspi_hw_rx_dma_disable(base);
        hal_dma_chan_stop(dma_rx);
        aicos_dcache_invalid_range((void *)(unsigned long)t->rx_data, t->data_len);
        if (qspi_master_dynamic_dma(qspi)) {
            hal_release_dma_chan(dma_rx);
            qspi->dma_rx = NULL;
        }
    }
out:
    if (qspi_master_dynamic_dma(qspi)) {
        if (qspi->dma_tx) {
            hal_release_dma_chan(qspi->dma_tx);
            qspi->dma_tx = NULL;
        }
        if (qspi->dma_rx) {
            hal_release_dma_chan(qspi->dma_rx);
            qspi->dma_rx = NULL;
        }
    }
    qspi_check_status(base);

    return ret;
}
#endif

int hal_qspi_master_transfer_sync(qspi_master_handle *h,
                                  struct qspi_transfer *t)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_master_state *)h;
#ifdef AIC_DMA_DRV
    if (qspi_master_can_dma(qspi, t))
        return qspi_master_transfer_dma_sync(qspi, t);
#endif
    return qspi_master_transfer_cpu_sync(qspi, t);
}

int hal_qspi_master_register_cb(qspi_master_handle *h, qspi_master_async_cb cb,
                                void *priv)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cb, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    qspi->cb = cb;
    qspi->cb_priv = priv;

    return 0;
}

static int qspi_master_transfer_cpu_async(struct qspi_master_state *qspi,
                                          struct qspi_transfer *t)
{
    u32 base, txlen, tx_1line_cnt, rxlen;
    int ret = 0;

    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    qspi_hw_reset_fifo(base);
    qspi_hw_interrupt_disable(base, ICR_BIT_CPU_MSK);
    qspi->status = HAL_QSPI_STATUS_IN_PROGRESS;
    if (t->tx_data && t->rx_data) {
        if (qspi_hw_get_bus_width(base) != QSPI_BUS_WIDTH_SINGLE) {
            hal_log_err("Full duplex mode did not support.\n");
            qspi->status = HAL_QSPI_STATUS_OK;
            ret = -1;
        } else {
            qspi->work_mode = QSPI_WORK_MODE_ASYNC_DUPLEX_CPU;
            qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
            qspi->async_tx = t->tx_data;
            qspi->async_tx_remain = t->data_len;
            qspi->async_rx = t->rx_data;
            qspi->async_rx_remain = t->data_len;
            qspi_hw_set_transfer_cnt(base, t->data_len, t->data_len, 0, 0);
            qspi_hw_drop_invalid_data(base, QSPI_RECV_ALL_INPUT_DATA);
            qspi_hw_start_transfer(base);
        }
    } else if (t->tx_data) {
        txlen = t->data_len;
        tx_1line_cnt = 0;
        if (qspi_hw_get_bus_width(base) == QSPI_BUS_WIDTH_SINGLE)
            tx_1line_cnt = txlen;
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_TX_CPU;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_rx = NULL;
        qspi->async_rx_remain = 0;
        qspi->async_tx = t->tx_data;
        qspi->async_tx_remain = txlen;
        qspi_hw_set_transfer_cnt(base, txlen, tx_1line_cnt, 0, 0);
        qspi_hw_start_transfer(base);
    } else if (t->rx_data) {
        rxlen = t->data_len;
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_RX_CPU;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_tx = NULL;
        qspi->async_tx_remain = 0;
        qspi->async_rx = t->rx_data;
        qspi->async_rx_remain = rxlen;
        qspi_hw_set_transfer_cnt(base, 0, 0, rxlen, 0);
        qspi_hw_start_transfer(base);
    }
    qspi_hw_interrupt_enable(base, ICR_BIT_CPU_MSK);

    return ret;
}

#ifdef AIC_DMA_DRV
static void qspi_master_dma_tx_callback(void *h)
{
    struct qspi_master_state *qspi = h;
    struct aic_dma_chan *dma_tx = qspi->dma_tx;
    u32 base;

    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA)
        return;

    qspi->status |= HAL_QSPI_STATUS_ASYNC_DMA_DONE;
    if (!QSPI_IS_ASYNC_ALL_DONE(qspi->status, qspi->done_mask))
        return;

    /* If handle_async_tdone already processed ALL_DONE, dma_tx is gone */
    if (!qspi->dma_tx)
        return;

    base = qspi_hw_index_to_base(qspi->idx);
    qspi_hw_tx_dma_disable(base);
    hal_dma_chan_stop(dma_tx);
    if (qspi_master_dynamic_dma(qspi)) {
        hal_release_dma_chan(dma_tx);
        qspi->dma_tx = NULL;
    }
    if (qspi->cb)
        qspi->cb(h, qspi->cb_priv);
}

static void qspi_master_dma_rx_callback(void *h)
{
    struct qspi_master_state *qspi = h;
    struct aic_dma_chan *dma_rx = qspi->dma_rx;
    struct aic_dma_task *task = NULL;
    u32 base;

    qspi->status |= HAL_QSPI_STATUS_ASYNC_DMA_DONE;
    if (!QSPI_IS_ASYNC_ALL_DONE(qspi->status, qspi->done_mask))
        return;

    /* For full-duplex: skip if handle_async_tdone already processed
     * completion (TDONE was cleared in the ALL_DONE section) */
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
        if (!(qspi->status & HAL_QSPI_STATUS_ASYNC_TDONE))
            return;
    }

    base = qspi_hw_index_to_base(qspi->idx);
    qspi_hw_rx_dma_disable(base);
    if (dma_rx && dma_rx->desc) {
        unsigned long dst;
        u32 len;

        task = dma_rx->desc;
        dst = task->dst;
        len = task->len;
        aicos_dcache_invalid_range((void *)dst, len);
    }
    hal_dma_chan_stop(dma_rx);
    if (qspi_master_dynamic_dma(qspi)) {
        hal_release_dma_chan(dma_rx);
        qspi->dma_rx = NULL;
    }

    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
        if (qspi->status == HAL_QSPI_STATUS_IN_PROGRESS)
            qspi->status = HAL_QSPI_STATUS_OK;
        else
            qspi->status &= ~HAL_QSPI_STATUS_IN_PROGRESS;
        qspi->status &= ~HAL_QSPI_STATUS_ASYNC_TDONE;
    }

    if (qspi->cb)
        qspi->cb(h, qspi->cb_priv);
}

static int qspi_master_transfer_dma_async(struct qspi_master_state *qspi,
                                          struct qspi_transfer *t)
{
    u32 base, txlen, tx_1line_cnt, rxlen;
    struct aic_dma_chan *dma_rx, *dma_tx;
    struct dma_slave_config dmacfg;
    int ret = 0;

    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    qspi_hw_reset_fifo(base);
    qspi_hw_interrupt_disable(base, ICR_BIT_DMA_MSK);
    qspi->status = HAL_QSPI_STATUS_IN_PROGRESS;
    if (t->tx_data && t->rx_data) {
        if (qspi_hw_get_bus_width(base) != QSPI_BUS_WIDTH_SINGLE) {
            hal_log_err("Full duplex mode did not support.\n");
            ret = -1;
            goto out;
        }
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_DUPLEX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_ALL_DONE;
        qspi_hw_tx_dma_enable(base);
        qspi_hw_rx_dma_enable(base);
        qspi_hw_set_transfer_cnt(base, t->data_len, t->data_len, 0, 0);
        qspi_hw_drop_invalid_data(base, QSPI_RECV_ALL_INPUT_DATA);

        /* config tx DMA channel */
        dma_tx = qspi->dma_tx;
        dmacfg.direction = DMA_MEM_TO_DEV;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)t->tx_data;
        dmacfg.dst_addr = (unsigned long)QSPI_REG_TXD(base);
        dmacfg.src_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.src_maxburst = qspi->dma_cfg.mem_max_burst;
        if (!(t->data_len % HAL_QSPI_DMA_4BYTES_LINE))
            dmacfg.dst_addr_width = qspi->dma_cfg.dev_bus_width;
        else
            dmacfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        dmacfg.dst_maxburst = qspi->dma_cfg.dev_max_burst;

        ret = hal_dma_chan_config(dma_tx, &dmacfg);
        if (ret)
            goto out;
        hal_dma_chan_register_cb(dma_tx, qspi_master_dma_tx_callback, qspi);
        ret = hal_dma_chan_prep_device(dma_tx, PTR2U32(QSPI_REG_TXD(base)),
                                       PTR2U32(t->tx_data), t->data_len,
                                       DMA_MEM_TO_DEV);
        if (ret)
            goto out;
        ret = hal_dma_chan_start(dma_tx);
        if (ret)
            goto out;

        /* config rx DMA channel */
        dma_rx = qspi->dma_rx;
        dmacfg.direction = DMA_DEV_TO_MEM;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)QSPI_REG_RXD(base);
        dmacfg.dst_addr = (unsigned long)t->rx_data;
        if (!(t->data_len % HAL_QSPI_DMA_4BYTES_LINE))
            dmacfg.src_addr_width = qspi->dma_cfg.dev_bus_width;
        else
            dmacfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
        dmacfg.src_maxburst = qspi->dma_cfg.dev_max_burst;
        dmacfg.dst_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.dst_maxburst = qspi->dma_cfg.mem_max_burst;

        ret = hal_dma_chan_config(dma_rx, &dmacfg);
        if (ret)
            goto out;
        hal_dma_chan_register_cb(dma_rx, qspi_master_dma_rx_callback, qspi);
        ret = hal_dma_chan_prep_device(dma_rx, PTR2U32(t->rx_data),
                                       PTR2U32(QSPI_REG_RXD(base)), t->data_len,
                                       DMA_DEV_TO_MEM);
        if (ret)
            goto out;
        ret = hal_dma_chan_start(dma_rx);
        if (ret)
            goto out;

        qspi_hw_start_transfer(base);
    } else if (t->tx_data) {
        txlen = t->data_len;
        tx_1line_cnt = 0;
        if (qspi_hw_get_bus_width(base) == QSPI_BUS_WIDTH_SINGLE)
            tx_1line_cnt = txlen;
        qspi_hw_tx_dma_enable(base);
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_TX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi_hw_set_transfer_cnt(base, txlen, tx_1line_cnt, 0, 0);
        dma_tx = qspi->dma_tx;

        dmacfg.direction = DMA_MEM_TO_DEV;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)t->tx_data;
        dmacfg.dst_addr = (unsigned long)QSPI_REG_TXD(base);

        dmacfg.src_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.src_maxburst = qspi->dma_cfg.mem_max_burst;
        dmacfg.dst_addr_width = qspi->dma_cfg.dev_bus_width;
        dmacfg.dst_maxburst = qspi->dma_cfg.dev_max_burst;

        ret = hal_dma_chan_config(dma_tx, &dmacfg);
        if (ret)
            goto out;
        hal_dma_chan_register_cb(dma_tx, qspi_master_dma_tx_callback, qspi);
        ret = hal_dma_chan_prep_device(dma_tx, PTR2U32(QSPI_REG_TXD(base)),
                                       PTR2U32(t->tx_data), txlen,
                                       DMA_MEM_TO_DEV);
        if (ret)
            goto out;
        ret = hal_dma_chan_start(dma_tx);
        if (ret)
            goto out;
        qspi_hw_start_transfer(base);
    } else if (t->rx_data) {
        rxlen = t->data_len;
        qspi_hw_rx_dma_enable(base);
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_RX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_ALL_DONE;
        qspi_hw_set_transfer_cnt(base, 0, 0, rxlen, 0);
        dma_rx = qspi->dma_rx;

        dmacfg.direction = DMA_DEV_TO_MEM;
        dmacfg.slave_id = qspi->dma_cfg.port_id;
        dmacfg.src_addr = (unsigned long)QSPI_REG_RXD(base);
        dmacfg.dst_addr = (unsigned long)t->rx_data;

        dmacfg.src_addr_width = qspi->dma_cfg.dev_bus_width;
        dmacfg.src_maxburst = qspi->dma_cfg.dev_max_burst;
        dmacfg.dst_addr_width = qspi->dma_cfg.mem_bus_width;
        dmacfg.dst_maxburst = qspi->dma_cfg.mem_max_burst;

        ret = hal_dma_chan_config(dma_rx, &dmacfg);
        if (ret)
            goto out;
        hal_dma_chan_register_cb(dma_rx, qspi_master_dma_rx_callback, qspi);
        ret = hal_dma_chan_prep_device(dma_rx, PTR2U32(t->rx_data),
                                       PTR2U32(QSPI_REG_RXD(base)), rxlen,
                                       DMA_DEV_TO_MEM);
        if (ret)
            goto out;
        ret = hal_dma_chan_start(dma_rx);
        if (ret)
            goto out;
        qspi_hw_start_transfer(base);
    }
out:
    if (ret && qspi_master_dynamic_dma(qspi)) {
        if (qspi->dma_tx) {
            hal_dma_chan_stop(qspi->dma_tx);
            hal_release_dma_chan(qspi->dma_tx);
            qspi->dma_tx = NULL;
        }
        if (qspi->dma_rx) {
            hal_dma_chan_stop(qspi->dma_rx);
            hal_release_dma_chan(qspi->dma_rx);
            qspi->dma_rx = NULL;
        }
    }
    qspi_hw_interrupt_enable(base, ICR_BIT_DMA_MSK);
    return ret;
}
#endif

int hal_qspi_master_transfer_async(qspi_master_handle *h,
                                   struct qspi_transfer *t)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_master_state *)h;
#ifdef AIC_DMA_DRV
    if (qspi_master_can_dma(qspi, t))
        return qspi_master_transfer_dma_async(qspi, t);
#endif
    return qspi_master_transfer_cpu_async(qspi, t);
}

static void handle_tx_fifo_irq(struct qspi_master_state *qspi, u32 base, u32 sts)
{
    u32 dolen, free_len;

    if (!(sts & ISTS_BIT_TF_EMP) && !(sts & ISTS_BIT_TF_RDY))
        return;
    if (!(qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_CPU ||
          qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU))
        return;
    if (!qspi->async_tx || !qspi->async_tx_remain)
        return;

    free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
    dolen = min(free_len, qspi->async_tx_remain);
    qspi_hw_write_fifo(base, qspi->async_tx, dolen);
    qspi->async_tx += dolen;
    qspi->async_tx_remain -= dolen;
}

static void handle_rx_fifo_irq(struct qspi_master_state *qspi, u32 base, u32 sts)
{
    u32 dolen;

    if (!(sts & ISTS_BIT_RF_FUL) && !(sts & ISTS_BIT_RF_RDY) && !(sts & ISTS_BIT_TDONE))
        return;
    if (!(qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_CPU ||
          qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU))
        return;
    if (!qspi->async_rx || !qspi->async_rx_remain)
        return;

    dolen = qspi_hw_get_rx_fifo_cnt(base);
    if (dolen > qspi->async_rx_remain)
        dolen = qspi->async_rx_remain;
    qspi_hw_read_fifo(base, qspi->async_rx, dolen);
    qspi->async_rx += dolen;
    qspi->async_rx_remain -= dolen;
}

static void handle_async_tdone(qspi_master_handle *h, u32 base)
{
    struct qspi_master_state *qspi = (struct qspi_master_state *)h;
#ifdef AIC_DMA_DRV
    struct aic_dma_chan *dma_rx = NULL;
    struct aic_dma_task *task = NULL;
#endif

    if (qspi->work_mode == QSPI_WORK_MODE_FLUSH_DISPLAY) {
        if (qspi->cb)
            qspi->cb(h, qspi->cb_priv);
        return;
    }

    if (qspi->work_mode != QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
        if (qspi->status == HAL_QSPI_STATUS_IN_PROGRESS)
            qspi->status = HAL_QSPI_STATUS_OK;
        else
            qspi->status &= ~HAL_QSPI_STATUS_IN_PROGRESS;
    }
    qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    qspi->async_rx = NULL;
    qspi->async_rx_remain = 0;
    qspi->async_tx = NULL;
    qspi->async_tx_remain = 0;
    qspi->status |= HAL_QSPI_STATUS_ASYNC_TDONE;
#ifdef AIC_DMA_DRV
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
        qspi_hw_drop_invalid_data(base, QSPI_DROP_INVALID_DATA);
        qspi_hw_tx_dma_disable(base);
        if (qspi->dma_tx) {
            hal_dma_chan_stop(qspi->dma_tx);
            if (qspi_master_dynamic_dma(qspi)) {
                hal_release_dma_chan(qspi->dma_tx);
                qspi->dma_tx = NULL;
            }
        }
    }
#endif
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU)
        qspi_hw_drop_invalid_data(base, QSPI_DROP_INVALID_DATA);
    if (QSPI_IS_ASYNC_ALL_DONE(qspi->status, qspi->done_mask)) {
#ifdef AIC_DMA_DRV
        if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_DMA) {
            if (qspi->dma_tx) {
                hal_dma_chan_stop(qspi->dma_tx);
                if (qspi_master_dynamic_dma(qspi)) {
                    hal_release_dma_chan(qspi->dma_tx);
                    qspi->dma_tx = NULL;
                }
            }
        }
        if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_DMA) {
            dma_rx = qspi->dma_rx;
            if (dma_rx && dma_rx->desc) {
                unsigned long dst;
                u32 len;

                task = dma_rx->desc;
                dst = task->dst;
                len = task->len;
                aicos_dcache_invalid_range((void *)dst, len);
            }
            if (dma_rx) {
                hal_dma_chan_stop(dma_rx);
                if (qspi_master_dynamic_dma(qspi)) {
                    hal_release_dma_chan(dma_rx);
                    qspi->dma_rx = NULL;
                }
            }
        }
        if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
            qspi_hw_rx_dma_disable(base);
            dma_rx = qspi->dma_rx;
            if (dma_rx && dma_rx->desc) {
                unsigned long dst;
                u32 len;

                task = dma_rx->desc;
                dst = task->dst;
                len = task->len;
                aicos_dcache_invalid_range((void *)dst, len);
            }
            if (dma_rx) {
                if (qspi_master_dynamic_dma(qspi)) {
                    hal_release_dma_chan(dma_rx);
                    qspi->dma_rx = NULL;
                }
            }
            if (qspi->status == HAL_QSPI_STATUS_IN_PROGRESS)
                qspi->status = HAL_QSPI_STATUS_OK;
            else
                qspi->status &= ~HAL_QSPI_STATUS_IN_PROGRESS;
            qspi->status &= ~HAL_QSPI_STATUS_ASYNC_TDONE;
        }
#endif
        if (qspi->cb)
            qspi->cb(h, qspi->cb_priv);
    }
}

void hal_qspi_master_irq_handler(qspi_master_handle *h)
{
    struct qspi_master_state *qspi;
    u32 base, sts;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    qspi_hw_get_interrupt_status(base, &sts);
    if (sts & ISTS_BIT_TF_UDR)
        qspi->status |= HAL_QSPI_STATUS_TX_UNDER_RUN;
    if (sts & ISTS_BIT_TF_OVF)
        qspi->status |= HAL_QSPI_STATUS_TX_OVER_FLOW;

    handle_tx_fifo_irq(qspi, base, sts);

    if (sts & ISTS_BIT_RF_UDR)
        qspi->status |= HAL_QSPI_STATUS_RX_UNDER_RUN;
    if (sts & ISTS_BIT_RF_OVF)
        qspi->status |= HAL_QSPI_STATUS_RX_OVER_FLOW;
    handle_rx_fifo_irq(qspi, base, sts);

    if (sts & ISTS_BIT_TDONE)
        handle_async_tdone(h, base);

    qspi_hw_clear_interrupt_status(base, sts);
}

int hal_qspi_master_get_status(qspi_master_handle *h)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    qspi = (struct qspi_master_state *)h;
    return (qspi->status) & (~HAL_QSPI_STATUS_INTERNAL_MSK);
}

static int hal_qspi_transfer_rxdelay_mode(qspi_master_handle *h, u32 delay_mode)
{
    struct qspi_master_state *qspi;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_master_state *)h;

    if (delay_mode == RX_SAMP_DLY_AUTO)
        return qspi_hw_freq_to_delay_mode(qspi->bus_hz);

    return delay_mode;
}

void hal_qspi_master_set_rxdelay_mode(qspi_master_handle *h, u32 delay_mode)
{
    struct qspi_master_state *qspi;
    u32 base, mode;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    mode = hal_qspi_transfer_rxdelay_mode(h, delay_mode);

    qspi_hw_set_rx_delay_mode(base, mode);
}

#ifdef AIC_QSPI_DRV_V11
int hal_qspi_master_set_qio_mode(qspi_master_handle *h)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_bus_width(base, 4);
    qspi_hw_set_qio_mode(base);

    return 0;
}

void hal_qspi_master_set_cs_owner(qspi_master_handle *h, u8 soft_hw)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    if (soft_hw == QSPI_CS_CTL_BY_SW) {
        qspi_hw_set_cs_owner(base, QSPI_CS_CTL_BY_SW);
    } else {
        qspi_hw_set_cs_owner(base, QSPI_CS_CTL_BY_HW);
    }
}

void hal_qspi_master_set_xip_burst_cfg(qspi_master_handle *h,
                                       struct qspi_xip_burst_cfg *cfg)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);
    CHECK_PARAM_RET(cfg);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_wrap_len(base, cfg->wrap.auto_wl8, cfg->wrap.auto_wl16,
                         cfg->wrap.auto_wl32, cfg->wrap.auto_wl64);

    qspi_hw_set_btr_cmd_index(base, cfg->cmd_set_burst);
    qspi_hw_set_btr_dummy_byte(base, cfg->cmd_dummy_byte);
    if (cfg->cmd_bits_width)
        qspi_hw_set_btr_width(base, 1);
    if (cfg->wrap_en == HAL_XIP_BURST_WRAPPED_WITH_FIXED_LEN)
        qspi_hw_set_btr_wrap_en(base, 1);
    if (cfg->wrap_en == HAL_XIP_BURST_WRAPPED_WITH_AUTO_SEL_LEN) {
        qspi_hw_set_btr_wrap_en(base, 1);
        qspi_hw_set_btr_auto_wrap_len_en(base, 1);
    }
    qspi_hw_set_btr_burst_wrap(base, cfg->wrap.fixed_len);
    qspi_hw_set_btr_burst_linear(base, cfg->wrap.disable);
}

void hal_qspi_master_set_xip_read_cfg(qspi_master_handle *h,
                                      struct qspi_xip_read_cfg *cfg)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);
    CHECK_PARAM_RET(cfg);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_rcm_cmd_index(base, cfg->read_cmd);
    qspi_hw_set_rcm_dummy_byte(base, cfg->dummy_byte);
    if (cfg->addr_mode)
        qspi_hw_set_rcm_addr_4byte_en(base, 1);
    qspi_hw_set_rcm_rdmode_byte_en(base, 1);
    if (cfg->read_cmd_bypass_en)
        qspi_hw_set_rcm_rdcmd_bypass_en(base, 1);

    qspi_hw_set_rcm_rdcmd_bypass_code(base, cfg->mode.bypass);
    qspi_hw_set_rcm_rdcmd_normal_code(base, cfg->mode.normal);
}

void hal_qspi_master_xip_enable(qspi_master_handle *h, bool enable)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_xip_en(base, enable);
}

/**
 *  spi display
 */

#define SPI_TCFG_REG 0x8
#define SPI_FCTL_REG 0x18

#define SPI_TRANS_CONFIG 0x186
#define SPI_FIFO_CONFIG  0x00200820

void hal_spi_disp_init(qspi_master_handle *h, struct spi_display_init_config *disp_config)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    writel(SPI_TRANS_CONFIG, base + SPI_TCFG_REG);
    writel(SPI_FIFO_CONFIG, base + SPI_FCTL_REG);

    spi_disp_dma_onoff(base, disp_config->dma_en);
    spi_disp_control_en(base);
    spi_disp_mode_choose(base, disp_config->spi_mode);
    spi_disp_en(base, disp_config->disp_on);
}

void hal_spi_disp_set_mode(qspi_master_handle *h, struct spi_display_param *disp_param)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    spi_disp_set_disp_mode(base, disp_param->mtw, disp_param->vsw_en, disp_param->vbp_en, disp_param->vfp_en);
    spi_disp_set_disp_vpara(base, disp_param->len_stride, disp_param->len);
    spi_disp_set_disp_hpara(base, disp_param->hvbp, disp_param->hvld, disp_param->hvfp);
    spi_disp_set_disp_cmd(base, disp_param->cmd_vsw, disp_param->cmd_vbp, disp_param->cmd_vld, disp_param->cmd_vfp);
    spi_disp_set_disp_timeout(base, disp_param->count);
    spi_disp_tx_delay_enable(base);
}

void hal_spi_disp_restart(qspi_master_handle *h, struct spi_display_data *disp_data)
{
    struct qspi_master_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_master_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi->work_mode = QSPI_WORK_MODE_FLUSH_DISPLAY;
    spi_disp_rw_disp_restart(base, disp_data->tx_buf, disp_data->en, disp_data->frm_cnt);
}

#endif /* AIC_QSPI_DRV_V11 */
