/*
 * Copyright (C) 2024-2026 ArtInChip Technology Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <aic_core.h>
#include <aic_common.h>
#include <ram_param.h>

#define EFUSE_CMU_REG ((void *)0x18020904)
#define EFUSE_218_REG ((void *)0x19010218)
#define EFUSE_21C_REG ((void *)0x1901021c)

#define PSRAM_SINGLE   0
#define PSRAM_PARALLEL 1

struct _psram_id {
    u8 psram_fuse_id;
    u32 psram_chip_id;
};

struct _psram_info {
    u8 mark_id;
    u8 psram_num;
    u32 psram_size;
    struct _psram_id psram_id;
    char *chip_name;
};

enum psram_fuse {
    PSRAM_FUSE_APS3208K = 0x00,
    PSRAM_FUSE_APS12816O_OB9 = 0x02,	//
    PSRAM_FUSE_APS25616N_OBX9 = 0x03,	// 32M
    PSRAM_FUSE_APS3208K_X2 = 0x04,	// 4M * 2, APS3208K * 2
    PSRAM_FUSE_W957D6NKR = 0x05,	// 16M HyperRAM 3.1 x16
    PSRAM_FUSE_W956D8NKR_PARALLEL = 0x06, // 16M HyperRAM 2.1 x8 x2
    PSRAM_FUSE_APS6408L_PARALLEL = 0x07,	// 4M Xccela x8
    PSRAM_FUSE_W955D8MKY = 0x08,	// 4M HyperBus x8
    PSRAM_FUSE_W955D8MKY_PARALLEL = 0x09, // 8M HyperBus x8 x2
    PSRAM_FUSE_NO_PSRAM = 0x0F,		// no psram
};

enum psram_size {
    PSRAM_SIZE_4M = 0x400000,
    PSRAM_SIZE_8M = 0x800000,
    PSRAM_SIZE_16M = 0x1000000,
    PSRAM_SIZE_32M = 0x2000000,
};

#define PSRAM_TABLE_INFO								\
{											\
    /* force use the cfg0 */								\
    {0x0, 0, 0, {0, 0}},								\
    /* D125CBS */									\
    {0x01, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_APS3208K_X2, 0x80c980c9}, "D125CBS"},	\
    /* D125CCS */									\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D125CCS"},	\
    /* D125CCS-U */									\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D125CCS-U"},	\
    /* D125DDS */									\
    {0x03, PSRAM_PARALLEL, PSRAM_SIZE_32M, {PSRAM_FUSE_APS25616N_OBX9, 0xdf8ddf8d}, "D125DDS"}, 	\
    /* D125EDS */									\
    {0x04, PSRAM_PARALLEL, PSRAM_SIZE_32M, {PSRAM_FUSE_APS25616N_OBX9, 0xdf8ddf8d}, "D125EDS"},	\
    /* D126FBV */									\
    {0x05, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_APS3208K_X2, 0x80c980c9}, "D126FBV"},	\
    /* D126FCR */									\
    {0x06, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS12816O_OB9, 0xdd8ddd8d}, "D126FCR"},	\
    /* D126FBR */									\
    {0x07, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_APS3208K_X2, 0x80c980c9}, "D126FBR"},	\
    /* G720ADU  */									\
    {0x21, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_APS3208K, 0x80c980c9}, "G720ADU"},		\
    /* G720AFU */									\
    {0x22, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_APS3208K, 0x80c980c9}, "G720AFU"},		\
    /* G720BGU */									\
    {0x23, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_APS3208K, 0x80c980c9}, "G720BGU"},		\
    /* ---- Winbond HyperRAM/HyperBus, fuse value decides the chip ---- */			\
    /* D125CBS 8M W955D8MKY x8 x2 */								\
    {0x01, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_W955D8MKY_PARALLEL, 0x5f005f00}, "D125CBS-W"},	\
    /* D125CCS 16M W957D6NKR */								\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W957D6NKR, 0x0c860c86}, "D125CCS-W"},	\
    /* D125CCS 16M W956D8NKR x8 x2 */							\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W956D8NKR_PARALLEL, 0x860c860c}, "D125CCS-V"},	\
    /* D125CCS 16M APS6408L x8 x2 */							\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS6408L_PARALLEL, 0x8b8d8b8d}, "D125CCS-A"},	\
    /* D125CCS-U 16M W957D6NKR */							\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W957D6NKR, 0x0c860c86}, "D125CCS-UW"},	\
    /* D125CCS-U 16M W956D8NKR x8 x2 */							\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W956D8NKR_PARALLEL, 0x860c860c}, "D125CCS-UV"},	\
    /* D125CCS-U 16M APS6408L x8 x2 */							\
    {0x02, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS6408L_PARALLEL, 0x8b8d8b8d}, "D125CCS-UA"},	\
    /* D126FBV 8M W955D8MKY x8 x2 */							\
    {0x05, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_W955D8MKY_PARALLEL, 0x5f005f00}, "D126FBV-W"},	\
    /* D126FCR 16M W957D6NKR */								\
    {0x06, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W957D6NKR, 0x0c860c86}, "D126FCR-W"},	\
    /* D126FCR 16M W956D8NKR x8 x2 */							\
    {0x06, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_W956D8NKR_PARALLEL, 0x860c860c}, "D126FCR-V"},	\
    /* D126FCR 16M APS6408L x8 x2 */							\
    {0x06, PSRAM_PARALLEL, PSRAM_SIZE_16M, {PSRAM_FUSE_APS6408L_PARALLEL, 0x8b8d8b8d}, "D126FCR-A"},	\
    /* D126FBR 8M W955D8MKY x8 x2 */									\
    {0x07, PSRAM_PARALLEL, PSRAM_SIZE_8M, {PSRAM_FUSE_W955D8MKY_PARALLEL, 0x5f005f00}, "D126FBR-W"},	\
    /* G720ADU 4M W955D8MKY */								\
    {0x21, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_W955D8MKY, 0x5f005f00}, "G720ADU-W"},		\
    /* G720AFU 4M W955D8MKY */								\
    {0x22, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_W955D8MKY, 0x5f005f00}, "G720AFU-W"},		\
    /* G720BGU 4M W955D8MKY */									\
    {0x23, PSRAM_SINGLE, PSRAM_SIZE_4M, {PSRAM_FUSE_W955D8MKY, 0x5f005f00}, "G720BGU-W"},		\
}

struct _psram_info psram_table_info[] = PSRAM_TABLE_INFO;

u8 psram_get_mark_id(void)
{
    u32 fuse_218 = readl(EFUSE_218_REG);
    u8 mark_id = fuse_218 & 0xff;

    pr_debug("fuse_218(0x19010218)=0x%x, mark_id=0x%x\n", fuse_218, mark_id);
    return mark_id;
}

u8 psram_get_psram_id(void)
{
    u32 fuse_21c = readl(EFUSE_21C_REG);
    u8 psram_id = (fuse_21c & 0xff) >> 4;

    pr_debug("fuse_21c(0x1901021c)=0x%x, psram_id=0x%x\n", fuse_21c, psram_id);
    return psram_id;
}

struct _psram_info *psram_get_info(u8 mark_id, u8 psram_fuse_id)
{
    u32 len = ARRAY_SIZE(psram_table_info);

    for (int i = 0; i < len; i++) {
        if (((mark_id == psram_table_info[i].mark_id) &&
             (psram_fuse_id == psram_table_info[i].psram_id.psram_fuse_id))) {
            return &psram_table_info[i];
        }
    }
    pr_info("can't get the psram table, return the default info.\n");
    return &psram_table_info[0]; //if not find anyone, return the default info.
}

static struct _psram_info *psram_info = NULL;

u32 aic_get_ram_size(void)
{
    u8 mark_id, psram_id;

    if (psram_info)
        return psram_info->psram_size;

    writel(0x1100, EFUSE_CMU_REG);
    mark_id = psram_get_mark_id();
    psram_id = psram_get_psram_id();
    psram_info = psram_get_info(mark_id, psram_id);
    writel(0x0, EFUSE_CMU_REG);

    return psram_info->psram_size;
}

void aic_show_ram_size(void)
{
    /* Map psram_fuse_id to PSRAM chip name */
    static const char *const psram_ids[] = {
        [PSRAM_FUSE_APS3208K]             = "APS32",
        [PSRAM_FUSE_APS12816O_OB9]        = "APS128",
        [PSRAM_FUSE_APS25616N_OBX9]       = "APS256",
        [PSRAM_FUSE_APS3208K_X2]          = "APS32x2",
        [PSRAM_FUSE_W957D6NKR]            = "W957D6",
        [PSRAM_FUSE_W956D8NKR_PARALLEL]   = "W956D8x2",
        [PSRAM_FUSE_APS6408L_PARALLEL]    = "APS64x2",
        [PSRAM_FUSE_W955D8MKY]            = "W955D8",
        [PSRAM_FUSE_W955D8MKY_PARALLEL]   = "W955D8x2",
    };
    const char *psram_name = "Unknown";
    u32 fuse_id;

    if (!psram_info)
        aic_get_ram_size();

    fuse_id = psram_info->psram_id.psram_fuse_id;
    if ((fuse_id < ARRAY_SIZE(psram_ids)) && psram_ids[fuse_id])
        psram_name = psram_ids[fuse_id];

    printf("PSRAM: %s, size %d MB\n",
           psram_name, psram_info->psram_size / 1024 / 1024);
}
