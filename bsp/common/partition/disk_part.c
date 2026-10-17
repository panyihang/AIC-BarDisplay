/*
 * Copyright (c) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dehuang Wu <dehuang.wu@artinchip.com>
 */

#include <aic_common.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <aic_partition.h>
#include <disk_part.h>
#include <aic_crc32.h>

static const efi_guid_t aic_disk_guid = DISK_DEFAULT_GUID;
static const efi_guid_t partition_basic_data_guid = PARTITION_BASIC_DATA_GUID;
static struct disk_blk_ops blk_ops = { 0, 0 };

void aic_disk_part_set_ops(struct disk_blk_ops *ops)
{
    blk_ops.blk_write = ops->blk_write;
    blk_ops.blk_read = ops->blk_read;
}

static unsigned long blk_dwrite(struct blk_desc *dev, u64 start, u64 blkcnt,
                                void *buf)
{
    if (blk_ops.blk_write)
        return blk_ops.blk_write(dev, start, blkcnt, buf);
    return 0;
}

static unsigned long blk_dread(struct blk_desc *dev, u64 start, u64 blkcnt,
                               const void *buf)
{
    if (blk_ops.blk_read)
        return blk_ops.blk_read(dev, start, blkcnt, buf);
    return 0;
}

/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 */
static inline u32 efi_crc32(const void *buf, u32 len)
{
    return crc32(0, buf, len);
}

/*
 * Private function prototypes
 */

int pmbr_part_valid(struct partition *part);
int is_pmbr_valid(legacy_mbr *mbr);
int is_gpt_valid(struct blk_desc *dev_desc, u64 lba, gpt_header *pgpt_head,
                 gpt_entry **pgpt_pte);
gpt_entry *alloc_read_gpt_entries(struct blk_desc *dev_desc,
                                  gpt_header *pgpt_head);
int is_pte_valid(gpt_entry *pte);
int find_valid_gpt(struct blk_desc *dev_desc, gpt_header *gpt_head,
                   gpt_entry **pgpt_pte);

static int validate_gpt_header(gpt_header *gpt_h, u64 lba, u64 lastlba)
{
    u32 crc32_backup = 0;
    u32 calc_crc32;

    /* Check the GPT header signature */
    if (gpt_h->signature != GPT_HEADER_SIGNATURE_EFI) {
        pr_debug("%s signature is wrong: 0x%llX != 0x%llX\n",
               "GUID Partition Table Header", gpt_h->signature,
               GPT_HEADER_SIGNATURE_EFI);
        return -1;
    }

    /* Check the GUID Partition Table CRC */
    memcpy(&crc32_backup, &gpt_h->header_crc32, sizeof(crc32_backup));
    memset(&gpt_h->header_crc32, 0, sizeof(gpt_h->header_crc32));

    calc_crc32 = efi_crc32((const unsigned char *)gpt_h, (gpt_h->header_size));

    memcpy(&gpt_h->header_crc32, &crc32_backup, sizeof(crc32_backup));

    if (calc_crc32 != (crc32_backup)) {
        pr_debug("%s CRC is wrong: 0x%x != 0x%x\n", "GUID Partition Table Header",
               (crc32_backup), calc_crc32);
        return -1;
    }

    /*
	 * Check that the my_lba entry points to the LBA that contains the GPT
	 */
    if (gpt_h->my_lba != lba) {
        pr_debug("GPT: my_lba incorrect: %llX != %llX\n", gpt_h->my_lba, lba);
        return -1;
    }

    /*
	 * Check that the first_usable_lba and that the last_usable_lba are
	 * within the disk.
	 */
    if (lastlba != 0 && gpt_h->first_usable_lba > lastlba) {
        pr_debug("GPT: first_usable_lba incorrect: %llX > %llX\n",
               gpt_h->first_usable_lba, lastlba);
        return -1;
    }
    if (lastlba != 0 && gpt_h->last_usable_lba > lastlba) {
        pr_debug("GPT: last_usable_lba incorrect: %llX > %llX\n",
               gpt_h->last_usable_lba, lastlba);
        return -1;
    }

    pr_debug(
        "GPT: first_usable_lba: %llX last_usable_lba: %llX last lba: %llX n",
        gpt_h->first_usable_lba, gpt_h->last_usable_lba, lastlba);

    return 0;
}

static int validate_gpt_entries(gpt_header *gpt_h, gpt_entry *gpt_e)
{
    u32 calc_crc32;

    /* Check the GUID Partition Table Entry Array CRC */
    calc_crc32 = efi_crc32((const unsigned char *)gpt_e,
                           (gpt_h->num_partition_entries) *
                               (gpt_h->sizeof_partition_entry));

    if (calc_crc32 != (gpt_h->partition_entry_array_crc32)) {
        pr_debug("%s: 0x%x != 0x%x\n",
               "GUID Partition Table Entry Array CRC is wrong",
               (gpt_h->partition_entry_array_crc32), calc_crc32);
        return -1;
    }

    return 0;
}

static void prepare_backup_gpt_header(gpt_header *gpt_h)
{
    u32 calc_crc32;
    u64 val;

    /* recalculate the values for the Backup GPT Header */
    val = gpt_h->my_lba;
    gpt_h->my_lba = gpt_h->alternate_lba;
    gpt_h->alternate_lba = (val);
    gpt_h->partition_entry_lba = gpt_h->last_usable_lba + 1;
    gpt_h->header_crc32 = 0;

    calc_crc32 = efi_crc32((const unsigned char *)gpt_h, (gpt_h->header_size));
    gpt_h->header_crc32 = (calc_crc32);
}

#define in_range(c, lo, up) ((u8)c >= lo && (u8)c <= up)
static char *print_efiname(gpt_entry *pte)
{
    static char name[PARTNAME_SZ + 1];
    int i;
    for (i = 0; i < PARTNAME_SZ; i++) {
        u8 c;
        c = pte->partition_name[i] & 0xff;
        c = (c && !isprint(c)) ? '.' : c;
        name[i] = c;
    }
    name[PARTNAME_SZ] = 0;
    return name;
}

static void print_uuid_bin(const unsigned char *uuid_bin, int guid_fmt)
{
    const u8 uuid_char_order[16] = { 0, 1, 2,  3,  4,  5,  6,  7,
                                     8, 9, 10, 11, 12, 13, 14, 15 };
    const u8 guid_char_order[16] = { 3, 2, 1,  0,  5,  4,  7,  6,
                                     8, 9, 10, 11, 12, 13, 14, 15 };
    const u8 *char_order;
    int i;

    /*
	 * UUID and GUID bin data - always in big endian:
	 * 4B-2B-2B-2B-6B
	 * be be be be be
	 */
    if (guid_fmt)
        char_order = guid_char_order;
    else
        char_order = uuid_char_order;

    for (i = 0; i < 16; i++) {
        printf("%02x", uuid_bin[char_order[i]]);
        switch (i) {
            case 3:
            case 5:
            case 7:
            case 9:
                printf("-");
                break;
        }
    }
}

int aic_disk_dump_gpt_parts(struct blk_desc *dev_desc)
{
    u8 gpt_buf[512];
    gpt_header *gpt_head = (void *)gpt_buf;
    gpt_entry *gpt_pte = NULL;
    int i = 0;
    unsigned char *uuid_bin;

    /* This function validates AND fills in the GPT header and PTE */
    if (find_valid_gpt(dev_desc, gpt_head, &gpt_pte) != 1)
        return 0;

    pr_debug("%s: gpt-entry at %p\n", __func__, gpt_pte);

    printf("Part\tStart LBA\tEnd LBA\t\tName\n");
    printf("\tAttributes\n");
    printf("\tType GUID\n");
    printf("\tPartition GUID\n");

    for (i = 0; i < (gpt_head->num_partition_entries); i++) {
        /* Stop at the first non valid PTE */
        if (!is_pte_valid(&gpt_pte[i]))
            break;

        printf("%3d\t0x%08llx\t0x%08llx\t\"%s\"\n", (i + 1),
               (gpt_pte[i].starting_lba), (gpt_pte[i].ending_lba),
               print_efiname(&gpt_pte[i]));
        printf("\tattrs:\t0x%016llx\n", gpt_pte[i].attributes.raw);
        uuid_bin = (unsigned char *)gpt_pte[i].partition_type_guid.b;
        printf("\ttype: ");
        print_uuid_bin(uuid_bin, 0);
        printf("\n");
        uuid_bin = (unsigned char *)gpt_pte[i].unique_partition_guid.b;
        printf("\tguid: ");
        print_uuid_bin(uuid_bin, 1);
        printf("\n");
    }

    /* Remember to free pte */
    free(gpt_pte);
    return i;
}

int aic_disk_dump_mbr_parts(struct blk_desc *dev_desc)
{
    u8 mbr_buf[512] = {0};
    legacy_mbr *p_mbr = (void *)mbr_buf;
    struct partition *pp;
    int i;

    if (blk_dread(dev_desc, 0, 1, p_mbr) != 1) {
        pr_err("** Can't read from device **\n");
        return 0;
    }

    if (p_mbr->signature != MSDOS_MBR_SIGNATURE) {
        pr_err("** Not MBR sector **\n");
        return 0;
    }

    if (is_pmbr_valid(p_mbr))
        return 0;

    for (i = 0; i < 4; i++) {
        pp = &p_mbr->partition_record[i];
        if ((pp->boot_ind != 0x00) && (pp->boot_ind != 0x80)) {
            break;
        }
        printf("Partition %d\n", i + 1);
        printf("  boot_ind   = 0x%x\n", pp->boot_ind);
        printf("  head       = 0x%x\n", pp->head);
        printf("  sector     = 0x%x\n", pp->sector);
        printf("  cyl        = 0x%x\n", pp->cyl);
        printf("  sys_ind    = 0x%x\n", pp->sys_ind);
        printf("  end_head   = 0x%x\n", pp->end_head);
        printf("  end_sector = 0x%x\n", pp->end_sector);
        printf("  end_cyl    = 0x%x\n", pp->end_cyl);
        printf("  start_sect = 0x%x\n", pp->start_sect);
        printf("  nr_sects   = 0x%x\n", pp->nr_sects);
    }

    return i;
}
void aic_disk_dump_parts(struct blk_desc *dev_desc)
{
    int ret;

    ret = aic_disk_dump_mbr_parts(dev_desc);
    if (ret)
        return;
    aic_disk_dump_gpt_parts(dev_desc);
}

struct aic_partition *aic_disk_get_parts(struct blk_desc *dev_desc)
{
    struct aic_partition *ret;

    ret = aic_disk_get_gpt_parts(dev_desc);
    if (ret)
        return ret;
    return aic_disk_get_mbr_parts(dev_desc);
}

struct aic_partition *aic_disk_get_mbr_parts(struct blk_desc *dev_desc)
{
    u8 mbr_buf[512] = {0};
    legacy_mbr *p_mbr = (void *)mbr_buf;
    struct partition *pp;
    struct aic_partition *parts, *p, *n;
    int i;

    if (blk_dread(dev_desc, 0, 1, p_mbr) != 1) {
        pr_err("** Can't read from device **\n");
        return NULL;
    }

    if (p_mbr->signature != MSDOS_MBR_SIGNATURE) {
        pr_info("** Not MBR sector **\n");
        return NULL;
    }

    parts = p = n = NULL;
    for (i = 0; i < 4; i++) {
        pp = &p_mbr->partition_record[i];
        if ((pp->boot_ind != 0x00) && (pp->boot_ind != 0x80)) {
            break;
        }
        if (pp->start_sect == 0)
            break;

        n = malloc(sizeof(*n));
        if (!n) {
            pr_err("%s, malloc buffer for partition failed.\n", __func__);
            goto err;
        }
        memset(n, 0, sizeof(*n));
        n->start = pp->start_sect * dev_desc->blksz;
        n->size = (u64)pp->nr_sects * dev_desc->blksz;
        n->index = i;

        if (parts == NULL) {
            parts = n;
        }
        if (p) {
            p->next = n;
            p = p->next;
        } else {
            p = n;
        }
    }

    return parts;
err:
    if (parts)
        aic_part_free(parts);
    return NULL;
}

struct aic_partition *aic_disk_get_gpt_parts(struct blk_desc *dev_desc)
{
    struct aic_partition *parts, *p, *n;
    u8 gpt_buf[512];
    gpt_header *gpt_head = (void *)gpt_buf;
    gpt_entry *gpt_pte = NULL;
    int i;

    /* This function validates AND fills in the GPT header and PTE */
    if (find_valid_gpt(dev_desc, gpt_head, &gpt_pte) != 1)
        return NULL;

    parts = NULL;
    p = NULL;
    for (i = 0; i < (gpt_head->num_partition_entries); i++) {
        if (!is_pte_valid(&gpt_pte[i])) {
            pr_debug("%s: *** Invalid partition number %d ***\n", __func__, i);
            break;
        }
        n = malloc(sizeof(*n));
        if (!n) {
            pr_err("%s, malloc buffer for partition failed.\n", __func__);
            goto err;
        }
        memset(n, 0, sizeof(*n));

        /* The 'u64' casting may limit the maximum disk size to 2 TB */
        n->start = gpt_pte[i].starting_lba * dev_desc->blksz;

        /* The ending LBA is inclusive, to calculate size, add 1 to it */
        n->size = (gpt_pte[i].ending_lba + 1 - gpt_pte[i].starting_lba) *
                  dev_desc->blksz;

        n->index = i;

        snprintf((char *)n->name, sizeof(n->name), "%s",
                 print_efiname(&gpt_pte[i]));
        if (parts == NULL) {
            parts = n;
        }
        if (p) {
            p->next = n;
            p = p->next;
        } else {
            p = n;
        }
    }

    n = parts;
    /* Remember to free pte */
    free(gpt_pte);
    return parts;

err:
    if (parts)
        aic_part_free(parts);
    free(gpt_pte);
    return NULL;
}

/**
 * set_protective_mbr(): Set the EFI protective MBR
 * @param dev_desc - block device descriptor
 *
 * @return - zero on success, otherwise error
 */
static int set_protective_mbr(struct blk_desc *dev_desc)
{
    /* Setup the Protective MBR */
    u8 mbr_buf[512] = {0};
    legacy_mbr *p_mbr = (void *)mbr_buf;

    if (p_mbr == NULL) {
        printf("%s: calloc failed!\n", __func__);
        return -1;
    }

    /* Read MBR to backup boot code if it exists */
    if (blk_dread(dev_desc, 0, 1, p_mbr) != 1) {
        pr_err("** Can't read from device **\n");
        return -1;
    }

    /* Clear all data in MBR except of backed up boot code */
    memset((char *)p_mbr + MSDOS_MBR_BOOT_CODE_SIZE, 0,
           sizeof(*p_mbr) - MSDOS_MBR_BOOT_CODE_SIZE);

    /* Append signature */
    p_mbr->signature = MSDOS_MBR_SIGNATURE;
    p_mbr->partition_record[0].sys_ind = EFI_PMBR_OSTYPE_EFI_GPT;
    p_mbr->partition_record[0].start_sect = 1;
    p_mbr->partition_record[0].nr_sects = (u32)dev_desc->lba_count - 1;

    /* Write MBR sector to the MMC device */
    if (blk_dwrite(dev_desc, 0, 1, p_mbr) != 1) {
        printf("** Can't write to device **\n");
        return -1;
    }

    return 0;
}

static int write_gpt_table(struct blk_desc *dev_desc, gpt_header *gpt_h,
                           gpt_entry *gpt_e)
{
    const int pte_blk_cnt =
        PAD_COUNT((gpt_h->num_partition_entries * sizeof(gpt_entry)), 512);
    u32 calc_crc32;

    pr_debug("max lba: %x\n", (u32)dev_desc->lba_count);
    /* Setup the Protective MBR */
    if (set_protective_mbr(dev_desc) < 0)
        goto err;

    /* Generate CRC for the Primary GPT Header */
    calc_crc32 = efi_crc32((const unsigned char *)gpt_e,
                           (gpt_h->num_partition_entries) *
                               (gpt_h->sizeof_partition_entry));
    gpt_h->partition_entry_array_crc32 = (calc_crc32);

    calc_crc32 = efi_crc32((const unsigned char *)gpt_h, (gpt_h->header_size));
    gpt_h->header_crc32 = (calc_crc32);

    /* Write the First GPT to the block right after the Legacy MBR */
    if (blk_dwrite(dev_desc, 1, 1, gpt_h) != 1)
        goto err;

    if (blk_dwrite(dev_desc, gpt_h->partition_entry_lba, pte_blk_cnt, gpt_e) !=
        pte_blk_cnt)
        goto err;

    prepare_backup_gpt_header(gpt_h);

    if (blk_dwrite(dev_desc, (u64)gpt_h->last_usable_lba + 1, pte_blk_cnt,
                   gpt_e) != pte_blk_cnt)
        goto err;

    if (blk_dwrite(dev_desc, (u64)(gpt_h->my_lba), 1, gpt_h) != 1)
        goto err;

    pr_debug("GPT successfully written to block device!\n");
    return 0;

err:
    printf("** Can't write to device **\n");
    return -1;
}

int gpt_fill_pte(struct blk_desc *dev_desc, gpt_header *gpt_h, gpt_entry *gpt_e,
                 struct aic_partition *partitions)
{
    u64 offset = (u64)(gpt_h->first_usable_lba);
    u64 last_usable_lba = (u64)(gpt_h->last_usable_lba);
    int i, k;
    u8 *p;
    struct aic_partition *part;
    size_t efiname_len, dosname_len;
    size_t hdr_start = gpt_h->my_lba;
    size_t hdr_end = hdr_start + 1;
    size_t pte_start = gpt_h->partition_entry_lba;
    size_t pte_end = pte_start + gpt_h->num_partition_entries *
                                     gpt_h->sizeof_partition_entry /
                                     dev_desc->blksz;

    part = partitions;
    i = 0;
    while (part) {
        /* partition starting lba */
        u64 start = part->start / dev_desc->blksz;
        u64 size = part->size / dev_desc->blksz;

        if (start) {
            offset = start + size;
        } else {
            start = offset;
            offset += size;
        }

        /*
		 * If our partition overlaps with either the GPT
		 * header, or the partition entry, reject it.
		 */
        if (((start < hdr_end && hdr_start < (start + size)) ||
             (start < pte_end && pte_start < (start + size)))) {
            printf("Partition overlap\n");
            return -1;
        }

        gpt_e[i].starting_lba = (start);

        if (offset > (last_usable_lba + 1)) {
            printf("Partitions layout exceds disk size\n");
            return -1;
        }
        /* partition ending lba */
        if ((part->next == NULL) && (size == 0))
            /* extend the last partition to maximuim */
            gpt_e[i].ending_lba = gpt_h->last_usable_lba;
        else
            gpt_e[i].ending_lba = (offset - 1);

        /* partition type GUID */
        memcpy(gpt_e[i].partition_type_guid.b, &partition_basic_data_guid, 16);
        memcpy(gpt_e[i].unique_partition_guid.b, &partition_basic_data_guid, 16);
        p = &gpt_e[i].unique_partition_guid.b[15];
        *p += (i + 1);

        /* partition attributes */
        memset(&gpt_e[i].attributes, 0, sizeof(gpt_entry_attributes));

        /* partition name */
        efiname_len = sizeof(gpt_e[i].partition_name) / sizeof(u16);
        dosname_len = sizeof(part->name);

        memset(gpt_e[i].partition_name, 0, sizeof(gpt_e[i].partition_name));

        for (k = 0; k < min(dosname_len, efiname_len); k++)
            gpt_e[i].partition_name[k] = (u16)(part->name[k]);

        part = part->next;
        i++;
    }

    return 0;
}

static u32 partition_entries_offset(struct blk_desc *dev_desc)
{
    u32 offset_blks = 2;

    /*
	 * The earliest LBA this can be at is LBA#2 (i.e. right behind
	 * the (protective) MBR and the GPT header.
	 */
    if (offset_blks < 2)
        offset_blks = 2;

    return offset_blks;
}

int gpt_fill_header(struct blk_desc *dev_desc, gpt_header *gpt_h, int part_cnt)
{
    gpt_h->signature = (GPT_HEADER_SIGNATURE_EFI);
    gpt_h->revision = (GPT_HEADER_REVISION_V1);
    gpt_h->header_size = (sizeof(gpt_header));
    gpt_h->my_lba = (1);
    gpt_h->alternate_lba = (dev_desc->lba_count - 1);
    gpt_h->last_usable_lba = (dev_desc->lba_count - 34);
    gpt_h->partition_entry_lba = (partition_entries_offset(dev_desc));
    gpt_h->first_usable_lba = ((gpt_h->partition_entry_lba) + 32);
    gpt_h->num_partition_entries = (part_cnt);
    gpt_h->sizeof_partition_entry = (sizeof(gpt_entry));
    gpt_h->header_crc32 = 0;
    gpt_h->partition_entry_array_crc32 = 0;

    memcpy(gpt_h->disk_guid.b, &aic_disk_guid, 16);

    return 0;
}

int aic_disk_write_gpt(struct blk_desc *dev_desc, struct aic_partition *parts)
{
    gpt_header *gpt_h;
    gpt_entry *gpt_e;
    int ret, size, cnt;
    struct aic_partition *p;

    p = parts;
    cnt = 0;
    while (p) {
        cnt++;
        p = p->next;
    }
    if (cnt > GPT_ENTRY_NUMBERS) {
        pr_err("Too many partition entries.\n");
        return -1;
    }

    size = PAD_SIZE(sizeof(gpt_header), 512);
    gpt_h = malloc(size);
    if (gpt_h == NULL) {
        printf("%s: calloc failed!\n", __func__);
        return -1;
    }
    memset(gpt_h, 0, size);

    size = PAD_SIZE(GPT_ENTRY_NUMBERS * sizeof(gpt_entry), 512);
    gpt_e = malloc(size);
    if (gpt_e == NULL) {
        printf("%s: calloc failed!\n", __func__);
        free(gpt_h);
        return -1;
    }
    memset(gpt_e, 0, size);

    /* Generate Primary GPT header (LBA1) */
    ret = gpt_fill_header(dev_desc, gpt_h, cnt);
    if (ret)
        goto err;

    /* Generate partition entries */
    ret = gpt_fill_pte(dev_desc, gpt_h, gpt_e, parts);
    if (ret)
        goto err;

    /* Write GPT partition table */
    ret = write_gpt_table(dev_desc, gpt_h, gpt_e);

err:
    free(gpt_e);
    free(gpt_h);
    return ret;
}

/**
 * gpt_convert_efi_name_to_char() - convert u16 string to char string
 *
 * TODO: this conversion only supports ANSI characters
 *
 * @s:	target buffer
 * @es:	u16 string to be converted
 * @n:	size of target buffer
 */
static void gpt_convert_efi_name_to_char(char *s, void *es, int n)
{
    char *ess = es;
    int i, j;

    memset(s, '\0', n);

    for (i = 0, j = 0; j < n; i += 2, j++) {
        s[j] = ess[i];
        if (!ess[i])
            return;
    }
}

int gpt_verify_headers(struct blk_desc *dev_desc, gpt_header *gpt_head,
                       gpt_entry **gpt_pte)
{
    /*
	 * This function validates AND
	 * fills in the GPT header and PTE
	 */
    if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA, gpt_head,
                     gpt_pte) != 1) {
        printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
        return -1;
    }

    /* Free pte before allocating again */
    free(*gpt_pte);

    /*
	 * Check that the alternate_lba entry points to the last LBA
	 */
    if ((gpt_head->alternate_lba) != (dev_desc->lba_count - 1)) {
        printf("%s: *** ERROR: Misplaced Backup GPT ***\n", __func__);
        return -1;
    }

    if (is_gpt_valid(dev_desc, (dev_desc->lba_count - 1), gpt_head, gpt_pte) !=
        1) {
        printf("%s: *** ERROR: Invalid Backup GPT ***\n", __func__);
        return -1;
    }

    return 0;
}

int gpt_verify_partitions(struct blk_desc *dev_desc,
                          struct aic_partition *partitions,
                          gpt_header *gpt_head, gpt_entry **gpt_pte)
{
    struct aic_partition *part;
    char efi_str[PARTNAME_SZ + 1];
    u64 gpt_part_size;
    gpt_entry *gpt_e;
    int ret, i;

    ret = gpt_verify_headers(dev_desc, gpt_head, gpt_pte);
    if (ret)
        return ret;

    gpt_e = *gpt_pte;

    part = partitions;
    i = 0;
    while (part) {
        if (i == gpt_head->num_partition_entries) {
            pr_err("More partitions than allowed!\n");
            return -1;
        }

        /* Check if GPT and ENV partition names match */
        gpt_convert_efi_name_to_char(efi_str, gpt_e[i].partition_name,
                                     PARTNAME_SZ + 1);

        pr_debug("%s: part: %2d name - GPT: %16s, ENV: %16s ", __func__, i,
                 efi_str, part->name);

        if (strncmp(efi_str, (char *)part->name, sizeof(part->name))) {
            pr_err("Partition name: %s does not match %s!\n", efi_str,
                   (char *)part->name);
            return -1;
        }

        /* Check if GPT and ENV sizes match */
        gpt_part_size = (gpt_e[i].ending_lba) - (gpt_e[i].starting_lba) + 1;
        pr_debug("size(LBA) - GPT: %8llu, ENV: %8llu ",
                 (unsigned long long)gpt_part_size,
                 (unsigned long long)part->size);

        if ((gpt_part_size) != part->size) {
            /* We do not check the extend partition size */
            if ((part->next == NULL) && (part->size == 0))
                continue;

            pr_err("Partition %s size: %llu does not match %llu!\n", efi_str,
                   (unsigned long long)gpt_part_size,
                   (unsigned long long)part->size);
            return -1;
        }

        /*
		 * Start address is optional - check only if provided
		 * in '$partition' variable
		 */
        if (!part->start) {
            pr_debug("\n");
            continue;
        }

        /* Check if GPT and ENV start LBAs match */
        pr_debug("start LBA - GPT: %8llu, ENV: %8llu\n",
                 (gpt_e[i].starting_lba), (unsigned long long)part->start);

        if ((gpt_e[i].starting_lba) != part->start) {
            pr_err("Partition %s start: %llu does not match %llu!\n", efi_str,
                   (gpt_e[i].starting_lba), (unsigned long long)part->start);
            return -1;
        }
    }

    return 0;
}

int is_valid_gpt_buf(struct blk_desc *dev_desc, void *buf)
{
    gpt_header *gpt_h;
    gpt_entry *gpt_e;

    /* determine start of GPT Header in the buffer */
    gpt_h = buf + (GPT_PRIMARY_PARTITION_TABLE_LBA * dev_desc->blksz);
    if (validate_gpt_header(gpt_h, GPT_PRIMARY_PARTITION_TABLE_LBA,
                            dev_desc->lba_count))
        return -1;

    /* determine start of GPT Entries in the buffer */
    gpt_e = buf + ((gpt_h->partition_entry_lba) * 512);
    if (validate_gpt_entries(gpt_h, gpt_e))
        return -1;

    return 0;
}


/*
 * Private functions
 */
/*
 * pmbr_part_valid(): Check for EFI partition signature
 *
 * Returns: 1 if EFI GPT partition type is found.
 */
int pmbr_part_valid(struct partition *part)
{
    u32 val;
    u8 *p;

    p = (void *)&part->start_sect;
    val = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    if ((part->sys_ind == EFI_PMBR_OSTYPE_EFI_GPT) && (val == 1)) {
        return 1;
    }

    return 0;
}

/*
 * is_pmbr_valid(): test Protective MBR for validity
 *
 * Returns: 1 if PMBR is valid, 0 otherwise.
 * Validity depends on two things:
 *  1) MSDOS signature is in the last two bytes of the MBR
 *  2) One partition of type 0xEE is found, checked by pmbr_part_valid()
 */
int is_pmbr_valid(legacy_mbr *mbr)
{
    int i = 0;

    if (!mbr || (mbr->signature) != MSDOS_MBR_SIGNATURE)
        return 0;

    for (i = 0; i < 4; i++) {
        if (pmbr_part_valid(&mbr->partition_record[i])) {
            return 1;
        }
    }
    return 0;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 *
 * lba is the logical block address of the GPT header to test
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error, 2 if ignored header
 * If valid, returns pointers to PTEs.
 */
int is_gpt_valid(struct blk_desc *dev_desc, u64 lba, gpt_header *pgpt_head,
                 gpt_entry **pgpt_pte)
{
    u8 mbr_buf[512] = {0};
    legacy_mbr *mbr = (void *)mbr_buf;
    /* Confirm valid arguments prior to allocation. */
    if (!dev_desc || !pgpt_head) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return 0;
    }

    /* Read MBR Header from device */
    if (blk_dread(dev_desc, 0, 1, (ulong *)mbr) != 1) {
        printf("*** ERROR: Can't read MBR header ***\n");
        return 0;
    }

    /* Read GPT Header from device */
    if (blk_dread(dev_desc, (u64)lba, 1, pgpt_head) != 1) {
        printf("*** ERROR: Can't read GPT header ***\n");
        return 0;
    }

    if (validate_gpt_header(pgpt_head, (u64)lba, dev_desc->lba_count))
        return 0;

    /* Read and allocate Partition Table Entries */
    *pgpt_pte = alloc_read_gpt_entries(dev_desc, pgpt_head);
    if (*pgpt_pte == NULL) {
        printf("GPT: Failed to allocate memory for PTE\n");
        return 0;
    }

    if (validate_gpt_entries(pgpt_head, *pgpt_pte)) {
        free(*pgpt_pte);
        return 0;
    }

    /* We're done, all's well */
    return 1;
}

/**
 * find_valid_gpt() - finds a valid GPT header and PTEs
 *
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if found a valid gpt,  0 on error.
 * If valid, returns pointers to PTEs.
 */
int find_valid_gpt(struct blk_desc *dev_desc, gpt_header *gpt_head,
                   gpt_entry **pgpt_pte)
{
    int r;

    r = is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA, gpt_head,
                     pgpt_pte);

    if (r != 1) {
        if (r != 2)
            pr_debug("%s: *** ERROR: Invalid GPT ***\n", __func__);

        if (is_gpt_valid(dev_desc, (dev_desc->lba_count - 1), gpt_head,
                         pgpt_pte) != 1) {
            pr_debug("%s: *** ERROR: Invalid Backup GPT ***\n", __func__);
            return 0;
        }
        if (r != 2)
            pr_debug("%s: ***        Using Backup GPT ***\n", __func__);
    }
    return 1;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @dev_desc
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
gpt_entry *alloc_read_gpt_entries(struct blk_desc *dev_desc,
                                  gpt_header *pgpt_head)
{
    size_t count = 0, blk_cnt;
    u64 blk;
    gpt_entry *pte = NULL;

    if (!dev_desc || !pgpt_head) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return NULL;
    }

    count = (pgpt_head->num_partition_entries) *
            (pgpt_head->sizeof_partition_entry);

    pr_debug("%s: count = %u * %u = %lu\n", __func__,
             (u32)(pgpt_head->num_partition_entries),
             (u32)(pgpt_head->sizeof_partition_entry), (ulong)count);

    /* Allocate memory for PTE, remember to FREE */
    if (count != 0) {
        pte = malloc(PAD_SIZE(count, 512));
    }

    if (count == 0 || pte == NULL) {
        printf("%s: ERROR: Can't allocate %#lX bytes for GPT Entries\n",
               __func__, (ulong)count);
        return NULL;
    }

    /* Read GPT Entries from device */
    blk = (pgpt_head->partition_entry_lba);
    blk_cnt = PAD_COUNT(count, 512);
    if (blk_dread(dev_desc, blk, (u64)blk_cnt, pte) != blk_cnt) {
        printf("*** ERROR: Can't read GPT Entries ***\n");
        free(pte);
        return NULL;
    }
    return pte;
}
#pragma GCC diagnostic pop
/**
 * is_pte_valid(): validates a single Partition Table Entry
 * @gpt_entry - Pointer to a single Partition Table Entry
 *
 * Description: returns 1 if valid,  0 on error.
 */
int is_pte_valid(gpt_entry *pte)
{
    efi_guid_t unused_guid;

    if (!pte) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return 0;
    }

    /* Only one validation for now:
	 * The GUID Partition Type != Unused Entry (ALL-ZERO)
	 */
    memset(unused_guid.b, 0, sizeof(unused_guid.b));

    if (memcmp(pte->partition_type_guid.b, unused_guid.b,
               sizeof(unused_guid.b)) == 0) {
        pr_debug("%s: Found an unused PTE GUID at 0x%08X\n", __func__,
                 (unsigned int)(uintptr_t)pte);

        return 0;
    } else {
        return 1;
    }
}
