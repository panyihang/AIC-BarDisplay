/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BL_UPG_UART_H_
#define __BL_UPG_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <aic_common.h>
#include <ringbuffer.h>

#define CONN_STATE_DETECTING 0
#define CONN_STATE_CONNECTED 1
#define DIR_HOST_RECV        0
#define DIR_HOST_SEND        1

#define SOH      0x01
#define STX      0x02
#define ACK      0x06
#define DC1_SEND 0x11
#define DC2_RECV 0x12
#define NAK      0x15
#define CAN      0x18
#define SIG_A    0x41
#define SIG_C    0x43

#define LNG_FRM_DLEN      (1024)
#define LNG_FRM_MAX_LEN   (LNG_FRM_DLEN + 5)
#define SHT_FRM_DLEN      (176)
#define SHT_FRM_MAX_LEN   (SHT_FRM_DLEN + 6)
#define UART_RECV_BUF_LEN (1200)
#define RESET_POS         (128)
#define UART_RBUF_SIZE    (1200)

#define UART_SKIP_DATA 0
#define UART_NO_DATA   -1
#define UART_ERR_DATA  -2

#define SIG_A_INTERVAL_MS 1000

enum stage {
    UPG_UART_CMD_RECV = 0,      /* Command Block Wrapper */
    UPG_UART_DATA_SEND = 1,        /* Data Out Phase */
    UPG_UART_DATA_SEND_BUF = 2,    /* Data Out ing Phase */
    UPG_UART_DATA_SEND_DONE = 3,    /* Data Out ing Phase */
    UPG_UART_DATA_RECV = 4,         /* Data In Phase */
    UPG_UART_DATA_RECV_BUF = 5,     /* Data In ing Phase */
    UPG_UART_DATA_RECV_DONE = 6,     /* Data In ing Phase */
    UPG_UART_SEND_ACK = 7,        /* Command Status Wrapper */
    UPG_UART_WAIT_ACK = 8,        /* Command Status Wrapper */
};

struct uart_status_fifo {
    struct ringbuffer rb;
    u8 buffer[8];
};

struct proto_task {
    struct list_head list;
    u8 *data;
    u32 data_len;
    u32 xfer_len;
    u32 status;
    u32 direction;
    u32 single_frame_trans_siz;
    u32 single_frame_transed_siz;
};

struct uart_proto_device {
    struct uart_status_fifo status_fifo;
    u32 last_recv_time;
    u32 last_send_time;
    u8 recv_blk_no;
    u8 send_blk_no;
    s32 first_connect;
};

int uart_proto_start_read(u8 *data, u32 data_len);
int uart_proto_start_write(u8 *data, u32 data_len);
int uart_proto_layer_init(int id);
int uart_proto_layer_deinit();
int uart_proto_layer_get_status(void);
void uart_proto_layer_set_status(int status);

#ifdef __cplusplus
}
#endif

#endif /* __BL_UPG_UART_H_ */
