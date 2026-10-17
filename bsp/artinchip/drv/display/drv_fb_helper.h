/*
 * Copyright (C) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: huahui.mai@artinchip.com
 */

#include "drv_fb.h"

void aicfb_enable_clk(struct aicfb_info *fbi, u32 on);

void aicfb_enable_panel(struct aicfb_info *fbi, u32 on);

void aicfb_update_alpha(struct aicfb_info *fbi);

void aicfb_update_layer(struct aicfb_info *fbi);

void aicfb_get_panel_info(struct aicfb_info *fbi);

void aicfb_fb_info_setup(struct aicfb_info *fbi);

void aicfb_register_panel_callback(struct aicfb_info *fbi);

void fb_color_block(struct aicfb_info *fbi, size_t fb_size);

#ifdef AIC_DISPLAY_TEST
void aicfb_pq_get_config(struct aicfb_info *fbi, struct aicfb_pq_config *config);

void aicfb_pq_set_config(struct aicfb_info *fbi, struct aicfb_pq_config *config);
#else
static inline void
aicfb_pq_get_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{

}

static inline void
aicfb_pq_set_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{

}
#endif
