/*
 * Copyright (C) 2020-2025 ArtInChip Technology Co., Ltd.
 * Authors:  matteo <duanmt@artinchip.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RGB_REG_H_
#define _RGB_REG_H_

#include <aic_common.h>
#include <aic_io.h>
#include "aic_hal_disp_reg_util.h"

enum aic_rgb_cko_phase_sel {
    CKO_PHASE_SEL_0 = 0x0,
    CKO_PHASE_SEL_90 = 0x1,
    CKO_PHASE_SEL_180 = 0x2,
    CKO_PHASE_SEL_270 = 0x3
};

/* RGB mode */
#define PRGB    0x0
#define SRGB    0x1

/*
 * PRGB format
 *
 * "HD" or "LD" specifies which of the 24 pins will be discarded: "HD" means
 * that the highest 6/8 pins of the 24 will be discarded, "LD" means that the
 * lowest 6/8 pins will be discarded.
 */
#define PRGB_24BIT      0x0
#define PRGB_18BIT_LD   0x1
#define PRGB_18BIT_HD   0x2
#define PRGB_16BIT_LD   0x3
#define PRGB_16BIT_HD   0x4

/* SRGB format */
#define SRGB_8BIT       0x0
#define SRGB_6BIT       0x1

/* RGB interface pixel clock output phase */
#define DEGREE_0        0x0
#define DEGREE_90       0x1
#define DEGREE_180      0x2
#define DEGREE_270      0x3

/* RGB interface output data sequence */
#define RGB    0x02100210
#define RBG    0x02010201
#define BGR    0x00120012
#define BRG    0x00210021
#define GRB    0x01200120
#define GBR    0x01020102

#define RGB_LCD_CTL_MODE_MASK           GENMASK(5, 4)
#define RGB_LCD_CTL_MODE(x)             (((x)&0x3) << 4)
#define RGB_LCD_CTL_EN                  BIT(0)

#define RGB_CLK_CTL_CKO_PHASE_MASK      GENMASK(1, 0)
#define RGB_CLK_CTL_CKO_PHASE(x)        (((x)&0x3) << 0)

#define RGB_LCD_CTL_PRGB_MODE_MASK      GENMASK(10, 8)
#define RGB_LCD_CTL_PRGB_MODE(x)        (((x)&0x7) << 8)
#define RGB_LCD_CTL_SRGB_MODE           BIT(12)

#define RGB_DATA_SEL_EVEN_DP2316_MASK   GENMASK(25, 24)
#define RGB_DATA_SEL_EVEN_DP2316(x)     (((x)&0x3) << 24)
#define RGB_DATA_SEL_EVEN_DP1508_MASK   GENMASK(21, 20)
#define RGB_DATA_SEL_EVEN_DP1508(x)     (((x)&0x3) << 20)
#define RGB_DATA_SEL_EVEN_DP0700_MASK   GENMASK(17, 16)
#define RGB_DATA_SEL_EVEN_DP0700(x)     (((x)&0x3) << 16)
#define RGB_DATA_SEL_OOD_DP2316_MASK    GENMASK(9, 8)
#define RGB_DATA_SEL_OOD_DP2316(x)      (((x)&0x3) << 8)
#define RGB_DATA_SEL_OOD_DP1508_MASK    GENMASK(5, 4)
#define RGB_DATA_SEL_OOD_DP1508(x)      (((x)&0x3) << 4)
#define RGB_DATA_SEL_OOD_DP0700_MASK    GENMASK(1, 0)
#define RGB_DATA_SEL_OOD_DP0700(x)      (((x)&0x3) << 0)

#define RGB_DATA_OUT_SEL_MASK           GENMASK(2, 0)
#define RGB_DATA_OUT_SEL(x)             (((x) & 0x7) << 0)
#define RGB_DATA_OUT_SEL_VALID_MASK     GENMASK(14, 4)
#define RGB_DATA_OUT_SEL_VALID(x)       (((x) & 0xFFF) << 4)

#define CKO_PHASE_SEL_MASK              GENMASK(1, 0)
#define CKO_PHASE_SEL(x)                (((x) & 0x3) << 0)

#define RGB_LCD_CTL         0x00
#define RGB_CLK_CTL         0x10
#define RGB_DATA_SEL        0x20
#define RGB_OOD_DATA        0x24
#define RGB_EVEN_DATA       0x28
#define RGB_DATA_SEQ_SEL    0x30
#define RGB_VERSION         0xFC

#endif // end of _RGB_REG_H_

