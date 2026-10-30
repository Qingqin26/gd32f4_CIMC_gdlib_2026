/************************************************************
 * 文件名: rtc_app.h
 * 描述:   RTC 业务层接口 
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __RTC_APP_H
#define __RTC_APP_H

#include "HeaderFiles.h"

/* 读取当前 RTC 时间并格式化为 "Time:20YY-MM-DD HH:MM:SS\r\n" */
void    rtc_app_get_time_str(char *buf);

// 设置 RTC 日期时间 (year 传公历年如2026, 内部自动取后两位)
uint8_t rtc_app_set_time(uint32_t year, uint32_t month, uint32_t day,
                         uint32_t hour, uint32_t minute, uint32_t second);

/* 用UTC秒级时间戳校时 */
uint8_t  rtc_app_set_timestamp(uint32_t ts);

/* 读取当前RTC时间并转为UTC秒级时间戳 */
uint32_t rtc_app_get_timestamp(void);

#endif
/****************************End*****************************/
