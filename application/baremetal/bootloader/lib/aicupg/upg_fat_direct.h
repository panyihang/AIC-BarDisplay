/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _UPG_FAT_DIRECT_H_
#define _UPG_FAT_DIRECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INTERFACE_NUM 4
#define MAX_WRITE_SIZE    (64 * 1024)
#define FRAME_LIST_SIZE   4096
#define MAX_SPL_SIZE      (256 * 1024)

int fat_direct_write_mmc(u32 sdmc_id, char *fpath, u32 doffset, char *attr);
int fat_direct_write_spinor(u32 spi_id, char *fpath, u32 doffset);
int fat_direct_write_spinand(u32 spi_id, char *fpath, u32 doffset,
                             u32 boot_flag, char *attr);

#ifdef __cplusplus
}
#endif

#endif /* _UPG_FAT_DIRECT_H_ */

