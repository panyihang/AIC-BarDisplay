/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sfud.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_log.h>
#include <aic_hal.h>
#include <hal_qspi.h>
#include <spinor_port.h>
#include <hal_dma.h>
#include <aic_dma_id.h>
#include <aic_clk_id.h>
#include <mtd.h>
#include <aic_osal.h>
#include <partition_table.h>
#include <spienc.h>

#ifdef IMAGE_CFG_JSON_PARTS_MTD
#define NOR_MTD_PARTS IMAGE_CFG_JSON_PARTS_MTD
#else
#define NOR_MTD_PARTS ""
#endif

#define SFUD_READ_SFDP_FREQ 50000000

#define QSPI_MAX_CNT 4

static struct aic_qspi_bus qspi_bus_arr[] = {
#if defined(AIC_USING_QSPI0) && defined(AIC_QSPI0_DEVICE_SPINOR)
    {
        .name = "qspi0",
        .idx = 0,
        .clk_id = CLK_QSPI0,
        .clk_in_hz = AIC_DEV_QSPI0_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI0_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI0,
        .irq_num = QSPI0_IRQn,
        .dl_width = AIC_QSPI0_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI0_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI0_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI0_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI0_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    {
        .name = "qspi1",
        .idx = 1,
        .clk_id = CLK_QSPI1,
        .clk_in_hz = AIC_DEV_QSPI1_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI1_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI1,
        .irq_num = QSPI1_IRQn,
        .dl_width = AIC_QSPI1_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI1_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI1_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI1_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI1_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_QSPI2_DEVICE_SPINOR)
    {
        .name = "qspi2",
        .idx = 2,
        .clk_id = CLK_QSPI2,
        .clk_in_hz = AIC_DEV_QSPI2_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI2_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI2,
        .irq_num = QSPI2_IRQn,
        .dl_width = AIC_QSPI2_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI2_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI2_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI2_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI2_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI3)
    {
        .name = "qspi3",
        .idx = 3,
        .clk_id = CLK_QSPI3,
        .clk_in_hz = AIC_DEV_QSPI3_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI3_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI3,
        .irq_num = QSPI3_IRQn,
        .dl_width = AIC_QSPI3_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI3_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI3_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI3_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI3_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_QSPI4)
    {
        .name = "qspi4",
        .idx = 4,
        .clk_id = CLK_QSPI4,
        .clk_in_hz = AIC_DEV_QSPI4_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_QSPI4_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SPI4,
        .irq_num = QSPI4_IRQn,
        .dl_width = AIC_QSPI4_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_QSPI4_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_QSPI4_DELAY_MODE,
#if defined(AIC_QSPI_DRV_V20)
        .txd_dylmode = AIC_DEV_QSPI4_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_QSPI4_TX_CLK_DELAY_MODE,
#endif
    },
#endif
#if defined(AIC_USING_SE_SPI) && defined(AIC_SE_SPI_DEVICE_SPINOR)
    {
        .name = "sespi",
        .idx = 5,
        .clk_id = CLK_SE_SPI,
        .clk_in_hz = AIC_DEV_SE_SPI_MAX_SRC_FREQ_HZ,
        .bus_hz = AIC_SE_SPI_DEVICE_SPINOR_FREQ,
        .dma_port_id = DMA_ID_SE_SPI,
        .irq_num = SE_SPI_IRQn,
        .dl_width = AIC_SE_SPI_BUS_WIDTH,
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
        .cs_num = AIC_SE_SPI_CS_NUM,
#endif
        .rxd_dylmode = AIC_DEV_SE_SPI_DELAY_MODE,
        .txd_dylmode = AIC_DEV_SE_SPI_TXD_DELAY_MODE,
        .txc_dylmode = AIC_DEV_SE_SPI_TX_CLK_DELAY_MODE,
    },
#endif
};

char *aic_spinor_get_partition_string(struct mtd_dev *mtd);
static void retry_delay_100us(void)
{
    aicos_udelay(100);
}

static struct aic_qspi_bus *get_qspi_by_index(u32 idx)
{
    struct aic_qspi_bus *qspi;
    u32 i;

    qspi = NULL;
    for (i = 0; i < ARRAY_SIZE(qspi_bus_arr); i++) {
        if (qspi_bus_arr[i].idx == idx) {
            qspi = &qspi_bus_arr[i];
            break;
        }
    }

    return qspi;
}

#ifdef SFUD_USING_QSPI

static u32 address_copy(u32 addr, u32 size, uint8_t *dst)
{
    u32 i;

    i = 0;
    while (size) {
        dst[i++] = (addr >> (8 * (size - 1))) & 0xFF;
        size--;
    }
    return i;
}

static sfud_err qspi_read(const struct __sfud_spi *spi, u32 addr,
                          sfud_qspi_read_cmd_format *rd_fmt, uint8_t *read_buf,
                          size_t read_size)
{
    struct aic_qspi_bus *qspi;
    struct qspi_transfer t;
    uint8_t cmdbuf1[16];
    uint8_t cmdbuf2[16];
    u32 addrsiz, cs_num = 0;
    int ret, single_cnt, rest_cnt, dummy_cnt, rest_buswidth;

    qspi = (struct aic_qspi_bus *)spi->user_data;

    single_cnt = 0;
    rest_cnt = 0;
    rest_buswidth = HAL_QSPI_BUS_WIDTH_SINGLE;
    dummy_cnt = (rd_fmt->address_lines * rd_fmt->dummy_cycles / 8);
    if (rd_fmt->instruction_lines == 1) {
        cmdbuf1[single_cnt] = rd_fmt->instruction;
        single_cnt++;
    } else if (rd_fmt->instruction_lines > 1) {
        cmdbuf2[rest_cnt] = rd_fmt->instruction;
        rest_cnt++;
        if (rd_fmt->instruction_lines > rest_buswidth)
            rest_buswidth = rd_fmt->instruction_lines;
    }

    addrsiz = rd_fmt->address_size / 8;
    if (rd_fmt->address_lines == 1) {
        single_cnt += address_copy(addr, addrsiz, &cmdbuf1[single_cnt]);
        memset(&cmdbuf1[single_cnt], 0, dummy_cnt);
        single_cnt += dummy_cnt;
    } else if (rd_fmt->address_lines > 1) {
        rest_cnt += address_copy(addr, addrsiz, &cmdbuf2[rest_cnt]);
        memset(&cmdbuf2[rest_cnt], 0, dummy_cnt);
        rest_cnt += dummy_cnt;
        if (rd_fmt->address_lines > rest_buswidth)
            rest_buswidth = rd_fmt->address_lines;
    }

#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, true);

    /* Command phase */
    if (single_cnt) {
        hal_qspi_master_set_bus_width(&qspi->handle, HAL_QSPI_BUS_WIDTH_SINGLE);
        t.rx_data = NULL;
        t.tx_data = cmdbuf1;
        t.data_len = single_cnt;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret)
            goto out;
    }
    if (rest_cnt) {
        hal_qspi_master_set_bus_width(&qspi->handle, rest_buswidth);
        t.rx_data = NULL;
        t.tx_data = cmdbuf2;
        t.data_len = rest_cnt;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret)
            goto out;
    }

    /* Read data phase */

    hal_qspi_master_set_bus_width(&qspi->handle, rd_fmt->data_lines);
    t.rx_data = read_buf;
    t.tx_data = NULL;
    t.data_len = read_size;
#if defined(AIC_SPIENC_DRV)
    spienc_set_cfg(qspi->idx, addr, 0, read_size);
    spienc_start();
#endif
    ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
#if defined(AIC_SPIENC_DRV)
    spienc_stop();
    if (spienc_check_empty())
        memset(read_buf, 0xFF, read_size);
#endif
out:
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    return ret;
}
#endif

static sfud_err spi_set_speed(const struct __sfud_spi *spi, uint32_t bus_hz)
{
    struct aic_qspi_bus *qspi;

    qspi = (struct aic_qspi_bus *)spi->user_data;

    if (qspi == NULL)
        return SFUD_ERR_NOT_FOUND;
    hal_qspi_master_set_bus_freq(&qspi->handle, bus_hz);

    return SFUD_SUCCESS;
}

static sfud_err spi_get_bus_id(const struct __sfud_spi *spi, uint32_t *bus_id)
{
    struct aic_qspi_bus *qspi;

    qspi = (struct aic_qspi_bus *)spi->user_data;

    if (qspi == NULL)
        return SFUD_ERR_NOT_FOUND;

    *bus_id = qspi->idx;

    return SFUD_SUCCESS;
}

static void spi_set_rx_delay(const struct __sfud_spi *spi, uint32_t mode)
{
    struct aic_qspi_bus *qspi;

    qspi = (struct aic_qspi_bus *)spi->user_data;

    if (qspi == NULL)
        return;
    hal_qspi_master_set_rxdelay_mode(&qspi->handle, mode);
}

static sfud_err spi_write_read(const struct __sfud_spi *spi,
                               const uint8_t *write_buf, size_t write_size,
                               uint8_t *read_buf, size_t read_size)
{
    struct aic_qspi_bus *qspi;
    struct qspi_transfer t;
    int ret = 0;
    u32 cs_num = 0;

    qspi = (struct aic_qspi_bus *)spi->user_data;

    hal_qspi_master_set_bus_width(&qspi->handle, HAL_QSPI_BUS_WIDTH_SINGLE);
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, true);
    if (write_size) {
        t.rx_data = NULL;
        t.tx_data = (uint8_t *)write_buf;
        t.data_len = write_size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
        if (ret < 0)
            goto out;
    }
    if (read_size) {
        t.rx_data = read_buf;
        t.tx_data = NULL;
        t.data_len = read_size;
        ret = hal_qspi_master_transfer_sync(&qspi->handle, &t);
    }
out:
#if defined(AIC_QSPI_MULTIPLE_CS_NUM)
    cs_num = qspi->cs_num;
#endif
    hal_qspi_master_set_cs(&qspi->handle, cs_num, false);
    return ret;
}

sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    /* port SPI device interface */
    flash->spi.wr = spi_write_read;
    flash->spi.set_speed = spi_set_speed;
    flash->spi.get_bus_id = spi_get_bus_id;
    flash->spi.set_rx_delay = spi_set_rx_delay;
#ifdef SFUD_USING_QSPI
    flash->spi.qspi_read = (void *)qspi_read;
#endif
    flash->spi.lock = NULL;
    flash->spi.unlock = NULL;
    flash->spi.user_data = flash->user_data;
    /* 100 microsecond delay */
    flash->retry.delay = retry_delay_100us;
    /* 60 seconds timeout */
    flash->retry.times = 60 * 10000;

    return result;
}

static int sfud_mtd_read(struct mtd_dev *mtd, u32 offset, uint8_t *data,
                         u32 len)
{
    sfud_flash *flash;
    u32 start, dolen;

    if ((!mtd) || (!mtd->priv))
        return -1;
    start = mtd->start + offset;
    dolen = len;
    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;
    flash = mtd->priv;
    return sfud_read(flash, start, dolen, data);
}

static int sfud_mtd_erase(struct mtd_dev *mtd, u32 offset, u32 len)
{
    sfud_flash *flash;
    u32 start, dolen;

    if ((!mtd) || (!mtd->priv))
        return -1;
    start = mtd->start + offset;
    dolen = len;
    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;
    flash = mtd->priv;
    return sfud_erase(flash, start, dolen);
}

static int sfud_mtd_write(struct mtd_dev *mtd, u32 offset, uint8_t *data,
                          u32 len)
{
    sfud_flash *flash;
    u32 start, dolen;

    if ((!mtd) || (!mtd->priv))
        return -1;
    start = mtd->start + offset;
    dolen = len;
    if ((mtd->size - offset) < dolen)
        dolen = mtd->size - offset;
    flash = mtd->priv;
    return sfud_write(flash, start, dolen, data);
}

sfud_flash *sfud_probe(u32 spi_bus)
{
    sfud_err result = SFUD_SUCCESS;
    struct aic_qspi_bus *qspi;
    struct mtd_dev *mtd;
    struct mtd_partition *part, *p;
    int ret;
    struct qspi_master_config cfg = {0};
    char *partstr;

    qspi = get_qspi_by_index(spi_bus);
    if (!qspi) {
        pr_err("spi bus is invalid: %d\n", spi_bus);
        return NULL;
    }

    if ((qspi->probe_flag) && (qspi->attached_flash.init_ok))
        return &qspi->attached_flash;

    memset(&cfg, 0, sizeof(cfg));
    cfg.idx = qspi->idx;
    cfg.clk_in_hz = qspi->clk_in_hz;
    cfg.clk_id = qspi->clk_id;
    cfg.cpol = HAL_QSPI_CPOL_ACTIVE_HIGH;
    cfg.cpha = HAL_QSPI_CPHA_FIRST_EDGE;
    cfg.cs_polarity = HAL_QSPI_CS_POL_VALID_LOW;
    cfg.rx_dlymode = qspi->rxd_dylmode;
    cfg.tx_dlymode = aic_convert_tx_dlymode(qspi->txc_dylmode, qspi->txd_dylmode);

    ret = hal_qspi_master_init(&qspi->handle, &cfg);
    if (ret) {
        pr_err("hal_qspi_master_init failed. ret %d\n", ret);
        return NULL;
    }

#ifdef AIC_DMA_DRV
    struct qspi_master_dma_config dmacfg;
    memset(&dmacfg, 0, sizeof(dmacfg));
    dmacfg.port_id = qspi->dma_port_id;

    ret = hal_qspi_master_dma_config(&qspi->handle, &dmacfg);
    if (ret) {
        pr_err("qspi dma config failed.\n");
        return NULL;
    }

    qspi->probe_flag = true;
#endif
    qspi->attached_flash.user_data = (void *)qspi;
    qspi->attached_flash.init_hz = SFUD_READ_SFDP_FREQ;
    qspi->attached_flash.bus_hz = qspi->bus_hz;

    result = sfud_device_init(&qspi->attached_flash);
    if (result != SFUD_SUCCESS) {
        pr_err("sfud_device_init failed: ret %d\n", result);
        return NULL;
    }

#ifdef SFUD_USING_QSPI
    sfud_qspi_fast_read_enable(&qspi->attached_flash, qspi->dl_width);
#endif

    mtd = malloc(sizeof(*mtd));
    memset(mtd, 0, sizeof(*mtd));
    mtd->name = strdup("nor0");
    mtd->name[3] += spi_bus;
    mtd->start = 0;
    mtd->size = qspi->attached_flash.chip.capacity;
    mtd->erasesize = qspi->attached_flash.chip.eraser[0].size;
    mtd->ops.erase = sfud_mtd_erase;
    mtd->ops.read = sfud_mtd_read;
    mtd->ops.write = sfud_mtd_write;
    mtd->priv = &qspi->attached_flash;
    mtd_add_device(mtd);

    partstr = aic_spinor_get_partition_string(mtd);
    part = mtd_parts_parse(partstr, spi_bus);
    free(partstr);
    p = part;
    while (p) {
#ifdef IMAGE_CFG_JSON_PARTS_LEVELX
        if (partition_levelx_is_exist(p->name, IMAGE_CFG_JSON_PARTS_LEVELX))
            p->attr = PART_ATTR_LEVELX;
        else
#endif
            p->attr = PART_ATTR_MTD;

        mtd = malloc(sizeof(*mtd));
        memset(mtd, 0, sizeof(*mtd));
        mtd->name = strdup(p->name);
        mtd->start = p->start;
        mtd->size = p->size;
        mtd->erasesize = qspi->attached_flash.chip.eraser[0].cmd;
        if (p->size == 0)
            mtd->size = qspi->attached_flash.chip.capacity - p->start;
        mtd->ops.erase = sfud_mtd_erase;
        mtd->ops.read = sfud_mtd_read;
        mtd->ops.write = sfud_mtd_write;
        mtd->priv = &qspi->attached_flash;
        mtd->attr = p->attr;
        mtd_add_device(mtd);
        p = p->next;
    }

    if (part)
        mtd_parts_free(part);

    qspi->probe_flag = true;
    return &qspi->attached_flash;
}
