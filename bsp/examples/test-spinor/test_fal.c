/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */

#include <fal.h>

#ifdef RT_VER_NUM
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <finsh.h>
#include <aic_core.h>

#define __is_print(ch)                ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH                 16
#define CMD_PROBE_INDEX               0
#define CMD_READ_INDEX                1
#define CMD_WRITE_INDEX               2
#define CMD_ERASE_INDEX               3
#define CMD_BENCH_INDEX               4

const char* help_info[] =
{
    [CMD_PROBE_INDEX]     = "fal probe [dev_name|part_name]   - probe flash device or partition by given name",
    [CMD_READ_INDEX]      = "fal read addr size               - read 'size' bytes starting at 'addr'",
    [CMD_WRITE_INDEX]     = "fal write addr data1 ... dataN   - write some bytes 'data' starting at 'addr'",
    [CMD_ERASE_INDEX]     = "fal erase addr size              - erase 'size' bytes starting at 'addr'",
    [CMD_BENCH_INDEX]     = "fal bench <blk_size>             - benchmark test with per block size",
};

static void fal_usage()
{
    int i;
    rt_kprintf("Usage:\n");
    for (i = 0; i < sizeof(help_info) / sizeof(char*); i++)
    {
        rt_kprintf("%s\n", help_info[i]);
    }
    rt_kprintf("\n");
}

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    if (us)
        tmp = tmp / us;
    else
        tmp = 0;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static const struct fal_flash_dev *flash_dev = NULL;
static const struct fal_partition *part_dev = NULL;

static void fal_do_probe(uint8_t argc, char **argv)
{
    if (argc < 1)
        return;

    char *dev_name = argv[0];
    if ((flash_dev = fal_flash_device_find(dev_name)) != NULL)
    {
        part_dev = NULL;
    }
    else if ((part_dev = fal_partition_find(dev_name)) != NULL)
    {
        flash_dev = NULL;
    }
    else
    {
        rt_kprintf("Device %s NOT found. Probe failed.\n", dev_name);
        flash_dev = NULL;
        part_dev = NULL;
    }

    if (flash_dev)
    {
        rt_kprintf("Probed a flash device | %s | addr: %ld | len: %d |.\n", flash_dev->name,
                flash_dev->addr, flash_dev->len);
    }
    else if (part_dev)
    {
        rt_kprintf("Probed a flash partition | %s | flash_dev: %s | offset: %ld | len: %d |.\n",
                part_dev->name, part_dev->flash_name, part_dev->offset, part_dev->len);
    }
    else
    {
        rt_kprintf("No flash device or partition was probed.\n");
        rt_kprintf("Usage: %s.\n", help_info[CMD_PROBE_INDEX]);
        fal_show_part_table();
    }

}

static void fal_do_read(uint8_t argc, char **argv)
{
    uint32_t addr;
    uint32_t size = 0;
    int result = -1;
    int i, j;

    if (argc < 2)
    {
        rt_kprintf("Usage: %s.\n", help_info[CMD_READ_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    uint8_t *data = rt_malloc(size);
    uint64_t start_us =  aic_get_time_us();
    if (NULL == data)
    {
        rt_kprintf("Low memory.\n");
        return;
    }

    if (flash_dev)
        result = flash_dev->ops.read(flash_dev, addr, data, size);
    else if (part_dev)
        result = fal_partition_read(part_dev, addr, data, size);
    else
        rt_kprintf("No flash device or partition was probed.\n");

    if (result < 0)
    {
        rt_kprintf("Read data failed.\n");
        rt_free(data);
        return;
    }
    show_speed("fal read speed", size, aic_get_time_us() - start_us);

    rt_kprintf("Read data success. Start from 0x%08X, size is %ld. The data is:\n", addr,
            size);
    rt_kprintf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    for (i = 0; i < size; i += HEXDUMP_WIDTH)
    {
        rt_kprintf("[%08X] ", addr + i);
        /* dump hex */
        for (j = 0; j < HEXDUMP_WIDTH; j++)
        {
            if (i + j < size)
            {
                rt_kprintf("%02X ", data[i + j]);
            }
            else
            {
                rt_kprintf("   ");
            }
        }
        /* dump char for hex */
        for (j = 0; j < HEXDUMP_WIDTH; j++)
        {
            if (i + j < size)
            {
                rt_kprintf("%c", __is_print(data[i + j]) ? data[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
    rt_kprintf("\n");
    rt_free(data);
    return;
}

static void fal_do_write(uint8_t argc, char **argv)
{
    uint32_t addr;
    uint32_t size = 0;
    int result = -1;
    int i;

    if (argc < 2)
    {
        rt_kprintf("Usage: %s.\n", help_info[CMD_WRITE_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = argc - 1;
    uint8_t *data = rt_malloc(size);
    if (NULL == data)
    {
        rt_kprintf("Low memory.\n");
        return;
    }

    for (i = 0; i < size; i++)
    {
        data[i] = strtol(argv[1 + i], NULL, 0);
    }

    if (flash_dev)
        result = flash_dev->ops.write(flash_dev, addr, data, size);
    else if (part_dev)
        result = fal_partition_write(part_dev, addr, data, size);
    else
        rt_kprintf("No flash device or partition was probed.\n");

    if (result < 0)
    {
        rt_kprintf("Write data failed.\n");
        rt_free(data);
        return;
    }

    rt_kprintf("Write data success. Start from 0x%08X, size is %ld.\n", addr, size);
    rt_kprintf("Write data: ");
    for (i = 0; i < size; i++)
    {
        rt_kprintf("%d ", data[i]);
    }
    rt_kprintf(".\n");
    rt_free(data);
    return;
}

static void fal_do_erase(uint8_t argc, char **argv)
{
    uint32_t addr;
    uint32_t size = 0;
    int result = -1;
    uint64_t start_us;

    if (argc < 2)
    {
        rt_kprintf("Usage: %s.\n", help_info[CMD_ERASE_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    start_us =  aic_get_time_us();
    if (flash_dev)
        result = flash_dev->ops.erase(flash_dev, addr, size);
    else if (part_dev)
        result = fal_partition_erase(part_dev, addr, size);
    else
        rt_kprintf("No flash device or partition was probed.\n");

    if (result < 0)
        rt_kprintf("Erase data failed.\n");
    else
        rt_kprintf("Erase data success. Start from 0x%08X, size is %ld.\n", addr, size);
    show_speed("fal erase speed", size, aic_get_time_us() - start_us);

    return;
}

static void fal_do_bench(uint8_t argc, char **argv)
{
    uint32_t size = 0;
    int result = -1;
    int i;

    if (argc < 1)
    {
        rt_kprintf("Usage: %s.\n", help_info[CMD_BENCH_INDEX]);
        return;
    }
    else if ((argc > 1 && strcmp(argv[1], "yes")) || argc < 2)
    {
        rt_kprintf("DANGER: It will erase full chip or partition! Please run 'fal bench %d yes'.\n", strtol(argv[0], NULL, 0));
        return;
    }
    /* full chip benchmark test */
    uint32_t start_time, time_cast;
    size_t write_size = strtol(argv[0], NULL, 0), read_size = strtol(argv[0], NULL, 0), cur_op_size;
    uint8_t *write_data = (uint8_t *)rt_malloc(write_size), *read_data = (uint8_t *)rt_malloc(read_size);

    if (write_data && read_data)
    {
        for (i = 0; i < write_size; i ++) {
            write_data[i] = i & 0xFF;
        }
        if (flash_dev)
        {
            size = flash_dev->len;
        }
        else if (part_dev)
        {
            size = part_dev->len;
        }
        /* benchmark testing */
        rt_kprintf("Erasing %ld bytes data, waiting...\n", size);
        start_time = rt_tick_get();
        if (flash_dev)
        {
            result = flash_dev->ops.erase(flash_dev, 0, size);
        }
        else if (part_dev)
        {
            result = fal_partition_erase(part_dev, 0, size);
        }
        if (result >= 0)
        {
            time_cast = rt_tick_get() - start_time;
            rt_kprintf("Erase benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                    time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
        }
        else
        {
            rt_kprintf("Erase benchmark has an error. Error code: %d.\n", result);
        }
        /* write test */
        rt_kprintf("Writing %ld bytes data, waiting...\n", size);
        start_time = rt_tick_get();
        for (i = 0; i < size; i += write_size)
        {
            if (i + write_size <= size)
            {
                cur_op_size = write_size;
            }
            else
            {
                cur_op_size = size - i;
            }
            if (flash_dev)
            {
                result = flash_dev->ops.write(flash_dev, i, write_data, cur_op_size);
            }
            else if (part_dev)
            {
                result = fal_partition_write(part_dev, i, write_data, cur_op_size);
            }
            if (result < 0)
            {
                break;
            }
        }
        if (result >= 0)
        {
            time_cast = rt_tick_get() - start_time;
            rt_kprintf("Write benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                    time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
        }
        else
        {
            rt_kprintf("Write benchmark has an error. Error code: %d.\n", result);
        }
        /* read test */
        rt_kprintf("Reading %ld bytes data, waiting...\n", size);
        start_time = rt_tick_get();
        for (i = 0; i < size; i += read_size)
        {
            if (i + read_size <= size)
            {
                cur_op_size = read_size;
            }
            else
            {
                cur_op_size = size - i;
            }
            if (flash_dev)
            {
                result = flash_dev->ops.read(flash_dev, i, read_data, cur_op_size);
            }
            else if (part_dev)
            {
                result = fal_partition_read(part_dev, i, read_data, cur_op_size);
            }
            /* data check */
            for (size_t index = 0; index < cur_op_size; index ++)
            {
                if (write_data[index] != read_data[index])
                {
                    rt_kprintf("%d %d %02x %02x.\n", i, index, write_data[index], read_data[index]);
                }
            }

            if (memcmp(write_data, read_data, cur_op_size))
            {
                result = -RT_ERROR;
                rt_kprintf("Data check ERROR! Please check you flash by other command.\n");
            }
            /* has an error */
            if (result < 0)
            {
                break;
            }
        }
        if (result >= 0)
        {
            time_cast = rt_tick_get() - start_time;
            rt_kprintf("Read benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                    time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
        }
        else
        {
            rt_kprintf("Read benchmark has an error. Error code: %d.\n", result);
        }
    }
    else
    {
        rt_kprintf("Low memory!\n");
    }
    rt_free(write_data);
    rt_free(read_data);
}

extern int fal_init_check(void);

static void fal(uint8_t argc, char **argv) {
    if (fal_init_check() != 1) {
        rt_kprintf("\n[Warning] FAL is not initialized or failed to initialize!\n\n");
        return;
    }

    if (argc < 2) {
        fal_usage();
        return;
    }

    const char *operator = argv[1];

    if (!strcmp(operator, "probe"))
    {
        fal_do_probe(argc - 2, &argv[2]);
        return;
    }

    if (!flash_dev && !part_dev)
    {
        rt_kprintf("No flash device or partition was probed. Please run 'fal probe'.\n");
        return;
    }
    if (!rt_strcmp(operator, "read"))
        fal_do_read(argc - 2, &argv[2]);
    else if (!strcmp(operator, "write"))
        fal_do_write(argc - 2, &argv[2]);
    else if (!rt_strcmp(operator, "erase"))
        fal_do_erase(argc - 2, &argv[2]);
    else if (!strcmp(operator, "bench"))
        fal_do_bench(argc - 2, &argv[2]);
    else
        fal_usage();

    return;
}
MSH_CMD_EXPORT(fal, FAL (Flash Abstraction Layer) operate);

#endif /* RT_VER_NUM */
