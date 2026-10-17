/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARTINCHIP_DMA_DRV_H_
#define __ARTINCHIP_DMA_DRV_H_

#include "hal_dma.h"

struct dma_chan {
    struct aic_dma_chan chan;
};

int drv_dma_init(void);

static inline struct dma_chan *dma_request_channel(void)
{
    return (struct dma_chan *)hal_request_dma_chan();
}

static inline void dma_release_channel(struct dma_chan *chan)
{
    hal_release_dma_chan((struct aic_dma_chan *)chan);
}

static inline enum dma_status dmaengine_tx_status(struct dma_chan *chan,
                                                  u32 *residue)
{
    return hal_dma_chan_tx_status((struct aic_dma_chan *)chan, residue);
}

static inline int dmaengine_prep_dma_memcpy(struct dma_chan *chan,
                                            u32 dest, u32 src, u32 len)
{
    return hal_dma_chan_prep_memcpy((struct aic_dma_chan *)chan,
                                     dest, src, len);
}

static inline int dmaengine_prep_dma_memset(struct dma_chan *chan,
                                            u32 buf, u32 val, u32 len)
{
    return hal_dma_chan_prep_memset((struct aic_dma_chan *)chan,
                                     buf, val, len);
}

static inline int dmaengine_prep_dma_device(struct dma_chan *chan,
                                            u32 dest, u32 src, u32 len,
                                            enum dma_transfer_direction dir)
{
    return hal_dma_chan_prep_device((struct aic_dma_chan *)chan,
                                     dest, src, len, dir);
}

static inline int dmaengine_prep_dma_cyclic(struct dma_chan *chan,
                                            u32 buf_addr, u32 buf_len,
                                            u32 period_len,
                                            enum dma_transfer_direction dir)
{
    return hal_dma_chan_prep_cyclic((struct aic_dma_chan *)chan, buf_addr,
                                     buf_len, period_len, dir);
}

static inline int dmaengine_submit(struct dma_chan *chan,
            dma_async_callback callback, void *callback_param)
{
    return hal_dma_chan_register_cb((struct aic_dma_chan *)chan,
                                     callback, callback_param);
}

static inline int dmaengine_slave_config(struct dma_chan *chan,
                                          struct dma_slave_config *config)
{
    return hal_dma_chan_config((struct aic_dma_chan *)chan, config);
}

static inline void dma_async_issue_pending(struct dma_chan *chan)
{
    hal_dma_chan_start((struct aic_dma_chan *)chan);
}

static inline int dmaengine_terminate_async(struct dma_chan *chan)
{
    return hal_dma_chan_terminate_all((struct aic_dma_chan *)chan);
}

static inline int dmaengine_pause(struct dma_chan *chan)
{
    return hal_dma_chan_pause((struct aic_dma_chan *)chan);
}

static inline int dmaengine_resume(struct dma_chan *chan)
{
    return hal_dma_chan_resume((struct aic_dma_chan *)chan);
}

void drv_dma_ch_stop(struct dma_chan *dma_ch);

#endif /* __ARTINCHIP_DMA_DRV_H_ */
