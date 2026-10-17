/*
 * Copyright (c) 2023-2026, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: xuan.wen <xuan.wen@artinchip.com>
 */

#include <rtdevice.h>
#include <aic_log.h>
#include <rtconfig.h>

#if defined(RT_USING_FAL)
#include <fal.h>
#include <sfud.h>
#include <spi_flash_sfud.h>

/**
 * @brief Get the number of SFUD flash devices.
 * @return Number of SFUD flash devices detected.
 */
extern rt_uint32_t spinor_sfud_port_get_flash_count(void);

/**
 * @brief Get SFUD flash device name by index.
 * @param idx Flash device index.
 * @return Pointer to flash device name string.
 */
extern const char *spinor_sfud_port_get_flash_name(rt_uint32_t idx);

/** FAL flash device table, dynamically allocated */
struct fal_flash_dev *fal_flash_table = NULL;
/** Pointer array to FAL flash devices, each element points to one flash device in fal_flash_table */
struct fal_flash_dev **fal_flash_table_ptr = NULL;

/** Number of flash devices in the table */
size_t fal_flash_devices_count = 0;

/**
 * @brief Initialize FAL flash device by finding SFUD device.
 *
 * This function locates the SFUD device by name, caches it in
 * user_data, and updates chip info in fal_flash_dev.
 *
 * @param dev Pointer to FAL flash device structure.
 * @return 0 on success, -1 on failure.
 */
static int fal_sfud_flash_init(struct fal_flash_dev *dev)
{
    sfud_flash_t sfud_dev = rt_sfud_flash_find_by_dev_name(dev->name);

    if (sfud_dev == NULL) {
        log_e("SFUD device '%s' not found", dev->name);
        return -1;
    }

    dev->user_data  = sfud_dev;
    dev->blk_size   = sfud_dev->chip.eraser[0].size;
    dev->len        = sfud_dev->chip.capacity;
    dev->addr       = 0;
    dev->write_gran = 256;

    return 0;
}

/**
 * @brief Read data from flash via SFUD library.
 *
 * @param dev Pointer to FAL flash device structure.
 * @param offset Read offset address within flash device.
 * @param buf Buffer to store read data.
 * @param size Number of bytes to read.
 * @return Number of bytes read on success, -1 on failure.
 */
static int fal_sfud_flash_read(const struct fal_flash_dev *dev, long offset, uint8_t *buf,
                               size_t size)
{
    sfud_flash_t sfud_dev = dev->user_data;

    assert(sfud_dev);
    sfud_read(sfud_dev, dev->addr + offset, size, buf);
    return size;
}

/**
 * @brief Write data to flash via SFUD library.
 *
 * @param dev Pointer to FAL flash device structure.
 * @param offset Write offset address within flash device.
 * @param buf Buffer containing data to write.
 * @param size Number of bytes to write.
 * @return Number of bytes written on success, -1 on failure.
 */
static int fal_sfud_flash_write(const struct fal_flash_dev *dev, long offset, const uint8_t *buf,
                                size_t size)
{
    sfud_flash_t sfud_dev = dev->user_data;

    assert(sfud_dev);
    if (sfud_write(sfud_dev, dev->addr + offset, size, buf)
        != SFUD_SUCCESS)
        return -1;
    return size;
}

/**
 * @brief Erase flash region via SFUD library.
 *
 * @param dev Pointer to FAL flash device structure.
 * @param offset Erase offset address within flash device.
 * @param size Number of bytes to erase.
 * @return Number of bytes erased on success, -1 on failure.
 */
static int fal_sfud_flash_erase(const struct fal_flash_dev *dev, long offset, size_t size)
{
    sfud_flash_t sfud_dev = dev->user_data;

    assert(sfud_dev);
    if (sfud_erase(sfud_dev, dev->addr + offset, size)
        != SFUD_SUCCESS)
        return -1;
    return size;
}

/**
 * @brief Prepare FAL flash device table from SFUD devices.
 *
 * This function queries the number of SFUD flash devices,
 * allocates memory for FAL flash table, and initializes
 * each device with shared SFUD operations.
 *
 * @return 0 on success, -1 on failure.
 */
static int prepare_fal_flash(void)
{
    rt_uint32_t cnt, i;
    const char *flash_name;

    /* Get number of SFUD flash devices detected on SPI buses */
    cnt = spinor_sfud_port_get_flash_count();
    if (cnt == 0)
        return -1;

    /* Allocate pointer array for external access to flash devices */
    fal_flash_table_ptr = (void *)rt_malloc(cnt * sizeof(void *));
    if (fal_flash_table_ptr == NULL)
        return -1;
    rt_memset(fal_flash_table_ptr, 0, (cnt * sizeof(void *)));

    /* Allocate contiguous memory block for flash device structures */
    fal_flash_table = (void *)rt_malloc(cnt * sizeof(struct fal_flash_dev));
    if (fal_flash_table == NULL) {
        /* Free previously allocated pointer array on failure */
        rt_free(fal_flash_table_ptr);
        fal_flash_table_ptr = NULL;
        return -1;
    }
    rt_memset(fal_flash_table, 0, (cnt * sizeof(struct fal_flash_dev)));
    fal_flash_devices_count = cnt;

    /* Initialize each flash device with name and SFUD operations */
    for (i = 0; i < cnt; i++) {
        flash_name = spinor_sfud_port_get_flash_name(i);
        rt_strcpy(fal_flash_table[i].name, flash_name);
        fal_flash_table[i].ops.init = fal_sfud_flash_init;
        fal_flash_table[i].ops.write = fal_sfud_flash_write;
        fal_flash_table[i].ops.read = fal_sfud_flash_read;
        fal_flash_table[i].ops.erase = fal_sfud_flash_erase;
        /* Set pointer array element to point to corresponding device */
        fal_flash_table_ptr[i] = &fal_flash_table[i];
    }

    return 0;
}

/**
 * @brief Initialize FAL subsystem with SFUD flash support.
 *
 * This is the main entry point for FAL initialization. It prepares
 * the flash device table, initializes FAL core, and creates RT-Thread
 * MTD and block devices for each FAL partition.
 *
 * @return 0 on success, negative error code on failure.
 */
static int fal_with_sfud_flash_init(void)
{
    const struct fal_partition *parts;
    struct rt_device *flash_dev;
    int result = 0;
    size_t i, len;

    result = prepare_fal_flash();
    if (result)
        return result;

    fal_init();

    parts = fal_get_partition_table(&len);
    if (!parts) {
        pr_info("No FAL partitions.\n");
        return result;
    }

    for (i = 0; i < len; i++) {
        flash_dev = fal_mtd_nor_device_create(parts[i].name);
        if (flash_dev == NULL) {
            pr_err("Can't create a mtd device on '%s' partition.", parts[i].name);
            result = -1;
            break;
        }

        flash_dev = fal_blk_device_create(parts[i].name);
        if (flash_dev == NULL) {
            pr_err("Can't create a blk device on '%s' partition.", parts[i].name);
            result = -1;
            break;
        }
    }

    return result;
}

INIT_DEVICE_EXPORT(fal_with_sfud_flash_init);
#endif
