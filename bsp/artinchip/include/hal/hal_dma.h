/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_HAL_DMA_H_
#define _ARTINCHIP_HAL_DMA_H_

#include <stdbool.h>

#include "aic_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum dma_status {
    DMA_COMPLETE,
    DMA_IN_PROGRESS,
    DMA_PAUSED,
    DMA_ERROR,
    DMA_OUT_OF_ORDER,
};

enum dma_transfer_direction {
    DMA_MEM_TO_MEM,
    DMA_MEM_TO_DEV,
    DMA_DEV_TO_MEM,
    DMA_DEV_TO_DEV,
    DMA_TRANS_NONE,
};

enum dma_transfer_type {
    TYPE_IO_SINGLE      = 0,
    TYPE_BURST          = 1,
    TYPE_MEMORY         = 2,
    TYPE_MEMORYSET      = 3,
    TYPE_IO_FAST        = 4,
    TYPE_IO_AUTO        = 10,
};

enum dma_slave_buswidth {
    DMA_SLAVE_BUSWIDTH_UNDEFINED = 0,
    DMA_SLAVE_BUSWIDTH_1_BYTE = 1,
    DMA_SLAVE_BUSWIDTH_2_BYTES = 2,
    DMA_SLAVE_BUSWIDTH_3_BYTES = 3,
    DMA_SLAVE_BUSWIDTH_4_BYTES = 4,
    DMA_SLAVE_BUSWIDTH_8_BYTES = 8,
    DMA_SLAVE_BUSWIDTH_16_BYTES = 16,
    DMA_SLAVE_BUSWIDTH_32_BYTES = 32,
    DMA_SLAVE_BUSWIDTH_64_BYTES = 64,
};

typedef void (*dma_async_callback)(void *dma_async_param);

struct dma_slave_config {
    enum dma_transfer_direction direction;
    unsigned long src_addr;
    unsigned long dst_addr;
    enum dma_slave_buswidth src_addr_width;
    enum dma_slave_buswidth dst_addr_width;
    u32 src_maxburst;
    u32 dst_maxburst;
    u32 slave_id;
};

struct dma_slave_table {
    u32 id;
    u32 burst_num;
    u32 width_num;
    const u32 *burst;
    const u32 *width;
};

#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11) \
    || defined(AIC_DMA_DRV_V12)
struct aic_dma_task {
    u32 cfg; /* dma transfer configuration */
    u32 src; /* source address of one transfer package */
    u32 dst; /* distination address of one transfer package */
    u32 len; /* data length of one transfer package */
    u32 delay; /* time delay for period transfer */
    u32 p_next; /* next package for dma controller */

    u32 mode; /* the negotiation mode, not used by phsical task list */
#if (CACHE_LINE_SIZE == 64)
    u32 pad[7];
#endif
    /*
     * virtual list for cpu maintain package list,
     * not used by dma controller
     */
    struct aic_dma_task *v_next;
};
#else
struct aic_dma_task {
    u32 link_id;
    u32 cfg1; /* dma transfer configuration */
    u32 block_len; /* block length*/
    u32 src; /* source address of one transfer package */
    u32 dst; /* distination address of one transfer package */
    u32 len; /* data length of one transfer package */
    u32 cfg2;
    u32 p_next; /* next package for dma controller */
    u32 data_src;
    u32 data_dst;
    u32 pad[4];
    u32 mode; /* the negotiation mode, not used by phsical task list */
    /*
     * virtual list for cpu maintain package list,
     * not used by dma controller
     */
    struct aic_dma_task *v_next;
};
#endif

struct aic_dma_chan {
    u8 ch_nr; /* drq port number */
    u8 used;
    u8 irq_type; /* irq types */
    bool cyclic; /* flag to mark if cyclic transfer one package */
    bool memset;
    unsigned long base;
    struct dma_slave_config cfg;
    volatile int lock;
    dma_async_callback callback;
    void *callback_param;
    struct aic_dma_task *desc;
};

#define dma_reg(x)            (volatile void *)(x + DMA_BASE)

int hal_dma_chan_prep_memset(struct aic_dma_chan *chan,
                             u32 p_dest,
                             u32 value,
                             u32 len);
int hal_dma_chan_prep_memcpy(struct aic_dma_chan *chan,
                             u32 p_dest,
                             u32 p_src,
                             u32 len);
int hal_dma_chan_prep_device(struct aic_dma_chan *chan,
                             u32 p_dest,
                             u32 p_src,
                             u32 len,
                             enum dma_transfer_direction dir);
int hal_dma_prep_mode_device(struct aic_dma_chan *chan,
                             u32 p_dest,
                             u32 p_src,
                             u32 len,
                             enum dma_transfer_direction dir,
                             enum dma_transfer_type transfer_mode);
int hal_dma_chan_prep_cyclic(struct aic_dma_chan *chan,
                             u32 p_buf_addr,
                             u32 buf_len,
                             u32 period_len,
                             enum dma_transfer_direction dir);
int hal_dma_prep_mode_cyclic(struct aic_dma_chan *chan,
                             u32 p_buf_addr,
                             u32 buf_len,
                             u32 period_len,
                             enum dma_transfer_direction dir,
                             enum dma_transfer_type transfer_mode);
enum dma_status hal_dma_chan_tx_status(struct aic_dma_chan *chan, u32 *left_size);
int hal_dma_chan_start(struct aic_dma_chan *chan);
int hal_dma_chan_stop(struct aic_dma_chan *chan);
int hal_dma_chan_pause(struct aic_dma_chan *chan);
int hal_dma_chan_resume(struct aic_dma_chan *chan);
int hal_dma_chan_terminate_all(struct aic_dma_chan *chan);
int hal_dma_chan_register_cb(struct aic_dma_chan *chan,
                             dma_async_callback callback,
                             void *callback_param);
int hal_dma_chan_config(struct aic_dma_chan *chan,
                        struct dma_slave_config *config);
int hal_release_dma_chan(struct aic_dma_chan *chan);
struct aic_dma_chan * hal_request_dma_chan(void);
int hal_dma_init(void);
int hal_dma_deinit(void);
int hal_dma_chan_dump(int ch_nr);
int hal_wait_dma_chans_finish(void);

irqreturn_t hal_dma_irq(int irq, void *arg);

#ifdef __cplusplus
}
#endif

#endif
