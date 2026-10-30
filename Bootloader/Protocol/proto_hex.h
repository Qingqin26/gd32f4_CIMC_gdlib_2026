/************************************************************
 * 文件名: proto_hex.h
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTO_HEX_H
#define __PROTO_HEX_H

#include "HeaderFiles.h"

//字节数组 -> 大写十六进制ASCII字符串
uint16_t bytes_to_hex(const uint8_t *bytes, uint16_t n, char *out, uint16_t out_max);

//描述: 十六进制ASCII字符串 -> 字节数组
uint16_t hex_to_bytes(const char *hex, uint16_t hex_len, uint8_t *out, uint16_t out_max);

#endif
/****************************End*****************************/
