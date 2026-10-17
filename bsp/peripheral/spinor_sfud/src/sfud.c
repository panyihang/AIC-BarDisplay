/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: serial flash operate functions by SFUD lib.
 * Created on: 2016-04-23
 */

#include <spienc.h>
#include "../inc/sfud.h"
#include "sfud_qe.h"
#include <string.h>
#include <inttypes.h>
#include "sfud_private.h"

/* send dummy data for read data */
#define DUMMY_DATA                               0xFF

#ifndef SFUD_FLASH_DEVICE_TABLE
#error "Please configure the flash device information table in (in sfud_cfg.h)."
#endif

/* user configured flash device information table */
static sfud_flash flash_table[] = SFUD_FLASH_DEVICE_TABLE;
/* supported manufacturer information table */
static const sfud_mf mf_table[] = SFUD_MF_TABLE;

#ifdef SFUD_USING_FLASH_INFO_TABLE
/* supported flash chip information table */
static const sfud_flash_chip flash_chip_table[] = SFUD_FLASH_CHIP_TABLE;
#endif

#ifdef SFUD_USING_QSPI
/**
 * flash read data mode
 */
enum sfud_qspi_read_mode {
    NORMAL_SPI_READ = 1 << 0,               /**< mormal spi read mode */
    DUAL_OUTPUT = 1 << 1,                   /**< qspi fast read dual output */
    DUAL_IO = 1 << 2,                       /**< qspi fast read dual input/output */
    QUAD_OUTPUT = 1 << 3,                   /**< qspi fast read quad output */
    QUAD_IO = 1 << 4,                       /**< qspi fast read quad input/output */
};

/* QSPI flash chip's extended information table */
static const sfud_qspi_flash_ext_info qspi_flash_ext_info_table[] = SFUD_FLASH_EXT_INFO_TABLE;
#endif /* SFUD_USING_QSPI */

static sfud_err software_init(const sfud_flash *flash);
static sfud_err hardware_init(sfud_flash *flash);
static sfud_err page256_or_1_byte_write(const sfud_flash *flash, uint32_t addr, size_t size, uint16_t write_gran,
        const uint8_t *data);
static sfud_err aai_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data);
static sfud_err wait_busy(const sfud_flash *flash, size_t tmo_us);
static sfud_err reset(const sfud_flash *flash);
static sfud_err read_jedec_id(sfud_flash *flash);
sfud_err set_write_enabled(const sfud_flash *flash, bool enabled);
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled);
static int make_adress_byte_array(const sfud_flash *flash, uint32_t addr, uint8_t *array);
static sfud_err auto_set_rx_delay_mode(sfud_flash *flash);

/**
 * SFUD initialize by flash device
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_device_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /* hardware initialize */
    result = hardware_init(flash);
    if (result == SFUD_SUCCESS) {
        result = software_init(flash);
    }

#if defined(AIC_SPIENC_DRV)
    if (result == SFUD_SUCCESS) {
        /* Enable SPIENC */
        uint32_t bus_id;

        result = flash->spi.get_bus_id(&flash->spi, &bus_id);
        if (result == SFUD_SUCCESS) {
            pr_info("init %d spienc...\n", (unsigned int)bus_id);
            if (spienc_init() != 0) {
                pr_err("spienc init failed.\n");
                result = SFUD_ERR_NOT_FOUND;
            }
        }
    }
#endif

    if (result == SFUD_SUCCESS) {
        flash->init_ok = true;
        SFUD_INFO("%s flash device initialized successfully.", flash->name);
    } else {
        flash->init_ok = false;
        SFUD_INFO("Error: %s flash device initialization failed.", flash->name);
    }

    return result;
}

/**
 * SFUD library initialize.
 *
 * @return result
 */
sfud_err sfud_init(void) {
    sfud_err cur_flash_result = SFUD_SUCCESS, all_flash_result = SFUD_SUCCESS;
    size_t i;

    SFUD_DEBUG("Start initialize Serial Flash Universal Driver(SFUD) V%s.", SFUD_SW_VERSION);
    SFUD_DEBUG("You can get the latest version on https://github.com/armink/SFUD .");
    /* initialize all flash device in flash device table */
    for (i = 0; i < sizeof(flash_table) / sizeof(sfud_flash); i++) {
        /* initialize flash device index of flash device information table */
        flash_table[i].index = i;
        cur_flash_result = sfud_device_init(&flash_table[i]);

        if (cur_flash_result != SFUD_SUCCESS) {
            all_flash_result = cur_flash_result;
        }
    }

    return all_flash_result;
}

/**
 * get flash device by its index which in the flash information table
 *
 * @param index the index which in the flash information table  @see flash_table
 *
 * @return flash device
 */
sfud_flash *sfud_get_device(size_t index) {
    if (index < sfud_get_device_num()) {
        return &flash_table[index];
    } else {
        return NULL;
    }
}

/**
 * get flash device total number on flash device information table  @see flash_table
 *
 * @return flash device total number
 */
size_t sfud_get_device_num(void) {
    return sizeof(flash_table) / sizeof(sfud_flash);
}

/**
 * get flash device information table  @see flash_table
 *
 * @return flash device table pointer
 */
const sfud_flash *sfud_get_device_table(void) {
    return flash_table;
}

#ifdef SFUD_USING_QSPI

static void qspi_set_read_cmd_format(sfud_flash *flash, uint8_t ins, uint8_t ins_lines, uint8_t addr_lines,
        uint8_t dummy_cycles, uint8_t data_lines) {

    /* if medium size greater than 16Mb, use 4-Byte address, instruction should be added one */
    if (flash->chip.capacity <= 0x1000000) {
        flash->read_cmd_format.instruction = ins;
        flash->read_cmd_format.address_size = 24;
    } else {
        if (ins == SFUD_CMD_READ_DATA) {
            flash->read_cmd_format.instruction = ins + 0x10;
        } else {
            flash->read_cmd_format.instruction = ins + 1;
        }
        flash->read_cmd_format.address_size = 32;
    }

    flash->read_cmd_format.instruction_lines = ins_lines;
    flash->read_cmd_format.address_lines = addr_lines;
    flash->read_cmd_format.alternate_bytes_lines = 0;
    flash->read_cmd_format.dummy_cycles = dummy_cycles;
    flash->read_cmd_format.data_lines = data_lines;
}

/**
 * Enbale the fast read mode in QSPI flash mode. Default read mode is normal SPI mode.
 *
 * it will find the appropriate fast-read instruction to replace the read instruction(0x03)
 * fast-read instruction @see SFUD_FLASH_EXT_INFO_TABLE
 *
 * @note When Flash is in QSPI mode, the method must be called after sfud_device_init().
 *
 * @param flash flash device
 * @param data_line_width the data lines max width which QSPI bus supported, such as 1, 2, 4
 *
 * @return result
 */
sfud_err sfud_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width) {
    size_t i = 0;
    uint8_t read_mode = NORMAL_SPI_READ;
    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data_line_width == 1 || data_line_width == 2 || data_line_width == 4);

    /* Set default value from SFDP, and it can be overwrite by user's setting */
    read_mode = flash->sfdp.hwcaps_fast_read;

    /* get read_mode, If don't found, the default is SFUD_QSPI_NORMAL_SPI_READ */
    for (i = 0; i < sizeof(qspi_flash_ext_info_table) / sizeof(sfud_qspi_flash_ext_info); i++) {
        if ((qspi_flash_ext_info_table[i].mf_id == flash->chip.mf_id)
                && (qspi_flash_ext_info_table[i].type_id == flash->chip.type_id)
                && (qspi_flash_ext_info_table[i].capacity_id == flash->chip.capacity_id)) {
            read_mode = qspi_flash_ext_info_table[i].read_mode;
        }
    }

    /* determine qspi supports which read mode and set read_cmd_format struct */
    switch (data_line_width) {
    case 1:
#ifdef SFUD_USING_SFDP
        if (flash->bus_hz > SFUD_MIN_FREQ_FAST_READ)
            qspi_set_read_cmd_format(flash, SFUD_CMD_FAST_READ, 1, 1, 8, 1);
        else
#endif
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        break;
    case 2:
        if (read_mode & DUAL_IO) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_DUAL_IO_READ_DATA, 1, 2, 4, 2);
        } else if (read_mode & DUAL_OUTPUT) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_DUAL_OUTPUT_READ_DATA, 1, 1, 8, 2);
        } else {
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        }
        break;
    case 4:
        if (read_mode & QUAD_IO) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_QUAD_IO_READ_DATA, 1, 4, 6, 4);
        } else if (read_mode & QUAD_OUTPUT) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_QUAD_OUTPUT_READ_DATA, 1, 1, 8, 4);
        } else {
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        }
        break;
    }

    return result;
}
#endif /* SFUD_USING_QSPI */

/*
* sfud init sfdp
*/
static bool sfud_init_info(sfud_flash *flash) {
    bool ret = false;
    size_t i = 0;

#ifdef SFUD_USING_SFDP
    extern bool sfud_read_sfdp(sfud_flash *flash);
        /* read SFDP parameters */
    ret =  sfud_read_sfdp(flash);
    if (ret) {
        flash->chip.name = NULL;
        flash->chip.capacity = flash->sfdp.capacity;
        /* only 1 byte or 256 bytes write mode for SFDP */
        if (flash->sfdp.write_gran == 1) {
            flash->chip.write_mode = SFUD_WM_BYTE;
        } else {
            flash->chip.write_mode = SFUD_WM_PAGE_256B;
        }
        /* find the the smallest erase sector size for eraser. then will use this size for erase granularity */
        flash->chip.eraser[0].size = flash->sfdp.eraser[0].size;
        flash->chip.eraser[0].cmd = flash->sfdp.eraser[0].cmd;
        for (i = 1; i < SFUD_SFDP_ERASE_TYPE_MAX_NUM; i++) {
            if (flash->sfdp.eraser[i].size != 0 && flash->chip.eraser[0].size > flash->sfdp.eraser[i].size) {
                flash->chip.eraser[0].size = flash->sfdp.eraser[i].size;
                flash->chip.eraser[0].cmd = flash->sfdp.eraser[i].cmd;
            }
        }
    } else {
#endif
#ifdef SFUD_USING_FLASH_INFO_TABLE
        /* read SFDP parameters failed then using SFUD library provided static parameter */
        for (i = 0; i < sizeof(flash_chip_table) / sizeof(sfud_flash_chip); i++) {
            if ((flash_chip_table[i].mf_id == flash->chip.mf_id)
                    && (flash_chip_table[i].type_id == flash->chip.type_id)
                    && (flash_chip_table[i].capacity_id == flash->chip.capacity_id)) {
                flash->chip.name = flash_chip_table[i].name;
                flash->chip.capacity = flash_chip_table[i].capacity;
                flash->chip.write_mode = flash_chip_table[i].write_mode;
                flash->chip.eraser[0].size = flash_chip_table[i].eraser[0].size;
                flash->chip.eraser[0].cmd = flash_chip_table[i].eraser[0].cmd;
                flash->chip.eraser[1].size = flash_chip_table[i].eraser[1].size;
                flash->chip.eraser[1].cmd = flash_chip_table[i].eraser[1].cmd;
                break;
            }
        }
#endif
#ifdef SFUD_USING_SFDP
    }
#endif
    return ret;
}

static sfud_err sfud_check_flash(sfud_flash *flash) {
    size_t i = 0;

    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 || flash->chip.eraser[0].size == 0
            || flash->chip.eraser[0].cmd == 0) {
        SFUD_INFO("Warning: This flash device is not found or not support.");
        return SFUD_ERR_NOT_FOUND;
    } else {
        const char *flash_mf_name = NULL;
        /* find the manufacturer information */
        for (i = 0; i < sizeof(mf_table) / sizeof(sfud_mf); i++) {
            if (mf_table[i].id == flash->chip.mf_id) {
                flash_mf_name = mf_table[i].name;
                break;
            }
        }
        /* print manufacturer and flash chip name */
        SFUD_INFO("Flash ID: 0x%02x%02x%02x", flash->chip.mf_id, flash->chip.type_id, flash->chip.capacity_id);
        if (flash_mf_name && flash->chip.name) {
            SFUD_INFO("Find a %s %s flash chip. Size is %" PRIu32 " bytes.", flash_mf_name, flash->chip.name,
                    flash->chip.capacity);
        } else if (flash_mf_name) {
            SFUD_INFO("Find a %s flash chip. Size is %" PRIu32 " bytes.", flash_mf_name, flash->chip.capacity);
        } else {
            SFUD_INFO("Find a flash chip. Size is %" PRIu32 " bytes.", flash->chip.capacity);
        }
    }

    return SFUD_SUCCESS;
}

/**
 * hardware initialize
 */
static sfud_err hardware_init(sfud_flash *flash) {
    extern sfud_err sfud_spi_port_init(sfud_flash * flash);

    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);

    result = sfud_spi_port_init(flash);
    if (result != SFUD_SUCCESS) {
        return result;
    }

    if (flash->spi.set_speed)
        flash->spi.set_speed(&flash->spi, flash->init_hz);
#ifdef SFUD_USING_QSPI
    /* set default read instruction */
    flash->read_cmd_format.instruction = SFUD_CMD_READ_DATA;

    /* Initialize legacy flash parameters and settings. */
    flash->quad_enable = spi_nor_quad_enable;
#endif /* SFUD_USING_QSPI */

    /* reset flash device */
    result = reset(flash);
    if (result != SFUD_SUCCESS) {
        return result;
    }

    /* SPI write read function must be initialize */
    SFUD_ASSERT(flash->spi.wr);
    /* if the user don't configure flash chip information then using SFDP parameter or static flash parameter table */
    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 || flash->chip.eraser[0].size == 0
            || flash->chip.eraser[0].cmd == 0) {
        /* read JEDEC ID include manufacturer ID, memory type ID and flash capacity ID */
        result = read_jedec_id(flash);
        if (result != SFUD_SUCCESS) {
            return result;
        }

    sfud_init_info(flash);
    }

    result = sfud_check_flash(flash);
    if (result != SFUD_SUCCESS) {
            return result;
    }

    /* The flash all blocks is protected,so need change the flash status to unprotected before write and erase operate. */
    if (flash->chip.write_mode & SFUD_WM_AAI) {
        result = sfud_write_status(flash, true, 0x00);
    } else {
        /* MX25L3206E */
        if ((0xC2 == flash->chip.mf_id) && (0x20 == flash->chip.type_id) && (0x16 == flash->chip.capacity_id)) {
            result = sfud_write_status(flash, true, 0x00);
        }
    }
    if (result != SFUD_SUCCESS) {
        return result;
    }

    /* if the flash is large than 16MB (256Mb) then enter in 4-Byte addressing mode */
    if (flash->chip.capacity > (1L << 24)) {
        result = set_4_byte_address_mode(flash, true);
    } else {
        flash->addr_in_4_byte = false;
    }

#ifdef SFUD_USING_SFDP
    /* After SFDP is read, switch to the working frequency */
    if (flash->spi.set_speed)
        flash->spi.set_speed(&flash->spi, flash->bus_hz);
    auto_set_rx_delay_mode(flash);
#endif

    /* Check whether the flash is in write protect state. If it is, exit the state. */
    spi_nor_sr_unlock_all(flash);

#ifdef SFUD_USING_QSPI
    quad_enable_func qe = flash->quad_enable;
    qe(flash);
#endif /* SFUD_USING_QSPI */

    flash->get_uid = spi_nor_get_unique_id;

    return result;
}

/**
 * software initialize
 *
 * @param flash flash device
 *
 * @return result
 */
static sfud_err software_init(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);

    return result;
}

static sfud_err sfud_read_data(const sfud_flash *flash, uint32_t addr, size_t size, uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size;
    uint32_t bus_id;

    result = spi->get_bus_id(spi, &bus_id);
    if (result)
        return result;

#ifdef SFUD_USING_QSPI
    if (flash->read_cmd_format.instruction != SFUD_CMD_READ_DATA) {
        result = spi->qspi_read(spi, addr, (sfud_qspi_read_cmd_format *)&flash->read_cmd_format,
                                data, size);
        return result;
    }
#endif
    {
        cmd_data[0] = SFUD_CMD_READ_DATA;
        make_adress_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;
#if defined(AIC_SPIENC_DRV)
        spienc_set_cfg(bus_id, addr, cmd_size, size);
        spienc_start();
#endif
        result = spi->wr(spi, cmd_data, cmd_size, data, size);
#if defined(AIC_SPIENC_DRV)
        spienc_stop();
        if (spienc_check_empty())
            memset(data, 0xFF, size);
#endif
    }

    return result;
}

#define READ_ALIGN_SIZE 8
/**
 * read flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size read size
 * @param data read data pointer
 *
 * @return result
 */
sfud_err sfud_read(const sfud_flash *flash, uint32_t addr, size_t size, uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t *p, page_data[SFUD_WRITE_MAX_PAGE_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));
    int copy_flag;
    size_t dolen;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    result = wait_busy(flash, SFUD_CHECK_BUSY_TIMEOUT);
    if (result != SFUD_SUCCESS) {
        if (spi->unlock)
            spi->unlock(spi);
        return result;
    }

    /* Default to use user's data buffer */
    copy_flag = 0;
    p = data;
    dolen = size;

    /* Try best to make the read transfer can use DMA */
    if ((size <= SFUD_WRITE_MAX_PAGE_SIZE) && (((unsigned long)data) & (CACHE_LINE_SIZE - 1))) {
        /*
         * Use internal page_data instead of user's data buffer:
         *  - Read data size is in one page
         *  - Data buffer address is not cache line aligned
         * When the reading is done, copy to user's data buffer.
         */
        p = page_data;
        copy_flag = 1;
        dolen = size;
    } else if ((size > SFUD_WRITE_MAX_PAGE_SIZE) && (((unsigned long)data) & (CACHE_LINE_SIZE - 1))) {
        /*
         * In this case:
         *  - Read data size is large than 1 page
         *  - Data buffer address is not cache line aligned
         * Let's split it to two transfer:
         *  - First transfer only read unaligned part
         *  - Second transfer read the rest with data address cache line aligned
         * In this case, no data copy
         */
        p = data;
        dolen = CACHE_LINE_SIZE - (((unsigned long)data) & (CACHE_LINE_SIZE - 1));
    }
    if (result == SFUD_SUCCESS) {
        do {
            if ((dolen & (CACHE_LINE_SIZE - 1)) && (dolen > SFUD_WRITE_MAX_PAGE_SIZE)) {
                dolen -= dolen & (CACHE_LINE_SIZE- 1);
            }

            result = sfud_read_data(flash, addr, dolen, p);
            if (result != SFUD_SUCCESS)
                break;
            if (copy_flag) {
                memcpy(data, p, dolen);
            }
            p += dolen;
            addr += dolen;
            size -= dolen;
            dolen = size;
        } while (size);
    }
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * erase all flash data
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_chip_erase(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[4];

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }

    cmd_data[0] = SFUD_CMD_ERASE_CHIP;
    /* dual-buffer write, like AT45DB series flash chip erase operate is different for other flash */
    if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER) {
        cmd_data[1] = 0x94;
        cmd_data[2] = 0x80;
        cmd_data[3] = 0x9A;
        result = spi->wr(spi, cmd_data, 4, NULL, 0);
    } else {
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    }
    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Flash chip erase SPI communicate error.");
        goto __exit;
    }
    result = wait_busy(flash, SFUD_CHIP_ERASE_TIMEOUT);

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

void choose_best_erase_cmd(const sfud_flash *flash, uint32_t addr, size_t size,
                            size_t *cur_erase_size, uint8_t *cur_erase_cmd)
{
    /* if this flash is support SFDP parameter, then used SFDP parameter supplies eraser */
#ifdef SFUD_USING_SFDP
    extern size_t sfud_sfdp_get_suitable_eraser(const sfud_flash *flash, uint32_t addr, size_t erase_size);

    size_t eraser_index;
    if (flash->sfdp.available) {
        /* get the suitable eraser for erase process from SFDP parameter */
        eraser_index = sfud_sfdp_get_suitable_eraser(flash, addr, size);
        *cur_erase_cmd = flash->sfdp.eraser[eraser_index].cmd;
        *cur_erase_size = flash->sfdp.eraser[eraser_index].size;
    } else {
#else
    {
#endif
        if (flash->chip.eraser[1].cmd > 0 && size >= flash->chip.eraser[1].size && !(addr % flash->chip.eraser[1].size)) {
            *cur_erase_cmd = flash->chip.eraser[1].cmd;
            *cur_erase_size = flash->chip.eraser[1].size;
        } else {
            *cur_erase_cmd = flash->chip.eraser[0].cmd;
            *cur_erase_size = flash->chip.eraser[0].size;
        }
    }
}

/**
 * erase flash data
 *
 * @note It will erase align by erase granularity.
 *
 * @param flash flash device
 * @param addr start address
 * @param size erase size
 *
 * @return result
 */
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size, cur_erase_cmd;
    size_t cur_erase_size;

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }

    if (addr == 0 && size == flash->chip.capacity) {
        return sfud_chip_erase(flash);
    }

    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* loop erase operate. erase unit is erase granularity */
    while (size) {
        choose_best_erase_cmd(flash, addr, size, &cur_erase_size, &cur_erase_cmd);
        /* set the flash write enable */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }

        cmd_data[0] = cur_erase_cmd;
        make_adress_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;
        result = spi->wr(spi, cmd_data, cmd_size, NULL, 0);
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash erase SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash, SFUD_SECTOR_ERASE_TIMEOUT);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        /* make erase align and calculate next erase address */
        if (addr % cur_erase_size != 0) {
            if (size > cur_erase_size - (addr % cur_erase_size)) {
                size -= cur_erase_size - (addr % cur_erase_size);
                addr += cur_erase_size - (addr % cur_erase_size);
            } else {
                goto __exit;
            }
        } else {
            if (size > cur_erase_size) {
                size -= cur_erase_size;
                addr += cur_erase_size;
            } else {
                goto __exit;
            }
        }
    }

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate) for write 1 to 256 bytes per page mode or byte write mode
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param write_gran write granularity bytes, only support 1 or 256
 * @param data write data
 *
 * @return result
 */
static sfud_err page256_or_1_byte_write(const sfud_flash *flash, uint32_t addr, size_t size, uint16_t write_gran,
        const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    /* Make the write buffer always cache line aligned */
    static uint8_t cmd_data[5 + SFUD_WRITE_MAX_PAGE_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));
    uint8_t cmd_size;
    size_t data_size;
    uint32_t bus_id;

    result = spi->get_bus_id(spi, &bus_id);
    if (result)
        return result;

    SFUD_ASSERT(flash);
    /* only support 1 or 256 */
    SFUD_ASSERT(write_gran == 1 || write_gran == 256);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* loop write operate. write unit is write granularity */
    while (size) {
        /* set the flash write enable */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        cmd_data[0] = SFUD_CMD_PAGE_PROGRAM;
        make_adress_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;

        /* make write align and calculate next write address */
        if (addr % write_gran != 0) {
            if (size > write_gran - (addr % write_gran)) {
                data_size = write_gran - (addr % write_gran);
            } else {
                data_size = size;
            }
        } else {
            if (size > write_gran) {
                data_size = write_gran;
            } else {
                data_size = size;
            }
        }
        size -= data_size;
        addr += data_size;

        memcpy(&cmd_data[cmd_size], data, data_size);

#if defined(AIC_SPIENC_DRV)
        spienc_set_cfg(bus_id, (addr - data_size), cmd_size, data_size);
        spienc_start();
#endif
        result = spi->wr(spi, cmd_data, cmd_size + data_size, NULL, 0);
#if defined(AIC_SPIENC_DRV)
        spienc_stop();
#endif
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash, SFUD_PAGE_PROGRAM_TIMEOUT);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        data += data_size;
    }

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate) for auto address increment mode
 *
 * If the address is odd number, it will place one 0xFF before the start of data for protect the old data.
 * If the latest remain size is 1, it will append one 0xFF at the end of data for protect the old data.
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
static sfud_err aai_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[8], cmd_size;
    bool first_write = true;
    uint32_t bus_id;

    result = spi->get_bus_id(spi, &bus_id);
    if (result)
        return result;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }
    /* The address must be even for AAI write mode. So it must write one byte first when address is odd. */
    if (addr % 2 != 0) {
        result = page256_or_1_byte_write(flash, addr++, 1, 1, data++);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        size--;
    }
    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }
    /* loop write operate. */
    cmd_data[0] = SFUD_CMD_AAI_WORD_PROGRAM;
    while (size >= 2) {
        if (first_write) {
            make_adress_byte_array(flash, addr, &cmd_data[1]);
            cmd_size = flash->addr_in_4_byte ? 5 : 4;
            cmd_data[cmd_size] = *data;
            cmd_data[cmd_size + 1] = *(data + 1);
            first_write = false;
        } else {
            cmd_size = 1;
            cmd_data[1] = *data;
            cmd_data[2] = *(data + 1);
        }

#if defined(AIC_SPIENC_DRV)
        spienc_set_cfg(bus_id, addr, cmd_size, size);
        spienc_start();
#endif
        result = spi->wr(spi, cmd_data, cmd_size + 2, NULL, 0);
#if defined(AIC_SPIENC_DRV)
        spienc_stop();
#endif
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }

        result = wait_busy(flash, SFUD_PAGE_PROGRAM_TIMEOUT);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }

        size -= 2;
        addr += 2;
        data += 2;
    }
    /* set the flash write disable for exit AAI mode */
    result = set_write_enabled(flash, false);
    /* write last one byte data when origin write size is odd */
    if (result == SFUD_SUCCESS && size == 1) {
        result = page256_or_1_byte_write(flash, addr, 1, 1, data);
    }

__exit:
    if (result != SFUD_SUCCESS) {
        set_write_enabled(flash, false);
    }
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate)
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;

    if (flash->chip.write_mode & SFUD_WM_PAGE_256B) {
        result = page256_or_1_byte_write(flash, addr, size, 256, data);
    } else if (flash->chip.write_mode & SFUD_WM_AAI) {
        result = aai_write(flash, addr, size, data);
    } else if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER) {
        //TODO dual-buffer write mode
        SFUD_INFO("Error: Dual-buffer write mode is not supported.");
        result = SFUD_ERR_WRITE;
    }

    return result;
}

/**
 * erase and write flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_erase_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;

    result = sfud_erase(flash, addr, size);

    if (result == SFUD_SUCCESS) {
        result = sfud_write(flash, addr, size, data);
    }

    return result;
}

static sfud_err reset(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_ENABLE_RESET;
    result = spi->wr(spi, cmd_data, 1, NULL, 0);
    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Flash device reset failed.");
        return result;
    }

    cmd_data[1] = SFUD_CMD_RESET;
    result = spi->wr(spi, &cmd_data[1], 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        SFUD_DEBUG("Flash device reset success.");
    } else {
        SFUD_INFO("Error: Flash device reset failed.");
    }

    aic_udelay(300);

    return result;
}

static sfud_err read_jedec_id(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[1], recv_data[3];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_JEDEC_ID;
    result = spi->wr(spi, cmd_data, sizeof(cmd_data), recv_data, sizeof(recv_data));
    if (result == SFUD_SUCCESS) {
        flash->chip.mf_id = recv_data[0];
        flash->chip.type_id = recv_data[1];
        flash->chip.capacity_id = recv_data[2];
        SFUD_DEBUG("The flash device manufacturer ID is 0x%02X, memory type ID is 0x%02X, capacity ID is 0x%02X.",
                flash->chip.mf_id, flash->chip.type_id, flash->chip.capacity_id);
    } else {
        SFUD_INFO("Error: Read flash device JEDEC ID error.");
    }

    if ((flash->chip.mf_id == 0xff && flash->chip.type_id == 0xff) || (flash->chip.mf_id == 0x00 && flash->chip.type_id == 0x00)) {
        SFUD_INFO("Error: flash device unsupport. ID: 0x%02X, 0x%02X, 0x%02X.",
                flash->chip.mf_id, flash->chip.type_id, flash->chip.capacity_id);
        return SFUD_ERR_NOT_FOUND;
    }

    return result;
}

/**
 * set the flash write enable or write disable
 *
 * @param flash flash device
 * @param enabled true: enable  false: disable
 *
 * @return result
 */
sfud_err set_write_enabled(const sfud_flash *flash, bool enabled) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd, register_status;

    SFUD_ASSERT(flash);

    if (enabled) {
        cmd = SFUD_CMD_WRITE_ENABLE;
    } else {
        cmd = SFUD_CMD_WRITE_DISABLE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        result = sfud_read_status(flash, &register_status);
        if (register_status & flash->bp_mask)
            SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", register_status);
    }

    if (result == SFUD_SUCCESS) {
        if (enabled && (register_status & SFUD_STATUS_REGISTER_WEL) == 0) {
            SFUD_INFO("Error: Can't enable write status.");
            return SFUD_ERR_WRITE;
        } else if (!enabled && (register_status & SFUD_STATUS_REGISTER_WEL) != 0) {
            SFUD_INFO("Error: Can't disable write status.");
            return SFUD_ERR_WRITE;
        }
    }

    return result;
}

/**
 * enable or disable 4-Byte addressing for flash
 *
 * @note The 4-Byte addressing just supported for the flash capacity which is large then 16MB (256Mb).
 *
 * @param flash flash device
 * @param enabled true: enable   false: disable
 *
 * @return result
 */
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd;

    SFUD_ASSERT(flash);

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        return result;
    }

    if (enabled) {
        cmd = SFUD_CMD_ENTER_4B_ADDRESS_MODE;
    } else {
        cmd = SFUD_CMD_EXIT_4B_ADDRESS_MODE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        flash->addr_in_4_byte = enabled ? true : false;
        SFUD_DEBUG("%s 4-Byte addressing mode success.", enabled ? "Enter" : "Exit");
    } else {
        SFUD_INFO("Error: %s 4-Byte addressing mode failed.", enabled ? "Enter" : "Exit");
    }

    return result;
}

/**
 * read flash register status
 *
 * @param flash flash device
 * @param status register status
 *
 * @return result
 */
sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *status) {
    uint8_t cmd = SFUD_CMD_READ_STATUS_REGISTER;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, &cmd, 1, status, 1);
}

sfud_err sfud_read_cr(const sfud_flash *flash, uint8_t *status) {
    uint8_t cmd = SFUD_CMD_READ_CONFIG_REGISTER;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, &cmd, 1, status, 1);
}

static sfud_err wait_busy(const sfud_flash *flash, size_t tmo_us) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t status;

    SFUD_ASSERT(flash);

    while (true) {
        result = sfud_read_status(flash, &status);
        if (result == SFUD_SUCCESS && ((status & SFUD_STATUS_REGISTER_BUSY)) == 0) {
            break;
        }
        if (status & flash->bp_mask)
            SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", status);
        /* retry counts */
        SFUD_RETRY_PROCESS(flash->retry.delay, tmo_us, result);
    }

    if (result != SFUD_SUCCESS || ((status & SFUD_STATUS_REGISTER_BUSY)) != 0) {
        SFUD_INFO("Error: Flash wait busy has an error.");
    }

    return result;
}

static int make_adress_byte_array(const sfud_flash *flash, uint32_t addr, uint8_t *array) {
    uint8_t len, i;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(array);

    len = flash->addr_in_4_byte ? 4 : 3;

    for (i = 0; i < len; i++) {
        array[i] = (addr >> ((len - (i + 1)) * 8)) & 0xFF;
    }

    return 0;
}

/**
 * write status register
 *
 * @param flash flash device
 * @param is_volatile true: volatile mode, false: non-volatile mode
 * @param status register status
 *
 * @return result
 */
sfud_err sfud_write_status(const sfud_flash *flash, bool is_volatile, uint8_t status) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    if (is_volatile) {
        cmd_data[0] = SFUD_VOLATILE_SR_WRITE_ENABLE;
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    } else {
        result = set_write_enabled(flash, true);
    }

    if (result == SFUD_SUCCESS) {
        cmd_data[0] = SFUD_CMD_WRITE_STATUS_REGISTER;
        cmd_data[1] = status;
        result = spi->wr(spi, cmd_data, 2, NULL, 0);
    }

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Write_status register failed.");
    }

    if (result == SFUD_SUCCESS)
        return wait_busy(flash, SFUD_WRITE_STATUS_TIMEOUT);
    else
        return result;
}

sfud_err sfud_write_status_ext(const sfud_flash *flash, bool is_volatile, uint8_t reg, uint8_t stsval) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    if (is_volatile) {
        cmd_data[0] = SFUD_VOLATILE_SR_WRITE_ENABLE;
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    } else {
        result = set_write_enabled(flash, true);
    }

    if (result == SFUD_SUCCESS) {
        switch (reg) {
            case 0x05:
                cmd_data[0] = SFUD_CMD_WRITE_STATUS_REGISTER;
                break;
            case 0x35:
                cmd_data[0] = SFUD_CMD_WRITE_STATUS2_REGISTER;
                break;
            case 0x15:
                cmd_data[0] = SFUD_CMD_WRITE_STATUS3_REGISTER;
                break;
            default:
                SFUD_INFO("Error: unsupport reg: 0x%x.", reg);
                return SFUD_ERR_NOT_FOUND;
        }
        cmd_data[1] = stsval;
        result = spi->wr(spi, cmd_data, 2, NULL, 0);
    }

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Write_status register failed.");
    }

    if (result == SFUD_SUCCESS)
        return wait_busy(flash, SFUD_WRITE_STATUS_TIMEOUT);
    else
        return result;
}

sfud_err sfud_write_status2(const sfud_flash *flash, uint8_t *status) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[3];

    SFUD_ASSERT(flash);

    result = set_write_enabled(flash, true);

    if (result == SFUD_SUCCESS) {
        cmd_data[0] = SFUD_CMD_WRITE_STATUS_REGISTER;
        cmd_data[1] = status[0];
        cmd_data[2] = status[1];
        result = spi->wr(spi, cmd_data, 3, NULL, 0);
    }

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Write_status register failed.");
    }

    if (result == SFUD_SUCCESS)
        return wait_busy(flash, SFUD_WRITE_STATUS_TIMEOUT);
    else
        return result;
}

sfud_err sfud_write_cr(const sfud_flash *flash, uint8_t *cr) {
    sfud_err result = SFUD_SUCCESS;

    result = sfud_write_reg(flash, SFUD_CMD_WRITE_STATUS2_REGISTER, cr);
    if (result == SFUD_SUCCESS)
        return wait_busy(flash, SFUD_WRITE_STATUS_TIMEOUT);
    else
        return result;
}

sfud_err sfud_write_reg(const sfud_flash *flash, uint8_t reg, uint8_t *status) {
    uint8_t cmd_data[2];
    cmd_data[0] = reg;
    cmd_data[1] = *status;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, cmd_data, 2, NULL, 0);
}

sfud_err sfud_read_reg(const sfud_flash *flash, uint8_t reg, uint8_t *status) {
    uint8_t cmd = reg;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, &cmd, 1, status, 1);
}

/**
 * makesure the spi work in the suitable delay mode
 *
 * @note
 *
 * @param flash flash device
 *
 * @return result
 */
static sfud_err auto_set_rx_delay_mode(sfud_flash *flash) {

    sfud_err result = SFUD_SUCCESS;
    uint8_t id[3] = {0};
    uint32_t mode = 0;

    SFUD_ASSERT(flash);

    id[0] = flash->chip.mf_id;
    id[1] = flash->chip.type_id;
    id[2] = flash->chip.capacity_id;

    for (mode = 3; mode > 0; mode--) {
        result = read_jedec_id(flash);
        if (result != SFUD_SUCCESS)
            return result;

        if ((id[0] == flash->chip.mf_id) && (id[1] == flash->chip.type_id) && (id[2] == flash->chip.capacity_id))
            return SFUD_SUCCESS;

        if (flash->spi.set_rx_delay)
            flash->spi.set_rx_delay(&flash->spi, mode);
    }

    return SFUD_ERR_NOT_FOUND;
}

#ifdef SFUD_USING_SECURITY_REGISTER
static void make_secur_adress_array(const sfud_flash *flash, uint32_t reg_num, uint8_t *array) {
    uint8_t len;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(array);

    len = flash->addr_in_4_byte ? 5 : 4;

    memset(array, 0x00, len);
    array[1] = (reg_num & 0x0F) << 4;
    array[len - 1] = SFUD_DUMMY_DATA;

}

/**
 * read flash Security Registers
 *
 * @param flash flash device
 * @param reg security registers num
 * @param data read data pointer
 *
 * @return result
 */
sfud_err sfud_read_secur(const sfud_flash *flash, uint8_t reg, uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[6], cmd_size;

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    result = wait_busy(flash, SFUD_CHECK_BUSY_TIMEOUT);
    if (result) {
        if (spi->unlock)
            spi->unlock(spi);
        return result;
    }

    cmd_data[0] = SFUD_CMD_READ_SECURITY_REGISTER;
    make_secur_adress_array(flash, reg, &cmd_data[1]);
    cmd_size = flash->addr_in_4_byte ? 6 : 5;
    result = spi->wr(spi, cmd_data, cmd_size, data, 256);

    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * erase security register data
 *
 * @param flash flash device
 * @param reg security registers num
 *
 * @return result
 */
sfud_err sfud_erase_secur(const sfud_flash *flash, uint8_t reg) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size;

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }

    cmd_data[0] = SFUD_CMD_ERASE_SECURITY_REGISTER;
    make_secur_adress_array(flash, reg, &cmd_data[1]);
    cmd_size = flash->addr_in_4_byte ? 5 : 4;
    result = spi->wr(spi, cmd_data, cmd_size, NULL, 0);
    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Security register %u erase error.", reg);
        goto __exit;
    }
    result = wait_busy(flash, SFUD_SECTOR_ERASE_TIMEOUT);

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash Security Registers
 *
 * @param flash flash device
 * @param reg ecurity registers num
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_write_secur(const sfud_flash *flash, uint8_t reg, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    /* Make the write buffer always cache line aligned */
    static uint8_t cmd_data[5 + SFUD_WRITE_MAX_PAGE_SIZE] __attribute__((aligned(CACHE_LINE_SIZE)));
    uint8_t cmd_size;

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (size > SFUD_WRITE_MAX_PAGE_SIZE) {
        SFUD_INFO("Error: Security Register write size out of bound.");
        return SFUD_ERR_NOT_FOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }
    cmd_data[0] = SFUD_CMD_SECURITY_REGISTER_PROGRAM;
    make_secur_adress_array(flash, reg, &cmd_data[1]);
    cmd_size = flash->addr_in_4_byte ? 5 : 4;

    memcpy(&cmd_data[cmd_size], data, size);

    result = spi->wr(spi, cmd_data, cmd_size + size, NULL, 0);

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Flash write SPI communicate error.");
        goto __exit;
    }
    result = wait_busy(flash, SFUD_PAGE_PROGRAM_TIMEOUT);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}
#endif // SFUD_USING_SECURITY_REGISTER

sfud_err sfud_read_unique_id(const sfud_flash *flash, uint8_t *send_bytes, uint8_t id_len, uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    static uint8_t cmd_data[5 + 16];

    SFUD_ASSERT(flash);

    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    memcpy(cmd_data, send_bytes, 5);
    result = spi->wr(spi, cmd_data, 5, cmd_data + 5, id_len);
    memcpy(data, &cmd_data[5], 8);

    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    if (!result)
        return id_len;

    return -1;
}
