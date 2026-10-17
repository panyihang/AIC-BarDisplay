/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Jianfeng Li <jianfeng.li@artinchip.com>
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <config_parse.h>

int boot_cfg_get_key_val(char *cfgtxt, int len, const char *key,  int klen,
                         char *val, int vlen)
{
    int idx = 0, cnt = 0;

    memset(val, 0, vlen);
    while (idx < len) {
        /* Begin of one string line */

        /* Skip blank */
        if (aic_isspace(cfgtxt[idx])) {
            idx++;
            continue;
        }

        /* Skip comment line */
        if (cfgtxt[idx] == '#') {
            while ((idx < len) && (cfgtxt[idx] != '\n'))
                idx++;
            continue;
        }

        if (strncmp(&cfgtxt[idx], key, klen)) {
            /* Skip line if key is not matched */
            while ((idx < len) && (cfgtxt[idx] != '\n'))
                idx++;
            continue;
        }

        /* Key is matched, but need to skip blank and '=' */
        idx += klen;
        while ((cfgtxt[idx] == ' ') || (cfgtxt[idx] == '\t') ||
               (cfgtxt[idx] == '=')) {
            idx++;
            if (idx >= len)
                break;
        }

        /* Now it is clean, get value */
        cnt = 0;
        while ((idx < len) && (cfgtxt[idx] != '\r') && (cfgtxt[idx] != '\n')) {
            val[cnt] = cfgtxt[idx];
            cnt++;
            idx++;
            if (cnt == vlen)
                break;
        }
        break;
    }

    return cnt;
}

/*
 * Check key value format:
 * - boot0=size@offset
 * - boot0=example.bin
 */
static int boot_cfg_is_offset_format(char *val, int len)
{
    int i = 0, sign_idx;

    sign_idx = 0;
    for (i = 0; i < len; i++) {
        if (aic_isspace(val[i]))
            continue;
        if ((val[i] == 'x') || (val[i] == 'X'))
            continue;
        if (val[i] == '@') {
            sign_idx = i;
            continue;
        }
        if (!aic_isxdigit(val[i])) {
            sign_idx = 0;
            break;
        }
    }

    return sign_idx;
}

/*
 * Get the given key's data file name and valid data length
 * - cfgtxt: The bootcfg.txt file data content
 * - clen: The bootcfg.txt file data length
 * - key: Key name
 * - fname: The output data file name
 * - nsiz: The fname buffer size
 * - offset: The firmware data's start offset in the file data(fname)
 * - flen: The firmware data's valid length
 */
int boot_cfg_parse_file(char *cfgtxt, int clen, char *key, char *fname,
                        int nsiz, int *offset, int *flen)
{
    char val[IMG_NAME_MAX_SIZ];
    int vlen = 0, sign_idx;

    vlen = boot_cfg_get_key_val(cfgtxt, clen, key, strlen(key), val,
                                IMG_NAME_MAX_SIZ);
    if (vlen == 0)
        return -1;

    sign_idx = boot_cfg_is_offset_format(val, vlen);
    if (sign_idx) {
        *flen = (int)strtol(val, NULL, 16);
        *offset = (int)strtol(&val[sign_idx + 1], NULL, 16);
        vlen = boot_cfg_get_key_val(cfgtxt, clen, "image", 5, val, IMG_NAME_MAX_SIZ);
        memcpy(fname, val, min(vlen, nsiz));
        fname[vlen] = 0;
    } else {
        *flen = 0;
        *offset = 0;
        memcpy(fname, val, min(vlen, nsiz));
        fname[vlen] = 0;
    }

    printf("0x%x @ %s\n", *offset, fname);
    return vlen;
}

int boot_cfg_get_boot0(char *cfgtxt, int clen, char *name, int nsiz,
                       int *offset, int *boot1len)
{
    return boot_cfg_parse_file(cfgtxt, clen, "boot0", name, nsiz, offset,
                               boot1len);
}

int boot_cfg_get_boot1(char *cfgtxt, int clen, char *name, int nsiz,
                       int *offset, int *boot1len)
{
    return boot_cfg_parse_file(cfgtxt, clen, "boot1", name, nsiz, offset,
                               boot1len);
}

int boot_cfg_get_image(char *cfgtxt, int clen, char *name, int nsiz)
{
    int ret = 0;

    if (NULL == name)
        return -1;

    ret = boot_cfg_get_key_val(cfgtxt, clen, "image", 5, name, nsiz);
    if (ret <= 0)
        return -1;

    return 0;
}

int boot_cfg_get_protection(char *cfgtxt, int clen, char *name, int nsiz)
{
    int ret = 0;

    if (NULL == name)
        return -1;

    ret = boot_cfg_get_key_val(cfgtxt, clen, "protection", 10, name, nsiz);
    if (ret <= 0)
        return -1;

    return 0;
}
