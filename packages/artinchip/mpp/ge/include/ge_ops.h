/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors:  Ning Fang <ning.fang@artinchip.com>
 */

#ifndef _GE_OPS_H_
#define _GE_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mpp_list.h"
#include "mpp_ge.h"

struct ge_ops;

struct mpp_ge {
    struct aic_ge_client *dev_fd;
    enum ge_mode mode;
    void *priv;
    struct ge_ops *ops;
    aicos_mutex_t lock;
};

struct ge_ops
{
    const char *name;
    int (*open)(struct mpp_ge *ge);
    int (*close)(struct mpp_ge *ge);
    int (*fillrect)(struct mpp_ge *ge, struct ge_fillrect *fill);
    int (*bitblt)(struct mpp_ge *ge, struct ge_bitblt *blt);
    int (*rotate)(struct mpp_ge *ge, struct ge_rotation *rot);
    int (*emit)(struct mpp_ge *ge);
    int (*sync)(struct mpp_ge *pGe);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GE_OPS_H_ */


