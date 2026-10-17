/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtconfig.h>
#include <rtdevice.h>
#include <drv_qspi.h>
#include <aic_log.h>
#include "spi_flash_sfud.h"
#include <string.h>

#if defined(RT_USING_SFUD)

#ifndef RT_SFUD_DEFAULT_SPI_CFG

#ifndef RT_SFUD_SPI_MAX_HZ
#define RT_SFUD_SPI_MAX_HZ 50000000
#endif

/* read the JEDEC SFDP command must run at 50 MHz or less */
#define RT_SFUD_DEFAULT_SPI_CFG                  \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = RT_SFUD_SPI_MAX_HZ,                \
}
#endif /* RT_SFUD_DEFAULT_SPI_CFG */

#ifdef SFUD_USING_QSPI
#define RT_SFUD_DEFAULT_QSPI_CFG                 \
{                                                \
    RT_SFUD_DEFAULT_SPI_CFG,                     \
    .medium_size = 0x800000,                     \
    .ddr_mode = 0,                               \
    .qspi_dl_width = 4,                          \
}
#endif /* SFUD_USING_QSPI */

/**
 * Flash device configuration structure for SFUD initialization
 */
struct sfud_flash_config {
    const char *qspi_bus_name;       /* QSPI bus name (e.g., "qspi0") */
    const char *spi_dev_name;        /* SPI device name (e.g., "qspi01") */
    const char *flash_dev_name;      /* SFUD flash device name (e.g., "norflash0") */
    rt_uint8_t chip_select;          /* Chip select number (0, 1, etc.) */
    rt_uint8_t bus_width;            /* QSPI data line width */
    rt_uint32_t freq_hz;             /* Maximum SPI/QSPI frequency */
    rt_bool_t enabled;               /* Whether this device is enabled */
};

/**
 * Flash device configuration table
 * Each entry represents one SPI NOR flash device on a QSPI bus
 */
const struct sfud_flash_config flash_config_table[] = {
#if defined(AIC_USING_QSPI0) && defined(AIC_QSPI0_DEVICE_SPINOR)
    {
        .qspi_bus_name = "qspi0",
        .spi_dev_name = "qspi01",
        .flash_dev_name = "norflash0",
        .chip_select = 0,
        .bus_width = AIC_QSPI0_BUS_WIDTH,
        .freq_hz = AIC_QSPI0_DEVICE_SPINOR_FREQ,
        .enabled = true,
    },
#endif
#if defined(AIC_USING_QSPI1) && defined(AIC_QSPI1_DEVICE_SPINOR)
    {
        .qspi_bus_name = "qspi1",
        .spi_dev_name = "qspi11",
        .flash_dev_name = "norflash1",
        .chip_select = 0,
        .bus_width = AIC_QSPI1_BUS_WIDTH,
        .freq_hz = AIC_QSPI1_DEVICE_SPINOR_FREQ,
        .enabled = true,
    },
#endif
#if defined(AIC_USING_QSPI2) && defined(AIC_QSPI2_DEVICE_SPINOR)
    {
        .qspi_bus_name = "qspi2",
        .spi_dev_name = "qspi21",
        .flash_dev_name = "norflash2",
        .chip_select = 0,
        .bus_width = AIC_QSPI2_BUS_WIDTH,
        .freq_hz = AIC_QSPI2_DEVICE_SPINOR_FREQ,
        .enabled = true,
    },
#endif
#if defined(AIC_USING_QSPI3) && defined(AIC_QSPI3_DEVICE_SPINOR)
    {
        .qspi_bus_name = "qspi3",
        .spi_dev_name = "qspi31",
        .flash_dev_name = "norflash3",
        .chip_select = 0,
        .bus_width = AIC_QSPI3_BUS_WIDTH,
        .freq_hz = AIC_QSPI3_DEVICE_SPINOR_FREQ,
        .enabled = true,
    },
#endif
#if defined(AIC_USING_QSPI4) && defined(AIC_QSPI4_DEVICE_SPINOR)
    {
        .qspi_bus_name = "qspi4",
        .spi_dev_name = "qspi41",
        .flash_dev_name = "norflash4",
        .chip_select = 0,
        .bus_width = AIC_QSPI4_BUS_WIDTH,
        .freq_hz = AIC_QSPI4_DEVICE_SPINOR_FREQ,
        .enabled = true,
    },
#endif
};

#define FLASH_CONFIG_COUNT ARRAY_SIZE(flash_config_table)

int rt_hw_spi_flash_with_sfud_init(void)
{
    rt_spi_flash_device_t flash_dev;
    int i;

    if (FLASH_CONFIG_COUNT == 0) {
        pr_err("No flash device configured.\n");
        return RT_ERROR;
    }

    /* Attach and probe each flash device sequentially */
    for (i = 0; i < FLASH_CONFIG_COUNT; i++) {
        const struct sfud_flash_config *cfg = &flash_config_table[i];
        rt_err_t ret;

        if (!cfg->enabled)
            continue;

        /* Step 1: Create SPI device (RT_Device_Class_SPIDevice) and Attach to QSPI bus device
         *         (e.g., "qspi01" -> "qspi0")
         */
        ret = aic_qspi_bus_attach_device(cfg->qspi_bus_name,
                                         cfg->spi_dev_name,
                                         cfg->chip_select,
                                         cfg->bus_width,
                                         RT_NULL, RT_NULL);
        if (ret < 0) {
            pr_err("Attach %s failed.\n", cfg->spi_dev_name);
            return RT_ERROR;
        }

        /* Step 2: Probe SFUD flash device and create device(RT_Device_Class_Block) */
#ifndef SFUD_USING_QSPI
        struct rt_spi_configuration spi_cfg = RT_SFUD_DEFAULT_SPI_CFG;

        spi_cfg.max_hz = cfg->freq_hz;
        flash_dev = rt_sfud_flash_probe_ex(cfg->flash_dev_name,
                                           cfg->spi_dev_name,
                                           &spi_cfg, RT_NULL);
#else
        struct rt_qspi_configuration qspi_cfg = RT_SFUD_DEFAULT_QSPI_CFG;

        qspi_cfg.parent.max_hz = cfg->freq_hz;
        flash_dev = rt_sfud_flash_probe_ex(cfg->flash_dev_name,
                                           cfg->spi_dev_name,
                                           &qspi_cfg.parent,
                                           &qspi_cfg);
#endif
        if (flash_dev == RT_NULL) {
            pr_err("SFUD probe %s failed.\n", cfg->flash_dev_name);
            return RT_ERROR;
        }
    }

    return RT_EOK;
}

rt_uint32_t spinor_sfud_port_get_flash_count(void)
{
    return FLASH_CONFIG_COUNT;
}

const char *spinor_sfud_port_get_flash_name(rt_uint32_t idx)
{
    if (idx < FLASH_CONFIG_COUNT) {
        return flash_config_table[idx].flash_dev_name;
    }

    return NULL;
}

void sfud_log_debug(const char *file, const long line, const char *fmt, ...)
{
    va_list args;
    char log_buf[RT_CONSOLEBUF_SIZE];
    int head_len;

    snprintf(log_buf, RT_CONSOLEBUF_SIZE, "[D] %s()%ld ", file, line);
    head_len = strlen(log_buf);
    va_start(args, fmt);
    vsnprintf(log_buf + head_len, sizeof(log_buf) - head_len - 1, fmt, args);
    va_end(args);
    puts(log_buf);
}

void sfud_log_info(const char *fmt, ...)
{
    va_list args;
    char log_buf[RT_CONSOLEBUF_SIZE];
    int head_len;

    snprintf(log_buf, RT_CONSOLEBUF_SIZE, "[I] ");
    head_len = strlen(log_buf);
    va_start(args, fmt);
    vsnprintf(log_buf + head_len, sizeof(log_buf) - head_len - 1, fmt, args);
    va_end(args);
    puts(log_buf);
}

INIT_PREV_EXPORT(rt_hw_spi_flash_with_sfud_init);
#endif
