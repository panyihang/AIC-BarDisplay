/*
 * Copyright (C) 2023-2024 ArtInChip Technology Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Dehuang Wu <dehuang.wu@artinchip.com>
 */
#include <rtconfig.h>
#define LREG			lw
#define SREG			sw
#define REGBYTES		4

.extern	boot_params_stash
#ifndef AIC_BOOTLOADER
.extern boot_arg
#endif

.global save_boot_params
.type   save_boot_params, %function
save_boot_params:
	la	t0, boot_params_stash
	SREG	a0, REGBYTES * 0(t0)
	SREG	a1, REGBYTES * 1(t0)
	SREG	a2, REGBYTES * 2(t0)
	SREG	a3, REGBYTES * 3(t0)
	SREG	a4, REGBYTES * 4(t0)
	SREG	a5, REGBYTES * 5(t0)
	SREG	a6, REGBYTES * 6(t0)
	SREG	a7, REGBYTES * 7(t0)
	SREG	s0, REGBYTES * 8(t0)
	SREG	s1, REGBYTES * 9(t0)
	SREG	s2, REGBYTES * 10(t0)
	SREG	s3, REGBYTES * 11(t0)
	SREG	s4, REGBYTES * 12(t0)
	SREG	s5, REGBYTES * 13(t0)
	SREG	s6, REGBYTES * 14(t0)
	SREG	s7, REGBYTES * 15(t0)
	SREG	s8, REGBYTES * 16(t0)
	SREG	s9, REGBYTES * 17(t0)
	SREG	s10, REGBYTES * 18(t0)
	SREG	s11, REGBYTES * 19(t0)
	SREG	sp, REGBYTES * 20(t0)
	SREG	ra, REGBYTES * 21(t0)
#ifndef AIC_BOOTLOADER
	beqz	a1, 2f
	/* copy boot_arg from SPL to OS */
	la  	t0, boot_arg
	mv  	t1, a1
	/* boot_arg size is 256 bytes */
	addi 	t2, t1, 0xFF
1:
	LREG  	t3, (t1)
	SREG  	t3, (t0)
	addi 	t0, t0, REGBYTES
	addi 	t1, t1, REGBYTES
	bltu 	t1, t2, 1b
2:
#endif
	j	save_boot_params_ret

