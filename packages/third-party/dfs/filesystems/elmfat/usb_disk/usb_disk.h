/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSD_DISKIO_H_
#define _MSD_DISKIO_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define usb_disk_status(pdrv)                           USB_disk_status()
#define usb_disk_initialize(pdrv)                       USB_disk_initialize()
#define usb_disk_read(pdrv, var1, var2, var3)           USB_disk_read(var1, var2, var3)
#define usb_disk_write(pdrv, var1, var2, var3)          USB_disk_write(var1, var2, var3)
#define usb_disk_ioctl(pdrv, var1, var2)                USB_disk_ioctl(var1, var2)

/*******************************************************************************
 * API
 ******************************************************************************/

int USB_disk_status(void);
int USB_disk_initialize(void);
int USB_disk_read(BYTE *buff, LBA_t sector, UINT count);
int USB_disk_write(const BYTE *buff, LBA_t sector, UINT count);
int USB_disk_ioctl(BYTE cmd, void *buff);


#endif /* _MSD_DISKIO_H_ */

