/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the flash types and specification macro definition head file for this library.
 * Created on: 2016-06-09
 */

#ifndef BSP_PERIPHERAL_SPINOR_SFUD_INC_SFUD_FLASH_DEF_H_
#define BSP_PERIPHERAL_SPINOR_SFUD_INC_SFUD_FLASH_DEF_H_

#include <stdint.h>
#include <sfud_cfg.h>
#include "sfud_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * flash program(write) data mode
 */
enum sfud_write_mode {
    SFUD_WM_PAGE_256B = 1 << 0,                            /**< write 1 to 256 bytes per page */
    SFUD_WM_BYTE = 1 << 1,                                 /**< byte write */
    SFUD_WM_AAI = 1 << 2,                                  /**< auto address increment */
    SFUD_WM_DUAL_BUFFER = 1 << 3,                          /**< dual-buffer write, like AT45DB series */
};

/* manufacturer information */
typedef struct {
    char *name;
    uint8_t id;
} sfud_mf;

/* flash chip information */
typedef struct {
    char *name;                                  /**< flash chip name */
    uint8_t mf_id;                               /**< manufacturer ID */
    uint8_t type_id;                             /**< memory type ID */
    uint8_t capacity_id;                         /**< capacity ID */
    uint32_t capacity;                           /**< flash capacity (bytes) */
    uint16_t write_mode;                         /**< write mode @see sfud_write_mode */
    struct {
        uint32_t size;                           /**< erase sector size (bytes). 0x00: not available */
        uint8_t cmd;                             /**< erase command */
    } eraser[2];
} sfud_flash_chip;

#ifdef SFUD_USING_QSPI
/* QSPI flash chip's extended information compared with SPI flash */
typedef struct {
    uint8_t mf_id;                               /**< manufacturer ID */
    uint8_t type_id;                             /**< memory type ID */
    uint8_t capacity_id;                         /**< capacity ID */
    uint8_t read_mode;                           /**< supported read mode on this qspi flash chip */
} sfud_qspi_flash_ext_info;

/*
*  QSPI flash chip's extended QE config information compared with SPI flash,
*  for special falsh(MX25L6433FM2I, MX25L12835FM2I) which is no support SFDP(or the basic_len < 15).
*/
typedef struct {
    uint8_t mf_id;                               /**< manufacturer ID */
    uint8_t type_id;                             /**< memory type ID */
    uint8_t capacity_id;                         /**< capacity ID */
    uint8_t wr_reg_status;                       /**< enable qe write status register addr */
    uint8_t rd_reg_status;                       /**< enable qe read status register addr */
    uint8_t qe_bit;                              /**< enable qe status register bit */
} sfud_qspi_flash_qe_info;
#endif

typedef struct {
    uint8_t mf_id;                               /**< manufacturer ID */
    uint8_t type_id;                             /**< memory type ID */
    uint8_t capacity_id;                         /**< capacity ID */
    uint8_t rd_cmd;                              /**< instruction of read Unique ID number */
    uint8_t byte1;                               /**< the 1st byte was sent to device immediately following the instruction */
    uint8_t byte2;                               /**< the 2nd byte was sent to device immediately following the instruction */
    uint8_t byte3;                               /**< the 3rd byte was sent to device immediately following the instruction */
    uint8_t byte4;                               /**< the 4th byte was sent to device immediately following the instruction */
    uint8_t id_len;                              /**< the length of Unique ID number, in bytes */
    uint32_t bp_mask;                           /**< the bit mask of block protect in status register */
} sfud_qspi_flash_private_info;

/* SFUD support manufacturer JEDEC ID */
#define SFUD_MF_ID_CYPRESS                             0x01
#define SFUD_MF_ID_FUJITSU                             0x04
#define SFUD_MF_ID_EON                                 0x1C
#define SFUD_MF_ID_ATMEL                               0x1F
#define SFUD_MF_ID_MICRON                              0x20
#define SFUD_MF_ID_AMIC                                0x37
#define SFUD_MF_ID_NOR_MEM                             0x52
#define SFUD_MF_ID_ZBIT                                0x5E
#define SFUD_MF_ID_SANYO                               0x62
#define SFUD_MF_ID_INTEL                               0x89
#define SFUD_MF_ID_ESMT                                0x8C
#define SFUD_MF_ID_FUDAN                               0xA1
#define SFUD_MF_ID_HYUNDAI                             0xAD
#define SFUD_MF_ID_SST                                 0xBF
#define SFUD_MF_ID_MACRONIX                            0xC2
#define SFUD_MF_ID_GIGADEVICE                          0xC8
#define SFUD_MF_ID_ISSI                                0x9D
#define SFUD_MF_ID_WINBOND                             0xEF
#define SFUD_MF_ID_ZETTA                               0xBA
#define SFUD_MF_ID_BOYA                                0x68
#define SFUD_MF_ID_XTX                                 0x0B
#define SFUD_MF_ID_PUYA                                0x85
#define SFUD_MF_ID_FUDANMICRO                          0x20

/*  | name | mf_id | type_id | read—_uid_cmd | addr0 | addr1 | addr2 | addr3 | id_len | */
#define SFUD_FLASH_PRIVATE_INFO_TABLE                                                              \
{                                                                                                  \
    /* BOYA128B */                                                                                 \
    {SFUD_MF_ID_BOYA, 0x40, 0x18, 0x48, 0x0, 0x0, 0x0, 0x0, 16,                                    \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* BOYA32B */                                                                                  \
    {SFUD_MF_ID_BOYA, 0x49, 0x19, 0x48, 0x0, 0x0, 0x0, 0x0, 16,                                    \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* XTX128B */                                                                                  \
    {SFUD_MF_ID_XTX, 0x40, 0x18, 0x5A, 0x0, 0x0, 0x94, 0xff, 16,                                   \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* "GD25Q256B", */                                                                             \
    {SFUD_MF_ID_GIGADEVICE, 0x40, 0x19, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                          \
        SNOR_F_HAS_4BIT_BP},                                                                       \
    /* "GD25Q128B", */                                                                             \
    {SFUD_MF_ID_GIGADEVICE, 0x40, 0x18, 0x4B, 0x0, 0x0, 0x0, 0xff, 16,                             \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* "GD25Q64B" */                                                                               \
    {SFUD_MF_ID_GIGADEVICE, 0x40, 0x17, 0x4B, 0x0, 0x0, 0x0, 0xff, 16,                             \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* "GD25Q32B" */                                                                               \
    {SFUD_MF_ID_GIGADEVICE, 0x40, 0x16, 0x4B, 0x0, 0x0, 0x0, 0xff, 16,                             \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* PUYA256B */                                                                                 \
    {SFUD_MF_ID_PUYA, 0x20, 0x19, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                                \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* PUYA128B */                                                                                 \
    {SFUD_MF_ID_PUYA, 0x20, 0x18, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                                \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* PUYA64B */                                                                                  \
    {SFUD_MF_ID_PUYA, 0x20, 0x17, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                                \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* PUYA32B */                                                                                  \
    {SFUD_MF_ID_PUYA, 0x20, 0x16, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                                \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* PUYA16B */                                                                                  \
    {SFUD_MF_ID_PUYA, 0x20, 0x15, 0x4B, 0xff, 0xff, 0xff, 0xff, 16,                                \
        SNOR_F_HAS_4BIT_BP | SNOR_F_HAS_SR_BP3_BIT6},                                              \
    /* ZB25VQ128 */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x18, 0x4B, 0x0, 0x0, 0x0, 0xff, 16, 0},                               \
    /* ZB25VQ164 */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x17, 0x4B, 0x0, 0x0, 0x0, 0xff, 16, 0},                               \
    /* ZB25VQ132 */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x16, 0x4B, 0x0, 0x0, 0x0, 0xff, 16, 0},                               \
    /* ZB25VQ116 */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x15, 0x4B, 0x0, 0x0, 0x0, 0xff, 16, 0},                               \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x18, 0x4B, 0x0, 0x0, 0x94, 0xff, 16,                              \
        SNOR_F_HAS_4BIT_BP},                                                                       \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x17, 0x4B, 0x0, 0x0, 0x94, 0xff, 16,                              \
        SNOR_F_HAS_4BIT_BP},                                                                       \
    {SFUD_MF_ID_ISSI, 0x60, 0x18, 0x4B, 0x0, 0x0, 0x94, 0xff, 16,                                  \
        SNOR_F_HAS_4BIT_BP},                                                                       \
}

/* SFUD supported manufacturer information table */
#define SFUD_MF_TABLE                                     \
{                                                         \
    {"Cypress",    SFUD_MF_ID_CYPRESS},                   \
    {"Fujitsu",    SFUD_MF_ID_FUJITSU},                   \
    {"EON",        SFUD_MF_ID_EON},                       \
    {"Atmel",      SFUD_MF_ID_ATMEL},                     \
    {"Micron",     SFUD_MF_ID_MICRON},                    \
    {"AMIC",       SFUD_MF_ID_AMIC},                      \
    {"Sanyo",      SFUD_MF_ID_SANYO},                     \
    {"Intel",      SFUD_MF_ID_INTEL},                     \
    {"ESMT",       SFUD_MF_ID_ESMT},                      \
    {"Fudan",      SFUD_MF_ID_FUDAN},                     \
    {"Hyundai",    SFUD_MF_ID_HYUNDAI},                   \
    {"SST",        SFUD_MF_ID_SST},                       \
    {"GigaDevice", SFUD_MF_ID_GIGADEVICE},                \
    {"ISSI",       SFUD_MF_ID_ISSI},                      \
    {"Winbond",    SFUD_MF_ID_WINBOND},                   \
    {"Macronix",   SFUD_MF_ID_MACRONIX},                  \
    {"NOR-MEM",    SFUD_MF_ID_NOR_MEM},                   \
    {"ZBIT",       SFUD_MF_ID_ZBIT},                      \
    {"ZETTA",      SFUD_MF_ID_ZETTA},                     \
    {"FUDANMICRO", SFUD_MF_ID_FUDANMICRO},                \
}

#ifdef SFUD_USING_FLASH_INFO_TABLE
/* SFUD supported flash chip information table. If the flash not support JEDEC JESD216 standard,
 * then the SFUD will find the flash chip information by this table. You can add other flash to here then
 *  notice me for update it. The configuration information name and index reference the sfud_flash_chip structure.
 * | name | mf_id | type_id | capacity_id | capacity | write_mode | eraser[2] |
 */
#define SFUD_FLASH_CHIP_TABLE                                                                                       \
{                                                                                                                   \
    {"AT45DB161E", SFUD_MF_ID_ATMEL, 0x26, 0x00, 2L*1024L*1024L, SFUD_WM_BYTE|SFUD_WM_DUAL_BUFFER, {{512, 0x81}, {0}}},      \
    {"W25Q40BV", SFUD_MF_ID_WINBOND, 0x40, 0x13, 512L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                        \
    {"W25X40CL", SFUD_MF_ID_WINBOND, 0x30, 0x13, 512L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                        \
    {"W25X16AV", SFUD_MF_ID_WINBOND, 0x30, 0x15, 2L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                    \
    {"W25Q16BV", SFUD_MF_ID_WINBOND, 0x40, 0x15, 2L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                    \
    {"W25Q32BV", SFUD_MF_ID_WINBOND, 0x40, 0x16, 4L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                    \
    {"W25Q64CV", SFUD_MF_ID_WINBOND, 0x40, 0x17, 8L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                    \
    {"W25Q64DW", SFUD_MF_ID_WINBOND, 0x60, 0x17, 8L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                    \
    {"W25Q128BV", SFUD_MF_ID_WINBOND, 0x40, 0x18, 16L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                  \
    {"W25Q256FV", SFUD_MF_ID_WINBOND, 0x40, 0x19, 32L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                  \
    {"MX25L25635E", SFUD_MF_ID_MACRONIX, 0x20, 0x19, 32L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},               \
    {"SST25VF080B", SFUD_MF_ID_SST, 0x25, 0x8E, 1L*1024L*1024L, SFUD_WM_BYTE|SFUD_WM_AAI, {{4096, 0x20}, {0}}},              \
    {"SST25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L*1024L*1024L, SFUD_WM_BYTE|SFUD_WM_AAI, {{4096, 0x20}, {0}}},              \
    {"M25P32", SFUD_MF_ID_MICRON, 0x20, 0x16, 4L*1024L*1024L, SFUD_WM_PAGE_256B, {{65536, 0xd8}, {0}}},                  \
    {"M25P80", SFUD_MF_ID_MICRON, 0x20, 0x14, 1L*1024L*1024L, SFUD_WM_PAGE_256B, {{65536, 0xd8}, {0}}},                  \
    {"M25P40", SFUD_MF_ID_MICRON, 0x20, 0x13, 512L*1024L, SFUD_WM_PAGE_256B, {{65536, 0xd8}, {0}}},                      \
    {"EN25Q32B", SFUD_MF_ID_EON, 0x30, 0x16, 4L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                        \
    {"GD25Q64B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x17, 8L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                 \
    {"GD25Q16B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x15, 2L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                 \
    {"GD25Q32C", SFUD_MF_ID_GIGADEVICE, 0x40, 0x16, 4L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                 \
    {"GD25Q128E", SFUD_MF_ID_GIGADEVICE, 0x40, 0x18, 16L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},               \
    {"GD25Q256E", SFUD_MF_ID_GIGADEVICE, 0x40, 0x19, 32L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},               \
    {"S25FL216K", SFUD_MF_ID_CYPRESS, 0x40, 0x15, 2L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                   \
    {"S25FL032P", SFUD_MF_ID_CYPRESS, 0x02, 0x15, 4L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                   \
    {"A25L080", SFUD_MF_ID_AMIC, 0x30, 0x14, 1L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                        \
    {"F25L004", SFUD_MF_ID_ESMT, 0x20, 0x13, 512L*1024L, SFUD_WM_BYTE|SFUD_WM_AAI, {{4096, 0x20}, {0}}},                     \
    {"PCT25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L*1024L*1024L, SFUD_WM_BYTE|SFUD_WM_AAI, {{4096, 0x20}, {0}}},              \
    {"NM25Q128EVB", SFUD_MF_ID_NOR_MEM, 0x21, 0x18, 16L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                \
    {"ZD25Q64B", SFUD_MF_ID_ZETTA, 0x32, 0x17, 8L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {0}}},                      \
    {"EFM25F128A", SFUD_MF_ID_FUDANMICRO, 0xBA, 0x18, 16L*1024L*1024L, SFUD_WM_PAGE_256B, {{4096, 0x20}, {65536, 0xd8}}},              \
}
#endif /* SFUD_USING_FLASH_INFO_TABLE */

#ifdef SFUD_USING_QSPI
/* This table saves flash read-fast instructions in QSPI mode,
 * SFUD can use this table to select the most appropriate read instruction for flash.
 * | mf_id | type_id | capacity_id | qspi_read_mode |
 */
#define SFUD_FLASH_EXT_INFO_TABLE                                                                  \
{                                                                                                  \
    /* W25Q40BV */                                                                                 \
    {SFUD_MF_ID_WINBOND, 0x40, 0x13, NORMAL_SPI_READ|DUAL_OUTPUT},                                 \
    /* W25Q80JV */                                                                                 \
    {SFUD_MF_ID_WINBOND, 0x40, 0x14, NORMAL_SPI_READ|DUAL_OUTPUT},                                 \
    /* W25Q16BV */                                                                                 \
    {SFUD_MF_ID_WINBOND, 0x40, 0x15, NORMAL_SPI_READ|DUAL_OUTPUT},                                 \
    /* W25Q32BV */                                                                                 \
    {SFUD_MF_ID_WINBOND, 0x40, 0x16, NORMAL_SPI_READ|DUAL_OUTPUT|QUAD_OUTPUT|QUAD_IO},             \
    /* W25Q64JV */                                                                                 \
    {SFUD_MF_ID_WINBOND, 0x40, 0x17, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},     \
    /* W25Q128JV */                                                                                \
    {SFUD_MF_ID_WINBOND, 0x40, 0x18, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},     \
    /* W25Q256FV */                                                                                \
    {SFUD_MF_ID_WINBOND, 0x40, 0x19, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},     \
    /* EN25Q32B */                                                                                 \
    {SFUD_MF_ID_EON, 0x30, 0x16, NORMAL_SPI_READ|DUAL_OUTPUT|QUAD_IO},                             \
    /* S25FL216K */                                                                                \
    {SFUD_MF_ID_CYPRESS, 0x40, 0x15, NORMAL_SPI_READ|DUAL_OUTPUT},                                 \
    /* A25L080 */                                                                                  \
    {SFUD_MF_ID_AMIC, 0x30, 0x14, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO},                            \
    /* A25LQ64 */                                                                                  \
    {SFUD_MF_ID_AMIC, 0x40, 0x17, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_IO},                    \
    /* MX25L3206E and KH25L3206E */                                                                \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x16, NORMAL_SPI_READ|DUAL_OUTPUT},                                \
    /* MX25L25635E */                                                                              \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x19, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},    \
    /* MX25L51245G */                                                                              \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x1A, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},    \
    /* GD25Q64B */                                                                                 \
    {SFUD_MF_ID_GIGADEVICE, 0x40, 0x17, NORMAL_SPI_READ|DUAL_OUTPUT},                              \
    /* NM25Q128EVB */                                                                              \
    {SFUD_MF_ID_NOR_MEM, 0x21, 0x18, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT|QUAD_IO},     \
    /* ZB25VQ128 */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x18, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT},                \
    /* ZB25VQ16C */                                                                                \
    {SFUD_MF_ID_ZBIT, 0x40, 0x15, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT},                \
    /* ZD25Q64B */                                                                                 \
    {SFUD_MF_ID_ZETTA, 0x32, 0x17, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT},               \
    /* EFM25F128A */                                                                               \
    {SFUD_MF_ID_FUDANMICRO, 0xBA, 0x18, NORMAL_SPI_READ|DUAL_OUTPUT|DUAL_IO|QUAD_OUTPUT},          \
}

/* those flash SFDP basic_len < 15, and the QE at SR1-BIT6, or specially SFDP (ZB25VQ16C QE info at Dword-14), the sfud can not recognition */
#define SFUD_FLASH_QE_INFO_TABLE                                                                                                  \
{                                                                                                                                 \
    /* MX25L6433FM2I */                                                                                                           \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x17, SFUD_CMD_WRITE_STATUS_REGISTER, SFUD_CMD_READ_STATUS_REGISTER, 6},                          \
    /* MX25L12835FM2I */                                                                                                          \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x18, SFUD_CMD_WRITE_STATUS_REGISTER, SFUD_CMD_READ_STATUS_REGISTER, 6},                          \
    /* MX25L25635E */                                                                                                             \
    {SFUD_MF_ID_MACRONIX, 0x20, 0x19, SFUD_CMD_WRITE_STATUS_REGISTER, SFUD_CMD_READ_STATUS_REGISTER, 6},                          \
    /* ZB25VQ16C */                                                                                                               \
    {SFUD_MF_ID_ZBIT, 0x40, 0x15, SFUD_CMD_WRITE_STATUS2_REGISTER, SFUD_CMD_READ_CONFIG_REGISTER, 1},                             \
    /* EFM25F128A */                                                                                                              \
    {SFUD_MF_ID_FUDANMICRO, 0xBA, 0x18, SFUD_CMD_WRITE_STATUS2_REGISTER, SFUD_CMD_READ_CONFIG_REGISTER, 1},                       \
}

#endif /* SFUD_USING_QSPI */

#ifdef __cplusplus
}
#endif

#endif /* BSP_PERIPHERAL_SPINOR_SFUD_INC_SFUD_FLASH_DEF_H_ */
