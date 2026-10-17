/*
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#ifndef _AIC_HAL_XSPI_REGS_H_
#define _AIC_HAL_XSPI_REGS_H_
#include <aic_common.h>
#include <aic_soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_DRAM                     (0x40000000)
#define XSPI_INVALID_BASE             (0xFFFFFFFF)

#define XSPI_REG_CTL(base)            (volatile void *)((base) + 0x00UL)
#define XSPI_REG_CLK(base)            (volatile void *)((base) + 0x04UL)
#define XSPI_REG_TCR(base)            (volatile void *)((base) + 0x08UL)
#define XSPI_REG_STAS(base)           (volatile void *)((base) + 0x0cUL)
#define XSPI_REG_CSO_CTL(base)        (volatile void *)((base) + 0x10UL)
#define XSPI_REG_CS0_DCTL(base)       (volatile void *)((base) + 0x14UL)
#define XSPI_REG_CS1_CTL(base)        (volatile void *)((base) + 0x18UL)
#define XSPI_REG_CS1_DCTL(base)       (volatile void *)((base) + 0x1cUL)
#define XSPI_REG_IER(base)            (volatile void *)((base) + 0x20UL)
#define XSPI_REG_ISR(base)            (volatile void *)((base) + 0x24UL)
#define XSPI_REG_FCR(base)            (volatile void *)((base) + 0x28UL)
#define XSPI_REG_FSR(base)            (volatile void *)((base) + 0x2cUL)
#define XSPI_REG_START(base)          (volatile void *)((base) + 0x30UL)
#define XSPI_REG_ADDR(base)           (volatile void *)((base) + 0x34UL)
#define XSPI_REG_FMR(base)            (volatile void *)((base) + 0x38UL)
#define XSPI_REG_BTR(base)            (volatile void *)((base) + 0x40UL)
#define XSPI_REG_RCC(base)            (volatile void *)((base) + 0x44UL)
#define XSPI_REG_NDMA_MODE_CTL(base)  (volatile void *)((base) + 0x48UL)
#define XSPI_REG_TO(base)             (volatile void *)((base) + 0x50UL)
#define XSPI_REG_LCKCR(base)          (volatile void *)((base) + 0x54UL)
#define XSPI_REG_LUT_UP(base)         (volatile void *)((base) + 0x58UL)
#define XSPI_REG_CS0_SEQUENCE(base)   (volatile void *)((base) + 0x60UL)
#define XSPI_REG_CS1_SEQUENCE(base)   (volatile void *)((base) + 0x64UL)
#define XSPI_REG_IO_CTL(base)         (volatile void *)((base) + 0x68UL)
#define XSPI_REG_CS0_IOCFG1(base)     (volatile void *)((base) + 0x70UL)
#define XSPI_REG_CS0_IOCFG2(base)     (volatile void *)((base) + 0x74UL)
#define XSPI_REG_CS0_IOCFG3(base)     (volatile void *)((base) + 0x78UL)
#define XSPI_REG_CS0_IOCFG4(base)     (volatile void *)((base) + 0x7cUL)
#define XSPI_REG_CS1_IOCFG1(base)     (volatile void *)((base) + 0x80UL)
#define XSPI_REG_CS1_IOCFG2(base)     (volatile void *)((base) + 0x84UL)
#define XSPI_REG_CS1_IOCFG3(base)     (volatile void *)((base) + 0x88UL)
#define XSPI_REG_CS1_IOCFG4(base)     (volatile void *)((base) + 0x8cUL)
#define XSPI_REG_TDR(base)            (volatile void *)((base) + 0x200UL)
#define XSPI_REG_RDR(base)            (volatile void *)((base) + 0x300UL)
#define XSPI_REG_DEBUG(base)          (volatile void *)((base) + 0x400UL)
#define XSPI_REG_DEBUG_SEL(base)      (volatile void *)((base) + 0x404UL)
#define XSPI_REG_VERSION(base)        (volatile void *)((base) + 0xffcUL)
#define XSPI_REG_LUTN(base, n)        (volatile void *)((base) + 0x100UL + 0x4*(n))


/*  0x0004 XSPI_CLK */
#define CLK_BIT_CKDIV2_OFS              (0)
#define CLK_BIT_CKDIV2_MSK              (0xff)
#define CLK_BIT_CKDIV2_VAL(v)           (((v) << CLK_BIT_CKDIV2_OFS) & CLK_BIT_CKDIV2_MSK)
#define CLK_BIT_CKDIV1_OFS              (8)
#define CLK_BIT_CKDIV1_MSK              (0xf << 8)
#define CLK_BIT_CKDIV1_VAL(v)           (((v) << CLK_BIT_CKDIV1_OFS) & CLK_BIT_CKDIV1_MSK)
#define CLK_BIT_CKDIV_SEL_OFS           (12)
#define CLK_BIT_CKDIV_SEL_MSK           (1UL << 12)
#define CLK_BIT_CKDIV_SEL_VAL(v)        (((v) << CLK_BIT_CKDIV_SEL_OFS) & CLK_BIT_CKDIV_SEL_MSK)

#define TCR_BIT_WR_HOLD_OFS             (16)
#define TCR_BIT_WR_HOLD_MSK             (0xf << TCR_BIT_WR_HOLD_OFS)
#define TCR_BIT_WR_HOLD_VAl(v)          (((v) << TCR_BIT_WR_HOLD_OFS) & TCR_BIT_WR_HOLD_MSK)
#define TCR_BIT_RD_HOLD_OFS             (20)
#define TCR_BIT_RD_HOLD_MSK             (0xf << TCR_BIT_RD_HOLD_OFS)
#define TCR_BIT_RD_HOLD_VAl(v)          (((v) << TCR_BIT_RD_HOLD_OFS) & TCR_BIT_RD_HOLD_MSK)
#define TCR_BIT_OPI_HOLD_OFS            (28)
#define TCR_BIT_OPI_HOLD_MSK            (0xfUL << TCR_BIT_OPI_HOLD_OFS)
#define TCR_BIT_OPI_HOLD_VAl(v)         (((v) << TCR_BIT_OPI_HOLD_OFS) & TCR_BIT_OPI_HOLD_MSK)

#define CTL_BIT_XSPI_MODE_OFS           (4)
#define CTL_BIT_XSPI_MODE_MSK           (0x3 << CTL_BIT_XSPI_MODE_OFS)
#define CTL_BIT_XSPI_MODE_VAL(v)        (((v) << CTL_BIT_XSPI_MODE_OFS) & CTL_BIT_XSPI_MODE_MSK)
#define CTL_BIT_BOUNDARY_EN_OFS         (12)
#define CTL_BIT_BOUNDARY_EN_MSK         (0x1 << CTL_BIT_BOUNDARY_EN_OFS)
#define CTL_BIT_BOUNDARY_EN_VAL(v)      (((v) << CTL_BIT_BOUNDARY_EN_OFS) & CTL_BIT_BOUNDARY_EN_MSK)
#define CTL_BIT_BOUNDARY_CTL_OFS        (13)
#define CTL_BIT_BOUNDARY_CTL_MSK        (0x3 << CTL_BIT_BOUNDARY_CTL_OFS)
#define CTL_BIT_BOUNDARY_CTL_VAL(v)     (((v) << CTL_BIT_BOUNDARY_CTL_OFS) & CTL_BIT_BOUNDARY_CTL_MSK)
#define CTL_BIT_PARALLEL_MODE_OFS       (6)
#define CTL_BIT_PARALLEL_MODE_MSK       (0x1 << CTL_BIT_PARALLEL_MODE_OFS)
#define CTL_BIT_PARALLEL_MODE_VAL(v)    (((v) << CTL_BIT_PARALLEL_MODE_OFS) & CTL_BIT_PARALLEL_MODE_MSK)
#define CTL_BIT_XSPI_EN_OFS             (0)
#define CTL_BIT_XSPI_EN_MSK             (0x1 << CTL_BIT_XSPI_EN_OFS)
#define CTL_BIT_XSPI_EN_VAL(v)          (((v) << CTL_BIT_XSPI_EN_OFS) & CTL_BIT_XSPI_EN_MSK)
#define CTL_BIT_XIP_EN_OFS              (2)
#define CTL_BIT_XIP_EN_MSK              (0x1 << CTL_BIT_XIP_EN_OFS)
#define CTL_BIT_XIP_EN_VAL(v)           (((v) << CTL_BIT_XIP_EN_OFS) & CTL_BIT_XIP_EN_MSK)
#define TCR_BIT_CS_SEL_OFS              (4)
#define TCR_BIT_CS_SEL_MSK              (0x1 << TCR_BIT_CS_SEL_OFS)
#define TCR_BIT_CS_SEL_VAL(v)           (((v) << TCR_BIT_CS_SEL_OFS) & TCR_BIT_CS_SEL_MSK)
#define CTL_BIT_AXI_BURST_OFS           (3)
#define CTL_BIT_AXI_BURST_MSK           (0x1 << CTL_BIT_AXI_BURST_OFS)
#define CTL_BIT_AXI_BURST_VAL(v)        (((v) << CTL_BIT_AXI_BURST_OFS) & CTL_BIT_AXI_BURST_MSK)
#define CTL_BIT_TIMEOUT_OFS             (7)
#define CTL_BIT_TIMEOUT_MSK             (0x1 << CTL_BIT_TIMEOUT_OFS)
#define CTL_BIT_TIMEOUT_VAL(v)          (((v) << CTL_BIT_TIMEOUT_OFS) & CTL_BIT_TIMEOUT_MSK)



/*  0x0028 XSPI_FCR */
#define FCR_BIT_RX_TRIG_LEVEL_OFS       (0)
#define FCR_BIT_RX_TRIG_LEVEL_MSK       (0xFFUL << FCR_BIT_RX_TRIG_LEVEL_OFS)
#define FCR_BIT_RX_TRIG_LEVEL_VAL(v)    (((v) << FCR_BIT_RX_TRIG_LEVEL_OFS) & FCR_BIT_RX_TRIG_LEVEL_MSK)
#define FCR_BIT_RF_DRQ_EN_OFS           (8)
#define FCR_BIT_RF_DRQ_EN_MSK           (1UL << FCR_BIT_RF_DRQ_EN_OFS)
#define FCR_BIT_RF_DRQ_EN_VAL(v)        (((v) << FCR_BIT_RF_DRQ_EN_OFS) & FCR_BIT_RF_DRQ_EN_MSK)
#define FCR_BIT_RF_RST_OFS              (15)
#define FCR_BIT_RF_RST_MSK              (1UL << FCR_BIT_RF_RST_OFS)
#define FCR_BIT_RF_RST_VAL(v)           (((v) << FCR_BIT_RF_RST_OFS) & FCR_BIT_RF_RST_MSK)
#define FCR_BIT_TX_TRIG_LEVEL_OFS       (16)
#define FCR_BIT_TX_TRIG_LEVEL_MSK       (0xFFUL << FCR_BIT_TX_TRIG_LEVEL_OFS)
#define FCR_BIT_TX_TRIG_LEVEL_VAL(v)    (((v) << FCR_BIT_TX_TRIG_LEVEL_OFS) & FCR_BIT_TX_TRIG_LEVEL_MSK)
#define FCR_BIT_TF_DRQ_EN_OFS           (24)
#define FCR_BIT_TF_DRQ_EN_MSK           (1UL << FCR_BIT_TF_DRQ_EN_OFS)
#define FCR_BIT_TF_DRQ_EN_VAL(v)        (((v) << FCR_BIT_TF_DRQ_EN_OFS) & FCR_BIT_TF_DRQ_EN_MSK)
#define FCR_BIT_TX_FIFO_RST_OFS         (31)
#define FCR_BIT_TX_FIFO_RST_MSK         (1UL << FCR_BIT_TX_FIFO_RST_OFS)
#define FCR_BIT_TX_FIFO_RST_VAL(v)      (((v) << FCR_BIT_TX_FIFO_RST_OFS) & FCR_BIT_TX_FIFO_RST_MSK)
#define FCR_BIT_DMA_EN_MSK              (FCR_BIT_TF_DRQ_EN_MSK | FCR_BIT_RF_DRQ_EN_MSK)

/* 0x002C XSPI_FSR */
#define FSR_BIT_RF_CNT_OFS              (0)
#define FSR_BIT_RF_CNT_MSK              (0xFF << FSR_BIT_RF_CNT_OFS)
#define FSR_BIT_RF_CNT_VAL(v)           (((v) << FSR_BIT_RF_CNT_OFS) & FSR_BIT_RF_CNT_MSK)
#define FSR_BIT_RF_RBUF_CNT_OFS         (12)
#define FSR_BIT_RF_RBUF_CNT_MSK         (0x7 << FSR_BIT_RF_RBUF_CNT_OFS)
#define FSR_BIT_RF_RBUF_CNT_VAL(v)      (((v) << FSR_BIT_RF_RBUF_CNT_OFS) & FSR_BIT_RF_RBUF_CNT_MSK)
#define FSR_BIT_RF_RBUF_STS_OFS         (15)
#define FSR_BIT_RF_RBUF_STS_MSK         (1UL << FSR_BIT_RF_RBUF_STS_OFS)
#define FSR_BIT_RF_RBUF_STS_VAL(v)      (((v) << FSR_BIT_RF_RBUF_STS_OFS) & FSR_BIT_RF_RBUF_STS_MSK)
#define FSR_BIT_TF_CNT_OFS              (16)
#define FSR_BIT_TF_CNT_MSK              (0xFF << FSR_BIT_TF_CNT_OFS)
#define FSR_BIT_TF_CNT_VAL(v)           (((v) << FSR_BIT_TF_CNT_OFS) & FSR_BIT_TF_CNT_MSK)
#define FSR_BIT_TF_WBUF_CNT_OFS         (28)
#define FSR_BIT_TF_WBUF_CNT_MSK         (0xfUL << FSR_BIT_TF_WBUF_CNT_OFS)
#define FSR_BIT_TF_WBUF_CNT_VAL(v)      (((v) << FSR_BIT_TF_WBUF_CNT_OFS) & FSR_BIT_TF_WBUF_CNT_MSK)
#define FSR_BIT_TF_WBUF_STS_OFS         (31)
#define FSR_BIT_TF_WBUF_STS_MSK         (1UL << FSR_BIT_TF_WBUF_STS_OFS)
#define FSR_BIT_TF_WBUF_STS_VAL(v)      (((v) << FSR_BIT_TF_WBUF_STS_OFS) & FSR_BIT_TF_WBUF_STS_MSK)

/* XSPI_CSx_DCTL */
#define CSX_DCTL_BIT_EN_DLL_OFS         (0)
#define CSX_DCTL_BIT_EN_DLL_MSK         (0x1 << CSX_DCTL_BIT_EN_DLL_OFS)
#define CSX_DCTL_BIT_EN_DLL_VAL(v)      (((v) << CSX_DCTL_BIT_EN_DLL_OFS) & CSX_DCTL_BIT_EN_DLL_MSK)
#define CSX_DCTL_BIT_EN_VCDL_OFS        (1)
#define CSX_DCTL_BIT_EN_VCDL_MSK        (0x1 << CSX_DCTL_BIT_EN_VCDL_OFS)
#define CSX_DCTL_BIT_EN_VCDL_VAL(v)     (((v) << CSX_DCTL_BIT_EN_VCDL_OFS) & CSX_DCTL_BIT_EN_VCDL_MSK)
#define CSX_DCTL_BIT_EN_CP_OFS          (2)
#define CSX_DCTL_BIT_EN_CP_MSK          (0x1 << CSX_DCTL_BIT_EN_CP_OFS)
#define CSX_DCTL_BIT_EN_CP_VAL(v)       (((v) << CSX_DCTL_BIT_EN_CP_OFS) & CSX_DCTL_BIT_EN_CP_MSK)
#define CSX_DCTL_BIT_EN_BYPASS_OFS      (3)
#define CSX_DCTL_BIT_EN_BYPASS_MSK      (0x1 << CSX_DCTL_BIT_EN_BYPASS_OFS)
#define CSX_DCTL_BIT_EN_BYPASS_VAL(v)   (((v) << CSX_DCTL_BIT_EN_BYPASS_OFS) & CSX_DCTL_BIT_EN_BYPASS_MSK)
#define CSX_DCTL_BIT_EN_LDO_OFS         (4)
#define CSX_DCTL_BIT_EN_LDO_MSK         (0x1 << CSX_DCTL_BIT_EN_LDO_OFS)
#define CSX_DCTL_BIT_EN_LDO_VAL(v)      (((v) << CSX_DCTL_BIT_EN_LDO_OFS) & CSX_DCTL_BIT_EN_LDO_MSK)
#define CSX_DCTL_BIT_EN_LVS_OFS         (5)
#define CSX_DCTL_BIT_EN_LVS_MSK         (0x1 << CSX_DCTL_BIT_EN_LVS_OFS)
#define CSX_DCTL_BIT_EN_LVS_VAL(v)      (((v) << CSX_DCTL_BIT_EN_LVS_OFS) & CSX_DCTL_BIT_EN_LVS_MSK)
#define CSX_DCTL_BIT_PHASE_SEL_OFS      (8)
#define CSX_DCTL_BIT_PHASE_SEL_MSK      (0xf << CSX_DCTL_BIT_PHASE_SEL_OFS)
#define CSX_DCTL_BIT_PHASE_SEL_VAL(v)   (((v) << CSX_DCTL_BIT_PHASE_SEL_OFS) & CSX_DCTL_BIT_PHASE_SEL_MSK)
#define CSX_DCTL_BIT_REG_ICP_OFS        (12)
#define CSX_DCTL_BIT_REG_ICP_MSK        (0x3 << CSX_DCTL_BIT_REG_ICP_OFS)
#define CSX_DCTL_BIT_REG_ICP_VAL(v)     (((v) << CSX_DCTL_BIT_REG_ICP_OFS) & CSX_DCTL_BIT_REG_ICP_MSK)
#define CSX_DCTL_BIT_REG_DLY_OFS        (16)
#define CSX_DCTL_BIT_REG_DLY_MSK        (0x3 << CSX_DCTL_BIT_REG_DLY_OFS)
#define CSX_DCTL_BIT_REG_DLY_VAL(v)     (((v) << CSX_DCTL_BIT_REG_DLY_OFS) & CSX_DCTL_BIT_REG_DLY_MSK)
#define CSX_DCTL_BIT_REG_BYPASS_OFS     (20)
#define CSX_DCTL_BIT_REG_BYPASS_MSK     (0x3 << CSX_DCTL_BIT_REG_BYPASS_OFS)
#define CSX_DCTL_BIT_REG_BYPASS_VAL(v)  (((v) << CSX_DCTL_BIT_REG_BYPASS_OFS) & CSX_DCTL_BIT_REG_BYPASS_MSK)
#define CSX_DCTL_BIT_REG_ATBSEL_OFS     (24)
#define CSX_DCTL_BIT_REG_ATBSEL_MSK     (0x3 << CSX_DCTL_BIT_REG_ATBSEL_OFS)
#define CSX_DCTL_BIT_REG_ATBSEL_VAL(v)  (((v) << CSX_DCTL_BIT_REG_ATBSEL_OFS) & CSX_DCTL_BIT_REG_ATBSEL_MSK)
#define CSX_DCTL_BIT_EN_ATB_OFS         (28)
#define CSX_DCTL_BIT_EN_ATB_MSK         (0x1 << CSX_DCTL_BIT_EN_ATB_OFS)
#define CSX_DCTL_BIT_EN_ATB_VAL(v)      (((v) << CSX_DCTL_BIT_EN_ATB_OFS) & CSX_DCTL_BIT_EN_ATB_MSK)

//XSPI_REG_LUTN
#define LUTN_BIT_OPERAND_L_OFS          (0)
#define LUTN_BIT_OPERAND_L_MSK          (0xff << LUTN_BIT_OPERAND_L_OFS)
#define LUTN_BIT_OPERAND_L_VAl(v)       (((v) << LUTN_BIT_OPERAND_L_OFS) & LUTN_BIT_OPERAND_L_MSK)
#define LUTN_BIT_IO_CFG_L_OFS           (8)
#define LUTN_BIT_IO_CFG_L_MSK           (0x3 << LUTN_BIT_IO_CFG_L_OFS)
#define LUTN_BIT_IO_CFG_L_VAl(v)        (((v) << LUTN_BIT_IO_CFG_L_OFS) & LUTN_BIT_IO_CFG_L_MSK)
#define LUTN_BIT_INS_L_OFS              (10)
#define LUTN_BIT_INS_L_MSK              (0x3F << LUTN_BIT_INS_L_OFS)
#define LUTN_BIT_INS_L_VAl(v)           (((v) << LUTN_BIT_INS_L_OFS) & LUTN_BIT_INS_L_MSK)
#define LUTN_BIT_OPERAND_H_OFS          (16)
#define LUTN_BIT_OPERAND_H_MSK          (0xff << LUTN_BIT_OPERAND_H_OFS)
#define LUTN_BIT_OPERAND_H_VAl(v)       (((v) << LUTN_BIT_OPERAND_H_OFS) & LUTN_BIT_OPERAND_H_MSK)
#define LUTN_BIT_IO_CFG_H_OFS           (24)
#define LUTN_BIT_IO_CFG_H_MSK           (0x3 << LUTN_BIT_IO_CFG_H_OFS)
#define LUTN_BIT_IO_CFG_H_VAl(v)        (((v) << LUTN_BIT_IO_CFG_H_OFS) & LUTN_BIT_IO_CFG_H_MSK)
#define LUTN_BIT_INS_H_OFS              (26)
#define LUTN_BIT_INS_H_MSK              (0x3fUL << LUTN_BIT_INS_H_OFS)
#define LUTN_BIT_INS_H_VAl(v)           (((v) << LUTN_BIT_INS_H_OFS) & LUTN_BIT_INS_H_MSK)

// XSPI_REG_CS0_IOCFG4
#define IOCFG4_BIT_IOCFG4_OFS          (0)
#define IOCFG4_BIT_IOCFG4_MSK          (0x3f << IOCFG4_BIT_IOCFG4_OFS)
#define IOCFG4_BIT_IOCFG4_VAl(v)       (((v) << IOCFG4_BIT_IOCFG4_OFS) & IOCFG4_BIT_IOCFG4_MSK)


#define XSPI_FIFO_DEPTH                 64


typedef enum xspi_ddr_sdr_mode {
    xspi_sdr = 0x0,
    xspi_ddr = 0x1,
}xspi_ddr_sdr_mode_t;

typedef enum xspi_cs_sel {
    cs0 = 0x0,
    cs1 = 0x1,
}xspi_cs_sel_t;

typedef enum xspi_mode {
    xccela = 0x0,
    hyperbus = 0x1,
    opi = 0x2,
    xspi_spi = 0x3,
}xspi_mode_t;

typedef enum xspi_boundary {
    xspi_2k = 0x0,
    xspi_1k = 0x1,
}xspi_boundary_t;

typedef enum xspi_parallel_mode {
    single_mode = 0x0,
    parellel_mode = 0x1,
}xspi_parallel_mode_t;

typedef enum xspi_dll_phase_sel {
    xspi_d_22b5 = 0x0,
    xspi_d_45b = 0x1,
    xspi_d_67b5 = 0x2,
    xspi_d_90b = 0x3,
    xspi_d_112b5 = 0x4,
    xspi_d_135b = 0x5,
    xspi_d_157b5 = 0x6,
    xspi_d_180b = 0x7,
    xspi_d_202b5 = 0x8,
    xspi_d_225b = 0x9,
    xspi_d_247b5 = 0xa,
    xspi_d_270b = 0xb,
    xspi_d_292b5 = 0xc,
    xspi_d_315b = 0xd,
    xspi_d_337b5 = 0xe,
    xspi_dbypass = 0xf,
}xspi_dll_phase_sel_t;

typedef enum xspi_dll_reg_icp {
    ICP_50_100M = 0x0,
    ICP_100_150M = 0x1,
    ICP_150_200M = 0x2,
    ICP_200_266M = 0x3,
}xspi_dll_reg_icp_t;

typedef enum xspi_lut_id {
    lut_id0 = 0x0,
    lut_id1 = 0x1,
    lut_id2 = 0x2,
    lut_id3 = 0x3,
    lut_id4 = 0x4,
    lut_id5 = 0x5,
    lut_id6 = 0x6,
    lut_id7 = 0x7
}xspi_lut_id_t;

typedef enum xspi_lut_ins {
    XSPI_CMD = 0x1,
    XSPI_CMD_EX = 0x2,
    XSPI_ADDR = 0x3,
    XSPI_WRITE = 0x4,
    XSPI_READ = 0x5,
    XSPI_CMD_DDR = 0x11,
    XSPI_CMD_EX_DDR = 0x12,
    XSPI_ADDR_DDR = 0x13,
    XSPI_WRITE_DDR = 0x14,
    XSPI_READ_DDR = 0x15,
    XSPI_DUMMY = 0x10,
    XSPI_JUMP_ID = 0x20,
    XSPI_JUMP_INS = 0X30,
    XSPI_STOP = 0x0,
}xspi_lut_ins_t;


typedef enum xspi_lut_io {
    IO_1 = 0x0,
    IO_2 = 0x1,
    IO_4 = 0x2,
    IO_8 = 0x3,
}xspi_lut_io_t;

typedef enum xspi_lut_lock {
    LUT_LOCK = 0x1,
    LUT_UNLOCK = 0x2,
}xspi_lut_lock_t;

static inline u32 xspi_hw_index_to_base(u32 idx)
{
    switch (idx) {
        case 0:
            return XSPI_BASE;
        default:
            return 0;
    }
    return 0;
}

static inline u32 xspi_hw_base_to_index(u32 base)
{
    switch (base) {
        case XSPI_BASE:
            return 0;
        default:
            return XSPI_INVALID_BASE;
    }
    return XSPI_INVALID_BASE;
}


/* 0x0060 XSPI_CS0_SEQUENCE && 0x0064 XSPI_CS1_SEQUENCE*/
static inline void xspi_hw_data_pin_override(u32 base, xspi_cs_sel_t sel, u32 pin_cfg)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    if (sel == cs0) {
        writel(pin_cfg, XSPI_REG_CS0_SEQUENCE(base));
    } else if(sel == cs1) {
        writel(pin_cfg, XSPI_REG_CS1_SEQUENCE(base));
    }
}


#define XSPI_CLK_DIVIDER_1 0
#define XSPI_CLK_DIVIDER_2 1
static inline void xspi_hw_set_clk_div(u32 base, u32 div_sel, u32 div)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    u32 val = readl(XSPI_REG_CLK(base));
    if (div_sel == XSPI_CLK_DIVIDER_2) {
        val &= ~(CLK_BIT_CKDIV_SEL_MSK);
        val &= ~(CLK_BIT_CKDIV2_MSK);
        val |= CLK_BIT_CKDIV_SEL_VAL(XSPI_CLK_DIVIDER_2);
        val |= CLK_BIT_CKDIV2_VAL(div);
    } else {
        val &= ~(CLK_BIT_CKDIV_SEL_MSK);
        val &= ~(CLK_BIT_CKDIV1_MSK);
        val |= CLK_BIT_CKDIV_SEL_VAL(XSPI_CLK_DIVIDER_1);
        val |= CLK_BIT_CKDIV1_VAL(div);
    }
    writel(val, XSPI_REG_CLK(base));
}

#define STAS_BIT_BUSY_MSK (0x1 << 0)
#define XSPI_BUSY 1
static inline u8 xspi_hw_check_idle_status(u32 base)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    return (readl(XSPI_REG_STAS(base)) & STAS_BIT_BUSY_MSK);
}

static inline bool xspi_hw_check_transfer_done(u32 base)
{
    return (readl(XSPI_REG_STAS(base)) & STAS_BIT_BUSY_MSK);
}

static inline void xspi_hw_reset(u32 base)
{
    writel(0, XSPI_REG_CTL(base));
}

static inline void xspi_hw_set_cs_write_hold(u32 base, u32 num)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_TCR(base));

    val &= ~(TCR_BIT_WR_HOLD_MSK);
    val |= TCR_BIT_WR_HOLD_VAl(num);

    writel(val, XSPI_REG_TCR(base));
}

static inline void xspi_hw_set_cs_read_hold(u32 base, u32 num)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_TCR(base));

    val &= ~(TCR_BIT_RD_HOLD_MSK);
    val |= TCR_BIT_RD_HOLD_VAl(num);

    writel(val, XSPI_REG_TCR(base));
}

static inline void xspi_hw_set_cs_opi_hold(u32 base, u32 num)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_TCR(base));

    val &= ~(TCR_BIT_OPI_HOLD_MSK);
    val |= TCR_BIT_OPI_HOLD_VAl(num);

    writel(val, XSPI_REG_TCR(base));
}

static inline void xspi_hw_set_xspi_mode(u32 base, xspi_mode_t xspi_mode)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_CTL(base));

    val &= ~(CTL_BIT_XSPI_MODE_MSK);
    val |= CTL_BIT_XSPI_MODE_VAL(xspi_mode);

    writel(val, XSPI_REG_CTL(base));
}


/*
*   data burst length, hardware auto to slipt data.
*   some psram max burst length is 1024bytes.
*/

static inline void xspi_hw_set_boudary_ctl(u32 base, xspi_boundary_t xspi_boundary)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_CTL(base));

    val &= ~(CTL_BIT_BOUNDARY_EN_MSK);
    val |= CTL_BIT_BOUNDARY_EN_VAL(1);

    val &= ~(CTL_BIT_BOUNDARY_CTL_MSK);
    val |= CTL_BIT_BOUNDARY_CTL_VAL(xspi_boundary);

    writel(val, XSPI_REG_CTL(base));
}


static inline void xspi_hw_set_parallel_mode(u32 base, xspi_parallel_mode_t parallel_mode)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_CTL(base));

    val &= ~(CTL_BIT_PARALLEL_MODE_MSK);
    val |= CTL_BIT_PARALLEL_MODE_VAL(parallel_mode);

    writel(val, XSPI_REG_CTL(base));
}


static inline void xspi_hw_set_xspi_en(u32 base, u8 en)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_CTL(base));

    val &= ~(CTL_BIT_XSPI_EN_MSK);
    val |= CTL_BIT_XSPI_EN_VAL(en);

    writel(val, XSPI_REG_CTL(base));
}

static inline void xspi_hw_set_xip_en(u32 base, u8 en)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_CTL(base));

    val &= ~(CTL_BIT_XIP_EN_MSK);
    val |= CTL_BIT_XIP_EN_VAL(en);

    writel(val, XSPI_REG_CTL(base));
}


static inline void xspi_hw_set_cs_sel(u32 base, xspi_cs_sel_t sel)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    u32 val = readl(XSPI_REG_TCR(base));

    val &= ~(TCR_BIT_CS_SEL_MSK);
    val |= TCR_BIT_CS_SEL_VAL(sel);

    writel(val, XSPI_REG_TCR(base));
}

static inline void xspi_hw_set_address(u32 base, u32 address)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    while(XSPI_BUSY == xspi_hw_check_idle_status(base)){};
    writel(address, XSPI_REG_ADDR(base));
}


static inline void xspi_hw_reset_fifo(u32 base)
{
    u32 val = readl(XSPI_REG_FCR(base));
    val |= (FCR_BIT_RF_RST_MSK | FCR_BIT_TX_FIFO_RST_MSK);
    writel(val, XSPI_REG_FCR(base));
}

#define XSPI_RX_WATERMARK 32
#define XSPI_TX_WATERMARK 32
static inline void xspi_hw_set_fifo_watermark(u32 base, u32 tx_wm, u32 rx_wm)
{
    u32 val = readl(XSPI_REG_FCR(base));
    val &= ~(FCR_BIT_TX_TRIG_LEVEL_MSK | FCR_BIT_RX_TRIG_LEVEL_MSK);
    val |= FCR_BIT_RX_TRIG_LEVEL_VAL(rx_wm);
    val |= FCR_BIT_TX_TRIG_LEVEL_VAL(tx_wm);
    writel(val, XSPI_REG_FCR(base));
}

static inline u32 xspi_hw_get_tx_fifo_cnt(u32 base)
{
    u32 val = readl(XSPI_REG_FSR(base));
    return ((val & FSR_BIT_TF_CNT_MSK) >> FSR_BIT_TF_CNT_OFS);
}

static inline void xspi_hw_write_fifo(u32 base, u8 *data, u32 len)
{
    while (len) {
        writeb(*data++, XSPI_REG_TDR(base));
        len--;
    }
}

static inline u32 xspi_hw_get_rx_fifo_cnt(u32 base)
{
    u32 val = readl(XSPI_REG_FSR(base));
    return ((val & FSR_BIT_RF_CNT_MSK) >> FSR_BIT_RF_CNT_OFS) * 4;
}

static inline void xspi_hw_read_fifo(u32 base, u8 *data, u32 len)
{
    while (len) {
        *data++ = readb(XSPI_REG_RDR(base));
        len--;
    }
}

/* capture config */
static inline void xspi_hw_set_dll_ctl(u32 base, xspi_cs_sel_t sel, xspi_dll_reg_icp_t reg_icp, xspi_dll_phase_sel_t phase_sel)
{
    u32 val = 0;
    volatile void *reg_addr = XSPI_REG_CS0_DCTL(base);
    if (sel == cs0) {
        reg_addr = XSPI_REG_CS0_DCTL(base);
    } else {
        reg_addr = XSPI_REG_CS1_DCTL(base);
    }

    val = readl(reg_addr);
    val &= ~(CSX_DCTL_BIT_PHASE_SEL_MSK);
    val |= (CSX_DCTL_BIT_REG_ICP_VAL(reg_icp) |
            CSX_DCTL_BIT_PHASE_SEL_VAL(phase_sel) |
            CSX_DCTL_BIT_EN_LDO_VAL(1) |
            CSX_DCTL_BIT_EN_LVS_VAL(1)
            );
    writel(val, reg_addr);
    aic_udelay(5);

    val = readl(reg_addr);
    /*0 ~14 : CSX_DCTL_BIT_EN_DLL_VAL(1) CSX_DCTL_BIT_EN_VCDL_VAL(1) CSX_DCTL_BIT_EN_CP_VAL(1) CSX_DCTL_BIT_EN_BYPASS_VAL(0)*/
    /*15    : CSX_DCTL_BIT_EN_DLL_VAL(0) CSX_DCTL_BIT_EN_VCDL_VAL(1) CSX_DCTL_BIT_EN_CP_VAL(0) CSX_DCTL_BIT_EN_BYPASS_VAL(1)*/
    if (phase_sel<= xspi_d_337b5 &&  phase_sel>= xspi_d_22b5) {
        val |= CSX_DCTL_BIT_EN_DLL_VAL(1);
        writel(val, reg_addr);
        aic_udelay(1);

        val |= CSX_DCTL_BIT_EN_VCDL_VAL(1);
        writel(val, reg_addr);
        aic_udelay(1);

        val |= CSX_DCTL_BIT_EN_CP_VAL(1);
        writel(val, reg_addr);
        aic_udelay(1);

        val &= ~(CSX_DCTL_BIT_EN_BYPASS_MSK);
        writel(val, reg_addr);
    } else if (phase_sel == xspi_dbypass) {
        val &= ~(CSX_DCTL_BIT_EN_DLL_MSK);
        writel(val, reg_addr);
        aic_udelay(1);

        val |= CSX_DCTL_BIT_EN_VCDL_VAL(1);
        writel(val, reg_addr);
        aic_udelay(1);

        val &= ~(CSX_DCTL_BIT_EN_CP_MSK);
        writel(val, reg_addr);
        aic_udelay(1);

        val |= CSX_DCTL_BIT_EN_BYPASS_VAL(1);
        writel(val, reg_addr);
    }

    aic_udelay(5);
}

static inline u32 xspi_hw_get_lut_cfg(u32 base, u32 lut_n)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    return readl(XSPI_REG_LUTN(base, lut_n));
}

static inline void xspi_hw_set_lut_cfg(u32 base, u32 lut_n, xspi_lut_ins_t ins_h, xspi_lut_io_t io_h, u32 oper_h, xspi_lut_ins_t ins_l, xspi_lut_io_t io_l, u32 oper_l)
{
    hal_log_debug("hal_xspi %s:%d\n", __FUNCTION__, __LINE__);
    u32 temp;
    temp = LUTN_BIT_INS_H_VAl(ins_h) | LUTN_BIT_IO_CFG_H_VAl(io_h) | LUTN_BIT_OPERAND_H_VAl(oper_h)
            | LUTN_BIT_INS_L_VAl(ins_l) | LUTN_BIT_IO_CFG_L_VAl(io_l) | LUTN_BIT_OPERAND_L_VAl(oper_l);

    writel(temp, XSPI_REG_LUTN(base, lut_n));
}

static inline void xspi_hw_lut_start(u32 base, u32 lut_id)
{
    writel(lut_id, XSPI_REG_START(base));
}

static inline void xspi_hw_lut_update(u32 base)
{
    writel(1, XSPI_REG_LUT_UP(base));
}

static inline void xspi_hw_lut_lock_ctl(u32 base, xspi_lut_lock_t lock_ctl)
{
    writel(lock_ctl,XSPI_REG_LCKCR(base));
}

static inline u8 xspi_hw_get_parallel_mode(u32 base)
{
    u32 val = readl(XSPI_REG_CTL(base));
    return ((val >> CTL_BIT_PARALLEL_MODE_OFS) & 0x01);
}

static inline void xspi_hw_set_phase_sel(u32 base, u8 sel, u8 phase_val)
{
    u32 val = 0;
    volatile void *reg_addr = XSPI_REG_CS0_DCTL(base);
    if (sel == cs0) {
        reg_addr = XSPI_REG_CS0_DCTL(base);
    } else {
        reg_addr = XSPI_REG_CS1_DCTL(base);
    }

    val = readl(reg_addr);
    val &= ~(CSX_DCTL_BIT_PHASE_SEL_MSK);
    val |= CSX_DCTL_BIT_PHASE_SEL_VAL(phase_val);
    writel(val, reg_addr);
}


static inline void xspi_hw_set_cs_iocfg(u32 base, u8 sel, u32 iocfg1_val, u32 iocfg2_val, u32 iocfg3_val, u32 iocfg4_val)
{
    if (sel == cs0) {
        writel(iocfg1_val, XSPI_REG_CS0_IOCFG1(base));
        writel(iocfg2_val, XSPI_REG_CS0_IOCFG2(base));
        writel(iocfg3_val, XSPI_REG_CS0_IOCFG3(base));
        writel((iocfg4_val & IOCFG4_BIT_IOCFG4_MSK), XSPI_REG_CS0_IOCFG4(base));
    } else if (sel == cs1) {
        writel(iocfg1_val, XSPI_REG_CS1_IOCFG1(base));
        writel(iocfg2_val, XSPI_REG_CS1_IOCFG2(base));
        writel(iocfg3_val, XSPI_REG_CS1_IOCFG3(base));
        writel((iocfg4_val & IOCFG4_BIT_IOCFG4_MSK), XSPI_REG_CS1_IOCFG4(base));
    }
}

static inline void xspi_hw_set_wrap_burst_split(u32 base, u8 enable)
{
    u32 val = 0;

    val = readl(XSPI_REG_CTL(base));
    val &= ~(CTL_BIT_AXI_BURST_MSK);
    val |= CTL_BIT_AXI_BURST_VAL(enable);

    writel(val, XSPI_REG_CTL(base));
}

static inline void xspi_hw_set_timeout(u32 base, u32 timeout)
{
    u32 val = 0;
    val = readl(XSPI_REG_CTL(base));
    val &= ~(CTL_BIT_TIMEOUT_MSK);
    val |= CTL_BIT_TIMEOUT_VAL(1);
    writel(val, XSPI_REG_CTL(base));

    val = timeout;
    writel(val, XSPI_REG_TO(base));

}


#ifdef __cplusplus
}
#endif

#endif
