/************************************************************
 * 文件名: ADC.h
 * 功能:   ADC0 点位采样驱动 (PC0=IN10, PC1=IN11)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __ADC_H
#define __ADC_H

#include "HeaderFiles.h"

void adc_init(void); /* 初始化 ADC0 */
uint16_t adc_read_raw(uint8_t channel);  /* 读指定通道12位值 */

#endif
/****************************End*****************************/
