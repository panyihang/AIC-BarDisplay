/*
 * Copyright (c) 2022-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_DRV_IRQ_H_
#define _AIC_DRV_IRQ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef AIC_GPIO_IRQ_DRV_EN
#define MAX_IRQ_ENTRY (MAX_IRQn+GPIO_MAX_PIN)
#else
#define MAX_IRQ_ENTRY (MAX_IRQn)
#endif

#define AIC_GPIO_TO_IRQ(GPIOn)(MAX_IRQn + (GPIOn))
#define AIC_IRQ_TO_GPIO(IRQn)((IRQn) - MAX_IRQn)

void drv_irq_call_isr(uint32_t irq_num);

/**
  \brief       enable irq.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
void drv_irq_enable(uint32_t irq_num);

/**
  \brief       disable irq.
  \param[in]   irq_num Number of IRQ.
  \return      None.
*/
void drv_irq_disable(uint32_t irq_num);

/**
  \brief       register irq handler.
  \param[in]   irq_num Number of IRQ.
  \param[in]   irq_handler IRQ Handler.
  \return      None.
*/
void drv_irq_register(uint32_t irq_num, void *irq_handler, void *data);

/**
  \brief       unregister irq handler.
  \param[in]   irq_num Number of IRQ.
  \param[in]   irq_handler IRQ Handler.
  \return      None.
*/
void drv_irq_unregister(uint32_t irq_num);


#ifdef __cplusplus
}
#endif

#endif /* _AIC_DRV_IRQ_H_ */

