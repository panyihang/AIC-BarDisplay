/*
 * Copyright (c) 2023-2025, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AIC_IOPOLL_H__
#define __AIC_IOPOLL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aic_common.h"
#include "aic_core.h"

#define time_after(a,b)     \
    ((int)((b) - (a)) < 0)

/**
 * read_poll_timeout - Periodically poll an address until a condition is met or a timeout occurs
 * @op: accessor function (takes @addr as its only argument)
 * @addr: Address to poll
 * @val: Variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @sleep_us: Time to sleep in us, 0 means never sleep, Minimum sleep time is 1 ms (1000 us)
 * @timeout_us: Timeout in us, 0 means never timeout
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @addr is stored in @val.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 */
#define read_poll_timeout(op, addr, val, cond, sleep_us, timeout_us)    \
({ \
    u64 timeout = aic_get_time_us() + timeout_us; \
    for (;;) { \
        (val) = op(addr); \
        if (cond) \
            break; \
        if (timeout_us && time_after(aic_get_time_us(), timeout)) { \
            (val) = op(addr); \
            break; \
        } \
        if (sleep_us) \
            aicos_msleep(sleep_us < 1000 ? 1 : sleep_us / 1000); \
    } \
    (cond) ? 0 : -ETIMEDOUT; \
})

#define readx_poll_sleep_timeout(op, addr, val, cond, sleep_us, timeout_us) \
    read_poll_timeout(op, addr, val, cond, sleep_us, timeout_us)

#define readl_poll_sleep_timeout(addr, val, cond, sleep_us, timeout_us) \
    readx_poll_sleep_timeout(readl, addr, val, cond, sleep_us, timeout_us)

#define readx_poll_timeout(op, addr, val, cond, timeout_us) \
    read_poll_timeout(op, addr, val, cond, false, timeout_us)

#define readb_poll_timeout(addr, val, cond, timeout_us) \
    readx_poll_timeout(readb, addr, val, cond, timeout_us)

#define readl_poll_timeout(addr, val, cond, timeout_us) \
    readx_poll_timeout(readl, addr, val, cond, timeout_us)

#ifdef __cplusplus
}
#endif

#endif /* __AIC_IOPOLL_H__ */
