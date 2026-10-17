/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include "aic_core.h"
#include "aic_reboot_reason.h"
#include "hal_wri.h"

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11)
#define WRI_FLAG_CMP_RST        BIT(12)
#define WRI_FLAG_TSEN_RST       BIT(11)
#endif
#if defined(AIC_WRI_DRV_V16)
#define WRI_FLAG_BOR_RST        BIT(13)
#define WRI_FLAG_TSEN_RST       BIT(12)
#define WRI_FLAG_WDT_FORCE_RST  BIT(11)
#endif
#define WRI_FLAG_WDT_RST        BIT(10)
#define WRI_FLAG_DM_RST         BIT(9)
#define WRI_FLAG_EXT_RST        BIT(8)
#define WRI_FLAG_RTC_RST        BIT(1)
#define WRI_FLAG_SYS_POR        BIT(0)

#if defined(AIC_WRI_DRV_V12) || defined(AIC_WRI_DRV_V11)
u32 wr_bit[WRI_TYPE_MAX] = {WRI_FLAG_SYS_POR, WRI_FLAG_RTC_RST,
                            WRI_FLAG_EXT_RST, WRI_FLAG_DM_RST,
                            WRI_FLAG_WDT_RST, WRI_FLAG_TSEN_RST,
                            WRI_FLAG_CMP_RST};
#elif defined(AIC_WRI_DRV_V10)
u32 wr_bit[WRI_TYPE_MAX] = {WRI_FLAG_SYS_POR, WRI_FLAG_RTC_RST,
                            WRI_FLAG_EXT_RST, WRI_FLAG_DM_RST,
                            WRI_FLAG_WDT_RST};
#elif defined(AIC_WRI_DRV_V16)
u32 wr_bit[WRI_TYPE_MAX] = {WRI_FLAG_SYS_POR,  WRI_FLAG_RTC_RST,
                            WRI_FLAG_EXT_RST,  WRI_FLAG_DM_RST,
                            WRI_FLAG_WDT_RST,  WRI_FLAG_WDT_FORCE_RST,
                            WRI_FLAG_TSEN_RST, WRI_FLAG_BOR_RST};
#endif

static int is_wdt_reset_type(enum aic_warm_reset_type hw)
{
    enum aic_warm_reset_type hw_reason[] = {
            WRI_TYPE_WDT,
#if defined(AIC_WRI_DRV_V16)
            WRI_TYPE_WDT_FORCE
#endif
        };

    for (int i = 0; i < ARRAY_SIZE(hw_reason); i++) {
        if (hw == hw_reason[i])
            return hw_reason[i];
    }
    return 0;
}

static void get_sw_reboot_reason(enum aic_reboot_reason sw,
                                 enum aic_reboot_reason r)
{
    printf("Reset action: Watchdog-Reset, reason: ");
    switch (sw) {
    case REBOOT_REASON_UPGRADE:
        printf("Upgrade-Mode\n");
        break;
    case REBOOT_REASON_CMD_REBOOT:
        printf("Command-Reboot\n");
        break;
    case REBOOT_REASON_SW_LOCKUP:
        printf("Software-Lockup\n");
        break;
    case REBOOT_REASON_HW_LOCKUP:
        printf("Hardware-Lockup\n");
        break;
    case REBOOT_REASON_PANIC:
        printf("Kernel-Panic\n");
        break;
    case REBOOT_REASON_RAMDUMP:
        printf("Ramdump\n");
        break;
    default:
        printf("Unknown(%d)\n", r);
        break;
    }
}

static void get_warm_reboot_reason(enum aic_warm_reset_type hw,
                                    enum aic_reboot_reason **r)
{
    switch (hw) {
    case WRI_TYPE_RTC:
        printf("RTC-Power-Down\n");
        **r = REBOOT_REASON_RTC;
        break;
    case WRI_TYPE_EXT:
        printf("External-PIN-Reset\n");
        **r = REBOOT_REASON_EXTEND;
        break;
    case WRI_TYPE_DM:
        printf("JTAG-Reset\n");
        **r = REBOOT_REASON_JTAG;
        break;
    case WRI_TYPE_TSEN:
        printf("OTP-Reset\n");
        **r = REBOOT_REASON_OTP;
        break;
    case WRI_TYPE_CMP:
        printf("Undervoltage-Reset\n");
        **r = REBOOT_REASON_UNDER_VOL;
        break;
    case WRI_TYPE_BOR:
        printf("Brown-Out-Reset\n");
        **r = REBOOT_REASON_UNDER_VOL;
        break;
    default:
        printf("Unknown(%d)\n", hw);
        break;
    }
}

static int get_hw_reboot_reason(enum aic_warm_reset_type hw,
                                enum aic_reboot_reason *r, u32 sw)
{
    if (*r == REBOOT_REASON_CMD_SHUTDOWN) {
        printf("Reset reason: Command-Poweroff\n");
        return *r;
    }

    if (*r == REBOOT_REASON_SUSPEND) {
        printf("Reset reason: Suspend\n");
        return *r;
    }

    if (*r == REBOOT_REASON_COLD) {
        if (hw == WRI_TYPE_POR) {
            printf("Reset reason: Power-On\n");
            return sw;
        }

        printf("Reset action: Warm-Reset, reason: ");
        get_warm_reboot_reason(hw, &r);
        return *r;
    }
    return -EINVAL;
}

const struct aic_wri_ops wri_ops = {
    .wri_bit = wr_bit,
    .is_wdt_reset = is_wdt_reset_type,
    .sw_reboot_reason = get_sw_reboot_reason,
    .hw_reboot_reason = get_hw_reboot_reason,
};
