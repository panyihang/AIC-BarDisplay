/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji Chen <jiji.chen@artinchip.com>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <aic_core.h>
#include <aic_drv.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <hal_dma.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>

#define ASYNC_DATA_SIZE 64

struct aic_qspi {
    struct rt_spi_bus dev;
    char *name;
    uint32_t idx;
    uint32_t clk_id;
    uint32_t clk_in_hz;
    uint32_t dma_port_id;
    uint32_t irq_num;
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    uint32_t cs_num;
#endif
    qspi_master_handle handle;
    struct rt_qspi_configuration configuration;
    rt_sem_t xfer_sem;
    bool inited;
    uint32_t rxd_dlymode;
    uint32_t txd_dlymode;
    uint32_t txc_dlymode;
    bool nonblock;
    struct rt_completion *completion;
    bool slave_en;
};

static struct aic_qspi spi_controller[] = {
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_BUS_SPI)
    {
        .name = "spi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI1_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI1_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI1_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI1_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_QSPI2_BUS_SPI)
    {
        .name = "spi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI2_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI2_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI2_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI2_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI3) && defined(AIC_QSPI3_BUS_SPI)
    {
        .name = "spi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI3_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI3_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI3_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI3_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI4) && defined(AIC_QSPI4_BUS_SPI)
    {
        .name = "spi4",
        .idx = 4,
        .clk_id = CLK_QSPI4,
        .clk_in_hz = AIC_DEV_QSPI4_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI4,
        .irq_num = QSPI4_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI4_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI4_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI4_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI4_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI5) && defined(AIC_QSPI5_BUS_SPI)
    {
        .name = "spi5",
        .idx = 5,
        .clk_id = CLK_QSPI5,
        .clk_in_hz = AIC_DEV_QSPI5_MAX_SRC_FREQ_HZ,
        .dma_port_id = DMA_ID_SPI5,
        .irq_num = QSPI5_IRQn,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI5_CS_NUM,
#endif
        .rxd_dlymode = AIC_DEV_QSPI5_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dlymode = AIC_DEV_QSPI5_TXD_DELAY_MODE,
        .txc_dlymode = AIC_DEV_QSPI5_TX_CLK_DELAY_MODE,
#endif
    },
#endif
};

static const u32 bit_mode_table[] = {
#ifdef AIC_QSPI1_BIT_MODE
    1,
#endif
#ifdef AIC_QSPI2_BIT_MODE
    2,
#endif
#ifdef AIC_QSPI3_BIT_MODE
    3,
#endif
};

static bool spi_support_bit_mode(struct aic_qspi *qspi)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(bit_mode_table); i++) {
        if (bit_mode_table[i] == qspi->idx)
            return true;
    }
    return false;
}

#ifdef AIC_QSPI_DRV_DEBUG
void dump_cmd(uint8_t *data, uint32_t len)
{
    uint32_t i;

    printf("CMD: (%lu) ", (unsigned long)len);
    for (i = 0; i < len; i++)
        printf("%02X ", data[i]);
    printf("\n");
}
#endif

static rt_uint32_t drv_spi_send_async(struct aic_qspi *qspi, struct qspi_transfer *t)
{
    rt_completion_init(qspi->completion);

    return hal_qspi_master_transfer_async(&qspi->handle, t);
}

static rt_uint32_t drv_spi_send(struct aic_qspi *qspi,
                                 struct rt_spi_message *message,
                                 const uint8_t *tx, uint32_t size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    t.rx_data = NULL;
    t.tx_data = (uint8_t *)tx;
    t.data_len = size;

    if (qspi->nonblock)
        ret = drv_spi_send_async(qspi, &t);
    else
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);

    return ret;
}

static rt_uint32_t drv_spi_receive(struct aic_qspi *qspi,
                                 struct rt_spi_message *message,
                                 const uint8_t *tx, uint32_t size)
{
    struct qspi_transfer t;
    u32 ret = 0;

    t.tx_data = NULL;
    t.rx_data = (uint8_t *)tx;
    t.data_len = size;
    ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    return ret;
}

static rt_uint32_t drv_spi_transfer(struct aic_qspi *qspi,
                             struct rt_spi_message *message)
{
    struct qspi_transfer t;

    t.tx_data = (u8 *)message->send_buf;
    t.rx_data = message->recv_buf;
    t.data_len = message->length;

    if (qspi->nonblock) {
        rt_completion_init(qspi->completion);
        return hal_qspi_master_transfer_async(&qspi->handle, &t);
    }
    return hal_qspi_master_transfer_sync(&qspi->handle, &t);
}

static void spi_set_cs_before(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct rt_qspi_configuration *cfg;
    struct aic_qspi *qspi;
    u32 cs_num = 0;

    qspi = (struct aic_qspi *)device->bus;
    cfg = &qspi->configuration;

    if (message->cs_take && !(cfg->parent.mode & RT_SPI_NO_CS)) {
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
        hal_qspi_master_set_cs(&qspi->handle, cs_num, true);
    }
}

static void spi_set_cs_after(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct rt_qspi_configuration *cfg;
    struct aic_qspi *qspi;
    u32 cs_num = 0;

    qspi = (struct aic_qspi *)device->bus;
    cfg = &qspi->configuration;

    if (message->cs_release && !(cfg->parent.mode & RT_SPI_NO_CS)) {
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
        if (!qspi->nonblock)
            hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    }
}

#ifdef AIC_CHIP_D13X
static rt_uint32_t drv_spi_slave_transfer(struct aic_qspi *qspi,
    struct rt_spi_message *message)
{
    u32 ret = 0;
    struct qspi_transfer t;

    t.tx_data = (u8 *)message->send_buf;
    t.rx_data = message->recv_buf;
    t.data_len = message->length;
    ret = hal_qspi_slave_transfer_async((qspi_slave_handle *)&qspi->handle, &t);
    return ret;
}
#endif

static rt_uint32_t spi_xfer(struct rt_spi_device *device,
                             struct rt_spi_message *message)
{
    struct aic_qspi *qspi;
    rt_uint32_t ret = 0;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;

#ifdef AIC_CHIP_D13X
    if (qspi->slave_en) {
        ret = drv_spi_slave_transfer(qspi, message);

        if (ret)
            return ret;
        return message->length;
    }
#endif

    spi_set_cs_before(device, message);

    if (message->recv_buf && message->send_buf)
        ret = drv_spi_transfer(qspi, message);
    else if (message->recv_buf)
        ret = drv_spi_receive(qspi, message, message->recv_buf, message->length);
    else if (message->send_buf)
        ret = drv_spi_send(qspi, message, message->send_buf, message->length);

    spi_set_cs_after(device, message);

    if (ret)
        return ret;
    return message->length;
}

static void qspi_master_async_callback(qspi_master_handle *h, void *priv)
{
    struct aic_qspi *qspi = priv;
    u32 cs_num = 0;

#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        cs_num = qspi->cs_num;
#endif
    if (qspi->xfer_sem)
        rt_sem_release(qspi->xfer_sem);
    hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    rt_completion_done(qspi->completion);
}

static irqreturn_t qspi_irq_handler(int irq_num, void *arg)
{
    qspi_master_handle *h = arg;

    rt_interrupt_enter();
    hal_qspi_master_irq_handler(h);
    rt_interrupt_leave();

    return IRQ_HANDLED;
}

static rt_err_t spi_master_init(struct aic_qspi *qspi, struct qspi_master_config *cfg)
{
    rt_err_t ret = RT_EOK;

    ret = hal_qspi_master_init(&qspi->handle, cfg);
    if (ret) {
        pr_err("spi init failed.\n");
        return ret;
    }
    if (!qspi->inited) {
#ifdef AIC_DMA_DRV
        /* Only request DMA channels on first init.
         * After deep sleep, old dma_rx/dma_tx are still valid. */
        struct qspi_master_dma_config dmacfg;
        rt_memset(&dmacfg, 0, sizeof(dmacfg));
        dmacfg.port_id = qspi->dma_port_id;

        ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
        if (ret) {
            pr_err("qspi dma config failed.\n");
            return ret;
        }
#endif
        ret = hal_qspi_master_register_cb(&qspi->handle, qspi_master_async_callback, qspi);
        if (ret) {
            pr_err("qspi register async callback failed.\n");
            return ret;
        }
        aicos_request_irq(qspi->irq_num, qspi_irq_handler, 0, NULL, (void *)&qspi->handle);
        aicos_irq_enable(qspi->irq_num);
    }
    hal_qspi_master_set_bus_freq(&qspi->handle, cfg->max_hz);

    return ret;
}

#ifdef AIC_CHIP_D13X
static void spi_slave_async_callback(qspi_slave_handle *h, void *priv)
{
    int status, __attribute__ ((unused)) cnt;

    status = hal_qspi_slave_get_status(h);
    cnt = 0;
    if (status == HAL_QSPI_STATUS_OK) {
        cnt = hal_qspi_slave_transfer_count(h);
        pr_debug("SPI slave transfer %d bytes done.\n", cnt);
    } else {
        pr_err("SPI slave transfer status err: %d.\n", status);
    }
}

static irqreturn_t spi_slave_irq_handler(int irq_num, void *arg)
{
    qspi_slave_handle *h = arg;

    rt_interrupt_enter();
    hal_qspi_slave_irq_handler(h);
    rt_interrupt_leave();

    return IRQ_HANDLED;
}

static rt_err_t spi_slave_init(struct aic_qspi *qspi, struct qspi_master_config *cfg)
{
    rt_err_t ret = RT_EOK;
    qspi_slave_handle *h = (qspi_slave_handle *)&qspi->handle;

    ret = hal_qspi_slave_init(h, (struct qspi_slave_config *)cfg);
    if (ret) {
        pr_err("spi init failed.\n");
        return ret;
    }

    if (!qspi->inited) {
        ret = hal_qspi_slave_register_cb(h, spi_slave_async_callback, NULL);
        if (ret) {
            pr_err("spi register async callback failed.\n");
            return ret;
        }
        aicos_request_irq(qspi->irq_num, spi_slave_irq_handler, 0, NULL, (void *)h);
        aicos_irq_enable(qspi->irq_num);
    }
    return ret;
}
#endif

static rt_err_t spi_hw_init(struct aic_qspi *qspi)
{
    struct qspi_master_config cfg;
    rt_uint32_t bus_hz;
    rt_err_t ret = RT_EOK;

    rt_memset(&cfg, 0, sizeof(cfg));
    cfg.idx = qspi->idx;
    cfg.clk_id = qspi->clk_id;

    bus_hz = qspi->configuration.parent.max_hz;
    if (bus_hz < HAL_QSPI_MIN_FREQ_HZ)
        bus_hz = HAL_QSPI_MIN_FREQ_HZ;
    if (bus_hz > HAL_QSPI_MAX_FREQ_HZ)
        bus_hz = HAL_QSPI_MAX_FREQ_HZ;
    cfg.max_hz = bus_hz;
    cfg.clk_in_hz = qspi->clk_in_hz;
    if (cfg.clk_in_hz > HAL_QSPI_MAX_FREQ_HZ)
        cfg.clk_in_hz = HAL_QSPI_MAX_FREQ_HZ;
    if (qspi->clk_in_hz % bus_hz)
        cfg.clk_in_hz = bus_hz;
    if (cfg.clk_in_hz < HAL_QSPI_INPUT_MIN_FREQ_HZ)
        cfg.clk_in_hz = HAL_QSPI_INPUT_MIN_FREQ_HZ;

    if (qspi->configuration.parent.mode & RT_SPI_MSB)
        cfg.lsb_en = false;
    else
        cfg.lsb_en = true;
    if (qspi->configuration.parent.mode & RT_SPI_CPHA)
        cfg.cpha = HAL_QSPI_CPHA_SECOND_EDGE;
    else
        cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;

    if (qspi->configuration.parent.mode & RT_SPI_CPOL)
        cfg.cpol = HAL_QSPI_CPOL_ACTIVE_LOW;
    else
        cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
    if (qspi->configuration.parent.mode & RT_SPI_CS_HIGH)
        cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_HIGH;
    else
        cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;
    if (qspi->configuration.parent.mode & RT_SPI_3WIRE)
        cfg.wire3_en = true;
    else
        cfg.wire3_en = false;

    cfg.rx_dlymode = qspi->rxd_dlymode;
    cfg.tx_dlymode = aic_convert_tx_dlymode(qspi->txc_dlymode, qspi->txd_dlymode);

#ifdef AIC_CHIP_D12P
    if (cfg.idx >= 3) {
        cfg.fifo_depth = HAL_QSPI_FIFO_DEPTH_V3LITE;
        cfg.dev_max_burst = HAL_QSPI_DMA_DEV_BURST_ONE;
    } else {
        cfg.fifo_depth = HAL_QSPI_FIFO_DEPTH;
        cfg.dev_max_burst = HAL_QSPI_DMA_DEV_MAXBURST;
    }
#else
    cfg.fifo_depth = HAL_QSPI_FIFO_DEPTH;
    cfg.dev_max_burst = HAL_QSPI_DMA_DEV_MAXBURST;
#endif

    if (qspi->slave_en) {
        cfg.slave_en = true;
#ifdef AIC_CHIP_D13X
        ret = spi_slave_init(qspi, &cfg);
#endif
    } else {
        cfg.slave_en = false;
        ret = spi_master_init(qspi, &cfg);
    }

    return ret;
}

#ifdef AIC_PM_DRV_V15
static int spi_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_qspi *qspi = rt_container_of(device, struct aic_qspi, dev.parent);
    rt_uint32_t status;

    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
        if (!qspi->slave_en) {
            status = hal_qspi_master_get_status(&qspi->handle);
            if (status & HAL_QSPI_STATUS_IN_PROGRESS) {
                pr_warn("spi%d transfer in progress, block light sleep\n",
                        qspi->idx);
                return -EBUSY;
            }
        }
        hal_clk_disable(qspi->clk_id);
        break;
    case PM_SLEEP_MODE_DEEP:
        break;
    default:
        break;
    }

    return 0;
}

static void spi_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aic_qspi *qspi = rt_container_of(device, struct aic_qspi, dev.parent);
    rt_err_t ret;

    if (!qspi->inited) {
        /* If QSPI was not initialized before sleep,
         * no need to reinit hardware or enable clk. */
        return;
    }
    switch (mode) {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_STANDBY:
        hal_clk_enable(qspi->clk_id);
        break;
    case PM_SLEEP_MODE_DEEP:
        /* DMA channel does not need to be re-requested.
         * hal_dma_init() keeps 'used' and clears 'desc'.
         * hal_qspi_master_init() enables clk and deasserts reset internally.
         * Just call spi_hw_init() directly here. */
        ret = spi_hw_init(qspi);
        if (ret)
            pr_err("spi%d resume reinit failed: %d\n", qspi->idx, ret);
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aic_spi_pm_ops = {
    SET_DEVICE_PM_OPS(spi_suspend, spi_resume)
    NULL,
};

static int spi_pm_init(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(spi_controller); i++) {
        rt_pm_device_register(&spi_controller[i].dev.parent,
                              &aic_spi_pm_ops);
    }

    return 0;
}
INIT_DEVICE_EXPORT(spi_pm_init);
#endif

static rt_err_t spi_configure(struct rt_spi_device *device,
                               struct rt_spi_configuration *configuration)
{
    struct aic_qspi *qspi;
    rt_err_t ret = RT_EOK;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    qspi = (struct aic_qspi *)device->bus;

    if (spi_support_bit_mode(qspi))
        qspi->handle.bit_mode = true;
    else
        qspi->handle.bit_mode = false;

    if ((rt_memcmp(&qspi->configuration, configuration,
                   sizeof(struct rt_spi_configuration)) != 0) ||
        (qspi->handle.bit_mode == true)) {
        rt_memcpy(&qspi->configuration, configuration,
                  sizeof(struct rt_spi_configuration));

        if (configuration->mode & RT_SPI_SLAVE)
            qspi->slave_en = true;
        else
            qspi->slave_en = false;

        ret = spi_hw_init(qspi);
        if (ret)
            return ret;
    }
    qspi->inited = true;
    return ret;
}

static rt_err_t spi_nonblock_set(struct rt_spi_device *device,
                               rt_uint32_t nonblock)
{
    struct aic_qspi *qspi;

    qspi = (struct aic_qspi *)device->bus;

    if (nonblock) {
        if (!qspi->completion) {
            qspi->completion = (struct rt_completion *)rt_malloc(sizeof(struct rt_completion));
            if (!qspi->completion)
                return -RT_ERROR;
        }
        qspi->nonblock = true;
    } else {
        qspi->nonblock = false;
        if (qspi->completion) {
            rt_free(qspi->completion);
            qspi->completion = NULL;
        }
    }

    return RT_EOK;
}

static rt_uint32_t spi_get_transfer_status(struct rt_spi_device *device)
{
    struct aic_qspi *qspi;

    qspi = (struct aic_qspi *)device->bus;

    return hal_qspi_master_get_status(&qspi->handle);
}

static rt_err_t spi_wait_completion(struct rt_spi_device *device)
{
    rt_err_t result = RT_EOK;
    struct aic_qspi *qspi;

    qspi = (struct aic_qspi *)device->bus;

    result = rt_completion_wait(qspi->completion, 1000);
    if (result != RT_EOK)
        pr_err("spi wait transfer done timeout\n");

    return result;
}

rt_err_t aic_spi_bus_attach_device(const char *bus_name,
                                    const char *device_name)
{
    struct rt_spi_device *spi_device = RT_NULL;
    rt_err_t result = RT_EOK;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    spi_device =
        (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (!spi_device)
        return -RT_ENOMEM;

    result = rt_spi_bus_attach_device(spi_device, device_name,
                                      bus_name, RT_NULL);

    if (result != RT_EOK)
        rt_free(spi_device);

    return result;
}

static const struct rt_spi_ops aic_spi_ops = {
    .configure = spi_configure,
    .xfer = spi_xfer,
    .nonblock = spi_nonblock_set,
    .gstatus = spi_get_transfer_status,
    .wait_completion = spi_wait_completion,
};

static int rt_hw_spi_bus_init(void)
{
    char sem_name[RT_NAME_MAX];
    rt_err_t ret = RT_EOK;
    int i;

    for (i = 0; i < ARRAY_SIZE(spi_controller); i++) {
        ret = rt_spi_bus_register(&spi_controller[i].dev,
                                   spi_controller[i].name, &aic_spi_ops);
        if (ret != RT_EOK)
            break;
        rt_sprintf(sem_name, "%s_s", spi_controller[i].name);
        spi_controller[i].xfer_sem =
            rt_sem_create(sem_name, 0, RT_IPC_FLAG_PRIO);
        if (!spi_controller[i].xfer_sem) {
            ret = RT_ENOMEM;
            break;
        }

    }

    return ret;
}

INIT_BOARD_EXPORT(rt_hw_spi_bus_init);
