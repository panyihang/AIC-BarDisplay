/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#include <string.h>
#include "mpp_ge.h"
#include "ge_ops.h"

extern struct ge_ops ge_normal_ops;
extern struct ge_ops ge_cmdq_ops;

struct ge_ops *ge_ops_lists[] =
{
    &ge_normal_ops,
    &ge_cmdq_ops,
    0
};

static enum ge_mode ge_get_mode(struct aic_ge_client *fd)
{
    enum ge_mode mode;

    int ret = aic_ge_ioctl(fd, IOC_GE_MODE, &mode);
    if (ret < 0)
        printf("ioctl() return %d\n", ret);

    return mode;
}

struct mpp_ge *mpp_ge_open()
{
    int ret;
    struct mpp_ge *ge;

    ge = (struct mpp_ge *)aicos_malloc(0, sizeof(struct mpp_ge));
    if (!ge) {
        printf("mpp_ge aicos_malloc failed!\n");
        return NULL;
    }

    memset(ge, 0, sizeof(struct mpp_ge));

    ge->dev_fd = aic_ge_open();
    if (ge->dev_fd == NULL) {
        printf("Failed to open.\n");
        goto EXIT;
    }

    ge->lock = aicos_mutex_create();
    ge->mode = ge_get_mode(ge->dev_fd);
    ge->ops = ge_ops_lists[ge->mode];

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->open(ge);
    aicos_mutex_give(ge->lock);

    if (ret < 0) {
        goto EXIT;
    }

    return ge;
EXIT:
    if (ge) {
        if(ge->dev_fd != NULL)
            aic_ge_close(ge->dev_fd);

        aicos_free(0, ge);
    }

    return NULL;
}

void mpp_ge_close(struct mpp_ge *ge)
{
    if (!ge)
        return;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ge->ops->close(ge);
    aicos_mutex_give(ge->lock);

    if (ge->dev_fd != NULL)
        aic_ge_close(ge->dev_fd);

    aicos_mutex_delete(ge->lock);
    aicos_free(0, ge);
}

enum ge_mode mpp_ge_get_mode(struct mpp_ge *ge)
{
    return ge->mode;
}

int mpp_ge_fillrect(struct mpp_ge *ge, struct ge_fillrect *fill)
{
    int ret;

    if (!ge)
        return -1;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->fillrect(ge, fill);
    aicos_mutex_give(ge->lock);

    return ret;
}

int mpp_ge_bitblt(struct mpp_ge *ge, struct ge_bitblt *blt)
{
    int ret;

    if (!ge)
        return -1;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->bitblt(ge, blt);
    aicos_mutex_give(ge->lock);

    return ret;
}

int mpp_ge_rotate(struct mpp_ge *ge, struct ge_rotation *rot)
{
    int ret;

    if (!ge)
        return -1;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->rotate(ge, rot);
    aicos_mutex_give(ge->lock);

    return ret;
}

int mpp_ge_emit(struct mpp_ge *ge)
{
    int ret;

    if (!ge)
        return -1;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->emit(ge);
    aicos_mutex_give(ge->lock);

    return ret;
}

int mpp_ge_sync(struct mpp_ge *ge)
{
    int ret;

    if (!ge)
        return -1;

    aicos_mutex_take(ge->lock, AICOS_WAIT_FOREVER);
    ret = ge->ops->sync(ge);
    aicos_mutex_give(ge->lock);

    return ret;
}
