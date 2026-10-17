/*
 * Copyright (c) 2023, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <aic_core.h>
#include <aic_common.h>
#include <aic_errno.h>
#include <log_buf.h>

#define AIC_LOG_MAGIC 0x4C434941

struct log_buffer {
	int magic;
	int istart;
	int icurr;
	int buf_size;
	char buf[];
};

#ifdef AICUPG_LOG_BUFFER_SUPPORT
#define LOG_BUF_ADDR AICUPG_LOG_BUFFER_ADDR
#define LOG_BUF_SIZE AICUPG_LOG_BUFFER_SIZE
#else
#define LOG_BUF_ADDR 0
#define LOG_BUF_SIZE 0
#endif
int log_buf_init(void)
{
#ifdef AICUPG_LOG_BUFFER_SUPPORT
	struct log_buffer *log;
	void *buff;
	int size;

	buff = (void *)LOG_BUF_ADDR;
	size = (int)LOG_BUF_SIZE;

	if (!buff || (size < sizeof(*log)))
		return -1;

	log = buff;
	if (log->magic == AIC_LOG_MAGIC)
		return 0;

	log->istart = 0;
	log->icurr = 0;
	log->buf_size = size - sizeof(*log);
	log->magic = AIC_LOG_MAGIC;
#endif
	return 0;
}

int log_buf_get_len(void)
{
#ifdef AICUPG_LOG_BUFFER_SUPPORT
	struct log_buffer *log = (void *)LOG_BUF_ADDR;

	if (!log)
		return 0;

	return (log->icurr + log->buf_size - log->istart) % log->buf_size;
#endif
	return 0;
}

int log_buf_write(char *in, int len)
{
#ifdef AICUPG_LOG_BUFFER_SUPPORT
	struct log_buffer *log = (void *)LOG_BUF_ADDR;
	int dolen, freelen;
	char *ps;

	if (!log)
		return 0;
	if (log->magic != AIC_LOG_MAGIC)
        return 0;

	dolen = len;
	freelen = log->buf_size - 1 - log_buf_get_len();
	if (freelen == 0)
		return 0;

	if (dolen > freelen)
		dolen = freelen;
	len = dolen;
	ps = in;
	while (dolen) {
		log->buf[log->icurr % log->buf_size] = *ps;
		log->icurr = (log->icurr + 1) % log->buf_size;
		dolen--;
		ps++;
	}
	return len;
#endif
	return 0;
}

int log_buf_read(char *out, int len)
{
#ifdef AICUPG_LOG_BUFFER_SUPPORT
	struct log_buffer *log = (void *)LOG_BUF_ADDR;
	int dolen, loglen;
	char *pd;

	if (!log)
		return 0;
	if (log->magic != AIC_LOG_MAGIC)
        return 0;

	dolen = len;
	loglen = log_buf_get_len();
	if (dolen > loglen)
		dolen = loglen;
	len = dolen;
	pd = out;
	while (dolen) {
		*pd = log->buf[log->istart % log->buf_size];
		log->istart = (log->istart + 1) % log->buf_size;
		dolen--;
		pd++;
	}
	return len;
#endif
	return 0;
}
