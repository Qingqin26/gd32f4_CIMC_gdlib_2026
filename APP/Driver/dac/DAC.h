/************************************************************
 * 文件名: dac.h
 * 功能:   DAC0 输出驱动 (PA4 = DAC0_OUT0)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __DAC_H
#define __DAC_H

#include "gd32f4xx.h"

/* DAC0_OUT0 输出引脚 */
#define DAC_PORT        GPIOA
#define DAC_RCU         RCU_GPIOA
#define DAC_PIN         GPIO_PIN_4

void dac_init(void);   /* 初始化 DAC0 (PA4 模拟输出) */
void dac_set_raw(uint16_t value);    /* 设置12位DAC值 (0~4095, 立即生效) */

#endif
/****************************End*****************************/
