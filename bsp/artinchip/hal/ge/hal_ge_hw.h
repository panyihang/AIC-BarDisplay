/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef _GE_HW_H_
#define _GE_HW_H_

#include "aic_common.h"
#include "aic_log.h"
#include "aic_osal.h"
#include "aic_io.h"
#include "aic_hal_ge.h"

u32 ge_get_version_id(unsigned long base_addr);
/**
 *@ func_select
 * 0: nomal
 * 1: rotation1
 * 2: horizontal shear
 * 3: vertical shear
 *
 *@ rot0_ctrl
 * 0: 0 degree
 * 1: 90 degree
 * 2: 180 degree
 * 3: 270 degree
 *
 *@ source_mode
 * 0: buffer mode
 * 1: fill color
 * 2: horizontal gradient fill rect
 * 3: vertical gradient fill rect
 */
void ge_config_src_ctrl(unsigned long base_addr,
            u32 global_alpha, u32 alpha_mode,
            u32 premul_en, u32 scan_order,
            u32 func_select, u32 fmt, u32 v_flip,
            u32 h_flip, u32 rot0_ctrl,
            u32 source_mode, u32 csc0_en);

/**
 *@ func_select
 * 0: nomal
 * 1: rotation1
 * 2: horizontal shear
 * 3: vertical shear
 */
void ge_config_src_simple(unsigned long base_addr,
              u32 global_alpha, u32 alpha_mode,
              u32 premul_en, u32 func_select,
              u32 fmt, u32 source_mode);

void ge_config_output_ctrl(unsigned long base_addr,
               u32 premul, int fmt,
               u32 dither_en, u32 csc2_en);

void ge_dst_enable(unsigned long base_addr, u32 global_alpha,
           u32 alpha_mode, int fmt, int csc1_en);

void ge_dst_disable(unsigned long base_addr);

/**
 *@ direction
 * 0: horizontal gradient
 * 1: vertical gradient
 */
void ge_config_fill_gradient(unsigned long base_addr,
                 int width, int height,
                 u32 start_color, u32 end_color,
                 u32 direction);

void ge_config_dither(unsigned long base_addr, u32 dither_addr);

void ge_config_fillrect(unsigned long base_addr, u32 fill_color);

void ge_config_rot1(unsigned long base_addr,
            int angle_sin, int angle_cos,
            int src_center_x, int src_center_y,
            int dst_center_x, int dst_center_y);

void ge_config_shear(unsigned long base_addr,
             int offset_x, int offset_y,
             int angle_tan);

void ge_config_color_key(unsigned long base_addr, u32 ck_value);

/**
 *@ src_alpha_coef
 *@ dst_alpha_coef
 * 0: 0.0
 * 1: 1.0
 * 2: sa(src alpha)
 * 3: 1.0 - da
 * 4: da(dst alpha)
 * 5: 1.0 - da
 */
void ge_config_blend(unsigned long base_addr,
             u32 src_de_premul, u32 dst_de_premul,
             u32 alpha_ctrl, u32 src_alpha_coef,
             u32 dst_alpha_coef, u32 ck_en,
             u32 alpha_en);

void ge_set_scaler0(unsigned long base_addr,
            u32 input_w, u32 input_h,
            u32 output_w, u32 output_h,
            int dx, int dy,
            int h_phase, int v_phase,
            u32 channel);

void ge_scaler0_enable(unsigned long base_addr, u32 enable);

/**
 *@ csc select
 * 0: csc0
 * 1: csc1
 */
void ge_set_csc_coefs(unsigned long base_addr, int color_space, u32 csc);

void ge_write_csc0_coefs(unsigned long base_addr, u32 *coefs);

/**
 * set csc2
 */
void ge_set_csc2_coefs(unsigned long base_addr, int color_space);

void ge_set_src_info(unsigned long base_addr, u32 w, u32 h,
             u32 stride0, u32 stride1, u32 addr[]);

void ge_set_dst_info(unsigned long base_addr, u32 w, u32 h,
             u32 stride0, u32 stride1, u32 addr[]);

void ge_set_output_info(unsigned long base_addr, u32 w, u32 h,
            u32 stride0, u32 stride1, u32 addr[]);

void ge_set_hw_timeout_cycle(unsigned long base_addr, u32 clk_num);

u32 ge_read_hw_counter(unsigned long base_addr);

u32 ge_read_soft_reset_cycle(unsigned long base_addr);

u32 ge_read_status(unsigned long base_addr);

void ge_clear_status(unsigned long base_addr, u32 status);

void ge_enable_interrupt(unsigned long base_addr);

void ge_disable_interrupt(unsigned long base_addr);

void ge_start(unsigned long base_addr);

#endif /*_GE_HW_H_ */

