/*
 * Copyright (c) 2023-2026, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MIPI_DSI_REG_V1_0_H_
#define _MIPI_DSI_REG_V1_0_H_

#include <aic_core.h>
#include <artinchip_fb.h>
#include "aic_hal_disp_reg_util.h"

enum dsi_mode {
    DSI_MOD_VID_PULSE        = BIT(0),
    DSI_MOD_VID_EVENT        = BIT(1),
    DSI_MOD_VID_BURST        = BIT(2),
    DSI_MOD_CMD_MODE         = BIT(3),

    DSI_CLOCK_NON_CONTINUOUS = BIT(4),
    /* disable EoT packets in HS mode */
    DSI_MOD_NO_EOT_PACKET    = BIT(5),
    /* hback-porch area sends DSI Blanking Pack in HS mode and not enter LP mode */
    DSI_MOD_VID_HBP          = BIT(6),
};

enum dsi_format {
    DSI_FMT_RGB888 = 0,
    DSI_FMT_RGB666L = 1,
    DSI_FMT_RGB666 = 2,
    DSI_FMT_RGB565 = 3,
    DSI_FMT_MAX
};

#define DSI_MAX_LANE_NUM    4

#define DSI_CTL_PKG_CFG_CRC_RX_EN   BIT(7)
#define DSI_CTL_PKG_CFG_ECC_RX_EN   BIT(6)
#define DSI_CTL_PKG_CFG_BTA_EN      BIT(5)
#define DSI_CTL_PKG_CFG_EOTP_RX_EN  BIT(4)
#define DSI_CTL_PKG_CFG_EOTP_TX_EN  BIT(3)
#define DSI_CTL_DSI_MODE        BIT(1)
#define DSI_CTL_EN          BIT(0)

#define DSI_CLK_CFG_TO_DIV_MASK     GENMASK(15, 8)
#define DSI_CLK_CFG_TO_DIV(x)       (((x) & 0xFF) << 8)
#define DSI_CLK_CFG_LP_DIV_MASK     GENMASK(7, 0)
#define DSI_CLK_CFG_LP_DIV(x)       (((x) & 0xFF) << 0)

#define DSI_DPI_VC_NUM_MASK     GENMASK(1, 0)
#define DSI_DPI_VC_NUM(x)       (((x) & 0x3) << 0)

#define DSI_DPI_IN_FMT_LOOSELY18    BIT(8)
#define DSI_DPI_IN_FMT_DT_MASK      GENMASK(3, 0)
#define DSI_DPI_IN_FMT_DT(x)        (((x) & 0xF) << 0)

#define DSI_DPI_IN_POL_COLORM       BIT(4)
#define DSI_DPI_IN_POL_SHUTDOWN     BIT(3)
#define DSI_DPI_IN_POL_HSYNC        BIT(2)
#define DSI_DPI_IN_POL_VSYNC        BIT(1)
#define DSI_DPI_IN_POL_DE       BIT(0)

#define DSI_POL_HIGH_ACTIVE     0
#define DSI_POL_LOW_ACTIVE      1

#define DSI_DPI_LPTX_TIME_OUTVACT_MASK  GENMASK(24, 16)
#define DSI_DPI_LPTX_TIME_OUTVACT(x)    (((x) & 0xFF) << 16)
#define DSI_DPI_LPTX_TIME_INVACT_MASK   GENMASK(7, 0)
#define DSI_DPI_LPTX_TIME_INVACT(x) (((x) & 0xFF) << 0)

#define DSI_VID_MODE_CFG_TEST_DIR       BIT(24)
#define DSI_VID_MODE_CFG_TEST_MODE      BIT(20)
#define DSI_VID_MODE_CFG_TEST_EN        BIT(16)
#define DSI_VID_MODE_CFG_CMD_LPTX_FORCE     BIT(15)
#define DSI_VID_MODE_CFG_FRAME_BTA_ACK_EN   BIT(14)
#define DSI_VID_MODE_CFG_LP_EN_HFP      BIT(13)
#define DSI_VID_MODE_CFG_LP_EN_HBP      BIT(12)
#define DSI_VID_MODE_CFG_LP_EN_VACT     BIT(11)
#define DSI_VID_MODE_CFG_LP_EN_VFP      BIT(10)
#define DSI_VID_MODE_CFG_LP_EN_VBP      BIT(9)
#define DSI_VID_MODE_CFG_LP_EN_VSA      BIT(8)
#define DSI_VID_MODE_CFG_TYPE_MASK      GENMASK(1, 0)
#define DSI_VID_MODE_CFG_TYPE(x)        ((((x) >> 1) & 0x3) << 0)

#define DSI_VID_HBP_TIME_MASK           GENMASK(27, 16)
#define DSI_VID_HBP_TIME(x)         (((x) & 0xFFF) << 16)
#define DSI_VID_HSA_TIME_MASK           GENMASK(11, 0)
#define DSI_VID_HSA_TIME(x)         (((x) & 0xFFF) << 0)

#define DSI_VID_VFP_LINE_MASK           GENMASK(25, 16)
#define DSI_VID_VFP_LINE(x)         (((x) & 0x3FF) << 16)
#define DSI_VID_VBP_TIME_MASK           GENMASK(9, 0)
#define DSI_VID_VBP_TIME(x)         (((x) & 0x3FF) << 0)

#define DSI_VID_VSA_LINE_MASK           GENMASK(25, 16)
#define DSI_VID_VSA_LINE(x)         (((x) & 0x3FF) << 16)
#define DSI_VID_VACT_TIME_MASK          GENMASK(13, 0)
#define DSI_VID_VACT_TIME(x)            (((x) & 0x3FFF) << 0)

#define DSI_CMD_MODE_CFG_PKG_TX_HS      0
#define DSI_CMD_MODE_CFG_PKG_TX_LP      1
#define DSI_CMD_MODE_CFG_MAX_RD_PKG_SIZE    BIT(24)
#define DSI_CMD_MODE_CFG_DCS_LW         BIT(19)
#define DSI_CMD_MODE_CFG_DCS_SR_0P      BIT(18)
#define DSI_CMD_MODE_CFG_DCS_SW_1P      BIT(17)
#define DSI_CMD_MODE_CFG_DCS_SW_0P      BIT(16)
#define DSI_CMD_MODE_CFG_GEN_LW         BIT(14)
#define DSI_CMD_MODE_CFG_GEN_SR_2P      BIT(13)
#define DSI_CMD_MODE_CFG_GEN_SR_1P      BIT(12)
#define DSI_CMD_MODE_CFG_GEN_SR_0P      BIT(11)
#define DSI_CMD_MODE_CFG_GEN_SW_2P      BIT(10)
#define DSI_CMD_MODE_CFG_GEN_SW_1P      BIT(9)
#define DSI_CMD_MODE_CFG_GEN_SW_0P      BIT(8)
#define DSI_CMD_MODE_CFG_ACK_REQ_EN     BIT(1)
#define DSI_CMD_MODE_CFG_TE_EN          BIT(0)

#define DSI_CMD_PKG_STA_RD_CMD_BUSY     BIT(6)
#define DSI_CMD_PKG_STA_PLD_R_FULL      BIT(5)
#define DSI_CMD_PKG_STA_PLD_R_EMPTY     BIT(4)
#define DSI_CMD_PKG_STA_PLD_W_FULL      BIT(3)
#define DSI_CMD_PKG_STA_PLD_W_EMPTY_SHIFT   2
#define DSI_CMD_PKG_STA_PLD_W_EMPTY     BIT(2)
#define DSI_CMD_PKG_STA_CMD_FULL        BIT(1)
#define DSI_CMD_PKG_STA_CMD_EMPTY       BIT(0)

#define DSI_IRQ_EN1_PHY_ERR_EN_DATA0_ERRCONTENTION_LP1           BIT(20)
#define DSI_IRQ_EN1_PHY_ERR_EN_DATA0_ERRCONTENTION_LP0           BIT(19)
#define DSI_IRQ_EN1_PHY_ERR_EN_DATA0_ERRCONTROL                  BIT(18)
#define DSI_IRQ_EN1_PHY_ERR_EN_DATA0_ERRSYNCESC_LPDT             BIT(17)
#define DSI_IRQ_EN1_PHY_ERR_EN_DATA0_ERRESC_ESCAPE_ENTRY         BIT(16)
#define DSI_IRQ_EN1_PHY_ERR_EN_MASK                              GENMASK(20, 16)
#define DSI_IRQ_EN1_PHY_ERR_EN_ALL                               (0x1F0000)

#define DSI_IRQ_EN1_ACK_ERR_EN_DSI_PROTOCOL_VIOLATION            BIT(15)
#define DSI_IRQ_EN1_ACK_ERR_EN_INVALID_TRANSMISSION_LENGTH       BIT(13)
#define DSI_IRQ_EN1_ACK_ERR_EN_DSI_VC_ID_INVALID                 BIT(12)
#define DSI_IRQ_EN1_ACK_ERR_EN_DSI_DATA_TYPE_NOT_RECOGNIZED      BIT(11)
#define DSI_IRQ_EN1_ACK_ERR_EN_LONG_PACKET_CHECKSUM_ERROR        BIT(10)
#define DSI_IRQ_EN1_ACK_ERR_EN_ECC_ERROR_MULTI_BIT               BIT(9)
#define DSI_IRQ_EN1_ACK_ERR_EN_ECC_ERROR_SINGLE_BIT              BIT(8)
#define DSI_IRQ_EN1_ACK_ERR_EN_CONTENTION_DETECTED               BIT(7)
#define DSI_IRQ_EN1_ACK_ERR_EN_FALSE_CONTROL_ERROR               BIT(6)
#define DSI_IRQ_EN1_ACK_ERR_EN_PERIPHERAL_TIMEOUT_ERROR          BIT(5)
#define DSI_IRQ_EN1_ACK_ERR_EN_LOW_POWER_TRANSMIT_SYNC_ERROR     BIT(4)
#define DSI_IRQ_EN1_ACK_ERR_EN_ESCAPE_MODE_ENTRY_COMMAND_ERROR   BIT(3)
#define DSI_IRQ_EN1_ACK_ERR_EN_EOT_SYNC_ERROR                    BIT(2)
#define DSI_IRQ_EN1_ACK_ERR_EN_SOT_SYNC_ERROR                    BIT(1)
#define DSI_IRQ_EN1_ACK_ERR_EN_SOT_ERROR                         BIT(0)
#define DSI_IRQ_EN1_ACK_ERR_EN_ALL_MASK                          GENMASK(15, 0)
#define DSI_IRQ_EN1_ACK_ERR_EN_ALL                               (0xFFFF)

#define DSI_IRQ_EN1_ALL_MASK                                    GENMASK(20, 0)
#define DSI_IRQ_EN1_ALL                                         (DSI_IRQ_EN1_PHY_ERR_EN_ALL | \
                                                                 DSI_IRQ_EN1_ACK_ERR_EN_ALL)

#define DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTENTION_LP1         BIT(20)
#define DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTENTION_LP0         BIT(19)
#define DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRCONTROL                BIT(18)
#define DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRSYNCESC_LPDT           BIT(17)
#define DSI_IRQ_STA1_PHY_ERR_STA_DATA0_ERRESC_ESCAPE_ENTRY       BIT(16)

#define DSI_IRQ_STA1_ACK_ERR_STA_DSI_PROTOCOL_VIOLATION          BIT(15)
#define DSI_IRQ_STA1_ACK_ERR_STA_INVALID_TRANSMISSION_LENGTH     BIT(13)
#define DSI_IRQ_STA1_ACK_ERR_STA_DSI_VC_ID_INVALID               BIT(12)
#define DSI_IRQ_STA1_ACK_ERR_STA_DSI_DATA_TYPE_NOT_RECOGNIZED    BIT(11)
#define DSI_IRQ_STA1_ACK_ERR_STA_LONG_PACKET_CHECKSUM_ERROR      BIT(10)
#define DSI_IRQ_STA1_ACK_ERR_STA_ECC_ERROR_MULTI_BIT             BIT(9)
#define DSI_IRQ_STA1_ACK_ERR_STA_ECC_ERROR_SINGLE_BIT            BIT(8)
#define DSI_IRQ_STA1_ACK_ERR_STA_CONTENTION_DETECTED             BIT(7)
#define DSI_IRQ_STA1_ACK_ERR_STA_FALSE_CONTROL_ERROR             BIT(6)
#define DSI_IRQ_STA1_ACK_ERR_STA_PERIPHERAL_TIMEOUT_ERROR        BIT(5)
#define DSI_IRQ_STA1_ACK_ERR_STA_LOW_POWER_TRANSMIT_SYNC_ERROR   BIT(4)
#define DSI_IRQ_STA1_ACK_ERR_STA_ESCAPE_MODE_ENTRY_COMMAND_ERROR BIT(3)
#define DSI_IRQ_STA1_ACK_ERR_STA_EOT_SYNC_ERROR                  BIT(2)
#define DSI_IRQ_STA1_ACK_ERR_STA_SOT_SYNC_ERROR                  BIT(1)
#define DSI_IRQ_STA1_ACK_ERR_STA_SOT_ERROR                       BIT(0)

#define DSI_IRQ_EN2_GEN_PLDRD_OV_EN             BIT(12)
#define DSI_IRQ_EN2_GEN_PLDRD_UD_EN             BIT(11)
#define DSI_IRQ_EN2_GEN_PLDSEND_UD_EN           BIT(10)
#define DSI_IRQ_EN2_GEN_PLD_OV_EN               BIT(9)
#define DSI_IRQ_EN2_GEN_CMD_OV_EN               BIT(8)
#define DSI_IRQ_EN2_DPI_FIFO_OV_EN              BIT(7)
#define DSI_IRQ_EN2_EOPT_ERR_EN                 BIT(6)
#define DSI_IRQ_EN2_PSIZE_ERR_EN                BIT(5)
#define DSI_IRQ_EN2_CRC_ERR_EN                  BIT(4)
#define DSI_IRQ_EN2_ECC_MERR_EN                 BIT(3)
#define DSI_IRQ_EN2_ECC_SERR_EN                 BIT(2)
#define DSI_IRQ_EN2_TO_LPRX_EN                  BIT(1)
#define DSI_IRQ_EN2_TO_HSTX_EN                  BIT(0)
#define DSI_IRQ_EN2_ALL_MASK                    GENMASK(12, 0)
#define DSI_IRQ_EN2_ALL                         (0x1FFF)

#define DSI_IRQ_STA2_GEN_PLDRD_OV_STA           BIT(12)
#define DSI_IRQ_STA2_GEN_PLDRD_UD_STA           BIT(11)
#define DSI_IRQ_STA2_GEN_PLDSEND_UD_STA         BIT(10)
#define DSI_IRQ_STA2_GEN_PLD_OV_STA             BIT(9)
#define DSI_IRQ_STA2_GEN_CMD_OV_STA             BIT(8)
#define DSI_IRQ_STA2_DPI_FIFO_OV_STA            BIT(7)
#define DSI_IRQ_STA2_EOPT_ERR_STA               BIT(6)
#define DSI_IRQ_STA2_PSIZE_ERR_STA              BIT(5)
#define DSI_IRQ_STA2_CRC_ERR_STA                BIT(4)
#define DSI_IRQ_STA2_ECC_MERR_STA               BIT(3)
#define DSI_IRQ_STA2_ECC_SERR_STA               BIT(2)
#define DSI_IRQ_STA2_TO_LPRX_STA                BIT(1)
#define DSI_IRQ_STA2_TO_HSTX_STA                BIT(0)

#define DSI_TO_CNT_CFG_HSTX_MASK    GENMASK(31, 16)
#define DSI_TO_CNT_CFG_HSTX(x)      (((x) & 0x3FF) << 16)
#define DSI_TO_CNT_CFG_LPRX_MASK    GENMASK(15, 0)
#define DSI_TO_CNT_CFG_LPRX(x)      (((x) & 0x3FF) << 0)

#define DSI_PHY_CLK_TIME_HS2LP_MASK GENMASK(25, 16)
#define DSI_PHY_CLK_TIME_HS2LP(x)   (((x) & 0x1FF) << 16)
#define DSI_PHY_CLK_TIME_LP2HS_MASK GENMASK(9, 0)
#define DSI_PHY_CLK_TIME_LP2HS(x)   (((x) & 0x1FF) << 0)

#define DSI_PHY_DATA_TIME_HS2LP_MASK    GENMASK(25, 16)
#define DSI_PHY_DATA_TIME_HS2LP(x)  (((x) & 0x1FF) << 16)
#define DSI_PHY_DATA_TIME_LP2HS_MASK    GENMASK(9, 0)
#define DSI_PHY_DATA_TIME_LP2HS(x)  (((x) & 0x1FF) << 0)

#define DSI_PHY_CFG_RST_PLL_FORCE       BIT(7)
#define DSI_PHY_CFG_RST_CLK_EN          BIT(6)
#define DSI_PHY_CFG_RST_RSTN            BIT(5)
#define DSI_PHY_CFG_RST_SHUTDOWNZ       BIT(4)
#define DSI_PHY_CFG_AUTO_CLK_EN         BIT(3)
#define DSI_PHY_CFG_HSCLK_REQ           BIT(2)

#define DSI_PHY_CFG_LP11_TIME_SHIFT 8
#define DSI_PHY_CFG_LP11_TIME_MASK  GENMASK(15, 8)
#define DSI_PHY_CFG_LP11_TIME(x)    (((x) & 0xFF) << 8)
#define DSI_PHY_CFG_DATA_LANE_MASK  GENMASK(1, 0)
#define DSI_PHY_CFG_DATA_LANE(x)    (((x) & 0x3) << 0)

#define DSI_PHY_STA_ULPS_ACTIVE_NOT_4   BIT(12)
#define DSI_PHY_STA_STOP_STATE_3    BIT(11)
#define DSI_PHY_STA_ULPS_ACTIVE_NOT_2   BIT(10)
#define DSI_PHY_STA_STOP_STATE_2    BIT(9)
#define DSI_PHY_STA_ULPS_ACTIVE_NOT_1   BIT(8)
#define DSI_PHY_STA_STOP_STATE_1    BIT(7)
#define DSI_PHY_STA_RXULPS_ESC_0    BIT(6)
#define DSI_PHY_STA_ULPS_ACTIVE_NOT_0   BIT(5)
#define DSI_PHY_STA_STOP_STATE_0_SHIFT  4
#define DSI_PHY_STA_STOP_STATE_0    BIT(4)
#define DSI_PHY_STA_ULPS_ACTIVE_NOT_C   BIT(3)
#define DSI_PHY_STA_STOP_STATE_C_SHIFT  2
#define DSI_PHY_STA_STOP_STATE_C    BIT(2)
#define DSI_PHY_STA_PHY_DIRECTION   BIT(1)
#define DSI_PHY_STA_PHY_LOCK        BIT(0)

#define DSI_PHY_TEST1_CLK       BIT(1)
#define DSI_PHY_TEST1_CLR       BIT(0)

#define DSI_PHY_TEST2_EN        BIT(16)
#define DSI_PHY_TEST2_DOUT_MASK     GENMASK(15, 8)
#define DSI_PHY_TEST2_DOUT(x)       (((x) & 0xFF) << 8)
#define DSI_PHY_TEST2_DIN_MASK      GENMASK(7, 0)
#define DSI_PHY_TEST2_DIN(x)        (((x) & 0xFF) << 0)

#define DSI_DPI_IF_CFG_COLORM       BIT(3)
#define DSI_DPI_IF_CFG_SHUTD        BIT(2)
#define DSI_DPI_IF_CFG_FMT_MASK     GENMASK(1, 0)
#define DSI_DPI_IF_CFG_FMT(x)       (((x) & 0x3) << 0)

#define DSI_ANA_CFG2_EN_CLK     BIT(12)
#define DSI_ANA_CFG2_RCAL_SHIFT     10
#define DSI_ANA_CFG2_RCAL_FLAG      BIT(10)
#define DSI_ANA_CFG2_ON_RESCAL      BIT(9)
#define DSI_ANA_CFG2_EN_RESCAL      BIT(8)
#define DSI_ANA_CFG2_EN_LDO_MASK    GENMASK(7, 3)
#define DSI_ANA_CFG2_EN_LDO(x)      (((x) & 0x1F) << 3)
#define DSI_ANA_CFG2_EN_VP_MASK     GENMASK(2, 1)
#define DSI_ANA_CFG2_EN_VP(x)       (((x) & 0x3) << 1)
#define DSI_ANA_CFG2_EN_BIAS        BIT(0)

#define DSI_DATA_LANE_POL_MASK      GENMASK(19, 16)
#define DSI_DATA_LANE_POL(x)        (((x) & 0xF) << 16)
#define DSI_DATA_CLK_POL            BIT(15)

/* The register of MIPI DSI controller */
#define DSI_CTL         0x00
#define DSI_CLK_CFG     0x04
#define DSI_DPI_IN_POL      0x10
#define DSI_DPI_IN_FMT      0x14
#define DSI_DPI_VC      0x18
#define DSI_DPI_LPTX_TIME   0x1C
#define DSI_GEN_PH_CFG      0x20
#define DSI_GEN_PD_CFG      0x24
#define DSI_GEN_VC_RX       0x28
#define DSI_VID_MODE_CFG    0x30
#define DSI_VID_PKG_SIZE    0x34
#define DSI_VID_CHK_NUM     0x38
#define DSI_VID_NULL_SIZE   0x3C
#define DSI_VID_HINACT_TIME 0x40
#define DSI_VID_HT_TIME     0x44
#define DSI_VID_VBLANK_LINE 0x48
#define DSI_VID_VACT_LINE   0x4C
#define DSI_CMD_MODE_CFG    0x50
#define DSI_EDPI_CMD_SIZE   0x54
#define DSI_CMD_PKG_STA     0x58
#define DSI_IRQ_EN1     0x60
#define DSI_IRQ_EN2     0x64
#define DSI_IRQ_STA1        0x68
#define DSI_IRQ_STA2        0x6C
#define DSI_TO_CNT_CFG      0x70
#define DSI_TO_RD       0x74
#define DSI_TO_WR       0x78
#define DSI_TO_BTA      0x7C
#define DSI_PHY_CFG     0x80
#define DSI_PHY_STA     0x84
#define DSI_PHY_CLK_TIME    0x88
#define DSI_PHY_DATA_TIME   0x8C
#define DSI_PHY_RD_TIME     0x90
#define DSI_PHY_TEST1       0xC0
#define DSI_PHY_TEST2       0xC4
#define DSI_DPI_IF_CFG      0x400
#define DSI_LANE_CFG        0x410
#define DSI_ANA_CFG1        0x500
#define DSI_ANA_CFG2        0x504
#define DSI_ANA_CFG3        0x508
#define DSI_ANA_CFG4        0x50C

/* Processor to Peripheral Direction (Processor-Sourced) Packet Data Types */
#define DSI_DT_VSS      0x01
#define DSI_DT_VSE      0x11
#define DSI_DT_HSS      0x21
#define DSI_DT_HSE      0x31
#define DSI_DT_EOT      0x08
#define DSI_DT_CM_OFF       0x02
#define DSI_DT_CM_ON        0x12
#define DSI_DT_SHUT_DOWN    0x22
#define DSI_DT_TURN_ON      0x32
#define DSI_DT_GEN_WR_P0    0x03
#define DSI_DT_GEN_WR_P1    0x13
#define DSI_DT_GEN_WR_P2    0x23
#define DSI_DT_GEN_RD_P0    0x04
#define DSI_DT_GEN_RD_P1    0x14
#define DSI_DT_GEN_RD_P2    0x24
#define DSI_DT_DCS_WR_P0    0x05
#define DSI_DT_DCS_WR_P1    0x15
#define DSI_DT_DCS_RD_P0    0x06
#define DSI_DT_MAX_RET_SIZE 0x37
#define DSI_DT_NULL     0x09
#define DSI_DT_BLK      0x19
#define DSI_DT_GEN_LONG_WR  0x29
#define DSI_DT_DCS_LONG_WR  0x39
#define DSI_DT_PIXEL_RGB565 0x0E
#define DSI_DT_PIXEL_RGB666P    0x1E
#define DSI_DT_PIXEL_RGB666 0x2E
#define DSI_DT_PIXEL_RGB888 0x3E
/* Data Types for Peripheral-sourced Packets */
#define DSI_DT_ACK_ERR      0x02
#define DSI_DT_EOT_PERI     0x08
#define DSI_DT_GEN_RD_R1    0x11
#define DSI_DT_GEN_RD_R2    0x12
#define DSI_DT_GEN_LONG_RD_R    0x1A
#define DSI_DT_DCS_LONG_RD_R    0x1C
#define DSI_DT_DCS_RD_R1    0x21
#define DSI_DT_DCS_RD_R2    0x22

#define DSI_DCS_ENTER_IDLE_MODE     0x39
#define DSI_DCS_ENTER_INVERT_MODE   0x21
#define DSI_DCS_ENTER_NORMAL_MODE   0x13
#define DSI_DCS_ENTER_PARTIAL_MODE  0x12
#define DSI_DCS_ENTER_SLEEP_MODE    0x10
#define DSI_DCS_EXIT_IDLE_MODE      0x38
#define DSI_DCS_EXIT_INVERT_MODE    0x20
#define DSI_DCS_EXIT_SLEEP_MODE     0x11
#define DSI_DCS_GET_ADDRESS_MODE    0x0b
#define DSI_DCS_GET_BLUE_CHANNEL    0x08
#define DSI_DCS_GET_DIAGNOSTIC_RESULT   0x0f
#define DSI_DCS_GET_DISPLAY_MODE    0x0d
#define DSI_DCS_GET_GREEN_CHANNEL   0x07
#define DSI_DCS_GET_PIXEL_FORMAT    0x0c
#define DSI_DCS_GET_POWER_MODE      0x0a
#define DSI_DCS_GET_RED_CHANNEL     0x06
#define DSI_DCS_GET_SCANLINE        0x45
#define DSI_DCS_GET_SIGNAL_MODE     0x0e
#define DSI_DCS_NOP         0x00
#define DSI_DCS_READ_DDB_CONTINUE   0xa8
#define DSI_DCS_READ_DDB_START      0xa1
#define DSI_DCS_READ_MEMORY_CONTINUE    0x3e
#define DSI_DCS_READ_MEMORY_START   0x2e
#define DSI_DCS_SET_ADDRESS_MODE    0x36
#define DSI_DCS_SET_COLUMN_ADDRESS  0x2a
#define DSI_DCS_SET_DISPLAY_OFF     0x28
#define DSI_DCS_SET_DISPLAY_ON      0x29
#define DSI_DCS_SET_GAMMA_CURVE     0x26
#define DSI_DCS_SET_PAGE_ADDRESS    0x2b
#define DSI_DCS_SET_PARTIAL_AREA    0x30
#define DSI_DCS_SET_PIXEL_FORMAT    0x3a
#define DSI_DCS_SET_SCROLL_AREA     0x33
#define DSI_DCS_SET_SCROLL_START    0x37
#define DSI_DCS_SET_TEAR_OFF        0x34
#define DSI_DCS_SET_TEAR_ON     0x35
#define DSI_DCS_SET_TEAR_SCANLINE   0x44
#define DSI_DCS_SOFT_RESET      0x01
#define DSI_DCS_WRITE_LUT       0x2d
#define DSI_DCS_WRITE_MEMORY_CONTINUE   0x3c
#define DSI_DCS_WRITE_MEMORY_START  0x2c

void dsi_set_lane_assign(void *base, u32 ln_assign);
void dsi_set_lane_polrs(void *base, u32 ln_polrs);
void dsi_set_data_clk_polrs(void *base, u32 dc_inv);

void dsi_set_clk_div(void *base, ulong mclk, ulong lp_rate);
void dsi_pkg_init(void *base, enum dsi_mode mode);
void dsi_phy_init(void *base, ulong mclk, u32 lane, enum dsi_mode mode);
void dsi_hs_clk(void *base, u32 enable);
void dsi_set_vm(void *base, enum dsi_mode mode, enum dsi_format format,
        u32 lane, u32 vc, const struct display_timing *timing);
void dsi_cmd_wr(void *base, u32 dt, u32 vc, const u8 *data, u32 len);
void dsi_dcs_lw(void *base, u32 enable);
void dsi_interrupt_init(void *base, u32 irq1_flag, u32 irq2_flag);

#endif // end of _MIPI_DSI_REG_V1_0_H_

