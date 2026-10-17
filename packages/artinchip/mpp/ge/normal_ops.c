/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>

#include "aic_core.h"
#include "ge_ops.h"

static int ge_open()
{
    return 0;
}

static int ge_close(struct mpp_ge *ge)
{
    (void)ge;
    return 0;
}

static int ge_fillrect(struct mpp_ge *ge, struct ge_fillrect *fill)
{
    int ret;

    if(!ge)
        return -1;

    ret = aic_ge_ioctl(ge->dev_fd, IOC_GE_FILLRECT, fill);
    if (ret < 0) {
        printf("IOC_GE_FILLRECT failed %d\n", ret);
        return -1;
    }

    return ret;
}

static int ge_bitblt(struct mpp_ge *ge, struct ge_bitblt *blt)
{
    int ret;

    if (!ge)
        return -1;

    ret = aic_ge_ioctl(ge->dev_fd, IOC_GE_BITBLT, blt);
    if (ret < 0) {
        printf("IOC_GE_BITBLT failed %d\n", ret);
        return -1;
    }

    return 0;
}

static int ge_rotate(struct mpp_ge *ge, struct ge_rotation *rot)
{
    int ret;

    if (!ge)
        return -1;

    ret = aic_ge_ioctl(ge->dev_fd, IOC_GE_ROTATE, rot);
    if (ret < 0) {
        printf("IOC_GE_ROTATE failed %d\n", ret);
        return -1;
    }

    return 0;
}

static int ge_sync(struct mpp_ge *ge)
{
    (void)ge;

    return 0;
}

static int ge_emit(struct mpp_ge *ge)
{
    (void)ge;

    return 0;
}

struct ge_ops ge_normal_ops = {
    .name           = "nomal"   ,
    .open           = ge_open,
    .close          = ge_close,
    .fillrect       = ge_fillrect,
    .bitblt         = ge_bitblt,
    .rotate         = ge_rotate,
    .emit           = ge_emit,
    .sync           = ge_sync,
};
