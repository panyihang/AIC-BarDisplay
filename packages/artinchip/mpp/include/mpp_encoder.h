/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <qi.xu@artinchip.com>
 * Desc: mpp encoder
 */

#ifndef __MPP_ENCODER_H__
#define __MPP_ENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mpp_dec_type.h"

enum mpp_enc_errno {
	ENC_OK = 0,
	ENC_ERR_NOT_SUPPORT = -1,
	ENC_ERR_NULL_PTR = -2,
	ENC_ERR_NOT_CREATE = -3,
};

struct mpp_encoder;

/**
 * struct encode_config - encode congig
 * @pix_fmt: pixel format of output frame
 */
struct encode_config {
	int quality; // encode quality, 1~100
};

/**
 * mpp_encoder_create - create encoder (h264/jpeg/png ...)
 * @type: encoder type
 */
struct mpp_encoder* mpp_encoder_create(enum mpp_codec_type type);

/**
 * mpp_encoder_destory - destory encoder
 * @encoder: mpp_encoder context
 */
void mpp_encoder_destory(struct mpp_encoder* encoder);

/**
 * mpp_encoder_init - init encoder
 * @encoder: mpp_encoder context
 * @config: configuration of encoder
 */
int mpp_encoder_init(struct mpp_encoder *encoder, struct encode_config *config);

/**
 * mpp_encoder_encode - encode one packet
 * @encoder: mpp_encoder context
 */
int mpp_encoder_encode(struct mpp_encoder* encoder, struct mpp_frame *frame, struct mpp_packet *packet);


/**
 * mpp_encoder_control - send a control command (like, set/get parameter) to mpp_encoder
 * @encoder: mpp_encoder context
 * @cmd: command name, see mpp_type.h
 * @param: command data
 */
int mpp_encoder_control(struct mpp_encoder* encoder, int cmd, void* param);

/**
 * mpp_encoder_reset - reset mpp_encoder
 * @encoder: mpp_encoder context
 */
int mpp_encoder_reset(struct mpp_encoder* encoder);

#ifdef __cplusplus
}
#endif

#endif
