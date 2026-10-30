/************************************************************
 * 版权：2025CIMC Copyright
 * 文件：RTC.h
 * 作者: Qiao Qin @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Qiao Qin     2025/4/20     V0.01    original
************************************************************/

#ifndef __RTC_H
#define __RTC_H
#include "HeaderFiles.h"

void RTC_Init(void);                          /* RTC初始化 */
uint8_t rtc_setup(uint32_t tmp_year, uint32_t tmp_month, uint32_t tmp_day,
                  uint32_t tmp_hh, uint32_t tmp_mm, uint32_t tmp_ss);  /* 设置完整日期时间 */
void    rtc_pre_config(void);                    /* RTC时钟源（LXTAL）预配置 */
uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec);    /* 仅设置时分秒 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date); /* 仅设置年月日 */
void mcu_deepsleep_with_rtc(uint32_t sec);   /* 深度睡眠 sec 秒后RTC闹钟唤醒 */

#endif
