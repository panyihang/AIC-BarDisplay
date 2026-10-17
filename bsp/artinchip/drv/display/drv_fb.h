/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: huahui.mai@artinchip.com
 */

#ifndef _DRV_FB_H_
#define _DRV_FB_H_

#if defined(KERNEL_RTTHREAD)
#include <rtthread.h>
#include <rtdef.h>
#include <rthw.h>
#include <rtdevice.h>
#endif
#include <string.h>
#include <stdbool.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <artinchip_fb.h>
#include <mpp_types.h>
#include <aic_hal_dsi.h>
#include <aic_hal_rgb.h>

#include "panel/panel_com.h"
#include "disp_conf.h"

#define ALIGN_EVEN(x) ((x) & (~1))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_64B(x) (((x) + (63)) & ~(63))
#define ALIGN_128B(x) (((x) + (127)) & ~(127))
#define ALIGN_1024B(x) (((x) + (1023)) & ~(1023))

#define PLL_DISP_FREQ_MIN   (360 * 1000 * 1000)

enum AIC_COM_TYPE {
    AIC_DE_COM   = 0x00,  /* display engine component */
    AIC_RGB_COM  = 0x01,  /* rgb component */
    AIC_LVDS_COM = 0x02,  /* lvds component */
    AIC_MIPI_COM = 0x03,  /* mipi dsi component */
    AIC_DBI_COM  = 0x04,  /* mipi dbi component */
};

#define AICFB_ON    1
#define AICFB_OFF   0

struct aicfb_info
{
    struct platform_driver *de;
    struct platform_driver *di;
    struct aic_panel *panel;
    struct panel_desc *desc;

#if defined(KERNEL_RTTHREAD)
    struct rt_device device;
#endif

    bool power_on;
    u32 fb_rotate;

    void *fb_start;
    u32 width;
    u32 height;
    u32 stride;
    size_t fb_size;
    u32 bits_per_pixel;
};

struct panel_dbi_commands {
	const u8 *buf;
	size_t len;
    unsigned int command_flag;
};

struct spi_cfg {
	unsigned int qspi_mode;
	unsigned int vbp_num;
	unsigned int code1_cfg;
	unsigned int code[3];
};

struct panel_dbi {
    unsigned int type;
    unsigned int format;
    unsigned int first_line;
    unsigned int other_line;
#ifdef AIC_DISP_MIPI_DBI_DRV_V13
    unsigned int flags;
#endif
    struct panel_dbi_commands commands;
    struct spi_cfg *spi;
};

struct rgb_spi_command {
    unsigned int len;
    unsigned int command_on;
    unsigned char *buf;
};

struct panel_rgb {
    unsigned int mode;
    unsigned int format;
    unsigned int clock_phase;
    unsigned int data_order;
    unsigned int data_mirror;
    struct rgb_spi_command spi_command;
};

enum lvds_mode {
    NS          = 0x0,
    JEIDA_24BIT = 0x1,
    JEIDA_18BIT = 0x2
};

enum lvds_link_mode {
    SINGLE_LINK0  = 0x0,
    SINGLE_LINK1  = 0x1,
    DOUBLE_SCREEN = 0x2,
    DUAL_LINK     = 0x3
};

struct panel_lvds {
    enum lvds_mode mode;
    enum lvds_link_mode link_mode;
    unsigned int link_swap;
    unsigned int pols[2];
    unsigned int lanes[2];
};

struct dsi_command
{
    unsigned int len;
    unsigned int command_on;
    unsigned char *buf;
};

struct panel_dsi {
    enum dsi_mode mode;
    enum dsi_format format;
    unsigned int lane_num;

    /**
     * just for panel dsi simple
     */
    unsigned int vc_num;
    unsigned int dc_inv;
    unsigned int ln_polrs;
    unsigned int ln_assign;
    struct dsi_command command;
};

struct aic_panel;
struct panel_desc;

struct de_funcs {
    int (*set_mode)(struct aic_panel *panel, struct panel_desc *desc, u32 output_bpp, u32 dither_en);
    int (*clk_enable)(void);
    int (*clk_disable)(void);
    int (*timing_enable)(void);
    int (*timing_disable)(void);
    int (*wait_for_vsync)(void);
    int (*get_layer_config)(struct aicfb_layer_data *layer_data);
    int (*update_layer_config)(struct aicfb_layer_data *layer_data);
    int (*update_layer_config_list)(struct aicfb_config_lists *list);
    int (*get_alpha_config)(struct aicfb_alpha_config *alpha);
    int (*update_alpha_config)(struct aicfb_alpha_config *alpha);
    int (*get_ck_config)(struct aicfb_ck_config *ck);
    int (*update_ck_config)(struct aicfb_ck_config *ck);
    int (*set_display_prop)(struct aicfb_disp_prop *disp_prop);
    int (*get_display_prop)(struct aicfb_disp_prop *disp_prop);
    int (*set_ccm_config)(struct aicfb_ccm_config *ccm);
    int (*get_ccm_config)(struct aicfb_ccm_config *ccm);
    int (*set_gamma_config)(struct aicfb_gamma_config *gamma);
    int (*get_gamma_config)(struct aicfb_gamma_config *gamma);
    int (*update_bg_blend_ui_config)(u32 en);
    int (*get_bg_blend_ui_config)(u32 *en);
};

struct di_funcs {
    enum AIC_COM_TYPE type;
    int (*clk_enable)(void);
    int (*clk_disable)(void);
    int (*enable)(void);
    int (*disable)(void);
    int (*attach_panel)(struct aic_panel *panel, struct panel_desc *desc);
    int (*get_output_bpp)(void);
    int (*set_videomode)(const struct display_timing *timings, int enable);
    int (*send_cmd)(u32 dt, u32 vc, const u8 *data, u32 len);
    int (*read_cmd)(u32 val);
};

struct platform_driver {
    const char *name;
    int component_type;
    int (*probe)(void);
    void (*remove)(void);
    union {
        struct di_funcs *di_funcs;
        struct de_funcs *de_funcs;
    };
};

struct aic_panel_callbacks {
    int (*di_enable)(void);
    int (*di_disable)(void);
    int (*di_send_cmd)(u32 dt, u32 vc, const u8 *data, u32 len);
    int (*di_set_videomode)(const struct display_timing *timings, int enable);
    int (*timing_enable)(void);
    int (*timing_disable)(void);
};

/* Each panel driver should define the follow functions. */
struct aic_panel_funcs {
    int (*prepare)(void);
    int (*enable)(struct aic_panel *panel);
    int (*disable)(struct aic_panel *panel);
    int (*unprepare)(void);
    int (*register_callback)(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback);
};

struct panel_desc {
    const char *name;
    struct display_timing *timings;
    struct aic_panel_funcs *funcs;

    union {
        struct panel_rgb  *rgb;
        struct panel_lvds *lvds;
        struct panel_dsi  *dsi;
        struct panel_dbi  *dbi;
    };
};

struct aic_panel {
    const char *name;
    struct aic_panel_funcs *funcs;
    struct aic_panel_callbacks callbacks;
    struct display_timing *timings;

    union {
        struct panel_rgb  *rgb;
        struct panel_lvds *lvds;
        struct panel_dsi  *dsi;
        struct panel_dbi  *dbi;
    };

    struct panel_desc *desc;
    unsigned int match_num;
    unsigned int match_id;

    int connector_type;
    #ifdef AIC_PM_INDEPENDENT_POWER_KEY
    unsigned char independent_pwkey;
    #endif
};

static inline void aic_delay_ms(u32 ms)
{
    aicos_udelay(ms * 1000);
}

static inline void aic_delay_us(u32 us)
{
    aicos_udelay(us);
}

extern struct platform_driver artinchip_rgb_driver;
extern struct platform_driver artinchip_lvds_driver;
extern struct platform_driver artinchip_dsi_driver;
extern struct platform_driver artinchip_dbi_driver;
extern struct platform_driver artinchip_de_driver;

void panel_backlight_enable(struct aic_panel *panel, u32 ms);
void panel_backlight_disable(struct aic_panel *panel, u32 ms);
#endif /* _DRV_FB_H_ */

