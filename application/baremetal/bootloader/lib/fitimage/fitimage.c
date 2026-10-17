/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mtd.h>
#include <mmc.h>
#include <aic_core.h>
#include <libfdt.h>
#include <aic_utils.h>
#include <aic_crc32.h>
#include "fitimage.h"
#include <boot_param.h>

extern struct boot_args boot_arg;

int fit_find_config_node(const void *fdt)
{
    const char *name;
    int conf, node, len;
    const char *dflt_conf_name;
    const char *dflt_conf_desc = NULL;
    int dflt_conf_node = -1;

    conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
    if (conf < 0) {
        printf("%s: Cannot find /configurations node: %d\n", __func__,
              conf);
        return -1;
    }

    dflt_conf_name = fdt_getprop(fdt, conf, "default", &len);

    for (node = fdt_first_subnode(fdt, conf);
         node >= 0;
         node = fdt_next_subnode(fdt, node)) {
        name = fdt_getprop(fdt, node, "description", &len);
        if (!name) {
            printf("%s: Missing FDT description in DTB\n",
                   __func__);
            return -1;
        }

        if (dflt_conf_name) {
            const char *node_name = fdt_get_name(fdt, node, NULL);
            if (strncmp(dflt_conf_name, node_name, strlen(dflt_conf_name) + 1) == 0) {
                dflt_conf_node = node;
                dflt_conf_desc = name;
            }
        }
    }

    if (dflt_conf_node != -1) {
        printf("Selecting default config '%s'\n", dflt_conf_desc);
        return dflt_conf_node;
    }

    return -1;
}

static int spl_simple_fit_parse(struct spl_fit_info *ctx)
{
    int len;
    const char *version;

    /* Find the correct subnode under "/configurations" */
    ctx->conf_node = fit_find_config_node(ctx->fit);
    if (ctx->conf_node < 0)
    {
        printf("%s: Cannot find /configurations node: %d\n", __func__,
              ctx->conf_node);
        return -1;
    }

    /* find the node holding the images information */
    ctx->images_node = fdt_path_offset(ctx->fit, FIT_IMAGES_PATH);
    if (ctx->images_node < 0) {
        printf("%s: Cannot find /images node: %d\n", __func__,
              ctx->images_node);
        return -1;
    }

    version = fdt_getprop(ctx->fit, ctx->images_node, "version", &len);
    strncpy(&boot_arg.image_version[0], version, 16);

    return 0;
}

int spl_fit_get_image_name(const struct spl_fit_info *ctx,
                  const char *type, int index,
                  const char **outname)
{
    const char *name, *str;
    int len, i;
    bool found = true;

    name = fdt_getprop(ctx->fit, ctx->conf_node, type, &len);
    if (!name) {
        printf("cannot find property '%s': %d\n", type, len);
        return -1;
    }

    str = name;
    for (i = 0; i < index; i++) {
        str = strchr(str, '\0') + 1;
        if (!str || (str - name >= len)) {
            found = false;
            break;
        }
    }

    if (!found)
        return -1;

    *outname = str;
    return 0;
}

int spl_fit_get_image_node(const struct spl_fit_info *ctx,
                  const char *type, int index)
{
    const char *str;
    int err;
    int node;

    err = spl_fit_get_image_name(ctx, type, index, &str);
    if (err)
        return err;

    node = fdt_subnode_offset(ctx->fit, ctx->images_node, str);
    if (node < 0) {
        printf("cannot find image node '%s': %d\n", str, node);
        return -1;
    }

    return node;
}

static void fit_get_debug(const void *fit, int noffset,
        char *prop_name, int err)
{
    printf("Can't get '%s' property from FIT 0x%08lx, node: offset %d, name %s (%s)\n",
          prop_name, (ulong)fit, noffset, fit_get_name(fit, noffset, NULL),
          fdt_strerror(err));
}

static int fit_image_get_address(const void *fit, int noffset, char *name,
              ulong *load)
{
    int len, cell_len;
    const fdt32_t *cell;
    uint64_t load64 = 0;

    cell = fdt_getprop(fit, noffset, name, &len);
    if (cell == NULL) {
        fit_get_debug(fit, noffset, name, len);
        return -1;
    }

    cell_len = len >> 2;
    /* Use load64 to avoid compiling warning for 32-bit target */
    while (cell_len--) {
        load64 = (load64 << 32) | uimage_to_cpu(*cell);
        cell++;
    }

    if (len > sizeof(ulong) && (uint32_t)(load64 >> 32)) {
        printf("Unsupported %s address size\n", name);
        return -1;
    }

    *load = (ulong)load64;

    return 0;
}

int fit_image_get_load(const void *fit, int noffset, ulong *load)
{
    return fit_image_get_address(fit, noffset, FIT_LOAD_PROP, load);
}

int fit_image_get_data(const void *fit, int noffset,
        const void **data, size_t *size)
{
    int len;

    *data = fdt_getprop(fit, noffset, FIT_DATA_PROP, &len);
    if (*data == NULL) {
        fit_get_debug(fit, noffset, FIT_DATA_PROP, len);
        *size = 0;
        return -1;
    }

    *size = len;
    return 0;
}

int fit_image_get_data_position(const void *fit, int noffset,
                int *data_position)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_POSITION_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_position = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_OFFSET_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_offset = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_data_size(const void *fit, int noffset, unsigned int *data_size)
{
    const fdt32_t *val;

    val = fdt_getprop(fit, noffset, FIT_DATA_SIZE_PROP, NULL);
    if (!val)
        return -ENOENT;

    *data_size = fdt32_to_cpu(*val);

    return 0;
}

int fit_image_get_entry(const void *fit, int noffset, ulong *entry)
{
    return fit_image_get_address(fit, noffset, FIT_ENTRY_PROP, entry);
}

int fit_image_get_hash_node(const void *fit, int noffset, int index)
{
    char node_name[16];
    int node;

    snprintf(node_name, 16, "%s-%d", FIT_HASH_NODE_PROP, index);
    node = fdt_subnode_offset(fit, noffset, node_name);
    if (node < 0) {
        printf("cannot find node '%s': %d\n", node_name, node);
        return -1;
    }

    return node;
}

int fit_image_get_hash_crc32(const void *fit, int noffset, u32 *crc32)
{
    int node, ret = 0;
    int len, index;
    const char *algo;
    const fdt32_t *cell;

    if (!fit || !crc32)
        return -1;

    index = 1;
    do {
        node = fit_image_get_hash_node(fit, noffset, index);
        if (node < 0) {
            ret = -1;
            break;
        }
        algo = fdt_getprop(fit, node, "algo", &len);
        if (algo == NULL) {
            ret = -1;
            break;
        }
        if (memcmp(algo, "crc32", 5) == 0) {
            ret = 0;
            break;
        }
        index++;
    } while(1);
    if (ret)
        return ret;

    cell = fdt_getprop(fit, node, "value", &len);
    if (cell == NULL) {
        fit_get_debug(fit, node, "value", len);
        return -1;
    }
    *crc32 = uimage_to_cpu(*cell);

    return 0;
}

static void fit_cipher_get_sizes(const char *algo, int *key_len, int *iv_len)
{
    if (strncmp(algo, "aes128", 6) == 0) {
        *key_len = FIT_CIPHER_AES128_KEY_SIZE;
        *iv_len  = FIT_CIPHER_AES_IV_SIZE;
    } else if (strncmp(algo, "aes192", 6) == 0) {
        *key_len = FIT_CIPHER_AES192_KEY_SIZE;
        *iv_len  = FIT_CIPHER_AES_IV_SIZE;
    } else if (strncmp(algo, "aes256", 6) == 0) {
        *key_len = FIT_CIPHER_AES256_KEY_SIZE;
        *iv_len  = FIT_CIPHER_AES_IV_SIZE;
    } else if (strncmp(algo, "chacha20", 8) == 0) {
        *key_len = FIT_CIPHER_CHACHA20_KEY_SIZE;
        *iv_len  = FIT_CIPHER_CHACHA20_IV_SIZE;
    } else {
        *key_len = 0;
        *iv_len  = 0;
    }
}

static int fit_image_get_cipher_iv(const void *fit, int cipher_node,
                                   u8 *iv, int iv_len)
{
    int len;
    const void *data;

    data = fdt_getprop(fit, cipher_node, FIT_CIPHER_IV_PROP, &len);
    if (!data || len != iv_len)
        return -1;

    memcpy(iv, data, iv_len);
    return 0;
}

static void fit_image_get_cipher_key_hint(const void *fit, int cipher_node,
                                          const char **hint)
{
    *hint = fdt_getprop(fit, cipher_node, FIT_CIPHER_KEY_HINT_PROP, NULL);
}

/*
 * Find cipher sub-node in a FIT image node.
 *
 * Returns:
 *  >= 0 - cipher node offset
 *  <  0 - no cipher node (not encrypted)
 */
int fit_image_get_cipher_node(const void *fit, int noffset)
{
    return fdt_subnode_offset(fit, noffset, FIT_CIPHER_NODE_PROP);
}

/*
 * Parse cipher metadata from an already-located cipher node.
 *
 * Returns:
 *   0  - success
 *  -1  - error (unsupported algo, missing IV/key, etc.)
 */
int fit_image_get_cipher_info(const void *fit, int cipher_node,
                              struct fit_cipher_info *ci)
{
    memset(ci, 0, sizeof(*ci));

    ci->algo = fdt_getprop(fit, cipher_node, FIT_CIPHER_ALGO_PROP, NULL);
    if (!ci->algo) {
        printf("Cannot get cipher algo\n");
        return -1;
    }

    fit_cipher_get_sizes(ci->algo, &ci->key_len, &ci->iv_len);
    if (ci->key_len == 0) {
        printf("Unsupported cipher algo: %s\n", ci->algo);
        return -1;
    }

    if (fit_image_get_cipher_iv(fit, cipher_node, ci->iv, ci->iv_len) < 0) {
        printf("Cannot get cipher IV/nonce from ITB\n");
        return -1;
    }

    fit_image_get_cipher_key_hint(fit, cipher_node, &ci->key_hint);

    if (fit_image_get_cipher_key(ci->algo, ci->key_hint,
                                 ci->key, ci->key_len) < 0) {
        printf("Cannot get cipher key\n");
        return -1;
    }

    return 0; /* success */
}

/*
 * Weak function to get cipher decryption key.
 * Override in platform-specific code to provide the actual key
 * (e.g. from eFuse, secure storage, spienc, etc.).
 */
__attribute__((weak))
int fit_image_get_cipher_key(const char *algo, const char *key_name_hint,
                             u8 *key, int key_len)
{
    (void)algo;
    (void)key_name_hint;
    memset(key, 0, key_len);
    return 0;
}

/*
 * Weak functions for cipher decryption.
 * Override in platform-specific code to use a different crypto backend
 * (e.g. hardware CE, mbedtls, ChaCha20 library, etc.).
 */
__attribute__((weak))
int fit_image_aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data,
                                  unsigned int length)
{
    (void)key; (void)iv; (void)data; (void)length;
    return -1;
}

__attribute__((weak))
int fit_image_aes_192_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data,
                                  unsigned int length)
{
    (void)key; (void)iv; (void)data; (void)length;
    return -1;
}

__attribute__((weak))
int fit_image_aes_256_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data,
                                  unsigned int length)
{
    (void)key; (void)iv; (void)data; (void)length;
    return -1;
}

__attribute__((weak))
int fit_image_chacha20_decrypt(const u8 *key, const u8 *nonce, u8 *data,
                               unsigned int length)
{
    (void)key; (void)nonce; (void)data; (void)length;
    return -1;
}

int fit_image_decrypt_data(const struct fit_cipher_info *ci,
                           u8 *data, unsigned int length)
{
    int ret;

    /* Dispatch to algorithm-specific decryption */
    if (strncmp(ci->algo, "aes128", 6) == 0)
        ret = fit_image_aes_128_cbc_decrypt(ci->key, ci->iv, data, length);
    else if (strncmp(ci->algo, "aes192", 6) == 0)
        ret = fit_image_aes_192_cbc_decrypt(ci->key, ci->iv, data, length);
    else if (strncmp(ci->algo, "aes256", 6) == 0)
        ret = fit_image_aes_256_cbc_decrypt(ci->key, ci->iv, data, length);
    else if (strncmp(ci->algo, "chacha20", 8) == 0)
        ret = fit_image_chacha20_decrypt(ci->key, ci->iv, data, length);
    else
        ret = -1;

    if (ret != 0) {
        printf("%s decryption failed: %d\n", ci->algo, ret);
        return -1;
    }

    printf("%s decrypt OK, length: %u\n", ci->algo, length);
    return 0;
}

/*
 * offset: The offset relative to the itb file start location
 */
static int spl_read(struct spl_load_info *info, ulong offset, void *buf, int size)
{
    int rdlen = 0;
    u8 *p = buf;

    //determines whether the read length is 0
    if (!size)
        return size;

    if (info->dev_type == DEVICE_SPINAND || info->dev_type == DEVICE_SPINOR) {
#if defined(AIC_MTD_BARE_DRV)
        struct mtd_dev *mtd = (struct mtd_dev *)info->dev;

        if (info->dev_type == DEVICE_SPINAND)
            offset = mtd_trans_lga_to_pha(mtd, offset);

        if (offset == UINT32_MAX)
            return -1;

        rdlen = mtd_read(mtd, offset, p, size);
#endif
    } else if (info->dev_type == DEVICE_MMC) {
#if defined(AIC_SDMC_DRV)
        int blkcnt, blkstart, blkoffset, byte_offset;
        struct aic_sdmc *host = (struct aic_sdmc *)info->dev;
        struct aic_partition *part = (struct aic_partition *)info->priv;

        blkstart = part->start / info->bl_len;
        blkoffset = offset / info->bl_len;
        byte_offset = offset % info->bl_len;
        blkcnt = ALIGN_UP(size, info->bl_len)  / info->bl_len;
        blkcnt = mmc_bread(host, blkstart + blkoffset, blkcnt, p - byte_offset);
        rdlen = info->bl_len * blkcnt;
        rdlen = min(rdlen, size);
#endif
    } else if (info->dev_type == DEVICE_XIPNOR || info->dev_type == DEVICE_RAM) {
        if (info->priv) {
            memcpy(p, ((u8 *)info->priv) + offset, size);
            rdlen = size;
        }
    }

    return rdlen;
}

int spl_load_fit_image(struct spl_load_info *info, struct spl_fit_info *ctx, int node, ulong *entry_point)
{
    ulong load_addr = 0;
    unsigned int length, reserve_size;
    int ret, offset = 0;
    const void *fit = ctx->fit;
    bool external_data = false;
    u64 start_us;
    int cipher_node;
#ifdef LPKG_USING_FDTLIB_CRC32_VERIFY
    u32 crc1, crc2;
#endif

    if (fit_image_get_load(fit, node, &load_addr))
    {
        printf("Can't load %s: No load address\n", fit_get_name(fit, node, NULL));
        return -1;
    }

#ifdef LPKG_USING_FDTLIB_CRC32_VERIFY
    ret = fit_image_get_hash_crc32(fit, node, &crc1);
    if (ret < 0)
        crc1 = 0;
#endif

    if (!fit_image_get_data_position(fit, node, &offset))
    {
        external_data = true;
    }
    else if (!fit_image_get_data_offset(fit, node, &offset))
    {
        offset += ctx->ext_data_offset;
        external_data = true;
    }

    if (external_data)
    {
        if (fit_image_get_data_size(fit, node, &length))
            goto __get_entry;

        reserve_size = AIC_BOOTLOADER_TEXT_BASE - 0x100 - load_addr;
        if (length > reserve_size)
        {
            printf("The %s size exceeds reservation size.", fit_get_name(fit, node, NULL));
            return -1;
        }

        if (info->dev_type == DEVICE_XIPNOR && load_addr >= 0x60000000)
            goto __get_entry;
        else
        {
            start_us =  aic_get_time_us();
            ret = spl_read(info, offset, (u8 *)load_addr, length);
            show_speed("spl read", length, aic_get_time_us() - start_us);
            if (ret < 0)
            {
                printf("spl read external_data error\n");
                return -1;
            }

#ifdef LPKG_USING_FDTLIB_CRC32_VERIFY
            if (crc1 != 0) {
                crc2 = crc32(0, (u8 *)load_addr, length);
                if (crc2 != crc1) {
                    printf("APP crc32 error: expect 0x%x, got 0x%x\n", crc1, crc2);
                    return -1;
                }
                printf("CRC32 verify OK.\n");
            }
#else
            printf("CRC32 verify is disabled.\n");
#endif

            /* Decrypt data if cipher node is present in ITB.
             * CRC32 is verified on ciphertext first, then decrypt.
             */
            cipher_node = fit_image_get_cipher_node(fit, node);
            if (cipher_node >= 0) {
                struct fit_cipher_info ci;

                if (fit_image_get_cipher_info(fit, cipher_node, &ci) < 0) {
                    printf("fit image get cipher info error\n");
                    return -1;
                }
                ret = fit_image_decrypt_data(&ci, (u8 *)load_addr, length);
                memset(&ci, 0, sizeof(ci));
                if (ret < 0)
                {
                    printf("fit image decrypt error\n");
                    return -1;
                }
            }
        }
    }
    else
    {
        printf("External_data not found in ITB, cann't load image\n");
        return -1;
    }

__get_entry:
    if (entry_point)
    {
        if (fit_image_get_entry(fit, node, entry_point))
        {
            printf("Can't load %s: No entry_point address\n", fit_get_name(fit, node, NULL));
            return -1;
        }
    }

    return 0;
}

int spl_load_simple_fit(struct spl_load_info *info, ulong *entry_point)
{
    struct fdt_header *header;
    struct spl_fit_info ctx;
    void *buf = NULL;
    int size, buf_size, ret = 0;
    int index = 0, node = -1;

    header = aicos_malloc(MEM_DEFAULT,
                          ALIGN_UP(sizeof(struct fdt_header), info->bl_len));
    if (!header)
    {
        printf("No space to malloc for header\n");
        return -1;
    }

    /* read itb tree header, to parse itb tree totalsize */
    ret = spl_read(info, 0, (void *)header, sizeof(struct fdt_header));
    if (ret < 0)
    {
        printf("spl read header error\n");
        goto __exit_header;
    }

    if (fdt_magic(header) != FDT_MAGIC)
    {
        printf("Not found FIT\n");
        goto __exit_header;
    }

    size = FIT_ALIGN(fdt_totalsize(header), 4);
    ctx.ext_data_offset = size;

    buf_size = size;
    if (info->dev_type == DEVICE_MMC) {
        buf_size = ROUNDUP(size, info->bl_len);
    }

    buf = aicos_malloc(MEM_DEFAULT, buf_size);
    if (!buf)
    {
        printf("No space to malloc for itb\n");
        goto __exit_header;
    }

    /* read itb tree */
    ret = spl_read(info, 0, buf, buf_size);
    if (ret < 0)
    {
        printf("mtd read itb error\n");
        goto __exit;
    }

    ctx.fit = (void *)buf;

    ret = spl_simple_fit_parse(&ctx);
    if (ret < 0)
    {
        printf("fit parse error\n");
        goto __exit;
    }

    for (; ; index++)
    {
        node = spl_fit_get_image_node(&ctx, FIT_FIRMWARE_PROP, index);
        if (node < 0)
        {
            if (!index)
            {
                printf("FIT get image node error\n");
                ret = -1;
                goto __exit;
            }
            else
            {
                break;
            }
        }

        /* Only seg0 node has entry attribute,  */
        if (index)
            entry_point = NULL;

        ret = spl_load_fit_image(info, &ctx, node, entry_point);
        if (ret)
        {
            printf("FIT load image error\n");
            goto __exit;
        }
    }

__exit:
    aicos_free(MEM_DEFAULT, buf);
__exit_header:
    aicos_free(MEM_DEFAULT, header);
    return ret;
}
