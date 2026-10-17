/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wudehuang <dehuang.wu@artinchip.com>
 */

#ifndef __BL_MTD_H_
#define __BL_MTD_H_

#include <aic_common.h>
#include <aic_list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAND_NAME 32
struct nftl_volume {
    char name[MAX_NAND_NAME];
    int vol_type;
    u32 size;
    struct nftl_volume *next;
};

struct nftl_mtd {
    char name[MAX_NAND_NAME];
    struct nftl_mtd *next;
    struct nftl_volume *vols;
};

enum part_attr {
    PART_ATTR_MTD = 0,
    PART_ATTR_NFTL,
    PART_ATTR_LEVELX,
};

#define MAX_MTD_NAME 64
struct mtd_partition {
    char name[MAX_MTD_NAME];
    u32 start;
    u32 size;
    struct mtd_partition *next;
    enum part_attr attr;
};

struct mtd_dev;
struct mtd_drv_ops {
    int (*read)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
    int (*erase)(struct mtd_dev *mtd, u32 offset, u32 len);
    int (*write)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
    int (*read_oob)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                    u8 *spare_data, u32 spare_len);
    int (*write_oob)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                     u8 *spare_data, u32 spare_len);
    int (*block_isbad)(struct mtd_dev *mtd, u32 offset);
    int (*block_markbad)(struct mtd_dev *mtd, u32 offset);
    int (*cont_read)(struct mtd_dev *mtd, u32 offset, u8 *data, u32 size);
    int (*map_user)(struct mtd_dev *mtd, u8 *oobbuf, u8 *buf, int start, int nbytes);
    int (*unmap_user)(struct mtd_dev *mtd, u8 *dst, u8* src, int start, int nbytes);
    u32 (*la2pa)(struct mtd_dev *mtd, u32 offset);
};

struct mtd_dev {
    struct list_head list;
    char *name;
    unsigned long start;
    unsigned long size;
    unsigned long erasesize;
    unsigned long writesize;
    unsigned long oobsize;
    struct mtd_drv_ops ops;
    void *priv;
    enum part_attr attr;
};

int mtd_probe(void);
int mtd_add_device(struct mtd_dev *mtd);
int mtd_del_device(struct mtd_dev *mtd);
u32 mtd_get_device_count(void);
struct mtd_dev *mtd_get_device_by_id(u32 id);
struct mtd_dev *mtd_get_device(const char *name);
int mtd_read(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
int mtd_erase(struct mtd_dev *mtd, u32 offset, u32 len);
int mtd_write(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
int mtd_read_oob(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                 u8 *spare_data, u32 spare_len);
int mtd_write_oob(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len,
                  u8 *spare_data, u32 spare_len);
int mtd_block_isbad(struct mtd_dev *mtd, u32 offset);
int mtd_block_markbad(struct mtd_dev *mtd, u32 offset);
struct mtd_partition *mtd_parts_parse(char *parts, u32 spi_bus);
void mtd_parts_free(struct mtd_partition *head);
int mtd_contread(struct mtd_dev *mtd, u32 offset, u8 *data, u32 len);
int mtd_unmap_oob_user_region(struct mtd_dev *mtd, u8 *dst, u8* src, int start, int nbytes);
int mtd_map_oob_user_region(struct mtd_dev *mtd, u8 *oobbuf, u8 *buf, int start, int nbytes);
u32 mtd_trans_lga_to_pha(struct mtd_dev *mtd, u32 offset);

struct nftl_mtd *build_nftl_list(char *nftlvols);
void free_nftl_list(struct nftl_mtd *nftl);
u8 partition_nftl_is_exist(char *mtd_name, struct nftl_mtd *nftl_list);
u8 partition_levelx_is_exist(char *mtd_name, char *levelx_partstr);

#ifdef __cplusplus
}
#endif

#endif /* __BL_MTD_H_ */
