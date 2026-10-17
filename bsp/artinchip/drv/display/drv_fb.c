/*
 * Copyright (C) 2024-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: huahui.mai@artinchip.com
 */

#include <aic_core.h>
#include <aic_hal_ge.h>
#include "drv_fb.h"
#include "drv_fb_helper.h"
#ifdef AIC_FB_ROTATE_EN
#include <aic_drv_ge.h>
#endif
#if defined(KERNEL_RTTHREAD)
#include <drivers/pm.h>
#endif

#undef pr_debug
#ifdef AIC_FB_DRV_DEBUG
#define pr_debug    pr_info
#else
#define pr_debug(fmt, ...)
#endif

static struct aicfb_info *g_aicfb_info;
static bool aicfb_probed = false;

static inline struct aicfb_info *aicfb_get_drvdata(void)
{
    return g_aicfb_info;
}

static inline void aicfb_set_drvdata(struct aicfb_info *fbi)
{
    g_aicfb_info = fbi;
}

#ifdef AIC_FB_ROTATE_EN
static int aicfb_rotate(struct aicfb_info *fbi, struct aicfb_layer_data *layer,
                u32 buf_id)
{
    struct ge_bitblt blt = {0};
    struct aic_ge_client *client = NULL;

    /* source buffer */
    blt.src_buf.buf_type = MPP_PHY_ADDR;
    blt.src_buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * buf_id;
    blt.src_buf.stride[0] = fbi->stride;
    blt.src_buf.size.width = fbi->width;
    blt.src_buf.size.height = fbi->height;
    blt.src_buf.format = layer->buf.format;

    /* destination buffer */
    blt.dst_buf.buf_type = MPP_PHY_ADDR;
    blt.dst_buf.phy_addr[0] = layer->buf.phy_addr[0];
    blt.dst_buf.stride[0] = layer->buf.stride[0];
    blt.dst_buf.size.width = layer->buf.size.width;
    blt.dst_buf.size.height = layer->buf.size.height;
    blt.dst_buf.format = layer->buf.format;

    switch (fbi->fb_rotate) {
    case 0:
        blt.ctrl.flags = 0;
        break;
    case 90:
        blt.ctrl.flags = MPP_ROTATION_90;
        break;
    case 180:
        blt.ctrl.flags = MPP_ROTATION_180;
        break;
    case 270:
        blt.ctrl.flags = MPP_ROTATION_270;
        break;
    default:
        pr_err("Invalid rotation degree\n");
        return -EINVAL;
    };

    return hal_ge_control(client, IOC_GE_BITBLT, &blt);
}
#endif

static int aicfb_pan_display(struct aicfb_info *fbi, u32 buf_id)
{
    struct aicfb_layer_data layer = {0};
    struct platform_driver *de = fbi->de;

#if !defined(AIC_PAN_DISPLAY) && !defined(AIC_FB_ROTATE_EN)
#ifdef AIC_BOOTLOADER_CMD_PROGRESS_BAR
#else
    pr_err("pan display do not enabled\n");
    return -EINVAL;
#endif
#endif

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 1;
    layer.buf.phy_addr[0] = (uintptr_t)fbi->fb_start + fbi->fb_size * buf_id;

#ifdef AIC_FB_ROTATE_EN
    if (fbi->fb_rotate)
    {
        layer.buf.phy_addr[0] += fbi->fb_size * AIC_FB_DRAW_BUF_NUM;

        aicfb_rotate(fbi, &layer, buf_id);
    }
#endif

    de->de_funcs->update_layer_config(&layer);
    return 0;
}

static int aicfb_ioctl_wait_for_vsync(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->wait_for_vsync();
}

static int aicfb_ioctl_get_screen_register(struct aicfb_info *fbi, void *args)
{
    if (!fbi->di->di_funcs->read_cmd) {
        pr_err("display interface do not supports AICFB_GET_SCREENREG\n");
        return -EINVAL;
    }

    return fbi->di->di_funcs->read_cmd(*(u32 *)args);
}

static int aicfb_ioctl_get_screen_info(struct aicfb_info *fbi, void *args)
{
    struct aicfb_screeninfo *info;

    info = (struct aicfb_screeninfo *) args;
    info->format = AICFB_FORMAT;
    info->bits_per_pixel = fbi->bits_per_pixel;
    info->stride = fbi->stride;
    info->framebuffer = (unsigned char *)fbi->fb_start;
    info->width = fbi->width;
    info->height = fbi->height;
    info->smem_len = fbi->fb_size;
    return 0;
}

static int aicfb_ioctl_pan_display(struct aicfb_info *fbi, void *args)
{
    return aicfb_pan_display(fbi, *(int *)args);
}

static int aicfb_ioctl_power_on(struct aicfb_info *fbi, void *args)
{
    struct aicfb_layer_data layer = {0};
    struct platform_driver *de = fbi->de;

    if (fbi->power_on)
        return 0;

    aicfb_enable_clk(fbi, AICFB_ON);

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 1;
    de->de_funcs->update_layer_config(&layer);

    aicfb_enable_panel(fbi, AICFB_ON);
    fbi->power_on = true;
    return 0;
}

static int aicfb_ioctl_power_off(struct aicfb_info *fbi, void *args)
{
    struct aicfb_layer_data layer = {0};
    struct platform_driver *de = fbi->de;

    if (!fbi->power_on)
        return 0;

    aicfb_enable_panel(fbi, AICFB_OFF);

    layer.layer_id = AICFB_LAYER_TYPE_UI;
    layer.rect_id = 0;
    de->de_funcs->get_layer_config(&layer);

    layer.enable = 0;
    de->de_funcs->update_layer_config(&layer);

    aicfb_enable_clk(fbi, AICFB_OFF);
    fbi->power_on = false;
    return 0;
}

static int aicfb_ioctl_get_layer_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_layer_config(args);
}

static int aicfb_ioctl_update_layer_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->update_layer_config(args);
}

static int aicfb_ioctl_update_layer_cfg_list(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->update_layer_config_list(args);
}

static int aicfb_ioctl_get_alpha_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_alpha_config(args);
}

static int aicfb_ioctl_update_alpha_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->update_alpha_config(args);
}

static int aicfb_ioctl_get_ck_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_ck_config(args);
}

static int aicfb_ioctl_update_ck_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->update_ck_config(args);
}

static int aicfb_ioctl_set_disp_prop(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->set_display_prop(args);
}

static int aicfb_ioctl_get_disp_prop(struct aicfb_info *fbi, void *args)
{
    u32 ret = 0;
    struct aicfb_disp_prop prop;

    ret = fbi->de->de_funcs->get_display_prop(&prop);
    if (ret)
        return ret;

    memcpy(args, &prop, sizeof(struct aicfb_disp_prop));
    return ret;
}

static int aicfb_ioctl_get_ccm_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_ccm_config(args);
}

static int aicfb_ioctl_update_ccm_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->set_ccm_config(args);
}

static int aicfb_ioctl_update_gamma_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->set_gamma_config(args);
}

static int aicfb_ioctl_get_gamma_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_gamma_config(args);
}

static int aicfb_ioctl_update_bg_blend_ui_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->update_bg_blend_ui_config(*(u32 *)args);
}

static int aicfb_ioctl_get_bg_blend_ui_cfg(struct aicfb_info *fbi, void *args)
{
    return fbi->de->de_funcs->get_bg_blend_ui_config(args);
}

#ifdef AIC_DISPLAY_TEST
static int aicfb_ioctl_pq_set_cfg(struct aicfb_info *fbi, void *args)
{
    struct aicfb_pq_config *config = args;

    aicfb_pq_set_config(fbi, config);

    return 0;
}
static int aicfb_ioctl_pq_get_cfg(struct aicfb_info *fbi, void *args)
{
    struct aicfb_pq_config *config = args;

    aicfb_pq_get_config(fbi, config);
    return 0;
}
#endif /* AIC_DISPLAY_TEST */

struct aicfb_ioctl_cmd {
    u32 cmd;
    int (*f)(struct aicfb_info *fbi, void *args);
    char desc[32];
};

struct aicfb_ioctl_cmd aicfb_ioctl_cmds[] = {
{AICFB_WAIT_FOR_VSYNC, aicfb_ioctl_wait_for_vsync, "Wait for vsync"},
{AICFB_PAN_DISPLAY, aicfb_ioctl_pan_display, "Pan display"},
{AICFB_UPDATE_LAYER_CONFIG, aicfb_ioctl_update_layer_cfg, "Update layer cfg"},
{AICFB_GET_LAYER_CONFIG, aicfb_ioctl_get_layer_cfg,"Get layer cfg"},
{AICFB_GET_SCREENINFO, aicfb_ioctl_get_screen_info, "Get screen info"},
{AICFB_UPDATE_LAYER_CONFIG_LISTS, aicfb_ioctl_update_layer_cfg_list, "Update layer cfg list"},
{AICFB_GET_ALPHA_CONFIG, aicfb_ioctl_get_alpha_cfg, "Get alpha cfg"},
{AICFB_UPDATE_ALPHA_CONFIG, aicfb_ioctl_update_alpha_cfg, "Update alpha cfg"},
{AICFB_GET_CK_CONFIG, aicfb_ioctl_get_ck_cfg, "Get Color Key cfg"},
{AICFB_UPDATE_CK_CONFIG, aicfb_ioctl_update_ck_cfg, "Update Color Key cfg"},
{AICFB_SET_DISP_PROP, aicfb_ioctl_set_disp_prop, "Set display prop"},
{AICFB_GET_DISP_PROP, aicfb_ioctl_get_disp_prop, "Get display prop"},
{AICFB_GET_CCM_CONFIG, aicfb_ioctl_get_ccm_cfg, "Get CCM cfg"},
{AICFB_UPDATE_CCM_CONFIG, aicfb_ioctl_update_ccm_cfg, "Update CCM cfg"},
{AICFB_UPDATE_GAMMA_CONFIG, aicfb_ioctl_update_gamma_cfg, "Update gamma cfg"},
{AICFB_GET_GAMMA_CONFIG, aicfb_ioctl_get_gamma_cfg, "Get gamma cfg"},
{AICFB_UPDATE_BG_BLEND_UI_CONFIG, aicfb_ioctl_update_bg_blend_ui_cfg, "Update bg blend ui cfg"},
{AICFB_GET_BG_BLEND_UI_CONFIG, aicfb_ioctl_get_bg_blend_ui_cfg, "Get bg blend ui cfg"},
{AICFB_POWERON, aicfb_ioctl_power_on, "Power on screen"},
{AICFB_POWEROFF, aicfb_ioctl_power_off, "Power off screen"},
{AICFB_GET_SCREENREG, aicfb_ioctl_get_screen_register, "Get screen register"},
#ifdef AIC_DISPLAY_TEST
{AICFB_PQ_SET_CONFIG, aicfb_ioctl_pq_set_cfg, "PQ Tools set cfg"},
{AICFB_PQ_GET_CONFIG, aicfb_ioctl_pq_get_cfg, "PQ Tools get cfg"},
#endif
{0xFFFF, NULL, ""}
};

int aicfb_ioctl(int cmd, void *args)
{
    struct aicfb_ioctl_cmd *ioctl = aicfb_ioctl_cmds;
    struct aicfb_info *fbi = aicfb_get_drvdata();

    do {
        if ((ioctl->cmd == cmd) && ioctl->f)
            return ioctl->f(fbi, args);

        ioctl++;
    } while (ioctl->cmd != 0xFFFF);

    pr_err("Invalid ioctl cmd %#x\n", cmd);
    return -EINVAL;
}

#if defined(KERNEL_RTTHREAD)
#ifdef RT_USING_PM
static int aicfb_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct aicfb_info *fbi = device->user_data;
    struct platform_driver *de = fbi->de;

    if (!aicfb_probed)
        return 0;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    {
        struct aicfb_layer_data layer = {0};

        #ifdef AIC_PM_INDEPENDENT_POWER_KEY
        fbi->panel->independent_pwkey = 1;
        #endif
        aicfb_enable_panel(fbi, AICFB_OFF);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 0;
        de->de_funcs->update_layer_config(&layer);

        aicfb_enable_clk(fbi, AICFB_OFF);
    }
        break;
    default:
        break;
    }

    return 0;
}

static void aicfb_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct aicfb_info *fbi = device->user_data;
    struct platform_driver *de = fbi->de;

    if (!aicfb_probed)
        return;

    switch (mode)
    {
    case PM_SLEEP_MODE_IDLE:
        break;
    case PM_SLEEP_MODE_LIGHT:
    case PM_SLEEP_MODE_DEEP:
    case PM_SLEEP_MODE_STANDBY:
    {
        struct aicfb_layer_data layer = {0};

        aicfb_enable_clk(fbi, AICFB_ON);

        layer.layer_id = AICFB_LAYER_TYPE_UI;
        layer.rect_id = 0;
        de->de_funcs->get_layer_config(&layer);

        layer.enable = 1;
        de->de_funcs->update_layer_config(&layer);
        aicfb_enable_panel(fbi, AICFB_ON);
        #ifdef AIC_PM_INDEPENDENT_POWER_KEY
        fbi->panel->independent_pwkey = 0;
        #endif
    }
        break;
    default:
        break;
    }
}

static struct rt_device_pm_ops aicfb_pm_ops =
{
    SET_DEVICE_PM_OPS(aicfb_suspend, aicfb_resume)
    NULL,
};
#endif /* RT_USING_PM */

rt_err_t aicfb_control(rt_device_t dev, int cmd, void *args)
{
    struct aicfb_info *fbi = aicfb_get_drvdata();
    int command;

    if (!fbi)
        return RT_EINVAL;

    switch (cmd)
    {
    case RTGRAPHIC_CTRL_WAIT_VSYNC:
        command = AICFB_WAIT_FOR_VSYNC;
        break;
    case RTGRAPHIC_CTRL_GET_INFO:
    {
        struct rt_device_graphic_info *info;

        info = (struct rt_device_graphic_info *) args;
        info->pixel_format = AICFB_FORMAT;
        info->bits_per_pixel = fbi->bits_per_pixel;
        info->pitch = (rt_uint16_t)fbi->stride;
        info->framebuffer = (rt_uint8_t*)fbi->fb_start;
        info->width = (rt_uint16_t)fbi->width;
        info->height = (rt_uint16_t)fbi->height;
        info->smem_len = fbi->fb_size;

        return RT_EOK;
    }
    case RTGRAPHIC_CTRL_PAN_DISPLAY:
        command = AICFB_PAN_DISPLAY;
        break;
    case RTGRAPHIC_CTRL_POWERON:
        command = AICFB_POWERON;
        break;
    case RTGRAPHIC_CTRL_POWEROFF:
        command = AICFB_POWEROFF;
        break;
    default:
        command = cmd;
        break;
    }

    return aicfb_ioctl(command, args);
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops aicfb_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    aicfb_control
};
#endif /* RT_USING_DEVICE_OPS */

static int aic_rt_fb_device_init(struct aicfb_info *fbi)
{
    struct rt_device *device = &fbi->device;
    int ret;

    device->type = RT_Device_Class_Graphic;

#ifdef RT_USING_DEVICE_OPS
    device->ops = &aicfb_ops;
#else
    device->init = RT_NULL;
    device->open = RT_NULL;
    device->close = RT_NULL;
    device->read = RT_NULL;
    device->write = RT_NULL;
    device->control = aicfb_control;
#endif /* RT_USING_DEVICE_OPS */

    device->user_data = fbi;

    /* register to device manager */
    ret = rt_device_register(device, "aicfb", RT_DEVICE_FLAG_RDWR);
    if(ret != RT_EOK)
    {
        pr_err("aicfb registered fail!\n");
        return ret;
    }

#ifdef RT_USING_PM
    rt_pm_device_register(device, &aicfb_pm_ops);
#endif
    return RT_EOK;
}
#endif /* KERNEL_RTTHREAD */

static struct platform_driver *drivers[] = {
#ifdef AIC_DISP_DE_DRV
    &artinchip_de_driver,
#endif
#ifdef AIC_DISP_RGB
    &artinchip_rgb_driver,
#endif
#ifdef AIC_DISP_LVDS
    &artinchip_lvds_driver,
#endif
#ifdef AIC_DISP_MIPI_DSI
    &artinchip_dsi_driver,
#endif
#ifdef AIC_DISP_MIPI_DBI
    &artinchip_dbi_driver,
#endif
};

static struct platform_driver *
aicfb_find_component(struct platform_driver **drv, int id, int len)
{
    struct platform_driver **driver = drv;
    int ret, i;

    for (i = 0; i < len; i++)
    {
        if (driver[i]->component_type == id)
            break;
    }

    if (i >= len || !driver[i] || !driver[i]->probe)
        return NULL;

    ret = driver[i]->probe();
    if (ret)
        return NULL;

    pr_debug("find component: %s\n", driver[i]->name);

    return driver[i];
}

static inline size_t aicfb_calc_fb_size(struct aicfb_info *fbi)
{
    unsigned int draw_buf_num = 0;
    unsigned int disp_buf_num = 0;

#ifdef AIC_FB_ROTATE_EN
    draw_buf_num = AIC_FB_DRAW_BUF_NUM;
#else
    draw_buf_num = 0;
#endif

#ifdef AIC_PAN_DISPLAY
    disp_buf_num = 2;
#else
    disp_buf_num = 1;
#endif

     return fbi->fb_size * (disp_buf_num + draw_buf_num);
}

int aicfb_probe(void)
{
    struct aicfb_info *fbi;
#ifndef AIC_MPP_VIN_DEV
    size_t fb_size = 0;
#endif
    int ret = -EINVAL;

    if (aicfb_probed)
        return 0;

    fbi = aicos_malloc(0, sizeof(*fbi));
    if (!fbi)
    {
        pr_err("alloc fb_info failed\n");
        return -ENOMEM;
    }
    memset(fbi, 0x0, sizeof(*fbi));

    fbi->de = aicfb_find_component(drivers, AIC_DE_COM, ARRAY_SIZE(drivers));
    if (!fbi->de)
    {
        pr_err("failed to find de component\n");
        goto err;
    }

    fbi->di = aicfb_find_component(drivers, AIC_DI_TYPE, ARRAY_SIZE(drivers));
    if (!fbi->di)
    {
        pr_err("failed to find di component\n");
        goto err;
    }

    fbi->panel = aic_find_panel(AIC_DI_TYPE);
    if (!fbi->panel)
    {
        pr_err("failed to find panel component\n");
        goto err;
    }

    aicfb_get_panel_info(fbi);
    aicfb_fb_info_setup(fbi);
    aicfb_register_panel_callback(fbi);

#ifndef AIC_MPP_VIN_DEV
    fb_size = aicfb_calc_fb_size(fbi);
    /* fb_start must be cache line align */
    fbi->fb_start = aicos_malloc_align(MEM_CMA, fb_size, CACHE_LINE_SIZE);
    if (!fbi->fb_start)
    {
        ret = -ENOMEM;
        pr_err("alloc frame buffer failed\n");
        goto err;
    }
    pr_info("fb0 allocated at 0x%x\n", (u32)(uintptr_t)fbi->fb_start);

    fb_color_block(fbi, fb_size);
#endif

#if defined(KERNEL_RTTHREAD)
    ret = aic_rt_fb_device_init(fbi);
    if(ret != RT_EOK)
        goto err;
#endif

    aicfb_probed = true;
    fbi->power_on = false;
    aicfb_set_drvdata(fbi);

    aicfb_enable_clk(fbi, AICFB_ON);
    aicfb_update_alpha(fbi);
    aicfb_update_layer(fbi);

#if defined(AIC_DISP_COLOR_BLOCK) || defined(AIC_DISP_BLACK_SCREEN)
    fbi->power_on = true;
    aicfb_enable_panel(fbi, AICFB_ON);
#endif
    return 0;

err:
    free(fbi);
    fbi = NULL;
    return ret;
}
#if defined(KERNEL_RTTHREAD) && !defined(AIC_MPP_VIN_DEV)
INIT_DEVICE_EXPORT(aicfb_probe);
#endif

void aicfb_remove(void)
{
    struct aicfb_info *fbi = aicfb_get_drvdata();

    aicfb_probed = false;

    /*
     * FIXME: We should release fbi and framebuffer at the same time.
     * But we keep the fbi pointer to ensure that some ioctl still works properly.
     */
    if (fbi->fb_start)
        aicos_free_align(MEM_CMA, fbi->fb_start);
}

