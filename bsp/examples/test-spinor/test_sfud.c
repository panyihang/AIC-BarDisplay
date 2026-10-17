/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Jiji.CHen <jiji.chen@artinchip.com>
 */


#include <stdint.h>
#include <string.h>
#include <rtdevice.h>
#include <spienc.h>
#include "spi_flash.h"
#include "spi_flash_sfud.h"
#include "aic_time.h"
#include <aic_core.h>
#include <finsh.h>

#define __is_print(ch)                ((unsigned int)((ch) - ' ') < 127u - ' ')

#define HEXDUMP_WIDTH                 16
#define CMD_PROBE_INDEX               0
#define CMD_READ_INDEX                1
#define CMD_WRITE_INDEX               2
#define CMD_ERASE_INDEX               3
#define CMD_RW_STATUS_INDEX           4
#define CMD_BYPASS_INDEX              5
#define CMD_BENCH_INDEX               6
#define CMD_WRITE_LEN_INDEX           7
#define CMD_REW_INDEX                 8

const char* sf_help_info[] = {
    [CMD_PROBE_INDEX]     = "sf probe [spi_device]           - probe and init SPI flash by given 'spi_device'",
    [CMD_READ_INDEX]      = "sf read addr size <dis_print>   - read 'size' bytes starting at 'addr' '1:dis-print|0:print' data",
    [CMD_WRITE_INDEX]     = "sf write addr data1 ... dataN   - write some bytes 'data' to flash starting at 'addr'",
    [CMD_ERASE_INDEX]     = "sf erase addr size              - erase 'size' bytes starting at 'addr'",
    [CMD_RW_STATUS_INDEX] = "sf status [<volatile> <status>] - read or write '1:volatile|0:non-volatile' 'status'",
#if defined(AIC_SPIENC_DRV)
    [CMD_BYPASS_INDEX]    = "sf bypass status                - status 0:disable' 1:'enable'",
#endif
    [CMD_BENCH_INDEX]     = "sf bench                        - full chip benchmark. DANGER: It will erase full chip!",
    [CMD_WRITE_LEN_INDEX] = "sf write_len addr size          - write 'size' bytes to flash starting at 'addr'",
    [CMD_REW_INDEX]       = "sf read_erase_write addr size   - test the whole speed of read-erase-write process",
};

  static sfud_flash *g_sfud_dev = NULL;

static void sf_usage()
{
    int i;

    rt_kprintf("Usage:\n");
    for (i = 0; i < sizeof(sf_help_info) / sizeof(char*); i++) {
        rt_kprintf("%s\n", sf_help_info[i]);
    }
    rt_kprintf("\n");
    return;
}

static void show_speed(char *msg, u32 len, u32 us)
{
    u32 tmp, speed;

    /* Split to serval step to avoid overflow */
    tmp = 1000 * len;
    tmp = tmp / us;
    tmp = 1000 * tmp;
    speed = tmp / 1024;

    printf("%s: %d byte, %d us -> %d KB/s\n", msg, len, us, speed);
}

static void hex_dump(uint8_t *data, uint32_t addr, uint32_t size, uint32_t dump_width)
{
    uint32_t i, j;

    for (i = 0; i < size; i += dump_width)
    {
        rt_kprintf("[%08X] ", addr + i);
        /* dump hex */
        for (j = 0; j < dump_width; j++) {
            if (i + j < size) {
                rt_kprintf("%02X ", data[i + j]);
            } else {
                rt_kprintf("   ");
            }
        }
        /* dump char for hex */
        for (j = 0; j < dump_width; j++) {
            if (i + j < size) {
                rt_kprintf("%c", __is_print(data[i + j]) ? data[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}

static void sf_do_probe(uint8_t argc, char **argv)
{
    static rt_spi_flash_device_t rtt_dev = NULL, rtt_dev_bak = NULL;

    if (argc < 1) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_PROBE_INDEX]);
        return;
    }

    char *spi_dev_name = argv[0];
    rtt_dev_bak = rtt_dev;

    /* delete the old SPI flash device */
    if(rtt_dev_bak) {
        rt_sfud_flash_delete(rtt_dev_bak);
    }

    rtt_dev = rt_sfud_flash_probe("sf_cmd", spi_dev_name);
    if (!rtt_dev) {
        rt_kprintf("sfud probe flash fail!\n");
        return;
    }

    g_sfud_dev = (sfud_flash_t)rtt_dev->user_data;
    if (g_sfud_dev->chip.capacity < 1024 * 1024)
        rt_kprintf("%d KB %s is current selected device.\n", g_sfud_dev->chip.capacity / 1024, g_sfud_dev->name);
    else
        rt_kprintf("%d MB %s is current selected device.\n", g_sfud_dev->chip.capacity / 1024 / 1024,
            g_sfud_dev->name);
}

static void sf_do_read(uint8_t argc, char **argv)
{
    sfud_err result = SFUD_SUCCESS;
    uint32_t addr, size;
    int dis_print = 0;
    uint64_t start_us;
    uint8_t *data;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_READ_INDEX]);
        return;
    }

    if (argc == 3)
        dis_print = strtol(argv[2], NULL, 0);
    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);

    if (data) {
        start_us =  aic_get_time_us();
        result = sfud_read(g_sfud_dev, addr, size, data);
        show_speed("sfud_read speed", size, aic_get_time_us() - start_us);

        if (result == SFUD_SUCCESS) {
            rt_kprintf("Read the %s flash data success. Start from 0x%08X, size is %ld.\n",
                g_sfud_dev->name, addr, size);
            if (dis_print != 1) {
                rt_kprintf("The data is:\n");
                rt_kprintf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
                hex_dump(data, addr, size, HEXDUMP_WIDTH);
            }
            rt_kprintf("\n");
        }
        aicos_free_align(0, data);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void sf_do_write(uint8_t argc, char **argv)
{
    uint32_t addr, size;
    uint8_t *data;
    int i;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_WRITE_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = argc - 1;
    data = rt_malloc(size);
    if (data) {
        for (i = 0; i < size; i++) {
            data[i] = strtol(argv[1 + i], NULL, 0);
        }
        if (SFUD_SUCCESS == sfud_write(g_sfud_dev, addr, size, data)) {
            rt_kprintf("Write the %s flash data success. Start from 0x%08X, size is %ld.\n",
                g_sfud_dev->name, addr, size);
            rt_kprintf("Write data: ");
            for (i = 0; i < size; i++) {
                rt_kprintf("%d ", data[i]);
            }
            rt_kprintf(".\n");
        }
        rt_free(data);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void sf_do_write_len(uint8_t argc, char **argv)
{
    sfud_err result = SFUD_SUCCESS;
    uint32_t addr, size;
    uint64_t start_us;
    uint8_t *data;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_WRITE_LEN_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    data = aicos_malloc_align(0, size, CACHE_LINE_SIZE);

    if (data) {
        start_us =  aic_get_time_us();
        result = sfud_write(g_sfud_dev, addr, size, data);
        show_speed("sfud_write speed", size, aic_get_time_us() - start_us);

        if (result == SFUD_SUCCESS) {
            rt_kprintf("Write the %s flash data success. Start from 0x%08X, size is %ld.\n",
                g_sfud_dev->name, addr, size);
        }
        aicos_free_align(0, data);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void sf_do_erase(uint8_t argc, char **argv)
{
    uint32_t addr, size;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_ERASE_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    if (SFUD_SUCCESS == sfud_erase(g_sfud_dev, addr, size)) {
        rt_kprintf("Erase the %s flash data success. Start from 0x%08X, size is %ld.\n", g_sfud_dev->name,
            addr, size);
    }
}

static void sf_do_status(uint8_t argc, char **argv)
{
    uint8_t status;
    bool is_volatile;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }

    if (argc < 1) {
        if (SFUD_SUCCESS == sfud_read_status(g_sfud_dev, &status)) {
            rt_kprintf("The %s flash status register current value is 0x%02X.\n", g_sfud_dev->name, status);
        }
    } else if (argc == 2) {
        is_volatile = strtol(argv[2], NULL, 0);
        status = strtol(argv[3], NULL, 0);
        if (SFUD_SUCCESS == sfud_write_status(g_sfud_dev, is_volatile, status))
            rt_kprintf("Write the %s flash status register to 0x%02X success.\n", g_sfud_dev->name, status);
    } else {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_RW_STATUS_INDEX]);
        return;
    }
}

static void sf_bench_write(uint32_t addr, uint32_t size, uint8_t *write_data)
{
    size_t write_size = SFUD_WRITE_MAX_PAGE_SIZE;
    uint32_t start_time, time_cast;
    sfud_err result = SFUD_SUCCESS;
    size_t cur_op_size;
    int i;

    rt_kprintf("Writing the %s %ld bytes data, waiting...\n", g_sfud_dev->name, size);
    start_time = rt_tick_get();
    for (i = 0; i < size; i += write_size) {
        if (i + write_size <= size) {
            cur_op_size = write_size;
        } else {
            cur_op_size = size - i;
        }
        result = sfud_write(g_sfud_dev, addr + i, cur_op_size, write_data);
        if (result != SFUD_SUCCESS) {
            rt_kprintf("Writing %s failed, already wr for %lu bytes, write %d each time\n", g_sfud_dev->name, i, write_size);
            break;
        }
    }
    if (result == SFUD_SUCCESS) {
        time_cast = rt_tick_get() - start_time;
        rt_kprintf("Write benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
            time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
    } else {
        rt_kprintf("Write benchmark has an error. Error code: %d.\n", result);
    }
}

static void sf_bench_read(uint32_t addr, uint32_t size, uint8_t *read_data, uint8_t * write_data)
{
    size_t read_size = SFUD_WRITE_MAX_PAGE_SIZE;
    uint32_t start_time, time_cast;
    sfud_err result = SFUD_SUCCESS;
    size_t cur_op_size;
    int i;

    rt_kprintf("Reading the %s %ld bytes data, waiting...\n", g_sfud_dev->name, size);
    start_time = rt_tick_get();
    for (i = 0; i < size; i += read_size) {
        if (i + read_size <= size) {
            cur_op_size = read_size;
        } else {
            cur_op_size = size - i;
        }
        result = sfud_read(g_sfud_dev, addr + i, cur_op_size, read_data);
        /* data check */
        if (memcmp(write_data, read_data, cur_op_size))
        {
            rt_kprintf("Data check ERROR! Please check you flash by other command.\n");
            result = SFUD_ERR_READ;
        }

        if (result != SFUD_SUCCESS) {
            rt_kprintf("Read %s failed, already rd for %lu bytes, read %d each time\n", g_sfud_dev->name, i, read_size);
            break;
        }
    }
    if (result == SFUD_SUCCESS) {
        time_cast = rt_tick_get() - start_time;
        rt_kprintf("Read benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
    } else {
        rt_kprintf("Read benchmark has an error. Error code: %d.\n", result);
    }
}

static void sf_do_bench(uint8_t argc, char **argv)
{
    uint8_t *write_data, *read_data;
    uint32_t start_time, time_cast;
    size_t read_size, write_size;
    sfud_err result = SFUD_SUCCESS;
    uint32_t addr, size;
    int i;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if ((argc > 0 && rt_strcmp(argv[0], "yes")) || argc < 1) {
        rt_kprintf("DANGER: It will erase full chip! Please run 'sf bench yes'.\n");
        return;
    }

    /* full chip benchmark test */
    addr = 0;
    size = g_sfud_dev->chip.capacity;
    read_size = SFUD_WRITE_MAX_PAGE_SIZE;
    write_size = SFUD_WRITE_MAX_PAGE_SIZE;

    write_data = rt_malloc(write_size);
    read_data = rt_malloc(read_size);

    if (write_data && read_data) {
        for (i = 0; i < write_size; i ++) {
            write_data[i] = i & 0xFF;
        }
        /* benchmark testing */
        rt_kprintf("Erasing the %s %ld bytes data, waiting...\n", g_sfud_dev->name, size);
        start_time = rt_tick_get();
        result = sfud_erase(g_sfud_dev, addr, size);
        if (result == SFUD_SUCCESS) {
            time_cast = rt_tick_get() - start_time;
            rt_kprintf("Erase benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
        } else {
            rt_kprintf("Erase benchmark has an error. Error code: %d.\n", result);
        }
        /* write test */
        sf_bench_write(addr, size, write_data);

        /* read test */
        sf_bench_read(addr, size, read_data, write_data);
    } else {
        rt_kprintf("Low memory!\n");
    }
    rt_free(write_data);
    rt_free(read_data);
}

static void sf_do_read_erase_write(uint8_t argc, char **argv)
{
    sfud_err result = SFUD_SUCCESS;
    uint32_t addr, size;
    uint64_t start_us;
    uint8_t *data1, *data2;

    if (!g_sfud_dev) {
        rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
        return;
    }
    if (argc < 2) {
        rt_kprintf("Usage: %s.\n", sf_help_info[CMD_REW_INDEX]);
        return;
    }

    addr = strtol(argv[0], NULL, 0);
    size = strtol(argv[1], NULL, 0);
    data1 = aicos_malloc_align(0, size, CACHE_LINE_SIZE);
    data2 = aicos_malloc_align(0, size, CACHE_LINE_SIZE);

    if (data1 != NULL && data2 != NULL) {
        start_us =  aic_get_time_us();
        result = sfud_read(g_sfud_dev, addr, size, data1);
        if (result != SFUD_SUCCESS) {
            rt_kprintf("Read data failed!\n");
            return;
        }
        result = sfud_erase(g_sfud_dev, addr, size);
        if (result != SFUD_SUCCESS) {
            rt_kprintf("Erase data failed!\n");
            return;
        }
        memset(data2, 0x00, size);
        result = sfud_write(g_sfud_dev, addr, size, data2);
        if (result != SFUD_SUCCESS) {
            rt_kprintf("Write data failed!\n");
            return;
        }
        show_speed("sfud read_erase_write speed", size, aic_get_time_us() - start_us);
        rt_kprintf("read data1:\n");
        hex_dump(data1, addr, 256, HEXDUMP_WIDTH);
        rt_kprintf("write data2:\n");
        hex_dump(data2, addr, 256, HEXDUMP_WIDTH);

        aicos_free_align(0, data1);
        aicos_free_align(0, data2);
    } else {
        rt_kprintf("Low memory!\n");
    }
}

static void sf(uint8_t argc, char **argv)
{
    if (argc < 2) {
        sf_usage();
        return;
    }
    const char *operator = argv[1];

    if (!strcmp(operator, "probe")) {
        sf_do_probe(argc - 2, &argv[2]);
#if defined(AIC_SPIENC_DRV)
    } else if (!strcmp(operator, "bypass")) {
        uint32_t status;
        if (!g_sfud_dev) {
            rt_kprintf("No flash device selected. Please run 'sf probe'.\n");
            return;
        }
        status = strtol(argv[2], NULL, 0);
        spienc_set_bypass(status);
#endif
    } else if (!rt_strcmp(operator, "read")) {
        sf_do_read(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "write")) {
        sf_do_write(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "write_len")) {
        sf_do_write_len(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "erase")) {
        sf_do_erase(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "status")) {
        sf_do_status(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "bench")) {
        sf_do_bench(argc - 2, &argv[2]);
    } else if (!rt_strcmp(operator, "read_erase_write")) {
        sf_do_read_erase_write(argc - 2, &argv[2]);
    } else {
        sf_usage();
    }
}
MSH_CMD_EXPORT(sf, SPI Flash operate);
