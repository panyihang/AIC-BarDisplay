/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_HAL_CMU_CLK_V2_H__
#define __AIC_HAL_CMU_CLK_V2_H__

#ifdef __cplusplus
extern "C" {
#endif

struct aic_clk_comm_cfg {
    struct aic_clk_ops *ops;
    const char *name;
    bool enable;
};

struct aic_clk_fixed_rate_cfg {
    struct aic_clk_comm_cfg comm;
    unsigned long rate;
    u8 id;
    u8 type;
    u8 parent_id;
    u8 flag;
};

/* Define types of pll */
enum aic_pll_type {
    AIC_PLL_INT, /* integer pll */
    AIC_PLL_FRA, /* fractional pll */
    AIC_PLL_SDM,    /* spread spectrum pll */
};

struct aic_clk_pll_cfg {
    struct aic_clk_comm_cfg comm;
    u32 offset_gen;
    u32 offset_fra;
    u32 offset_sdm;
    u8 id;
    u8 type;
    u8 parent_id;
    u8 flag;
};

enum aic_fixed_parent_type {
    AIC_FPCLK_NORMAL,
    AIC_FPCLK_FIXED_FACTOR,
};

/*
 * This structure is designed for compatible with multiple dividers.
 * For example, CPU has three parent clocks: CLK_24M, PLL_INT0, PLL_INT1,
 * with corresponding frequency division coefficients of 1, (DIV + 1), 2, respectively.
 * If shift < 0, wd.div represents the frequency division.
 * If shift >= 0, wd.width represents the width of frequency division in register.
 */
struct table_div {
    s16 shift;
    union {
        u16 width;
        u16 div;
    } wd;
};

struct aic_clk_fixed_parent_cfg {
    struct aic_clk_comm_cfg comm;
    u32 offset_reg;
    s8 *table_gates;
    u8 num_gates;
    struct table_div *table_div;
    u8 num_div;
    u8 id;
    u8 parent_id;
    u8 flag;
};

struct aic_clk_multi_parent_cfg {
    struct aic_clk_comm_cfg comm;
    u32 offset_reg;
    s8 *table_gates;
    u8 num_gates;
    u8 mux_bit;
    u8 mux_mask;
    struct table_div *table_div;
    u8 num_div;
    u8 id;
    u8 num_parents;
    const u8 *parent_ids;
};

struct aic_clk_auth_cfg {
    struct aic_clk_comm_cfg comm;
    u32 offset_reg;
    u32 offset_wr_auth_reg;
    u32 key_code;
    u8 key_bit;
    u16 key_mask;
    u8 wr_auth_bit;
    s8 *table_gates;
    u8 num_gates;
    u8 mux_bit;
    u8 mux_mask;
    struct table_div *table_div;
    u8 num_div;
    u8 id;
    u8 num_parents;
    const u8 *parent_ids;
};

struct aic_clk {
    const struct aic_clk_comm_cfg *comm_cfg;
    unsigned long rate;
    u8 count;
    u8 parent_id;
    u8 flag;
};

struct aic_pll_vco {
    unsigned long vco_min;
    unsigned long vco_max;
    char *name;
};

struct aic_clk_ops {
    int (*enable)(struct aic_clk_comm_cfg *comm_cfg);
    void (*disable)(struct aic_clk_comm_cfg *comm_cfg);
    int (*is_enabled)(struct aic_clk_comm_cfg *comm_cfg);
    int (*set_rate)(struct aic_clk_comm_cfg *comm_cfg, unsigned long rate,
                    unsigned long parent_rate);
    unsigned long (*recalc_rate)(struct aic_clk_comm_cfg *comm_cfg,
                                 unsigned long parent_rate);
    long (*round_rate)(struct aic_clk_comm_cfg *comm_cfg, unsigned long rate,
                       unsigned long *parent_rate);
    int (*set_parent)(struct aic_clk_comm_cfg *comm_cfg, unsigned int index);
    unsigned int (*get_parent)(struct aic_clk_comm_cfg *comm_cfg);
    int (*enable_clk_deassert_rst)(struct aic_clk_comm_cfg *comm_cfg);
    void (*disable_clk_assert_rst)(struct aic_clk_comm_cfg *comm_cfg);
};

#define cmu_reg(x)            (volatile void *)(x + CMU_BASE)
#ifdef AIC_CMU_CROSS_ZONE_CTL
#define zc_cmu_reg(x)         (volatile void *)(x + CMU_BASE1)
#endif
#define PARENT(x)             (x)
#define AIC_CLK_CFG_ADDR(_id) [_id] = &(aic_clk_cfg_##_id.comm)
#define AIC_CLK_CFG(_id)      AIC_CLK_CFG_ADDR(_id)
#define DUMMY_CFG(_id)        [_id] = NULL

/* For clocks fixed rate */
#define FRCLK_DEF(_id, _name, _rate) \
    static const struct aic_clk_fixed_rate_cfg aic_clk_cfg_##_id = { \
        .id        = _id, \
        .parent_id = 0, \
        .rate      = _rate, \
        .comm.enable = 1, \
        .comm.ops  = &aic_clk_fixed_rate_ops, \
        .comm.name = _name,  \
    }
#define FRCLK(_id, _name, _rate) FRCLK_DEF(_id, _name, _rate)

/* For PLL clock */
#define PLL_DEF(_id, _name, _type, _parent_id, _gen, _fra, _sdm, _flag) \
    static const struct aic_clk_pll_cfg aic_clk_cfg_##_id = { \
        .id         = _id, \
        .parent_id  = _parent_id, \
        .type       = _type, \
        .offset_gen = _gen, \
        .offset_fra = _fra, \
        .offset_sdm = _sdm, \
        .flag       = _flag, \
        .comm.ops   = &aic_clk_pll_ops, \
        .comm.name = _name,  \
    }
#define PLL_INT(_id, _name, _parent_id, _parent_name, _gen, _flag) \
    PLL_DEF(_id, _name, AIC_PLL_INT, _parent_id, _gen, 0, 0, _flag)
#define PLL_FRA(_id, _name, _parent_id, _parent_name, _gen, _fra, _sdm, _flag) \
    PLL_DEF(_id, _name, AIC_PLL_FRA, _parent_id, _gen, _fra, _sdm, _flag)
#define PLL_SDM(_id, _name, _parent_id, _parent_name, _gen, _fra, _sdm, _flag) \
    PLL_DEF(_id, _name, AIC_PLL_SDM, _parent_id, _gen, _fra, _sdm, _flag)

/* For clocks fixed parent */
#define FPCLK(_id, _name, _parent_id, _parent_name, _reg, _table_gates, \
                  _table_div, _flag) \
    static const struct aic_clk_fixed_parent_cfg aic_clk_cfg_##_id = { \
        .id           = _id, \
        .parent_id    = _parent_id, \
        .offset_reg   = _reg, \
        .table_gates  = _table_gates,\
        .num_gates    = ARRAY_SIZE(_table_gates),\
        .table_div    = _table_div,\
        .num_div      = ARRAY_SIZE(_table_div),\
        .flag         = _flag, \
        .comm.ops     = &aic_clk_fixed_parent_ops, \
        .comm.name = _name,  \
    }

/* For clocks that has multi-parent source */
#define MPCLK(_id, _name, _parent, _reg, _table_gates, _mux, _muxw, _table_div) \
    static const struct aic_clk_multi_parent_cfg aic_clk_cfg_##_id = { \
        .id           = _id, \
        .parent_ids   = _parent, \
        .num_parents  = ARRAY_SIZE(_parent), \
        .offset_reg   = _reg, \
        .table_gates  = _table_gates,\
        .num_gates    = ARRAY_SIZE(_table_gates),\
        .mux_bit      = _mux, \
        .mux_mask     = ((1 << _muxw) - 1), \
        .table_div    = _table_div,\
        .num_div      = ARRAY_SIZE(_table_div),\
        .comm.ops     = &aic_clk_multi_parent_ops, \
        .comm.name    = _name, \
    }

#define AUTHCLK(_id, _name, _parent, _reg, _wr_auth_reg, _key_code, _key, _keyw, _wr_auth_bit, _table_gates, _mux, _muxw, _table_div) \
    static const struct aic_clk_auth_cfg aic_clk_cfg_##_id = { \
        .id          = _id, \
        .parent_ids  = _parent, \
        .num_parents = ARRAY_SIZE(_parent), \
        .offset_reg  = _reg, \
        .offset_wr_auth_reg = _wr_auth_reg,\
        .key_code    = _key_code, \
        .key_bit     = _key, \
        .key_mask    = ((1 << _keyw) - 1), \
        .wr_auth_bit = _wr_auth_bit,\
        .table_gates = _table_gates,\
        .num_gates   = ARRAY_SIZE(_table_gates),\
        .mux_bit     = _mux, \
        .mux_mask    = ((1 << _muxw) - 1), \
        .table_div    = _table_div,\
        .num_div      = ARRAY_SIZE(_table_div),\
        .comm.ops    = &aic_clk_auth_ops, \
        .comm.name = _name,  \
    }

/* For display clock */
#define DISPCLK_DEF(_id, _name, _parent_id, _parent_name, _reg, _divn, \
                    _nwidth, _divm, _mwidth, _divl, _lwidth, _pix_divsel, \
                    _pix_divsel_width) \
    static const struct aic_clk_disp_cfg aic_clk_cfg_##_id = { \
        .id              = _id, \
        .parent_id       = _parent_id, \
        .offset_reg      = _reg, \
        .divn_bit        = _divn, \
        .divn_mask       = ((1 << _nwidth) - 1), \
        .divm_bit        = _divm, \
        .divm_mask       = ((1 << _mwidth) - 1), \
        .divl_bit        = _divl, \
        .divl_mask       = ((1 << _lwidth) - 1), \
        .pix_divsel_bit  = _pix_divsel, \
        .pix_divsel_mask = ((1 << _pix_divsel_width) - 1), \
        .comm.ops        = &aic_clk_disp_ops, \
        .comm.name = _name,  \
    }
#define DISPCLK(_id, _name, _parent_id, _parent_name, _reg, _divn, _nwidth, \
                _divm, _mwidth, _divl, _lwidth, _pix_divsel, \
                _pix_divsel_width) \
    DISPCLK_DEF(_id, _name, _parent_id, _parent_name, _reg, _divn, _nwidth, \
                _divm, _mwidth, _divl, _lwidth, _pix_divsel, \
                _pix_divsel_width)

/*
 * flags used across common struct clk.  these flags should only affect the
 * top-level framework.  custom flags for dealing with hardware specifics
 * belong in struct clk_foo
 *
 * Please update clk_flags[] in drivers/clk/clk.c when making changes here!
 */
#define CLK_SET_RATE_GATE   BIT(0) /* must be gated across rate change */
#define CLK_SET_PARENT_GATE BIT(1) /* must be gated across re-parent */
#define CLK_SET_RATE_PARENT BIT(2) /* propagate rate change up one level */
#define CLK_IGNORE_UNUSED   BIT(3) /* do not gate even if unused */

#define CLK_CROSS_ZONE_CTL  BIT(4) /* Control clk of other zone */
#define CLK_NO_CHANGE       BIT(5) /* don't change rate and parent */

#define CLK_GET_RATE_NOCACHE     BIT(6) /* do not use the cached clk rate */
#define CLK_SET_RATE_NO_REPARENT BIT(7) /* don't re-parent on rate change */
#define CLK_GET_ACCURACY_NOCACHE BIT(8) /* do not use the cached clk accuracy */
#define CLK_RECALC_NEW_RATES     BIT(9) /* recalc rates after notifications */
#define CLK_SET_RATE_UNGATE      BIT(10) /* clock needs to run to set rate */
#define CLK_IS_CRITICAL          BIT(11) /* do not gate, ever */
/* parents need enable during gate/ungate, set rate and re-parent */
#define CLK_OPS_PARENT_ENABLE BIT(12)
/* duty cycle call may be forwarded to the parent clock */
#define CLK_DUTY_CYCLE_PARENT BIT(13)
#define CLK_DONT_HOLD_STATE   BIT(14) /* Don't hold state */


extern const unsigned long fpga_board_rate[];


#ifdef __cplusplus
}
#endif

#endif /* __AIC_HAL_CMU_CLK_V2_H__ */
