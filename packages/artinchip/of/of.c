/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Li Siyao <siyao.li@artinchip.com>
 */

#include <libfdt.h>
#include <stdlib.h>
#include <rtconfig.h>
#include <errno.h>
#include <string.h>
#include <aic_core.h>
#include <env.h>
#include "of.h"
#include <math.h>
#include <aic_partition.h>
#ifdef KERNEL_BAREMETAL
#include <mtd.h>
#endif
#ifdef KERNEL_RTTHREAD
#include <rtdevice.h>
#endif

#if defined(AIC_SDMC_DRV) && defined(KERNEL_BAREMETAL)
#include <mmc.h>
#include <partition_table.h>

#define GPT_HEADER_SIZE     (34 * 512)
#ifdef IMAGE_CFG_JSON_PARTS_GPT
#define MMC_GPT_PARTS       IMAGE_CFG_JSON_PARTS_GPT
#else
#define MMC_GPT_PARTS       ""
#endif
#endif

#define AIC_BYTES_SIZE          4
#define AIC_CONFIG_PART         "config"
#define AIC_MMC_CONFIG_DEV      "mmc0p8"
#define AIC_DTB_DATA_BASE_ADDR  0x800

#ifdef AIC_DRAM_TOTAL_SIZE
#define AIC_DDR_DTB_ADDR        0x40000000 + AIC_DRAM_TOTAL_SIZE - 256 * 1024
#endif
#ifdef AIC_PSRAM_SIZE
#define AIC_PSRAM_DTB_ADDR      0x40000000 + AIC_PSRAM_SIZE - 256 * 1024
#endif

static void *g_aic_initial_blob = NULL;

bool of_fdt_device_is_available(ofnode node)
{
    CHECK_PARAM(g_aic_initial_blob, false);
    const char *status;
    status = fdt_getprop(g_aic_initial_blob, node.offset, "status", NULL);

    if (!status)
       return true;

    if (!strcmp(status, "ok") || !strcmp(status, "okay"))
       return true;

    return false;
}

void of_relocate_dtb(unsigned long pos)
{
    g_aic_initial_blob = (void *)pos;
}

/**
 * This function is used to debug the result of the mtd parsing
 */
#if 0
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')
static void mtd_dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16)
    {
        printf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}
#endif

int of_dtb_is_available(void)
{
    int ret = -1;
    int dtb_data_size = 0;
    struct fdt_header *dtb_header_data = g_aic_initial_blob;

    dtb_data_size = fdt_totalsize(dtb_header_data);
    if (dtb_data_size == 0) {
        printf("The DTB data size is 0 K\n");
        goto __exit;
    } else {
        printf("The %s partition exist, and the DTB data size is %d K\n",
               AIC_CONFIG_PART, dtb_data_size / 1024);
    }

    ret = 0;

__exit:
    return ret;
}

#ifdef KERNEL_BAREMETAL
#ifdef AIC_SDMC_DRV
char *aic_mmc_get_partition_string(int mmc_id);
int of_fdt_dt_init_bare_mmc(void)
{
    int ret = -1;
    u8 *mmc_data = NULL;
    int dtb_data_size = 0;
    int mmc_blk_size = 512;
    unsigned long mmc_id = 0;
    unsigned long blkoffset = 0;
    struct aic_sdmc *host = NULL;
    void *psram_or_ddr_dtb_addr = NULL;
    char *partstr;
    struct aic_partition *part = NULL;
    struct aic_partition *parts = NULL;
    struct fdt_header *dtb_header_data = NULL;
    u32 itb_head_offset = AIC_DTB_DATA_BASE_ADDR / mmc_blk_size;

    ret = mmc_init(mmc_id);
    if (ret) {
        printf("SDMC %ld init failed.\n", mmc_id);
        return ret;
    }

    host = find_mmc_dev_by_index(mmc_id);
    if (host== NULL) {
        printf("Can't find mmc device!");
        return ret;
    }
    partstr = aic_mmc_get_partition_string(mmc_id);
    parts = aic_part_gpt_parse(partstr);
    if (!parts)
        return ret;

    if (parts->start != GPT_HEADER_SIZE) {
        pr_err("First partition start offset is not correct\n");
        goto __exit;
    }

    part = parts;
    blkoffset = part->start;

    while (part) {
        if (!strcmp(part->name, AIC_CONFIG_PART))
            break;
        blkoffset += part->size;
        part = part->next;
    }

    blkoffset /= mmc_blk_size;

    dtb_header_data = (struct fdt_header *)malloc(sizeof(struct fdt_header));
    if (dtb_header_data == NULL) {
        printf("Unable to allocate memory for dtb_header_data");
        goto __exit;
    }

    mmc_data = malloc(mmc_blk_size);
    if (mmc_data == NULL) {
        printf("Unable to allocate memory for mmc_data");
        goto __exit;
    }

    /* Read the size of the dtb data in the dtb data header.*/
    mmc_bread(host, blkoffset + itb_head_offset, 1, (void *)mmc_data);

    memcpy(dtb_header_data, mmc_data, sizeof(struct fdt_header));
    dtb_data_size = fdt_totalsize(dtb_header_data);
    if (dtb_data_size == 0) {
        printf("The DTB data size is 0 K\n");
        goto __exit;
    }

#ifdef AIC_PSRAM_SIZE
    psram_or_ddr_dtb_addr = (void *)AIC_PSRAM_DTB_ADDR;
#endif
#ifdef AIC_DRAM_TOTAL_SIZE
    psram_or_ddr_dtb_addr = (void *)AIC_DDR_DTB_ADDR;
#endif

    if (psram_or_ddr_dtb_addr == NULL) {
        printf("Please check RAM config\n");
        goto __exit;
    }

    mmc_bread(host, blkoffset + itb_head_offset, dtb_data_size / mmc_blk_size,
              (void *)psram_or_ddr_dtb_addr);

    g_aic_initial_blob = psram_or_ddr_dtb_addr;
    if (fdt_check_header(g_aic_initial_blob) != 0) {
        printf("ERROR: Invalid device tree blob\n");
        goto __exit;
    }

    ret = 0;

__exit:
    if (mmc_data)
        free(mmc_data);
    if (dtb_header_data)
        free(dtb_header_data);
    if (parts)
        aic_part_free(parts);
    return ret;
}
#endif

#if defined(AIC_SPINOR_DRV) || defined(AIC_SPINAND_DRV)

int of_fdt_dt_init_bare_nornand(void)
{
    struct mtd_dev *mtd;
    unsigned long offset = AIC_DTB_DATA_BASE_ADDR;
    struct fdt_header dtb_header_data;
    int dtb_data_size = 0;
    void *psram_or_ddr_dtb_addr = NULL;

    if (g_aic_initial_blob != NULL) {
        printf("FDT has inited\n");
        return 0;
    }

    mtd = mtd_get_device(AIC_CONFIG_PART);
    if (!mtd) {
        printf("No %s partition\n", AIC_CONFIG_PART);
        return -1;
    }

    /* Read the size of the dtb data in the dtb data header.*/
    mtd_read(mtd, offset, (void *)&dtb_header_data, sizeof(struct fdt_header));

    dtb_data_size = fdt_totalsize(&dtb_header_data);
    if (dtb_data_size == 0) {
        printf("The DTB data size is 0 K\n");
        return -1;
    }

/* store the dtb data in the last 256k address of psram/ddr */
#ifdef AIC_PSRAM_SIZE
    psram_or_ddr_dtb_addr = (void *)AIC_PSRAM_DTB_ADDR;
#endif
#ifdef AIC_DRAM_TOTAL_SIZE
    psram_or_ddr_dtb_addr = (void *)AIC_DDR_DTB_ADDR;
#endif
    mtd_read(mtd, offset, psram_or_ddr_dtb_addr, dtb_data_size);
    g_aic_initial_blob = psram_or_ddr_dtb_addr;

    if (fdt_check_header(g_aic_initial_blob) != 0) {
        printf("ERROR: Invalid device tree blob\n");
        return -1;
    }

    return 0;
}
#endif
#endif

int of_find_node_by_path(const char *path, ofnode *node)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    node->offset = fdt_path_offset(g_aic_initial_blob, path);

    return 0;
}

int of_node_to_offset(ofnode node)
{
    return node.offset;
}

int of_find_subnode(ofnode node, const char *subnode_name, ofnode *subnode)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    subnode->offset = fdt_subnode_offset(g_aic_initial_blob,
                                        of_node_to_offset(node), subnode_name);

    return 0;
}

const char *of_get_nodename(ofnode node)
{
    CHECK_PARAM(g_aic_initial_blob, NULL);

    return fdt_get_name(g_aic_initial_blob, of_node_to_offset(node), NULL);
}

ofnode of_first_subnode(ofnode node)
{
    ofnode subnode;

    if (!g_aic_initial_blob) {
        subnode.offset = 0;
    } else {
        subnode.offset = fdt_first_subnode(g_aic_initial_blob,
                                           of_node_to_offset(node));
    }

    return subnode;
}

/**
 * of_next_subnode - get the sibling node by given name
 *
 * @node: the node
 * @return the Sibling node, or NULL if there is none
 */
ofnode of_next_subnode(ofnode node)
{
    ofnode subnode;

    if (!g_aic_initial_blob) {
        subnode.offset = 0;
    } else {
        subnode.offset = fdt_next_subnode(g_aic_initial_blob,
                                      of_node_to_offset(node));
    }

    return subnode;
}

int of_parent_node(ofnode node, ofnode *parent_node)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    parent_node->offset = fdt_parent_offset(g_aic_initial_blob,
                                           of_node_to_offset(node));
    return 0;
}

int of_read_addr_cells(ofnode node)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    return fdt_address_cells(g_aic_initial_blob, of_node_to_offset(node));
}

int of_read_size_cells(ofnode node)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    return fdt_size_cells(g_aic_initial_blob, of_node_to_offset(node));
}

int of_node_write_prop(ofnode node, const char *name, const void *val, int len)
{
    int ret;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    ret = fdt_setprop(g_aic_initial_blob, of_node_to_offset(node), name,
                      val, len);
    if (ret < 0) {
        printf("Set the prop-%s failed\n", name);
        return -1;
    }

    return 0;
}

int of_node_delete_prop(ofnode node, const char *name)
{
    int ret;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    ret = fdt_delprop(g_aic_initial_blob, of_node_to_offset(node), name);
    if (ret < 0) {
        printf("Delete the prop-%s failed\n", name);
        return -1;
    }

    return 0;
}

/**
 * fdt_appendprop_string - append a string to a property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to change
 * @name: name of the property to change
 * @str: string value to append to the property
 */
int of_node_append_prop_string(ofnode node, const char *name, const void *str)
{
    int ret;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    ret = fdt_appendprop_string(g_aic_initial_blob, of_node_to_offset(node), name, str);
    if (ret < 0) {
        printf("Append the prop-%s failed\n", (char *)str);
        return -1;
    }

    return 0;
}

/**
 * of_node_append_prop_u32 - append a 32-bit integer value to a property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to change
 * @name: name of the property to change
 * @val: 32-bit integer value to append to the property (native endian)
 */
int of_node_append_prop_u32(ofnode node, const char *name, uint32_t val)
{
    int ret;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    ret = fdt_appendprop_u32(g_aic_initial_blob, of_node_to_offset(node), name, val);
    if (ret < 0) {
        printf("Append the prop-%s failed\n", name);
        return -1;
    }

    return 0;
}

const int of_property_read_size(ofnode node, const char *name)
{
    int size;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    if (fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size) == NULL)
        return -1;

    return size;
}

int of_property_read_u64(ofnode node, const char *name, u64 *data)
{
    int size;
    const fdt64_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    if (size < AIC_BYTES_SIZE) {
        printf("Error: value array length is less than 4 bytes.");
        return -1;
    }
    *data = fdt64_to_cpu(*value);

    return 0;
}

const int of_property_read_u64_array(ofnode node, const char *name,
                                     int *cnt, u64 *data_arr)
{
    int size;
    const fdt64_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 8;
     for (int i = 0; i < *cnt; i++) {
        data_arr[i] = 0;
        data_arr[i] = fdt64_to_cpu(*value++);
    }
    return 0;
}

const int of_property_get_u64_array_number(ofnode node, const char *name,
                                           int *cnt)
{
    int size;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 8;

    return 0;
}

const int of_property_read_u64_by_index(ofnode node, const char *name,
                                        int index, u64 *out_value)
{
    int size;
    const fdt64_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    if (value)
        *out_value = fdt64_to_cpu(*(value + index));

    return 0;
}

int of_property_read_u32(ofnode node, const char *name, u32 *data)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    if (size < AIC_BYTES_SIZE) {
        printf("Error: value array length is less than 4 bytes.\n");
        return -1;
    }
    *data = fdt32_to_cpu(*value);

    return 0;
}

const int of_property_read_u32_array(ofnode node, const char *name,
                                     int *cnt, u32 *data_arr)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;
     for (int i = 0; i < *cnt; i++) {
        data_arr[i] = 0;
        data_arr[i] = fdt32_to_cpu(*value++);
    }
    return 0;
}

const int of_property_get_u32_array_number(ofnode node, const char *name, int *cnt)
{
    int size;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;

    return 0;
}

const int of_property_read_u32_by_index(ofnode node, const char *name,
                                        int index, u32 *out_value)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *out_value = fdt32_to_cpu(*(value + index));

    return 0;
}

int of_property_read_u16(ofnode node, const char *name, u16 *data)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    if (size < AIC_BYTES_SIZE / 2) {
        printf("Error: value array length is less than 2 bytes.\n");
        return -1;
    }
    *data = fdt16_to_cpu(*value >> 16);

    return 0;
}

const int of_property_read_u16_array(ofnode node, const char *name,
                                     int *cnt, u16 *data_arr)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;
     for (int i = 0; i < *cnt; i++) {
        data_arr[i] = 0;
        data_arr[i] = fdt16_to_cpu(*value++ >> 16);
    }
    return 0;
}

const int of_property_get_u16_array_number(ofnode node, const char *name,
                                           int *cnt)
{
    int size;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;

    return 0;
}

const int of_property_read_u16_by_index(ofnode node, const char *name,
                                        int index, u16 *out_value)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *out_value = fdt16_to_cpu(*(value + index) >> 16);

    return 0;
}

int of_property_read_u8(ofnode node, const char *name, u8 *data)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    if (size < AIC_BYTES_SIZE / 4) {
        printf("Error: value array length is less than 1 bytes.\n");
        return -1;
    }
    *data = *value >> 24;

    return 0;
}

const int of_property_read_u8_array(ofnode node, const char *name, int *cnt,
                                    u8 *data_arr)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;
     for (int i = 0; i < *cnt; i++) {
        data_arr[i] = *value++ >> 24;
    }
    return 0;
}

const int of_property_get_u8_array_number(ofnode node, const char *name,
                                          int *cnt)
{
    int size;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *cnt = size / 4;

    return 0;
}

const int of_property_read_u8_by_index(ofnode node, const char *name,
                                       int index, u8 *out_value)
{
    int size;
    const fdt32_t *value = NULL;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    value = fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, &size);
    *out_value = *(value + index) >> 24;

    return 0;
}

const bool of_property_read_bool(ofnode node, const char *name)
{
    const struct fdt_property *data;

    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    data = fdt_get_property(g_aic_initial_blob, of_node_to_offset(node), name, NULL);

    return data ? true : false;
}

const void *of_property_read_string(ofnode node, const char *name, int *size)
{
    CHECK_PARAM(g_aic_initial_blob, NULL);

    return fdt_getprop(g_aic_initial_blob, of_node_to_offset(node), name, size);
}

const int of_property_read_string_count(ofnode node, const char *name)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    return fdt_stringlist_count(g_aic_initial_blob, of_node_to_offset(node),
                                name);
}

const char **of_property_read_string_array(ofnode node, const char *name)
{
    int len;
    int str_cnt;
    const char **value = NULL;

    CHECK_PARAM(g_aic_initial_blob, NULL);

    str_cnt = of_property_read_string_count(node, name);
    value = malloc(str_cnt * sizeof(char *));
    if (value == NULL)
        return NULL;

    for (int i = 0; i < str_cnt; i++)
        value[i] = fdt_stringlist_get(g_aic_initial_blob,
                                      of_node_to_offset(node), name, i, &len);

    return value;
}

const int of_property_read_string_index(ofnode node, const char *name,
                                        const char *propname, int *index)
{
    CHECK_PARAM(g_aic_initial_blob, -EINVAL);

    *index = fdt_stringlist_search(g_aic_initial_blob, of_node_to_offset(node),
                                   name, propname);
    if (index == NULL)
        return 0;
    return -1;
}

const char *of_property_read_string_by_index(ofnode node, const char *name,
                                             int index)
{
    CHECK_PARAM(g_aic_initial_blob, NULL);

    int len;
    const char *value;

    value = fdt_stringlist_get(g_aic_initial_blob, of_node_to_offset(node),
                               name, index, &len);
    if (value == NULL)
        return NULL;
    return value;
}

int aic_of_fdt_example(void)
{
    ofnode node = {0};
    ofnode subnode = {0};
    ofnode subsubnode = {0};
    ofnode parent_node = {0};
    ofnode cur_node = {0};
    int len;
    u64 val_u64 =0;
    u32 val_u32 =0;
    u16 val_u16 = 0;
    u8 val_u8 = 0;
    int cnt = 0;
    int cnt_u64 = 0;
    int cnt_u32 = 0;
    int cnt_u16 = 0;
    int cnt_u8 = 0;
    const char **str_arr;
    const char *prop_str;
    const char *subnode_name;
    bool bool_ret;
    int size_u64, size_u32, size_u16, size_u8;
    int str_idx = 0;
    u64 val_u64_out = 0;
    u32 val_u32_out = 0;
    u16 val_u16_out = 0;
    u8 val_u8_out = 0;

    of_find_node_by_path("/soc/gpai/gpai0", &node);
    prop_str = of_property_read_string(node, "status", &len);
    if (prop_str)
        printf("[status] %s\n",  prop_str);
    of_node_write_prop(node, "status", "okay", sizeof("okay"));
    prop_str = of_property_read_string(node, "status", &len);
    if (prop_str)
        printf("[status] %s - After modify\n\n",  prop_str);
    of_node_delete_prop(node, "status");
    prop_str = of_property_read_string(node, "status", &len);
    if (prop_str == NULL)
        printf("[status] :NONE \n\n");

    printf("[aic,high-level-thd]\n");
    of_property_read_u32(node, "aic,high-level-thd", &val_u32);
    printf("u32: 0x%08X  ",  val_u32);
    of_property_read_u16(node, "aic,high-level-thd", &val_u16);
    printf("u16: 0x%04X  ",  val_u16);
    of_property_read_u8(node, "aic,high-level-thd", &val_u8);
    printf("u8: 0x%02X\n",  val_u8);

    of_find_node_by_path("/soc", &node);
    bool_ret = of_property_read_bool(node, "u-boot,dm-pre-reloc");
    if (bool_ret) {
        printf("prop has u-boot,dm-pre-reloc\n");
    }
    printf("addr_cell: %d\n", of_read_addr_cells(node));
    printf("size_cell: %d\n\n", of_read_size_cells(node));

    of_property_read_u64(node, "clock-frequency", &val_u64);
    printf("[clock-frequency]-u64: 0x%16llX\n",  val_u64);
    size_u64 = of_property_read_size(node, "clock-frequency");
    if (size_u64) {
        u64 *value_reg_64 = (u64 *)malloc(2 * sizeof(u64));
        memset(value_reg_64, 0, 2 * sizeof(u64));
        of_property_read_u64_array(node, "clock-frequency", &cnt_u64,
                                   value_reg_64);
        printf("[clock-frequency] %d numbers\n", cnt_u64);
        cnt_u64 = 0;
        of_property_get_u64_array_number(node, "clock-frequency", &cnt_u64);
        printf("[clock-frequency] %d numbers\n", cnt_u64);
        for (int i = 0; i < cnt_u64; i++) {
            printf("0x%16llX ", value_reg_64[i]);
        }
        of_property_read_u64_by_index(node, "clock-frequency", 0 ,
                                      &val_u64_out);
        printf("\n");
        printf("[clock-frequency]-NO.%d : 0x%16llX \n", 0, val_u64_out);
        printf("\n");
        free(value_reg_64);
    }

    of_find_node_by_path("/soc/gpai", &node);
    int str_cnt = of_property_read_string_count(node, "clock-names");
    printf("[clock-names]-str %d numbers\n",  str_cnt);
    of_property_read_string_index(node, "clock-names", "gpai", &str_idx);
    printf("[clock-names]-gpai-index : %d\n",  str_idx);
    prop_str = of_property_read_string_by_index(node, "clock-names", str_idx);
    if (prop_str)
        printf("[clock-names]-%d index : %s\n", str_idx, prop_str);

    of_find_node_by_path("/soc/gpai", &node);
    str_arr= of_property_read_string_array(node, "clock-names");
    if (str_arr == NULL) {
        printf("Can't get string array\n");
    } else {
        int str_cnt = of_property_read_string_count(node, "clock-names");
        for (int i = 0; i < str_cnt; i++) {
            printf("%d-%s  ", i, str_arr[i]);
        }
        printf("\n\n");
    }

    size_u32 = of_property_read_size(node, "reg");
    if (size_u32 > 0) {
        u32 *value_reg = (u32 *)malloc(size_u32 * sizeof(u32));
        memset(value_reg, 0, size_u32 * sizeof(u32));
        of_property_read_u32_array(node, "reg", &cnt, value_reg);
        printf("[reg] %d numbers\n", cnt);
        for (int i = 0; i < cnt; i++) {
            printf("0x%08X ", value_reg[i]);
        }
        printf("\n\n");
        free(value_reg);
    }

    uint32_t appd_val = 0xaaabbb;
    of_node_append_prop_u32(node, "brightness-levels", appd_val);
    size_u32 = of_property_read_size(node, "brightness-levels");
    if (size_u32 > 0) {
        u32 *value_u32 = (u32 *)malloc(size_u32 * sizeof(u32));
        memset(value_u32, 0, size_u32 * sizeof(u32));
        of_property_read_u32_array(node, "brightness-levels", &cnt_u32,
                                   value_u32);
        printf("[brightness-levels] %d numbers\n", cnt_u32);
        for (int i = 0; i < cnt_u32; i++) {
            printf("0x%08X ", value_u32[i]);
            if (!((i + 1) % 4))
                printf("\n");
        }
        of_property_read_u32_by_index(node, "brightness-levels", 6 , &val_u32_out);
        printf("[brightness-levels]-NO.%d : %d \n", 6, val_u32_out);
        of_property_get_u32_array_number(node, "brightness-levels",&cnt_u32);
        printf("[brightness-levels]-u32 %d numbers\n", cnt_u32);
        printf("\n");
        free(value_u32);
    }

    size_u16 = of_property_read_size(node, "brightness-levels");
    if (size_u16 > 0) {
        u16 *value_u16 = (u16 *)malloc(size_u16 * sizeof(u16));
        memset(value_u16, 0, size_u16 * sizeof(u16));
        of_property_read_u16_array(node, "brightness-levels", &cnt_u16,
                                   value_u16);
        printf("[brightness-levels]-u16 %d numbers\n", cnt_u16);
        for (int i = 0; i < cnt_u16; i++) {
            printf("0x%04X ", value_u16[i]);
            if (!((i + 1) % 4))
                printf("\n");
        }
        of_property_read_u16_by_index(node, "brightness-levels", 6 ,
                                      &val_u16_out);
        printf("[brightness-levels]-NO.%d : %d \n", 6, val_u16_out);
        printf("\n");
        free(value_u16);
    }

    size_u8 = of_property_read_size(node, "brightness-levels");
    if (size_u8 > 0) {
        u8 *value_u8 = (u8 *)malloc(size_u8 * sizeof(u8));
        memset(value_u8, 0, size_u8 * sizeof(u8));
        of_property_read_u8_array(node, "brightness-levels", &cnt_u8, value_u8);
        printf("[brightness-levels]-u8 %d numbers\n", cnt_u8);
        for (int i = 0; i < cnt_u8; i++) {
            printf("0x%02X ", value_u8[i]);
            if (!((i + 1) % 4))
                printf("\n");
        }
        of_property_read_u8_by_index(node, "brightness-levels", 6 ,
                                     &val_u8_out);
        printf("[brightness-levels]-NO.%d : %d \n", 6, val_u8_out);
        printf("\n");
        free(value_u8);
    }

    cnt = 0;
    of_find_node_by_path("/soc", &parent_node);
    printf("[for_each_subnode]:\n");
    of_for_each_subnode(cur_node, parent_node) {
        if (of_get_nodename(cur_node))
            printf("[%d] %s\n", cnt++, of_get_nodename(cur_node));
    }

    of_find_node_by_path("/soc/gpai", &node);
    const char *name = of_get_nodename(node);
    printf("\nNode name: %s\n", name);

    of_find_node_by_path("/soc", &node);
    of_find_subnode(node, "gpai", &subnode);
    subnode_name = of_get_nodename(subnode);
    printf("Subnode name: %s\n", subnode_name);
    subsubnode = of_first_subnode(subnode);
    printf("Subsubnode name: %s\n", of_get_nodename(subsubnode));
    subsubnode = of_next_subnode(subnode);
    printf("Subsubnode name: %s\n", of_get_nodename(subsubnode));
    of_parent_node(subnode, &parent_node);
    printf("Parentnode name: %s\n", of_get_nodename(parent_node));

    if (of_fdt_device_is_available(subnode))
        printf("subnode_name is available\n");

    return 0;
}
