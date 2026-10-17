/*
 * Copyright (C) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_COM_H_
#define _PANEL_COM_H_

#include "../drv_fb.h"

struct gpio_desc {
    unsigned int g;
    unsigned int p;
};

struct aic_panel_callbacks;

struct aic_panel *aic_find_panel(u32 connector_type);

extern struct aic_panel aic_panel_rgb;
extern struct aic_panel aic_panel_lvds;

/*
 * MIPI-DSI Panel
 */
extern struct aic_panel dsi_simple;
extern struct aic_panel dsi_xm91080;
extern struct aic_panel dsi_st7797;
extern struct aic_panel dsi_st7703;
extern struct aic_panel dsi_fl7707_av092k5u_b;
extern struct aic_panel dsi_fl7707_boe1126_b;
extern struct aic_panel dsi_icnl9707_txw935003b;
extern struct aic_panel dsi_ili9881c;
extern struct aic_panel dsi_hx8394;
extern struct aic_panel dsi_jd9365;
extern struct aic_panel dsi_axs15231b;
extern struct aic_panel dsi_nv3051;
extern struct aic_panel dsi_fl7707;
extern struct aic_panel dsi_ft8201;
extern struct aic_panel dsi_st7701s;
extern struct aic_panel dsi_jd9366tc;
extern struct aic_panel dsi_gc9703;
extern struct aic_panel dsi_st7102;
extern struct aic_panel dsi_edp_lt9811exb;
extern struct aic_panel dsi_ek79007ad;
extern struct aic_panel dsi_svc1k26;

/*
 * MIPI-DBI Type B I8080 Panel
 */
extern struct aic_panel dbi_ili9488;
extern struct aic_panel dbi_ili9486l;
extern struct aic_panel dbi_st7789;
extern struct aic_panel dbi_gc9b72na;
extern struct aic_panel dbi_axs15231e;

/*
 * MIPI-DBI Type C SPI Panel
 */
extern struct aic_panel dbi_ili9341;
extern struct aic_panel dbi_st77903;
extern struct aic_panel dbi_st7789t3;
extern struct aic_panel dbi_st7789v;
extern struct aic_panel dbi_ili9327;
extern struct aic_panel dbi_co5300;
extern struct aic_panel dbi_st77916;
extern struct aic_panel dbi_gc9108;
extern struct aic_panel dbi_st77912;
extern struct aic_panel dbi_il79400a;
extern struct aic_panel spi_general;

/*
 * RGB Panel SPI Init
 */
extern struct aic_panel rgb_st7701s;
extern struct aic_panel rgb_gc9a01a;
extern struct aic_panel rgb_nt35560;
extern struct aic_panel rgb_st77922;
extern struct aic_panel rgb_st77916;
extern struct aic_panel rgb_jd9855;
extern struct aic_panel rgb_st7102;

/*
 * SRGB Panel
 */
extern struct aic_panel srgb_hx8238;

/*
 * Bridge Panel
 */
extern struct aic_panel bridge_lt8911;
extern struct aic_panel lcos_hx7033;

void panel_di_enable(struct aic_panel *panel, u32 ms);
void panel_di_disable(struct aic_panel *panel, u32 ms);
void panel_de_timing_enable(struct aic_panel *panel, u32 ms);
void panel_de_timing_disable(struct aic_panel *panel, u32 ms);

int panel_default_prepare(void);
int panel_default_unprepare(void);
int panel_default_enable(struct aic_panel *panel);
int panel_default_disable(struct aic_panel *panel);
int panel_register_callback(struct aic_panel *panel,
                struct aic_panel_callbacks *pcallback);

void panel_backlight_enable(struct aic_panel *panel, u32 ms);

void panel_backlight_disable(struct aic_panel *panel, u32 ms);

void panel_send_command(u8 *para_cmd, u32 size, struct aic_panel *panel);

void panel_get_gpio(struct gpio_desc *desc, char *name);

void panel_gpio_set_value(struct gpio_desc *desc, u32 value);

#ifdef AIC_PANEL_SPI_EMULATION
struct panel_spi_device {
    struct gpio_desc cs;
    struct gpio_desc sdi;
    struct gpio_desc scl;
    struct gpio_desc dc;
};

void panel_spi_data_wr(u8 data);
void panel_spi_cmd_wr(u8 cmd);
#ifndef AIC_SPI_EMULATION_WITH_DC
void panel_spi_device_emulation(char *cs, char *sdi, char *scl);
#else
void panel_spi_device_emulation(char *cs, char *sdi, char *scl, char *dc);
#endif

#define panel_spi_send_seq(command, seq...) do {                    \
        static const u8 d[] = { seq };                              \
        int i;                                                      \
        panel_spi_cmd_wr(command);                                  \
        for (i = 0; i < ARRAY_SIZE(d); i++)                         \
            panel_spi_data_wr(d[i]);                                \
    } while (0)
#endif

#endif /* _PANEL_COM_H_ */

