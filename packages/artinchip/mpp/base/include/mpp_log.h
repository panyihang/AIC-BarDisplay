/*
 * Copyright (c) 2022-2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <qi.xu@artinchip.com>
 * Desc: log module
 */

#ifndef MPP_LOG_H
#define MPP_LOG_H

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

enum log_level {
	MPP_LOGL_ERROR = 0,
	MPP_LOGL_WARNING,
	MPP_LOGL_INFO,
	MPP_LOGL_DEBUG,
	MPP_LOGL_VERBOSE,
	MPP_LOGL_COUNT,

	MPP_LOGL_DEFAULT = 	MPP_LOGL_ERROR,
	MPP_LOGL_FORCE_DEBUG = 0x10,
};

#ifdef LOG_DEBUG
#define _LOG_DEBUG	MPP_LOGL_FORCE_DEBUG
#else
#define _LOG_DEBUG	0
#endif

/*avoid  redefine warning */
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "aic_mpp"
#else
#define LOG_TAG "aic_mpp"
#endif

#define TAG_ERROR	"error  "
#define TAG_WARNING	"warning"
#define TAG_INFO	"info   "
#define TAG_DEBUG	"debug  "
#define TAG_VERBOSE	"verbose"

#define mpp_log(level, tag, fmt, arg...) ({ \
	int _l = level; \
	if (((_LOG_DEBUG != 0) && (_l <= MPP_LOGL_DEBUG)) || \
	    (_l <= MPP_LOGL_DEFAULT)) \
		printf("%s: %s <%s:%d>: "fmt"\n", tag, LOG_TAG, __FUNCTION__, __LINE__, ##arg); \
	})



#define loge(fmt, arg...) mpp_log(MPP_LOGL_ERROR, TAG_ERROR, "\033[40;31m"fmt"\033[0m", ##arg)
#define logw(fmt, arg...) mpp_log(MPP_LOGL_WARNING, TAG_WARNING, "\033[40;33m"fmt"\033[0m", ##arg)
#define logi(fmt, arg...) mpp_log(MPP_LOGL_INFO, TAG_INFO, "\033[40;32m"fmt"\033[0m", ##arg)
#define logd(fmt, arg...) mpp_log(MPP_LOGL_DEBUG, TAG_DEBUG, fmt, ##arg)
#define logv(fmt, arg...) mpp_log(MPP_LOGL_VERBOSE, TAG_VERBOSE, fmt, ##arg)

#define mpp_assert(cond) do { \
	if (!(cond)) { \
		loge("Assertion failed!"); \
	} \
} while (0)

#define time_start(tag) unsigned int time_##tag##_start = aic_get_time_us()
#define time_end(tag) unsigned int time_##tag##_end = aic_get_time_us();\
			fprintf(stderr, #tag " time: %u us\n",\
			time_##tag##_end - time_##tag##_start)

#define MPP_ABS(x,y) ((x>y)?(x-y):(y-x))
#endif














