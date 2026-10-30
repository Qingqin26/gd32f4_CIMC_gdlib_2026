/************************************************************
 * 文件名: protocol.h
 * 描述:   Bootloader 通信协议层
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "HeaderFiles.h"
#include "proto_cmd.h"
#include "proto_frame.h"

void protocol_init(void);

uint8_t protocol_feed_cmd(const uint8_t *data, uint16_t len, uint16_t *out_cmd);

uint16_t protocol_build_ack(uint16_t cmd, uint8_t ok, uint8_t *resp, uint16_t cap);

/* 当前设备ID */
uint16_t protocol_dev_id(void);

/* 读取持久化波特率 */
uint32_t protocol_saved_baudrate(void);

#endif
/****************************End*****************************/
