/*
 * Copyright (C) 2022-2025 ArtInChip Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <lvgl.h>
#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#endif

#include "aic_core.h"
#include "aic_osal.h"
#include "aic_ui.h"
#include "aic_time.h"

#ifdef RT_USING_DFS
#include <dfs_fs.h>
void lv_wait_fs_mounted(void)
{
    struct dfs_filesystem *fs = NULL;

    for (int i = 0; i < 1000; i++) {
        fs = dfs_filesystem_lookup(LVGL_STORAGE_PATH);
        if (fs != NULL) {
            /* Continue to look up until fs->path is not equal to '/'. */
            if (strncmp(LVGL_STORAGE_PATH, fs->path, 3) == 0)
                return;
        }

        aicos_msleep(10);
    }
    LV_LOG_ERROR("Wait fs mound timeout, path: %s\n", LVGL_STORAGE_PATH);
}
#endif

void lv_user_gui_init(void)
{
#if defined(AIC_LVGL_DEMO) && !defined(RT_USING_MODULE)
    aic_ui_init();
#endif
}

#if defined(KERNEL_RTTHREAD) && !defined(APP_USB_DISP)
#ifdef RT_USING_MODULE
extern int dm_daemon(void);
INIT_LATE_APP_EXPORT(dm_daemon);
#else
extern int lvgl_thread_init(void);
INIT_LATE_APP_EXPORT(lvgl_thread_init);
#endif  // RT_USING_MODULE
#endif  // KERNEL_RTTHREAD
