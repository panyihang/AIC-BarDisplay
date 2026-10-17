/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include "aic_getopt.h"

#ifndef KERNEL_RTTHREAD

int optind = 0;
char *optarg = NULL;

void aic_getopt_clr(void)
{
    optind = 1;
    optarg = NULL;
}

int aic_getopt(int argc, char **argv, const char *sopts,
               const struct option *lopts, char *index)
{
    char *cur_opt = NULL, *tmp = NULL;
    int ret = 0;

    if (argc == 1) {
        aic_getopt_clr();
        return -1;
    }

    if (optind == 0)
        aic_getopt_clr();

    if (optind >= argc || !sopts || !argv[0]) {
        aic_getopt_clr();
        return -1;
    }

    cur_opt = argv[optind++];
    if (!cur_opt) {
        aic_getopt_clr();
        return -1;
    }

    if (cur_opt[0] == '-')
        ret = cur_opt[1];
    else
        return -1;

    if (ret < 'A' || ret > 'z')
        return 0;

    tmp = strchr(sopts, ret);
    if (!tmp)
        return 0;
    if (tmp[1] != ':') {
        /* No argument */
        optarg = NULL;
        return ret;
    }

    /* Need argument */
    if (argc <= optind) {
        aic_getopt_clr();
        return -1;
    }

    if (!argv[optind] || argv[optind][0] == '-') {
        aic_getopt_clr();
        return -1;
    }

    optarg = argv[optind++];
    return ret;
}

#endif // end of KERNEL_RTTHREAD
