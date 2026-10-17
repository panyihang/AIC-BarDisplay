/*
 * Copyright (c) 2024, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIPI_DISPLAY_H
#define MIPI_DISPLAY_H

/* MIPI DSI Processor-to-Peripheral transaction types */
#define MIPI_DSI_V_SYNC_START                        0x01
#define MIPI_DSI_V_SYNC_END                          0x11
#define MIPI_DSI_H_SYNC_START                        0x21
#define MIPI_DSI_H_SYNC_END                          0x31

#define MIPI_DSI_COLOR_MODE_OFF                      0x02
#define MIPI_DSI_COLOR_MODE_ON                       0x12
#define MIPI_DSI_SHUTDOWN_PERIPHERAL                 0x22
#define MIPI_DSI_TURN_ON_PERIPHERAL                  0x32

#define MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM         0x03
#define MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM         0x13
#define MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM         0x23

#define MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM        0x04
#define MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM        0x14
#define MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM        0x24

#define MIPI_DSI_DCS_SHORT_WRITE                     0x05
#define MIPI_DSI_DCS_SHORT_WRITE_PARAM               0x15

#define MIPI_DSI_DCS_READ                            0x06

#define MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE      0x37

#define MIPI_DSI_END_OF_TRANSMISSION                 0x08

#define MIPI_DSI_NULL_PACKET                         0x09
#define MIPI_DSI_BLANKING_PACKET                     0x19
#define MIPI_DSI_GENERIC_LONG_WRITE                  0x29
#define MIPI_DSI_DCS_LONG_WRITE                      0x39

#define MIPI_DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20 0x0c
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR24         0x1c
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR16         0x2c

#define MIPI_DSI_PACKED_PIXEL_STREAM_30              0x0d
#define MIPI_DSI_PACKED_PIXEL_STREAM_36              0x1d
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR12         0x3d

#define MIPI_DSI_PACKED_PIXEL_STREAM_16              0x0e
#define MIPI_DSI_PACKED_PIXEL_STREAM_18              0x1e
#define MIPI_DSI_PIXEL_STREAM_3BYTE_18               0x2e
#define MIPI_DSI_PACKED_PIXEL_STREAM_24              0x3e


/* MIPI DCS commands */
#define MIPI_DCS_NOP                     0x00
#define MIPI_DCS_SOFT_RESET              0x01
#define MIPI_DCS_GET_DISPLAY_ID          0x04
#define MIPI_DCS_GET_RED_CHANNEL         0x06
#define MIPI_DCS_GET_GREEN_CHANNEL       0x07
#define MIPI_DCS_GET_BLUE_CHANNEL        0x08
#define MIPI_DCS_GET_DISPLAY_STATUS      0x09
#define MIPI_DCS_GET_POWER_MODE          0x0A
#define MIPI_DCS_GET_ADDRESS_MODE        0x0B
#define MIPI_DCS_GET_PIXEL_FORMAT        0x0C
#define MIPI_DCS_GET_DISPLAY_MODE        0x0D
#define MIPI_DCS_GET_SIGNAL_MODE         0x0E
#define MIPI_DCS_GET_DIAGNOSTIC_RESULT   0x0F
#define MIPI_DCS_ENTER_SLEEP_MODE        0x10
#define MIPI_DCS_EXIT_SLEEP_MODE         0x11
#define MIPI_DCS_ENTER_PARTIAL_MODE      0x12
#define MIPI_DCS_ENTER_NORMAL_MODE       0x13
#define MIPI_DCS_EXIT_INVERT_MODE        0x20
#define MIPI_DCS_ENTER_INVERT_MODE       0x21
#define MIPI_DCS_SET_GAMMA_CURVE         0x26
#define MIPI_DCS_SET_DISPLAY_OFF         0x28
#define MIPI_DCS_SET_DISPLAY_ON          0x29
#define MIPI_DCS_SET_COLUMN_ADDRESS      0x2A
#define MIPI_DCS_SET_PAGE_ADDRESS        0x2B
#define MIPI_DCS_WRITE_MEMORY_START      0x2C
#define MIPI_DCS_WRITE_LUT               0x2D
#define MIPI_DCS_READ_MEMORY_START       0x2E
#define MIPI_DCS_SET_PARTIAL_AREA        0x30
#define MIPI_DCS_SET_SCROLL_AREA         0x33
#define MIPI_DCS_SET_TEAR_OFF            0x34
#define MIPI_DCS_SET_TEAR_ON             0x35
#define MIPI_DCS_SET_ADDRESS_MODE        0x36
#define MIPI_DCS_SET_SCROLL_START        0x37
#define MIPI_DCS_EXIT_IDLE_MODE          0x38
#define MIPI_DCS_ENTER_IDLE_MODE         0x39
#define MIPI_DCS_SET_PIXEL_FORMAT        0x3A
#define MIPI_DCS_WRITE_MEMORY_CONTINUE   0x3C
#define MIPI_DCS_READ_MEMORY_CONTINUE    0x3E
#define MIPI_DCS_SET_TEAR_SCANLINE       0x44
#define MIPI_DCS_GET_SCANLINE            0x45
#define MIPI_DCS_SET_DISPLAY_BRIGHTNESS  0x51
#define MIPI_DCS_GET_DISPLAY_BRIGHTNESS  0x52
#define MIPI_DCS_WRITE_CONTROL_DISPLAY   0x53
#define MIPI_DCS_GET_CONTROL_DISPLAY     0x54
#define MIPI_DCS_WRITE_POWER_SAVE        0x55
#define MIPI_DCS_GET_POWER_SAVE          0x56
#define MIPI_DCS_SET_CABC_MIN_BRIGHTNESS 0x5E
#define MIPI_DCS_GET_CABC_MIN_BRIGHTNESS 0x5F
#define MIPI_DCS_READ_DDB_START          0xA1
#define MIPI_DCS_READ_DDB_CONTINUE       0xA8

#endif

