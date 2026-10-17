/*
 * Copyright (c) 2025-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: huahui.mai@artinchip.com
 */

#include <aic_core.h>
#include <aic_hal.h>
#include "board.h"
#include <aic_utils.h>

#ifdef AIC_NO_CONSOLE_SUSPEND
#define KEEP_CONSOLE_IN_SUSPEND     FLAG_WAKEUP_SOURCE
#else
#define KEEP_CONSOLE_IN_SUSPEND     0
#endif

struct aic_pinmux aic_pinmux_config[] = {
#ifdef AIC_USING_CIR
    /* cir */
    {3, PIN_PULL_DIS, 3, "PA.0"},
    {3, PIN_PULL_UP, 3, "PA.1"},
#endif
#ifdef AIC_USING_UART0
    /* D125CBS UART0, matching luban-lite-fina. */
    {5, PIN_PULL_DIS, 3, "PD.0", KEEP_CONSOLE_IN_SUSPEND},
    {5, PIN_PULL_UP, 3, "PD.1", KEEP_CONSOLE_IN_SUSPEND},
#endif
#ifdef AIC_USING_UART1
    /* uart1 */
    {5, PIN_PULL_DIS, 3, "PA.2"},
    {5, PIN_PULL_UP, 3, "PA.3"},
#endif
#ifdef AIC_USING_CANFD0
    /* can0 */
    {4, PIN_PULL_DIS, 3, "PA.4"},
    {4, PIN_PULL_DIS, 3, "PA.5"},
#endif
#ifdef AIC_USING_I2C0
    {4, PIN_PULL_DIS, 3, "PA.6"},  // SCK
    {4, PIN_PULL_DIS, 3, "PA.7"},  // SDA
#endif
#ifdef AIC_USING_I2C1
    {4, PIN_PULL_DIS, 3, "PD.2"},  // SCK
    {4, PIN_PULL_DIS, 3, "PD.3"},  // SDA
#endif
#ifdef AIC_USING_RTP
    {2, PIN_PULL_DIS, 3, "PA.8"},
    {2, PIN_PULL_DIS, 3, "PA.9"},
    {2, PIN_PULL_DIS, 3, "PA.10"},
    {2, PIN_PULL_DIS, 3, "PA.11"},
#endif
#ifdef AIC_USING_QSPI1
    /* qspi0 */
    {2, PIN_PULL_UP, 3, "PB.0"},
    {2, PIN_PULL_UP, 3, "PB.1"},
    {2, PIN_PULL_UP, 3, "PB.2"},
    {2, PIN_PULL_UP, 3, "PB.3"},
    {2, PIN_PULL_UP, 3, "PB.4"},
    {2, PIN_PULL_UP, 3, "PB.5"},
#endif
#ifdef AIC_USING_SDMC0
    {2, PIN_PULL_UP, 7, "PB.6"},
    {2, PIN_PULL_UP, 7, "PB.7"},
    {2, PIN_PULL_UP, 7, "PB.8"},
    {2, PIN_PULL_UP, 7, "PB.9"},
    {2, PIN_PULL_UP, 7, "PB.10"},
    {2, PIN_PULL_UP, 7, "PB.11"},
#endif
#ifdef AIC_USING_QSPI0
#ifndef AIC_SYSCFG_SIP_FLASH_ENABLE
    /* qspi0 */
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
#ifdef AIC_USING_SDMC1
    {2, PIN_PULL_UP, 7, "PC.0"},
    {2, PIN_PULL_UP, 7, "PC.1"},
    {2, PIN_PULL_UP, 7, "PC.2"},
    {2, PIN_PULL_UP, 7, "PC.3"},
    {2, PIN_PULL_UP, 7, "PC.4"},
    {2, PIN_PULL_UP, 7, "PC.5"},
#endif
#ifdef AIC_USING_QSPI2
    {3, PIN_PULL_UP, 7, "PC.6"},
    {3, PIN_PULL_UP, 7, "PC.7"},
    {3, PIN_PULL_UP, 7, "PC.8"},
    {3, PIN_PULL_UP, 7, "PC.9"},
    {3, PIN_PULL_UP, 7, "PC.10"},
    {3, PIN_PULL_UP, 7, "PC.11"},
#endif
#ifdef AIC_USING_I2S0
    {4, PIN_PULL_DIS, 3, "PD.11"},
    {4, PIN_PULL_DIS, 3, "PD.12"},
    {4, PIN_PULL_DIS, 3, "PD.13"},
    {4, PIN_PULL_DIS, 3, "PD.14"},
    {4, PIN_PULL_DIS, 3, "PD.15"},
#endif
#ifdef AIC_PRGB_24BIT
    {2, PIN_PULL_DIS, 3, "PD.0"},
    {2, PIN_PULL_DIS, 3, "PD.1"},
    {2, PIN_PULL_DIS, 3, "PD.2"},
    {2, PIN_PULL_DIS, 3, "PD.3"},
    {2, PIN_PULL_DIS, 3, "PD.4"},
    {2, PIN_PULL_DIS, 3, "PD.5"},
    {2, PIN_PULL_DIS, 3, "PD.6"},
    {2, PIN_PULL_DIS, 3, "PD.7"},
    {2, PIN_PULL_DIS, 3, "PD.8"},
    {2, PIN_PULL_DIS, 3, "PD.9"},
    {2, PIN_PULL_DIS, 3, "PD.10"},
    {2, PIN_PULL_DIS, 3, "PD.11"},
    {2, PIN_PULL_DIS, 3, "PD.12"},
    {2, PIN_PULL_DIS, 3, "PD.13"},
    {2, PIN_PULL_DIS, 3, "PD.14"},
    {2, PIN_PULL_DIS, 3, "PD.15"},
    {2, PIN_PULL_DIS, 3, "PD.16"},
    {2, PIN_PULL_DIS, 3, "PD.17"},
    {2, PIN_PULL_DIS, 3, "PD.18"},
    {2, PIN_PULL_DIS, 3, "PD.19"},
    {2, PIN_PULL_DIS, 3, "PD.20"},
    {2, PIN_PULL_DIS, 3, "PD.21"},
    {2, PIN_PULL_DIS, 3, "PD.22"},
    {2, PIN_PULL_DIS, 3, "PD.23"},
    {2, PIN_PULL_DIS, 3, "PD.24"},
    {2, PIN_PULL_DIS, 3, "PD.25"},
    {2, PIN_PULL_DIS, 3, "PD.26"},
    {2, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_PRGB_18BIT_LD
    {2, PIN_PULL_DIS, 3, "PD.6"},
    {2, PIN_PULL_DIS, 3, "PD.7"},
    {2, PIN_PULL_DIS, 3, "PD.8"},
    {2, PIN_PULL_DIS, 3, "PD.9"},
    {2, PIN_PULL_DIS, 3, "PD.10"},
    {2, PIN_PULL_DIS, 3, "PD.11"},
    {2, PIN_PULL_DIS, 3, "PD.12"},
    {2, PIN_PULL_DIS, 3, "PD.13"},
    {2, PIN_PULL_DIS, 3, "PD.14"},
    {2, PIN_PULL_DIS, 3, "PD.15"},
    {2, PIN_PULL_DIS, 3, "PD.16"},
    {2, PIN_PULL_DIS, 3, "PD.17"},
    {2, PIN_PULL_DIS, 3, "PD.18"},
    {2, PIN_PULL_DIS, 3, "PD.19"},
    {2, PIN_PULL_DIS, 3, "PD.20"},
    {2, PIN_PULL_DIS, 3, "PD.21"},
    {2, PIN_PULL_DIS, 3, "PD.22"},
    {2, PIN_PULL_DIS, 3, "PD.23"},
    {2, PIN_PULL_DIS, 3, "PD.24"},
    {2, PIN_PULL_DIS, 3, "PD.25"},
    {2, PIN_PULL_DIS, 3, "PD.26"},
    {2, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_PRGB_16BIT_LD
    {2, PIN_PULL_DIS, 3, "PD.8"},
    {2, PIN_PULL_DIS, 3, "PD.9"},
    {2, PIN_PULL_DIS, 3, "PD.10"},
    {2, PIN_PULL_DIS, 3, "PD.11"},
    {2, PIN_PULL_DIS, 3, "PD.12"},
    {2, PIN_PULL_DIS, 3, "PD.13"},
    {2, PIN_PULL_DIS, 3, "PD.14"},
    {2, PIN_PULL_DIS, 3, "PD.15"},
    {2, PIN_PULL_DIS, 3, "PD.16"},
    {2, PIN_PULL_DIS, 3, "PD.17"},
    {2, PIN_PULL_DIS, 3, "PD.18"},
    {2, PIN_PULL_DIS, 3, "PD.19"},
    {2, PIN_PULL_DIS, 3, "PD.20"},
    {2, PIN_PULL_DIS, 3, "PD.21"},
    {2, PIN_PULL_DIS, 3, "PD.22"},
    {2, PIN_PULL_DIS, 3, "PD.23"},
    {2, PIN_PULL_DIS, 3, "PD.24"},
    {2, PIN_PULL_DIS, 3, "PD.25"},
    {2, PIN_PULL_DIS, 3, "PD.26"},
    {2, PIN_PULL_DIS, 3, "PD.27"},
#endif
#ifdef AIC_DISP_MIPI_DSI
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
#ifdef AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_AV092K5U_B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_AV092K5U_B_RESET_GPIO},
#endif
#ifdef AIC_PANEL_FL7707_BOE1126_B_PEN_GPIO
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_BOE1126_B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_FL7707_BOE1126_B_RESET_GPIO},
#endif
#ifdef AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ICNL9707_TXW935003B_PEN_GPIO},
    {1, PIN_PULL_DIS, 3, AIC_PANEL_ICNL9707_TXW935003B_RESET_GPIO},
#endif
#ifdef AIC_USING_CTP
    {1, PIN_PULL_DIS, 3, AIC_TOUCH_PANEL_RST_PIN},
    {1, PIN_PULL_DIS, 3, AIC_TOUCH_PANEL_INT_PIN},
#endif
#ifdef AIC_USING_DVP
    {3, PIN_PULL_DIS, 3, "PE.0"},
    {3, PIN_PULL_DIS, 3, "PE.1"},
    {3, PIN_PULL_DIS, 3, "PE.2"},
    {3, PIN_PULL_DIS, 3, "PE.3"},
    {3, PIN_PULL_DIS, 3, "PE.4"},
    {3, PIN_PULL_DIS, 3, "PE.5"},
    {3, PIN_PULL_DIS, 3, "PE.6"},
    {3, PIN_PULL_DIS, 3, "PE.7"},
    {3, PIN_PULL_DIS, 3, "PE.8"},
    {3, PIN_PULL_DIS, 3, "PE.9"},
    {3, PIN_PULL_DIS, 3, "PE.10"},
#endif
#ifdef AIC_USING_XPWM3
    {3, PIN_PULL_DIS, 3, "PE.12"},
#endif
#ifdef AIC_USING_XPWM4
    {3, PIN_PULL_DIS, 3, "PE.13"},
#endif
#ifdef AIC_USING_XPWM6
    {3, PIN_PULL_DIS, 3, "PE.17"},
#endif
#ifdef AIC_USING_XPWM7
    {3, PIN_PULL_DIS, 3, "PE.18"},
#endif
#ifdef AIC_USING_CLK_OUT0
    {4, PIN_PULL_DIS, 3, "PE.11"},
#endif
};

uint32_t aic_pinmux_config_size = ARRAY_SIZE(aic_pinmux_config);
