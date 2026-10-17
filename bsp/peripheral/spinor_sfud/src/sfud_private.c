/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../inc/sfud.h"
#include <string.h>
#include "sfud_flash_def.h"
#include <stdlib.h>

static const sfud_qspi_flash_private_info qspi_flash_private_info_table[] = SFUD_FLASH_PRIVATE_INFO_TABLE;

int spi_nor_get_unique_id(sfud_flash *flash, uint8_t *data)
{
    uint8_t id_len = 16;
    int ret = 0;
    uint8_t *send_byte;

    send_byte = malloc(5 * sizeof(uint8_t));
    if (!send_byte) {
        SFUD_INFO("Malloc failed!\n");
        return -1;
    }

    /*
     * Default situation of readding Unique ID Number.
     * SI byte0: instruction 0x4b
     * SI byte1 - byte4: 4 bytes dummy
     * SO 128-bit Unique Serial Number
     * */
    memset(send_byte, 0xff, 5 * sizeof(uint8_t));
    send_byte[0] = 0x4b;

    for (int i = 0;
        i < sizeof(qspi_flash_private_info_table) / sizeof(sfud_qspi_flash_private_info); i++) {
       if ((qspi_flash_private_info_table[i].mf_id == flash->chip.mf_id) &&
           (qspi_flash_private_info_table[i].type_id == flash->chip.type_id) &&
           (qspi_flash_private_info_table[i].capacity_id ==
            flash->chip.capacity_id)) {
            send_byte[0] = qspi_flash_private_info_table[i].rd_cmd;
            send_byte[1] = qspi_flash_private_info_table[i].byte1;
            send_byte[2] = qspi_flash_private_info_table[i].byte2;
            send_byte[3] = qspi_flash_private_info_table[i].byte3;
            send_byte[4] = qspi_flash_private_info_table[i].byte4;
            id_len = qspi_flash_private_info_table[i].id_len;
            break;
       }
   }

    ret = sfud_read_unique_id(flash, send_byte, id_len, data);
    free (send_byte);

    return ret;
}

#define SR_BP0		(1 << 2)	/* Block protect 0 */
#define SR_BP1		(1 << 3)	/* Block protect 1 */
#define SR_BP2		(1 << 4)	/* Block protect 2 */
#define SR_BP3		(1 << 5)	/* Block protect 3 */
#define SR_BP4  	(1 << 6)	/* Block protect 3 */

static uint8_t spi_nor_get_sr_bp_mask(sfud_flash *flash)
{
    uint8_t mask = SR_BP0 | SR_BP1 | SR_BP2;

    for (int i = 0;
        i < sizeof(qspi_flash_private_info_table) / sizeof(sfud_qspi_flash_private_info); i++) {
       if ((qspi_flash_private_info_table[i].mf_id == flash->chip.mf_id) &&
           (qspi_flash_private_info_table[i].type_id == flash->chip.type_id) &&
           (qspi_flash_private_info_table[i].capacity_id ==
            flash->chip.capacity_id)) {
            flash->bp_mask = qspi_flash_private_info_table[i].bp_mask;
            break;
       }
   }

    if (flash->bp_mask & SNOR_F_HAS_SR_BP3_BIT6)
        return mask | SR_BP4 | SR_BP3;

    if (flash->bp_mask & SNOR_F_HAS_4BIT_BP)
        return mask | SR_BP3;

    return mask;
}

static int spi_nor_write_sr1_and_check(sfud_flash *flash, uint8_t status)
{
    int ret;
    uint8_t status_rd;

    ret = sfud_write_status(flash, false, status);
    if (ret)
        return ret;

    ret = sfud_read_status(flash, &status_rd);
    if (ret)
        return ret;

    if (status_rd != status) {
        SFUD_INFO("SR1: read back test failed\n");
        return -1;
    }

    return 0;
}

#define SR2_QUAD_EN_BIT1    (1 << 1)
static int spi_nor_write_16bit_sr_and_check(sfud_flash *flash, uint8_t status)
{
    int ret;
    uint8_t sr_cr[2], cr_written;

    sr_cr[0] = status;

    /* Make sure we don't overwrite the contents of Status Register 2. */
	if (!(flash->flags & SNOR_F_NO_READ_CR)) {
		ret = sfud_read_cr(flash, &sr_cr[1]);
		if (ret)
			return ret;
    } else if (flash->quad_enable) {
        sr_cr[1] = SR2_QUAD_EN_BIT1;
    } else {
        sr_cr[1] = 0;
    }

    ret = sfud_write_status2(flash, sr_cr);
    if (ret)
		return ret;

    cr_written = sr_cr[1];

    sfud_read_status(flash, &sr_cr[0]);
    if (sr_cr[0] != status) {
        SFUD_INFO("SR1: read back test failed\n");
        return -1;
    }

    if (flash->flags & SNOR_F_NO_READ_CR)
        return 0;

    sfud_read_cr(flash, &sr_cr[1]);
    if (sr_cr[1] != cr_written) {
        SFUD_INFO("CR: read back test failed\n");
        return -1;
    }

    return 0;
}

static int spi_nor_write_sr_and_check(sfud_flash *flash, uint8_t status)
{
#ifdef SFUD_USING_QSPI
    if (flash->flags & SNOR_F_HAS_16BIT_SR)
        return spi_nor_write_16bit_sr_and_check(flash, status);
#endif
    return spi_nor_write_sr1_and_check(flash, status);
}

/*
 * Unlock all of blocks of the flash.
 *
 * Returns negative on errors, 0 on success.
 */
int spi_nor_sr_unlock_all(sfud_flash *flash)
{
    uint8_t status, bp_mask;

    bp_mask = spi_nor_get_sr_bp_mask(flash);
    flash->bp_mask = bp_mask;

    sfud_read_status(flash, &status);

    /* If nothing in flash is locked, we don't need to do anything */
    if (!(status & bp_mask))
        return 0;

    SFUD_WP_INFO("Flash is in write protection state: 0x%x.\n", status);

    status &= ~bp_mask;

    return spi_nor_write_sr_and_check(flash, status);
}
