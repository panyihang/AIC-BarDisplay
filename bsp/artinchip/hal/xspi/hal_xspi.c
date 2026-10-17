/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <rtconfig.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <hal_dma.h>
#include <hal_xspi.h>
#include "xspi_hw_v1.0.h"
#include "aic_hal_reset.h"

#define XSPI_WORK_MODE_SYNC_RX_CPU  0
#define XSPI_WORK_MODE_SYNC_TX_CPU  1
#define XSPI_WORK_MODE_ASYNC_RX_CPU 2
#define XSPI_WORK_MODE_ASYNC_TX_CPU 3
#define XSPI_WORK_MODE_ASYNC_RX_DMA 4
#define XSPI_WORK_MODE_ASYNC_TX_DMA 5

int hal_xspi_init(hal_xspi_handle *h, struct hal_xspi_config *cfg)
{
    struct hal_xspi_state *xspi;
    u32 base, sclk;
    int ret;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(cfg, -EINVAL);

    hal_reset_assert(RESET_XSPI);
    ret = hal_reset_deassert(RESET_XSPI);
    if(ret < 0) {
        hal_log_err("xspi clk reset deassert failed");
        return -1;
    }

    xspi = (struct hal_xspi_state *)h;

    base = xspi_hw_index_to_base(cfg->idx);
    if (base == XSPI_INVALID_BASE) {
        hal_log_err("invalid spi controller index %d\n", cfg->idx);
        return -ENODEV;
    }

    /* CLK init */
    sclk = cfg->clk_in_hz;
    if (sclk > HAL_XSPI_MAX_FREQ_HZ)
        sclk = HAL_XSPI_MAX_FREQ_HZ;
    else if (sclk < HAL_XSPI_MIN_FREQ_HZ)
        sclk = HAL_XSPI_MIN_FREQ_HZ;

    hal_clk_disable(cfg->clk_id);
    aic_udelay(10);

    hal_clk_set_freq(cfg->clk_id, sclk);
    ret = hal_clk_enable(cfg->clk_id);
    if (ret < 0) {
        hal_log_err("XSPI %d clk enable failed!\n", cfg->idx);
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(cfg->clk_id);
    if (ret < 0) {
        hal_log_err("XSPI %d reset deassert failed!\n", cfg->idx);
        return -EFAULT;
    }

    if (cfg->cs0_port != 0) {
        xspi_hw_data_pin_override(base, cs0, cfg->cs0_port);
    }
    if (cfg->cs1_port != 0) {
        xspi_hw_data_pin_override(base, cs1, cfg->cs1_port);
    }

    xspi_hw_set_clk_div(base, 0, 0);
    xspi_hw_set_cs_write_hold(base, 8);
    xspi_hw_set_cs_read_hold(base, 2);
    xspi_hw_set_xspi_mode(base, opi);

    /* need to enable wrap burst sliop */
    xspi_hw_set_wrap_burst_split(base, 1);

    xspi_hw_set_boudary_ctl(base, xspi_1k);
    xspi_hw_set_parallel_mode(base, single_mode);
    //xspi_hw_set_cs_sel(base, cs0);

    /* IO drive */
    xspi_hw_set_cs_iocfg(base, cs0, 0x36363636, 0x36363636, 0x36363636, 0x37);
    xspi_hw_set_cs_iocfg(base, cs1, 0x36363636, 0x36363636, 0x36363636, 0x37);

    /* just enable dll, the phase not use */
    xspi_hw_set_dll_ctl(base, cs0, ICP_150_200M, xspi_d_90b);
    xspi_hw_set_dll_ctl(base, cs1, ICP_150_200M, xspi_d_90b);
    xspi_hw_set_xspi_en(base, 1);

    xspi->idx = cfg->idx;
    xspi->clk_id = cfg->clk_id;
    xspi->cb = NULL;
    xspi->cb_priv = NULL;

    return 0;
}

int hal_xspi_reset_clk(hal_xspi_handle *h, u32 reset_clock)
{
    struct hal_xspi_state *xspi;
    u32 base, sclk;
    u32 ret = 0;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);


    /* CLK init */
    sclk = reset_clock;
    if (sclk > HAL_XSPI_MAX_FREQ_HZ)
        sclk = HAL_XSPI_MAX_FREQ_HZ;
    else if (sclk < HAL_XSPI_MIN_FREQ_HZ)
        sclk = HAL_XSPI_MIN_FREQ_HZ;

    hal_clk_disable(xspi->clk_id);
    aic_udelay(10);

    hal_clk_set_freq(xspi->clk_id, sclk);
    ret = hal_clk_enable(xspi->clk_id);
    if (ret < 0) {
        hal_log_err("XSPI %d clk enable failed!\n", xspi->idx);
        return -EFAULT;
    }

    ret = hal_clk_enable_deassertrst(xspi->clk_id);
    if (ret < 0) {
        hal_log_err("XSPI %d reset deassert failed!\n", xspi->idx);
        return -EFAULT;
    }
    (void) base;
    return 0;
}
int hal_xspi_set_cmd_width(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 0);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    if (ddr_sdr_mode == xspi_ddr) {
        ins_h = XSPI_ADDR_DDR;
    } else {
        ins_h = XSPI_ADDR;
    }
    io_h = (xspi_lut_io_t)lines;
    xspi_hw_set_lut_cfg(base, 0, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);

    return 0;
}

int hal_xspi_set_cmd(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 cmd)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 0);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    if (ddr_sdr_mode == xspi_ddr) {
        ins_h = XSPI_CMD_DDR;
    } else {
        ins_h = XSPI_CMD;
    }
    oper_h = cmd;
    xspi_hw_set_lut_cfg(base, 0, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);

    return 0;
}

/*
*   ddr_sdr_mode: see the "typedef enum xspi_lut_ins"
*   lines: IO_1 = 0x01, IO_2 = 0x1, IO_4 = 0x2, IO_8 = 0x3
*   bw_3_4_bytes: 3bytes = 3, 4bytes = 4;
*/
int hal_xspi_set_addr_width(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u8 bw_3_4_bytes)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 0);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    if (ddr_sdr_mode == xspi_ddr) {
        ins_l = XSPI_ADDR_DDR;
    } else {
        ins_l = XSPI_ADDR;
    }
    io_l = (xspi_lut_io_t)lines;
    oper_l = bw_3_4_bytes * 8;
    xspi_hw_set_lut_cfg(base, 0, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);

    return 0;
}

int hal_xspi_set_addr(hal_xspi_handle *h, u8 addr)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_address(base, addr);
    return 0;
}

int hal_xspi_set_dummy(hal_xspi_handle *h, u8 lines, u8 dummy)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 1);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    ins_h = XSPI_DUMMY;
    io_h = (xspi_lut_io_t)lines;
    oper_h = dummy;
    xspi_hw_set_lut_cfg(base, 1, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);

    return 0;
}

int hal_xspi_set_write_cnt(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u32 count)
{

    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 1);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    if (ddr_sdr_mode == xspi_ddr) {
        ins_l = XSPI_WRITE_DDR;
    } else {
        ins_l = XSPI_WRITE;
    }
    io_l = (xspi_lut_io_t)lines;
    oper_l = count-1;
    xspi_hw_set_lut_cfg(base, 1, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);

    return 0;
}

int hal_xspi_set_read_cnt(hal_xspi_handle *h, u8 ddr_sdr_mode, u8 lines, u32 count)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_lut_ins_t ins_h;
    xspi_lut_io_t io_h;
    u32 oper_h;
    xspi_lut_ins_t ins_l;
    xspi_lut_io_t io_l;
    u32 oper_l;
    u32 val = xspi_hw_get_lut_cfg(base, 1);
    ins_h = (val & LUTN_BIT_INS_H_MSK) >> LUTN_BIT_INS_H_OFS;
    io_h = (val & LUTN_BIT_IO_CFG_H_MSK) >> LUTN_BIT_IO_CFG_H_OFS;
    oper_h = (val & LUTN_BIT_OPERAND_H_MSK) >> LUTN_BIT_OPERAND_H_OFS;
    ins_l = (val & LUTN_BIT_INS_L_MSK) >> LUTN_BIT_INS_L_OFS;
    io_l = (val & LUTN_BIT_IO_CFG_L_MSK) >> LUTN_BIT_IO_CFG_L_OFS;
    oper_l = (val & LUTN_BIT_OPERAND_L_MSK) >> LUTN_BIT_OPERAND_L_OFS;

    if (ddr_sdr_mode == xspi_ddr) {
        ins_l = XSPI_READ_DDR;
    } else {
        ins_l = XSPI_READ;
    }
    io_l = (xspi_lut_io_t)lines;
    oper_l = count-1;
    xspi_hw_set_lut_cfg(base, 1, ins_h, io_h, oper_h, ins_l, io_l, oper_l);

    xspi_hw_set_lut_cfg(base, 2,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    xspi_hw_set_lut_cfg(base, 3,XSPI_STOP,IO_1,0x00,XSPI_STOP,IO_1,0x00);
    return 0;
}

int hal_xspi_start_transfer(hal_xspi_handle *h)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_lut_start(base, 0);

    return 0;
}

static int hal_xspi_fifo_write(u32 base, u8 *data, u32 len, u32 tmo)
{
    u32 dolen, free_len;
    u64 start_us;

    start_us = aic_get_time_us();
    while (len) {
        free_len = XSPI_FIFO_DEPTH - xspi_hw_get_tx_fifo_cnt(base);
        if (free_len <= (XSPI_FIFO_DEPTH >> 3)) {
            if ((aic_get_time_us() - start_us) > tmo)
                return -ETIMEDOUT;
            continue;
        }
        dolen = min(free_len, len);
        xspi_hw_write_fifo(base, data, dolen);
        xspi_hw_lut_start(base, 0);
        data += dolen;
        len -= dolen;
    }

    /* Data are written to FIFO, waiting all data are sent out */
    while (xspi_hw_get_tx_fifo_cnt(base)) {
        if ((aic_get_time_us() - start_us) > tmo)
            return -ETIMEDOUT;
    }
    return 0;
}

static int hal_xspi_fifo_read(u32 base, u8 *data, u32 len, u32 tmo_us)
{
    u32 dolen;
    u64 start_us;

    start_us = aic_get_time_us();
    while (len) {
        dolen = xspi_hw_get_rx_fifo_cnt(base);
        if (dolen == 0) {
            if ((aic_get_time_us() - start_us) > tmo_us)
                return -ETIMEDOUT;
            continue;
        } else if (dolen > len) {
            dolen = len;
        }
        xspi_hw_read_fifo(base, data, dolen);
        data += dolen;
        len -= dolen;
    }

    return 0;
}

static u32 hal_xspi_calc_timeout(u32 bus_hz, u32 bw, u32 len)
{
    u32 tmo_us;

    if (bus_hz < HAL_XSPI_MIN_FREQ_HZ)
        tmo_us = (1000000 * (len * 8 / bw)) / bus_hz;
    else if (bus_hz < 1000000)
        tmo_us = (1000 * (len * 8 / bw)) / (bus_hz / 1000);
    else
        tmo_us = (len * 8 / bw) / (bus_hz / 1000000);

    /* Add 100ms time padding */
    tmo_us += 100000;

    return tmo_us;
}

static int hal_xspi_wait_transfer_done(u32 base, u32 tmo)
{
    u64 start_us, cur;

    start_us = aic_get_time_us();
    while (xspi_hw_check_idle_status(base) == XSPI_BUSY) {
        cur = aic_get_time_us();
        if ((cur - start_us) > tmo)
            return -ETIMEDOUT;
    }
    return 0;
}


int hal_xspi_transfer_cpu_sync(hal_xspi_handle *h, struct hal_xspi_transfer *t)
{
    u32 base, tmo_us, txlen, rxlen;// sts, tx_1line_cnt,;
    struct hal_xspi_state *xspi;
    int ret;

    CHECK_PARAM(h, -EINVAL);
    CHECK_PARAM(t, -EINVAL);

    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    if ((t->tx_data == NULL) && (t->rx_data == NULL))
        return -EINVAL;
    if (t->data_len == 0)
        return -EINVAL;

    tmo_us = hal_xspi_calc_timeout(xspi->bus_hz, xspi->bus_width, t->data_len);
    /* CPU mode, spend more time */
    tmo_us *= 10;
    xspi_hw_reset_fifo(base);

    if (t->tx_data) {
        txlen = t->data_len;
        xspi->work_mode = XSPI_WORK_MODE_SYNC_TX_CPU;
        hal_xspi_set_write_cnt(h, xspi_ddr, IO_8, txlen);
        ret = hal_xspi_fifo_write(base, t->tx_data, txlen, tmo_us);
        if (ret < 0) {
            hal_log_err("TX write fifo failure.\n");
            goto out;
        }
    } else if (t->rx_data) {
        rxlen = t->data_len;
        xspi->work_mode = XSPI_WORK_MODE_SYNC_RX_CPU;
        hal_xspi_set_read_cnt(h, xspi_ddr, IO_8, rxlen);
        hal_xspi_start_transfer(h);
        ret = hal_xspi_fifo_read(base, t->rx_data, rxlen, tmo_us);
        if (ret < 0) {
            hal_log_err("RX read fifo failure: rxlen %d, tmo %d.\n", rxlen,
                        tmo_us);
            goto out;
        }
    }
    ret = hal_xspi_wait_transfer_done(base, tmo_us);
    if (ret < 0) {
        hal_log_err("Wait transfer done timeout.\n");
        goto out;
    }
out:
    return ret;
}

int hal_xspi_xip_cfg(hal_xspi_handle *h, hal_xspi_proto_cfg_t xip_proto_cfg)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_lut_lock_ctl(base, LUT_UNLOCK);

    /* write config */
    if(xip_proto_cfg.clk_mode == xspi_ddr) {
        xspi_hw_set_lut_cfg(base, 0, XSPI_CMD_DDR, xip_proto_cfg.wr_cmd_lines, xip_proto_cfg.wr_cmd_val,
                                XSPI_ADDR_DDR, xip_proto_cfg.addr_lines, xip_proto_cfg.addr_width * 8);
        xspi_hw_set_lut_cfg(base, 1, XSPI_DUMMY, IO_8, xip_proto_cfg.wr_dummy,
                                XSPI_WRITE_DDR, xip_proto_cfg.wr_cnt_lines, xip_proto_cfg.wr_cnt-1);
    } else {
        xspi_hw_set_lut_cfg(base, 0, XSPI_CMD, xip_proto_cfg.wr_cmd_lines, xip_proto_cfg.wr_cmd_val,
                                XSPI_ADDR, xip_proto_cfg.addr_lines, xip_proto_cfg.addr_width * 8);
        xspi_hw_set_lut_cfg(base, 1, XSPI_DUMMY, IO_8, xip_proto_cfg.wr_dummy,
                                XSPI_WRITE, xip_proto_cfg.wr_cnt_lines, xip_proto_cfg.wr_cnt-1);
    }
    xspi_hw_set_lut_cfg(base, 2, XSPI_STOP, IO_1, 0x00, XSPI_STOP, IO_1, 0x00);
    xspi_hw_set_lut_cfg(base, 3, XSPI_STOP, IO_1, 0x00, XSPI_STOP, IO_1, 0x00);


    /* read config */
    if(xip_proto_cfg.clk_mode == xspi_ddr) {
        xspi_hw_set_lut_cfg(base, 16, XSPI_CMD_DDR, xip_proto_cfg.rd_cmd_lines, xip_proto_cfg.rd_cmd_val,
                                XSPI_ADDR_DDR, xip_proto_cfg.addr_lines, xip_proto_cfg.addr_width * 8);
        xspi_hw_set_lut_cfg(base, 17, XSPI_DUMMY, IO_8, xip_proto_cfg.rd_dummy,
                                XSPI_READ_DDR, xip_proto_cfg.rd_cnt_lines, xip_proto_cfg.rd_cnt-1);

    } else {
        xspi_hw_set_lut_cfg(base, 16, XSPI_CMD, xip_proto_cfg.rd_cmd_lines, xip_proto_cfg.rd_cmd_val,
                                XSPI_ADDR, xip_proto_cfg.addr_lines, xip_proto_cfg.addr_width * 8);
        xspi_hw_set_lut_cfg(base, 17, XSPI_DUMMY, IO_8, xip_proto_cfg.rd_dummy,
                                XSPI_READ, xip_proto_cfg.rd_cnt_lines, xip_proto_cfg.rd_cnt-1);
    }
    xspi_hw_set_lut_cfg(base, 18, XSPI_STOP, IO_1, 0x00, XSPI_STOP, IO_1, 0x00);
    xspi_hw_set_lut_cfg(base, 19, XSPI_STOP, IO_1, 0x00, XSPI_STOP, IO_1, 0x00);

    xspi_hw_lut_update(base);
    xspi_hw_lut_lock_ctl(base, LUT_LOCK);

    return 0;
}

int hal_xspi_xip_enable(hal_xspi_handle *h)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_xip_en(base, 1);
    return 0;
}

int hal_xspi_set_cs(hal_xspi_handle *h, u8 sel)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_cs_sel(base, sel);

    return 0;
}


int hal_xspi_set_boudary(hal_xspi_handle *h, u8 by)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_boudary_ctl(base, by);
    return 0;
}

int hal_xspi_set_parallel_mode(hal_xspi_handle *h, u8 mode)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_parallel_mode(base, mode);
    return 0;
}

/* set the clock and phase_sel */
int hal_xspi_set_dll_ctl(hal_xspi_handle *h, u8 sel, u8 reg_icp, u8 phase_sel)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_dll_ctl(base, sel, reg_icp, phase_sel);
    return 0;
}

int hal_xspi_set_phase_sel(hal_xspi_handle *h, u8 sel, u8 phase_sel)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_phase_sel(base, sel, phase_sel);
    return 0;
}

int hal_xspi_set_timeout(hal_xspi_handle *h, u32 timeout)
{
    struct hal_xspi_state *xspi;
    u32 base;
    CHECK_PARAM(h, -EINVAL);
    xspi = (struct hal_xspi_state *)h;
    base = xspi_hw_index_to_base(xspi->idx);

    xspi_hw_set_timeout(base, timeout);
    return 0;
}


