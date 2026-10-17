/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../inc/sfud.h"
#include <string.h>
#include "sfud_flash_def.h"
#include "sfud_qe.h"

#ifdef SFUD_USING_QSPI

/* Status Register 2 bits. */
#define SR2_QUAD_EN_BIT1    (1 << 1)
#define SR1_QUAD_EN_BIT6    (1 << 6)
#define SR2_QUAD_EN_BIT7    (1 << 7)


static const sfud_qspi_flash_qe_info qspi_flash_qe_info_table[] = SFUD_FLASH_QE_INFO_TABLE;

#define SR1_RW_MSK 0xFC
static int spi_nor_write_16bit_cr_and_check(sfud_flash *flash, uint8_t cr)
{
    int ret;
    uint8_t sr_cr[2];
    uint8_t sr_written;

    /* Keep the current value of the Status Register 1. */
    ret = sfud_read_status(flash, sr_cr);
    if (ret)
        return ret;
    if (sr_cr[0] & flash->bp_mask)
            SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", sr_cr[0]);

    sr_cr[1] = cr;

    ret = sfud_write_status2(flash, sr_cr);
    if (ret)
        return ret;

    sr_written = sr_cr[0] & SR1_RW_MSK;

    ret = sfud_read_status(flash, sr_cr);
    if (ret)
        return ret;
    if (sr_cr[0] & flash->bp_mask)
            SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", sr_cr[0]);

    /* Only check the writable bits */
    if (sr_written != (sr_cr[0] & SR1_RW_MSK)) {
        SFUD_INFO("SR: Read back test failed\n");
        return -1;
    }

    if (flash->flags & SNOR_F_NO_READ_CR)
        return 0;

    ret = sfud_read_cr(flash, &sr_cr[1]);
    if (ret)
        return ret;

    if (cr != sr_cr[1]) {
        /* write again, some flash can not support write 16bit base register1
         * addr.
         */
        sfud_write_cr(flash, &cr);
        sfud_read_cr(flash, &sr_cr[1]);
        if (cr != sr_cr[1]) {
            SFUD_INFO("CR: read back test failed\n");
            return -1;
        }
    }

    return 0;
}

static int spi_nor_write_sr1_and_check(sfud_flash *flash, uint8_t sr1)
{
    int ret;
    uint8_t status;

    ret = sfud_write_status(flash, true, sr1);
    if (ret)
        return ret;

    ret = sfud_read_status(flash, &status);
    if (ret)
        return ret;
    if (status & flash->bp_mask)
            SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", status);

    if (status!= sr1) {
        SFUD_INFO("SR1: read back test failed\n");
        return -1;
    }

    return 0;
}

/**
 * spi_nor_sr1_bit6_quad_enable() - Set the Quad Enable BIT(6) in the Status
 * Register 1.
 *
 * Bit 6 of the Status Register 1 is the QE bit for Macronix like QSPI memories.
 *
 * Return: 0 on success, -errno otherwise.
 */
int spi_nor_sr1_bit6_quad_enable(sfud_flash *flash)
{
    int ret;
    uint8_t sr;

    ret = sfud_read_status(flash, &sr);
    if (ret)
        return ret;

    if (sr & SR1_QUAD_EN_BIT6)
        return 0;

    sr |= SR1_QUAD_EN_BIT6;

    return spi_nor_write_sr1_and_check(flash, sr);
}

/**
 * spi_nor_sr2_bit1_quad_enable() - set the Quad Enable BIT(1) in the Status
 * Register 2.
 * @nor:       pointer to a 'struct spi_nor'.
 *
 * Bit 1 of the Status Register 2 is the QE bit for Spansion like QSPI memories.
 *
 * Return: 0 on success, -errno otherwise.
 */
int spi_nor_sr2_bit1_quad_enable(sfud_flash *flash)
{
    int ret;
    uint8_t buf[2];

    if (flash->flags & SNOR_F_NO_READ_CR)
        return spi_nor_write_16bit_cr_and_check(flash, SR2_QUAD_EN_BIT1);

    ret = sfud_read_cr(flash, buf);
    if (ret)
        return ret;

    if (buf[0] & SR2_QUAD_EN_BIT1)
        return 0;

    buf[0] |= SR2_QUAD_EN_BIT1;

    return spi_nor_write_16bit_cr_and_check(flash, buf[0]);
}

extern sfud_err set_write_enabled(const sfud_flash *flash, bool enabled);
#define QUAD_EN_BIT(offset) 1UL << offset
int spi_nor_quad_enable(sfud_flash *flash)
{
    uint8_t buf_data = 0;
    uint8_t wr_reg_status = 0;
    uint8_t rd_reg_status = 0;
    uint8_t qe_bit = 0;

    /* get wr_reg_status, rd_reg_status and qe_bit, If don't found, the default is compatible sr2_bit1 or SFDP */
    for (int i = 0;
         i < sizeof(qspi_flash_qe_info_table) / sizeof(sfud_qspi_flash_qe_info);
         i++) {
        if ((qspi_flash_qe_info_table[i].mf_id == flash->chip.mf_id) &&
            (qspi_flash_qe_info_table[i].type_id == flash->chip.type_id) &&
            (qspi_flash_qe_info_table[i].capacity_id ==
             flash->chip.capacity_id)) {
            wr_reg_status = qspi_flash_qe_info_table[i].wr_reg_status;
            rd_reg_status = qspi_flash_qe_info_table[i].rd_reg_status;
            qe_bit = qspi_flash_qe_info_table[i].qe_bit;
            goto qe_table;
        }
    }
    goto sr2_bit1;

qe_table:
    set_write_enabled(flash, true);
    sfud_read_reg(flash, rd_reg_status, &buf_data);
    buf_data |= QUAD_EN_BIT(qe_bit);
    sfud_write_reg(flash, wr_reg_status, &buf_data);

    sfud_read_reg(flash, rd_reg_status, &buf_data);
    if (buf_data & QUAD_EN_BIT(qe_bit)) {
        return 0;
    }
    return 1;

sr2_bit1:
    spi_nor_sr2_bit1_quad_enable(flash);
    return 0;
}
#endif
