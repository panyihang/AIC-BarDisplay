/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Huahui <huahui.mai@artinchip.com>
 */

#ifndef _UAPI_ARTINCHIP_DE_H_
#define _UAPI_ARTINCHIP_DE_H_

typedef void (*de_vsync_cb)(void *data);

/* Atomic callback, will called from DE VSYNC INTERRUPT context */
void de_register_vsync_cb(de_vsync_cb cb, void *data);
void de_unregister_vsync_cb(void);

#endif /* _UAPI_ARTINCHIP_DE_H_ */
