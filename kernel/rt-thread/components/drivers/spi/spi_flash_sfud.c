/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-09-28     armink       first version.
 */

#include <stdint.h>
#include <string.h>
#include <rtdevice.h>
#include <spienc.h>
#include "spi_flash.h"
#include "spi_flash_sfud.h"
#include "aic_time.h"
#include <aic_core.h>

#ifdef RT_USING_SFUD

#ifndef RT_SFUD_DEFAULT_SPI_CFG

#ifndef RT_SFUD_SPI_MAX_HZ
#define RT_SFUD_SPI_MAX_HZ 50000000
#endif

#define RT_SPI_MAX_HZ      133000000

/* read the JEDEC SFDP command must run at 50 MHz or less */
#define RT_SFUD_DEFAULT_SPI_CFG                  \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = RT_SPI_MAX_HZ,                     \
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

static rt_err_t rt_sfud_control(rt_device_t dev, int cmd, void *args) {
    RT_ASSERT(dev);

    switch (cmd) {
    case RT_DEVICE_CTRL_BLK_GETGEOME: {
        struct rt_device_blk_geometry *geometry = (struct rt_device_blk_geometry *) args;
        struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (dev->user_data);

        if (rtt_dev == RT_NULL || geometry == RT_NULL) {
            return -RT_ERROR;
        }

        geometry->bytes_per_sector = rtt_dev->geometry.bytes_per_sector;
        geometry->sector_count = rtt_dev->geometry.sector_count;
        geometry->block_size = rtt_dev->geometry.block_size;
        break;
    }
    case RT_DEVICE_CTRL_BLK_ERASE: {
        rt_uint32_t *addrs = (rt_uint32_t *) args, start_addr = addrs[0], end_addr = addrs[1], phy_start_addr;
        struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (dev->user_data);
        sfud_flash *sfud_dev = (sfud_flash *) (rtt_dev->user_data);
        rt_size_t phy_size;

        if (addrs == RT_NULL || start_addr > end_addr || rtt_dev == RT_NULL || sfud_dev == RT_NULL) {
            return -RT_ERROR;
        }

        if (end_addr == start_addr) {
            end_addr ++;
        }

        phy_start_addr = start_addr * rtt_dev->geometry.bytes_per_sector;
        phy_size = (end_addr - start_addr) * rtt_dev->geometry.bytes_per_sector;

        if (sfud_erase(sfud_dev, phy_start_addr, phy_size) != SFUD_SUCCESS) {
            return -RT_ERROR;
        }
        break;
    }
    }

    return RT_EOK;
}


static rt_size_t rt_sfud_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size) {
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (dev->user_data);
    sfud_flash *sfud_dev = (sfud_flash *) (rtt_dev->user_data);

    RT_ASSERT(dev);
    RT_ASSERT(rtt_dev);
    RT_ASSERT(sfud_dev);
    /* change the block device's logic address to physical address */
    rt_off_t phy_pos = pos * rtt_dev->geometry.bytes_per_sector;
    rt_size_t phy_size = size * rtt_dev->geometry.bytes_per_sector;

    if (sfud_read(sfud_dev, phy_pos, phy_size, buffer) != SFUD_SUCCESS) {
        return 0;
    } else {
        return size;
    }
}

static rt_size_t rt_sfud_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size) {
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (dev->user_data);
    sfud_flash *sfud_dev = (sfud_flash *) (rtt_dev->user_data);

    RT_ASSERT(dev);
    RT_ASSERT(rtt_dev);
    RT_ASSERT(sfud_dev);
    /* change the block device's logic address to physical address */
    rt_off_t phy_pos = pos * rtt_dev->geometry.bytes_per_sector;
    rt_size_t phy_size = size * rtt_dev->geometry.bytes_per_sector;

    if (sfud_erase_write(sfud_dev, phy_pos, phy_size, buffer) != SFUD_SUCCESS) {
        return 0;
    } else {
        return size;
    }
}

static sfud_err spi_set_speed(const sfud_spi *spi, uint32_t bus_hz)
{
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    rt_spi_flash_device_t rtt_dev = RT_NULL;

    rtt_dev = sfud_dev->user_data;
#ifndef SFUD_USING_QSPI
    struct rt_spi_configuration spi_cfg = RT_SFUD_DEFAULT_SPI_CFG;

    spi_cfg.max_hz = bus_hz;
    return rt_spi_configure(rtt_dev->rt_spi_device, &spi_cfg);
#else
    struct rt_qspi_configuration qspi_cfg = RT_SFUD_DEFAULT_QSPI_CFG;
    struct rt_qspi_device *qspi_dev = RT_NULL;

    qspi_dev = (struct rt_qspi_device *)(rtt_dev->rt_spi_device);
    qspi_cfg.parent.max_hz = bus_hz;
    return rt_qspi_configure(qspi_dev, &qspi_cfg);
#endif
}

static sfud_err spi_get_bus_id(const sfud_spi *spi, uint32_t *bus_id)
{
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);

    *bus_id = sfud_dev->index;

    return SFUD_SUCCESS;
}

static void spi_set_rx_delay(const sfud_spi *spi, uint32_t mode)
{
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    rt_spi_flash_device_t rtt_dev = RT_NULL;

    rtt_dev = sfud_dev->user_data;

    rt_spi_set_rx_delay_mode(rtt_dev->rt_spi_device, mode);
}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
        size_t read_size) {
    sfud_err result = SFUD_SUCCESS;
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (sfud_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(sfud_dev);
    RT_ASSERT(rtt_dev);
#ifdef SFUD_USING_QSPI
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif
    if (write_size) {
        RT_ASSERT(write_buf);
    }
    if (read_size) {
        RT_ASSERT(read_buf);
    }
#ifdef SFUD_USING_QSPI
    if(rtt_dev->rt_spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI) {
        qspi_dev = (struct rt_qspi_device *) (rtt_dev->rt_spi_device);
        if (write_size && read_size) {
            if (rt_qspi_send_then_recv(qspi_dev, write_buf, write_size, read_buf, read_size) <= 0) {
                result = SFUD_ERR_TIMEOUT;
            }
        } else if (write_size) {
            if (rt_qspi_send(qspi_dev, write_buf, write_size) <= 0) {
                result = SFUD_ERR_TIMEOUT;
            }
        }
    }
    else
#endif
    {
        if (write_size && read_size) {
            if (rt_spi_send_then_recv(rtt_dev->rt_spi_device, write_buf, write_size, read_buf, read_size) != RT_EOK) {
                result = SFUD_ERR_TIMEOUT;
            }
        } else if (write_size) {
            if (rt_spi_send(rtt_dev->rt_spi_device, write_buf, write_size) <= 0) {
                result = SFUD_ERR_TIMEOUT;
            }
        } else {
            if (rt_spi_recv(rtt_dev->rt_spi_device, read_buf, read_size) <= 0) {
                result = SFUD_ERR_TIMEOUT;
            }
        }
    }

    return result;
}

#ifdef SFUD_USING_QSPI
/**
 * QSPI fast read data
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format, uint8_t *read_buf, size_t read_size) {
    struct rt_qspi_message message;
    sfud_err result = SFUD_SUCCESS;

    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (sfud_dev->user_data);
    struct rt_qspi_device *qspi_dev = (struct rt_qspi_device *) (rtt_dev->rt_spi_device);
    uint32_t single_cnt, rest_cnt, dmycnt, dmybw;

    RT_ASSERT(spi);
    RT_ASSERT(sfud_dev);
    RT_ASSERT(rtt_dev);
    RT_ASSERT(qspi_dev);

    /* set message struct */
    message.instruction.content = qspi_read_cmd_format->instruction;
    message.instruction.qspi_lines = qspi_read_cmd_format->instruction_lines;

    message.address.content = addr;
    message.address.size = qspi_read_cmd_format->address_size / 8;
    message.address.qspi_lines = qspi_read_cmd_format->address_lines;

    message.alternate_bytes.content = 0;
    message.alternate_bytes.size = 0;
    message.alternate_bytes.qspi_lines = 0;

    message.dummy_cycles = qspi_read_cmd_format->dummy_cycles;

    message.parent.send_buf = RT_NULL;
    message.parent.recv_buf = read_buf;
    message.parent.length = read_size;
    message.parent.cs_release = 1;
    message.parent.cs_take = 1;
    message.qspi_data_lines = qspi_read_cmd_format->data_lines;

    single_cnt = 0;
    rest_cnt = 0;
    dmybw = 1;
    if (message.instruction.qspi_lines == 1) {
        single_cnt++;
    } else if (message.instruction.qspi_lines > 1) {
        rest_cnt++;
        dmybw = message.instruction.qspi_lines;
    }

    if ((message.address.size != 0) &&
        (message.address.qspi_lines == 1)) {
        single_cnt += message.address.size;
        dmybw = message.address.qspi_lines;
    } else if ((message.address.size != 0) &&
               (message.address.qspi_lines > 1)) {
        rest_cnt += message.address.size;
        dmybw = message.address.qspi_lines;
    }

    dmycnt = (dmybw * message.dummy_cycles) / 8;
    if ((dmycnt != 0) && (dmybw == 1)) {
        single_cnt += dmycnt;
    } else if (dmycnt != 0) {
        rest_cnt += dmycnt;
    }

#if defined(AIC_SPIENC_DRV)
    spienc_set_cfg(sfud_dev->index, addr, single_cnt + rest_cnt, read_size);
    spienc_start();
#endif
    if (rt_qspi_transfer_message(qspi_dev, &message) != read_size) {
        result = SFUD_ERR_TIMEOUT;
    }
#if defined(AIC_SPIENC_DRV)
    spienc_stop();
    if (spienc_check_empty())
        memset(read_buf, 0xFF, read_size);
#endif

    return result;
}
#endif

static void spi_lock(const sfud_spi *spi) {
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (sfud_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(sfud_dev);
    RT_ASSERT(rtt_dev);

    rt_mutex_take(&(rtt_dev->lock), RT_WAITING_FOREVER);
}

static void spi_unlock(const sfud_spi *spi) {
    sfud_flash *sfud_dev = (sfud_flash *) (spi->user_data);
    struct spi_flash_device *rtt_dev = (struct spi_flash_device *) (sfud_dev->user_data);

    RT_ASSERT(spi);
    RT_ASSERT(sfud_dev);
    RT_ASSERT(rtt_dev);

    rt_mutex_release(&(rtt_dev->lock));
}

static void retry_delay_100us(void) {
    /* 100 microsecond delay */
    aicos_udelay(100);
}

sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    RT_ASSERT(flash);

    /* port SPI device interface */
    flash->spi.wr = spi_write_read;
    flash->spi.set_speed = spi_set_speed;
    flash->spi.get_bus_id = spi_get_bus_id;
    flash->spi.set_rx_delay = spi_set_rx_delay;
#ifdef SFUD_USING_QSPI
    flash->spi.qspi_read = qspi_read;
#endif
    flash->spi.lock = spi_lock;
    flash->spi.unlock = spi_unlock;
    flash->spi.user_data = flash;

    /* 100 microsecond delay */
    flash->retry.delay = retry_delay_100us;
    /* 60 seconds timeout */
    flash->retry.times = 60 * 10000;

    return result;
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops flash_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_sfud_read,
    rt_sfud_write,
    rt_sfud_control
};
#endif

/**
 * Probe SPI flash by SFUD (Serial Flash Universal Driver) driver library and though SPI device by specified configuration.
 *
 * @param spi_flash_dev_name the name which will create SPI flash device
 * @param spi_dev_name using SPI device name
 * @param spi_cfg SPI device configuration
 * @param qspi_cfg QSPI device configuration
 *
 * @return probed SPI flash device, probe failed will return RT_NULL
 */
rt_spi_flash_device_t rt_sfud_flash_probe_ex(const char *spi_flash_dev_name, const char *spi_dev_name,
        struct rt_spi_configuration *spi_cfg, struct rt_qspi_configuration *qspi_cfg)
{
    rt_spi_flash_device_t rtt_dev = RT_NULL;
    sfud_flash *sfud_dev = RT_NULL;
    char *spi_flash_dev_name_bak = RT_NULL, *spi_dev_name_bak = RT_NULL;
    extern sfud_err sfud_device_init(sfud_flash *flash);
#ifdef SFUD_USING_QSPI
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif

    RT_ASSERT(spi_flash_dev_name);
    RT_ASSERT(spi_dev_name);

    rtt_dev = (rt_spi_flash_device_t) rt_malloc(sizeof(struct spi_flash_device));
    sfud_dev = (sfud_flash_t) rt_malloc(sizeof(sfud_flash));
    spi_flash_dev_name_bak = (char *) rt_malloc(rt_strlen(spi_flash_dev_name) + 1);
    spi_dev_name_bak = (char *) rt_malloc(rt_strlen(spi_dev_name) + 1);

    if (!sfud_dev)
        goto error;

    rt_memset(sfud_dev, 0, sizeof(sfud_flash));
    sfud_dev->init_hz = RT_SFUD_SPI_MAX_HZ;

    if (rtt_dev) {
        rt_memset(rtt_dev, 0, sizeof(struct spi_flash_device));
        /* initialize lock */
        rt_mutex_init(&(rtt_dev->lock), spi_flash_dev_name, RT_IPC_FLAG_PRIO);
    }

    if (rtt_dev && sfud_dev && spi_flash_dev_name_bak && spi_dev_name_bak) {
        rt_strncpy(spi_flash_dev_name_bak, spi_flash_dev_name, rt_strlen(spi_flash_dev_name));
        rt_strncpy(spi_dev_name_bak, spi_dev_name, rt_strlen(spi_dev_name));
        /* make string end sign */
        spi_flash_dev_name_bak[rt_strlen(spi_flash_dev_name)] = '\0';
        spi_dev_name_bak[rt_strlen(spi_dev_name)] = '\0';
        /* SPI configure */
        {
            /* RT-Thread SPI device initialize */
            rtt_dev->rt_spi_device = (struct rt_spi_device *) rt_device_find(spi_dev_name);
            if (rtt_dev->rt_spi_device == RT_NULL || rtt_dev->rt_spi_device->parent.type != RT_Device_Class_SPIDevice) {
                LOG_E("ERROR: SPI device %s not found!", spi_dev_name);
                goto error;
            }
            sfud_dev->spi.name = spi_dev_name_bak;

#ifdef SFUD_USING_SFDP
            /* Read SFDP has 50MHz limitation by SPEC */
#ifdef SFUD_USING_QSPI
            sfud_dev->bus_hz = qspi_cfg->parent.max_hz;
            qspi_cfg->parent.max_hz = sfud_dev->init_hz;
#else
            sfud_dev->bus_hz = spi_cfg->max_hz;
            spi_cfg->max_hz = sfud_dev->init_hz;
#endif
#endif

#ifdef SFUD_USING_QSPI
            /* set the qspi line number and configure the QSPI bus */
            if(rtt_dev->rt_spi_device->bus->mode &RT_SPI_BUS_MODE_QSPI) {
                qspi_dev = (struct rt_qspi_device *)rtt_dev->rt_spi_device;
                qspi_cfg->qspi_dl_width = qspi_dev->config.qspi_dl_width;
                rt_qspi_configure(qspi_dev, qspi_cfg);
            }
            else
#endif
                rt_spi_configure(rtt_dev->rt_spi_device, spi_cfg);
#ifdef SFUD_USING_SFDP
            /* Read SFDP has 50MHz limitation by SPEC */
#ifdef SFUD_USING_QSPI
            qspi_cfg->parent.max_hz = sfud_dev->bus_hz;
#else
            spi_cfg->max_hz = sfud_dev->bus_hz;
#endif
#endif
        }
        /* SFUD flash device initialize */
        {
            sfud_dev->name = spi_flash_dev_name_bak;
            /* accessed each other */
            rtt_dev->user_data = sfud_dev;
            rtt_dev->rt_spi_device->user_data = rtt_dev;
            rtt_dev->flash_device.user_data = rtt_dev;
            sfud_dev->user_data = rtt_dev;
            /* initialize SFUD device */
            if (sfud_device_init(sfud_dev) != SFUD_SUCCESS) {
                LOG_E("ERROR: SPI flash probe failed by SPI device %s.", spi_dev_name);
                goto error;
            }
            /* when initialize success, then copy SFUD flash device's geometry to RT-Thread SPI flash device */
            rtt_dev->geometry.sector_count = sfud_dev->chip.capacity / sfud_dev->chip.eraser[0].size;
            rtt_dev->geometry.bytes_per_sector = sfud_dev->chip.eraser[0].size;
            rtt_dev->geometry.block_size = sfud_dev->chip.eraser[0].size;
#ifdef SFUD_USING_QSPI
            /* reconfigure the QSPI bus for medium size */
            if(rtt_dev->rt_spi_device->bus->mode &RT_SPI_BUS_MODE_QSPI) {
                qspi_cfg->medium_size = sfud_dev->chip.capacity;
                rt_qspi_configure(qspi_dev, qspi_cfg);
                if(qspi_dev->enter_qspi_mode != RT_NULL)
                    qspi_dev->enter_qspi_mode(qspi_dev);

                /* set data lines width */
                sfud_qspi_fast_read_enable(sfud_dev, qspi_dev->config.qspi_dl_width);
            }
#endif /* SFUD_USING_QSPI */
        }

        /* register device */
        rtt_dev->flash_device.type = RT_Device_Class_Block;
#ifdef RT_USING_DEVICE_OPS
        rtt_dev->flash_device.ops  = &flash_device_ops;
#else
        rtt_dev->flash_device.init = RT_NULL;
        rtt_dev->flash_device.open = RT_NULL;
        rtt_dev->flash_device.close = RT_NULL;
        rtt_dev->flash_device.read = rt_sfud_read;
        rtt_dev->flash_device.write = rt_sfud_write;
        rtt_dev->flash_device.control = rt_sfud_control;
#endif

        rt_device_register(&(rtt_dev->flash_device), spi_flash_dev_name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);

        LOG_I("Probe SPI flash %s by SPI device %s success.",spi_flash_dev_name, spi_dev_name);
        return rtt_dev;
    } else {
        LOG_E("ERROR: Low memory.");
        goto error;
    }

error:

    if (rtt_dev) {
        rt_mutex_detach(&(rtt_dev->lock));
    }
    /* may be one of objects memory was malloc success, so need free all */
    rt_free(rtt_dev);
    rt_free(sfud_dev);
    rt_free(spi_flash_dev_name_bak);
    rt_free(spi_dev_name_bak);

    return RT_NULL;
}

/**
 * Probe SPI flash by SFUD(Serial Flash Universal Driver) driver library and though SPI device.
 *
 * @param spi_flash_dev_name the name which will create SPI flash device
 * @param spi_dev_name using SPI device name
 *
 * @return probed SPI flash device, probe failed will return RT_NULL
 */
rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name)
{
    struct rt_spi_configuration cfg = RT_SFUD_DEFAULT_SPI_CFG;
#ifndef SFUD_USING_QSPI
    return rt_sfud_flash_probe_ex(spi_flash_dev_name, spi_dev_name, &cfg, RT_NULL);
#else
    struct rt_qspi_configuration qspi_cfg = RT_SFUD_DEFAULT_QSPI_CFG;

    return rt_sfud_flash_probe_ex(spi_flash_dev_name, spi_dev_name, &cfg, &qspi_cfg);
#endif
}

/**
 * Delete SPI flash device
 *
 * @param spi_flash_dev SPI flash device
 *
 * @return the operation status, RT_EOK on successful
 */
rt_err_t rt_sfud_flash_delete(rt_spi_flash_device_t spi_flash_dev) {
    sfud_flash *sfud_flash_dev = (sfud_flash *) (spi_flash_dev->user_data);

    RT_ASSERT(spi_flash_dev);
    RT_ASSERT(sfud_flash_dev);

    rt_device_unregister(&(spi_flash_dev->flash_device));

    rt_mutex_detach(&(spi_flash_dev->lock));

    rt_free(sfud_flash_dev->spi.name);
    rt_free(sfud_flash_dev->name);
    rt_free(sfud_flash_dev);
    rt_free(spi_flash_dev);

    return RT_EOK;
}

sfud_flash_t rt_sfud_flash_find(const char *spi_dev_name)
{
    rt_spi_flash_device_t  rtt_dev       = RT_NULL;
    struct rt_spi_device  *rt_spi_device = RT_NULL;
    sfud_flash_t           sfud_dev      = RT_NULL;

    rt_spi_device = (struct rt_spi_device *) rt_device_find(spi_dev_name);
    if (rt_spi_device == RT_NULL || rt_spi_device->parent.type != RT_Device_Class_SPIDevice) {
        LOG_E("ERROR: SPI device %s not found!", spi_dev_name);
        goto __error;
    }

    rtt_dev = (rt_spi_flash_device_t) (rt_spi_device->user_data);
    if (rtt_dev && rtt_dev->user_data) {
        sfud_dev = (sfud_flash_t) (rtt_dev->user_data);
        return sfud_dev;
    } else {
        LOG_E("ERROR: SFUD flash device not found!");
        goto __error;
    }

__error:
    return RT_NULL;
}

sfud_flash_t rt_sfud_flash_find_by_dev_name(const char *flash_dev_name)
{
    rt_spi_flash_device_t  rtt_dev       = RT_NULL;
    sfud_flash_t           sfud_dev      = RT_NULL;

    rtt_dev = (rt_spi_flash_device_t) rt_device_find(flash_dev_name);
    if (rtt_dev == RT_NULL || rtt_dev->flash_device.type != RT_Device_Class_Block) {
        LOG_E("ERROR: Flash device %s not found!", flash_dev_name);
        goto __error;
    }

    if (rtt_dev->user_data) {
        sfud_dev = (sfud_flash_t) (rtt_dev->user_data);
        return sfud_dev;
    } else {
        LOG_E("ERROR: SFUD flash device not found!");
        goto __error;
    }

__error:
    return RT_NULL;
}

#endif /* RT_USING_SFUD */
