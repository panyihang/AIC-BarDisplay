/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aic_core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int offset;
} ofnode;

ofnode of_first_subnode(ofnode node);
ofnode of_next_subnode(ofnode node);
int of_node_to_offset(ofnode node);

int of_fdt_dt_init_bare_nornand(void);
int of_fdt_dt_init_bare_mmc(void);
#define of_for_each_subnode(node, parent) \
    for (node = of_first_subnode(parent); \
         of_node_to_offset(node) >= 0; \
         node = of_next_subnode(node))

int of_dtb_is_available(void);
void of_relocate_dtb(unsigned long pos);
int aic_of_fdt_example(void);
bool of_fdt_device_is_available(ofnode node);
int of_find_node_by_path(const char *path, ofnode *node);
int of_find_subnode(ofnode node, const char *subnode_name, ofnode *subnode);
int of_parent_node(ofnode node, ofnode *parent_node);
const char *of_get_nodename(ofnode node);
int of_get_subnode_by_name(ofnode node, const char *name);
int of_read_addr_cells(ofnode node);
int of_read_size_cells(ofnode node);
int of_node_delete_prop(ofnode node, const char *name);
int of_node_write_prop(ofnode node, const char *name, const void *val, int len);
int of_node_append_prop_string(ofnode node, const char *name, const void *str);
int of_node_append_prop_u32(ofnode node, const char *name, uint32_t val);
const int of_property_read_size(ofnode node, const char *name);
int of_property_read_u64(ofnode node, const char *name, u64 *data);
int of_property_read_u32(ofnode node, const char *name, u32 *data);
int of_property_read_u16(ofnode node, const char *name, u16 *data);
int of_property_read_u8(ofnode node, const char *name, u8 *data);
const int of_property_read_u64_array(ofnode node, const char *name,
                                     int *cnt, u64 *data_arr);
const int of_property_read_u32_array(ofnode node, const char *name,
                                     int *count, u32 *data_array);
const int of_property_read_u16_array(ofnode node, const char *name,
                                     int *count, u16 *data_array);
const int of_property_read_u8_array(ofnode node, const char *name,
                                    int *count, u8 *data_array);
const int of_property_get_u64_array_number(ofnode node, const char *name,
                                           int *cnt);
const int of_property_get_u32_array_number(ofnode node, const char *name,
                                           int *count);
const int of_property_get_u16_array_number(ofnode node, const char *name,
                                           int *count);
const int of_property_get_u8_array_number(ofnode node, const char *name,
                                          int *count);
const int of_property_read_u64_by_index(ofnode node, const char *name,
                                        int index, u64 *out_value);
const int of_property_read_u32_by_index(ofnode node, const char *name,
                                        int index, u32 *out_value);
const int of_property_read_u16_by_index(ofnode node, const char *name,
                                        int index, u16 *out_value);
const int of_property_read_u8_by_index(ofnode node, const char *name,
                                       int index, u8 *out_value);
const bool of_property_read_bool(ofnode node, const char *name);
const void *of_property_read_string(ofnode node, const char *name, int *size);
const int of_property_read_string_count(ofnode node, const char *name);
const char **of_property_read_string_array(ofnode node, const char *name);
const int of_property_read_string_index(ofnode node, const char *name,
                                        const char *propname, int *index);
const char *of_property_read_string_by_index(ofnode node, const char *name,
                                             int index);
#ifdef __cplusplus
}
#endif

