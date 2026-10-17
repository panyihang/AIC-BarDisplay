/*
 * Definitions for the ArtInChip media process platform interface
 *
 * Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd.
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UAPI_MPP_TYPES_H_
#define _UAPI_MPP_TYPES_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* rotate flags for GE/VE ctrl */
#define MPP_ROTATION_0            (0 << 0)
#define MPP_ROTATION_90           (1 << 0)
#define MPP_ROTATION_180          (2 << 0)
#define MPP_ROTATION_270          (3 << 0)
#define MPP_FLIP_H                (1 << 8)
#define MPP_FLIP_V                (1 << 9)

#define MPP_ROTATION_GET(flags)   ((flags) & 0x3)
#define MPP_FLIP_H_GET(flags)     (((flags) >> 8) & 0x1)
#define MPP_FLIP_V_GET(flags)     (((flags) >> 9) & 0x1)

/* scan order flags for GE ctrl */
#define MPP_LR_TB                 (0 << 16)  /* left to right, top to buttom */
#define MPP_RL_TB                 (1 << 16)  /* right to left, top to buttom */
#define MPP_LR_BT                 (2 << 16)  /* left to right, buttom to top */
#define MPP_RL_BT                 (3 << 16)  /* right to left, buttom to top */

#define MPP_SCAN_ORDER_GET(flags) (((flags) >> 16) & 0x3)

struct mpp_rect {
    int x;
    int y;
    int width;
    int height;
};

struct mpp_point {
    int x;
    int y;
};

struct mpp_size {
    int width;
    int height;
};

enum mpp_color_space {
    MPP_BT601, /* the standard for SDTV */
    MPP_BT709, /* the standard for HDTV */
    MPP_BT601_FULL_RANGE,
    MPP_BT709_FULL_RANGE,
};

enum mpp_pixel_format {
    MPP_FMT_ARGB_8888            = 0x00,
    MPP_FMT_ABGR_8888            = 0x01,
    MPP_FMT_RGBA_8888            = 0x02,
    MPP_FMT_BGRA_8888            = 0x03,
    MPP_FMT_XRGB_8888            = 0x04,
    MPP_FMT_XBGR_8888            = 0x05,
    MPP_FMT_RGBX_8888            = 0x06,
    MPP_FMT_BGRX_8888            = 0x07,
    MPP_FMT_RGB_888              = 0x08,
    MPP_FMT_BGR_888              = 0x09,
    MPP_FMT_ARGB_1555            = 0x0a,
    MPP_FMT_ABGR_1555            = 0x0b,
    MPP_FMT_RGBA_5551            = 0x0c,
    MPP_FMT_BGRA_5551            = 0x0d,
    MPP_FMT_RGB_565              = 0x0e,
    MPP_FMT_BGR_565              = 0x0f,
    MPP_FMT_ARGB_4444            = 0x10,
    MPP_FMT_ABGR_4444            = 0x11,
    MPP_FMT_RGBA_4444            = 0x12,
    MPP_FMT_BGRA_4444            = 0x13,

    MPP_FMT_YUV420P              = 0x20,
    MPP_FMT_NV12                 = 0x21,
    MPP_FMT_NV21                 = 0x22,
    MPP_FMT_YUV422P              = 0x23,
    MPP_FMT_NV16                 = 0x24,
    MPP_FMT_NV61                 = 0x25,
    MPP_FMT_YUYV                 = 0x26,
    MPP_FMT_YVYU                 = 0x27,
    MPP_FMT_UYVY                 = 0x28,
    MPP_FMT_VYUY                 = 0x29,
    MPP_FMT_YUV400               = 0x2a,
    MPP_FMT_YUV444P              = 0x2b,

    MPP_FMT_YUV420_64x32_TILE    = 0x30,
    MPP_FMT_YUV420_128x16_TILE   = 0x31,
    MPP_FMT_YUV422_64x32_TILE    = 0x32,
    MPP_FMT_YUV422_128x16_TILE   = 0x33,
    MPP_FMT_MAX,
};

/*
 * enum mpp_buf_type - mpp buf types:
 *
 * MPP_DMA_BUF_FD: dma-buf fd
 * MPP_PHY_ADDR: physical address
 */
enum mpp_buf_type {
    MPP_DMA_BUF_FD,
    MPP_PHY_ADDR,
};

/**
 * Flags to describe struct mpp_buf
 */

/* color space flags for YUV format */
#define MPP_COLOR_SPACE_BT601                (0 << 0)
#define MPP_COLOR_SPACE_BT709                (1 << 0)
#define MPP_COLOR_SPACE_BT601_FULL_RANGE     (2 << 0)
#define MPP_COLOR_SPACE_BT709_FULL_RANGE     (3 << 0)

/**
 * @MEDIA_BUS_UNKNOWN:  unknown bus type, no mediabus configuration
 * @MEDIA_BUS_PARALLEL: parallel interface with hsync and vsync
 * @MEDIA_BUS_BT656:    parallel interface with embedded synchronisation, can
 *          also be used for BT.1120
 * @MEDIA_BUS_CSI1: MIPI CSI-1 serial interface
 * @MEDIA_BUS_CCP2: CCP2 (Compact Camera Port 2)
 * @MEDIA_BUS_CSI2_DPHY: MIPI CSI-2 serial interface, with D-PHY
 * @MEDIA_BUS_CSI2_CPHY: MIPI CSI-2 serial interface, with C-PHY
 * @MEDIA_BUS_INVALID:  invalid bus type (keep as last)
 */
enum mpp_mbus_type {
    MEDIA_BUS_UNKNOWN,
    MEDIA_BUS_PARALLEL,
    MEDIA_BUS_BT656,
    MEDIA_BUS_BT1120,
    MEDIA_BUS_CSI1,
    MEDIA_BUS_CCP2,
    MEDIA_BUS_CSI2_DPHY,
    MEDIA_BUS_CSI2_CPHY,
    MEDIA_BUS_RAW8_MONO,
    MEDIA_BUS_RAW8_RGB,
    MEDIA_BUS_INVALID,
};

/* Media bus format */
enum mpp_mbus_fmt {
    MEDIA_BUS_FMT_RGB444_2X8_PADHI_BE = 0x1001,
    MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE = 0x1002,
    MEDIA_BUS_FMT_RGB555_2X8_PADHI_BE = 0x1003,
    MEDIA_BUS_FMT_RGB555_2X8_PADHI_LE = 0x1004,

    MEDIA_BUS_FMT_RGB565_2X8_BE = 0x1007,
    MEDIA_BUS_FMT_RGB565_2X8_LE = 0x1008,
    /* YUV (including grey) */
    MEDIA_BUS_FMT_Y8_1X8        = 0x2001,
    MEDIA_BUS_FMT_UYVY8_2X8     = 0x2006,
    MEDIA_BUS_FMT_VYUY8_2X8     = 0x2007,
    MEDIA_BUS_FMT_YUYV8_2X8     = 0x2008,
    MEDIA_BUS_FMT_YVYU8_2X8     = 0x2009,
    /* Bayer */
    MEDIA_BUS_FMT_SBGGR8_1X8    = 0x3001,
    MEDIA_BUS_FMT_SGBRG8_1X8    = 0x3013,
    MEDIA_BUS_FMT_SGRBG8_1X8    = 0x3002,
    MEDIA_BUS_FMT_SRGGB8_1X8    = 0x3014,
    /* JPEG compressed formats */
    MEDIA_BUS_FMT_JPEG_1X8      = 0x4001,
};

/* premultiply flags for ARGB format */
#define MPP_BUF_IS_PREMULTIPLY               (1 << 8)

#define MPP_BUF_PAN_DISPLAY_DMABUF         (1 << 9)

#define MPP_BUF_COLOR_SPACE_GET(flags)       ((flags) & 0x3)
#define MPP_BUF_PREMULTIPLY_GET(flags)       (((flags) >> 8) & 0x1)
#define MPP_BUF_PAN_DISPLAY_DMABUF_GET(flags)       (((flags) >> 9) & 0x1)

/**
 * struct mpp_buf - mpp frame buffer
 * @buf_type: mpp buffer type
 * @fd[3]: the dma buffer fd of frame
 * @stride[3]: stride for all planes
 * @size: width and height of mpp buffer
 * @crop_en: crop disable/enable
 *  0: disable crop
 *  1: enable crop
 * @crop: crop info
 * @format: color format
 * @flags: buffer flags
 */
struct mpp_buf {
    enum mpp_buf_type       buf_type;
    union {
        int             fd[3];
        unsigned int    phy_addr[3];
    };
    unsigned int            stride[3];
    struct mpp_size         size;
    unsigned int            crop_en;
    struct mpp_rect         crop;
    enum mpp_pixel_format   format;
    unsigned int            flags;
};

struct mpp_frame {
    struct mpp_buf          buf;
    long long               pts;
    unsigned int            id;
    unsigned int            flags;
};

/**
 * Transfer dma-buf fd from userspace to the importers in kernel.
 *
 * @fd: the actual fd, requested by mmap() in userspace
 * @phy_addr: the physical address of the actual fd
 *
 */
struct dma_buf_info {
    int fd;
    unsigned int phy_addr;
};

/**
 * The output video format of Camera / the input video format of DVP
 * @width:  image width
 * @height: image height
 * @code:   data format code
 * @colorspace: colorspace of the data
 * @flag:   the polarity of input signal
 * @mux:    the number of mux channels
 */
struct mpp_video_fmt {
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    enum mpp_mbus_fmt code;
    enum mpp_mbus_type bus_type;
    unsigned int colorspace;
    unsigned int flags;
    unsigned int mux;
};

/**
 * enum mpp_stitch_mode - Video input stitch mode
 *
 * MPP_STITCH_INVALID: Invalid mode, only channel 0 is enabled
 * MPP_STITCH_NONE:    No stitch, each channel outputs to its own buffer
 * MPP_STITCH_V_MODE:  Vertical stitch, CH0 on top, CH1 on bottom
 * MPP_STITCH_H_MODE:  Horizontal stitch, CH0 on left, CH1 on right
 *
 * Buffer layout for stitch mode:
 *
 *    MPP_STITCH_V_MODE              MPP_STITCH_H_MODE
 *    +-----------+                  +-----------+-----------+
 *    | CH0       |                  | CH0       | CH1       | Y
 *    +-----------+ Y                +-----------+-----------+
 *    | CH1       |                  | CH0       | CH1       | UV
 *    +-----------+                  +-----------+-----------+
 *    | CH0       |
 *    +-----------+ UV
 *    | CH1       |
 *    +-----------+
 *
 * This enum is used by DVP, CSI and other video input modules.
 */
enum mpp_stitch_mode {
    MPP_STITCH_INVALID = -1,
    MPP_STITCH_NONE = 0,
    MPP_STITCH_V_MODE = 1,
    MPP_STITCH_H_MODE = 2,
};
#define MPP_IS_STITCH(m)  (((m) == MPP_STITCH_V_MODE) || ((m) == MPP_STITCH_H_MODE))

#if defined(__cplusplus)
}
#endif

#endif /* _UAPI_MPP_TYPES_H_ */

