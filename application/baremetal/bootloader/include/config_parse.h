/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef __CONFIG_PARSE_H__
#define __CONFIG_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_core.h>

#define IMG_NAME_MAX_SIZ 128
#define PROTECTION_PARTITION_LEN 128

int boot_cfg_parse_file(char *cfgtxt, int clen, char *key, char *fname,
                        int nsiz, int *offset, int *flen);
int boot_cfg_get_boot0(char *cfgtxt, int clen, char *name, int nsiz,
                       int *offset, int *boot1len);
int boot_cfg_get_boot1(char *cfgtxt, int clen, char *name, int nsiz,
                       int *offset, int *boot1len);
int boot_cfg_get_image(char *cfgtxt, int clen, char *name, int nsiz);
int boot_cfg_get_protection(char *cfgtxt, int clen, char *name, int nsiz);
int boot_cfg_get_key_val(char *cfgtxt, int len, const char *key, int klen,
                         char *val, int vlen);

#ifdef __cplusplus
}
#endif

#endif/* __CONFIG_PARSE_H__ */
