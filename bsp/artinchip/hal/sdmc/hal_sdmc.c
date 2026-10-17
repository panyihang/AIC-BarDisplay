/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"

#include "bouncebuf.h"
#include "hal_sdmc.h"

#define PAGE_SIZE           4096

static inline void sdmc_writel(struct aic_sdmc_host *host, int reg, u32 val)
{
    writel(val, host->base + reg);
}

static inline u32 sdmc_readl(struct aic_sdmc_host *host, int reg)
{
    return readl(host->base + reg);
}

#ifdef AIC_SDMC_IRQ_MODE
void hal_sdmc_idma_update_intstat(struct aic_sdmc_host *host)
{
    u32 ctrl = 0;

    ctrl = sdmc_readl(host, SDMC_IDMAST);

    sdmc_writel(host, SDMC_IDMAST, ctrl);
}
#endif

static void hal_sdmc_idma_prepare_desc(struct aic_sdmc_idma_desc *idmac,
                                       u32 flags, u32 cnt, u32 addr)
{
    struct aic_sdmc_idma_desc *desc = idmac;

    desc->flags = flags;
    desc->cnt = cnt;
    desc->addr = addr;
    desc->next_addr = (ulong)desc + sizeof(struct aic_sdmc_idma_desc);
}

int hal_sdmc_idma_start(struct aic_sdmc_host *host, u32 size, u32 read,
                        u32 *buf, struct bounce_buffer *bbstate)
{
    if (read)
        return bounce_buffer_start(bbstate, (void *)buf, size, GEN_BB_WRITE);
    else
        return bounce_buffer_start(bbstate, (void *)buf, size, GEN_BB_READ);
}

int hal_sdmc_idma_stop(struct aic_sdmc_host *host,
                       struct bounce_buffer *bbstate, u32 read)
{
    u32 i = 0, mask, ctrl, retry = SDMC_TIMEOUT;
    s32 ret = 0;

    if (read)
        mask = SDMC_IDMAC_INT_RI;
    else
        mask = SDMC_IDMAC_INT_TI;

#ifdef AIC_SDMC_IRQ_MODE
    host->is_busy = 1;
    ret = aicos_sem_take(host->complete, 2000);
#endif

    ctrl = sdmc_readl(host, SDMC_HCTRL1);
    ctrl &= ~(SDMC_HCTRL1_DMA_EN);
    sdmc_writel(host, SDMC_HCTRL1, ctrl);

    bounce_buffer_stop(bbstate);

#ifdef AIC_SDMC_IRQ_MODE
    host->is_busy = 0;
#endif

    if (ret < 0 || i == retry) {
        pr_warn("%s interrupt timeout.\n", mask == SDMC_IDMAC_INT_RI ? "Rx" : "Tx");
        return -1;
    }
    return 0;
}

void hal_sdmc_idma_disable(struct aic_sdmc_host *host)
{
    u32 temp;

    temp = sdmc_readl(host, SDMC_HCTRL1);
    temp &= ~SDMC_HCTRL1_USE_IDMAC;
    temp |= SDMC_HCTRL1_DMA_RESET;
    sdmc_writel(host, SDMC_HCTRL1, temp);

    temp = sdmc_readl(host, SDMC_PBUSCFG);
    temp &= ~(SDMC_PBUSCFG_IDMAC_EN | SDMC_PBUSCFG_IDMAC_FB);
    temp |= SDMC_PBUSCFG_IDMAC_SWR;
    sdmc_writel(host, SDMC_PBUSCFG, temp);
}

u8 hal_sdmc_get_idma_status(struct aic_sdmc_host *host)
{
    return (sdmc_readl(host, SDMC_PBUSCFG) & SDMC_PBUSCFG_IDMAC_EN);
}

void hal_sdmc_idma_prepare(struct aic_sdmc_host *host,
                           u32 blksize, u32 blks,
                           struct aic_sdmc_idma_desc *cur_idma,
                           void *bounce_buffer)
{
    unsigned long ctrl;
    unsigned int i = 0, flags, cnt, blk_cnt = blks;
    ulong data_start, data_end;
    unsigned int blk_cnt_in_page = PAGE_SIZE / blksize;

    data_start = (ulong)cur_idma;
    sdmc_writel(host, SDMC_IDMASADDR, (ulong)cur_idma);

    do {
        flags = SDMC_IDMAC_OWN | SDMC_IDMAC_CH;
        flags |= (i == 0) ? SDMC_IDMAC_FS : 0;
        if (blk_cnt <= blk_cnt_in_page) {
            flags |= SDMC_IDMAC_LD;
            cnt = blksize * blk_cnt;
        } else {
            cnt = blksize * blk_cnt_in_page;
        }

        hal_sdmc_idma_prepare_desc(cur_idma, flags, cnt,
                                   (ulong)bounce_buffer +
                                   (i * PAGE_SIZE));

        cur_idma++;
        if (blk_cnt <= blk_cnt_in_page)
            break;
        blk_cnt -= blk_cnt_in_page;
        i++;
    } while (1);

    data_end = (ulong)cur_idma;
    aicos_dcache_clean_invalid_range((ulong *)data_start,
        roundup(data_end - data_start, ARCH_DMA_MINALIGN));

    ctrl = sdmc_readl(host, SDMC_HCTRL1);
    ctrl |= SDMC_HCTRL1_USE_IDMAC | SDMC_HCTRL1_DMA_RESET;
    sdmc_writel(host, SDMC_HCTRL1, ctrl);

    ctrl = sdmc_readl(host, SDMC_PBUSCFG);
    ctrl |= SDMC_PBUSCFG_IDMAC_FB | SDMC_PBUSCFG_IDMAC_EN;
    sdmc_writel(host, SDMC_PBUSCFG, ctrl);
}

int hal_sdmc_fifo_rdy(struct aic_sdmc_host *host, u32 bit, u32 *len)
{
    u64 start_us;

    start_us = aic_get_time_us();
    *len = sdmc_readl(host, SDMC_CTRST);
    while (*len & bit) {
        *len = sdmc_readl(host, SDMC_CTRST);
        if ((aic_get_time_us() - start_us) > SDMC_TIMEOUT) {
            pr_warn("%s() - FIFO underflow timeout\n", __func__);
            return -1;
        }
    }

    return 0;
}

int hal_sdmc_data_rx(struct aic_sdmc_host *host, u32 *buf, u32 size)
{
    u32 i, len = 0;

    sdmc_writel(host, SDMC_OINTST, SDMC_INT_RXDR | SDMC_INT_DAT_DONE);
    while (size) {
        if (hal_sdmc_fifo_rdy(host, SDMC_CTRST_FIFO_EMPTY, &len) < 0)
            return size;

        len = SDMC_CTRST_FCNT(len);
        len = min(size, len);
        for (i = 0; i < len; i++)
            *buf++ = sdmc_readl(host, SDMC_FIFO_DATA);
        size = size > len ? (size - len) : 0;
    }

    return size;
}

int hal_sdmc_data_tx(struct aic_sdmc_host *host, u32 *buf, u32 size)
{
    u32 i, len = 0;
    u32 fifo_depth = (((host->fifoth_val & RX_WMARK_MASK) >>
                       RX_WMARK_SHIFT) + 1) * 2;

    while (size) {
        if (hal_sdmc_fifo_rdy(host, SDMC_CTRST_FIFO_FULL, &len) < 0)
            return size;

        len = fifo_depth - SDMC_CTRST_FCNT(len);
        len = min(size, len);
        for (i = 0; i < len; i++)
            sdmc_writel(host, SDMC_FIFO_DATA, *buf++);
        size = size > len ? (size - len) : 0;
    }

    sdmc_writel(host, SDMC_OINTST, SDMC_INT_TXDR);
    return size;
}

u32 hal_sdmc_int_stat(struct aic_sdmc_host *host)
{
    return sdmc_readl(host, SDMC_OINTST);
}

void hal_sdmc_int_clr(struct aic_sdmc_host *host, u32 mask)
{
    sdmc_writel(host, SDMC_OINTST, mask);
}

void hal_sdmc_set_blk(struct aic_sdmc_host *host, u32 blksize, u32 blks)
{
    sdmc_writel(host, SDMC_BLKSIZ, blksize);
    sdmc_writel(host, SDMC_BLKCNT, blks);
}

void hal_sdmc_set_arg(struct aic_sdmc_host *host, u32 arg)
{
    sdmc_writel(host, SDMC_CMDARG, arg);
}

void hal_sdmc_set_cmd(struct aic_sdmc_host *host, u32 cmd)
{
    sdmc_writel(host, SDMC_CMD, cmd);
}

u32 hal_sdmc_wait_cmd_started(struct aic_sdmc_host *host)
{
    int status, timeout = SDMC_TIMEOUT;

    do {
        status = sdmc_readl(host, SDMC_CMD);
        if (timeout-- < 0) {
            pr_warn("Timeout!\n");
            return -1;
        }
    } while (status & SDMC_CMD_START);

    return 0;
}

void hal_sdmc_get_rsp(struct aic_sdmc_host *host, u32 *buf, u32 all)
{
    if (all) {
        buf[0] = sdmc_readl(host, SDMC_RESP3);
        buf[1] = sdmc_readl(host, SDMC_RESP2);
        buf[2] = sdmc_readl(host, SDMC_RESP1);
        buf[3] = sdmc_readl(host, SDMC_RESP0);
    } else {
        buf[0] = sdmc_readl(host, SDMC_RESP0);
    }
}

int hal_sdmc_reset(struct aic_sdmc_host *host, u32 value)
{
    u64 start_us;
    u32 ctrl;

    ctrl = sdmc_readl(host, SDMC_HCTRL1);
    ctrl |= value;
    sdmc_writel(host, SDMC_HCTRL1, ctrl);

    start_us = aic_get_time_us();
    while (1) {
        ctrl = sdmc_readl(host, SDMC_HCTRL1);
        if (!(ctrl & value))
            return 0;

        if ((aic_get_time_us() - start_us) > SDMC_TIMEOUT)
            break;
    }

    return -ETIME;
}

int hal_sdmc_is_busy(struct aic_sdmc_host *host)
{
    return sdmc_readl(host, SDMC_CTRST) & SDMC_CTRST_BUSY;
}

void aic_sdmc_set_ext_clk_mux(struct aic_sdmc_host *host, u32 mux)
{
    u32 temp = sdmc_readl(host, SDMC_DLYCTRL);

    temp &= ~SDMC_DLYCTRL_EXT_CLK_MUX_MASK;
    switch (mux) {
    case 1:
        temp |= (u32)SDMC_DLYCTRL_EXT_CLK_MUX_1 <<
                SDMC_DLYCTRL_EXT_CLK_MUX_SHIFT;
        break;
    case 2:
        temp |= SDMC_DLYCTRL_EXT_CLK_MUX_2 << SDMC_DLYCTRL_EXT_CLK_MUX_SHIFT;
        break;
    case 4:
    default:
        temp |= SDMC_DLYCTRL_EXT_CLK_MUX_4 << SDMC_DLYCTRL_EXT_CLK_MUX_SHIFT;
        break;
    }

    sdmc_writel(host, SDMC_DLYCTRL, temp);
}

void hal_sdmc_set_phase(struct aic_sdmc_host *host, u32 drv, u32 smp)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_DLYCTRL);
    temp &= ~SDMC_DLYCTRL_CLK_DRV_PHA_MASK;
    temp &= ~SDMC_DLYCTRL_CLK_SMP_PHA_MASK;
    temp |= drv << SDMC_DLYCTRL_CLK_DRV_PHA_SHIFT |
            smp << SDMC_DLYCTRL_CLK_SMP_PHA_SHIFT;
    sdmc_writel(host, SDMC_DLYCTRL, temp);
}

void hal_sdmc_set_delay(struct aic_sdmc_host *host, u32 drv, u32 smp)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_DLYCTRL);
    temp &= ~SDMC_DLYCTRL_CLK_DRV_DLY_MASK;
    temp &= ~SDMC_DLYCTRL_CLK_SMP_DLY_MASK;
    temp |= drv << SDMC_DLYCTRL_CLK_DRV_DLY_SHIFT |
            smp << SDMC_DLYCTRL_CLK_SMP_DLY_SHIFT;
    sdmc_writel(host, SDMC_DLYCTRL, temp);
}

void hal_sdmc_set_buswidth(struct aic_sdmc_host *host, u32 buswidth)
{
    u32 val = 0;

    val = sdmc_readl(host, SDMC_HCTRL2);
    val &= ~(SDMC_CTYPE_RESERVED << SDMC_HCTRL2_BW_SHIFT);
    val |= buswidth << SDMC_HCTRL2_BW_SHIFT;
    sdmc_writel(host, SDMC_HCTRL2, val);
}

void hal_sdmc_set_ddrmode(struct aic_sdmc_host *host, u32 ddr)
{
    u32 val = 0;

    val = sdmc_readl(host, SDMC_HCTRL2);
    if (ddr)
        val |= SDMC_HCTRL2_DDR_MODE;
    else
        val &= ~SDMC_HCTRL2_DDR_MODE;
    sdmc_writel(host, SDMC_HCTRL2, val);
}

void hal_sdmc_clk_disable(struct aic_sdmc_host *host)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_CLKCTRL);
    temp &= ~SDMC_CLKCTRL_EN;
    sdmc_writel(host, SDMC_CLKCTRL, temp);
}

void hal_sdmc_clk_enable(struct aic_sdmc_host *host)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_CLKCTRL);
    temp |= SDMC_CLKCTRL_EN;
    if (!host->is_sdio)
        temp |= SDMC_CLKCTRL_LOW_PWR;
    sdmc_writel(host, SDMC_CLKCTRL, temp);
}

void hal_sdmc_sdio_irq_enable(struct aic_sdmc_host *host, u32 en)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_INTEN);
    if (en)
        temp |= SDMC_INT_FROM_SDIO;
    else
        temp &= ~SDMC_INT_FROM_SDIO;
    sdmc_writel(host, SDMC_INTEN, temp);
}

void hal_sdmc_set_div(struct aic_sdmc_host *host, u32 div)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_CLKCTRL);
    temp &= ~SDMC_CLKCTRL_EN;
    sdmc_writel(host, SDMC_CLKCTRL, temp);

    temp = sdmc_readl(host, SDMC_CLKCTRL);
    temp &= ~SDMC_CLKCTRL_DIV_MASK;
    temp |= (div << SDMC_CLKCTRL_DIV_SHIFT) & SDMC_CLKCTRL_DIV_MASK;
    sdmc_writel(host, SDMC_CLKCTRL, temp);
}

void hal_sdmc_fifo_init(struct aic_sdmc_host *host, u32 *thd)
{
    if (*thd == 0) {
        u32 depth = 0;

        depth = sdmc_readl(host, SDMC_FIFOCFG);
        depth = ((depth & RX_WMARK_MASK) >> RX_WMARK_SHIFT) + 1;
        *thd = MSIZE(0x2) | RX_WMARK(depth / 2 - 1) | TX_WMARK(depth / 2);
    }
    sdmc_writel(host, SDMC_FIFOCFG, *thd);
    sdmc_writel(host, SDMC_CTC,
                (512 << SDMC_CTC_CARDTHR_SHIFT) | SDMC_CTC_CARDRDTHR_EN);
}

void hal_sdmc_soft_reset(struct aic_sdmc_host *host)
{
    u32 temp = 0;

    temp = sdmc_readl(host, SDMC_INTEN);
    sdmc_writel(host, SDMC_INTEN, 0);

    sdmc_writel(host, SDMC_IDMAINTEN, 0);

    if (hal_sdmc_reset(host, SDMC_HCTRL1_RESET_ALL)) {
        pr_err("Failed to reset!\n");
        return;
    }

    sdmc_writel(host, SDMC_INTEN, temp);

    hal_sdmc_set_cmd(host, SDMC_CMD_PRV_DAT_WAIT | SDMC_CMD_UPD_CLK | SDMC_CMD_START);

    if (hal_sdmc_wait_cmd_started(host)) {
        pr_err("Failed to wait cmd started!\n");
        return;
    }

    temp = sdmc_readl(host, SDMC_OINTST);
    sdmc_writel(host, SDMC_OINTST, temp);
}

void hal_sdmc_init(struct aic_sdmc_host *host)
{
    sdmc_writel(host, SDMC_OINTST, 0xFFFFFFFF);
    sdmc_writel(host, SDMC_INTEN, 0);

    sdmc_writel(host, SDMC_TTMC, 0xFFFFFFFF);

    sdmc_writel(host, SDMC_IDMAINTEN, 0);
    sdmc_writel(host, SDMC_PBUSCFG, 1);

#ifdef AIC_SDMC_IRQ_MODE
    sdmc_writel(host, SDMC_HCTRL1,
                sdmc_readl(host, SDMC_HCTRL1) | SDMC_HCTRL1_INT_EN);
    sdmc_writel(host, SDMC_INTEN, SDMC_INT_DAT_DONE);
    if (!host->complete)
        host->complete = aicos_sem_create(0);
    host->is_busy = 0;
#endif
}
