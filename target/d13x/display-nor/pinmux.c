/*
 * Copyright (c) 2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * D133CBS TXW935003B panel board pin assignment:
 *   UART6: PE.6/PE.7
 *   DSI:   PD.18..PD.27
 *   LCD:   BL_PWM=PD.10, LCD_PEN=PD.15, DSI_RST=PD.16
 */

#include <aic_core.h>
#include <aic_hal.h>
#include <aic_utils.h>
#include "board.h"

#ifdef AIC_NO_CONSOLE_SUSPEND
#define KEEP_CONSOLE_IN_SUSPEND FLAG_WAKEUP_SOURCE
#else
#define KEEP_CONSOLE_IN_SUSPEND 0
#endif

struct aic_pinmux aic_pinmux_config[] = {
#ifdef AIC_USING_UART6
    {5, PIN_PULL_DIS, 3, "PE.6", KEEP_CONSOLE_IN_SUSPEND},
    {5, PIN_PULL_UP,  3, "PE.7", KEEP_CONSOLE_IN_SUSPEND},
#endif
#ifdef AIC_USING_QSPI0
#ifndef AIC_SYSCFG_SIP_FLASH_ENABLE
    {2, PIN_PULL_UP, 3, "PB.0"},
    {2, PIN_PULL_UP, 3, "PB.1"},
    {2, PIN_PULL_UP, 3, "PB.2"},
    {2, PIN_PULL_UP, 3, "PB.3"},
    {2, PIN_PULL_UP, 3, "PB.4"},
    {2, PIN_PULL_UP, 3, "PB.5"},
#else
    {8, PIN_PULL_UP, 3, "PB.12"},
    {8, PIN_PULL_UP, 3, "PB.13"},
    {8, PIN_PULL_UP, 3, "PB.14"},
    {8, PIN_PULL_UP, 3, "PB.15"},
    {8, PIN_PULL_UP, 3, "PB.16"},
    {8, PIN_PULL_UP, 3, "PB.17"},
#endif
#endif
#ifdef AIC_DISP_MIPI_DSI
    /* D3, D2, CLK, D1 and D0 differential pairs, in PCB order. */
    {4, PIN_PULL_DIS, 3, "PD.18"},
    {4, PIN_PULL_DIS, 3, "PD.19"},
    {4, PIN_PULL_DIS, 3, "PD.20"},
    {4, PIN_PULL_DIS, 3, "PD.21"},
    {4, PIN_PULL_DIS, 3, "PD.22"},
    {4, PIN_PULL_DIS, 3, "PD.23"},
    {4, PIN_PULL_DIS, 3, "PD.24"},
    {4, PIN_PULL_DIS, 3, "PD.25"},
    {4, PIN_PULL_DIS, 3, "PD.26"},
    {4, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_PANEL_ENABLE_GPIO
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ENABLE_GPIO},
#endif
#ifdef AIC_PANEL_DSI_ICNL9707_TXW935003B
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO},
#endif
#ifdef AIC_PANEL_DSI_FL7707_AV092K5U_B
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO},
#endif
#ifdef AIC_PANEL_DSI_FL7707_BOE1126_B
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_BOE1126_B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_BOE1126_B_RESET_GPIO},
#endif
};

uint32_t aic_pinmux_config_size = ARRAY_SIZE(aic_pinmux_config);
