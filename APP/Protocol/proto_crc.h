/************************************************************
 * 文件名: proto_crc.h
 * 功能:   CRC-16-Modbus 校验 (init=0xFFFF, poly=0xA001)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTO_CRC_H
#define __PROTO_CRC_H

#include "HeaderFiles.h"

/* 计算 CRC-16-Modbus, 返回16位结果(协议中按大端塞入帧) */
uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

#endif
/****************************End*****************************/
