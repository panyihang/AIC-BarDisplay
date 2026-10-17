/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _AIC_MMC_H_
#define _AIC_MMC_H_

#include <aic_core.h>
#include <hal_sdmc.h>
#include <aic_partition.h>

struct aic_sdmc_pdata {
    ulong base;
    int irq;
    int clk;
    int clk_freq;
    u32 is_sdio;
    u8 id;
    u8 buswidth;
    u8 drv_phase;
    u8 smp_phase;
};

struct aic_sdmc_dev {
    void *priv;
    u32 voltages;
    u32 version;
    u32 freq_min;
    u32 freq_max;
    u32 high_capacity;
    u32 bus_width;
    u32 clock;
    u32 card_caps;
    u32 card_capacity; /* unit: KB*/
    u32 host_caps;
    u32 valid_ocr;
    u32 scr[2];
    u32 rca;
    u32 part_config;
    u32 boot_bus_cond;
    u32 part_num;
    u32 read_bl_len;
    u32 erase_grp_size; /* in 512-byte sectors */
    u32 blk_max;
    u32 sdmc_id;
    u32 max_seg_size;
    u32 max_dma_segs;
    u32 max_blk_size;
    u32 max_blk_count;
    u32 flags;
};

struct aic_sdmc_cmd {
    u32 cmd_code;
    u32 resp_type;
    u32 arg;
    u32 resp[4];
    u32 flags;
    u32 auto_stop_flag;
    int err;
};

struct aic_sdmc_data {
    u8 *buf;
    u32 flags;
    u32 blks;
    u32 blksize;
    int err;
};

/**
 * struct aic_sdmc - Information about a ArtInChip SDMC host
 *
 * @quirks:     Quick flags - see SDMC_QUIRK_...
 * @caps:       Capabilities - see MMC_MODE_...
 * @sclk_rate:  The rate of SDMC clk in Hz. It's the basis clk for SDMC.
 * @div:        Clock divider value for use by controller
 * @buswidth:   Bus width in bits (8 or 4)
 */
struct aic_sdmc {
    struct aic_sdmc_dev *dev;
    struct aic_sdmc_cmd *cmd;
    struct aic_sdmc_data *data;
    struct aic_sdmc_host host;

    u32 *buf;
    u32 clk;
    u32 irq;
    u32 index;
    u32 cid[4];

    unsigned int quirks;
    unsigned int caps;
    unsigned int version;
    unsigned int clock;
    unsigned int sclk_rate;
    unsigned int div;
    int ddr_mode;

    /* use fifo mode to read and write data */
    int fifo_mode;

    struct aic_sdmc_pdata *pdata;
};

/* Maximum block size for MMC */
#define MMC_MAX_BLOCK_LEN   512

/*
 * The number of MMC physical partitions.  These consist of:
 * boot partitions (2), general purpose partitions (4) in MMC v4.4.
 */
#define MMC_NUM_BOOT_PARTITION 2
#define MMC_PART_RPMB          3 /* RPMB partition number */

#define SD_VERSION_SD       0x20000
#define SD_VERSION_2        (SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0      (SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10     (SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC     0x10000
#define MMC_VERSION_UNKNOWN (MMC_VERSION_MMC)
#define MMC_VERSION_1_2     (MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4     (MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2     (MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3       (MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4       (MMC_VERSION_MMC | 0x40)
#define MMC_VERSION_4_1     (MMC_VERSION_MMC | 0x41)
#define MMC_VERSION_4_2     (MMC_VERSION_MMC | 0x42)
#define MMC_VERSION_4_3     (MMC_VERSION_MMC | 0x43)
#define MMC_VERSION_4_41    (MMC_VERSION_MMC | 0x44)
#define MMC_VERSION_4_5     (MMC_VERSION_MMC | 0x45)
#define MMC_VERSION_5_0     (MMC_VERSION_MMC | 0x50)
#define MMC_VERSION_5_1     (MMC_VERSION_MMC | 0x51)

#define MMC_MODE_HS       0x001
#define MMC_MODE_HS_52MHz 0x010
#define MMC_MODE_4BIT     0x100
#define MMC_MODE_8BIT     0x200
#define MMC_MODE_SPI      0x400
#define MMC_MODE_HC       0x800

#define SD_DATA_4BIT 0x00040000

#define IS_SD(x) (x->dev->version & SD_VERSION_SD)

#define MMC_CMD_BOOT       1
#define MMC_CMD_BOOT_ABORT 2
#define MMC_AUTO_STOP      1

#define MMC_DATA_READ  1
#define MMC_DATA_WRITE 2

#define NO_CARD_ERR  -16 /* No SD/MMC card inserted */
#define UNUSABLE_ERR -17 /* Unusable Card */
#define COMM_ERR     -18 /* Communications Error */
#define TIMEOUT      -19

#define MMC_CMD_GO_IDLE_STATE        0
#define MMC_CMD_SEND_OP_COND         1
#define MMC_CMD_ALL_SEND_CID         2
#define MMC_CMD_SET_RELATIVE_ADDR    3
#define MMC_CMD_SET_DSR              4
#define MMC_CMD_SWITCH               6
#define MMC_CMD_SELECT_CARD          7
#define MMC_CMD_SEND_EXT_CSD         8
#define MMC_CMD_SEND_CSD             9
#define MMC_CMD_SEND_CID             10
#define MMC_CMD_STOP_TRANSMISSION    12
#define MMC_CMD_SEND_STATUS          13
#define MMC_CMD_SET_BLOCKLEN         16
#define MMC_CMD_READ_SINGLE_BLOCK    17
#define MMC_CMD_READ_MULTIPLE_BLOCK  18
#define MMC_CMD_SET_BLOCK_COUNT      23
#define MMC_CMD_WRITE_SINGLE_BLOCK   24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK 25
#define MMC_CMD_ERASE_GROUP_START    35
#define MMC_CMD_ERASE_GROUP_END      36
#define MMC_CMD_ERASE                38
#define MMC_CMD_APP_CMD              55
#define MMC_CMD_SPI_READ_OCR         58
#define MMC_CMD_SPI_CRC_ON_OFF       59

#define SD_CMD_SEND_RELATIVE_ADDR 3
#define SD_CMD_SWITCH_FUNC        6
#define SD_CMD_SEND_IF_COND       8

#define SD_CMD_APP_SET_BUS_WIDTH  6
#define SD_CMD_ERASE_WR_BLK_START 32
#define SD_CMD_ERASE_WR_BLK_END   33
#define SD_CMD_APP_SEND_OP_COND   41
#define SD_CMD_APP_SEND_SCR       51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY      0x00020000
#define SD_HIGHSPEED_SUPPORTED 0x00020000

#define MMC_HS_TIMING 0x00000100
#define MMC_HS_52MHZ  0x2

#define OCR_BUSY         0x80000000
#define OCR_HCS          0x40000000
#define OCR_VOLTAGE_MASK 0x00FFFF80
#define OCR_ACCESS_MODE  0x60000000

#define MMC_ERASE_ARG    0x00000000
#define MMC_SECURE_ERASE 0x80000000

#define MMC_STATUS_MASK         (~0x0206BF7F)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)
#define MMC_STATUS_CURR_STATE   (0xf << 9)
#define MMC_STATUS_ERROR        (1 << 19)

#define MMC_VDD_165_195 0x00000080 /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21   0x00000100 /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22   0x00000200 /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23   0x00000400 /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24   0x00000800 /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25   0x00001000 /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26   0x00002000 /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27   0x00004000 /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28   0x00008000 /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29   0x00010000 /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30   0x00020000 /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31   0x00040000 /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32   0x00080000 /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33   0x00100000 /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34   0x00200000 /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35   0x00400000 /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36   0x00800000 /* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET    0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS   0x01 /* Set bits in EXT_CSD byte addressed by index which are 1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS 0x02 /* Clear bits in EXT_CSD byte addressed by index, which are 1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE 0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK  0
#define SD_SWITCH_SWITCH 1

#define SD_BUS_WIDTH_1 0 /* Card is in 1 bit mode */
#define SD_BUS_WIDTH_4 2 /* Card is in 4 bit mode */

/*
 * EXT_CSD fields
 */
#define EXT_CSD_PARTITION_SUPPORT  160 /* RO */
#define EXT_CSD_BOOT_BUS_COND      177 /* R/W */
#define EXT_CSD_PART_CONF          179 /* R/W */
#define EXT_CSD_BUS_WIDTH          183 /* R/W */
#define EXT_CSD_HS_TIMING          185 /* R/W */
#define EXT_CSD_CARD_TYPE          196 /* RO */
#define EXT_CSD_REV                192 /* RO */
#define EXT_CSD_SEC_CNT            212 /* RO, 4 bytes */

/*
 * EXT_CSD field definitions
 */
#define EXT_CSD_CMD_SET_NORMAL   (1 << 0)
#define EXT_CSD_CMD_SET_SECURE   (1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE (1 << 2)

#define EXT_CSD_CARD_TYPE_26 (1 << 0) /* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52 (1 << 1) /* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1         0        /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4         (1 << 0) /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8         (1 << 1) /* Card is in 8 bit mode */
#define EXT_CSD_MUTBLKWRITE         (1 << 2)
#define EXT_CSD_SUP_SDIO_IRQ        (1 << 4) /* support signal pending SDIO IRQs */
#define EXT_CSD_SUP_HIGHSPEED       (1 << 5) /* support high speed */
#define EXT_CSD_SUP_HIGHSPEED_DDR   (1 << 6) /* support high speed(DDR) */
#define EXT_CSD_SUP_HIGHSPEED_HS200 (1 << 7) /* support high speed HS200 */
#define EXT_CSD_SUP_HIGHSPEED_HS400 (1 << 8) /* support high speed HS400 */


#define R1_ILLEGAL_COMMAND (1 << 22)
#define R1_APP_CMD         (1 << 5)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136     (1 << 1) /* 136 bit response */
#define MMC_RSP_CRC     (1 << 2) /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3) /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4) /* response contains opcode */

#define MMC_RSP_MASK (0xF)
#define MMC_RSP_NONE (0)
#define MMC_RSP_R1   (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)
#define MMC_RSP_R1b  (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE | MMC_RSP_BUSY)
#define MMC_RSP_R2   (MMC_RSP_PRESENT | MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RSP_R3   (MMC_RSP_PRESENT)
#define MMC_RSP_R4   (MMC_RSP_PRESENT)
#define MMC_RSP_R5   (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)
#define MMC_RSP_R6   (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)
#define MMC_RSP_R7   (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)

#define MMC_CMD_MASK (3 << 4)
#define MMC_CMD_AC   (0 << 4)
#define MMC_CMD_ADTC (1 << 4)
#define MMC_CMD_BC   (2 << 4)
#define MMC_CMD_BCR  (3 << 4)

#define MMCPART_NOAVAILABLE (0xff)
#define PART_ACCESS_MASK    (0x7)
#define PART_SUPPORT        (0x1)
#define PART_BOOT_PART_MASK (0x7 << 3)
#define PART_BOOT_PART_NONE (0x0)
#define PART_BOOT_PART_1    (0x1)
#define PART_BOOT_PART_2    (0x2)
#define PART_BOOT_USER      (0x7)
#define PART_BOOT_ACK_MASK  (0x1 << 6)
#define PART_BOOT_ACK_ENB   (0x1)

#define MMCBOOT_BUS_NOAVAILABLE (0xff)
#define BOOT_MODE_MASK          (0x3 << 3)
#define BOOT_SDR_NORMAL         (0x0)
#define BOOT_SDR_HS             (0x1)
#define BOOT_DDR                (0x2)
#define BOOT_RST_BUS_COND_MASK  (0x1 << 2)
#define BOOT_RST_BUS_COND       (0x0)
#define BOOT_RETAIN_BUS_COND    (0x1)
#define BOOT_BUS_WIDTH_MASK     (0x3 << 0)
#define BOOT_BUS_SDRx1_DDRx4    (0x0)
#define BOOT_BUS_SDRx4_DDRx4    (0x1)
#define BOOT_BUS_SDRx8_DDRx8    (0x2)

#define MMC_TYPE_SD_CARD   (1)
#define MMC_TYPE_EMMC_UDA  (2)
#define MMC_TYPE_EMMC_BOOT (3)

#define MMC_BLOCK_SIZE   512

s32 mmc_init(int id);
s32 mmc_deinit(int id);
s32 mmc_switch_part(struct aic_sdmc *host, u32 part_num);
u32 mmc_bread(void *priv, u32 start, u32 blkcnt, u8 *dst);
u32 mmc_bwrite(struct aic_sdmc *host, u32 start, u32 blkcnt, const u8 *src);
u32 mmc_berase(struct aic_sdmc *host, u32 start, u32 blkcnt);
struct aic_sdmc *find_mmc_dev_by_index(int id);
struct aic_partition *mmc_new_partition(char *s, u64 start);
void mmc_free_partition(struct aic_partition *part);
struct aic_partition *mmc_create_gpt_part(void);
struct aic_partition *mmc_create_gpt_part2(int mmc_id);
void sdcard_hotplug_init(void);
void sdcard_hotplug_act(void);
int mmc_block_init(struct aic_sdmc *host);
int mmc_block_refresh(struct aic_sdmc *host);
int mmc_block_deinit(struct aic_sdmc *host);

void mmc_set_rx_phase(void *priv, u32 phase);
void mmc_set_rx_delay(void *priv, u32 delay);
void mmc_set_tx_phase(void *priv, u32 phase);
void mmc_set_tx_delay(void *priv, u32 delay);

int mmc_rpmb_get_counter(struct aic_sdmc *host, unsigned long *pcounter);
int mmc_rpmb_set_key(struct aic_sdmc *host, void *key);
int mmc_rpmb_read(struct aic_sdmc *host, void *addr, unsigned short blk,
		  unsigned short cnt, unsigned char *key);
int mmc_rpmb_write(struct aic_sdmc *host, void *addr, unsigned short blk,
		  unsigned short cnt, unsigned char *key);
int mmc_rpmb_route_frames(struct aic_sdmc *host, void *req, unsigned long reqlen,
			  void *rsp, unsigned long rsplen);


#endif /* _AIC_MMC_H_ */
