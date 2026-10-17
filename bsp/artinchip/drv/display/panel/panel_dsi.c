/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_dsi.h"

int mipi_dsi_generic_send(struct aic_panel *panel, const u8 *data, size_t size)
{
    u8 type;

    switch (size) {
    case 0:
        type = MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM;
        break;

    case 1:
        type = MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM;
        break;

    case 2:
        type = MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM;
        break;

    default:
        type = MIPI_DSI_GENERIC_LONG_WRITE;
        break;
    }

    return panel->callbacks.di_send_cmd((u32)type, 0, data, size);
}

int mipi_dsi_dcs_send(struct aic_panel *panel, const u8 *data, size_t size)
{
    u8 type;

    switch (size) {
    case 0:
        return -EINVAL;

    case 1:
        type = MIPI_DSI_DCS_SHORT_WRITE;
        break;

    case 2:
        type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
        break;

    default:
        type = MIPI_DSI_DCS_LONG_WRITE;
        break;
    }

    return panel->callbacks.di_send_cmd((u32)type, 0, data, size);
}

int panel_dsi_dcs_set_display_on(struct aic_panel *panel)
{
    ssize_t err;

    err = mipi_dsi_dcs_send(panel, (u8[]){ MIPI_DCS_SET_DISPLAY_ON }, 1);
    if (err < 0)
        return err;

    return 0;
}

int panel_dsi_dcs_set_display_off(struct aic_panel *panel)
{
    ssize_t err;

    err = mipi_dsi_dcs_send(panel, (u8[]){ MIPI_DCS_SET_DISPLAY_OFF }, 1);
    if (err < 0)
        return err;

    return 0;
}

int panel_dsi_dcs_exit_sleep_mode(struct aic_panel *panel)
{
    ssize_t err;

    err = mipi_dsi_dcs_send(panel, (u8[]){ MIPI_DCS_EXIT_SLEEP_MODE }, 1);
    if (err < 0)
        return err;

    return 0;
}

int panel_dsi_dcs_enter_sleep_mode(struct aic_panel *panel)
{
    ssize_t err;

    err = mipi_dsi_dcs_send(panel, (u8[]){ MIPI_DCS_ENTER_SLEEP_MODE }, 1);
    if (err < 0)
        return err;

    return 0;
}

int panel_dsi_set_maximum_return_packet_size(struct aic_panel *panel, u16 value)
{
    u8 type = MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE;
    u8 tx[2] = { value & 0xff, value >> 8 };

    return panel->callbacks.di_send_cmd((u32)type, 0, tx, sizeof(tx));
}

int panel_dsi_enter_command_mode(struct aic_panel *panel)
{
    return panel->callbacks.di_send_cmd(0, 0, NULL, 0);
}

/* Set mipi dsi to command mode, ready to send command */
void panel_dsi_send_perpare(struct aic_panel *panel)
{
    struct panel_desc *desc = NULL;
    struct display_timing *timing;

    if (panel->desc && panel->match_num) {
        desc = &panel->desc[panel->match_id];
        timing = desc->timings;
    } else {
        timing = panel->timings;
    }

    if (panel && panel->callbacks.di_set_videomode)
        panel->callbacks.di_set_videomode(timing, false);
}

/* Set mipi dsi to its real mode */
void panel_dsi_setup_realmode(struct aic_panel *panel)
{
    struct panel_desc *desc = NULL;
    struct display_timing *timing;

    if (panel->desc && panel->match_num) {
        desc = &panel->desc[panel->match_id];
        timing = desc->timings;
    } else {
        timing = panel->timings;
    }

    if (panel && panel->callbacks.di_set_videomode)
        panel->callbacks.di_set_videomode(timing, true);
}
