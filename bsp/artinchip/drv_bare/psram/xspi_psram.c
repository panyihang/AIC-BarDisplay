/*
 *
 * Copyright (c) 2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: Mingfeng.Li <mingfeng.li@artinchip.com>
 */

#include <rtconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <aic_common.h>
#include <string.h>
#include <aic_core.h>
#include <aic_hal.h>
#include <driver.h>
#include <xspi_psram.h>
#include <hal_xspi.h>
#include "aic_core.h"

#define XSPI_DDR_MODE 1
#define XSPI_SDR_MODE 0

#define XSPI_MODE_XCCELA   0
#define XSPI_MODE_HYPERBUS 1
#define XSPI_MODE_OPI      2
#define XSPI_MODE_SPI      3

#define XSPI_IO_1 0x0
#define XSPI_IO_2 0x1
#define XSPI_IO_4 0x2
#define XSPI_IO_8 0x3

#define XSPI_PARALLEL_MODE 1
#define XSPI_SINGLE_MODE   0

#define BASE_PSRAM 0x40000000

// usually config by menuconfig
#ifndef AIC_XSPI_PSRAM_CLK
#define AIC_XSPI_PSRAM_CLK 99000000
#endif
#ifndef AIC_XSPI_PSRAM_CS0_PINS
#define AIC_XSPI_PSRAM_CS0_PINS 0
#endif
#ifndef AIC_XSPI_PSRAM_CS1_PINS
#define AIC_XSPI_PSRAM_CS1_PINS 0
#endif

static struct aic_xspi aic_xspi_controller[] = {
    {
        .name = "xspi",
        .idx = 0,
        .clk_id = CLK_XSPI,
        /* XSPI IP hw Frequency division=2 */
        .clk_in_hz = AIC_XSPI_PSRAM_CLK * 2,
    },
};

static struct aic_xspi *aic_get_xspi_by_index(u32 idx)
{
    struct aic_xspi *xspi;
    u32 i;

    xspi = NULL;
    for (i = 0; i < ARRAY_SIZE(aic_xspi_controller); i++) {
        if (i == idx) {
            xspi = &aic_xspi_controller[i];
            break;
        }
    }
    return xspi;
}

static u32 aic_xspi_psram_dev_reset(hal_xspi_handle *handle)
{
    hal_xspi_set_cmd_width(handle, XSPI_DDR_MODE, XSPI_IO_8);
    hal_xspi_set_cmd(handle, XSPI_DDR_MODE, 0xff);
    hal_xspi_set_addr_width(handle, XSPI_DDR_MODE, XSPI_IO_8, 3);
    hal_xspi_set_addr(handle, 0x0);
    hal_xspi_set_dummy(handle, XSPI_IO_8, 0x0);
    hal_xspi_set_write_cnt(handle, XSPI_DDR_MODE, XSPI_IO_8, 1);

    u8 buf = 0;
    struct hal_xspi_transfer t;
    t.rx_data = NULL;
    t.tx_data = &buf;
    t.data_len = 1;
    hal_xspi_transfer_cpu_sync(handle, &t);
    pr_debug("aic_xspi_psram_dev_reset %s:%d\n\n\n", __FUNCTION__, __LINE__);
    return 0;
}

[[maybe_unused]] static u8 aic_xspi_psram_mr4(hal_xspi_handle *handle)
{
    u8 data = 0;

    hal_xspi_set_cmd_width(handle, XSPI_DDR_MODE, XSPI_IO_8);
    hal_xspi_set_cmd(handle, XSPI_DDR_MODE, 0x40);
    hal_xspi_set_addr_width(handle, XSPI_DDR_MODE, XSPI_IO_8, 3);
    hal_xspi_set_addr(handle, 0x04);
    hal_xspi_set_dummy(handle, XSPI_IO_8, 4);
    hal_xspi_set_read_cnt(handle, XSPI_DDR_MODE, XSPI_IO_8, 1);

    struct hal_xspi_transfer t3;
    t3.rx_data = (u8 *)&data;
    t3.tx_data = NULL;
    t3.data_len = 1;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("MA4(0x04) read: 0x%x \n", data);

    return data;
}

static u32 aic_xspi_psram_dev_init(hal_xspi_handle *handle)
{
    u8 mr4 = aic_xspi_psram_mr4(handle);
    hal_xspi_set_cmd_width(handle, XSPI_DDR_MODE, XSPI_IO_8);
    hal_xspi_set_cmd(handle, XSPI_DDR_MODE, 0xc0);
    hal_xspi_set_addr_width(handle, XSPI_DDR_MODE, XSPI_IO_8, 3);
    hal_xspi_set_addr(handle, 0x0);

    mr4 &= (1 << 7);
    if (mr4 == 0) {
        hal_xspi_set_dummy(handle, XSPI_IO_8, 0x0);
    } else {
        hal_xspi_set_dummy(handle, XSPI_IO_8, 0x2);
    }

    hal_xspi_set_write_cnt(handle, XSPI_DDR_MODE, XSPI_IO_8, 1);
    u8 buf = 0x19;
    struct hal_xspi_transfer t2;
    t2.rx_data = NULL;
    t2.tx_data = (u8 *)&buf;
    t2.data_len = 1;
    hal_xspi_transfer_cpu_sync(handle, &t2);

    hal_xspi_set_addr(handle, 0x4);
    buf = 0x80;
    t2.rx_data = NULL;
    t2.tx_data = (u8 *)&buf;
    t2.data_len = 1;
    hal_xspi_transfer_cpu_sync(handle, &t2);

    return 0;
}

[[maybe_unused]] static u32 aic_xspi_psram_read_id(hal_xspi_handle *handle)
{
    u16 data = 0;

    hal_xspi_set_cmd_width(handle, XSPI_DDR_MODE, XSPI_IO_8);
    hal_xspi_set_cmd(handle, XSPI_DDR_MODE, 0x40);
    hal_xspi_set_addr_width(handle, XSPI_DDR_MODE, XSPI_IO_8, 3);
    hal_xspi_set_addr(handle, 0x01);
    hal_xspi_set_dummy(handle, XSPI_IO_8, 4);
    hal_xspi_set_read_cnt(handle, XSPI_DDR_MODE, XSPI_IO_8, 2);

    struct hal_xspi_transfer t3;
    t3.rx_data = (u8 *)&data;
    t3.tx_data = NULL;
    t3.data_len = 2;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("APS3208K_ID= 0x%x\n", data);

    return 0;
}

[[maybe_unused]] static u32 aic_xspi_psram_read(hal_xspi_handle *handle)
{
    u8 data[2] = { 0 };

    hal_xspi_set_cmd_width(handle, XSPI_DDR_MODE, XSPI_IO_8);
    hal_xspi_set_cmd(handle, XSPI_DDR_MODE, 0x40);
    hal_xspi_set_addr_width(handle, XSPI_DDR_MODE, XSPI_IO_8, 3);
    hal_xspi_set_addr(handle, 0x00);
    hal_xspi_set_dummy(handle, XSPI_IO_8, 4);
    hal_xspi_set_read_cnt(handle, XSPI_DDR_MODE, XSPI_IO_8, 2);

    struct hal_xspi_transfer t3;
    t3.rx_data = (u8 *)data;
    t3.tx_data = NULL;
    t3.data_len = 2;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("MA(0x00) read: D0=0x%x(MR0), D1=0x%x(MR1), \n", data[0], data[1]);

    hal_xspi_set_addr(handle, 0x01);
    t3.rx_data = (u8 *)data;
    t3.tx_data = NULL;
    t3.data_len = 2;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("MA(0x01) read: D0=0x%x(MR1), D1=0x%x(MR2), \n", data[0], data[1]);

    hal_xspi_set_addr(handle, 0x02);
    t3.rx_data = (u8 *)data;
    t3.tx_data = NULL;
    t3.data_len = 2;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("MA(0x02) read: D0=0x%x(MR2), D1=0x%x(MR4), \n", data[0], data[1]);

    hal_xspi_set_addr(handle, 0x04);
    t3.rx_data = (u8 *)data;
    t3.tx_data = NULL;
    t3.data_len = 2;
    hal_xspi_transfer_cpu_sync(handle, &t3);
    pr_debug("MA(0x04) read: D0=0x%x(MR4), D1=0x%x(MR0), \n", data[0], data[1]);
    return 0;
}

static u32 aic_xspi_psram_xip(hal_xspi_handle *handle,
                              hal_xspi_proto_cfg_t proto)
{
    hal_xspi_xip_cfg(handle, proto);
    hal_xspi_xip_enable(handle);

    return 0;
}

static u8 aic_xspi_psram_mem_test(long address, u32 size)
{
    u32 i;

    /**< 32bit test */
    {
        u32 *p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            *p_u32++ = (u32)i;
        }

        p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            if (*p_u32 != (u32)i) {
                return 1;
            }
            p_u32++;
        }
    }

    /**< 32bit Loopback test */
    {
        u32 *p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            *p_u32 = (long)p_u32;
            p_u32++;
        }

        p_u32 = (u32 *)address;
        for (i = 0; i < size / sizeof(u32); i++) {
            if (*p_u32 != (long)p_u32) {
                return 1;
            }
            p_u32++;
        }
    }
    return 0;
}

u32 aic_xspi_psram_training(hal_xspi_handle *h, u8 sel, u8 reg_icp,
                            void *psram_buf, u32 len)
{
    u8 temp = 0;
    u8 temp1 = 0;
    u8 temp2 = 0;
    u8 ret_memtest = 0;

    CHECK_PARAM(h, -EINVAL);

    hal_xspi_set_cs(h, sel);
    hal_xspi_set_parallel_mode(h, XSPI_SINGLE_MODE);
    hal_xspi_set_dll_ctl(h, sel, reg_icp, 3);

    aicos_dcache_clean();
    for (u8 i = 0; i < 15; i++) {
        hal_xspi_set_phase_sel(h, sel, i);
        aic_udelay(5);
        temp1 = i;
        ret_memtest = aic_xspi_psram_mem_test((long)psram_buf, len);
        if (!ret_memtest)
            break;
    }
    if (temp1 == 15) {
        hal_log_err("Trainning failed...\n");
        return 1;
    }

    aicos_dcache_clean();
    for (u8 i = 14; i >= 0; i--) {
        hal_xspi_set_phase_sel(h, sel, i);
        aic_udelay(5);
        temp2 = i;
        ret_memtest = aic_xspi_psram_mem_test((long)psram_buf, len);
        if (!ret_memtest)
            break;
    }

    temp = (temp1 + temp2) / 2;
    hal_xspi_set_dll_ctl(h, sel, reg_icp, temp);
    pr_err(" %s:%d cs=%d, temp=%d, temp1=%d, temp2=%d...\n", __FUNCTION__,
           __LINE__, sel, temp, temp1, temp2);
    return 0;
}

#define AIC_XSPI_ICP_50_100M  0
#define AIC_XSPI_ICP_100_150M 1
#define AIC_XSPI_ICP_150_200M 2
u32 aic_xspi_psram_icp_calc(u32 clk_in_hz)
{
    if ((clk_in_hz >= 99000000) && (clk_in_hz <= 198000000)) {
        return AIC_XSPI_ICP_50_100M;
    } else if ((clk_in_hz > 198000000) && (clk_in_hz <= 247500000)) {
        return AIC_XSPI_ICP_100_150M;
    } else if ((clk_in_hz > 247500000) && (clk_in_hz <= 396000000)) {
        return AIC_XSPI_ICP_150_200M;
    }
    return AIC_XSPI_ICP_50_100M;
}

u32 aic_xspi_psram_init(void)
{
    u32 ret = 0;

    struct aic_xspi *xspi;
    struct hal_xspi_config cfg;

    xspi = aic_get_xspi_by_index(0);
    if (!xspi)
        return PSRAM_INIT_FAILED;

    if (xspi->inited)
        return PSRAM_INIT_OK;

    memset(&cfg, 0, sizeof(cfg));
    cfg.idx = xspi->idx;
    cfg.clk_in_hz = xspi->clk_in_hz;
    cfg.clk_id = xspi->clk_id;
    cfg.cs0_port = AIC_XSPI_PSRAM_CS0_PINS;
    cfg.cs1_port = AIC_XSPI_PSRAM_CS1_PINS;

    ret = hal_xspi_init(&xspi->handle, &cfg);
    if (ret) {
        pr_err("xspi init failed.\n");
        return PSRAM_INIT_FAILED;
    }

    hal_xspi_set_cs(&xspi->handle, 0);
    aic_xspi_psram_dev_reset(&xspi->handle);
    aic_udelay(100);
    aic_xspi_psram_dev_init(&xspi->handle);

    hal_xspi_set_cs(&xspi->handle, 1);
    aic_xspi_psram_dev_reset(&xspi->handle);
    aic_udelay(100);
    aic_xspi_psram_dev_init(&xspi->handle);

    // set Boundary_Control = 2k.
    hal_xspi_set_boudary(&xspi->handle, BOUNDARY_2K);

    hal_xspi_proto_cfg_t psram_proto_cfg = { 0 };
    psram_proto_cfg.mode = XSPI_MODE_OPI;
    psram_proto_cfg.clk_mode = XSPI_DDR_MODE;
    psram_proto_cfg.parallel_mode = XSPI_SINGLE_MODE;
    psram_proto_cfg.wr_cmd_lines = XSPI_IO_8;
    psram_proto_cfg.wr_cmd_val = 0x80;
    psram_proto_cfg.addr_lines = XSPI_IO_8;
    psram_proto_cfg.addr_width = 3;
    //psram_proto_cfg.wr_dummy = 0;
    psram_proto_cfg.wr_dummy = 2; //  >166M
    psram_proto_cfg.rd_dummy = 0x06;
    psram_proto_cfg.wr_cnt_lines = XSPI_IO_8;
    psram_proto_cfg.wr_cnt = 2;
    psram_proto_cfg.rd_cnt_lines = XSPI_IO_8;
    psram_proto_cfg.rd_cnt = 3;
    psram_proto_cfg.rd_cmd_lines = XSPI_IO_8;
    psram_proto_cfg.rd_cmd_val = 0x00;

    aic_xspi_psram_xip(&xspi->handle, psram_proto_cfg);

    void *psram_training_buf = (u32 *)(BASE_PSRAM);
    u32 len = 1024 * 64;
    u32 icp_t = aic_xspi_psram_icp_calc(cfg.clk_in_hz);

    ret = aic_xspi_psram_training(&xspi->handle, 0, icp_t, psram_training_buf,
                                  len);
    if (ret) {
        pr_err("xspi cs0 trainning failed.\n");
        return PSRAM_INIT_FAILED;
    }

    ret = aic_xspi_psram_training(&xspi->handle, 1, icp_t, psram_training_buf,
                                  len);
    if (ret) {
        pr_err("xspi cs0 trainning failed.\n");
        return PSRAM_INIT_FAILED;
    }

    hal_xspi_set_parallel_mode(&xspi->handle, XSPI_PARALLEL_MODE);

    return PSRAM_INIT_OK;
}
