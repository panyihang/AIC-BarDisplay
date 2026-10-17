/*
 * Copyright (C) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: huahui.mai@artinchip.com
 *
 * ArtInChip framebuffer helper functions
 */

#include "drv_fb_helper.h"
#include "disp_conf.h"

void aicfb_enable_clk(struct aicfb_info *fbi, u32 on)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;

    if (on == AICFB_ON)
    {
        de->de_funcs->clk_enable();
        di->di_funcs->clk_enable();
    }
    else
    {
        di->di_funcs->clk_disable();
        de->de_funcs->clk_disable();
    }
}

void aicfb_update_alpha(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct aicfb_alpha_config alpha = {0};

    alpha.layer_id = AICFB_LAYER_TYPE_UI;
    de->de_funcs->get_alpha_config(&alpha);

    de->de_funcs->update_alpha_config(&alpha);
}

void aicfb_update_layer(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct aicfb_layer_data layer = {0};

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 1;

    switch (fbi->fb_rotate)
    {
    case 0:
        layer.buf.size.width = fbi->width;
        layer.buf.size.height = fbi->height;
        layer.buf.stride[0] = fbi->stride;
        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start;
        break;
#ifdef AIC_FB_ROTATE_EN
    case 90:
    case 270:
    {
        unsigned int stride = ALIGN_8B(fbi->height * fbi->bits_per_pixel / 8);

        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * AIC_FB_DRAW_BUF_NUM;
        layer.buf.size.width = fbi->height;
        layer.buf.size.height = fbi->width;
        layer.buf.stride[0] = stride;
        break;
    }
    case 180:
        layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * AIC_FB_DRAW_BUF_NUM;
        layer.buf.size.width = fbi->width;
        layer.buf.size.height = fbi->height;
        layer.buf.stride[0] = fbi->stride;
        break;
#endif
    default:
        pr_err("Invalid rotation degree\n");
        return;
    }

    layer.buf.crop_en = 0;
    layer.buf.format = AICFB_FORMAT;
    layer.buf.flags = 0;

    de->de_funcs->update_layer_config(&layer);
}

static void aicfb_puts_panel_info(struct aicfb_info *fbi)
{
    const char *com_type[] = { "DE", "RGB", "LVDS", "DSI", "DBI" };
    struct display_timing *timing;
    u32 vtotal, htotal;

    timing = fbi->desc ? fbi->desc->timings : fbi->panel->timings;

    vtotal = timing->vactive + timing->vback_porch + timing->vfront_porch + timing->vsync_len;
    htotal = timing->hactive + timing->hback_porch + timing->hfront_porch + timing->hsync_len;

    printf("%s type: %s pclk: %d Mhz h: %d v: %d fps: %d\n",
            fbi->panel->name,
            com_type[fbi->panel->connector_type],
            timing->pixelclock / 1000000,
            timing->hactive,
            timing->vactive,
            timing->pixelclock / (vtotal * htotal));
}

void aicfb_enable_panel(struct aicfb_info *fbi, u32 on)
{
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;
    struct aic_panel_funcs *funcs;

    funcs = desc ? desc->funcs : panel->funcs;

    if (on == AICFB_ON)
    {
        funcs->prepare();
        funcs->enable(panel);
        aicfb_puts_panel_info(fbi);
    }
    else
    {
        funcs->disable(panel);
        funcs->unprepare();
    }
}

#if AICFB_GET_PANEL_MATCH_ID
#ifdef AIC_DISP_MIPI_DSI
static struct display_timing panel_dsi_rx_timing = {
    .pixelclock = 60000000,
    .hactive = 720,
    .hfront_porch = 180,
    .hback_porch = 180,
    .hsync_len = 100,
    .vactive = 720,
    .vfront_porch = 40,
    .vback_porch = 22,
    .vsync_len = 18,
};

static struct panel_dsi panel_dsi_dsi = {
    .mode = DSI_MOD_VID_BURST,
    .format = DSI_FMT_RGB888,
    .lane_num = 4,
};

/*
 * The panel_dsr_rx_desc is only used for reading the panel ID.
 * The display interface needs a set of default parameters to read the panel ID,
 * and the panel driver parameters will be reloaded according to the ID after reading.
 */
static struct panel_desc panel_dsr_rx_desc = {
        .name = "panel-dsi-rx",
        .dsi = &panel_dsi_dsi,
        .timings = &panel_dsi_rx_timing,
};

#include <mipi_display.h>
static int aicfb_dsi_set_maximum_return_packet_size(struct aicfb_info *fbi, u16 value)
{
    struct platform_driver *di = fbi->di;
    u8 type = MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE;
    u8 tx[2] = { value & 0xff, value >> 8 };

    if (!di->di_funcs->send_cmd)
        return -1;

    return di->di_funcs->send_cmd((u32)type, 0, tx, sizeof(tx));
}

static unsigned int panel_match_module(u32 id)
{
    unsigned int index = 0;

#define FL7707_ID 0x1f2138
#define JD9365_ID 0x046593
    /*
     * FIXME: The panel ID comes from the panel IC driver datasheet.
     * For example, FL7707's ID is 0x1f2138 and JD9365's ID is 0x046593.
     *
     * Modify the index according to the ID read back from the screen.
     */
    if (id == FL7707_ID)
        index = 0;
    else if (id == JD9365_ID)
        index = 1;
    else
        index = 0;

    return index;
}

static unsigned int aicfb_dsi_read_id(struct aicfb_info *fbi, u8 cmd)
{
    struct platform_driver *di = fbi->di;
    unsigned int id = 0;

    if (di->di_funcs->attach_panel)
        di->di_funcs->attach_panel(fbi->panel, &panel_dsr_rx_desc);

    if (di->di_funcs->clk_enable)
        di->di_funcs->clk_enable();

    if (di->di_funcs->enable)
        di->di_funcs->enable();

    if (di->di_funcs->set_videomode)
        di->di_funcs->set_videomode(&panel_dsi_rx_timing, false);

    /*
     * TODO: If the panel needs GPIO initialization, please add it here.
     * We can implement a GPIO init function in the specific panel driver
     * and declare it as extern. For example:
     *
     * extern void jd9365_panel_gpio_init();
     * jd9365_panel_gpio_init();
     */

    aicfb_dsi_set_maximum_return_packet_size(fbi, 3);

    if (di->di_funcs->read_cmd)
        id = di->di_funcs->read_cmd(cmd);

    if (di->di_funcs->clk_disable)
        di->di_funcs->clk_disable();

    return panel_match_module(id);
}
#endif /* AIC_DISP_MIPI_DSI */
#endif /* AICFB_GET_PANEL_MATCH_ID */

#if AICFB_GET_PANEL_MATCH_ID
static unsigned int aicfb_get_panel_id(struct aicfb_info *fbi)
{
#ifdef AIC_DISP_MIPI_DSI
    return aicfb_dsi_read_id(fbi, MIPI_DCS_GET_DISPLAY_ID);
#else
/*
 * TODO: RGB and LVDS interfaces cannot read the panel ID from the screen.
 * We can get the panel ID from an input GPIO, a system file, or other
 * methods, which need to be customized.
 */
#error AICFB_GET_PANEL_MATCH_ID only supports MIPI-DSI interface
    return 0;
#endif
    return 0;
}
#endif /* AICFB_GET_PANEL_MATCH_ID */

void aicfb_get_panel_info(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;
    struct aic_panel *panel = fbi->panel;
    u32 bpp = 24, dither_en = 1;
    struct panel_desc *desc = NULL;
    unsigned int id;

    if (panel->desc && panel->match_num) {
        id = panel->match_id;
#if AICFB_GET_PANEL_MATCH_ID
        id = aicfb_get_panel_id(fbi);
        panel->match_id = id;
#endif
        desc = &panel->desc[id];
        fbi->desc = desc;
    }

    if(di->di_funcs->attach_panel)
        di->di_funcs->attach_panel(panel, desc);

    if(di->di_funcs->get_output_bpp)
        bpp = di->di_funcs->get_output_bpp();

#ifdef AIC_DISABLE_DITHER
    dither_en = 0;
#endif

    if(de->de_funcs->set_mode)
        de->de_funcs->set_mode(panel, desc, bpp, dither_en);
}


static void aicfb_get_hv_active(struct aicfb_info *fbi, u32 *active_w, u32 *active_h)
{
#ifdef AIC_SCREEN_CROP
    *active_w = AIC_SCREEN_CROP_WIDTH;
    *active_h = AIC_SCREEN_CROP_HEIGHT;
#else
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;

    if (desc) {
        *active_w = desc->timings->hactive;
        *active_h = desc->timings->vactive;
    } else {
        *active_w = panel->timings->hactive;
        *active_h = panel->timings->vactive;
    }
#endif
}

static inline int aicfb_format_bpp(enum mpp_pixel_format format)
{
    switch (format) {
    case MPP_FMT_ARGB_8888:
    case MPP_FMT_ABGR_8888:
    case MPP_FMT_RGBA_8888:
    case MPP_FMT_BGRA_8888:
    case MPP_FMT_XRGB_8888:
    case MPP_FMT_XBGR_8888:
    case MPP_FMT_RGBX_8888:
    case MPP_FMT_BGRX_8888:
        return 32;
    case MPP_FMT_RGB_888:
    case MPP_FMT_BGR_888:
        return 24;
    case MPP_FMT_ARGB_1555:
    case MPP_FMT_ABGR_1555:
    case MPP_FMT_RGBA_5551:
    case MPP_FMT_BGRA_5551:
    case MPP_FMT_RGB_565:
    case MPP_FMT_BGR_565:
    case MPP_FMT_ARGB_4444:
    case MPP_FMT_ABGR_4444:
    case MPP_FMT_RGBA_4444:
    case MPP_FMT_BGRA_4444:
        return 16;
    default:
        break;
    }
    return 32;
}

void aicfb_fb_info_setup(struct aicfb_info *fbi)
{
    u32 stride, bpp;
    u32 active_w;
    u32 active_h;
    size_t fb_size;

    bpp = aicfb_format_bpp(AICFB_FORMAT);

#ifdef AIC_FB_ROTATE_EN
    fbi->fb_rotate = AIC_FB_ROTATE_DEGREE;
#else
    fbi->fb_rotate = 0;
#endif

    aicfb_get_hv_active(fbi, &active_w, &active_h);

    if (fbi->fb_rotate == 90 || fbi->fb_rotate == 270)
    {
        u32 tmp = active_w;

        active_w = active_h;
        active_h = tmp;
    }

    stride = ALIGN_8B(active_w * bpp / 8);
    fb_size = active_h * stride;

    fbi->width = active_w;
    fbi->height = active_h;

    fbi->bits_per_pixel = bpp;
    fbi->stride = stride;
    fbi->fb_size = fb_size;
}

void aicfb_register_panel_callback(struct aicfb_info *fbi)
{
    struct platform_driver *de = fbi->de;
    struct platform_driver *di = fbi->di;
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;
    struct aic_panel_callbacks cb;

    cb.di_enable = di->di_funcs->enable;
    cb.di_disable = di->di_funcs->disable;
    cb.di_send_cmd = di->di_funcs->send_cmd;
    cb.di_set_videomode = di->di_funcs->set_videomode;
    cb.timing_enable = de->de_funcs->timing_enable;
    cb.timing_disable = de->de_funcs->timing_disable;

    if (desc)
        desc->funcs->register_callback(panel, &cb);
    else
        panel->funcs->register_callback(panel, &cb);
}

void fb_color_block(struct aicfb_info *fbi, size_t fb_size)
{
#ifdef AIC_DISP_COLOR_BLOCK
#ifdef AIC_FB_ROTATE_EN
    if (!fb_size)
        return;

    memset(fbi->fb_start, 0x0, fb_size);
    aicos_dcache_clean_range((unsigned long *)fbi->fb_start, fb_size);
#else
    u32 width, height;
    u32 i, j, index;
    unsigned char color[5][3] = {
        { 0x00, 0x00, 0xFF },
        { 0x00, 0xFF, 0x00 },
        { 0xFF, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF },
    };
    unsigned char *pos = (unsigned char *)fbi->fb_start;
    (void)fb_size;

    width = fbi->width;
    height = fbi->height;

    switch (fbi->bits_per_pixel) {
#if defined(AICFB_ARGB8888) || defined(AICFB_ABGR8888) || defined(AICFB_XRGB8888)
    case 32:
    {
        for (i = 0; i < height; i++) {
            pos = (unsigned char *)fbi->fb_start + i * fbi->stride;

            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = color[index][0];
                *(pos++) = color[index][1];
                *(pos++) = color[index][2];
                *(pos++) = 0;
            }
        }
        break;
    }
#endif
#if defined(AICFB_RGB888)
    case 24:
    {
        for (i = 0; i < height; i++) {
            pos = (unsigned char *)fbi->fb_start + i * fbi->stride;

            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = color[index][0];
                *(pos++) = color[index][1];
                *(pos++) = color[index][2];
            }
        }
        break;
    }
#endif
#if defined(AICFB_RGB565)
    case 16:
    {
        for (i = 0; i < height; i++) {
            pos = (unsigned char *)fbi->fb_start + i * fbi->stride;

            for (j = 0; j < width; j++) {
                index = (i / 100 + j / 100) % 5;

                *(pos++) = (color[index][0] >> 3) | ((color[index][1] & 0x1c) << 3);
                *(pos++) = ((color[index][1] & 0xe0) >> 5) | (color[index][2] & 0xf8);
            }
        }
        break;
    }
#endif
    default:
        *pos = color[0][0];
        index = AICFB_FORMAT;
        i = width;
        j = height;

        pr_info("format(%d) do not support %dx%d color block.\n", index, i, j);
        return;
    }

    aicos_dcache_clean_range((unsigned long *)fbi->fb_start, fbi->fb_size);
#endif /* AIC_FB_ROTATE_EN */
#endif /* AIC_DISP_COLOR_BLOCK */
#ifdef AIC_DISP_BLACK_SCREEN
    if (!fb_size)
        return;

    memset(fbi->fb_start, 0x0, fb_size);
    aicos_dcache_clean_range((unsigned long *)fbi->fb_start, fb_size);
#endif
}

#ifdef AIC_DISPLAY_TEST
static void aicfb_reset(struct aicfb_info *fbi)
{
    struct aicfb_layer_data layer = {0};
    struct platform_driver *de = fbi->de;
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;

    aicfb_get_panel_info(fbi);
    aicfb_fb_info_setup(fbi);
    aicfb_enable_panel(fbi, AICFB_OFF);

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 0;
    de->de_funcs->update_layer_config(&layer);
    aicfb_enable_clk(fbi, AICFB_OFF);

    aic_delay_ms(20);

    aicfb_enable_clk(fbi, AICFB_ON);

    layer.enable = 1;
    layer.rect_id = 0;

    layer.buf.size.width = desc ? desc->timings->hactive : panel->timings->hactive;
    layer.buf.size.height = desc ? desc->timings->vactive : panel->timings->vactive;

    layer.buf.stride[0] = fbi->stride;
    de->de_funcs->update_layer_config(&layer);
    aicfb_enable_panel(fbi, AICFB_ON);
}

static int handle_dbi_data(struct aic_panel *panel,
                           struct panel_desc *desc,
                           struct panel_dbi *dbi,
                           u32 *cmd_offset)
{
    struct panel_dbi_commands *target = desc ?
                                       &desc->dbi->commands :
                                       &panel->dbi->commands;

    if (!target->buf) {
        target->buf = aicos_malloc(MEM_CMA, target->len);
        if (!target->buf) {
            pr_err("Malloc dbi buf failed!\n");
            return -1;
        }
    }

    const u32 total = dbi->commands.len + *cmd_offset;

    if (total < target->len) {
        memcpy((u8*)target->buf + *cmd_offset,
                dbi->commands.buf, dbi->commands.len);

        *cmd_offset += dbi->commands.len;
    } else if (total == target->len) {
        memcpy((u8*)target->buf + *cmd_offset,
                dbi->commands.buf, dbi->commands.len);

        *cmd_offset = 0;
    } else {
        pr_err("Commands len is too long!\n");
        *cmd_offset = 0;
        return -1;
    }

    return 0;
}

static int handle_dsi_data(struct aic_panel *panel,
                           struct panel_desc *desc,
                           struct panel_dsi *dsi,
                           u32 *cmd_offset)
{
    struct dsi_command *target = desc ?
                                &desc->dsi->command :
                                &panel->dsi->command;

    if (!target->buf) {
        target->buf = aicos_malloc(MEM_CMA, 4096);
        if (!target->buf) {
            pr_err("Malloc dsi buf failed!\n");
            return -1;
        }
    }

    if ((*cmd_offset + dsi->command.len) > 4096) {
        pr_err("Command buffer overflow!\n");
        return -1;
    }

    memcpy(target->buf + *cmd_offset,
           dsi->command.buf,
           dsi->command.len);

    *cmd_offset += dsi->command.len;
    target->len = *cmd_offset;

    target->command_on = 1;

    return 0;
}

static int handle_spi_rgb_data(struct aic_panel *panel,
                               struct panel_desc *desc,
                               struct panel_rgb *rgb,
                               u32 *cmd_offset)
{
    struct rgb_spi_command *target = desc ?
                                    &desc->rgb->spi_command :
                                    &panel->rgb->spi_command;

    if (!target->buf) {
        target->buf = aicos_malloc(MEM_CMA, 4096);
        if(!target->buf) {
            pr_err("Malloc rgb spi command buf failed\n");
            return -1;
        }
    }

    if ((*cmd_offset + rgb->spi_command.len) > 4096) {
        pr_err("Command buffer overflow!\n");
        return -1;
    }

    memcpy(target->buf + *cmd_offset,
           rgb->spi_command.buf,
           rgb->spi_command.len);

    *cmd_offset += rgb->spi_command.len;
    target->len = *cmd_offset;

    target->command_on = 1;

    return 0;
}

static bool handle_rgb_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;
    struct panel_rgb *rgb = config->data;
    struct panel_rgb *target_rgb = desc ? desc->rgb : panel->rgb;
    static u32 rgb_spi_command_offset = 0;

#define RGB_SPI_COMMAND_UPDATE  0
#define RGB_SPI_COMMAND_DISABLE 1
#define RGB_SPI_COMMAND_CLEAR   2
#define RGB_SPI_COMMAND_SEND    3

    switch (rgb->spi_command.command_on)
    {
    case RGB_SPI_COMMAND_UPDATE:
    {
        if (handle_spi_rgb_data(panel, desc, rgb, &rgb_spi_command_offset) < 0) {
            pr_err("handld spi-rgb command error!\n");
        }
        return true;
    }
    case RGB_SPI_COMMAND_DISABLE:
    {
        target_rgb->spi_command.command_on = 0;
        return true;
    }
    case RGB_SPI_COMMAND_CLEAR:
    {
        target_rgb->spi_command.command_on = 2;
        rgb_spi_command_offset = 0;
        return true;
    }
    default:
    {
        target_rgb->mode        = rgb->mode;
        target_rgb->format      = rgb->format;
        target_rgb->data_order  = rgb->data_order;
        target_rgb->data_mirror = rgb->data_mirror;
        target_rgb->clock_phase = rgb->clock_phase;
        return false;
    }
    }
}

static bool handle_mipi_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;
    struct panel_dsi *dsi = config->data;
    struct panel_dsi *target_dsi = desc ? desc->dsi : panel->dsi;
    static u32 dsi_commands_offset = 0;

#define MIPI_DSI_DISABLE_COMMAND     0
#define MIPI_DSI_ADB_UPDATE_COMMAND  1
#define MIPI_DSI_SEND_COMMAND        2
#define MIPI_DSI_UART_UPDATE_COMMAND 3
#define MIPI_DSI_UART_COPY_COMMAND   4

    switch (dsi->command.command_on)
    {
    case MIPI_DSI_ADB_UPDATE_COMMAND:
    {
        if (!target_dsi->command.buf)
            target_dsi->command.buf = aicos_malloc(MEM_CMA, 4096);

        target_dsi->command.command_on = 1;
        target_dsi->command.len = dsi->command.len;
        memcpy(target_dsi->command.buf, dsi->command.buf, dsi->command.len);
        return true;
    }
    case MIPI_DSI_UART_COPY_COMMAND:
    {
        if (handle_dsi_data(panel, desc, dsi, &dsi_commands_offset) < 0) {
            pr_err("handle dsi command error!\n");
        }
        memcpy(target_dsi->command.buf + dsi_commands_offset,
               dsi->command.buf, dsi->command.len);
        target_dsi->command.len = dsi_commands_offset;
        target_dsi->command.command_on = 1;
        return true;
    }
    case MIPI_DSI_UART_UPDATE_COMMAND:
    {
        target_dsi->command.command_on = 2;
        dsi_commands_offset = 0;
        return true;
    }
    case MIPI_DSI_DISABLE_COMMAND:
    {
        target_dsi->command.command_on = 0;
        return true;
    }
    default:
    {
        target_dsi->mode      = dsi->mode;
        target_dsi->format    = dsi->format;
        target_dsi->lane_num  = dsi->lane_num;
        target_dsi->dc_inv    = dsi->dc_inv;
        target_dsi->vc_num    = dsi->vc_num;
        target_dsi->ln_polrs  = dsi->ln_polrs;
        target_dsi->ln_assign = dsi->ln_assign;
        return false;
    }
    }
}

void aicfb_pq_set_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;

    switch (panel->connector_type)
    {
    case AIC_RGB_COM:
    {
        if (handle_rgb_config(fbi, config))
            return;
        break;
    }
    case AIC_LVDS_COM:
    {
        memcpy(desc ? desc->lvds : panel->lvds, config->data,
               sizeof(struct panel_lvds));
        break;
    }
    case AIC_MIPI_COM:
    {
        if (handle_mipi_config(fbi, config))
            return;
        break;
    }
    case AIC_DBI_COM:
    {
        static u32 dbi_commands_offset = 0;
        struct panel_dbi *dbi = config->data;
        struct panel_dbi *target_dbi = desc ? desc->dbi : panel->dbi;

#define MIPI_DBI_UPDATE_COMMAND  0
#define MIPI_DBI_SEND_COMMAND    1

        if (dbi->commands.command_flag == MIPI_DBI_UPDATE_COMMAND) {
            if (handle_dbi_data(panel, desc, dbi, &dbi_commands_offset) < 0) {
                pr_err("handle dbi command error!\n");
                return;
            }
            return;
        } else {
            target_dbi->type = dbi->type;
            target_dbi->format = dbi->format;
            target_dbi->first_line = dbi->first_line;
            target_dbi->other_line = dbi->other_line;

            if (target_dbi->spi != NULL && dbi->spi != NULL) {
                target_dbi->spi->qspi_mode = dbi->spi->qspi_mode;
                target_dbi->spi->vbp_num   = dbi->spi->vbp_num;
                target_dbi->spi->code1_cfg = dbi->spi->code1_cfg;
                target_dbi->spi->code[0]   = dbi->spi->code[0];
                target_dbi->spi->code[1]   = dbi->spi->code[1];
                target_dbi->spi->code[2]   = dbi->spi->code[2];
            }
        }
        break;
    }
    default:
        break;
    }

    if (desc)
        memcpy(desc->timings, config->timing, sizeof(struct display_timing));
    else
        memcpy(panel->timings, config->timing, sizeof(struct display_timing));

    aicfb_reset(fbi);
}

void aicfb_pq_get_config(struct aicfb_info *fbi, struct aicfb_pq_config *config)
{
    struct aic_panel *panel = fbi->panel;
    struct panel_desc *desc = fbi->desc;

    if (desc) {
        memcpy(config->timing, desc->timings, sizeof(struct display_timing));
    } else {
        memcpy(config->timing, panel->timings, sizeof(struct display_timing));
    }

    if (config->connector_type != panel->connector_type)
        return;

    switch (panel->connector_type)
    {
    case AIC_RGB_COM:
    {
        if (desc) {
            memcpy(config->data, desc->rgb, sizeof(struct panel_rgb));
        } else {
            memcpy(config->data, panel->rgb, sizeof(struct panel_rgb));
        }
        break;
    }
    case AIC_LVDS_COM:
    {
        if (desc) {
            memcpy(config->data, desc->lvds, sizeof(struct panel_lvds));
        } else {
            memcpy(config->data, panel->lvds, sizeof(struct panel_lvds));
        }
        break;
    }
    case AIC_MIPI_COM:
    {
        struct panel_dsi *dsi = config->data;
        struct panel_dsi *src_dsi = desc ? desc->dsi : panel->dsi;

        dsi->mode = src_dsi->mode;
        dsi->format = src_dsi->format;
        dsi->lane_num = src_dsi->lane_num;
        dsi->dc_inv = src_dsi->dc_inv;
        dsi->vc_num = src_dsi->vc_num;
        dsi->ln_polrs = src_dsi->ln_polrs;
        dsi->ln_assign = src_dsi->ln_assign;

        break;
    }
    case AIC_DBI_COM:
    {
        struct panel_dbi *dbi = config->data;
        struct panel_dbi *src_dbi = desc ? desc->dbi : panel->dbi;

        dbi->type = src_dbi->type;
        dbi->format = src_dbi->format;

        if (src_dbi->first_line) {
            dbi->first_line = src_dbi->first_line;
        } else {
            dbi->first_line = 0x2C;
        }

        if (src_dbi->other_line) {
            dbi->other_line = src_dbi->other_line;
        } else {
            dbi->other_line = 0x3C;
        }

        if (src_dbi->spi != NULL && dbi->spi != NULL) {
            dbi->spi->qspi_mode  = src_dbi->spi->qspi_mode;
            dbi->spi->vbp_num    = src_dbi->spi->vbp_num;
            dbi->spi->code1_cfg  = src_dbi->spi->code1_cfg;
            dbi->spi->code[0]    = src_dbi->spi->code[0];
            dbi->spi->code[1]    = src_dbi->spi->code[1];
            dbi->spi->code[2]    = src_dbi->spi->code[2];
        }
        break;
    }
    default:
        pr_err("AIPQ get config failed!\n");
        break;
    }
}
#endif /* AIC_DISPLAY_TEST */
