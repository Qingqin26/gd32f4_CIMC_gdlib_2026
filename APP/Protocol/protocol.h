/************************************************************
 * 文件名: protocol.h
 * 功能:   通信协议层 (RS485, ASCII-hex 帧)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "HeaderFiles.h"

/* 协议层初始化 */
void     protocol_init(void);

/* RS485 原始字节流*/
uint16_t protocol_rs485_feed(const uint8_t *data, uint16_t len,
                             uint8_t *resp, uint16_t resp_max);

/* 设备重启轮询 */
void     protocol_reboot_poll(void);

/* 波特率延迟切换 */
void     protocol_baud_poll(void);

/* 构造本机心跳帧(0x8888)到 resp(ASCII-hex),返回长度; 供上电后主动发送一次 */
uint16_t protocol_build_heartbeat(uint8_t *resp, uint16_t resp_max);

/* 定时自动上报 */
uint16_t protocol_report_poll(uint8_t *resp, uint16_t resp_max);

/* 查询定时自动上报是否启动 */
uint8_t  protocol_report_is_active(void);

/* 睡眠 */
void     protocol_sleep_poll(void);

/* 告警 */
uint16_t protocol_alarm_poll(uint8_t *resp, uint16_t resp_max);

#endif
/****************************End*****************************/
