/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Jianfeng Li <jianfeng.li@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <upg_internal.h>
#include <aic_core.h>
#include <aicupg.h>
#include <mtd.h>
#include <aic_utils.h>
#include <upg_fat_direct.h>

int aicupg_fat_direct_write(char *dst_type, u32 intf_id, char *fpath,
                            u32 doffset, u32 boot_flag, char *attr)
{
    pr_debug("Write %s %d: %s to offset 0x%x\n", dst_type, intf_id, fpath, doffset);
    if (boot_flag)
        pr_debug("\t It is a bootloader to BROM.\n");

    if (dst_type == NULL || fpath == NULL) {
        pr_err("Invalid parameter.\n");
        return -1;
    }
    if (strcmp(dst_type, "spi-nand") == 0) {
#if defined(AICUPG_NAND_ARTINCHIP)
        return fat_direct_write_spinand(intf_id, fpath, doffset, boot_flag,
                                        attr);
#endif
    } else if (strcmp(dst_type, "spi-nor") == 0) {
#if defined(AICUPG_NOR_ARTINCHIP)
        return fat_direct_write_spinor(intf_id, fpath, doffset);
#endif
    } else if (strcmp(dst_type, "mmc") == 0) {
#if defined(AICUPG_MMC_ARTINCHIP)
        if (boot_flag) {
            if ((doffset != 0x4400) && (doffset != 0x24400) &&
                (doffset != 0x44400)) {
                doffset = 0x4400;
            }
        }
        return fat_direct_write_mmc(intf_id, fpath, doffset, attr);
#endif
    } else {
        pr_err("Unknown storage type: %s\n", dst_type);
        return -1;
    }

    return 0;
}
