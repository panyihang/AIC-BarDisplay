/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PANEL_DSI_H_
#define _PANEL_DSI_H_

#include <mipi_display.h>
#include "panel_com.h"

int mipi_dsi_generic_send(struct aic_panel *panel, const u8 *data, size_t size);
int mipi_dsi_dcs_send(struct aic_panel *panel, const u8 *data, size_t size);

#define panel_dsi_generic_send_seq(panel, seq...) do {              \
        static const u8 d[] = { seq };                              \
        int ret;                                                    \
        ret = mipi_dsi_generic_send(panel, d, ARRAY_SIZE(d));       \
        if (ret < 0)                                                \
            return ret;                                             \
    } while (0)

#define panel_dsi_dcs_send_seq(panel, seq...) do {                  \
        static const u8 d[] = { seq };                              \
        int ret;                                                    \
        ret = mipi_dsi_dcs_send(panel, d, ARRAY_SIZE(d));           \
        if (ret < 0)                                                \
            return ret;                                             \
    } while (0)

int panel_dsi_dcs_set_display_on(struct aic_panel *panel);
int panel_dsi_dcs_set_display_off(struct aic_panel *panel);

int panel_dsi_dcs_exit_sleep_mode(struct aic_panel *panel);
int panel_dsi_dcs_enter_sleep_mode(struct aic_panel *panel);

int panel_dsi_set_maximum_return_packet_size(struct aic_panel *panel, u16 value);

void panel_dsi_send_perpare(struct aic_panel *panel);
void panel_dsi_setup_realmode(struct aic_panel *panel);

int panel_dsi_enter_command_mode(struct aic_panel *panel);

#endif /* _PANEL_DSI_H_ */

