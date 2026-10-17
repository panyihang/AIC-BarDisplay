/*
 * Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "string.h"
#include <aic_core.h>
#include "aic_hal_clk.h"

extern const struct aic_clk_comm_cfg *aic_clk_cfgs[];

int hal_clk_enable_deassertrst(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL &&
                        cfg->ops->enable_clk_deassert_rst != NULL,
                -EINVAL);

    return (cfg->ops->enable_clk_deassert_rst(cfg));
}

int hal_clk_disable_assertrst(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL &&
                        cfg->ops->disable_clk_assert_rst != NULL,
                -EINVAL);

    cfg->ops->disable_clk_assert_rst(cfg);
    return 0;
}

int hal_clk_enable(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);
    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];

    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->enable != NULL,
                -EINVAL);

    return (cfg->ops->enable(cfg));
}

int hal_clk_disable(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->disable != NULL,
                -EINVAL);

    cfg->ops->disable(cfg);
    return 0;
}

int hal_clk_is_enabled(uint32_t clk_id)
{
    uint32_t parent_clk_id;
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL, -EINVAL);

    if (cfg->ops->is_enabled == NULL) {
        parent_clk_id = hal_clk_get_parent(clk_id);
        return hal_clk_is_enabled(parent_clk_id);
    }
    return (cfg->ops->is_enabled(cfg));
}

int hal_clk_set_rate(uint32_t clk_id, unsigned long rate,
                     unsigned long parent_rate)
{
    struct aic_clk_comm_cfg *cfg;
    unsigned long old_rate;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->set_rate != NULL,
                -EINVAL);

    old_rate = hal_clk_recalc_rate(clk_id, parent_rate);
    if (old_rate == rate)
        return 0;

    return (cfg->ops->set_rate(cfg, rate, parent_rate));
}

unsigned long hal_clk_recalc_rate(uint32_t clk_id, unsigned long parent_rate)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL &&
                        cfg->ops->recalc_rate != NULL,
                -EINVAL);

    return (cfg->ops->recalc_rate(cfg, parent_rate));
}

long hal_clk_round_rate(uint32_t clk_id, unsigned long rate,
                        unsigned long parent_rate)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->round_rate != NULL,
                -EINVAL);

    return (cfg->ops->round_rate(cfg, rate, &parent_rate));
}

int hal_clk_set_parent(uint32_t clk_id, unsigned int parent_clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->set_parent != NULL,
                -EINVAL);

    return (cfg->ops->set_parent(cfg, parent_clk_id));
}

unsigned int hal_clk_get_parent(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, -EINVAL);

    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->ops != NULL && cfg->ops->get_parent != NULL,
                -EINVAL);

    return (cfg->ops->get_parent(cfg));
}

unsigned long hal_clk_get_freq(uint32_t clk_id)
{
    uint32_t parent_clk_id;
    unsigned long parent_freq;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, 0);

    parent_clk_id = hal_clk_get_parent(clk_id);

    if (0 == parent_clk_id)
        return hal_clk_recalc_rate(clk_id, 0);
    else
        parent_freq = hal_clk_get_freq(parent_clk_id);

    return hal_clk_recalc_rate(clk_id, parent_freq);
}

int hal_clk_set_freq(uint32_t clk_id, unsigned long freq)
{
    uint32_t parent_clk_id;
    unsigned long parent_freq;
    unsigned long old_freq;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, 0);

    parent_clk_id = hal_clk_get_parent(clk_id);
    parent_freq   = hal_clk_get_freq(parent_clk_id);

    /* Avoid set same value */
    old_freq = hal_clk_recalc_rate(clk_id, parent_freq);
    if (freq == old_freq)
        return 0;

    return hal_clk_set_rate(clk_id, freq, parent_freq);
}

int hal_clk_enable_iter(uint32_t clk_id)
{
    uint32_t parent_clk_id;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, 0);

    parent_clk_id = hal_clk_get_parent(clk_id);

    if (0 == parent_clk_id)
        return 0;

    if (!hal_clk_is_enabled(parent_clk_id))
        hal_clk_enable_iter(parent_clk_id);

    return hal_clk_enable(clk_id);
}

int hal_clk_enable_deassertrst_iter(uint32_t clk_id)
{
    uint32_t parent_clk_id;
    int ret = 0;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, 0);

    parent_clk_id = hal_clk_get_parent(clk_id);

    if (0 == parent_clk_id)
        return 0;

    if (!hal_clk_is_enabled(parent_clk_id))
        hal_clk_enable_iter(parent_clk_id);

    ret = hal_clk_enable_deassertrst(clk_id);
    if (ret)
        ret = hal_clk_enable(clk_id);
    return ret;
}

const char *hal_clk_get_name(uint32_t clk_id)
{
    struct aic_clk_comm_cfg *cfg;

    CHECK_PARAM(clk_id < AIC_CLK_NUM && clk_id > 0, 0);
    cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
    CHECK_PARAM(cfg != NULL && cfg->name != NULL, NULL);

    return cfg->name;
}

int32_t hal_clk_get_id(char *name)
{
    uint32_t clk_id;
    struct aic_clk_comm_cfg *cfg;

    for (clk_id = 1; clk_id < AIC_CLK_NUM; clk_id++) {
        cfg = (struct aic_clk_comm_cfg *)aic_clk_cfgs[clk_id];
        if (cfg == NULL || cfg->name == NULL)
            continue;
        if (strcmp(name, cfg->name) == 0)
            return clk_id;
    }

    return -EINVAL;
}
