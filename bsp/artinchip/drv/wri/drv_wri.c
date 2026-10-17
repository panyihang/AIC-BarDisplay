/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "aic_core.h"
#include "aic_reboot_reason.h"

int drv_wri_init(void)
{
    /* From WRI V1.2, the follow API is defined in WRI instead of RTC */
    aic_get_reboot_reason();
    aic_clr_reboot_reason();
    aic_show_startup_time();
    return 0;
}
INIT_ENV_EXPORT(drv_wri_init);

#if defined(RT_USING_FINSH)
#include <finsh.h>

#ifdef AIC_USING_WDT
void cmd_reboot(int argc, char **argv);
#endif

static void cmd_aicupg(int argc, char **argv)
{
    if ((argc == 2) && !strcmp(argv[1], "gotobl"))
        aic_set_reboot_reason(REBOOT_REASON_BL_UPGRADE);
    else if ((argc == 2) && !strcmp(argv[1], "hid"))
        aic_set_reboot_reason(REBOOT_REASON_BL_HID_UPGRADE);
    else
        aic_set_reboot_reason(REBOOT_REASON_UPGRADE);
#ifdef AIC_USING_WDT
    cmd_reboot(0, NULL);
#endif
}

MSH_CMD_EXPORT_ALIAS(cmd_aicupg, aicupg, Reboot to the upgrade mode);

#endif
