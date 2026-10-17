/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __REBOOT_REASON_H__
#define __REBOOT_REASON_H__

#include "aic_common.h"

enum aic_reboot_reason {
    REBOOT_REASON_COLD              = 0,
    REBOOT_REASON_CMD_REBOOT,
    REBOOT_REASON_CMD_SHUTDOWN,
    REBOOT_REASON_SUSPEND,
    REBOOT_REASON_UPGRADE,          /* Goto BROM upgrade mode */
    REBOOT_REASON_BL_UPGRADE,       /* Goto Bootloader upgrade mode */
    REBOOT_REASON_BL_HID_UPGRADE,   /* Goto Bootloader HID upgrade mode */

    /* Software exception reason, MUST begin with 10 */
    REBOOT_REASON_SW_LOCKUP         = 10,
    REBOOT_REASON_HW_LOCKUP,
    REBOOT_REASON_PANIC,
    REBOOT_REASON_RAMDUMP,

    /* Hardware exception reason, MUST begin with 16 */
    REBOOT_REASON_RTC               = 16,
    REBOOT_REASON_EXTEND,
    REBOOT_REASON_JTAG,
    REBOOT_REASON_OTP,
    REBOOT_REASON_UNDER_VOL,

    REBOOT_REASON_INVALID = 0xff,
};

/* Defined in ArtInChip RTC/WRI module */

void aic_set_reboot_reason(enum aic_reboot_reason reason);
enum aic_reboot_reason aic_get_reboot_reason(void);
void aic_clr_reboot_reason(void);
void aic_show_gtc_time(char *tag, u32 val);
void aic_show_startup_time(void);

#endif // end of __REBOOT_REASON_H__
