/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARTINCHIP_HAL_DMA_REG_V1X_H_
#define _ARTINCHIP_HAL_DMA_REG_V1X_H_

#include "aic_core.h"
#include "hal_dma.h"
#include "aic_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_MAX_NUM            36

#define DELAY_DEF_VAL           0x40

#define DMA_IRQ_CHAN_NR         4
#define MAX_LEN                 0x2000000

#define DMA_IRQ_EN_REG(x)       ((x) * 0x04 + 0x00)
#define DMA_IRQ_DIS_REG(x)      ((x) * 0x04 + 0x20)

#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11) \
    || defined(AIC_DMA_DRV_V12)
#define DMA_CHAN_OFFSET         (0x40)
#define DMA_IRQ_STA_REG(x)      ((x) * 0x04 + 0x10)
#define DMA_CH_STA_REG          (0x0030)
#define DMA_LINK_END_FLAG       0xFFFFF800
#endif
/*
 * define dma_v1.x register list
 */
#define DMA_MEM_CFG             (0x0020)
#define DMA_GATE_REG            (0x0028)
#define DMA_CH_EN_REG           (0x0000)
#define DMA_CH_PAUSE_REG        (0x0004)
#define DMA_CH_TASK_REG         (0x0008)
#define DMA_CH_CFG_REG          (0x000C)
#define DMA_CH_SRC_REG          (0x0010)
#define DMA_CH_SINK_REG         (0x0014)
#define DMA_CH_LEFT_REG         (0x0018)
#define DMA_CH_RDC_COMPLETE_REG (0x0020)
#define DMA_CH_MODE_REG         (0x0028)
#define DMA_CH_PKG_NUM_REG      (0x0030)
#define DMA_CH_MEMSET_VAL_REG   (0x0034)

/*
 * define macro for access register for specific channel
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11) \
    || defined(AIC_DMA_DRV_V12)
#define DMA_IRQ_HALF_TASK   BIT(0)
#define DMA_IRQ_ONE_TASK    BIT(1)
#define DMA_IRQ_ALL_TASK    BIT(2)
#define DMA_IRQ_TASK_DONE   (DMA_IRQ_HALF_TASK | DMA_IRQ_ONE_TASK | DMA_IRQ_ALL_TASK)
#define DMA_IRQ_CH_WIDTH    (4)
#define DMA_IRQ_MASK(ch)    (GENMASK(2, 0) << DMA_IRQ_SHIFT(ch))
#endif

#define DMA_IRQ_SHIFT(ch)   (DMA_IRQ_CH_WIDTH * (ch))
#define AIC_DMA_BUS_WIDTH                                                   \
    (BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | BIT(DMA_SLAVE_BUSWIDTH_2_BYTES)  |    \
     BIT(DMA_SLAVE_BUSWIDTH_4_BYTES) | BIT(DMA_SLAVE_BUSWIDTH_8_BYTES) |    \
     BIT(DMA_SLAVE_BUSWIDTH_16_BYTES))
/*
 * define bit index in channel configuration register
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11) \
    || defined(AIC_DMA_DRV_V12)
/* dma_v1.x task  config */
#define DST_WIDTH_BITSHIFT  25
#define DST_ADDR_BITSHIFT   24
#define DST_BURST_BITSHIFT  22
#define DST_PORT_BITSHIFT   16
#define SRC_WIDTH_BITSHIFT  9
#define SRC_ADDR_BITSHIFT   8
#define SRC_BURST_BITSHIFT  6
#define SRC_PORT_BITSHIFT   0
#define ADDR_LINEAR_MODE    0
#define ADDR_FIXED_MODE     1
#endif
#define GET_DMA_DST_BURST(x)    (((x) << DST_BURST_BITSHIFT) & GENMASK(1, 0))
#define GET_DMA_SRC_BURST(x)    (((x) << DST_BURST_BITSHIFT) & GENMASK(1, 0))
#define DMA_DRQ_PORT_MASK   0x3F
#define DMA_WAIT_MODE       0
#define DMA_HANDSHAKE_MODE  1
#define DMA_DST_MODE_SHIFT  3
#define DMA_SRC_MODE_SHIFT  2
#if defined(AIC_DMA_DRV_V12)
#define DMA_SRC_HANDSHAKE_ENABLE    5
#define DMA_DST_HANDSHAKE_ENABLE    6
#else
#define DMA_SRC_HANDSHAKE_ENABLE    4
#define DMA_DST_HANDSHAKE_ENABLE    4
#endif
#define DMA_S_WAIT_D_HANDSHAKE  (DMA_HANDSHAKE_MODE << DMA_DST_MODE_SHIFT)
#define DMA_S_HANDSHAKE_D_WAIT  (DMA_HANDSHAKE_MODE << DMA_SRC_MODE_SHIFT)
#define DMA_S_WAIT_D_WAIT   (DMA_WAIT_MODE)
#define DMA_FIFO_SIZE       0x200

/*
 * define bit index in channel pause register
 */
#if defined(AIC_DMA_DRV_V10) || defined(AIC_DMA_DRV_V11) \
    || defined(AIC_DMA_DRV_V12)
#define  DMA_CH_RESUME      0x00
#define  DMA_CH_PAUSE       0x01
#endif

#define DMA_CH_MEMSET       0x10

#ifdef __cplusplus
}
#endif

#define DMA_SLAVE_DEF(_id, _burst, _width) \
    static const struct dma_slave_table aic_dma_cfg_##_id = { \
        .id = _id, \
        .burst = _burst, \
        .burst_num = ARRAY_SIZE(_burst), \
        .width = _width, \
        .width_num = ARRAY_SIZE(_width), \
    }

#define AIC_DMA_CFG(_id)  [_id] = &(aic_dma_cfg_##_id)

static const u32 dma_width_1_byte[] = {DMA_SLAVE_BUSWIDTH_1_BYTE};
static const u32 dma_width_2_bytes[] = {DMA_SLAVE_BUSWIDTH_2_BYTES};
static const u32 dma_width_4_bytes[] = {DMA_SLAVE_BUSWIDTH_4_BYTES};
static const u32 dma_width_2_4_bytes[] = {DMA_SLAVE_BUSWIDTH_2_BYTES, DMA_SLAVE_BUSWIDTH_4_BYTES};
static const u32 dma_width_1_4_bytes[] = {DMA_SLAVE_BUSWIDTH_1_BYTE, DMA_SLAVE_BUSWIDTH_4_BYTES};

static const u32 dma_burst_1[] = {1};
static const u32 dma_burst_4[] = {4};
static const u32 dma_burst_8[] = {8};
static const u32 dma_burst_16[] = {16};
static const u32 dma_burst_1_8[] = {1, 8};

#if defined(AIC_DMA_DRV_V10)
/*                 ID                 burst            witdh(byte) */
DMA_SLAVE_DEF(DMA_ID_PSADC_Q1,      dma_burst_1,    dma_width_4_bytes);
DMA_SLAVE_DEF(DMA_ID_PSADC_Q2,      dma_burst_1,    dma_width_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI2,          dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_SPI3,          dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_SPI0,          dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_SPI1,          dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_I2S0,          dma_burst_1,    dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_I2S1,          dma_burst_1,    dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_AUDIO_DMIC,    dma_burst_1,    dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_AUDIO_ADC,     dma_burst_1,    dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_UART0,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART1,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART2,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART3,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART4,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART5,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART6,         dma_burst_1,    dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART7,         dma_burst_1,    dma_width_1_byte);
static const struct dma_slave_table *aic_dma_slave_table[AIC_DMA_PORTS] = {
    AIC_DMA_CFG(DMA_ID_PSADC_Q1),
    AIC_DMA_CFG(DMA_ID_PSADC_Q2),
    AIC_DMA_CFG(DMA_ID_SPI2),
    AIC_DMA_CFG(DMA_ID_SPI3),
    AIC_DMA_CFG(DMA_ID_SPI0),
    AIC_DMA_CFG(DMA_ID_SPI1),
    AIC_DMA_CFG(DMA_ID_I2S0),
    AIC_DMA_CFG(DMA_ID_I2S1),
    AIC_DMA_CFG(DMA_ID_AUDIO_DMIC),
    AIC_DMA_CFG(DMA_ID_AUDIO_ADC),
    AIC_DMA_CFG(DMA_ID_UART0),
    AIC_DMA_CFG(DMA_ID_UART1),
    AIC_DMA_CFG(DMA_ID_UART2),
    AIC_DMA_CFG(DMA_ID_UART3),
    AIC_DMA_CFG(DMA_ID_UART4),
    AIC_DMA_CFG(DMA_ID_UART5),
    AIC_DMA_CFG(DMA_ID_UART6),
    AIC_DMA_CFG(DMA_ID_UART7),
};
#elif defined(AIC_DMA_DRV_V11)
/*                 ID                   burst              witdh(byte)*/
DMA_SLAVE_DEF(DMA_ID_PSADC_Q1,      dma_burst_1,        dma_width_4_bytes);
DMA_SLAVE_DEF(DMA_ID_PSADC_Q2,      dma_burst_1,        dma_width_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI2,          dma_burst_8,        dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI3,          dma_burst_8,        dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI0,          dma_burst_8,        dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI1,          dma_burst_8,        dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_I2S0,          dma_burst_1,        dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_I2S1,          dma_burst_1,        dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_AUDIO_DMIC,    dma_burst_1,        dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_UART0,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART1,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART2,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART3,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART4,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART5,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART6,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART7,         dma_burst_1,        dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_XSPI,          dma_burst_16,       dma_width_1_byte);
static const struct dma_slave_table *aic_dma_slave_table[AIC_DMA_PORTS] = {
    AIC_DMA_CFG(DMA_ID_PSADC_Q1),
    AIC_DMA_CFG(DMA_ID_PSADC_Q2),
    AIC_DMA_CFG(DMA_ID_SPI2),
    AIC_DMA_CFG(DMA_ID_SPI3),
    AIC_DMA_CFG(DMA_ID_SPI0),
    AIC_DMA_CFG(DMA_ID_SPI1),
    AIC_DMA_CFG(DMA_ID_I2S0),
    AIC_DMA_CFG(DMA_ID_I2S1),
    AIC_DMA_CFG(DMA_ID_AUDIO_DMIC),
    AIC_DMA_CFG(DMA_ID_UART0),
    AIC_DMA_CFG(DMA_ID_UART1),
    AIC_DMA_CFG(DMA_ID_UART2),
    AIC_DMA_CFG(DMA_ID_UART3),
    AIC_DMA_CFG(DMA_ID_UART4),
    AIC_DMA_CFG(DMA_ID_UART5),
    AIC_DMA_CFG(DMA_ID_UART6),
    AIC_DMA_CFG(DMA_ID_UART7),
    AIC_DMA_CFG(DMA_ID_XSPI),
};
#elif defined(AIC_DMA_DRV_V12)
/*                 ID                   burst              witdh(byte)*/
DMA_SLAVE_DEF(DMA_ID_SPI0,          dma_burst_8,      dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_SPI1,          dma_burst_8,      dma_width_1_4_bytes);
DMA_SLAVE_DEF(DMA_ID_AUDIO_DMIC,    dma_burst_1,      dma_width_2_4_bytes);
DMA_SLAVE_DEF(DMA_ID_UART0,         dma_burst_1,      dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART1,         dma_burst_1,      dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART2,         dma_burst_1,      dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_UART3,         dma_burst_1,      dma_width_1_byte);
DMA_SLAVE_DEF(DMA_ID_XSPI,          dma_burst_16,     dma_width_1_byte);
static const struct dma_slave_table *aic_dma_slave_table[AIC_DMA_PORTS] = {
    AIC_DMA_CFG(DMA_ID_SPI0),
    AIC_DMA_CFG(DMA_ID_SPI1),
    AIC_DMA_CFG(DMA_ID_AUDIO_DMIC),
    AIC_DMA_CFG(DMA_ID_UART0),
    AIC_DMA_CFG(DMA_ID_UART1),
    AIC_DMA_CFG(DMA_ID_UART2),
    AIC_DMA_CFG(DMA_ID_UART3),
    AIC_DMA_CFG(DMA_ID_XSPI),
};
#endif /* DMA_SLAVE_TABLE */

struct aic_dma_dev {
    struct aic_dma_task task[TASK_MAX_NUM];
    s32 inited;
    unsigned long base;
    u32 burst_length; /* burst length capacity */
    u32 addr_widths; /* address width support capacity */
    struct aic_dma_chan dma_chan[AIC_DMA_CH_NUM];
    struct aic_dma_task *freetask;
    const struct dma_slave_table **slave_table;
} __ALIGNED(CACHE_LINE_SIZE);

struct aic_dma_dev *get_aic_dma_dev(void);
void *aic_dma_task_add(struct aic_dma_task *prev,
                        struct aic_dma_task *next,
                        struct aic_dma_chan *chan);
int aic_set_burst(struct dma_slave_config *sconfig,
                    enum dma_transfer_direction direction,
                    u32 *p_cfg);
struct aic_dma_task *aic_dma_task_alloc(void);
void aic_dma_free_desc(struct aic_dma_chan *chan);

#endif /*_ARTINCHIP_HAL_DMA_REG_V1X_H_  */
