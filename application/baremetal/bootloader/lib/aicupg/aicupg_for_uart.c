/*
 * Copyright (c) 2023-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aic_common.h>
#include <aic_utils.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <uart.h>
#include <data_trans_layer.h>
#include <uart_proto_layer.h>
#include <aicupg_for_uart.h>
#include <aicupg.h>
#include <crc16.h>

//#define AICUPG_UART_DEBUG
#ifdef AICUPG_UART_DEBUG
#undef pr_err
#undef pr_warn
#undef pr_info
#undef pr_debug
#define pr_err printf
#define pr_warn printf
#define pr_info printf
#define pr_debug printf
#endif

#define TRANS_DATA_BUFF_MAX_SIZE (64 * 1024)

static struct aicupg_trans_info trans_info;
static int update_baudrate = 0;

static uint32_t uart_upg_ep_start_read(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    uart_proto_start_read((u8 *)data, (u32)data_len);
    return data_len;
}

static uint32_t uart_upg_ep_start_write(uint8_t *data, uint32_t data_len)
{
    trans_info.transfer_len = data_len;
    uart_proto_start_write((u8 *)data, (u32)data_len);
    return data_len;
}

static void uart_upg_notify_handler(uint8_t event, void *arg)
{
    pr_debug("Start reading aic cbw\r\n");
    if (!trans_info.trans_pkt_buf) {
        trans_info.trans_pkt_siz = TRANS_DATA_BUFF_MAX_SIZE;
        trans_info.trans_pkt_buf = aicupg_malloc_align(trans_info.trans_pkt_siz, CACHE_LINE_SIZE);
        if (!trans_info.trans_pkt_buf) {
            pr_err("malloc trans pkt buf(%u) failed.\n", (u32)trans_info.trans_pkt_siz);
            return;
        }
    }
    trans_info.stage = AICUPG_READ_CBW;
    trans_info.recv((uint8_t *)trans_info.cbw, sizeof(struct aic_cbw));
}

static void uart_upg_in_callback(uint8_t ep, uint32_t nbytes)
{
    pr_debug("actual in len:%d\r\n", (u32)nbytes);
    data_trans_layer_in_proc(&trans_info);
}

static void uart_upg_out_callback(uint8_t ep, uint32_t nbytes)
{
    pr_debug("actual out len:%d\r\n", (u32)nbytes);
    data_trans_layer_out_proc(&trans_info);
}

static u32 uart_upg_get_baudrate(int id)
{
    usart_handle_t h;

    h = hal_usart_initialize(id, NULL, NULL);
    return hal_usart_get_cur_baudrate(h);
}

void aic_uart_upg_baudrate_update(int baudrate)
{
    update_baudrate = baudrate;
    pr_debug("update burn baudrate:%d\n", baudrate);
}


void aic_uart_upg_loop(int id)
{
    int status = 0;

    status = uart_proto_layer_get_status();

    if (status == UPG_UART_DATA_SEND_DONE) {
        uart_upg_in_callback(id, 0);
    }

    if (status == UPG_UART_DATA_RECV_DONE) {
        uart_proto_layer_set_status(UPG_UART_WAIT_ACK);
        uart_upg_out_callback(id, 0);
    }

    if (trans_info.stage == AICUPG_WAIT_CSW && update_baudrate) {
        pr_info("Update baudrate: %d bps\n", update_baudrate);
        if (uart_config_update(0, update_baudrate)) {
            pr_err("update uart baud rate %d failed.\n", update_baudrate);
            update_baudrate = 0;
            return;
        }
        update_baudrate = 0;
    }
}

void aic_uart_upg_init(int id)
{
    int brate;

    brate = uart_upg_get_baudrate(id);
    uart_init(id);
    uart_reset(id);
    if (brate > 0)
        uart_config_update(id, brate);

    uart_proto_layer_init(id);

    memset((uint8_t *)&trans_info, 0, sizeof(struct aicupg_trans_info));

    trans_info.send = uart_upg_ep_start_write;
    trans_info.recv = uart_upg_ep_start_read;

    trans_info.cbw = aicupg_malloc_align(sizeof(struct aic_cbw), CACHE_LINE_SIZE);
    trans_info.csw = aicupg_malloc_align(sizeof(struct aic_csw), CACHE_LINE_SIZE);

    uart_upg_notify_handler(0, NULL);
}

void aic_uart_upg_deinit(int id)
{
    uart_proto_layer_deinit();
}
