/************************************************************
 * 文件名: dac_app.h
 * 功能:   DAC 应用层 (电压 -> 12位值)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __DAC_APP_H
#define __DAC_APP_H

#include "dac.h"

void dac_app_set_voltage(float volt);   /* 设置DAC输出电压 (0~Vref, 自动钳位) */
void dac_app_set_raw(uint16_t raw);     /* 直接设置DAC 12位原始值 (0~4095, 超出钳位) */

#endif
/****************************End*****************************/
