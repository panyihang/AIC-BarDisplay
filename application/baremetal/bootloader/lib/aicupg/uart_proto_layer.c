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
#include <uart.h>
#include <uart_proto_layer.h>
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

#define update_last_recv_time() \
    do { \
        uart_proto.last_recv_time = aic_get_time_ms(); \
    } while(0)

static LIST_HEAD(task);

struct uart_proto_device uart_proto;
static u8 g_uart_send_recv_buf[LNG_FRM_MAX_LEN];
static int uart_proto_id = 0;

static int uart_proto_add_task(u32 dir, u8 *data, u32 data_len)
{
	struct proto_task *t = NULL;

    t = aicos_malloc(0, sizeof(struct proto_task));
    if (!t)
    {
        pr_err("alloc task failed\n");
        return -1;
    }

    t->direction = dir;
    t->status = UPG_UART_CMD_RECV;
    t->data = data;
    t->data_len = data_len;
    t->xfer_len = 0;
    t->single_frame_trans_siz = 0;
    t->single_frame_transed_siz = 0;

	list_add_tail(&t->list, &task);
	return 0;
}

static struct proto_task *uart_proto_get_task(void)
{
	struct list_head *pos;

	list_for_each(pos, &task) {
		return list_entry(pos, struct proto_task, list);
	}

	return NULL;
}

static int uart_proto_del_task(void)
{
	struct proto_task *t;

	t = uart_proto_get_task();
	if (!t) {
		pr_info("list is NULL.\n");
		return -ENOENT;
	}

	list_del(&t->list);
	free(t);
	return 0;
}

static void uart_proto_putchar(int id, u8 ch)
{
    pr_debug("---> Send 0x%x\n", ch);
    uart_putchar(id, ch);
    uart_proto.last_send_time = aic_get_time_ms();
}

static int uart_proto_gets(int id, u8 *buf, int len)
{
    int ret = 0;

    ret = uart_gets(id, buf, len);
    if (ret) {
        uart_proto.last_recv_time = aic_get_time_ms();
#ifdef AICUPG_UART_DEBUG
        if (ret <= 64)
            hexdump_msg("Recvs:", buf, ret, 1);
#endif
    }
    return ret;
}

static int uart_proto_puts(int id, u8 *buf, int len)
{
    int ret = 0;

    ret = uart_puts(id, buf, len);
    if (ret) {
        uart_proto.last_send_time = aic_get_time_ms();
#ifdef AICUPG_UART_DEBUG
        if (ret <= 64)
            hexdump_msg("Sends:", buf, ret, 1);
#endif
    }
    return ret;
}

static int pack_frame(u8 blk, u8 *frame, u8 *data, int len)
{
    u16 crc16;
    int flen;

    if (len == LNG_FRM_DLEN) {
        pr_debug("pack long frame\n");
        frame[0] = STX;
        frame[1] = blk;
        frame[2] = 255 - blk;
        memcpy(&frame[3], data, len);
        crc16 = crc16_ccitt(0, data, len);
        frame[LNG_FRM_DLEN + 3] = (u8)(crc16 >> 8);
        frame[LNG_FRM_DLEN + 4] = (u8)(crc16 & 0xFF);
        flen = len + 5;
    } else if (len <= SHT_FRM_DLEN) {
        pr_debug("pack short frame\n");
        frame[0] = SOH;
        frame[1] = blk;
        frame[2] = 255 - blk;
        frame[3] = (u8)len;
        memcpy(&frame[4], data, len);
        crc16 = crc16_ccitt(0, data, len);
        frame[len + 4] = (u8)(crc16 >> 8);
        frame[len + 5] = (u8)(crc16 & 0xFF);
        flen = len + 6;
    } else {
        return 0;
    }

    return flen;
}

static int read_short_frame_data(u8 *buf, int len)
{
    int flen;
    u16 crc16, crc16_i;
    u8 blk1, blk2, nextblk;

    pr_debug("SHORT: %llu\n", aic_get_time_us());
    // hexdump(buf, len, 1);
    blk1 = buf[1];
    blk2 = buf[2];
    flen = buf[3];
    if (blk1 != (255 - blk2)) {
        pr_err("blk1 %d != blk2 %d\n", blk1, (255 - blk2));
        return UART_ERR_DATA;
    }
    pr_debug("blk no %d\n", blk1);

    crc16_i = buf[4 + flen] << 8 | buf[4 + flen + 1];
    crc16 = crc16_ccitt(0, &buf[4], flen);
    if (crc16 != crc16_i) {
        pr_err("crc16 error: 0x%x != 0x%x\n", crc16, crc16_i);
        return UART_ERR_DATA;
    }

    nextblk = uart_proto.recv_blk_no + 1;
    if (blk1 != 1 && nextblk != blk1 && !uart_proto.first_connect) {
        pr_err("blk no discontinue should be %d, got %d\n", nextblk, blk1);
        return UART_ERR_DATA;
    }

    if (blk1 == uart_proto.recv_blk_no) {
        printf("Repeat frame.\n");
        return UART_SKIP_DATA;
    }
    uart_proto.recv_blk_no = blk1;
    uart_proto.first_connect = 0;
    pr_debug("%s done\n", __func__);
    return flen;
}

/*
 * return > 0: ACK
 * return = 0: ACK
 * return < 0: NAK
 */
static int read_long_frame_data(u8 *buf, int len)
{
    int flen;
    u16 crc16, crc16_i;
    u8 blk1, blk2, nextblk;

    pr_debug("LONG: %llu\n", aic_get_time_us());
    blk1 = buf[1];
    blk2 = buf[2];
    flen = LNG_FRM_DLEN;
    if (blk1 != (255 - blk2)) {
        pr_err("blk1 %d != blk2 %d\n", blk1, (255 - blk2));
        return UART_ERR_DATA;
    }
    pr_debug("blk no %d\n", blk1);

    crc16_i = buf[3 + flen] << 8 | buf[3 + flen + 1];
    crc16 = crc16_ccitt(0, &buf[3], flen);
    if (crc16 != crc16_i) {
        pr_err("crc16 error: 0x%x != 0x%x\n", crc16, crc16_i);
        hexdump(buf, LNG_FRM_MAX_LEN, 1);
        return UART_ERR_DATA;
    }

    nextblk = uart_proto.recv_blk_no + 1;
    if (blk1 != 1 && nextblk != blk1 && !uart_proto.first_connect) {
        pr_err("blk no discontinue should be %d, got %d\n", nextblk, blk1);
        return UART_ERR_DATA;
    }

    /* Recv repeat frame, just skip it */
    if (blk1 == uart_proto.recv_blk_no) {
        pr_warn("Repeat frame.\n");
        return UART_SKIP_DATA;
    }
    uart_proto.recv_blk_no = blk1;
    uart_proto.first_connect = 0;
    pr_debug("%s done\n", __func__);
    return flen;
}

static int read_frame_data(u8 *buf, int len)
{
    if (buf[0] == SOH) {
        return read_short_frame_data(buf, len);
    } else if (buf[0] == STX) {
        return read_long_frame_data(buf, len);
    } else {
        return UART_ERR_DATA;
    }
}

static int uart_proto_layer_host_send_proc(struct proto_task *t, u8 *pframe)
{
    int ret, rest;
    u8 ch = 0, *p;
    u64 delta = 0;

    switch (t->status) {
    case UPG_UART_DATA_RECV:
        if (t->single_frame_transed_siz == 0) {
            ret = uart_proto_gets(uart_proto_id, &pframe[0], 1);
            if (ret == 0) {
                //pr_debug("read head failed.\n");
                return -1;
            }

            if (pframe[0] != SOH && pframe[0] != STX) {
                //pr_err("frame error. unknown frame data.\n");
                break;
            }
            t->single_frame_transed_siz++;
        }

        if (t->single_frame_transed_siz == 1) {
            ret = uart_proto_gets(uart_proto_id, &pframe[1], 1);
            if (ret == 0) {
                pr_err("read blk1 failed.\n");
                return -1;
            }
            t->single_frame_transed_siz++;
        }

        if (t->single_frame_transed_siz == 2) {
            ret = uart_proto_gets(uart_proto_id, &pframe[2], 1);
            if (ret == 0) {
                pr_err("read blk2 failed.\n");
                return -1;
            }
            t->single_frame_transed_siz++;
        }

        if (t->single_frame_transed_siz == 3) {
            ret = uart_proto_gets(uart_proto_id, &pframe[3], 1);
            if (ret == 0) {
                pr_err("read frm_len failed.\n");
                return -1;
            }
            t->single_frame_transed_siz++;
            if (pframe[0] == SOH) {
                t->single_frame_trans_siz = pframe[3] + 6;
            } else if (pframe[0] == STX) {
                t->single_frame_trans_siz = LNG_FRM_DLEN + 5;
            }
        }

        rest = t->single_frame_trans_siz - t->single_frame_transed_siz;
        ret = uart_proto_gets(uart_proto_id, &pframe[t->single_frame_transed_siz], rest);
        if (ret != rest) {
            t->single_frame_transed_siz += ret;
            delta = aic_get_time_ms() - uart_proto.last_recv_time;
            if (delta > 1000) {
                pr_warn("Long time no data, re-recv it ...\n");
                update_last_recv_time();
                t->single_frame_trans_siz = 0;
                t->single_frame_transed_siz = 0;
                uart_proto_putchar(uart_proto_id, NAK);
            }
            return -1;
        } else {
            t->single_frame_transed_siz += ret;
            ret = read_frame_data(pframe, t->single_frame_transed_siz);
            if (ret > 0) {
                pr_debug("Recv %d\n", ret);
                p = t->data + t->xfer_len;
                if (pframe[0] == SOH)
                    memcpy(p, &pframe[4], ret);
                else if (pframe[0] == STX)
                    memcpy(p, &pframe[3], ret);
                t->xfer_len += ret;
                uart_proto_putchar(uart_proto_id, ACK);
            } else if (ret == 0) {
                uart_proto_putchar(uart_proto_id, ACK);
            } else {
                t->single_frame_trans_siz = 0;
                t->single_frame_transed_siz = 0;
                uart_proto_putchar(uart_proto_id, NAK);
                break;
            }
            t->single_frame_trans_siz = 0;
            t->single_frame_transed_siz = 0;
            memset(pframe, 0, LNG_FRM_MAX_LEN);
            pr_debug("Ack frame\n");
        }

        if (t->xfer_len >= t->data_len) {
            pr_debug("Recv in data len %d\n", t->xfer_len);
            uart_proto_layer_set_status(UPG_UART_DATA_RECV_DONE);
            t->status = UPG_UART_CMD_RECV;
            uart_proto_del_task();
            return -1;
        }
        break;
    case UPG_UART_CMD_RECV:
        ret = uart_proto_gets(uart_proto_id, &ch, 1);
        if (ret == 0) {
            delta = aic_get_time_ms() - uart_proto.last_recv_time;
            if (delta > SIG_A_INTERVAL_MS) {
                update_last_recv_time();
                pr_debug("Wait host connect ...\n");
            }
            return -1;
        }

        if (ch == SOH || ch == STX) {
            pframe[0] = ch;
            t->status = UPG_UART_DATA_RECV;
            t->single_frame_transed_siz++;
        } else if (ch == SIG_C) {
            uart_proto_putchar(uart_proto_id, ACK);
            pr_debug("Ack SIG_C\n");
            break;
        } else if (ch == DC1_SEND) {
            uart_proto_putchar(uart_proto_id, ACK);
            t->direction = DIR_HOST_SEND;
            t->status = UPG_UART_DATA_RECV;
            memset(pframe, 0, LNG_FRM_MAX_LEN);
            pr_debug("Host switch to send mode.\n");
            break;
        } else if (ch == DC2_RECV) {
            uart_proto_putchar(uart_proto_id, ACK);
            t->direction = DIR_HOST_RECV;
            t->status = UPG_UART_DATA_SEND;
            pr_debug("Host switch to recv mode.\n");
            //csi_coret_config(drv_get_sys_freq() / 200, CORET_IRQn);
            break;
        }
        break;
    }

    return 0;
}

static int uart_proto_layer_host_recv_proc(struct proto_task *t, u8 *pframe)
{
    int ret, rest, slice, flen;
    u8 ch = 0, *p;
    u64 delta = 0;

    switch (t->status) {
    case UPG_UART_DATA_SEND:
        p = t->data + t->xfer_len;
        rest = t->data_len - t->xfer_len;
        if (rest > 0) {
            if (rest > LNG_FRM_DLEN)
                slice = LNG_FRM_DLEN;
            else if (rest > SHT_FRM_DLEN)
                slice = SHT_FRM_DLEN;
            else
                slice = rest;
            t->single_frame_transed_siz = slice;
            flen = pack_frame(uart_proto.send_blk_no, pframe, p, slice);
            ret = uart_proto_puts(uart_proto_id, pframe, flen);
            if (ret != flen) {
                pr_err("write frame data error. ret:%d\n", ret);
                return -1;
            }
            pr_debug("frame data is written.\n");
        }
        t->status = UPG_UART_CMD_RECV;
        break;
    case UPG_UART_CMD_RECV:
        ret = uart_proto_gets(uart_proto_id, &ch, 1);
        if (ret == 0) {
            delta = aic_get_time_ms() - uart_proto.last_recv_time;
            if (delta > 20) {
                update_last_recv_time();
                t->status = UPG_UART_DATA_SEND;
                pr_debug("Timeout retry ...\n");
                break;
            }
            return -1;
        }

        if (ch == SIG_C) {
            uart_proto_putchar(uart_proto_id, ACK);
            pr_debug("Ack SIG_C\n");
            break;
        } else if (ch == DC1_SEND) {
            uart_proto_putchar(uart_proto_id, ACK);
            t->direction = DIR_HOST_SEND;
            t->status = UPG_UART_DATA_RECV;
            memset(pframe, 0, LNG_FRM_MAX_LEN);
            pr_debug("Host switch to send mode.\n");
            break;
        } else if (ch == DC2_RECV) {
            uart_proto_putchar(uart_proto_id, ACK);
            t->direction = DIR_HOST_RECV;
            t->status = UPG_UART_DATA_SEND;
            pr_debug("Host switch to recv mode.\n");
            //csi_coret_config(drv_get_sys_freq() / 200, CORET_IRQn);
            break;
        } else if (ch == NAK) {
            t->status = UPG_UART_DATA_SEND;
            break;
        } else if (ch == ACK) {
            t->xfer_len += t->single_frame_transed_siz;
            uart_proto.send_blk_no++;
            t->status = UPG_UART_DATA_SEND;
        }

        if (t->xfer_len >= t->data_len) {
            pr_debug("Send out data len %d\n", t->xfer_len);
            uart_proto_layer_set_status(UPG_UART_DATA_SEND_DONE);
            t->status = UPG_UART_CMD_RECV;
            uart_proto_del_task();
            return -1;
        }
        break;
    }

    return 0;
}

static int uart_proto_layer_proc(void)
{
    struct proto_task *t = NULL;
    int ret;
    u8  *pframe;

    t = uart_proto_get_task();
    if (!t)
        return 0;

    pframe = g_uart_send_recv_buf;
    while (1) {
        switch (t->direction) {
        case DIR_HOST_SEND:
            ret = uart_proto_layer_host_send_proc(t, pframe);
            if (ret) {
                return -1;
            }
            break;
        case DIR_HOST_RECV:
            ret = uart_proto_layer_host_recv_proc(t, pframe);
            if (ret) {
                return -1;
            }
            break;
        }
    }

    return 0;
}

int uart_proto_start_read(u8 *data, u32 data_len)
{
    return uart_proto_add_task(DIR_HOST_SEND, data, data_len);
}

int uart_proto_start_write(u8 *data, u32 data_len)
{
    return uart_proto_add_task(DIR_HOST_RECV, data, data_len);
}

int uart_proto_layer_get_status(void)
{
    u8 status = 0;

    ringbuf_out(&(uart_proto.status_fifo.rb), &status, 1);

    return status;
}

void uart_proto_layer_set_status(int status)
{
    ringbuf_in(&(uart_proto.status_fifo.rb), &status, 1);
}

irqreturn_t uart_proto_layer_irqhandler(int irq, void *data)
{
    csi_coret_config(drv_get_sys_freq() / 5000, CORET_IRQn);
    uart_proto_layer_proc();

    return 0;
}

int uart_proto_layer_init(int id)
{
    uart_proto_id = id;

	INIT_LIST_HEAD(&task);

    memset(&uart_proto, 0, sizeof(uart_proto));
    uart_proto.first_connect = 1;

    ringbuf_init(&(uart_proto.status_fifo.rb), uart_proto.status_fifo.buffer, 4);

    //10ms
    csi_coret_config(drv_get_sys_freq() / 1000, CORET_IRQn);
    aicos_request_irq(CORET_IRQn, uart_proto_layer_irqhandler, 0, NULL, NULL);
    aicos_irq_set_prio(CORET_IRQn, 8);
    aicos_irq_enable(CORET_IRQn);

    /* Send CAN first */
    uart_proto_putchar(uart_proto_id, CAN);

    return 0;
}

int uart_proto_layer_deinit()
{
    aicos_irq_disable(CORET_IRQn);
    return 0;
}
