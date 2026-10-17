/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "aic_core.h"
#include "aic_dma_id.h"

#include "hal_dma_reg_v1x.h"
#include "hal_dma.h"

static struct aic_dma_dev aich_dma = {
    .base = DMA_BASE,
    .burst_length = BIT(1) | BIT(4) | BIT(8) | BIT(16),
    .addr_widths = AIC_DMA_BUS_WIDTH,
    .slave_table = aic_dma_slave_table,
};

struct aic_dma_dev *get_aic_dma_dev(void)
{
    return &aich_dma;
}

static void hal_dma_reg_dump(struct aic_dma_chan *chan)
{
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();
    printf("Common register: \n"
            "    IRQ_EN 0x%x, \tIRQ_STA 0x%x, \tCH_STA 0x%x, GATE 0x%x\n",
            readl(aich_dma->base + DMA_IRQ_EN_REG(0)),
            readl(aich_dma->base + DMA_IRQ_STA_REG(0)),
            readl(aich_dma->base + DMA_CH_STA_REG),
            readl(aich_dma->base + DMA_GATE_REG));

    printf("Ch%d register: \n"
            "    Enable 0x%x, \tMode 0x%x, \tPause 0x%x\n"
            "    Task 0x%x, \tConfig 0x%x, \tSrc 0x%x, \tSink 0x%x\n"
            "    Left 0x%x, \tPackage_cnt %d\n",
            chan->ch_nr,
            readl(chan->base + DMA_CH_EN_REG),
            readl(chan->base + DMA_CH_MODE_REG),
            readl(chan->base + DMA_CH_PAUSE_REG),
            readl(chan->base + DMA_CH_TASK_REG),
            readl(chan->base + DMA_CH_CFG_REG),
            readl(chan->base + DMA_CH_SRC_REG),
            readl(chan->base + DMA_CH_SINK_REG),
            readl(chan->base + DMA_CH_LEFT_REG),
            readl(chan->base + DMA_CH_PKG_NUM_REG));
}

static void hal_dma_task_dump(struct aic_dma_chan *chan)
{
    struct aic_dma_task *task;

    printf("DMA Ch%d: desc = 0x%lx\n", chan->ch_nr, (unsigned long)chan->desc);

    for (task = chan->desc; task != NULL; task = task->v_next)
    {
        printf(" task (0x%lx):\n"
                "\tcfg - 0x%x, src - 0x%x, dst - 0x%x, len - 0x%x\n"
                "\tdelay - 0x%x, p_next - 0x%x, mode - 0x%x, v_next - 0x%lx\n",
                (unsigned long)task,
                task->cfg, task->src, task->dst, task->len,
                task->delay, task->p_next, task->mode,
                (unsigned long)task->v_next);
    }
}

int hal_dma_chan_dump(int ch_nr)
{
    struct aic_dma_chan *chan;
    struct aic_dma_dev *aich_dma;

    aich_dma = get_aic_dma_dev();

    CHECK_PARAM(ch_nr < AIC_DMA_CH_NUM, -EINVAL);

    chan = &aich_dma->dma_chan[ch_nr];

    hal_dma_task_dump(chan);
    hal_dma_reg_dump(chan);

    return 0;
}

static inline s8 convert_burst(u32 maxburst)
{
    switch (maxburst) {
    case 1:
        return 0;
    case 4:
        return 1;
    case 8:
        return 2;
    case 16:
        return 3;
    default:
        return -EINVAL;
    }
}

static inline s8 convert_buswidth(enum dma_slave_buswidth addr_width)
{
    switch (addr_width) {
    case DMA_SLAVE_BUSWIDTH_2_BYTES:
        return 1;
    case DMA_SLAVE_BUSWIDTH_4_BYTES:
        return 2;
    case DMA_SLAVE_BUSWIDTH_8_BYTES:
        return 3;
    default:
        /* For 1 byte width or fallback */
        return 0;
    }
}

static int aic_set_param(struct dma_slave_config *sconfig,
                    u32 *dev_maxburst, u32 *mem_maxburst,
                    enum dma_slave_buswidth *dev_addr_width,
                    enum dma_slave_buswidth *mem_addr_width)
{
    u8 j, temp_burst;
    struct aic_dma_dev *aich_dma;
    enum dma_slave_buswidth temp_addr_width;
    const struct dma_slave_table *slave_table;

    aich_dma = get_aic_dma_dev();
    slave_table = aich_dma->slave_table[sconfig->slave_id];
    CHECK_PARAM(slave_table->id == sconfig->slave_id, -EINVAL);

    *mem_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    *mem_maxburst = *mem_maxburst ? *mem_maxburst : 8;
    temp_addr_width = slave_table->width[0];
    for (j = 0; j < slave_table->width_num; j++) {
        if (slave_table->width[j] == *dev_addr_width)
            temp_addr_width = slave_table->width[j];
    }
    temp_burst = slave_table->burst[0];
    for (j = 0; j < slave_table->burst_num; j++) {
        if (slave_table->burst[j] == *dev_maxburst)
            temp_burst = slave_table->burst[j];
    }
    *dev_maxburst = temp_burst;
    *dev_addr_width = temp_addr_width;

    return 0;
}

int aic_set_burst(struct dma_slave_config *sconfig,
                             enum dma_transfer_direction direction,
                             u32 *p_cfg)
{
    enum dma_slave_buswidth src_addr_width, dst_addr_width;
    struct aic_dma_dev *aich_dma;
    u32 src_maxburst, dst_maxburst;
    s8 src_width, dst_width, src_burst, dst_burst;

    CHECK_PARAM(sconfig->slave_id < AIC_DMA_PORTS
                && sconfig->slave_id > 0, -EINVAL);

    aich_dma = get_aic_dma_dev();
    src_addr_width = sconfig->src_addr_width;
    dst_addr_width = sconfig->dst_addr_width;
    src_maxburst = sconfig->src_maxburst;
    dst_maxburst = sconfig->dst_maxburst;

    switch (direction) {
    case DMA_MEM_TO_DEV:
        aic_set_param(sconfig, &dst_maxburst, &src_maxburst,
                    &dst_addr_width, &src_addr_width);
        break;
    case DMA_DEV_TO_MEM:
        aic_set_param(sconfig, &src_maxburst, &dst_maxburst,
                    &src_addr_width, &dst_addr_width);
        break;
    default:
        return -EINVAL;
    }

#ifdef AIC_DMA_CFG_TEST
    static u8 i = 0;
    if (!i) {
        i++;
        printf("src_addr_width = %d dst_addr_width = %d "
               "dst_maxburst = %d src_maxburst = %d\n",
               src_addr_width, dst_addr_width, dst_maxburst, src_maxburst);
    }
#endif

    if (!(BIT(src_addr_width) & aich_dma->addr_widths))
        return -EINVAL;
    if (!(BIT(dst_addr_width) & aich_dma->addr_widths))
        return -EINVAL;
    if (!(BIT(src_maxburst) & aich_dma->burst_length))
        return -EINVAL;
    if (!(BIT(dst_maxburst) & aich_dma->burst_length))
        return -EINVAL;

    src_width = convert_buswidth(src_addr_width);
    dst_width = convert_buswidth(dst_addr_width);
    dst_burst = convert_burst(dst_maxburst);
    src_burst = convert_burst(src_maxburst);

    *p_cfg = (src_width << SRC_WIDTH_BITSHIFT) |
         (dst_width << DST_WIDTH_BITSHIFT) |
         (src_burst << SRC_BURST_BITSHIFT) |
         (dst_burst << DST_BURST_BITSHIFT);

    return 0;
}

struct aic_dma_task *aic_dma_task_alloc(void)
{
    struct aic_dma_task *task;
    unsigned long state;
    /* Remove the QH structure from the freelist */

    aicos_local_irq_save(&state);
    task = aich_dma.freetask;

    if (task) {
        aich_dma.freetask = task->v_next;
        memset(task, 0, sizeof(struct aic_dma_task));
    }
    aicos_local_irq_restore(state);
    return task;
}

static void aic_dma_task_free(struct aic_dma_task *task)
{
    unsigned long state;
    CHECK_PARAM_RET(task != NULL);
    aicos_local_irq_save(&state);
    task->v_next = aich_dma.freetask;
    aich_dma.freetask = task;
    aicos_local_irq_restore(state);
}

void *aic_dma_task_add(struct aic_dma_task *prev,
                                 struct aic_dma_task *next,
                                 struct aic_dma_chan *chan)
{
    CHECK_PARAM((chan != NULL || prev != NULL) && next != NULL, NULL);

    if (!prev)
    {
        chan->desc = next;
    }
    else
    {
        prev->p_next = __pa((unsigned long)next);
        prev->v_next = next;
    }

    next->p_next = DMA_LINK_END_FLAG;
    next->v_next = NULL;

    return next;
}

void aic_dma_free_desc(struct aic_dma_chan *chan)
{
    unsigned long state;
    struct aic_dma_task *task;
    struct aic_dma_task *next;

    CHECK_PARAM_RET(chan != NULL);
    CHECK_PARAM_RET(chan->desc != NULL);

    aicos_local_irq_save(&state);
    task = chan->desc;
    chan->desc = NULL;

    while (task)
    {
        next = task->v_next;
        aic_dma_task_free(task);
        task = next;
    }

    aicos_local_irq_restore(state);
}

enum dma_status hal_dma_chan_tx_status(struct aic_dma_chan *chan,
                                       u32 *left_size)
{
    CHECK_PARAM(chan != NULL && left_size != NULL, -EINVAL);

    *left_size = readl(chan->base + DMA_CH_LEFT_REG);

    if (!(readl(aich_dma.base + DMA_CH_STA_REG) & BIT(chan->ch_nr)))
        return DMA_COMPLETE;

    return DMA_IN_PROGRESS;
}

int hal_dma_chan_stop(struct aic_dma_chan *chan)
{
    u32 irq_reg, irq_offset;
    u32 cnt = 0;
    u32 value;

    CHECK_PARAM(chan != NULL, -EINVAL);
    if (chan->cfg.direction == DMA_DEV_TO_MEM)
        aicos_dma_sync();

    irq_reg = chan->ch_nr / DMA_IRQ_CHAN_NR;
    irq_offset = chan->ch_nr % DMA_IRQ_CHAN_NR;

    /* disable irq */
    value = readl(aich_dma.base + DMA_IRQ_EN_REG(irq_reg));
    value &= ~(DMA_IRQ_MASK(irq_offset));
    writel(value, aich_dma.base + DMA_IRQ_EN_REG(irq_reg));

    /* pause */
    hal_dma_chan_pause(chan);

    /* wait for read task complete */
    while(readl(chan->base + DMA_CH_RDC_COMPLETE_REG)) {
        aic_udelay(10);
        if (cnt++ > 100) {
            pr_err("Wait dma complete timeout!\n");
            return -ETIMEDOUT;
        }
    }
    cnt = 0;

    /* stop */
    writel(0x00, chan->base + DMA_CH_EN_REG);
    while(readl(chan->base + DMA_CH_EN_REG)) {
        aic_udelay(10);
        if (cnt++ > 100) {
            pr_err("Wait dma chn disable timeout!\\n");
            return -ETIMEDOUT;
        }
    }

    /* resume */
    hal_dma_chan_resume(chan);

    chan->cyclic = false;
    chan->memset = false;

    /* free task list */
    aic_dma_free_desc(chan);
    return 0;
}

int hal_dma_chan_pause(struct aic_dma_chan *chan)
{
    u32 val;

    CHECK_PARAM(chan != NULL, -EINVAL);

    /* pause */
    val = readl(chan->base + DMA_CH_PAUSE_REG);
    val |= DMA_CH_PAUSE;
    writel(val, chan->base + DMA_CH_PAUSE_REG);

    return 0;
}

int hal_dma_chan_resume(struct aic_dma_chan *chan)
{
    u32 val;

    CHECK_PARAM(chan != NULL, -EINVAL);

    /* resume */
    val = readl(chan->base + DMA_CH_PAUSE_REG);
    val &= ~ DMA_CH_PAUSE;
    writel(DMA_CH_RESUME, chan->base + DMA_CH_PAUSE_REG);

    return 0;
}

int hal_dma_chan_terminate_all(struct aic_dma_chan *chan)
{
    CHECK_PARAM(chan != NULL, -EINVAL);

    hal_dma_chan_stop(chan);

    return 0;
}

int hal_dma_chan_register_cb(struct aic_dma_chan *chan,
                                     dma_async_callback callback,
                                     void *callback_param)
{
    CHECK_PARAM(chan != NULL && callback != NULL, -EINVAL);

    chan->callback = callback;
    chan->callback_param = callback_param;

    return 0;
}

int hal_dma_chan_config(struct aic_dma_chan *chan,
                               struct dma_slave_config *config)
{

    CHECK_PARAM(chan != NULL && config != NULL, -EINVAL);

    memcpy(&chan->cfg, config, sizeof(*config));

    return 0;
}

int hal_release_dma_chan(struct aic_dma_chan *chan)
{
    CHECK_PARAM(chan != NULL && chan->used != 0, -EINVAL);

    /* free task list */
    aic_dma_free_desc(chan);

    chan->used = 0;
    chan->callback = NULL;
    chan->callback_param = NULL;

    return 0;
}

struct aic_dma_chan *hal_request_dma_chan(void)
{
    int i = 0;
    struct aic_dma_chan *chan;
    unsigned long state;

    aicos_local_irq_save(&state);
    for (i = 0; i < AIC_DMA_CH_NUM; i++)
    {
        chan = &aich_dma.dma_chan[i];
        if (chan->used == 0)
        {
            chan->used = 1;
            chan->cyclic = false;
            chan->memset = false;
            chan->irq_type = 0;
            chan->callback = NULL;
            chan->callback_param = NULL;
            chan->desc = NULL;
            aicos_local_irq_restore(state);
            return chan;
        }
    }
    aicos_local_irq_restore(state);
    return NULL;
}

int hal_wait_dma_chans_finish(void)
{
    int i = 0;
    struct aic_dma_chan *chan;
    u32 left_size = 0;
    u64 start_us = 0;

    start_us = aic_get_time_us();
retry:
    for (i = 0; i < AIC_DMA_CH_NUM; i++)
    {
        if ((aic_get_time_us() - start_us) > 3000) {
            printf("%s timeout!\n", __func__);
            return -EBUSY;
        }
        chan = &aich_dma.dma_chan[i];
        if (hal_dma_chan_tx_status(chan, &left_size) != DMA_COMPLETE) {
            aic_udelay(1);
            goto retry;
        }
    }

    return 0;
}

int hal_dma_init(void)
{
    int i;

    aich_dma.base = DMA_BASE;

    for (i = 0; i < AIC_DMA_CH_NUM; i++) {
        aich_dma.dma_chan[i].ch_nr = i;
        aich_dma.dma_chan[i].base = aich_dma.base + 0x100
                                    + i * DMA_CHAN_OFFSET;
    }

    aich_dma.freetask = NULL;
    for (i = 0; i < TASK_MAX_NUM; i++)
        aic_dma_task_free(&aich_dma.task[i]);


    for (i = 0; i < AIC_DMA_CH_NUM / DMA_IRQ_CHAN_NR; i++) {
        writel(0x0, aich_dma.base + DMA_IRQ_EN_REG(i));
    }

    #ifdef AIC_DMA_DRV_V12
    writel(BIT(27), aich_dma.base + DMA_MEM_CFG);
    #endif
    return 0;
}

irqreturn_t hal_dma_irq(int irq, void *arg)
{
    int i;
    u32 status;
    struct aic_dma_chan *chan;
    struct aic_dma_dev *aich_dma;
    dma_async_callback cb = NULL;
    void *cb_data = NULL;

    aich_dma = get_aic_dma_dev();

    /* get dma irq pending */
    status = readl(aich_dma->base + DMA_IRQ_STA_REG(0));
    if (!status) {
        /* none irq trigger */
        return IRQ_NONE;
    }
    pr_debug("IRQ status: 0x%x\n", status);

    /* clear irq pending */
    writel(status, aich_dma->base + DMA_IRQ_STA_REG(0));

    /* process irq for every dma channel */
    for (i = 0; (i < AIC_DMA_CH_NUM) && status; i++, status >>= DMA_IRQ_CH_WIDTH) {
        chan = &aich_dma->dma_chan[i];

        if ((!chan->used) || !(status & chan->irq_type))
            continue;

        /* Only trigger callback on completion interrupt */
        if (!(status & DMA_IRQ_TASK_DONE)) {
            pr_err("DMA ch%d unexpected irq, status: 0x%x\n", i, status);
            continue;
        }

        cb = chan->callback;
        cb_data = chan->callback_param;
        if (cb)
        {
            cb(cb_data);
        }
    }

    return IRQ_HANDLED;
}


#ifndef AIC_DMA_DRV_V10
int hal_dma_chan_prep_memset(struct aic_dma_chan *chan,
                             u32 p_dest, u32 value, u32 len)
{
    struct aic_dma_task *task;

    CHECK_PARAM(chan != NULL && len != 0 && len < MAX_LEN, -EINVAL);

    CHECK_PARAM((p_dest % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->src = p_dest;
    task->dst = p_dest;
    task->len = len;
    task->cfg = (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
             (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
             (3 << DST_BURST_BITSHIFT) | (3 << SRC_BURST_BITSHIFT) |
             (2 << DST_WIDTH_BITSHIFT) | (2 << SRC_WIDTH_BITSHIFT) |
             (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
             (DMA_ID_DRAM << SRC_PORT_BITSHIFT);
    task->delay = DELAY_DEF_VAL;
    task->mode = DMA_S_WAIT_D_WAIT;

    aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);

    aic_dma_task_add(NULL, task, chan);

    writel(value, chan->base + DMA_CH_MEMSET_VAL_REG);
    chan->memset = true;

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}
#endif

int hal_dma_chan_prep_memcpy(struct aic_dma_chan *chan,
                             u32 p_dest, u32 p_src, u32 len)
{
    unsigned long state;
    struct aic_dma_task *task = NULL;
    struct aic_dma_task *prev = NULL;

    CHECK_PARAM(chan != NULL && len != 0 && len < MAX_LEN, -EINVAL);
    CHECK_PARAM((p_dest % AIC_DMA_ALIGN_SIZE) == 0
            && (p_src % AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->src = p_src;
    task->dst = p_dest;
    task->len = len;
    task->cfg = (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
             (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
             (3 << DST_BURST_BITSHIFT) | (3 << SRC_BURST_BITSHIFT) |
             (2 << DST_WIDTH_BITSHIFT) | (2 << SRC_WIDTH_BITSHIFT) |
             (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
             (DMA_ID_DRAM << SRC_PORT_BITSHIFT);
    task->delay = DELAY_DEF_VAL;
    task->mode = DMA_S_WAIT_D_WAIT;

    aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
    aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);

    aicos_local_irq_save(&state);
    prev = chan->desc;
    if (prev != NULL) {
        while (prev->v_next != NULL) {
            prev = prev->v_next;
        }
    }

    aic_dma_task_add(prev, task, chan);
    aicos_local_irq_restore(state);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_prep_device(struct aic_dma_chan *chan,
                             u32 p_dest, u32 p_src, u32 len,
                             enum dma_transfer_direction dir)
{
    struct aic_dma_task *task;
    u32 task_cfg;
    int ret;

   CHECK_PARAM(chan != NULL && len != 0 && len < MAX_LEN, -EINVAL);

    CHECK_PARAM((p_dest%AIC_DMA_ALIGN_SIZE) == 0 && (p_src%AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    ret = aic_set_burst(&chan->cfg, dir, &task_cfg);
    if (ret) {
        hal_log_err("Invalid DMA configuration\n");
        return -EINVAL;
    }

    task = aic_dma_task_alloc();
    CHECK_PARAM(task != NULL, -ENOMEM);

    task->delay = DELAY_DEF_VAL;
    task->len = len;
    task->src = p_src;
    task->dst = p_dest;
    task->cfg = task_cfg;
    if (dir == DMA_MEM_TO_DEV) {
        task->cfg |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                  ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                  (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
                  (ADDR_FIXED_MODE << DST_ADDR_BITSHIFT);
        task->mode = DMA_S_WAIT_D_HANDSHAKE | \
                     (GET_DMA_DST_BURST(task_cfg) != 1) << DMA_DST_HANDSHAKE_ENABLE;
        aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
    } else {
        task->cfg |= (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
                  ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << SRC_PORT_BITSHIFT) |
                  (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
                  (ADDR_FIXED_MODE << SRC_ADDR_BITSHIFT);
        task->mode = DMA_S_HANDSHAKE_D_WAIT | \
                     (GET_DMA_DST_BURST(task_cfg) != 1) << DMA_SRC_HANDSHAKE_ENABLE;
        aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
    }

    aic_dma_task_add(NULL, task, chan);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_prep_cyclic(struct aic_dma_chan *chan,
                             u32 p_buf_addr, u32 buf_len, u32 period_len,
                             enum dma_transfer_direction dir)
{
    struct aic_dma_task *task = NULL;
    struct aic_dma_task *prev = NULL;
    u32 task_cfg;
    u32 periods;
    u32 i;
    int ret;

    CHECK_PARAM(chan != NULL && buf_len != 0 && period_len != 0, -EINVAL);

    CHECK_PARAM((p_buf_addr%AIC_DMA_ALIGN_SIZE) == 0 && (buf_len%AIC_DMA_ALIGN_SIZE) == 0, -EINVAL);

    ret = aic_set_burst(&chan->cfg, dir, &task_cfg);
    if (ret) {
        hal_log_err("Invalid DMA configuration\n");
        return -EINVAL;
    }

    periods = buf_len / period_len;

    for (i = 0; i < periods; i++) {
        task = aic_dma_task_alloc();
        if (task == NULL) {
            aic_dma_free_desc(chan);
            return -ENOMEM;
        }

        task->len = period_len;
        if (dir == DMA_MEM_TO_DEV) {
            chan->irq_type = DMA_IRQ_HALF_TASK;
            task->src = p_buf_addr + period_len * i;
            task->dst = chan->cfg.dst_addr;
            task->cfg = task_cfg;
            task->cfg |= (DMA_ID_DRAM << SRC_PORT_BITSHIFT) |
                      ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << DST_PORT_BITSHIFT) |
                      (ADDR_LINEAR_MODE << SRC_ADDR_BITSHIFT) |
                      (ADDR_FIXED_MODE << DST_ADDR_BITSHIFT);
            task->delay = DELAY_DEF_VAL;
            task->mode = DMA_S_WAIT_D_HANDSHAKE;
            aicos_dcache_clean_range((void *)(unsigned long)task->src, task->len);
        } else {
            chan->irq_type = DMA_IRQ_ONE_TASK;
            task->src = chan->cfg.src_addr;
            task->dst = p_buf_addr + period_len * i;
            task->cfg = task_cfg;
            task->cfg |= (DMA_ID_DRAM << DST_PORT_BITSHIFT) |
                      ((chan->cfg.slave_id & DMA_DRQ_PORT_MASK) << SRC_PORT_BITSHIFT) |
                      (ADDR_LINEAR_MODE << DST_ADDR_BITSHIFT) |
                      (ADDR_FIXED_MODE << SRC_ADDR_BITSHIFT);
            task->delay = DELAY_DEF_VAL;
            task->mode = DMA_S_HANDSHAKE_D_WAIT;
            aicos_dcache_clean_invalid_range((void *)(unsigned long)task->dst, task->len);
        }

        prev = aic_dma_task_add(prev, task, chan);
    }

    prev->p_next = __pa((unsigned long)chan->desc);

    chan->cyclic = true;

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_task_dump(chan);
#endif
    return 0;
}

int hal_dma_chan_start(struct aic_dma_chan *chan)
{
    u32 value;
    struct aic_dma_task *task;
    struct aic_dma_dev *aich_dma;
    aich_dma = get_aic_dma_dev();

    CHECK_PARAM(chan != NULL && chan->desc != NULL, -EINVAL);

    for (task = chan->desc; task != NULL; task = task->v_next)
        aicos_dcache_clean_range((void *)(unsigned long)task, sizeof(*task));

    if (!chan->cyclic)
        chan->irq_type = DMA_IRQ_ALL_TASK;

    value = readl(aich_dma->base + DMA_IRQ_EN_REG(0));
    value &= ~(DMA_IRQ_MASK(chan->ch_nr));
    value |= chan->irq_type << DMA_IRQ_SHIFT(chan->ch_nr);
    writel(value, aich_dma->base + DMA_IRQ_EN_REG(0));

    writel(chan->desc->mode, chan->base + DMA_CH_MODE_REG);
    writel((u32)(unsigned long)(chan->desc), chan->base + DMA_CH_TASK_REG);
    if (chan->memset)
        writel(DMA_CH_MEMSET, chan->base + DMA_CH_PAUSE_REG);
    else
        writel(0x00, chan->base + DMA_CH_PAUSE_REG);
    writel(0x01, chan->base + DMA_CH_EN_REG);

#ifdef AIC_DMA_DRV_DEBUG
    hal_dma_reg_dump(chan);
#endif

    return 0;
}
