/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#ifndef _AIC_DISK_PART_H_
#define _AIC_DISK_PART_H_

#include <aic_common.h>
#include <aic_core.h>
#include <aic_partition.h>

#define EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)                    \
    {                                                                        \
        {                                                                    \
            (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff,               \
                ((a) >> 24) & 0xff, (b)&0xff, ((b) >> 8) & 0xff, (c)&0xff,   \
                ((c) >> 8) & 0xff, (d0), (d1), (d2), (d3), (d4), (d5), (d6), \
                (d7)                                                         \
        }                                                                    \
    }

#define DISK_DEFAULT_GUID \
	EFI_GUID( 0xC09F9B8E, 0xAC16, 0x442B, \
		0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xB0)

#define PARTITION_BASIC_DATA_GUID \
	EFI_GUID( 0xEBD0A0A2, 0xB9E5, 0x4433, \
		0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC0)

#define MSDOS_MBR_SIGNATURE 0xAA55
#define MSDOS_MBR_BOOT_CODE_SIZE 440
#define EFI_PMBR_OSTYPE_EFI 0xEF
#define EFI_PMBR_OSTYPE_EFI_GPT 0xEE

#define GPT_HEADER_SIGNATURE_EFI 0x5452415020494645ULL // 'EFI PART'

#define GPT_HEADER_REVISION_V1 0x00010000
#define GPT_PRIMARY_PARTITION_TABLE_LBA 1ULL
#define GPT_ENTRY_NUMBERS		128
#define GPT_ENTRY_SIZE			128

typedef struct {
    u8 b[16];
} efi_guid_t __attribute__((aligned(8)));

struct partition {
	u8 boot_ind;		/* 0x80 - active */
	u8 head;		/* starting head */
	u8 sector;		/* starting sector */
	u8 cyl;			/* starting cylinder */
	u8 sys_ind;		/* What partition type */
	u8 end_head;		/* end head */
	u8 end_sector;		/* end sector */
	u8 end_cyl;		/* end cylinder */
	u32 start_sect;	/* starting sector counting from 0 */
	u32 nr_sects;	/* nr of sectors in partition */
} __packed;

typedef struct _gpt_header {
	u64 signature;
	u32 revision;
	u32 header_size;
	u32 header_crc32;
	u32 reserved1;
	u64 my_lba;
	u64 alternate_lba;
	u64 first_usable_lba;
	u64 last_usable_lba;
	efi_guid_t disk_guid;
	u64 partition_entry_lba;
	u32 num_partition_entries;
	u32 sizeof_partition_entry;
	u32 partition_entry_array_crc32;
} __packed gpt_header;

typedef union _gpt_entry_attributes {
	struct {
		u64 required_to_function:1;
		u64 no_block_io_protocol:1;
		u64 legacy_bios_bootable:1;
		u64 reserved:45;
		u64 type_guid_specific:16;
	} fields;
	unsigned long long raw;
} __packed gpt_entry_attributes;

#define PARTNAME_SZ	(72 / sizeof(u16))
typedef struct _gpt_entry {
	efi_guid_t partition_type_guid;
	efi_guid_t unique_partition_guid;
	u64 starting_lba;
	u64 ending_lba;
	gpt_entry_attributes attributes;
	u16 partition_name[PARTNAME_SZ];
} __packed gpt_entry;

typedef struct _legacy_mbr {
	u8 boot_code[MSDOS_MBR_BOOT_CODE_SIZE];
	u32 unique_mbr_signature;
	u16 unknown;
	struct partition partition_record[4];
	u16 signature;
} __packed legacy_mbr;


struct blk_desc {
    u64 lba_count;
    unsigned long blksz;
    void *priv;
};

struct disk_blk_ops {
    unsigned long (*blk_write)(struct blk_desc *block_dev, u64 start,
                               u64 blkcnt, void *buffer);
    unsigned long (*blk_read)(struct blk_desc *block_dev, u64 start, u64 blkcnt,
                              const void *buffer);
};

enum disk_part_type {
    DISK_PART_MBR = 0,
    DISK_PART_GPT,
};

void aic_disk_part_set_ops(struct disk_blk_ops *ops);
int aic_disk_write_gpt(struct blk_desc *dev_desc, struct aic_partition *parts);
int aic_disk_get_part_type(struct blk_desc *dev_desc);
void aic_disk_dump_parts(struct blk_desc *dev_desc);
struct aic_partition *aic_disk_get_gpt_parts(struct blk_desc *dev_desc);
struct aic_partition *aic_disk_get_mbr_parts(struct blk_desc *dev_desc);
struct aic_partition *aic_disk_get_parts(struct blk_desc *dev_desc);

#endif	/* _AIC_DISK_PART_H_ */
