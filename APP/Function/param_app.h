/************************************************************
 * 文件名: param_app.h
 * 功能:   设备参数持久化管理（内部flash）
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#ifndef __PARAM_APP_H
#define __PARAM_APP_H

#include "HeaderFiles.h"

/* 参数区地址 ( MCU内部Flash划分: 0x08010000~0x08010FFF, 4KB) */
#define PARAM_FLASH_ADDR     0x08010000U
#define PARAM_MAGIC          0x43494D43U
#define PARAM_VERSION        0x0003U
#define PARAM_ALARM_MAX      10U

/* 单条告警记录 */
typedef struct {
    uint16_t year;       /* 触发时间: 年 */
    uint8_t  month;      /* 月 */
    uint8_t  day;        /* 日 */
    uint8_t  hour;       /* 时 */
    uint8_t  minute;     /* 分 */
    uint8_t  second;     /* 秒 */
    uint8_t  channel;    /* 告警通道 (0=CH0, 1=CH1) */
    float    threshold;  /* 告警时阈值 */
    float    value;      /* 触发值 */
} alarm_record_t;

/* 设备参数结构体 (整体写入参数页; 字段按4字节自然对齐排列) */
typedef struct {
    uint32_t magic;                          /* 魔数 PARAM_MAGIC, 判断是否已初始化 */
    uint16_t version;                        /* 结构版本 */
    uint16_t device_id;                      /* 设备ID */
    float    ratio_ch0;                      /* CH0 变比 */
    float    ratio_ch1;                      /* CH1 变比 */
    float    threshold_ch0;                  /* CH0 阈值 */
    float    threshold_ch1;                  /* CH1 阈值 */
    uint32_t baudrate;                       /* RS485 波特率 */
    uint32_t report_interval_s;              /* 数据上报间隔 */
    uint32_t alarm_report_en;                /* 告警主动上报开关 */
    uint32_t alarm_count;                    /* 已存告警条数 */
    alarm_record_t alarms[PARAM_ALARM_MAX];  /* 告警记录 */
    uint32_t crc;                            /* CRC16-Modbus */
} device_param_t;

extern device_param_t g_param;

/* 上电初始化: 从 Flash 读入并校验 */
void param_app_init(void);
/* 载入出厂默认值到 g_param */
void param_app_load_default(void);
/* 将 g_param 写回内部 Flash 参数页 */
uint8_t param_app_save(void);

#endif
/****************************End*****************************/
