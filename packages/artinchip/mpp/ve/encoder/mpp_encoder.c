/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <che.jiang@artinchip.com>
 * Desc:  mpp_encoder interface
 */

#include "mpp_codec.h"
#include "frame_allocator.h"
#include "mpp_log.h"

extern struct mpp_encoder *create_jpeg_encoder();

struct mpp_encoder *mpp_encoder_create(enum mpp_codec_type type)
{
	if (type == MPP_CODEC_VIDEO_ENCODER_MJPEG)
		return create_jpeg_encoder();

	return NULL;
}

void mpp_encoder_destory(struct mpp_encoder *encoder)
{
	if (encoder == NULL)
		return;

	encoder->ops->destory(encoder);
}

int mpp_encoder_init(struct mpp_encoder *encoder, struct encode_config *config)
{
	if (encoder == NULL || config == NULL)
		return ENC_ERR_NULL_PTR;

	return encoder->ops->init(encoder, config);
}

int mpp_encoder_encode(struct mpp_encoder *encoder, struct mpp_frame *frame, struct mpp_packet *packet)
{
	if (encoder == NULL || frame == NULL || packet == NULL)
		return ENC_ERR_NULL_PTR;

	return encoder->ops->encode(encoder, frame, packet);
}

int mpp_encoder_control(struct mpp_encoder *encoder, int cmd, void *param)
{
	return 0;
}

int mpp_encoder_reset(struct mpp_encoder *encoder)
{
	return 0;
}
