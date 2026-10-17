/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     the first version
 * 2022-05-10     Meco Man     improve rt-thread initialization process
 */

#ifdef __RTTHREAD__

#include <lvgl.h>
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG    "LVGL"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifndef LPKG_LVGL_THREAD_STACK_SIZE
    #define LPKG_LVGL_THREAD_STACK_SIZE 4096
#endif /* LPKG_LVGL_THREAD_STACK_SIZE */

#ifndef LPKG_LVGL_THREAD_PRIO
    #define LPKG_LVGL_THREAD_PRIO (RT_THREAD_PRIORITY_MAX*2/3)
#endif /* LPKG_LVGL_THREAD_PRIO */

#ifndef LPKG_LVGL_DISP_REFR_PERIOD
    #define LPKG_LVGL_DISP_REFR_PERIOD 33
#endif /* LPKG_LVGL_DISP_REFR_PERIOD */

#define LPKG_LVGL_DELAY_TIME 1

extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_user_gui_init(void);
extern void lv_wait_fs_mounted(void);

static struct rt_thread lvgl_thread;

#ifdef rt_align
    rt_align(RT_ALIGN_SIZE)
#else
    ALIGN(RT_ALIGN_SIZE)
#endif

#ifndef LV_MAIN_THREAD_STACK_SIZE
    #define LV_MAIN_THREAD_STACK_SIZE LPKG_LVGL_THREAD_STACK_SIZE
#endif /* LV_MAIN_THREAD_STACK_SIZE */

static rt_uint8_t lvgl_thread_stack[LV_MAIN_THREAD_STACK_SIZE];

#if LV_USE_LOG
static void lv_rt_log(lv_log_level_t level, const char *buf)
{
    (void)level;
    LOG_I(buf);
}
#endif /* LV_USE_LOG */

#if LV_USE_PROFILER
#include <aic_core.h>

static uint32_t tick_start = 0;
static FILE *log_file = NULL;

static uint32_t my_get_tick_us_cb(void)
{
    return (uint32_t)aic_get_time_us();
}

static int my_get_tid_cb(void)
{
    return (int)((int)rt_thread_self() & 0xFFFFFFFF);
}

static int my_get_cpu_cb(void)
{
    return 0;
}

static void my_log_print_cb(const char * buf)
{
    int elaps = lv_tick_elaps(tick_start);
    if (elaps < 0)
        return;

    if (elaps > LV_PROFILE_END_TIME_MS) {
        if (log_file) {
            rt_kprintf("Write file finish, file = %s\n", LV_PROFILE_FILE);
            fclose(log_file);
            log_file = NULL;
        }
        return;
    }

    if (elaps > LV_PROFILE_START_TIME_MS && elaps < LV_PROFILE_END_TIME_MS) {
        if (log_file) {
            fprintf(log_file, "%s", buf);
        } else {
            printf("%s", buf);
        }
    }
}

static void my_profiler_init(void)
{
    rt_thread_mdelay(LV_PROFILE_INIT_SLEEP_MS);

    log_file = fopen(LV_PROFILE_FILE, "a");
    if (!log_file) {
        LV_LOG_ERROR("Failed to open log file, %s", LV_PROFILE_FILE);
        log_file = NULL;
    }

    lv_profiler_builtin_config_t config;
    lv_profiler_builtin_config_init(&config);
    config.tick_per_sec = 1000000; /* One second is equal to 1000000 microseconds */
    config.tick_get_cb = my_get_tick_us_cb;
    config.tid_get_cb = my_get_tid_cb;
    config.cpu_get_cb = my_get_cpu_cb;
    config.flush_cb = my_log_print_cb;
    lv_profiler_builtin_init(&config);

    tick_start = lv_tick_get();
}
#endif

static void lvgl_thread_entry(void *parameter)
{
#ifdef RT_USING_DFS
    lv_wait_fs_mounted();
#endif

#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
    lv_tick_set_cb(&rt_tick_get_millisecond);
    lv_port_disp_init();
    lv_port_indev_init();
    lv_user_gui_init();
#if LV_USE_PROFILER
    my_profiler_init();
#endif
    /* handle the tasks of LVGL */
    while(1)
    {
#ifdef RT_USING_PM
        rt_pm_module_request(PM_MAIN_ID, PM_SLEEP_MODE_NONE);
#endif
        lv_task_handler();
#ifdef RT_USING_PM
        rt_pm_module_release(PM_MAIN_ID, PM_SLEEP_MODE_NONE);
#endif
        rt_thread_mdelay(LPKG_LVGL_DELAY_TIME);
    }
}

int lvgl_thread_init(void)
{
    rt_err_t err;

    err = rt_thread_init(&lvgl_thread, "LVGL", lvgl_thread_entry, RT_NULL,
           &lvgl_thread_stack[0], sizeof(lvgl_thread_stack), LPKG_LVGL_THREAD_PRIO, 10);
    if(err != RT_EOK)
    {
        LOG_E("Failed to create LVGL thread");
        return -1;
    }
    rt_thread_startup(&lvgl_thread);

    return 0;
}
//INIT_ENV_EXPORT(lvgl_thread_init);

#endif /*__RTTHREAD__*/
