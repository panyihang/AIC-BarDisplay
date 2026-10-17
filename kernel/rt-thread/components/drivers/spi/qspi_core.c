/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-16     zylx      first version.
 */

#include <drivers/spi.h>

rt_err_t rt_qspi_configure(struct rt_qspi_device *device, struct rt_qspi_configuration *cfg)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    struct rt_qspi_device *qspi_device = (struct rt_qspi_device *)device;
    rt_err_t result = RT_EOK;

    /* copy configuration items */
    qspi_device->config.parent.mode = cfg->parent.mode;
    qspi_device->config.parent.max_hz = cfg->parent.max_hz;
    qspi_device->config.parent.data_width = cfg->parent.data_width;
    qspi_device->config.parent.reserved = cfg->parent.reserved;
    qspi_device->config.medium_size = cfg->medium_size;
    qspi_device->config.ddr_mode = cfg->ddr_mode;
    qspi_device->config.qspi_dl_width = cfg->qspi_dl_width;

    result = rt_spi_configure(&device->parent, &cfg->parent);

    return result;
}

rt_err_t rt_qspi_bus_register(struct rt_spi_bus *bus, const char *name, const struct rt_spi_ops *ops)
{
    rt_err_t result = RT_EOK;

    result = rt_spi_bus_register(bus, name, ops);
    if(result == RT_EOK)
    {
        /* set SPI bus to qspi modes */
        bus->mode = RT_SPI_BUS_MODE_QSPI;
    }

    return result;
}

rt_size_t rt_qspi_transfer_message(struct rt_qspi_device  *device, struct rt_qspi_message *message)
{
    rt_err_t result;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    result = rt_mutex_take(&(device->parent.bus->lock), RT_WAITING_FOREVER);
    if (result != RT_EOK)
    {
        rt_set_errno(-RT_EBUSY);

        return 0;
    }

    /* reset errno */
    rt_set_errno(RT_EOK);

    /* configure SPI bus */
    if (device->parent.bus->owner != &device->parent)
    {
        /* not the same owner as current, re-configure SPI bus */
        result = device->parent.bus->ops->configure(&device->parent, &device->parent.config);
        if (result == RT_EOK)
        {
            /* set SPI bus owner */
            device->parent.bus->owner = &device->parent;
        }
        else
        {
            /* configure SPI bus failed */
            rt_set_errno(-RT_EIO);
            goto __exit;
        }
    }

    /* transmit each SPI message */

    result = device->parent.bus->ops->xfer(&device->parent, &message->parent);
    if (result == 0)
    {
        rt_set_errno(-RT_EIO);
    }

__exit:
    /* release bus lock */
    rt_mutex_release(&(device->parent.bus->lock));

    return result;
}

rt_err_t rt_qspi_send_then_recv(struct rt_qspi_device *device, const void *send_buf,
                                rt_size_t send_length, void *recv_buf, rt_size_t recv_length)
{
    RT_ASSERT(send_buf);
    RT_ASSERT(recv_buf);
    RT_ASSERT(send_length != 0);
    RT_ASSERT(device != RT_NULL);

    struct rt_qspi_message message;
    rt_err_t result = 0;

    rt_memset(&message, 0, sizeof(message));

    result = rt_mutex_take(&(device->parent.bus->lock), RT_WAITING_FOREVER);
    if (result != RT_EOK) {
        rt_set_errno(-RT_EBUSY);
        return 0;
    }

    /* reset errno */
    rt_set_errno(RT_EOK);

    /* configure SPI bus */
    if (device->parent.bus->owner != &device->parent) {
        /* not the same owner as current, re-configure SPI bus */
        result = device->parent.bus->ops->configure(&device->parent, &device->parent.config);
        if (result == RT_EOK) {
            /* set SPI bus owner */
            device->parent.bus->owner = &device->parent;
        } else {
            /* configure SPI bus failed */
            rt_set_errno(-RT_EIO);
            result = 0;
            goto __exit;
        }
    }

    /* transmit each SPI message */
    message.qspi_data_lines = 0;
    if (send_length > 0)
        message.qspi_data_lines = 1;
    message.parent.send_buf = send_buf;
    message.parent.recv_buf = RT_NULL;
    message.parent.length = send_length;
    message.parent.cs_take = 1;
    message.parent.cs_release = 0;
    message.parent.next = RT_NULL;
    result = device->parent.bus->ops->xfer(&device->parent, &message.parent);
    if (result == 0) {
        rt_set_errno(-RT_EIO);
        goto __exit;
    }

    message.qspi_data_lines = 0;
    if (recv_length > 0)
        message.qspi_data_lines = 1;
    message.parent.send_buf = RT_NULL;
    message.parent.recv_buf = recv_buf;
    message.parent.length = recv_length;
    message.parent.cs_take = 0;
    message.parent.cs_release = 1;
    message.parent.next = RT_NULL;

    result = device->parent.bus->ops->xfer(&device->parent, &message.parent);
    if (result == 0) {
        rt_set_errno(-RT_EIO);
        goto __exit;
    }

__exit:
    /* release bus lock */
    rt_mutex_release(&(device->parent.bus->lock));

    if (result == 0)
        result = -RT_EIO;
    else
        result = recv_length;
    return result;
}

rt_err_t rt_qspi_send(struct rt_qspi_device *device, const void *send_buf, rt_size_t length)
{
    RT_ASSERT(send_buf);
    RT_ASSERT(length != 0);

    struct rt_qspi_message message;
    rt_err_t result = RT_EOK;

    rt_memset(&message, 0, sizeof(message));
    if (length > 0)
        message.qspi_data_lines = 1;
    /* set send buf and send size */
    message.parent.send_buf = send_buf;
    message.parent.recv_buf = RT_NULL;
    message.parent.length = length;
    message.parent.cs_take = 1;
    message.parent.cs_release = 1;

    result = rt_qspi_transfer_message(device, &message);
    if (result == 0)
    {
        result = -RT_EIO;
    }
    else
    {
        result = length;
    }

    return result;
}
