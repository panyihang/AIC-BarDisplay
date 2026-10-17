/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AIC_CONFIG_H__
#define AIC_CONFIG_H__

#ifdef __riscv
#define ARCH_RISCV

#if(__riscv_xlen == 64)
#define ARCH_RISCV64
#elif(__riscv_xlen == 32)
#define ARCH_RISCV32
#else
#endif

#if(__riscv_flen == 64)
#define ARCH_RISCV_FPU
#define ARCH_RISCV_FPU_D
#elif(__riscv_flen == 32)
#define ARCH_RISCV_FPU
#define ARCH_RISCV_FPU_S
#else
#endif

#ifdef __riscv_dsp
#define ARCH_RISCV_DSP
#endif

#ifdef __riscv_xthead
#ifdef __riscv_xtheade
#define CONFIG_THEAD_EXT_SPUSHEN
#define CONFIG_THEAD_EXT_SPSWAPEN
#endif
#endif

#endif // __riscv
#endif // AIC_CONFIG_H__