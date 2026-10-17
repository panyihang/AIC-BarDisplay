/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */
#ifndef __FITIMAGE_H__
#define __FITIMAGE_H__

#include "libfdt.h"

struct spl_fit_info {
    const void *fit;    /* Pointer to a valid FIT blob */
    size_t ext_data_offset; /* Offset to FIT external data (end of FIT) */
    int images_node;    /* FDT offset to "/images" node */
    int conf_node;      /* FDT offset to selected configuration node */
};

typedef enum
{
    DEVICE_MMC,
    DEVICE_SPINAND,
    DEVICE_SPINOR,
    DEVICE_XIPNOR,
    DEVICE_RAM,
} boot_dev_type;

/**
 * Information required to load data from a device
 *
 * @dev: Pointer to the device, e.g. struct mmc *
 * @priv: Private data for the device
 * @bl_len: Block length for reading in bytes
 */
struct spl_load_info {
    void *dev;
    void *priv;
    boot_dev_type dev_type;
    int bl_len;
};


#define FIT_IMAGES_PATH     "/images"
#define FIT_CONFS_PATH      "/configurations"

/* image node */
#define FIT_DATA_PROP       "data"
#define FIT_DATA_POSITION_PROP  "data-position"
#define FIT_DATA_OFFSET_PROP    "data-offset"
#define FIT_DATA_SIZE_PROP  "data-size"
#define FIT_TIMESTAMP_PROP  "timestamp"
#define FIT_DESC_PROP       "description"
#define FIT_ARCH_PROP       "arch"
#define FIT_TYPE_PROP       "type"
#define FIT_OS_PROP     "os"
#define FIT_COMP_PROP       "compression"
#define FIT_ENTRY_PROP      "entry"
#define FIT_LOAD_PROP       "load"

#define FIT_HASH_NODE_PROP  "hash"
#define FIT_HASH_ALGO_PROP  "algo"
#define FIT_HASH_VALUE_PROP "value"

/* cipher node */
#define FIT_CIPHER_NODE_PROP    "cipher"
#define FIT_CIPHER_ALGO_PROP    "algo"
#define FIT_CIPHER_IV_PROP      "iv"
#define FIT_CIPHER_KEY_HINT_PROP "key-name-hint"

/* Cipher algorithm parameters */
#define FIT_CIPHER_AES128_KEY_SIZE  16
#define FIT_CIPHER_AES192_KEY_SIZE  24
#define FIT_CIPHER_AES256_KEY_SIZE  32
#define FIT_CIPHER_AES_IV_SIZE      16
#define FIT_CIPHER_CHACHA20_KEY_SIZE 32
#define FIT_CIPHER_CHACHA20_IV_SIZE  12
#define FIT_CIPHER_MAX_KEY_SIZE     32
#define FIT_CIPHER_MAX_IV_SIZE      16

/* Parsed cipher metadata from FIT image node */
struct fit_cipher_info {
    const char *algo;
    const char *key_hint;
    u8  iv[FIT_CIPHER_MAX_IV_SIZE];
    u8  key[FIT_CIPHER_MAX_KEY_SIZE];
    int key_len;
    int iv_len;
};

/* configuration node */
#define FIT_KERNEL_PROP     "kernel"
#define FIT_RAMDISK_PROP    "ramdisk"
#define FIT_FDT_PROP        "fdt"
#define FIT_LOADABLE_PROP   "loadables"
#define FIT_DEFAULT_PROP    "default"
#define FIT_SETUP_PROP      "setup"
#define FIT_FPGA_PROP       "fpga"
#define FIT_FIRMWARE_PROP   "firmware"
#define FIT_STANDALONE_PROP "standalone"

static inline const char *fit_get_name(const void *fit_hdr,
        int noffset, int *len)
{
    return fdt_get_name(fit_hdr, noffset, len);
}

#define uimage_to_cpu(x)    __REV(x)
#define FIT_ALIGN(size, align)      (((size) + (align) - 1) & ~((align) - 1))

int spl_load_simple_fit(struct spl_load_info *info, ulong *entry_point);

/* Cipher node lookup, metadata extraction and decryption */
int fit_image_get_cipher_node(const void *fit, int noffset);
int fit_image_get_cipher_info(const void *fit, int cipher_node,
                              struct fit_cipher_info *ci);
int fit_image_decrypt_data(const struct fit_cipher_info *ci,
                           u8 *data, unsigned int length);

/* Weak functions for cipher operations (override in platform code) */
int fit_image_get_cipher_key(const char *algo, const char *key_name_hint,
                             u8 *key, int key_len);
int fit_image_aes_128_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, unsigned int len);
int fit_image_aes_192_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, unsigned int len);
int fit_image_aes_256_cbc_decrypt(const u8 *key, const u8 *iv, u8 *data, unsigned int len);
int fit_image_chacha20_decrypt(const u8 *key, const u8 *nonce, u8 *data, unsigned int len);

#endif
