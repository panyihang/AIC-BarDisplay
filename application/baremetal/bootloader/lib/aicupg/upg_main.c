/*
 * Copyright (c) 2023-2026, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <aic_core.h>
#include <aicupg.h>
#include "upg_internal.h"

#define false 0
#define true  1

struct upg_internal upg_info = {
    .cur_cmd = NULL,
    .dev_type = UPG_DEV_TYPE_RAM,
    .dev_id = 0,
    .cfg = {
        .mode = 0,
    }
};

static int check_cmd_header(struct cmd_header *h)
{
    u32 sum;

    if (h->magic != UPG_CMD_HEADER_MAGIC)
        return false;
    if (h->protocol != UPG_PROTO_TYPE)
        return false;
    if (h->version != UPG_PROTO_VERSION)
        return false;

    sum = 0;
    sum += h->magic;
    sum += ((h->reserved << 24) | (h->command << 16) | (h->version << 8) |
            h->protocol);
    sum += h->data_length;
    if (sum != h->checksum)
        return false;

    return true;
}

void aicupg_gen_resp(struct resp_header *h, u8 cmd, u8 sts, u32 len)
{
    u32 sum;

    h->magic = UPG_CMD_RESP_MAGIC;
    h->protocol = UPG_PROTO_TYPE;
    h->version = UPG_PROTO_VERSION;
    h->command = cmd;
    h->status = sts;
    h->data_length = len;

    sum = 0;
    sum += h->magic;
    sum += ((h->status << 24) | (h->command << 16) | (h->version << 8) |
            h->protocol);
    sum += h->data_length;
    h->checksum = sum;
}

static char *get_upg_mode_name(int mode)
{
    char *modes[] = {
        "Full disk upgrade",
        "Partition upgrade",
        "Burn UserID",
        "Dump partition",
        "Force upgrade",
        "Burn frozen",
    };
    char *invalid = "Invalid mode";

    if (mode < 0 || mode >= UPG_MODE_INVALID)
        return invalid;
    return modes[mode];
}

void aicupg_show_upg_cfg_mode(int mode)
{
    printf("UPGMODE: %s\n", get_upg_mode_name(mode));
}

void aicupg_show_init_cfg_mode(int mode_bits)
{
    int i;

    printf("Init UPGMODE:\n");
    for (i = 0; i < UPG_MODE_INVALID; i++) {
        if (mode_bits & (1 << i))
            printf("    %s\n", get_upg_mode_name(i));
    }
}

s32 aicupg_set_upg_cfg(struct upg_cfg *cfg)
{
    if (!cfg) {
        pr_info("Invalide parameter.\n");
        return -1;
    }

    memcpy(&upg_info.cfg, cfg, sizeof(*cfg));
    aicupg_show_upg_cfg_mode(upg_info.cfg.mode);

    return 0;
}

s32 aicupg_initialize(struct upg_init *param)
{
    upg_info.init.mode_bits = param->mode_bits;
    aicupg_show_init_cfg_mode(upg_info.init.mode_bits);
    return 0;
}

s32 aicupg_get_upg_mode(void)
{
    return (s32)upg_info.cfg.mode;
}

void set_current_command(struct upg_cmd *cmd)
{
    upg_info.cur_cmd = cmd;
}

struct upg_cmd *get_current_command(void)
{
    return upg_info.cur_cmd;
}

enum upg_cmd_state get_current_command_state(void)
{
    if (upg_info.cur_cmd)
        return upg_info.cur_cmd->state;
    return CMD_STATE_IDLE;
}

void set_current_device_type(enum upg_dev_type type)
{
    upg_info.dev_type = type;
}

enum upg_dev_type get_current_device_type(void)
{
    return upg_info.dev_type;
}

const char *get_current_device_name(enum upg_dev_type type)
{
    char *dev_list[] = {
        "RAM",
        "MMC",
        "SPI_NAND",
        "SPI_NOR",
        "RAW_NAND",
        "UNKNOWN",
    };

    return dev_list[type];
}

void set_current_device_id(int id)
{
    upg_info.dev_id = id;
}

int get_current_device_id(void)
{
    return upg_info.dev_id;
}

const char *get_upg_media_type(void)
{
    return upg_info.media_type;
}

u32 get_upg_media_dev_id(void)
{
    return upg_info.media_dev_id;
}

static struct upg_cmd *find_command(struct cmd_header *h)
{
    struct upg_cmd *cmd = NULL;

    cmd = find_basic_command(h);
    if (cmd)
        return cmd;

    /* Not basic command, maybe it is FWC relative command. */
    cmd = find_fwc_command(h);
    return cmd;
}

s32 aicupg_data_packet_write(u8 *data, s32 len)
{
    struct cmd_header h;
    struct upg_cmd *cmd;
    u32 clen;

    clen = 0;
    if (len >= sizeof(struct cmd_header))
        memcpy(&h, data, sizeof(struct cmd_header));

    if ((len >= sizeof(struct cmd_header)) &&
        (check_cmd_header(&h) == true)) {
        /* Command start packet, find the command handler */
        cmd = find_command(&h);
        set_current_command(cmd);
        if (cmd)
            cmd->start(cmd, h.data_length);
        clen = sizeof(struct cmd_header);
    }

    /* Maybe this packet is cmd_header only */
    if (clen == len)
        return clen;
    /* There is command data */
    cmd = get_current_command();
    if (cmd && cmd->write_input_data)
        clen += cmd->write_input_data(cmd, data, len - clen);

    /* End CMD after CSW is sent */
    if (get_current_command_state() == CMD_STATE_END)
        cmd->end(cmd);

    pr_debug("%s, l: %d\n", __func__, __LINE__);
    return clen;
}

s32 aicupg_data_packet_read(u8 *data, s32 len)
{
    struct upg_cmd *cmd;
    s32 rlen = 0;

    /* Host read data from device */
    cmd = get_current_command();
    if (cmd && cmd->read_output_data)
        rlen = cmd->read_output_data(cmd, data, len);

    /* End CMD before CSW is sent */
    if (get_current_command_state() == CMD_STATE_END)
        cmd->end(cmd);

    return rlen;
}

int aicupg_get_fwc_attr(struct fwc_info *fwc)
{
    int attr = 0;

    if (!fwc)
        return 0;

    if (strstr(fwc->meta.attr, "required"))
        attr |= FWC_ATTR_REQUIRED;
    else if (strstr(fwc->meta.attr, "optional"))
        attr |= FWC_ATTR_OPTIONAL;

    if (strstr(fwc->meta.attr, "run"))
        attr |= FWC_ATTR_ACTION_RUN;
    else if (strstr(fwc->meta.attr, "burn"))
        attr |= FWC_ATTR_ACTION_BURN;

    if (strstr(fwc->meta.attr, "block"))
        attr |= FWC_ATTR_DEV_BLOCK;
    else if (strstr(fwc->meta.attr, "mtd"))
        attr |= FWC_ATTR_DEV_MTD;
    else if (strstr(fwc->meta.attr, "ubi"))
        attr |= FWC_ATTR_DEV_UBI;
    else if (strstr(fwc->meta.attr, "uffs"))
        attr |= FWC_ATTR_DEV_UFFS;

    return attr;
}

/*
 * Init fwc and config fwc->meta
 */
void fwc_meta_config(struct fwc_info *fwc, struct fwc_meta *pmeta)
{
    memset((void *)fwc, 0, sizeof(struct fwc_info));
    memcpy(&fwc->meta, pmeta, sizeof(struct fwc_meta));
}

/*
 * Get memory type by header and flash_index
 * - For multi-flash, media_type uses ";" as separator, e.g. "spi-nor;spi-nand"
 * - flash_index selects which token to return
 * - A stack-local copy is used because strtok_r modifies the string in-place
 */
static enum upg_dev_type media_type_get(struct image_header_upgrade *header, int flash_index)
{
    char media_type_copy[64];
    char *token;
    char *saveptr;
    static enum upg_dev_type type;
    int index = 0;

    pr_debug("%s, %s\n", __func__, header->media_type);

    /* Copy to avoid modifying the original header string */
    strncpy(media_type_copy, header->media_type, sizeof(media_type_copy) - 1);
    media_type_copy[sizeof(media_type_copy) - 1] = '\0';

    /* Split by ";" and walk to the flash_index-th token */
    token = strtok_r(media_type_copy, ";", &saveptr);
    while (token != NULL && index <= flash_index) {
        if (index == flash_index) {
            if (strcmp(token, "mmc") == 0)
                type = UPG_DEV_TYPE_MMC;
            else if (strcmp(token, "spi-nand") == 0)
                type = UPG_DEV_TYPE_SPI_NAND;
            else if (strcmp(token, "spi-nor") == 0)
                type = UPG_DEV_TYPE_SPI_NOR;
            else
                type = UPG_DEV_TYPE_UNKNOWN;
            break;
        }
        token = strtok_r(NULL, ";", &saveptr);
        index++;
    }

    return type;
}

/*
 * Prepare write data
 * - For multi-flash, iterate all flashes parsed from media_type
 * - Each flash ID is extracted from media_dev_id (1 byte per flash)
 * - Call the corresponding prepare function for each flash
 */
s32 media_device_prepare(struct fwc_info *fwc, struct image_header_upgrade
        *header)
{
    enum upg_dev_type type;
    s32 ret = 0;
    char *media_type_copy;
    char *token;
    char *saveptr;
    int flash_count = 0;
    int i;
    u8 dev_id;

    /* Save media_type and media_dev_id to upg_info for later use */
    strncpy(upg_info.media_type, header->media_type, sizeof(upg_info.media_type) - 1);
    upg_info.media_type[sizeof(upg_info.media_type) - 1] = '\0';
    upg_info.media_dev_id = header->media_dev_id;

    /* Count how many flashes are specified in media_type (separated by ";") */
    media_type_copy = strdup(header->media_type);
    if (!media_type_copy) {
        pr_err("Memory allocation failed\n");
        return -1;
    }

    token = strtok_r(media_type_copy, ";", &saveptr);
    while (token != NULL) {
        flash_count++;
        token = strtok_r(NULL, ";", &saveptr);
    }
    free(media_type_copy);

    /* Prepare each flash in order */
    for (i = 0; i < flash_count; i++) {
        type = media_type_get(header, i);
        /* Extract the i-th flash ID from media_dev_id (1 byte per flash) */
        dev_id = (header->media_dev_id >> (i * 8)) & 0xFF;

        set_current_device_type(type);
        set_current_device_id(dev_id);

        switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_prepare(fwc, dev_id);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_prepare(fwc, dev_id);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_prepare(fwc, dev_id);
                break;
#endif
            default:
                pr_err("device type %d is not support!...\n", type);
                ret = -1;
                break;
        }

        /* Stop on first failure */
        if (ret < 0)
            break;
    }

    return ret;
}

/*
 * Start write data
 * - Select function based on type
 */
void media_data_write_start(struct fwc_info *fwc)
{
    enum upg_dev_type type;

    type = get_current_device_type();
    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            mmc_fwc_start(fwc);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            nand_fwc_start(fwc);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            nor_fwc_start(fwc);
            break;
#endif
        default:
            pr_err("device type is not support!...\n");
            break;
    }
}

/*
 * Write data to memory device
 * - Make the data size into whole block
 * - Select function based on type
 */
s32 media_data_write(struct fwc_info *fwc, u8 *buf, u32 len)
{
    enum upg_dev_type type;
    s32 ret, len_to_write;

    type = get_current_device_type();
    if (len % fwc->block_size)
        len_to_write = len + fwc->block_size - (len % fwc->block_size);
    else
        len_to_write = len;

    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            ret = mmc_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            ret = nand_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            ret = nor_fwc_data_write(fwc, buf, len_to_write);
            break;
#endif
        default:
            ret = 0;
            pr_err("device type is not support!...\n");
            break;
    }

    /* The size of the data we actually write is len */
    if (ret != len_to_write)
        ret = 0;
    else
        ret = len;

    return ret;
}

/*
 * End write data
 * - Select function based on type
 */
void media_data_write_end(struct fwc_info *fwc)
{
    enum upg_dev_type type;

    type = get_current_device_type();
    switch (type) {
#if defined(AICUPG_MMC_ARTINCHIP)
        case UPG_DEV_TYPE_MMC:
            mmc_fwc_data_end(fwc);
            break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NAND:
            nand_fwc_data_end(fwc);
            break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        case UPG_DEV_TYPE_SPI_NOR:
            nor_fwc_data_end(fwc);
            break;
#endif
        default:
            pr_err("device type %d is not support!...\n", type);
            break;
    }
}

void *aicupg_malloc_align(u32 size, size_t align)
{
    return aicos_malloc_align(MEM_RESERVED, size, align);
}

void aicupg_free_align(void *ptr)
{
    aicos_free_align(MEM_RESERVED, ptr);
}
