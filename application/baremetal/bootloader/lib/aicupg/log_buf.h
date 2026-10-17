/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#ifndef _LOG_BUF_H_
#define _LOG_BUF_H_

int log_buf_init(void);
int log_buf_get_len(void);
int log_buf_write(char *in, int len);
int log_buf_read(char *out, int len);

#endif
