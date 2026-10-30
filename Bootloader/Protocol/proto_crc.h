/************************************************************
 * 文件名: proto_crc.h
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTO_CRC_H
#define __PROTO_CRC_H

#include "HeaderFiles.h"

uint16_t crc16_modbus(const uint8_t *data, uint16_t len);

#endif
/****************************End*****************************/
