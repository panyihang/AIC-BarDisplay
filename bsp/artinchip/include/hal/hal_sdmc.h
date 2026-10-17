/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _ARTINCHIP_HAL_SDMC_H_
#define _ARTINCHIP_HAL_SDMC_H_

#include <bouncebuf.h>

#define SDMC_CLOCK_MIN       400000      /* 400KHz */

#define FIFO_MIN            8
#define FIFO_MAX            4096
#define SDMC_TIMEOUT        2000000

/* SDMC controller register */
#define SDMC_BLKCNT             0x000
#define SDMC_BLKSIZ             0x004
#define SDMC_CMDARG             0x008
#define SDMC_CMD                0x00c
#define SDMC_RESP0              0x010
#define SDMC_RESP1              0x014
#define SDMC_RESP2              0x018
#define SDMC_RESP3              0x01c
#define SDMC_TTMC               0x020
#define SDMC_TCBC               0x024
#define SDMC_TFBC               0x028
#define SDMC_CTRST              0x02c
#define SDMC_HCTRL1             0x030
#define SDMC_CLKCTRL            0x034
#define SDMC_HCTRL2             0x038
#define SDMC_INTEN              0x03c
#define SDMC_INTST              0x040
#define SDMC_OINTST             0x044
#define SDMC_FIFOCFG            0x048
#define SDMC_HINFO              0x050
#define SDMC_PBUSCFG            0x080
#define SDMC_IDMARCAP           0x084
#define SDMC_IDMASADDR          0x088
#define SDMC_IDMAST             0x08c
#define SDMC_IDMAINTEN          0x090
#define SDMC_IDMACDA            0x094
#define SDMC_IDMACBA            0x098
#define SDMC_CTC                0x100
#define SDMC_DLYCTRL            0x104
#define SDMC_EMCR               0x108
#define SDMC_VERID              0x118
#define SDMC_FIFO_DATA          0x200

/* Command configure register defines */
#define SDMC_CMD_START          BIT(31)
#define SDMC_CMD_USE_HOLD_REG   BIT(29)
#define SDMC_CMD_UPD_CLK        BIT(21)
#define SDMC_CMD_INIT           BIT(15)
#define SDMC_CMD_STOP           BIT(14)
#define SDMC_CMD_PRV_DAT_WAIT   BIT(13)
#define SDMC_CMD_SEND_STOP      BIT(12)
#define SDMC_CMD_DAT_WR         BIT(10)
#define SDMC_CMD_DAT_EXP        BIT(9)
#define SDMC_CMD_RESP_CRC       BIT(8)
#define SDMC_CMD_RESP_LEN       BIT(7)
#define SDMC_CMD_RESP_EXP       BIT(6)

/* Controller status register defines */
#define SDMC_CTRST_FCNT(x)     (((x) >> 17) & 0x1FFF)
#define SDMC_CTRST_BUSY        BIT(9)
#define SDMC_CTRST_FIFO_FULL   BIT(3)
#define SDMC_CTRST_FIFO_EMPTY  BIT(2)

/* Host control 1 register defines */
#define SDMC_HCTRL1_USE_IDMAC           BIT(25)
#define SDMC_HCTRL1_SEND_AS_CCSD        BIT(10)
#define SDMC_HCTRL1_DMA_EN              BIT(5)
#define SDMC_HCTRL1_INT_EN              BIT(4)
#define SDMC_HCTRL1_DMA_RESET           BIT(2)
#define SDMC_HCTRL1_FIFO_RESET          BIT(1)
#define SDMC_HCTRL1_RESET               BIT(0)
#define SDMC_HCTRL1_RESET_ALL \
    (SDMC_HCTRL1_RESET | SDMC_HCTRL1_FIFO_RESET | SDMC_HCTRL1_DMA_RESET)

/* Clock control register defines */
#define SDMC_CLKCTRL_LOW_PWR            BIT(16)
#define SDMC_CLKCTRL_DIV_SHIFT          8
#define SDMC_CLKCTRL_DIV_MASK           GENMASK(15, 8)
#define SDMC_CLKCTRL_DIV_MAX \
            (SDMC_CLKCTRL_DIV_MASK >> SDMC_CLKCTRL_DIV_SHIFT)
#define SDMC_CLKCTRL_EN                 BIT(0)

/* Host control 2 register defines */
#define SDMC_HCTRL2_BW_8BIT             BIT(29)
#define SDMC_HCTRL2_BW_4BIT             BIT(28)
#define SDMC_HCTRL2_BW_1BIT             0
#define SDMC_HCTRL2_BW_SHIFT            28
#define SDMC_HCTRL2_DDR_MODE            BIT(16)
#define SDMC_HCTRL2_VOLT_18V            BIT(0)

/* Card-type */
#define SDMC_CTYPE_1BIT         (0)
#define SDMC_CTYPE_4BIT         (0x1)
#define SDMC_CTYPE_8BIT         (0x2)
#define SDMC_CTYPE_RESERVED     (0x3)

/* Data-rate */
#define SDMC_SDR_MODE           (0)
#define SDMC_DDR_MODE           (0x1)

/* Interrupt status & enable register defines */
#define SDMC_INT_ALL            0xffffffff
#define SDMC_INT_FROM_SDIO      BIT(16)
#define SDMC_INT_EBE            BIT(15)
#define SDMC_INT_AUTO_CMD_DONE  BIT(14)
#define SDMC_INT_SBE            BIT(13)
#define SDMC_INT_HLE            BIT(12)
#define SDMC_INT_FRUN           BIT(11)
#define SDMC_INT_HTO            BIT(10)
#define SDMC_INT_DRTO           BIT(9)
#define SDMC_INT_RTO            BIT(8)
#define SDMC_INT_DCRC           BIT(7)
#define SDMC_INT_RCRC           BIT(6)
#define SDMC_INT_RXDR           BIT(5)
#define SDMC_INT_TXDR           BIT(4)
#define SDMC_INT_DAT_DONE       BIT(3)
#define SDMC_INT_CMD_DONE       BIT(2)
#define SDMC_INT_RESP_ERR       BIT(1)
#define SDMC_INT_CD             BIT(0)
#define SDMC_DATA_ERR   (SDMC_INT_EBE | SDMC_INT_SBE | SDMC_INT_HLE | \
                         SDMC_INT_FRUN | SDMC_INT_EBE | SDMC_INT_DCRC)
#define SDMC_DATA_TOUT  (SDMC_INT_HTO | SDMC_INT_DRTO)

/* FIFO configuration register defines */
#define MSIZE(x)                ((x) << 28)
#define RX_WMARK(x)             ((x) << 16)
#define TX_WMARK(x)             (x)
#define RX_WMARK_SHIFT          16
#define RX_WMARK_MASK           (0x7ff << RX_WMARK_SHIFT)

/* Peripheral bus configuration register defines */
#define SDMC_PBUSCFG_IDMAC_EN      BIT(7)
#define SDMC_PBUSCFG_IDMAC_FB      BIT(1)
#define SDMC_PBUSCFG_IDMAC_SWR     BIT(0)

/* Internal DMAC interrupt defines */
#define SDMC_IDMAC_INT_AIS        BIT(9)
#define SDMC_IDMAC_INT_NIS        BIT(8)
#define SDMC_IDMAC_INT_CES        BIT(5)
#define SDMC_IDMAC_INT_DU         BIT(4)
#define SDMC_IDMAC_INT_FBE        BIT(2)
#define SDMC_IDMAC_INT_RI         BIT(1)
#define SDMC_IDMAC_INT_TI         BIT(0)
#define SDMC_IDMAC_INT_MASK       (SDMC_IDMAC_INT_AIS | SDMC_IDMAC_INT_NIS | \
                                 SDMC_IDMAC_INT_CES | SDMC_IDMAC_INT_DU | \
                                 SDMC_IDMAC_INT_FBE | SDMC_IDMAC_INT_RI | \
                                 SDMC_IDMAC_INT_TI)

#define SDMC_IDMAC_OWN          BIT(31)
#define SDMC_IDMAC_CH           BIT(4)
#define SDMC_IDMAC_FS           BIT(3)
#define SDMC_IDMAC_LD           BIT(2)

/* Card Threshold Control */
#define SDMC_CTC_CARDTHR_SHIFT  (16)
#define SDMC_CTC_CARDTHR_MASK   GENMASK(27, 16)
#define SDMC_CTC_CARDRDTHR_EN   BIT(0)

#define SDMC_DLYCTRL_EXT_CLK_MUX_SHIFT  30
#define SDMC_DLYCTRL_EXT_CLK_MUX_MASK   GENMASK(31, 30)
#define SDMC_DLYCTRL_EXT_CLK_MUX_1  2
#define SDMC_DLYCTRL_EXT_CLK_MUX_2  1
#define SDMC_DLYCTRL_EXT_CLK_MUX_4  0
#define SDMC_DLYCTRL_CLK_DRV_PHA_MASK   GENMASK(29, 28)
#define SDMC_DLYCTRL_CLK_DRV_PHA_SHIFT  28
#define SDMC_DLYCTRL_CLK_DRV_PHA_0  0
#define SDMC_DLYCTRL_CLK_DRV_PHA_90 1
#define SDMC_DLYCTRL_CLK_DRV_PHA_180    2
#define SDMC_DLYCTRL_CLK_DRV_PHA_270    3
#define SDMC_DLYCTRL_CLK_DRV_DLY_MASK   GENMASK(27, 23)
#define SDMC_DLYCTRL_CLK_DRV_DLY_SHIFT  23
#define SDMC_DLYCTRL_CLK_SMP_PHA_MASK   GENMASK(22, 21)
#define SDMC_DLYCTRL_CLK_SMP_PHA_SHIFT  21
#define SDMC_DLYCTRL_CLK_SMP_PHA_0  0
#define SDMC_DLYCTRL_CLK_SMP_PHA_90 1
#define SDMC_DLYCTRL_CLK_SMP_PHA_180    2
#define SDMC_DLYCTRL_CLK_SMP_PHA_270    3
#define SDMC_DLYCTRL_CLK_DRV_DLY_MAX \
            (SDMC_DLYCTRL_CLK_DRV_DLY_MASK >> SDMC_DLYCTRL_CLK_DRV_DLY_SHIFT)
#define SDMC_DLYCTRL_CLK_SMP_DLY_MASK   GENMASK(20, 16)
#define SDMC_DLYCTRL_CLK_SMP_DLY_SHIFT  16
#define SDMC_DLYCTRL_CLK_SMP_DLY_MAX \
            (SDMC_DLYCTRL_CLK_SMP_DLY_MASK >> SDMC_DLYCTRL_CLK_SMP_DLY_SHIFT)

/* quirks */
#define SDMC_QUIRK_DISABLE_SMU  (1 << 0)

struct aic_sdmc_idma_desc {
    u32 flags;
    u32 cnt;
    u32 addr;
    u32 next_addr;
} __aligned(8);

struct aic_sdmc_host {
    volatile void *base;
    u32 is_sdio;
    u32 fifoth_val;
#ifdef AIC_SDMC_IRQ_MODE
    aicos_sem_t complete;
    u32 is_busy;
#endif
};

void hal_sdmc_idma_update_intstat(struct aic_sdmc_host *host);
int hal_sdmc_idma_start(struct aic_sdmc_host *host, u32 size, u32 read,
                        u32 *buf, struct bounce_buffer *bbstate);
int hal_sdmc_idma_stop(struct aic_sdmc_host *host,
                       struct bounce_buffer *bbstate, u32 read);
void hal_sdmc_idma_disable(struct aic_sdmc_host *host);
u8 hal_sdmc_get_idma_status(struct aic_sdmc_host *host);
void hal_sdmc_idma_prepare(struct aic_sdmc_host *host,
                           u32 blksize, u32 blks,
                           struct aic_sdmc_idma_desc *cur_idma,
                           void *bounce_buffer);
int hal_sdmc_data_rx(struct aic_sdmc_host *host, u32 *buf, u32 size);
int hal_sdmc_data_tx(struct aic_sdmc_host *host, u32 *buf, u32 size);

u32 hal_sdmc_int_stat(struct aic_sdmc_host *host);
void hal_sdmc_int_clr(struct aic_sdmc_host *host, u32 mask);
int hal_sdmc_is_busy(struct aic_sdmc_host *host);

void hal_sdmc_set_blk(struct aic_sdmc_host *host, u32 blksize, u32 blks);
void hal_sdmc_set_arg(struct aic_sdmc_host *host, u32 arg);
void hal_sdmc_set_cmd(struct aic_sdmc_host *host, u32 cmd);
u32 hal_sdmc_wait_cmd_started(struct aic_sdmc_host *host);
void hal_sdmc_get_rsp(struct aic_sdmc_host *host, u32 *buf, u32 all);

void aic_sdmc_set_ext_clk_mux(struct aic_sdmc_host *host, u32 mux);
void hal_sdmc_set_phase(struct aic_sdmc_host *host, u32 drv, u32 smp);
void hal_sdmc_set_delay(struct aic_sdmc_host *host, u32 drv, u32 smp);
void hal_sdmc_set_buswidth(struct aic_sdmc_host *host, u32 buswidth);
void hal_sdmc_set_ddrmode(struct aic_sdmc_host *host, u32 ddr);
void hal_sdmc_clk_disable(struct aic_sdmc_host *host);
void hal_sdmc_clk_enable(struct aic_sdmc_host *host);
void hal_sdmc_sdio_irq_enable(struct aic_sdmc_host *host, u32 en);
void hal_sdmc_set_div(struct aic_sdmc_host *host, u32 div);

void hal_sdmc_fifo_init(struct aic_sdmc_host *host, u32 *thd);
void hal_sdmc_soft_reset(struct aic_sdmc_host *host);
int hal_sdmc_reset(struct aic_sdmc_host *host, u32 value);
void hal_sdmc_init(struct aic_sdmc_host *host);

#endif
