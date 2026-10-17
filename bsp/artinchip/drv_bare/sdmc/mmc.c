/*
 * Copyright (c) 2022-2026, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xiong Hao <hao.xiong@artinchip.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_common.h>
#include <aic_core.h>
#include <aic_soc.h>
#include <aic_hal.h>
#include <mmc.h>
#include "sdmc.h"

#define MAX_MMC_DEV_NUM  3
#define CONFIG_SYS_MMC_MAX_BLK_COUNT    300

static void *mmc_dev[MAX_MMC_DEV_NUM];
static struct aic_sdmc_pdata sdmc_pdata[] = {
#if defined(AIC_USING_SDMC0)
    {
        .id = 0,
        .base = SDMC0_BASE,
        .irq = SDMC0_IRQn,
        .clk = CLK_SDMC0,
#if defined(AIC_SDMC0_BUSWIDTH1)
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#if defined(AIC_SDMC0_BUSWIDTH4)
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#if defined(AIC_SDMC0_BUSWIDTH8)
        .buswidth = SDMC_CTYPE_8BIT,
#endif
        .drv_phase = AIC_SDMC0_DRV_PHASE,
        .smp_phase = AIC_SDMC0_SMP_PHASE,
        .clk_freq = AIC_SDMC0_CLK_FREQ,
    },
#endif
#if defined(AIC_USING_SDMC1)
    {
        .id = 1,
        .base = SDMC1_BASE,
        .irq = SDMC1_IRQn,
        .clk = CLK_SDMC1,
#if defined(AIC_SDMC1_BUSWIDTH1)
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#if defined(AIC_SDMC1_BUSWIDTH4)
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#if defined(AIC_SDMC1_BUSWIDTH8)
        .buswidth = SDMC_CTYPE_8BIT,
#endif
#if defined(AIC_SDMC1_IS_SDIO)
        .is_sdio = 1,
#endif
        .drv_phase = AIC_SDMC1_DRV_PHASE,
        .smp_phase = AIC_SDMC1_SMP_PHASE,
        .clk_freq = AIC_SDMC1_CLK_FREQ,
    },
#endif
#if defined(AIC_USING_SDMC2)
    {
        .id = 2,
        .base = SDMC2_BASE,
        .irq = SDMC2_IRQn,
        .clk = CLK_SDMC2,
#if defined(AIC_SDMC2_BUSWIDTH1)
        .buswidth = SDMC_CTYPE_1BIT,
#endif
#if defined(AIC_SDMC2_BUSWIDTH4)
        .buswidth = SDMC_CTYPE_4BIT,
#endif
#if defined(AIC_SDMC2_BUSWIDTH8)
        .buswidth = SDMC_CTYPE_8BIT,
#endif
#if defined(AIC_SDMC2_IS_SDIO)
        .is_sdio = 1,
#endif
        .drv_phase = AIC_SDMC2_DRV_PHASE,
        .smp_phase = AIC_SDMC2_SMP_PHASE,
        .clk_freq = AIC_SDMC2_CLK_FREQ,
    },
#endif
};


static void mmc_trace_before_send(struct aic_sdmc_cmd *cmd)
{
#ifdef SDMC_DUMP_CMD
    printf("CMD_SEND: %d\n", cmd->cmd_code);
    printf("\t\tARG\t\t\t 0x%08x\n", cmd->arg);
#endif
}

static void mmc_trace_after_send(struct aic_sdmc_cmd *cmd)
{
#ifdef SDMC_DUMP_CMD
    int i;
    u8 *ptr;

    switch (cmd->resp_type) {
    case MMC_RSP_NONE:
        printf("\t\tRSP_NONE\n\n");
        break;
    case MMC_RSP_R1:
        printf("\t\tRSP_R1,5,6,7 \t\t 0x%08x \n",
            cmd->resp[0]);
        break;
    case MMC_RSP_R1b:
        printf("\t\tRSP_R1B\t\t 0x%08x \n",
            cmd->resp[0]);
        break;
    case MMC_RSP_R2:
        printf("\t\tRSP_R2\t\t\t 0x%08x \n", cmd->resp[0]);
        printf("\t\t          \t\t 0x%08x \n", cmd->resp[1]);
        printf("\t\t          \t\t 0x%08x \n", cmd->resp[2]);
        printf("\t\t          \t\t 0x%08x \n", cmd->resp[3]);
        printf("\n");
        printf("\t\t\t\t\tDUMPING DATA\n\n");
        for (i = 0; i < 4; i++) {
            int j;
            printf("\t\t\t\t\t%03d - ", i*4);
            ptr = (u8 *)&cmd->resp[i];
            ptr += 3;
            for (j = 0; j < 4; j++)
                printf("%02x ", *ptr--);
            printf("\n");
        }
        break;
    case MMC_RSP_R3:
        printf("\t\tRSP_R3,4\t\t 0x%08x \n",
            cmd->resp[0]);
        break;
    default:
        printf("\t\tERROR MMCSD rsp not supported 0x%x\n", cmd->resp_type);
        break;
    }
#endif
}

int mmc_send_cmd(struct aic_sdmc *host, struct aic_sdmc_cmd *cmd,
                      struct aic_sdmc_data *data)
{
    memset(cmd->resp, 0, sizeof(cmd->resp));

    mmc_trace_before_send(cmd);
    aic_sdmc_request(host, cmd, data);
    mmc_trace_after_send(cmd);

    if (cmd->err || (data && data->err))
        hal_sdmc_soft_reset(&host->host);

    return cmd->err;
}

static int mmc_go_idle(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err;

    memset(&cmd, 0, sizeof(struct aic_sdmc_cmd));

    cmd.cmd_code = MMC_CMD_GO_IDLE_STATE;
    cmd.resp_type = MMC_RSP_NONE;
    cmd.arg = 0;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    aicos_mdelay(1);

    return 0;
}

static int sd_send_op_cond(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err, timeout = 1000;

    do {
        cmd.cmd_code = MMC_CMD_APP_CMD;
        cmd.resp_type = MMC_RSP_R1;
        cmd.arg = 0;
        cmd.flags = 0;

        err = mmc_send_cmd(host, &cmd, NULL);
        if (err)
            return err;

        cmd.cmd_code = SD_CMD_APP_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;

        /*
         * Most cards do not answer if some reserved bits
         * in the ocr are set. However, Some controller
         * can set bit 7 (reserved for low voltages), but
         * how to manage low voltages SD card is not yet
         * specified.
         */
        cmd.arg = (host->dev->voltages & 0xff8000);
        if (host->dev->version == SD_VERSION_2)
            cmd.arg |= OCR_HCS;

        err = mmc_send_cmd(host, &cmd, NULL);
        if (err)
            return err;

        aicos_mdelay(1);
    } while ((!(cmd.resp[0] & OCR_BUSY)) && timeout--);

    if (timeout <= 0)
        return -1;

    if (host->dev->version != SD_VERSION_2)
        host->dev->version = SD_VERSION_1_0;

    host->dev->valid_ocr = cmd.resp[0];

    host->dev->high_capacity = ((host->dev->valid_ocr & OCR_HCS) == OCR_HCS);
    host->dev->rca = 0;

    return 0;
}

static int mmc_send_op_cond(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err, timeout = 1000;

    /* Some cards seem to need this */
    mmc_go_idle(host);

    /* CMD1: Asking to the card its capabilities */
    cmd.cmd_code = MMC_CMD_SEND_OP_COND;
    cmd.resp_type = MMC_RSP_R3;
    cmd.arg = 0;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    aicos_mdelay(1);

    do {
        cmd.cmd_code = MMC_CMD_SEND_OP_COND;
        cmd.resp_type = MMC_RSP_R3;
        cmd.arg = ((host->dev->voltages & (cmd.resp[0] & OCR_VOLTAGE_MASK)) |
                   (cmd.resp[0] & OCR_ACCESS_MODE));

        if (host->dev->host_caps & MMC_MODE_HC)
            cmd.arg |= OCR_HCS;

        cmd.flags = 0;

        err = mmc_send_cmd(host, &cmd, NULL);
        if (err)
            return err;

        aicos_mdelay(1);
    } while ((!(cmd.resp[0] & OCR_BUSY)) && timeout--);

    if (timeout <= 0)
        return -1;

    host->dev->version = MMC_VERSION_UNKNOWN;
    host->dev->valid_ocr = cmd.resp[0];
    host->dev->high_capacity = ((host->dev->valid_ocr & OCR_HCS) == OCR_HCS);
    host->dev->rca = 1;

    return 0;
}

static int mmc_send_ext_csd(struct aic_sdmc *host, u8 *ext_csd)
{
    struct aic_sdmc_cmd cmd = {0};
    struct aic_sdmc_data data = {0};
    int err;

    cmd.cmd_code = MMC_CMD_SEND_EXT_CSD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.arg = 0;
    cmd.flags = 0;

    data.buf = ext_csd;
    data.blks = 1;
    data.blksize = 512;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd(host, &cmd, &data);

    return err;
}

static int mmc_switch(struct aic_sdmc *host, u8 set, u8 index, u8 value)
{
    struct aic_sdmc_cmd cmd = {0};
    int err;

    cmd.cmd_code = MMC_CMD_SWITCH;
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
    (index << 16) | (value << 8) | set;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
      return err;

    return 0;
}

static int mmc_select_bus_width(struct aic_sdmc *host)
{
    int err = 0;
    u32 ext_csd_bus_width;

    /* buswidth 1 not need to switch */
    if (host->pdata->buswidth == SDMC_CTYPE_1BIT)
        return 0;
    else if (host->dev->card_caps & MMC_MODE_4BIT &&
                host->pdata->buswidth == SDMC_CTYPE_4BIT)
        /* Set the card to use 4 bit*/
        ext_csd_bus_width = EXT_CSD_BUS_WIDTH_4;
    else if (host->dev->card_caps & MMC_MODE_8BIT &&
               host->pdata->buswidth == SDMC_CTYPE_8BIT)
        /* Set the card to use 8 bit*/
        ext_csd_bus_width = EXT_CSD_BUS_WIDTH_8;
    else
        /* Default use 1 bit */
        ext_csd_bus_width = EXT_CSD_BUS_WIDTH_1;

    /* card configuration */
    err = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL,
                     EXT_CSD_BUS_WIDTH,
                     ext_csd_bus_width);
    if (err)
      return err;

    /* host configuration */
    aic_sdmc_set_cfg(host);

    return 0;
}

static int mmc_get_capabilities(struct aic_sdmc *host, u8 *ext_csd)
{
    char cardtype = ext_csd[EXT_CSD_CARD_TYPE];

    host->dev->card_caps = 0;

    /* Only version 4 supports high-speed */
    if (host->dev->version < MMC_VERSION_4)
        return 0;

    /* Default is 4-line mode */
    host->dev->card_caps |= MMC_MODE_4BIT | MMC_MODE_HC;

    if (cardtype & EXT_CSD_CARD_TYPE_26)
        host->dev->card_caps |= MMC_MODE_HS;

    if (cardtype & EXT_CSD_CARD_TYPE_52)
        host->dev->card_caps |= MMC_MODE_HS_52MHz;

    return 0;
}

static int sd_switch(struct aic_sdmc *host, int mode, int group, u8 value, u8 *resp)
{
    struct aic_sdmc_cmd cmd = {0};
    struct aic_sdmc_data data = {0};

    /* Switch the frequency */
    cmd.cmd_code = SD_CMD_SWITCH_FUNC;
    cmd.resp_type = MMC_RSP_R1;
    cmd.arg = (mode << 31) | 0xffffff;
    cmd.arg &= ~(0xf << (group * 4));
    cmd.arg |= value << (group * 4);

    data.buf = resp;
    data.blksize = 64;
    data.blks = 1;
    data.flags = MMC_DATA_READ;

    return mmc_send_cmd(host, &cmd, &data);
}

static int sd_get_capabilities(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    struct aic_sdmc_data data = {0};
    ALLOC_CACHE_ALIGN_BUFFER(u32, scr, 16);
    ALLOC_CACHE_ALIGN_BUFFER(u32, switch_status, 16);
    int err, timeout;

    host->dev->card_caps = 0;

    /* Read the SCR to find out if this card supports higher speeds */
    cmd.cmd_code = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.arg = host->dev->rca << 16;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    cmd.cmd_code = SD_CMD_APP_SEND_SCR;
    cmd.resp_type = MMC_RSP_R1;
    cmd.arg = 0;
    cmd.flags = 0;

    timeout = 3;

retry_scr:
    data.buf = (u8 *)scr;
    data.blksize = 8;
    data.blks = 1;
    data.flags = MMC_DATA_READ;

    err = mmc_send_cmd(host, &cmd, &data);
    if (err) {
        if (timeout--)
            goto retry_scr;

        return err;
    }

    host->dev->scr[0] = be32_to_cpu(scr[0]);
    host->dev->scr[1] = be32_to_cpu(scr[1]);

    switch ((host->dev->scr[0] >> 24) & 0xf) {
    case 0:
        host->dev->version = SD_VERSION_1_0;
        pr_debug("SD Ver 1.0\n");
        break;
    case 1:
        host->dev->version = SD_VERSION_1_10;
        pr_debug("SD Ver 1.1\n");
        break;
    case 2:
        host->dev->version = SD_VERSION_2;
        pr_debug("SD Ver 2.0\n");
        break;
    default:
        host->dev->version = SD_VERSION_1_0;
        pr_debug("SD Ver 1.0\n");
        break;
    }

    if (host->dev->version < MMC_VERSION_4)
        return 0;

    /* Default is 4-line mode */
    if (host->dev->scr[0] & SD_DATA_4BIT) {
        host->dev->card_caps |= MMC_MODE_4BIT;
        pr_debug("Card caps 4 bit\n");
    }

    /* Version 1.0 doesn't support switching */
    if (host->dev->version == SD_VERSION_1_0)
        return 0;

    timeout = 4;
    while (timeout--) {
        err = sd_switch(host, SD_SWITCH_CHECK, 0, 1, (u8 *)switch_status);
        if (err)
            return err;

        /* The high-speed function is busy. Try again */
        if (!(be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
            break;
    }

    /* If high-speed isn't supported, we return */
    if (be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED)
        host->dev->card_caps |= MMC_MODE_HS;

    return 0;
}

static int sd_set_card_speed(struct aic_sdmc *host, u32 hs)
{
    int err;
    ALLOC_CACHE_ALIGN_BUFFER(uint, switch_status, 16);

    /* SD version 1.00 and 1.01 does not support CMD 6 */
    if (host->dev->version == SD_VERSION_1_0)
        return 0;

    err = sd_switch(host, SD_SWITCH_SWITCH, 0, hs, (u8 *)switch_status);
    if (err)
        return err;

    if (((be32_to_cpu(switch_status[4]) >> 24) & 0xF) != hs)
        return -1;

    return 0;
}

static int sd_select_bus_width(struct aic_sdmc *host, int w)
{
    int err;
    struct aic_sdmc_cmd cmd = {0};

    if ((w != 4) && (w != 1))
        return -EINVAL;

    cmd.cmd_code = MMC_CMD_APP_CMD;
    cmd.resp_type = MMC_RSP_R1;
    cmd.arg = host->dev->rca << 16;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    cmd.cmd_code = SD_CMD_APP_SET_BUS_WIDTH;
    cmd.resp_type = MMC_RSP_R1;
    if (w == 4)
        cmd.arg = 2;
    else if (w == 1)
        cmd.arg = 0;
    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    return 0;
}

static void mmc_update_version(struct aic_sdmc *host, int version)
{
    switch (version) {
    case 0:
        host->dev->version = MMC_VERSION_1_2;
        break;
    case 1:
        host->dev->version = MMC_VERSION_1_4;
        break;
    case 2:
        host->dev->version = MMC_VERSION_2_2;
        break;
    case 3:
        host->dev->version = MMC_VERSION_3;
        break;
    case 4:
        host->dev->version = MMC_VERSION_4;
        break;
    default:
        host->dev->version = MMC_VERSION_1_2;
        break;
    }
}

static int mmc_get_csd(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int erase_gsz, erase_gmul;
    int err = 0;
    u32 csize = 0, cmult = 0;
    u64 capacity;

    /* CMD9 - Get the Card-Specific Data */
    cmd.cmd_code = MMC_CMD_SEND_CSD;
    cmd.resp_type = MMC_RSP_R2;
    cmd.arg = host->dev->rca << 16;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        return err;

    if (host->dev->version == MMC_VERSION_UNKNOWN)
        mmc_update_version(host, (cmd.resp[0] >> 26) & 0xf);

    host->dev->read_bl_len = 1 << ((cmd.resp[1] >> 16) & 0xf);
    if (host->dev->high_capacity) {
        csize = (cmd.resp[1] & 0x3f) << 16 | (cmd.resp[2] & 0xffff0000) >> 16;
        cmult = 8;
    } else {
        csize = (cmd.resp[1] & 0x3ff) << 2 | (cmd.resp[2] & 0xc0000000) >> 30;
        cmult = (cmd.resp[2] & 0x00038000) >> 15;
    }

    /* Calculate the group size from the csd value. */
    erase_gsz = (cmd.resp[2] & 0x00007c00) >> 10;
    erase_gmul = (cmd.resp[2] & 0x000003e0) >> 5;
    host->dev->erase_grp_size = (erase_gsz + 1) * (erase_gmul + 1);

    capacity = (csize + 1) << (cmult + 2);
    capacity *= host->dev->read_bl_len;
    host->dev->card_capacity = (u32)(capacity >> 10); /* in KB */

    if (host->dev->read_bl_len > MMC_MAX_BLOCK_LEN)
        host->dev->read_bl_len = MMC_MAX_BLOCK_LEN;

    return 0;
}

static int mmc_go_transfer_mode(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err = 0;

    /* CMD7 - Select the card, and put it into Transfer Mode */
    cmd.cmd_code = MMC_CMD_SELECT_CARD;
    cmd.resp_type = MMC_RSP_R1b;
    cmd.arg = host->dev->rca << 16;

    err = mmc_send_cmd(host, &cmd, NULL);

    return err;
}

static int sd_startup(struct aic_sdmc *host)
{
    s32 err;

    err = sd_get_capabilities(host);
    if (err != 0)
        return err;

    if (host->dev->card_caps & MMC_MODE_4BIT &&
        host->pdata->buswidth == SDMC_CTYPE_4BIT) {
        sd_select_bus_width(host, 4);
        aic_sdmc_set_cfg(host);
    } else {
        host->pdata->buswidth = SDMC_CTYPE_1BIT;
        sd_select_bus_width(host, 1);
        aic_sdmc_set_cfg(host);
    }

    if (host->dev->card_caps & MMC_MODE_HS) {
        sd_set_card_speed(host, 1);
        host->dev->clock = 50000000;
    } else {
        sd_set_card_speed(host, 0);
        host->dev->clock = 25000000;
    }
    aic_sdmc_set_cfg(host);

    return 0;
}

static int emmc_startup(struct aic_sdmc *host)
{
    s32 err;
    u8 ext_csd[512] = {0};

    if (host->dev->version >= MMC_VERSION_4) {
        /* check  ext_csd version and capacity */
        err = mmc_send_ext_csd(host, ext_csd);
        /* update mmcdev version */
        switch (ext_csd[EXT_CSD_REV]) {
        case 0:
            host->dev->version = MMC_VERSION_4;
            break;
        case 1:
            host->dev->version = MMC_VERSION_4_1;
            break;
        case 2:
            host->dev->version = MMC_VERSION_4_2;
            break;
        case 3:
            host->dev->version = MMC_VERSION_4_3;
            break;
        case 5:
            host->dev->version = MMC_VERSION_4_41;
            break;
        case 6:
            host->dev->version = MMC_VERSION_4_5;
            break;
        case 7:
            host->dev->version = MMC_VERSION_5_0;
            break;
        case 8:
            host->dev->version = MMC_VERSION_5_1;
            break;
        }

        /* Check Read only ext_csd[160] whether support partition.  */
        if (ext_csd[EXT_CSD_PARTITION_SUPPORT] & PART_SUPPORT)
            host->dev->part_config = ext_csd[EXT_CSD_PART_CONF];
    }

    if (host->dev->version >= MMC_VERSION_4_2) {
        /*
		 * According to the JEDEC Standard, the value of
		 * ext_csd's capacity is valid if the value is more
		 * than 2GB
		 */
        u64 capacity;
        capacity = ext_csd[EXT_CSD_SEC_CNT] << 0 |
                   ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
                   ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
                   ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
        capacity *= host->dev->read_bl_len;
        if ((capacity >> 20) > 2 * 1024)
            capacity = capacity;
        host->dev->card_capacity = (u32)(capacity >> 10); /* in KB */
    }

    err = mmc_get_capabilities(host, ext_csd);
    if (err != 0)
        return err;

    /* Restrict card's capabilities by what the host can do */
    host->dev->card_caps &= host->dev->host_caps;

    /* Select bus width */
    err = mmc_select_bus_width(host);
    if (err != 0)
        return err;

    /* Select clock */
    if (host->dev->card_caps & MMC_MODE_HS) {
        if (host->dev->card_caps & MMC_MODE_HS_52MHz)
            host->dev->clock = 52000000;
        else
            host->dev->clock = 26000000;
    } else {
        /* MMC Legacy */
        host->dev->clock = 25000000;
    }

    aic_sdmc_set_cfg(host);

    return 0;
}

static int mmc_go_identify_mode(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err = 0;

    /* CMD2 - Put the Card in Identify Mode */
    cmd.cmd_code = MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
    cmd.resp_type = MMC_RSP_R2;
    cmd.arg = 0;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);

    memcpy(host->cid, cmd.resp, 16);

    return err;
}

static s32 mmc_relative_addr(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err = 0;

    /*
     * CMD3 - For MMC cards, set the Relative Address.
     * For SD cards, get the Relative Address.
     * This also puts the cards into Standby State
     */
    cmd.cmd_code = SD_CMD_SEND_RELATIVE_ADDR; /* cmd not supported in spi */
    cmd.resp_type = MMC_RSP_R6;
    cmd.arg = host->dev->rca << 16;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);

    if (IS_SD(host))
        host->dev->rca = (cmd.resp[0] >> 16) & 0xffff;

    return err;
}

static int mmc_startup(struct aic_sdmc *host)
{
    int err = 0;

    /* Get Card-Specific-Data and update parameters */
    err = mmc_get_csd(host);
    if (err)
        return err;

    err = mmc_go_transfer_mode(host);
    if (err)
        return err;

    host->dev->part_config = MMCPART_NOAVAILABLE;
    host->dev->boot_bus_cond = MMCPART_NOAVAILABLE;

    if (IS_SD(host))
        err = sd_startup(host);
    else
        err = emmc_startup(host);
    if (err)
        return err;

    return 0;
}

static int mmc_send_if_cond(struct aic_sdmc *host)
{
    struct aic_sdmc_cmd cmd = {0};
    int err;

    cmd.cmd_code = SD_CMD_SEND_IF_COND;
    /* We set the bit if the host supports voltages between 2.7 and 3.6 V */
    cmd.arg = ((host->dev->voltages & 0xff8000) != 0) << 8 | 0xaa;
    cmd.resp_type = MMC_RSP_R7;
    cmd.flags = 0;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err) {
        pr_err("snd cmd failed.\n");
        return err;
    }

    /*
     * 0xaa is 8bit "check pattern", if the card is SD v2.0 or later, it will
     * echo the check pattern value.
     */
    if ((cmd.resp[0] & 0xff) != 0xaa) {
        pr_err("resp failed(%d).\n", cmd.resp[0]);
        return -1;
    }

    host->dev->version = SD_VERSION_2;

    return 0;
}

u32 mmc_read_blocks(struct aic_sdmc *host, void *dst, u32 start, u32 blkcnt)
{
    struct aic_sdmc_cmd cmd = {0};
    struct aic_sdmc_data data = {0};
    int err;

    memset(&cmd, 0, sizeof(struct aic_sdmc_cmd));
    memset(&data, 0, sizeof(struct aic_sdmc_data));
    /* Change for send stop cmd manually */
    if (blkcnt > 1) {
        cmd.cmd_code = MMC_CMD_READ_MULTIPLE_BLOCK;
        cmd.auto_stop_flag = 0;
    } else {
        cmd.cmd_code = MMC_CMD_READ_SINGLE_BLOCK;
        cmd.auto_stop_flag = 0;
    }

    if (host->dev->high_capacity)
        cmd.arg = start;
    else
        cmd.arg = start * host->dev->read_bl_len;

    cmd.resp_type = MMC_RSP_R1;
    cmd.flags = 0;
    data.buf = dst;
    data.blks = blkcnt;
    data.blksize = host->dev->read_bl_len;
    data.flags = MMC_DATA_READ;

    /*
     * Change for send stop cmd manually conditions
     * Cmd 18 should be couple used with cmd 12
     * 1.Response error(CRC error;timeout);
     * 2.Data timeout;
     * 3.Data error;
     * 4.Transfer complete normally
     */
    err = mmc_send_cmd(host, &cmd, &data);
    if (err) {
        pr_err("Send cmd %d error = %d\n", cmd.cmd_code, err);
        return 0;
    }

    if ((cmd.cmd_code == MMC_CMD_READ_MULTIPLE_BLOCK) &&
        !(cmd.auto_stop_flag)) {
        cmd.cmd_code = MMC_CMD_STOP_TRANSMISSION;
        cmd.resp_type = MMC_RSP_R1b;
        cmd.arg = 0;
        cmd.flags = 0;
        if (mmc_send_cmd(host, &cmd, NULL)) {
            pr_err("Failed to stop multi-block read. err -%d\n", cmd.err);
            return 0;
        }
    }

    if (cmd.err || data.err) {
        printf("read blocks failed, cmd.err:%d, data.err:%d, data.flags:0x%08x, data.blksize:0x%08x\n",
               cmd.err, data.err, data.flags, data.blksize);
        return 0;
    }

    return blkcnt;
}

void mmc_set_rx_phase(void *priv, u32 phase)
{
    struct aic_sdmc *host = (struct aic_sdmc *)priv;
    hal_sdmc_set_phase(&host->host, host->pdata->drv_phase, phase);
}

void mmc_set_rx_delay(void *priv, u32 delay)
{
    struct aic_sdmc *host = (struct aic_sdmc *)priv;
    hal_sdmc_set_delay(&host->host, 0, delay);
}

void mmc_set_tx_phase(void *priv, u32 phase)
{
    struct aic_sdmc *host = (struct aic_sdmc *)priv;
    hal_sdmc_set_phase(&host->host, phase, host->pdata->smp_phase);
}

void mmc_set_tx_delay(void *priv, u32 delay)
{
    struct aic_sdmc *host = (struct aic_sdmc *)priv;
    hal_sdmc_set_delay(&host->host, delay, 0);
}

u32 mmc_bread(void *priv, u32 start, u32 blkcnt, u8 *dst)
{
    struct aic_sdmc *host = (struct aic_sdmc *)priv;
    u32 cur, blk_todo = blkcnt;

    if (blkcnt == 0)
        return 0;

    do {
        if (blk_todo > host->dev->blk_max)
            cur = host->dev->blk_max;
        else
            cur = blk_todo;

        if (mmc_read_blocks(host, dst, start, cur) != cur)
            return 0;

        blk_todo -= cur;
        start += cur;
        dst += cur * host->dev->read_bl_len;
    } while (blk_todo > 0);

    return blkcnt;
}

u32 mmc_write_blocks(struct aic_sdmc *host, const u8 *src, u32 start, u32 blkcnt)
{
    struct aic_sdmc_cmd cmd = {0};
    struct aic_sdmc_data data = {0};

    if (blkcnt > 1)
        cmd.cmd_code = MMC_CMD_WRITE_MULTIPLE_BLOCK;
    else
        cmd.cmd_code = MMC_CMD_WRITE_SINGLE_BLOCK;

    if (host->dev->high_capacity)
        cmd.arg = start;
    else
        cmd.arg = start * host->dev->read_bl_len;

    cmd.resp_type = MMC_RSP_R1;
    cmd.flags = 0;

    data.buf = (u8 *)src;
    data.blks = blkcnt;
    data.blksize = host->dev->read_bl_len;
    data.flags = MMC_DATA_WRITE;

    if (mmc_send_cmd(host, &cmd, &data)) {
        pr_err("write blocks failed, error = -%d\n", -cmd.err);
        return 0;
    }

    if (cmd.cmd_code == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
        cmd.cmd_code = MMC_CMD_STOP_TRANSMISSION;
        cmd.resp_type = MMC_RSP_R1b;
        cmd.arg = 0;
        cmd.flags = 0;
        if (mmc_send_cmd(host, &cmd, NULL)) {
            pr_err("Failed to stop mulit-block write. err -%d\n", -cmd.err);
            return 0;
        }
    }

    if (cmd.err || data.err) {
        printf("write blocks failed, cmd.err:%d, data.err:%d, data.flags:0x%08x, data.blksize:0x%08x\n",
               cmd.err, data.err, data.flags, data.blksize);
        return 0;
    }

    return blkcnt;
}

u32 mmc_bwrite(struct aic_sdmc *host, u32 start, u32 blkcnt, const u8 *src)
{
    u32 cur, blocks_todo = blkcnt;

    if (blkcnt == 0)
        return 0;

    do {
        if (blocks_todo > host->dev->blk_max)
            cur = host->dev->blk_max;
        else
            cur = blocks_todo;

        if (mmc_write_blocks(host, src, start, cur) != cur)
            return 0;
        blocks_todo -= cur;
        start += cur;
        src += cur * host->dev->read_bl_len;
    } while (blocks_todo > 0);

    return blkcnt;
}

u32 mmc_erase_t(struct aic_sdmc *host, u32 start, u32 blkcnt)
{
    struct aic_sdmc_cmd cmd = {0};
    u32 end;
    int err;

    if (host->dev->high_capacity) {
        end = start + blkcnt - 1;
    } else {
        end = (start + blkcnt -1) * host->dev->read_bl_len;
        start = start * host->dev->read_bl_len;
    }

    if (IS_SD(host))
        cmd.cmd_code = SD_CMD_ERASE_WR_BLK_START;
    else
        cmd.cmd_code = MMC_CMD_ERASE_GROUP_START;
    cmd.arg = start;
    cmd.resp_type = MMC_RSP_R1;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        goto err_out;

    if (IS_SD(host))
        cmd.cmd_code = SD_CMD_ERASE_WR_BLK_END;
    else
        cmd.cmd_code = MMC_CMD_ERASE_GROUP_END;
    cmd.arg = end;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        goto err_out;

    cmd.cmd_code = MMC_CMD_ERASE;
    cmd.arg = MMC_ERASE_ARG;
    cmd.resp_type = MMC_RSP_R1b;

    err = mmc_send_cmd(host, &cmd, NULL);
    if (err)
        goto err_out;

    return 0;

err_out:
    pr_err("Erase blocks failed, error = %d\n", err);
    return err;
}

u32 mmc_berase(struct aic_sdmc *host, u32 start, u32 blkcnt)
{
    int err = 0;
    u32 blk = 0, blk_r = 0;
    u64 timeout = 1000000;
    u64 start_us = 0;

    while (blk < blkcnt) {
        start_us = aic_get_time_us();
        blk_r = ((blkcnt - blk) > host->dev->erase_grp_size) ?
                    host->dev->erase_grp_size :
                    (blkcnt - blk);
        err = mmc_erase_t(host, start + blk, blk_r);
        if (err)
            break;

        blk += blk_r;

        /* Waiting for the ready status */
        while (hal_sdmc_is_busy(&host->host)) {
            if (aic_get_time_us() - start_us > timeout) {
                pr_warn("Data transfer is busy\n");
                return 0;
            }
        }
    }

    return blk;
}

void mmc_setup_cfg(struct aic_sdmc *host)
{
    host->dev->freq_min = SDMC_CLOCK_MIN;
    host->dev->freq_max = host->sclk_rate;
    host->dev->host_caps = MMC_MODE_HC | MMC_MODE_HS | MMC_MODE_HS_52MHz | MMC_MODE_4BIT;
    host->dev->valid_ocr = MMC_VDD_32_33 | MMC_VDD_33_34;
    host->dev->voltages = MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 |
                          MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_34_35 |
                          MMC_VDD_35_36;
    host->dev->flags = EXT_CSD_SUP_SDIO_IRQ | EXT_CSD_SUP_HIGHSPEED | EXT_CSD_MUTBLKWRITE;

    if (host->pdata->buswidth == SDMC_CTYPE_4BIT)
        host->dev->flags |= EXT_CSD_BUS_WIDTH_4;
    if (host->pdata->buswidth == SDMC_CTYPE_8BIT)
        host->dev->flags |= EXT_CSD_BUS_WIDTH_8;

    host->dev->max_seg_size = 4096;
    host->dev->max_dma_segs = 256;
    host->dev->max_blk_size = 512;
    host->dev->max_blk_count = 65535;
    host->dev->blk_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;
}

static int mmc_identification(struct aic_sdmc *host)
{
    int err = 0;

    /* CMD0 - Reset the Card */
    err = mmc_go_idle(host);
    if (err) {
        pr_err("CMD0 failed, no card.\n");
        return err;
    }

    /* Trying SDMC for eMMC: First try eMMC, if it is failed, then try SD Card */
    if (host->index == 0) { // Try eMMC
        err = mmc_send_op_cond(host);
        if (err) {
            /*
             * CMD8 - SD_CMD_SEND_IF_COND
             * Test for SD version 2 or later
             */
            mmc_send_if_cond(host);
            /*
             * ACMD41 - SD_CMD_APP_SEND_OP_COND
             * Now try to get the SD card's operating condition
             */
            err = sd_send_op_cond(host); // Spend about 0.3 ms
        }

        if (err) {
            pr_err("Unknown card type.\n");
            return err;
        }
    /* Trying SDMC for SD: First try SD Card, if it is failed, then try eMMC */
    } else { // Try SD Card
        /* CMD8 - SD_CMD_SEND_IF_COND, Test for SD version 2 or later */
        mmc_send_if_cond(host);
        /*
         * ACMD41 - SD_CMD_APP_SEND_OP_COND, Now try to get the SD card's operating
         * condition
         */
        err = sd_send_op_cond(host); // Spend about 0.3 ms
        if (err) {
            /* The card is not SD,then check if it is eMMC */
            err = mmc_send_op_cond(host);
            if (err) {
                pr_err("Card did not respond to voltage select!\n");
                return -1;
            }
        }
    }

    /* CMD2 - Put the Card in Identify Mode */
    err = mmc_go_identify_mode(host);
    if (err) {
        pr_err("CMD2 failed.\n");
        return err;
    }

    err = mmc_relative_addr(host);

    return err;
}

struct aic_sdmc *find_mmc_dev_by_index(int id)
{
    int i;

    for (i = 0; i < MAX_MMC_DEV_NUM; i++) {
        if (mmc_dev[i] && ((struct aic_sdmc *)mmc_dev[i])->index == id)
            return mmc_dev[i];
    }

    return NULL;
}

s32 mmc_switch_part(struct aic_sdmc *host, u32 part_num)
{
    int err;

    if (part_num == MMC_PART_RPMB)
        err = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF, part_num);
    else
        err = mmc_select_bus_width(host);

    if (err) {
        pr_err("mmc switch part %d failed.\n", part_num);
        return err;
    }

    return 0;
}

s32 mmc_init(int id)
{
    struct aic_sdmc *host = NULL;
    struct aic_sdmc_pdata *pdata = NULL;
    struct aic_sdmc_dev *dev = NULL;
    int i, ret;

    if (id < 0 || id > MAX_MMC_DEV_NUM - 1) {
        pr_err("Invalid SDMC ID %d\n", id);
        return -1;
    }

    if (mmc_dev[id]) {
        pr_info("SDMC%d was already inited\n", id);
        return 0;
    }

    host = malloc(sizeof(struct aic_sdmc));
    if (!host) {
        pr_err("Failed to malloc(%d)\n", (u32)sizeof(struct aic_sdmc));
        return -1;
    }
    memset(host, 0, sizeof(struct aic_sdmc));

    dev = malloc(sizeof(struct aic_sdmc_dev));
    if (dev == NULL) {
        pr_err("malloc dev failed.\n");
        goto out;
    }
    memset(dev, 0, sizeof(struct aic_sdmc_dev));

    for (i = 0; i < ARRAY_SIZE(sdmc_pdata); i++) {
        if (sdmc_pdata[i].id == id) {
            pdata = &sdmc_pdata[i];
            break;
        }
    }

    if (pdata == NULL)
        goto out;

    host->index = pdata->id;
    host->irq = pdata->irq;
    host->clk = pdata->clk;
    host->host.base = (volatile void *)pdata->base;
    host->pdata = pdata;
    host->dev = dev;
    mmc_dev[id] = host;

    ret = aic_sdmc_clk_init(host);
    if (ret)
        goto out;

    aicos_request_irq(host->irq, aic_sdmc_irq, 0, NULL, host);

    host->host.fifoth_val = MSIZE(2) | RX_WMARK(7) | TX_WMARK(8);
    host->host.is_sdio = pdata->is_sdio;
    mmc_setup_cfg(host);

    aic_sdmc_init(host);
    pr_info("SDMC%d driver loaded\n", pdata->id);

    ret = mmc_identification(host);
    if (ret)
        goto out;

    ret = mmc_startup(host);
    if (ret)
        goto out;

    mmc_block_init(host);
    return 0;

out:
    if (host) {
        free(host);
        mmc_dev[id] = NULL;
    }
    if (dev)
        free(dev);

    return -1;
}

s32 mmc_deinit(int id)
{
    struct aic_sdmc *host = (struct aic_sdmc *)mmc_dev[id];

    if (id < 0 || id > MAX_MMC_DEV_NUM - 1) {
        pr_err("Invalid SDMC ID %d\n", id);
        return -1;
    }

    if (host == NULL) {
        pr_info("SDMC%d was already deinited\n", id);
        return 0;
    }

    mmc_block_deinit(host);

    if (host->dev)
        free(host->dev);

    if (host) {
        free(host);
        mmc_dev[id] = NULL;
    }

    return 0;
}
