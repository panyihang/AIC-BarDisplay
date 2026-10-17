/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Xiong Hao <hao.xiong@artinchip.com>
 */

#ifndef _DATA_TRANS_LAYER_H_
#define _DATA_TRANS_LAYER_H_

#include <aic_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AIC Vendor ID */
#define DRIVER_VENDOR_ID 0x33C3
/* 'fw' 0x66 0x77: product id for firmware upgrade */
#define DRIVER_PRODUCT_ID 0x6677

#define AIC_USB_SIGN_USBC        0x43425355
#define AIC_USB_SIGN_USBS        0x53425355
#define AIC_CBW_DATA_HOST_TO_DEV 0x0
#define AIC_CBW_DATA_DEV_TO_HOST 0x80
#define AIC_CSW_STATUS_PASSED    0x00
#define AIC_CSW_STATUS_FAILED    0x01
#define AIC_CSW_STATUS_PHASE_ERR 0x02

/* USB UPG Data transport layer commands */
#define TRANS_LAYER_CMD_WRITE 0x01
#define TRANS_LAYER_CMD_READ  0x02

struct aic_cbw {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t reserved0;
    uint8_t bCBWCBLength;
    uint8_t bCommand;
    uint8_t reserved[15];
} __attribute__((packed));

#define USB_SIZEOF_AIC_CBW 31

struct aic_csw {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} __attribute__((packed));

#define USB_SIZEOF_AIC_CSW 13

enum Stage {
    AICUPG_READ_CBW = 0,        /* Command Block Wrapper */
    AICUPG_DATA_OUT = 1,        /* Data Out Phase */
    AICUPG_DATA_OUT_BUF = 2,    /* Data Out ing Phase */
    AICUPG_DATA_IN = 3,         /* Data In Phase */
    AICUPG_DATA_IN_BUF = 4,     /* Data In ing Phase */
    AICUPG_SEND_CSW = 5,        /* Command Status Wrapper */
    AICUPG_WAIT_CSW = 6,        /* Command Status Wrapper */
};

struct aicupg_trans_info {
    enum Stage stage;
    __attribute__ ((aligned(CACHE_LINE_SIZE))) struct aic_cbw *cbw;
    __attribute__ ((aligned(CACHE_LINE_SIZE))) struct aic_csw *csw;

    uint32_t remain;
    uint32_t transfer_len;

    uint8_t *trans_pkt_buf;
    uint32_t trans_pkt_siz;

    uint32_t single_ep_trans_siz;

    uint32_t single_pkt_trans_siz;
    uint32_t single_pkt_transed_siz;

    uint32_t single_task_trans_siz;
    uint32_t single_task_transed_siz;

    uint32_t transferred_len;
    uint32_t (*send)(uint8_t *buf, uint32_t len);
    uint32_t (*recv)(uint8_t *buf, uint32_t len);
};

int32_t data_trans_layer_out_proc(struct aicupg_trans_info *info);
int32_t data_trans_layer_in_proc(struct aicupg_trans_info *info);

#ifdef __cplusplus
}
#endif

#endif /* _DATA_TRANS_LAYER_H_ */
