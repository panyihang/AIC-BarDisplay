/*
 * Copyright (c) 2020-2025 ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: <qi.xu@artinchip.com>
*/

#ifndef __MPP_ZLIB_H__
#define __MPP_ZLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * mpp_zlib_uncompressed - uncompressed data
 * @param[in] compressed_data,compressed_data addr
 * @param[in] compressed_len
 * @param[in] uncompressed_data,uncompressed_data addr
 * @param[in] uncompressed_len,it must larger than or eq  real_uncompressed_len
 *
 * @return:return real_uncompressed_len if uncompressed ok,or other return -1;
 *
 * @note
 * The compressed_data addr should be aligned with 2^n bytes.  n = 4,5,6.....
 * compressed_len should be valid data len.
 * call aicos_dcache_clean_range to flush cache after fill data into compressed_data
 *
 * uncompressed_data addr should be aligned with 2^n bytes .n=3,4,5.....
 * uncompressed_len should be larger than or eq real_uncompressed_len and should be aligned with  2^n bytes. n= 3,4,5.....
 * call aicos_dcache_invalid_range to invalid cache before get data from uncompressed_data
 */
 int mpp_zlib_uncompressed(
        unsigned char *compressed_data,
        unsigned int compressed_len,
        unsigned char *uncompressed_data,
        unsigned int uncompressed_len);

/******************************************/
enum COMPRESS_TYPE {
    MPP_ZLIB,
    MPP_GZIP
};

void* mpp_unzip_create(void);
void mpp_unzip_destroy(void *ctx);
int mpp_unzip_uncompressed(void *ctx, enum COMPRESS_TYPE type,
        unsigned char *src_buf, unsigned int src_buf_len,
        unsigned int offset, unsigned int data_len,
        unsigned char *dst, unsigned int dst_len,
        int first_part, int last_part);

#ifdef __cplusplus
}
#endif

#endif
