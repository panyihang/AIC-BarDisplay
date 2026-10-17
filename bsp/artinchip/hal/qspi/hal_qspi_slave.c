/*
 * Copyright (c) 2024-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xuan.Wen <xuan.wen@artinchip.com>
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

#ifdef AIC_CHIP_D13X
#include "qspi_hw_v1.1.h"

int hal_qspi_slave_init(qspi_slave_handle *h, struct qspi_slave_config *cfg)
{
    struct qspi_slave_state *qspi;
    u32 base, sclk;
    int ret;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cfg, -EINVAL);

    qspi = (struct qspi_slave_state *)h;

    base = qspi_hw_index_to_base(cfg->idx);
    if (base == QSPI_INVALID_BASE) {
        hal_log_err("invalid spi controller index %d\n", cfg->idx);
        return -ENODEV;
    }

    sclk = cfg->clk_in_hz;
    if (sclk > HAL_QSPI_MAX_FREQ_HZ)
        sclk = HAL_QSPI_MAX_FREQ_HZ;
    else if (sclk < HAL_QSPI_MIN_FREQ_HZ)
        sclk = HAL_QSPI_MIN_FREQ_HZ;
    qspi->idx = cfg->idx;

    show_freq("freq (input)", qspi->idx, sclk);
    hal_clk_set_freq(cfg->clk_id, sclk);
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
    qspi_hw_set_ctrl_mode(base, QSPI_CTRL_MODE_SLAVE);
    qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
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

    return 0;
}

void hal_qspi_slave_fifo_reset(qspi_slave_handle *h, u32 fifo)
{
    struct qspi_slave_state *qspi;
    u32 base;

    CHECK_PARAM_RET(h);
    qspi = (struct qspi_slave_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    hal_qspi_fifo_reset(base, fifo);
}

int hal_qspi_slave_deinit(qspi_slave_handle *h)
{
    struct qspi_slave_state *qspi;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_slave_state *)h;
    qspi->cb = NULL;
    qspi->cb_priv = NULL;
    qspi->async_tx = NULL;
    qspi->async_rx = NULL;
    qspi->async_tx_remain = 0;
    qspi->async_rx_remain = 0;
    return 0;
}

int hal_qspi_slave_set_bus_width(qspi_slave_handle *h, u32 bus_width)
{
    struct qspi_slave_state *qspi;
    u32 base;

    CHECK_PARAM(h, -EINVAL);

    qspi = (struct qspi_slave_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);

    qspi_hw_set_bus_width(base, bus_width);
    qspi->bus_width = bus_width;
    if (qspi->bus_width == 0)
        qspi->bus_width = QSPI_BUS_WIDTH_SINGLE;

    return 0;
}

int hal_qspi_slave_register_cb(qspi_slave_handle *h, qspi_slave_async_cb cb, void *priv)
{
    struct qspi_slave_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cb, -EINVAL);

    qspi = (struct qspi_slave_state *)h;
    qspi->cb = cb;
    qspi->cb_priv = priv;
    return 0;
}

int hal_qspi_slave_get_status(qspi_slave_handle *h)
{
    struct qspi_slave_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    qspi = (struct qspi_slave_state *)h;
    return (qspi->status) & (~HAL_QSPI_STATUS_INTERNAL_MSK);
}

void hal_qspi_show_ists(u32 id, u32 sts)
{
    if (sts) {
        printf("QSPI%d:\n", id);
    }

    if (sts & ISTS_BIT_RF_RDY)
        printf("  ISTS_BIT_RF_RDY\n");
    if (sts & ISTS_BIT_RF_EMP)
        printf("  ISTS_BIT_RF_EMP\n");
    if (sts & ISTS_BIT_RF_FUL)
        printf("  ISTS_BIT_RF_FUL\n");
    if (sts & ISTS_BIT_TF_RDY)
        printf("  ISTS_BIT_TF_RDY\n");
    if (sts & ISTS_BIT_TF_EMP)
        printf("  ISTS_BIT_TF_EMP\n");
    if (sts & ISTS_BIT_TF_FUL)
        printf("  ISTS_BIT_TF_FUL\n");
    if (sts & ISTS_BIT_RF_OVF)
        printf("  ISTS_BIT_RF_OVF\n");
    if (sts & ISTS_BIT_RF_UDR)
        printf("  ISTS_BIT_RF_UDR\n");
    if (sts & ISTS_BIT_TF_OVF)
        printf("  ISTS_BIT_TF_OVF\n");
    if (sts & ISTS_BIT_TF_UDR)
        printf("  ISTS_BIT_TF_UDR\n");
    if (sts & ISTS_BIT_CS_INV)
        printf("  ISTS_BIT_CS_INV\n");
    if (sts & ISTS_BIT_TDONE)
        printf("  ISTS_BIT_TDONE\n");
}

static void qspi_irq_tx_remain_handle(struct qspi_slave_state *qspi, u32 sts)
{
    u32 base, total, free_len, dolen, imsk;

    base = qspi_hw_index_to_base(qspi->idx);

    if (qspi->async_tx) {
        if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_CPU || qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU) {
            free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
            if (qspi->async_tx_remain) {
                dolen = min(free_len, qspi->async_tx_remain);
                qspi_hw_write_fifo(base, qspi->async_tx, dolen);
                qspi->async_tx += dolen;
                qspi->async_tx_wcnt += dolen;
                qspi->async_tx_remain -= dolen;
            } else {
                imsk = ISTS_BIT_TF_EMP | ISTS_BIT_TF_RDY;
                qspi_hw_interrupt_disable(base, imsk);
            }
            total = qspi->async_tx_remain + qspi->async_tx_wcnt;
            qspi->async_tx_count = total - qspi_hw_get_tx_fifo_cnt(base);
            if ((sts & ISTS_BIT_TDONE) && qspi_hw_get_tx_fifo_cnt(base))
                qspi->async_tx_count -= 4;
        }
    }
}

static void qspi_irq_rx_remain_handle(struct qspi_slave_state *qspi, u32 sts)
{
    u32 base, dolen;

    base = qspi_hw_index_to_base(qspi->idx);

    if (qspi->async_rx && qspi->async_rx_remain) {
        if ((qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_CPU) || qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU) {
            dolen = qspi_hw_get_rx_fifo_cnt(base);
            if (dolen > qspi->async_rx_remain)
                dolen = qspi->async_rx_remain;
            qspi_hw_read_fifo(base, qspi->async_rx, dolen);
            qspi->async_rx += dolen;
            qspi->async_rx_count += dolen;
            qspi->async_rx_remain -= dolen;
        }
    }
}

static void qspi_irq_dma_rx_mantissa_handle(struct qspi_slave_state *qspi)
{
    u32 base, dolen;

    base = qspi_hw_index_to_base(qspi->idx);

    qspi->async_rx_count = qspi->async_rx_remain - qspi_hw_get_idma_rx_len(base);
    qspi->async_rx_remain = qspi_hw_get_idma_rx_len(base);
    aicos_dcache_invalid_range(qspi->async_rx, qspi->async_rx_count);
    qspi_hw_set_idma_rx_en(base, 0);

    dolen = qspi_hw_get_rx_fifo_cnt(base);
    if (dolen == 0)
        return;

    if (dolen > qspi->async_rx_remain)
        dolen = qspi->async_rx_remain;

    qspi_hw_read_fifo(base, qspi->async_rx + qspi->async_rx_count, dolen);

    qspi->async_rx += dolen;
    qspi->async_rx_count += dolen;
    qspi->async_rx_remain -= dolen;
}

void hal_qspi_slave_irq_handler(qspi_slave_handle *h)
{
    struct qspi_slave_state *qspi;
    u32 base, sts, imsk;

    CHECK_PARAM_RET(h);

    qspi = (struct qspi_slave_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    qspi_hw_get_interrupt_status(base, &sts);

    hal_log_debug("status:0x%x\n", sts);

    if (sts & ISTS_BIT_TF_OVF)
        qspi->status |= HAL_QSPI_STATUS_TX_OVER_FLOW;

    /* remain work handle in cpu copy */
    if ((sts & ISTS_BIT_TF_EMP) || (sts & ISTS_BIT_TF_RDY) || (sts & ISTS_BIT_TDONE))
        qspi_irq_tx_remain_handle(qspi, sts);
    if ((sts & ISTS_BIT_RF_FUL) || (sts & ISTS_BIT_RF_RDY) || (sts & ISTS_BIT_TDONE))
        qspi_irq_rx_remain_handle(qspi, sts);

    if (sts & ISTS_BIT_RF_UDR)
        qspi->status |= HAL_QSPI_STATUS_RX_UNDER_RUN;
    if (sts & ISTS_BIT_RF_OVF) {
        /* ignore the RF_OVF in tx. */
        if (!(qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_CPU || qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_DMA))
            qspi->status |= HAL_QSPI_STATUS_RX_OVER_FLOW;
    }

    if (sts & ISTS_BIT_TDONE) {
        if (qspi->status == HAL_QSPI_STATUS_IN_PROGRESS)
            qspi->status = HAL_QSPI_STATUS_OK;
        else
            qspi->status &= ~HAL_QSPI_STATUS_IN_PROGRESS;
        imsk = ICR_BIT_ALL_MSK;
        imsk &= ~ICR_BIT_TDONE_INTE;
        imsk &= ~ICR_BIT_CS_INV_INTE;
        qspi_hw_interrupt_disable(base, imsk);
        qspi->status |= HAL_QSPI_STATUS_ASYNC_TDONE;
        if (QSPI_IS_ASYNC_ALL_DONE(qspi->status, qspi->done_mask)) {
            if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_DMA || qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
                qspi_irq_dma_rx_mantissa_handle(qspi);
            }
            if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_DMA || qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA) {
                qspi->async_tx_count = qspi->async_tx_remain - qspi_hw_get_idma_tx_len(base);
            }
            if (qspi->cb)
                qspi->cb(h, qspi->cb_priv);
        }
    }
    qspi_hw_clear_interrupt_status(base, sts);
}

int qspi_slave_transfer_cpu_async(struct qspi_slave_state *qspi,
                                         struct qspi_transfer *t)
{
    u32 base, txlen, rxlen;
    u32 free_len, dolen;
    int ret = 0;

    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    qspi_hw_reset_fifo(base);
    qspi_hw_ctrl_reset(base);
    qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    qspi->status = HAL_QSPI_STATUS_IN_PROGRESS;
    if (t->tx_data && t->rx_data) {
        if (qspi->bus_width > 1) {
            hal_log_err("Full duplex mode did not support. bus width: %d\n", qspi->bus_width);
            return -1;
        }
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_DUPLEX_CPU;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_rx = t->rx_data;
        qspi->async_rx_count = 0;
        qspi->async_rx_remain = t->data_len;
        qspi->async_tx = t->tx_data;
        qspi->async_tx_count = 0;
        qspi->async_tx_wcnt = 0;
        qspi->async_tx_remain = t->data_len;

        free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
        if (qspi->async_tx_remain) {
            dolen = min(free_len, qspi->async_tx_remain);
            qspi_hw_write_fifo(base, qspi->async_tx, dolen);
            qspi->async_tx += dolen;
            qspi->async_tx_wcnt += dolen;
            qspi->async_tx_remain -= dolen;
        }

        qspi_hw_set_slave_output_en(base, 1);
        qspi_hw_interrupt_enable(base, ICR_BIT_ERRS | ICR_BIT_TDONE_INTE |
                                           ISTS_BIT_TF_RDY | ISTS_BIT_TF_EMP |
                                           ICR_BIT_CS_INV_INTE);
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
    } else if (t->tx_data) {
        txlen = t->data_len;
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_TX_CPU;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_rx = NULL;
        qspi->async_rx_count = 0;
        qspi->async_rx_remain = 0;
        qspi->async_tx = t->tx_data;
        qspi->async_tx_count = 0;
        qspi->async_tx_wcnt = 0;
        qspi->async_tx_remain = txlen;

        free_len = QSPI_FIFO_DEPTH - qspi_hw_get_tx_fifo_cnt(base);
        if (qspi->async_tx_remain) {
            dolen = min(free_len, qspi->async_tx_remain);
            qspi_hw_write_fifo(base, qspi->async_tx, dolen);
            qspi->async_tx += dolen;
            qspi->async_tx_wcnt += dolen;
            qspi->async_tx_remain -= dolen;
        }

        if (qspi->bus_width > 1)
            qspi_hw_set_slave_output_en(base, 1);
        else
            qspi_hw_set_slave_output_en(base, 0);
        qspi_hw_interrupt_enable(base, ICR_BIT_ERRS | ICR_BIT_TDONE_INTE |
                                           ISTS_BIT_TF_RDY | ISTS_BIT_TF_EMP |
                                           ICR_BIT_CS_INV_INTE);
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
    } else if (t->rx_data) {
        rxlen = t->data_len;
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_RX_CPU;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_tx = NULL;
        qspi->async_tx_count = 0;
        qspi->async_tx_remain = 0;
        qspi->async_rx = t->rx_data;
        qspi->async_rx_count = 0;
        qspi->async_rx_remain = rxlen;
        qspi_hw_set_slave_output_en(base, 0);
        qspi_hw_interrupt_enable(base, ICR_BIT_ERRS | ICR_BIT_TDONE_INTE |
                                           ICR_BIT_CS_INV_INTE);
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
    }

    return ret;
}

static int qspi_slave_transfer_dma_async(struct qspi_slave_state *qspi, struct qspi_transfer *t)
{
    u32 base, txlen, rxlen, imsk;
    int ret = 0;

    base = qspi_hw_index_to_base(qspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    qspi_hw_reset_fifo(base);
    qspi_hw_ctrl_reset(base);
    qspi_hw_set_idma_busrt_auto_len_en(base, 1);
    qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    qspi->status = HAL_QSPI_STATUS_IN_PROGRESS;
    if (t->tx_data && t->rx_data) {
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_DUPLEX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        qspi->async_tx_remain = t->data_len;
        qspi->async_tx = t->tx_data;
        qspi->async_rx = t->rx_data;
        qspi_hw_set_slave_output_en(base, 1);
        aicos_dcache_clean_range(qspi->async_tx, t->data_len);
        qspi_hw_set_idma_tx_addr(base, (u32)t->tx_data);
        qspi_hw_set_idma_tx_len(base, (u32)t->data_len);
        qspi_hw_set_idma_tx_en(base, 1);

        qspi->async_rx_remain = t->data_len;
        qspi->async_rx = t->rx_data;
        qspi_hw_set_idma_rx_addr(base, (u32)t->rx_data);
        qspi_hw_set_idma_rx_len(base, (u32)t->data_len);
        qspi_hw_set_idma_rx_en(base, 1);
        qspi_hw_interrupt_enable(base, ICR_BIT_IDMA_MSK | ICR_BIT_CS_INV_INTE);
    } else if (t->tx_data) {
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_TX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        txlen = t->data_len;
        qspi->async_tx_remain = txlen;
        qspi->async_tx = t->tx_data;
        aicos_dcache_clean_range(qspi->async_tx, txlen);
        if (qspi->bus_width > 1)
            qspi_hw_set_slave_output_en(base, 1);
        qspi_hw_set_idma_tx_addr(base, (u32)t->tx_data);
        qspi_hw_set_idma_tx_len(base, (u32)txlen);
        qspi_hw_set_idma_tx_en(base, 1);
        imsk = ICR_BIT_IDMA_MSK | ICR_BIT_CS_INV_INTE;
        imsk &= ~ISTS_BIT_RF_OVF;
        qspi_hw_interrupt_enable(base, imsk);
    } else if (t->rx_data) {
        qspi->work_mode = QSPI_WORK_MODE_ASYNC_RX_DMA;
        qspi->done_mask = HAL_QSPI_STATUS_ASYNC_TDONE;
        rxlen = t->data_len;
        qspi->async_rx_remain = rxlen;
        qspi->async_rx = t->rx_data;
        qspi_hw_set_slave_output_en(base, 0);
        qspi_hw_set_idma_rx_addr(base, (u32)t->rx_data);
        qspi_hw_set_idma_rx_len(base, (u32)rxlen);
        qspi_hw_set_idma_rx_en(base, 1);
        imsk = ICR_BIT_IDMA_MSK | ICR_BIT_CS_INV_INTE;
        imsk &= ~ISTS_BIT_TF_UDR;
        qspi_hw_interrupt_enable(base, imsk);
    }
    qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
    return ret;
}

static int qspi_slave_can_dma(struct qspi_slave_state *qspi, struct qspi_transfer *t)
{
#ifdef AIC_DMA_DRV
    if (t->tx_data) {
        /* Meet DMA's address align requirement */
        if (((unsigned long)t->tx_data) & (AIC_DMA_ALIGN_SIZE - 1))
            return 0;
    }
    if (t->rx_data) {
        /* Meet DMA's address align requirement */
        if (((unsigned long)t->rx_data) & (AIC_DMA_ALIGN_SIZE - 1))
            return 0;
    }
    if (t->data_len < QSPI_FIFO_DEPTH)
        return 0;
    return 1;
#else
    return 0;
#endif
}

int hal_qspi_slave_transfer_async(qspi_slave_handle *h, struct qspi_transfer *t)
{
    struct qspi_slave_state *qspi;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    qspi = (struct qspi_slave_state *)h;
    if (qspi_slave_can_dma(qspi, t))
        return qspi_slave_transfer_dma_async(qspi, t);
    return qspi_slave_transfer_cpu_async(qspi, t);
}

int hal_qspi_slave_transfer_abort(qspi_slave_handle *h)
{
    struct qspi_slave_state *qspi;
    u32 base;

    qspi = (struct qspi_slave_state *)h;
    base = qspi_hw_index_to_base(qspi->idx);
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_CPU) {
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
        qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    }
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_CPU) {
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
        qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
    }
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_DMA) {
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
        qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
        qspi_hw_set_idma_rx_en(base, 0);
        qspi_hw_set_idma_rx_len(base, 0);
    }
    if (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_DMA) {
        qspi_hw_clear_interrupt_status(base, ISTS_BIT_ALL_MSK);
        qspi_hw_interrupt_disable(base, ICR_BIT_ALL_MSK);
        qspi_hw_set_idma_tx_en(base, 0);
        qspi_hw_set_idma_tx_len(base, 0);
    }
    return 0;
}

int hal_qspi_slave_transfer_count(qspi_slave_handle *h)
{
    struct qspi_slave_state *qspi;

    qspi = (struct qspi_slave_state *)h;
    if ((qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_CPU) ||
        (qspi->work_mode == QSPI_WORK_MODE_ASYNC_RX_DMA) ||
        (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_CPU) ||
        (qspi->work_mode == QSPI_WORK_MODE_ASYNC_DUPLEX_DMA)) {
        return qspi->async_rx_count;
    }
    if ((qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_CPU) ||
        (qspi->work_mode == QSPI_WORK_MODE_ASYNC_TX_DMA)) {
        return qspi->async_tx_count;
    }
    return -1;
}

#endif /* AIC_CHIP_D13X */
