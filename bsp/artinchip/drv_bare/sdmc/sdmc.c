/*
 * Copyright (c) 2022-2025, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_log.h>
#include <aic_hal.h>
#include <hal_sdmc.h>
#include <bouncebuf.h>
#include <hal_dma.h>
#include <partition_table.h>
#include "sdmc.h"

#define MSEC_PER_SEC        1000

static u32 aic_sdmc_buswidth(u32 type)
{
    switch (type) {
        case SDMC_CTYPE_8BIT:
            return 8;
        case SDMC_CTYPE_4BIT:
            return 4;
        case SDMC_CTYPE_1BIT:
            return 1;
        default:
        case SDMC_CTYPE_RESERVED:
            pr_warn("Invalid Card type %d\n", type);
            return 1;
    }
}

static u32 aic_sdmc_get_timeout(struct aic_sdmc *host, const u32 size)
{
    unsigned int timeout;

    timeout = size * 8;
    timeout /= aic_sdmc_buswidth(host->pdata->buswidth);
    timeout *= 10;      /* wait 10 times as long */
    timeout /= (host->clock / MSEC_PER_SEC);
    timeout /= host->ddr_mode ? 2 : 1;
    timeout = (timeout < MSEC_PER_SEC) ? MSEC_PER_SEC : timeout;
    return timeout;
}

static int aic_sdmc_data_transfer(struct aic_sdmc *host,
                                  struct aic_sdmc_data *data)
{
    int ret = 0;
    u32 mask, size, total;
    u64 timeout, end, start = aic_get_time_ms();

    total = data->blksize * data->blks;
    timeout = aic_sdmc_get_timeout(host, total);

    size = total / 4;
    for (;;) {
        mask = hal_sdmc_int_stat(&host->host);
        if (mask & (SDMC_DATA_ERR | SDMC_DATA_TOUT)) {
            pr_err("Data error! size %d/%d, mode %s-%s, status 0x%x\n",
                   size * 4, total,
                   host->fifo_mode ? "FIFO" : "IDMA",
                   data->flags == MMC_DATA_READ ? "Read" : "Write", mask);
            data->err = -1;
            ret = -EINVAL;
            break;
        }

        //pr_err("%s: mask %#x, size %d, timeout %d\n",
        //       data->flags == MMC_DATA_READ ? "Read" : "Write", mask, size,
        //       timeout);

        if (host->fifo_mode && size) {
            if (data->flags == MMC_DATA_READ &&
                    (mask & (SDMC_INT_RXDR | SDMC_INT_DAT_DONE))) {
                size = hal_sdmc_data_rx(&host->host, (u32 *)data->buf, size);
            } else if (data->flags == MMC_DATA_WRITE &&
                       (mask & SDMC_INT_TXDR)) {
                size = hal_sdmc_data_tx(&host->host, (u32 *)data->buf, size);
            }
        }

        if (mask & SDMC_INT_DAT_DONE) {
            ret = 0;
            break;
        }

        end = aic_get_time_ms();
        if (end - start > timeout) {
            pr_err("Data timeout %lld - %lld > %lld! size %d/%d, mode %s-%s, status 0x%x\n",
                    end, start,
                    timeout, size * 4, total,
                    host->fifo_mode ? "FIFO" : "IDMA",
                    data->flags == MMC_DATA_READ ? "Read" : "Write", mask);
            data->err = -1;
            ret = -1;//RT_ETIMEOUT;
            break;
        }
    }

    hal_sdmc_int_clr(&host->host, mask);

    return ret;
}

#ifdef AIC_SDMC_IRQ_MODE
static void aic_sdmc_enable_sdio_irq(struct aic_sdmc *host, int en)
{
    hal_sdmc_sdio_irq_enable(&host->host, en);
}
#endif

/* Only enable two types of interrupt: SDIO and IDMA */
irqreturn_t aic_sdmc_irq(int irq, void *arg)
{
#ifdef AIC_SDMC_IRQ_MODE
    struct aic_sdmc *host = (struct aic_sdmc *)arg;
    u32 stat = 0;

    stat = hal_sdmc_int_stat(&host->host);
    pr_debug("SDMC IRQ status: 0x%x\n", stat);

    if (stat & SDMC_INT_FROM_SDIO) {
        hal_sdmc_int_clr(&host->host, SDMC_INT_FROM_SDIO);
        if (host->dev->flags & EXT_CSD_SUP_SDIO_IRQ)
            aic_sdmc_enable_sdio_irq(host, 0);
    }

    /* Then, check the DMA status */
    if (stat & (SDMC_IDMAC_INT_MASK | SDMC_INT_DAT_DONE)) {
        hal_sdmc_idma_update_intstat(&host->host);
        aicos_sem_give(host->host.complete);
    }
#endif
    return IRQ_HANDLED;
}

s32 aic_sdmc_clk_init(struct aic_sdmc *host)
{
    s32 ret = 0;

    /* First, disable SDMC controller to reset FIFO status */
    hal_clk_disable_assertrst(host->clk);
    hal_clk_disable(host->clk);

    ret = hal_clk_get_freq(hal_clk_get_parent(host->clk));
    hal_clk_set_freq(host->clk, host->pdata->clk_freq);
    host->sclk_rate = hal_clk_get_freq(host->clk) / 2;
    pr_info("SDMC%d sclk: %d KHz, parent clk %d KHz\n",
            host->index, host->sclk_rate / 1000, ret / 1000);

    ret = hal_clk_enable(host->clk);
    if (ret < 0) {
        pr_err("SDMC%d clk enable failed!\n", host->index);
        return -1;
    }

    ret = hal_clk_enable_deassertrst(host->clk);
    if (ret < 0) {
        pr_err("SDMC%d reset deassert failed!\n", host->index);
        return -1;
    }

    return 0;
}

static int aic_sdmc_set_transfer_mode(struct aic_sdmc *host,
                                      struct aic_sdmc_data *data)
{
    unsigned long mode = SDMC_CMD_DAT_EXP;

    if (data->flags & MMC_DATA_WRITE)
        mode |= SDMC_CMD_DAT_WR;

    return mode;
}

void aic_sdmc_request(struct aic_sdmc *host, struct aic_sdmc_cmd *cmd,
                             struct aic_sdmc_data *data)
{
    s32 ret = 0, flags = 0, i;
    u32 retry = SDMC_TIMEOUT;
    u32 mask = 0;
    u64 timeout = 500000;
    u64 start = aic_get_time_us();
    struct bounce_buffer bbstate;
    ALLOC_CACHE_ALIGN_BUFFER(struct aic_sdmc_idma_desc, cur_idma,
                             data ? DIV_ROUND_UP(data->blks, 8) : 0);

    if (cmd->cmd_code != MMC_CMD_STOP_TRANSMISSION) {
        while (hal_sdmc_is_busy(&host->host)) {
            if (aic_get_time_us() - start > timeout) {
                pr_warn("Data transfer is busy\n");
                cmd->err = -1;
                return;
            }
        }
    }

    // pr_debug("cmd_code: %02d, arg: %08x, flags: %08x --> ", cmd->cmd_code, cmd->arg, cmd->flags);

    hal_sdmc_int_clr(&host->host, SDMC_INT_ALL);
    if (data) {
        if ((data->blksize * data->blks) < 512)
            host->fifo_mode = 1;
        else
            host->fifo_mode = 0;

        hal_sdmc_set_blk(&host->host, data->blksize, data->blks);
        hal_sdmc_reset(&host->host, SDMC_HCTRL1_FIFO_RESET);
        if (!host->fifo_mode) {
            ret = hal_sdmc_idma_start(&host->host, data->blksize * data->blks,
                                      data->flags == MMC_DATA_READ,
                                      (u32 *)data->buf, &bbstate);
            if (ret) {
                printf("hal_sdmc_idma_start failed.\n");
                cmd->err = -1;
                return;
            }

            hal_sdmc_idma_prepare(&host->host, data->blksize, data->blks,
                                  cur_idma, bbstate.bounce_buffer);
        } else {
            if (hal_sdmc_get_idma_status(&host->host))
                hal_sdmc_idma_disable(&host->host);
        }
    }
#ifdef SDMC_DUMP_CMD
    if (data) {
        printf("fifo_mode: %d, block: %d x %d\n", host->fifo_mode,
                  data->blksize, data->blks);
    } else {
        printf("fifo_mode: %d\n", host->fifo_mode);
    }
#endif

    hal_sdmc_set_arg(&host->host, cmd->arg);

    if (data) {
        flags = aic_sdmc_set_transfer_mode(host, data);
    }

    if (cmd->cmd_code == MMC_CMD_GO_IDLE_STATE)
        flags |= SDMC_CMD_INIT;

    if (cmd->cmd_code == MMC_CMD_STOP_TRANSMISSION)
        flags |= SDMC_CMD_STOP;
    else
        flags |= SDMC_CMD_PRV_DAT_WAIT;

    if (cmd->resp_type & MMC_RSP_PRESENT)
        flags |= SDMC_CMD_RESP_EXP;
    if (cmd->resp_type & MMC_RSP_136)
        flags |= SDMC_CMD_RESP_LEN;
    if (cmd->resp_type & MMC_RSP_CRC)
        flags |= SDMC_CMD_RESP_CRC;

    flags |= (cmd->cmd_code | SDMC_CMD_START | SDMC_CMD_USE_HOLD_REG);
    hal_sdmc_set_cmd(&host->host, flags);

    for (i = 0; i < retry; i++) {
        mask = hal_sdmc_int_stat(&host->host);
        if (mask & SDMC_INT_CMD_DONE) {
            if (!data)
                hal_sdmc_int_clr(&host->host, mask);
            break;
        }
    }
#ifdef SDMC_DUMP_CMD
    printf("Flags: 0x%x, mask: 0x%x\n", flags, mask);
#endif

    if (i == retry) {
        pr_err("CMD%d done timeout.\n", cmd->cmd_code);
        cmd->err = -1;
        return;
    }

    if (mask & SDMC_INT_RTO) {
        pr_debug("CMD%d Response Timeout.\n", cmd->cmd_code);
        cmd->err = -1;
        return;
    } else if (mask & SDMC_INT_RESP_ERR) {
        pr_err("CMD%d Response Error.\n", cmd->cmd_code);
        cmd->err = -1;
        return;
    } else if ((cmd->resp_type & MMC_RSP_CRC) && (mask & SDMC_INT_RCRC)) {
        pr_err("CMD%d Response CRC Error.\n", cmd->cmd_code);
        cmd->err = -1;
        return;
    }

    if ((cmd->resp_type & MMC_RSP_MASK) != MMC_RSP_NONE) {
        hal_sdmc_get_rsp(&host->host, (u32 *)cmd->resp,
                         (cmd->resp_type & MMC_RSP_MASK) == MMC_RSP_R2);
    }
    if (data) {
        ret = aic_sdmc_data_transfer(host, data);

        if (!host->fifo_mode)
            ret = hal_sdmc_idma_stop(&host->host, &bbstate,
                                     data->flags == MMC_DATA_READ);
    }

    return;
}

static u32 aic_sdmc_get_best_div(u32 sclk, u32 target_freq)
{
    u32 down, up, f_down, f_up;

    down = sclk / target_freq;
    up = DIV_ROUND_UP(sclk, target_freq);

    f_down = down == 0 ? sclk : sclk / down;
    f_up = up == 0 ? sclk : sclk / up;

    /* Select the closest div parameter */
    if ((f_down - target_freq) < (target_freq - f_up))
        return down;
    return up;
}

static int aic_sdmc_setup_bus(struct aic_sdmc *host, u32 freq)
{
    u32 mux, div, sclk = host->sclk_rate;

    if ((freq == host->clock) || (freq == 0))
        return 0;

    if (sclk == freq) {
        /* bypass mode */
        mux = 1;
        div = 0;
    } else {
        div = aic_sdmc_get_best_div(sclk, freq);
        if (div <= 4) {
            mux = DIV_ROUND_UP(div, 2);
        } else {
            if (div % 8)
                mux = 2;
            else
                mux = 4;
        }
        div /= mux * 2;
        if (div > SDMC_CLKCTRL_DIV_MAX)
            div = SDMC_CLKCTRL_DIV_MAX;
    }
    aic_sdmc_set_ext_clk_mux(&host->host, mux);
    printf("SDMC%d BW %d, sclk %d KHz, clk expt %d KHz(act %d KHz), div %d-%d\n",
            host->index, aic_sdmc_buswidth(host->pdata->buswidth),
            sclk / 1000, freq / 1000,
            div ? sclk / mux / div / 2 / 1000 : sclk / mux / 1000,
            mux, div * 2);

    hal_sdmc_set_div(&host->host, div);
    hal_sdmc_set_cmd(&host->host,
                    SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);

    if (hal_sdmc_wait_cmd_started(&host->host))
        return -1;

    hal_sdmc_clk_enable(&host->host);
    hal_sdmc_set_cmd(&host->host,
                    SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);

    if (hal_sdmc_wait_cmd_started(&host->host))
        return -1;

    host->clock = freq;
    return 0;
}

int aic_sdmc_init(struct aic_sdmc *host)
{
    if (hal_sdmc_reset(&host->host, SDMC_HCTRL1_RESET_ALL)) {
        pr_err("Failed to reset!\n");
        return -1;
    }

    aic_sdmc_setup_bus(host, host->dev->freq_min);

    hal_sdmc_init(&host->host);
    hal_sdmc_fifo_init(&host->host, &host->host.fifoth_val);
    hal_sdmc_set_phase(&host->host,
                       host->pdata->drv_phase, host->pdata->smp_phase);
    hal_sdmc_clk_enable(&host->host);
    return 0;
}

void aic_sdmc_set_cfg(struct aic_sdmc *host)
{
    aic_sdmc_setup_bus(host, host->dev->clock);

    pr_debug("SDMC%d Buswidth %d, DDR mode %d, Clock: %d KHz\n",
        host->index, aic_sdmc_buswidth(host->pdata->buswidth),
        host->ddr_mode, host->dev->clock / 1000);

    hal_sdmc_set_buswidth(&host->host, host->pdata->buswidth);
    hal_sdmc_set_ddrmode(&host->host, host->ddr_mode);
}
