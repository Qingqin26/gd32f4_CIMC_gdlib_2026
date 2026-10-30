/************************************************************
 * 文件名: proto_frame.h
 * 功能:   协议帧编解码 (字节层) —— 解析与组帧
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTO_FRAME_H
#define __PROTO_FRAME_H

#include "HeaderFiles.h"

#define PROTO_FRAME_OVERHEAD  13U 
#define PROTO_PAYLOAD_MAX     64U 
#define PROTO_FRAME_MAX       (PROTO_FRAME_OVERHEAD + PROTO_PAYLOAD_MAX)

/* 解析后的帧字段 (content 指向原缓冲, 勿越生命周期使用) */
typedef struct {
    uint16_t       dev_id;    /* 目标设备ID    */
    uint8_t        type;      /* 帧类型        */
    uint16_t       cmd;       /* 命令字        */
    uint8_t        len;       /* 内容长度 N    */
    const uint8_t *content;   /* 内容指针(N=0时为NULL) */
} proto_frame_t;

/* 解析字节帧: 校验起始/结束/版本/CRC + 长度 */
int proto_frame_parse(const uint8_t *bytes, uint16_t n, proto_frame_t *out);

/* 组字节帧 */
uint16_t proto_frame_build(uint16_t dev_id, uint8_t type, uint16_t cmd,
                           const uint8_t *content, uint8_t n,
                           uint8_t *out, uint16_t out_max);

#endif
/****************************End*****************************/
