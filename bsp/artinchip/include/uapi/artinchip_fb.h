/*
 * Definitions for the ArtinChip frambuffer driver
 *
 * Copyright (C) 2020-2026 ArtinChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UAPI__ARTINCHIP_FB_H_
#define _UAPI__ARTINCHIP_FB_H_

#include "mpp_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The AICFB is developed based on the Linux frame buffer. Besides the basic
 * functions provided by the Linux frame buffer, the AICFB also provides
 * extendedfunctions for controlling graphics layers such as updating layer
 * config data.
 */

#define AICFB_LAYER_TYPE_VIDEO    0
#define AICFB_LAYER_TYPE_UI    1

#define AICFB_PLANE_NUM 3

#define AICFB_ACTIVATE_ROTATE_PANDISP    (1 << 3)

enum display_flags {
	DISPLAY_FLAGS_HSYNC_LOW		= (1 << 0),
	DISPLAY_FLAGS_HSYNC_HIGH	= (1 << 1),
	DISPLAY_FLAGS_VSYNC_LOW		= (1 << 2),
	DISPLAY_FLAGS_VSYNC_HIGH	= (1 << 3),
};

struct display_timing {
    unsigned int pixelclock;

    unsigned int hactive;        /* hor. active video */
    unsigned int hfront_porch;   /* hor. front porch */
    unsigned int hback_porch;    /* hor. back porch */
    unsigned int hsync_len;      /* hor. sync len */
    unsigned int vactive;        /* ver. active video */
    unsigned int vfront_porch;   /* ver. front porch */
    unsigned int vback_porch;    /* ver. back porch */
    unsigned int vsync_len;      /* ver. sync len */

    unsigned int flags;
};

/**
 * struct aicfb_pq_config - aipq tools config
 * @timing: display timing
 * @connector_type: type of display interface
 * @data: display interface data
 *
 */
struct aicfb_pq_config {
    struct display_timing *timing;
    unsigned int connector_type;
    void *data;
};

/**
 * struct aicfb_layer_num - aicfb layer number
 * @vi_num: number of video layers
 * @ui_num: number of UI layers
 *
 *  total_layer_num = vi_num + ui_num
 *
 *  layer id range: [0, total_layer_num - 1]
 */
struct aicfb_layer_num {
    unsigned int vi_num;
    unsigned int ui_num;
};

/**
 * Flags to describe layer capability
 */

 /* layer supports scaling */
#define AICFB_CAP_SCALING_FLAG           (1 << 0)

/* layer with multi-rectangular windows */
#define AICFB_CAP_4_RECT_WIN_FLAG        (1 << 1)

/* layer supports alpha blending */
#define AICFB_CAP_ALPHA_FLAG             (1 << 2)

/* layer supports color key blending */
#define AICFB_CAP_CK_FLAG                (1 << 3)

/**
 * struct aicfb_layer_capability - aicfb layer capability
 * @layer_id: the layer id
 * @layer_type: the layer type
 *  0: UI layer
 *  1: Video layer
 * @max_width:  the max pixels per line
 * @max_height: the max lines
 * @cap_flags:  flags of layer capability
 */
struct aicfb_layer_capability {
    unsigned int layer_id;
    unsigned int layer_type;
    unsigned int max_width;
    unsigned int max_height;
    unsigned int cap_flags;
};

/**
 * struct aicfb_layer_data - aicfb layer data
 * @enable
 *  0: disable the layer
 *  1: enable the layer
 * @layer_id: the layer id
 *
 * @rect_id: the rectanglular window id of the layer
 *  only used by layers with multi-rectangular windows
 *  for example: if the layer has 4 rectangular windows,
 *  rect_id can be 0,1,2 or 3 for different windows
 *
 * @scale_size:  scaling size
 *  if the layer can be scaled. the scaling size can be different
 *  from the input buffer. the input buffer can be original aicfb_buffer
 *  or crop aicfb_buffer, otherwise, the scaling size will be ignore
 *
 * @pos: left-top x/y coordinate of the screen in pixels
 * @buf: frame buffer
 */
struct aicfb_layer_data {
    unsigned int enable;
    unsigned int layer_id;
    unsigned int rect_id;
    struct mpp_size scale_size;
    struct mpp_point pos;
    struct mpp_buf buf;
};

/**
 * struct aicfb_config_lists - aicfb config lists
 * @num: the total number of layer data config lists
 * @layers[]: the array of aicfb_layer_data lists
 */
struct aicfb_config_lists {
    unsigned int num;
    struct aicfb_layer_data layers[];
};

#define AICFB_PIXEL_ALPHA_MODE      0
#define AICFB_GLOBAL_ALPHA_MODE     1
#define AICFB_MIXDER_ALPHA_MODE     2

/**
 * struct aicfb_alpha_config - aicfb layer alpha blending config
 *
 * @layer_id: the layer id
 *
 * @enable
 *  0: disable alpha
 *  1: enable alpha
 *
 * @mode: alpha mode
 *  0: pixel alpha mode
 *  1: global alpha mode
 *  2: mixder alpha mode(alpha = pixel alpha * global alpha / 255)
 *
 * @value: global alpha value (0~255)
 *  used by global alpha mode and mixer alpha mode
 *
 */
struct aicfb_alpha_config {
    unsigned int layer_id;
    unsigned int enable;
    unsigned int mode;
    unsigned int value;
};

/**
 * struct aicfb_ck_config - aicfb layer color key blending config
 *
 * @layer_id: the layer id
 *
 * @ck_enable
 *  0: disable color key
 *  1: enable color key
 *
 *
 * @ck_value: color key rgb value to match the layer pixels
 *  bit[31:24]: reserved
 *  bit[23:16]: R value
 *  bit[15:8]: G value
 *  bit[7:0]: B value
 *
 */
struct aicfb_ck_config {
    unsigned int layer_id;
    unsigned int enable;
    unsigned int value;
};

/*
 * struct aicfb_disp_prop - aicfb display property
 *
 * @bright: bright in percent, range [0, 100], 50 means no effect
 * @contrast: contrast in percent, range [0, 100], 50 means no effect
 * @saturation: saturation in percent, range [0, 100], 50 means no effect
 * @hue: hue in percent, range [0, 100], 50 means no effect
 */
struct aicfb_disp_prop {
    unsigned int bright;
    unsigned int contrast;
    unsigned int saturation;
    unsigned int hue;
};

struct aicfb_ccm_config {
    unsigned int enable;
    int ccm_table[12];
};

enum gamma_lut {
    GAMMA_RED,
    GAMMA_GREEN,
    GAMMA_BLUE,
};

struct aicfb_gamma_config {
    unsigned int enable;
    unsigned int gamma_lut[3][16];
};

/*
 * struct aicfb_screeninfo - aicfb screen info
 *
 * @format: color format
 * @bits_per_pixel: bits per pixel
 * @stride: stride of screen
 * @width: screen width in pixels
 * @height: screen height in pixels
 * @framebuffer: start of frame buffer mem
 * @smem_len: length of frame buffer mem
 */
struct aicfb_screeninfo {
    enum mpp_pixel_format format;
    unsigned int bits_per_pixel;
    unsigned int stride;

    unsigned int width;
    unsigned int height;

    unsigned char *framebuffer;
    unsigned int smem_len;
};

#define IOC_TYPE_FB       'F'

#define AICFB_WAIT_FOR_VSYNC  _IOW(IOC_TYPE_FB, 0x20, unsigned int)

/** get layer number */
#define AICFB_GET_LAYER_NUM  _IOR(IOC_TYPE_FB, 0x21, struct aicfb_layer_num)

/** get layer capability */
#define AICFB_GET_LAYER_CAPABILITY  _IOWR(IOC_TYPE_FB, 0x22,\
                    struct aicfb_layer_capability)

/** get layer config data */
#define AICFB_GET_LAYER_CONFIG  _IOWR(IOC_TYPE_FB, 0x23, \
                    struct aicfb_layer_data)

/** update layer config data */
#define AICFB_UPDATE_LAYER_CONFIG  _IOW(IOC_TYPE_FB, 0x24, \
                    struct aicfb_layer_data)

/** update layer config data lists */
#define AICFB_UPDATE_LAYER_CONFIG_LISTS  _IOW(IOC_TYPE_FB, 0x25, \
						struct aicfb_config_lists)

/** get layer alpha blendig config */
#define AICFB_GET_ALPHA_CONFIG  _IOWR(IOC_TYPE_FB, 0x26, \
                    struct aicfb_alpha_config)

/** update layer alpha blendig config */
#define AICFB_UPDATE_ALPHA_CONFIG  _IOW(IOC_TYPE_FB, 0x27, \
                    struct aicfb_alpha_config)

/** get layer color key config */
#define AICFB_GET_CK_CONFIG  _IOWR(IOC_TYPE_FB, 0x28, struct aicfb_ck_config)

/** update layer color key config */
#define AICFB_UPDATE_CK_CONFIG  _IOW(IOC_TYPE_FB, 0x29, struct aicfb_ck_config)

/** pan display */
#define AICFB_PAN_DISPLAY  _IOR(IOC_TYPE_FB, 0x43, unsigned int)

/** set display property */
#define AICFB_SET_DISP_PROP _IOW(IOC_TYPE_FB, 0x60, struct aicfb_disp_prop)

/** get display property */
#define AICFB_GET_DISP_PROP _IOR(IOC_TYPE_FB, 0x61, struct aicfb_disp_prop)

/** update ccm config */
#define AICFB_UPDATE_CCM_CONFIG _IOR(IOC_TYPE_FB, 0x65, struct aicfb_ccm_config)

/** get ccm config */
#define AICFB_GET_CCM_CONFIG _IOR(IOC_TYPE_FB, 0x66, struct aicfb_ccm_config)

/** update gamma config */
#define AICFB_UPDATE_GAMMA_CONFIG _IOR(IOC_TYPE_FB, 0x67, struct aicfb_gamma_config)

/** get gamma config */
#define AICFB_GET_GAMMA_CONFIG _IOR(IOC_TYPE_FB, 0x68, struct aicfb_gamma_config)

/* get screen register value */
#define AICFB_GET_SCREENREG   _IOR(IOC_TYPE_FB, 0x69, unsigned int)

#define AICFB_PQ_GET_CONFIG _IOR(IOC_TYPE_FB, 0x70, unsigned int)
#define AICFB_PQ_SET_CONFIG _IOR(IOC_TYPE_FB, 0x71, unsigned int)

/* get screen info */
#define AICFB_GET_SCREENINFO _IOR(IOC_TYPE_FB, 0x62, struct aicfb_screeninfo)

/* enable aic fb, calls panel enable callback */
#define AICFB_POWERON   _IOR(IOC_TYPE_FB, 0x63, unsigned int)

/* disable aic fb, calls panel disable callback */
#define AICFB_POWEROFF   _IOR(IOC_TYPE_FB, 0x64, unsigned int)

/** update background blend ui config */
#define AICFB_UPDATE_BG_BLEND_UI_CONFIG _IOW(IOC_TYPE_FB, 0x72, unsigned int)

/** get background blend ui config */
#define AICFB_GET_BG_BLEND_UI_CONFIG _IOR(IOC_TYPE_FB, 0x73, unsigned int)

int aicfb_probe(void);
int aicfb_ioctl(int cmd, void *args);
void aicfb_remove(void);

#if defined(__cplusplus)
}
#endif
#endif /* _UAPI__ARTINCHIP_FB_H_ */

