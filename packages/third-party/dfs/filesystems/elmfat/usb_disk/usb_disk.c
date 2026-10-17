/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"

__WEAK int USB_disk_status(void)
{
    return 0;
}
__WEAK int USB_disk_initialize(void)
{
    return 0;
}
__WEAK int USB_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    return 0;
}
__WEAK int USB_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    return 0;
}
__WEAK int USB_disk_ioctl(BYTE cmd, void *buff)
{
    return 0;
}

