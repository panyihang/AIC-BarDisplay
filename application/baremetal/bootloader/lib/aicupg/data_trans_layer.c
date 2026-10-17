/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#include <data_trans_layer.h>
#include <aicupg.h>
#include <aic_utils.h>
//#define aicupg_debug
#ifdef aicupg_debug
#undef pr_err
#undef pr_warn
#undef pr_info
#undef pr_debug
#define pr_err printf
#define pr_warn printf
#define pr_info printf
#define pr_debug printf
#endif

static void aicupg_send_csw(struct aicupg_trans_info *info, uint8_t status)
{
    info->csw->dCSWSignature = AIC_USB_SIGN_USBS;
    info->csw->bCSWStatus = status;

    /* updating the State Machine , so that we wait CSW when this
     * transfer is complete, ie when we get a bulk in callback
     */
    info->stage = AICUPG_WAIT_CSW;

    pr_debug("send CSW tag 0x%X, status = %d\n", (u32)info->csw->dCSWTag, info->csw->bCSWStatus);
    //hexdump_msg("CSW ", (u8 *)&info->csw, sizeof(struct aic_csw), 1);
    info->send((u8 *)info->csw, sizeof(struct aic_csw));
}

static bool AICUPG_processReadBuf(struct aicupg_trans_info *info)
{
    uint32_t retlen;

    pr_debug("%s, %d\n", __func__, __LINE__);
    retlen = info->send(info->trans_pkt_buf + info->single_pkt_transed_siz, info->single_pkt_trans_siz);

    info->single_pkt_transed_siz += retlen;

    if (info->single_pkt_transed_siz >= info->single_pkt_trans_siz) {
        info->single_pkt_transed_siz = 0;
        info->stage = AICUPG_DATA_IN;
    }

    if (info->remain <= retlen) {
        info->remain = 0;
        info->stage = AICUPG_SEND_CSW;
    } else {
        info->remain -= retlen;
    }

    return true;
}

static bool AICUPG_processRead(struct aicupg_trans_info *info)
{
    uint32_t transfer_len, slice;

    pr_debug("%s, %d\n", __func__, __LINE__);
    if (info->remain >= info->trans_pkt_siz)
        slice = info->trans_pkt_siz;
    else
        slice = info->remain;

    transfer_len = aicupg_data_packet_read(info->trans_pkt_buf, slice);
    if (transfer_len != slice) {
        pr_err("transfer len:%d != slice len:%d\n", (u32)transfer_len, (u32)slice);
        return false;
    }

    info->stage = AICUPG_DATA_IN_BUF;
    info->single_pkt_trans_siz = transfer_len;
    AICUPG_processReadBuf(info);

    return true;
}

static bool AICUPG_processWriteBuf(struct aicupg_trans_info *info)
{
    uint32_t retlen;

    pr_debug("%s, %d\n", __func__, __LINE__);
    retlen = info->recv(info->trans_pkt_buf + info->single_pkt_transed_siz, info->single_pkt_trans_siz);

    info->single_pkt_transed_siz += retlen;

    pr_debug("single_pkt_transed_siz:0x%x, single_pkt_trans_siz:0x%x\n",
            (u32)info->single_pkt_transed_siz, (u32)info->single_pkt_trans_siz);
    if (info->single_pkt_transed_siz >= info->single_pkt_trans_siz)
    {
        info->single_pkt_transed_siz = 0;
        info->stage = AICUPG_DATA_OUT;
    }

    return true;
}

static bool AICUPG_processWrite(struct aicupg_trans_info *info, uint32_t nbytes)
{
    uint32_t slice, transfer_len = 0;

    pr_debug("%s, %d\n", __func__, __LINE__);

    //hexdump_msg("DATA ", (u8 *)info->trans_pkt_buf, nbytes, 1);
    transfer_len = aicupg_data_packet_write(info->trans_pkt_buf, nbytes);
    if (transfer_len != nbytes) {
        pr_err("transfer len:%d != nbytes len:%d\n", (u32)transfer_len, (u32)nbytes);
        return false;
    }

    info->remain -= transfer_len;
    info->csw->dCSWDataResidue -= transfer_len;

    if (info->remain == 0) {
        aicupg_send_csw(info, AIC_CSW_STATUS_PASSED);
    }

    if (info->remain != 0) {
        if (info->remain >= info->trans_pkt_siz)
            slice = info->trans_pkt_siz;
        else
            slice = info->remain;
        info->stage = AICUPG_DATA_OUT_BUF;
        info->single_pkt_trans_siz = slice;
        AICUPG_processWriteBuf(info);
    }

    return true;
}

static bool AICUPG_read10(struct aicupg_trans_info *info, uint8_t **data, uint32_t *len)
{
    (void)data;
    (void)len;

    if ((info->cbw->bmCBWFlags != 0x80) || (info->cbw->bCBWCBLength == 0)) {
        pr_err("Flag:0x%02x\r\n", info->cbw->bmCBWFlags);
        return false;
    }

    info->stage = AICUPG_DATA_IN;
    info->remain = info->cbw->dCBWDataTransferLength;

    pr_debug("%s:TLength:0x%x\r\n", __func__, (u32)info->cbw->dCBWDataTransferLength);

    return AICUPG_processRead(info);
}

static bool AICUPG_write10(struct aicupg_trans_info *info, uint8_t **data, uint32_t *len)
{
    uint32_t slice;

    if ((info->cbw->bmCBWFlags != 0x0) || (info->cbw->bCBWCBLength == 0)) {
        pr_err("Flag:0x%02x\r\n", info->cbw->bmCBWFlags);
        return false;
    }

    info->stage = AICUPG_DATA_OUT_BUF;
    info->remain = info->cbw->dCBWDataTransferLength;

    pr_debug("%s:TLength:0x%x\r\n", __func__, (u32)info->cbw->dCBWDataTransferLength);

    if (info->remain >= info->trans_pkt_siz)
        slice = info->trans_pkt_siz;
    else
        slice = info->remain;

    info->single_pkt_trans_siz = slice;
    AICUPG_processWriteBuf(info);

    return true;
}

static bool AICUPG_CBWDecode(struct aicupg_trans_info *info, uint32_t nbytes)
{
    bool ret = false;

    if (nbytes != sizeof(struct aic_cbw)) {
        //pr_err("%s:%d:size != sizeof(aic_cbw)\r\n", __func__, __LINE__);
        return true;
    }

    info->csw->dCSWTag = info->cbw->dCBWTag;
    info->csw->dCSWDataResidue = info->cbw->dCBWDataTransferLength;

    //hexdump_msg("CBW ", (u8 *)&info->cbw, sizeof(struct aic_cbw), 1);
    if (info->cbw->dCBWSignature != AIC_USB_SIGN_USBC) {
        pr_err("CBW signature not correct\n");
        hexdump_msg("CBW ", (u8 *)info->cbw, sizeof(struct aic_cbw), 1);
        /* Status error or CBW not valid, just skip pakcet here */
        return false;
    } else {
        switch (info->cbw->bCommand) {
            case TRANS_LAYER_CMD_READ:
                ret = AICUPG_read10(info, NULL, 0);
                break;
            case TRANS_LAYER_CMD_WRITE:
                ret = AICUPG_write10(info, NULL, 0);
                break;
            default:
                pr_warn("unsupported cmd:0x%02x\r\n", info->cbw->bCommand);
                ret = false;
                break;
        } 
    }
    if (ret) {
        if (info->stage == AICUPG_READ_CBW) {
            aicupg_send_csw(info, AIC_CSW_STATUS_PASSED);
        }
    }
    return ret;
}

int32_t data_trans_layer_out_proc(struct aicupg_trans_info *info)
{
    pr_debug("%s, %d\n", __func__, __LINE__);
    pr_debug("stage:%d, bCommand:%d\n", info->stage, info->cbw->bCommand);
    switch (info->stage) {
        case AICUPG_READ_CBW:
            //memcpy(&info->cbw, info->trans_pkt_buf, sizeof(struct aic_cbw));
            if (AICUPG_CBWDecode(info, info->transfer_len) == false) {
                pr_err("Command:0x%02x decode err\r\n", info->cbw->bCommand);
                return -1;
            }
            break;
        case AICUPG_DATA_OUT_BUF:
            switch (info->cbw->bCommand) {
                case TRANS_LAYER_CMD_WRITE:
                    if (AICUPG_processWriteBuf(info) == false) {
                        /* send fail status to host, and the host will retry */
                        aicupg_send_csw(info, AIC_CSW_STATUS_FAILED);
                    }
                    break;
                default:
                    break;
            }
            break;
        case AICUPG_DATA_OUT:
            switch (info->cbw->bCommand) {
                case TRANS_LAYER_CMD_WRITE:
                    if (AICUPG_processWrite(info, info->transfer_len) == false) {
                        /* send fail status to host, and the host will retry */
                        aicupg_send_csw(info, AIC_CSW_STATUS_FAILED);
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return 0;
}

int32_t data_trans_layer_in_proc(struct aicupg_trans_info *info)
{
    pr_debug("%s, %d\n", __func__, __LINE__);
    pr_debug("stage:%d, bCommand:%d\n", info->stage, info->cbw->bCommand);
    switch (info->stage) {
        case AICUPG_DATA_IN:
            switch (info->cbw->bCommand) {
                case TRANS_LAYER_CMD_READ:
                    if (AICUPG_processRead(info) == false) {
                        /* send fail status to host, and the host will retry */
                        aicupg_send_csw(info, AIC_CSW_STATUS_FAILED);
                        return -1;
                    }
                    break;
                default:
                    break;
            }
            break;
        case AICUPG_DATA_IN_BUF:
            switch (info->cbw->bCommand) {
                case TRANS_LAYER_CMD_READ:
                    if (AICUPG_processReadBuf(info) == false) {
                        /* send fail status to host, and the host will retry */
                        aicupg_send_csw(info, AIC_CSW_STATUS_FAILED);
                        return -1;
                    }
                    break;
                default:
                    break;
            }
            break;
        case AICUPG_SEND_CSW:
            aicupg_send_csw(info, AIC_CSW_STATUS_PASSED);
            break;
        case AICUPG_WAIT_CSW:
            info->stage = AICUPG_READ_CBW;
            info->recv((uint8_t *)info->cbw, USB_SIZEOF_AIC_CBW);
            pr_debug("%s,%d, stage:%d, bCommand:%d\n", __func__, __LINE__, info->stage, info->cbw->bCommand);
            break;
        default:
            break;
    }
    return 0;
}
