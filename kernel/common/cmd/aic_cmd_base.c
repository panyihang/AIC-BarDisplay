/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/time.h>
#include <stdio.h>

#include "aic_common.h"

#ifdef RT_USING_POSIX_CLOCK

void gettimespec(struct timespec *t)
{
    if (!t)
        return;

    clock_gettime(CLOCK_REALTIME, t);
}

float timespec_diff(struct timespec *start, struct timespec *end)
{
    float diff = 0.0;

    if (!start || !end)
        return diff;

    if (end->tv_nsec < start->tv_nsec) {
        diff = (float)(NS_PER_SEC + end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - 1 - start->tv_sec;
    } else {
        diff = (float)(end->tv_nsec - start->tv_nsec)/NS_PER_SEC;
        diff += end->tv_sec - start->tv_sec;
    }

    return diff;
}

void show_timespec_diff(char *head, char *tail,
                        struct timespec *start, struct timespec *end)
{
    float diff = timespec_diff(start, end);

    if (tail)
        printf("%s: %d.%06d sec%s", head,
               (u32)diff, (u32)(diff * US_PER_SEC) % US_PER_SEC, tail);
    else
        printf("%s: %d.%06d sec\n", head,
               (u32)diff, (u32)(diff * US_PER_SEC) % US_PER_SEC);
}

void show_fps(char *mod, struct timespec *start, struct timespec *end, int cnt)
{
    double diff = timespec_diff(start, end);

    printf("%s frame rate: %d.%d (%d frames / %d.%03d sec)\n", mod,
           (u32)(cnt / diff), (u32)(cnt * MS_PER_SEC / diff) % 10, cnt,
           (u32)diff, (u32)(diff * MS_PER_SEC) % MS_PER_SEC);
}
#endif

#ifdef RT_USING_FINSH
#include <finsh.h>
#include <msh.h>
#include <string.h>

#include "aic_osal.h"
#include "aic_log.h"

static void test_thread(void *arg)
{
    char *cmd = (char *)arg;

    msh_exec(cmd, strlen(cmd));
}

static void cmd_run_in_thread(int argc, char **argv)
{
    aicos_thread_t thid = NULL;

    if (argc < 2) {
        printf("You should input as follow:\n");
        printf("\t%s [\"Command string\"]\n", argv[0]);
        return;
    }

    thid = aicos_thread_create(argv[1], 8192, 0, test_thread, argv[1]);
    if (thid == NULL)
        pr_err("Failed to create test thread\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_run_in_thread, run_in_thread, run a command in thread);

static void test_in_loop_thread(void *arg)
{
    char backup[124] = "";
    char tmp[128] = "";
    u32 cnt = 0, err_cnt = 0;

    strncpy(backup, (char *)arg, 123);
    while (1) {
        memset(tmp, 0, 128);
        strncpy(tmp, backup, 127);
        err_cnt += msh_exec(tmp, strlen(tmp));
        cnt++;
        printf("\n======== Run [%s] %d time, %d error ========\n\n", backup, cnt, err_cnt);
        aicos_msleep(10);
    }
}

static void cmd_run_in_loop_thread(int argc, char **argv)
{
    aicos_thread_t thid = NULL;

    if (argc < 2) {
        printf("You should input as follow:\n");
        printf("\t%s [\"Command string\"]\n", argv[0]);
        return;
    }

    thid = aicos_thread_create(argv[1], 8192, 0, test_in_loop_thread, argv[1]);
    if (thid == NULL)
        pr_err("Failed to create test thread\n");
}
MSH_CMD_EXPORT_ALIAS(cmd_run_in_loop_thread, run_in_loop_thread, run a command in loop thread);
#endif
