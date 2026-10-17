/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#ifndef _SDMC_DISK_H_
#define _SDMC_DISK_H_

#include <stdint.h>
#include <ff.h>
#include "diskio.h"
#include "mmc.h"

/*!
 * @addtogroup SDMC Disk
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*************************************************************************************************
 * API
 ************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name SDMC Disk Function
 * @{
 */

/*!
 * @brief Initializes SDMC disk.
 *
 * @param device_name the name of device which includes a file system.
 * @retval disk handle.
 */
void *sdmc_disk_initialize(const char *device_name);

/*!
 * Gets SDMC disk status
 *
 * @param hdisk disk handle.
 * @retval STA_NOINIT Failed.
 * @retval RES_OK Success.
 */
DSTATUS sdmc_disk_status(void *hdisk);

/*!
 * @brief Reads SDMC disk.
 *
 * @param hdisk disk handle.
 * @param buf The data buffer pointer to store read content.
 * @param sector The start sector number to be read.
 * @param cnt The sector count to be read.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_read(void *hdisk, uint8_t *buf, uint32_t sector, rt_size_t cnt);

/*!
 * @brief Writes SDMC disk.
 *
 * @param hdisk disk handle.
 * @param buf The data buffer pointer to store write content.
 * @param sector The start sector number to be written.
 * @param cnt The sector count to be written.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_write(void *hdisk, const uint8_t *buf, uint32_t sector, rt_size_t cnt);

/*!
 * @brief SDMC disk IO operation.
 *
 * @param hdisk disk handle.
 * @param command The command to be set.
 * @param buf The buffer to store command result.
 * @retval RES_PARERR Failed.
 * @retval RES_OK Success.
 */
DRESULT sdmc_disk_ioctl(void *hdisk, uint8_t command, void *buf);

/* @} */
#if defined(__cplusplus)
}
#endif

#endif /* _SDMC_DISK_H_ */
