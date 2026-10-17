/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-07     aozima       the first version
 * 2018-11-16     Ernest Chen  add finsh command and update adc function
 * 2022-05-11     Stanley Lwin add finsh voltage conversion command
 */

#ifndef __ADC_H__
#define __ADC_H__
#include <rtthread.h>

struct rt_adc_device;
struct rt_adc_ops
{
    rt_err_t (*enabled)(struct rt_adc_device *device, rt_uint32_t channel, rt_bool_t enabled);
    rt_err_t (*convert)(struct rt_adc_device *device, rt_uint32_t channel, rt_uint32_t *value);
    rt_err_t (*active_dma)(struct rt_adc_device *device, void *arg);
    rt_err_t (*config_dma)(struct rt_adc_device *device, void *dma_info);
#ifdef AIC_GPAI_DRV
    rt_err_t (*stop_dma)(struct rt_adc_device *device, rt_uint32_t channel);
    rt_err_t (*get_mode)(struct rt_adc_device *device, void *chan_info);
    rt_uint32_t (*get_obtaining_data_mode)(struct rt_adc_device *device,
                                           rt_uint32_t channel);
    rt_uint32_t (*get_irq_count)(struct rt_adc_device *device,
                                 rt_uint32_t channel);
    rt_err_t (*irq_callback)(struct rt_adc_device *device, void *dma_info);
    rt_err_t (*get_ch_info)(struct rt_adc_device *device, void *chan_info);
#endif
#ifdef AIC_PSADC_DRV
    rt_err_t (*get_adc_values_poll)(struct rt_adc_device *device,
                                    void *values);
    rt_err_t (*get_adc_values)(struct rt_adc_device *device,
                                void *values);
    rt_uint32_t (*get_chan_count)(struct rt_adc_device *device);
#endif
    rt_uint8_t (*get_resolution)(struct rt_adc_device *device);
    rt_int16_t (*get_vref) (struct rt_adc_device *device);
};

struct rt_adc_device
{
    struct rt_device parent;
    const struct rt_adc_ops *ops;
};
typedef struct rt_adc_device *rt_adc_device_t;

typedef enum
{
    RT_ADC_CMD_ENABLE = RT_DEVICE_CTRL_BASE(ADC) + 1,
    RT_ADC_CMD_DISABLE = RT_DEVICE_CTRL_BASE(ADC) + 2,
    RT_ADC_CMD_GET_RESOLUTION = RT_DEVICE_CTRL_BASE(ADC) + 3, /* get the resolution in bits */
    RT_ADC_CMD_GET_VREF = RT_DEVICE_CTRL_BASE(ADC) + 4, /* get reference voltage */
    RT_ADC_CMD_ACTIVE_DMA = RT_DEVICE_CTRL_BASE(ADC) + 6,
    RT_ADC_CMD_CONFIG_DMA = RT_DEVICE_CTRL_BASE(ADC) + 7,
#ifdef AIC_GPAI_DRV
    RT_ADC_CMD_IRQ_COUNT = RT_DEVICE_CTRL_BASE(ADC) + 5,
    RT_ADC_CMD_GET_DMA_DATA = RT_ADC_CMD_ACTIVE_DMA,
    RT_ADC_CMD_OBTAIN_DATA_MODE = RT_DEVICE_CTRL_BASE(ADC) + 8,
    RT_ADC_CMD_IRQ_CALLBACK = RT_DEVICE_CTRL_BASE(ADC) + 9,
    RT_ADC_CMD_GET_CH_INFO = RT_DEVICE_CTRL_BASE(ADC) + 10,
    RT_ADC_CMD_GET_MODE = RT_DEVICE_CTRL_BASE(ADC) + 11,
    RT_ADC_CMD_STOP_DMA = RT_DEVICE_CTRL_BASE(ADC) + 12,
#endif
#ifdef AIC_PSADC_DRV
    RT_ADC_CMD_GET_VALUES = RT_DEVICE_CTRL_BASE(ADC) + 13,
    RT_ADC_CMD_GET_CHAN_COUNT = RT_DEVICE_CTRL_BASE(ADC) + 14,
    RT_ADC_CMD_GET_VALUES_POLL = RT_DEVICE_CTRL_BASE(ADC) + 15,
#endif
} rt_adc_cmd_t;

rt_err_t rt_hw_adc_register(rt_adc_device_t adc,const char *name, const struct rt_adc_ops *ops, const void *user_data);

rt_uint32_t rt_adc_read(rt_adc_device_t dev, rt_uint32_t channel);
rt_err_t rt_adc_enable(rt_adc_device_t dev, rt_uint32_t channel);
rt_err_t rt_adc_disable(rt_adc_device_t dev, rt_uint32_t channel);
rt_int16_t rt_adc_voltage(rt_adc_device_t dev, rt_uint32_t channel);
#if defined(AIC_GPAI_DRV) || defined(AIC_PSADC_DRV)
rt_err_t rt_adc_control(rt_adc_device_t dev, int cmd, void *args);
#endif

#endif /* __ADC_H__ */
