/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_GETOPT_H__
#define __AIC_GETOPT_H__

#include <rtconfig.h>

#ifdef KERNEL_RTTHREAD
#include <getopt.h>
#else

#define no_argument         0
#define required_argument   1

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

#define getopt_long aic_getopt

extern int optind;
extern char *optarg;

void aic_getopt_clr(void);
int aic_getopt(int argc, char **argv, const char *sopts,
               const struct option *lopts, char *index);

#endif // end of #ifdef KERNEL_RTTHREAD

#endif // end of __AIC_GETOPT_H__
