/************************************************************
 * 文件名: proto_frame.c
 * 功能:   协议帧编解码实现
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "proto_frame.h"
#include "proto_crc.h"
#include "proto_cmd.h"

/************************************************************
 * 函数: proto_frame_parse
 * 描述: 解析并校验一帧(字节), 提取字段
 ************************************************************/
int proto_frame_parse(const uint8_t *b, uint16_t n, proto_frame_t *out)
{
    uint8_t  N;
    uint16_t crc_calc, crc_recv;

    if (b == NULL || out == NULL) {
        return PROTO_ERR_PARAM;
    }
    if (n < PROTO_FRAME_OVERHEAD) {
        return PROTO_ERR_LENGTH;
    }
    /* 起始标志 0xA5B6 */
    if (b[0] != 0xA5U || b[1] != 0xB6U) {
        return PROTO_ERR_HEAD;
    }
    N = b[7];
    if (n != (uint16_t)(PROTO_FRAME_OVERHEAD + N)) {
        return PROTO_ERR_LENGTH;
    }
    /* 结束标志 0xB6A5 */
    if (b[11 + N] != 0xB6U || b[12 + N] != 0xA5U) {
        return PROTO_ERR_TAIL;
    }
    /* 协议版本 */
    if (b[8] != PROTO_VERSION) {
        return PROTO_ERR_VERSION;
    }
    /* CRC-16-Modbus 对 [起始标志~内容] = b[0 .. 9+N-1] (共 9+N 字节), 大端存放 */
    crc_calc = crc16_modbus(b, (uint16_t)(9 + N));
    crc_recv = (uint16_t)(((uint16_t)b[9 + N] << 8) | b[10 + N]);
    if (crc_calc != crc_recv) {
        return PROTO_ERR_CRC;
    }
    /* 提取字段 */
    out->dev_id  = (uint16_t)(((uint16_t)b[2] << 8) | b[3]);
    out->type    = b[4];
    out->cmd     = (uint16_t)(((uint16_t)b[5] << 8) | b[6]);
    out->len     = N;
    out->content = (N > 0U) ? &b[9] : NULL;
    return PROTO_OK;
}

/************************************************************
 * 函数: proto_frame_build
 * 描述: 用字段组装一帧
 ************************************************************/
uint16_t proto_frame_build(uint16_t dev_id, uint8_t type, uint16_t cmd,
                           const uint8_t *content, uint8_t n,
                           uint8_t *out, uint16_t out_max)
{
    uint16_t crc, i;
    uint16_t flen = (uint16_t)(PROTO_FRAME_OVERHEAD + n);

    if (out == NULL || flen > out_max) {
        return 0U;
    }
    out[0] = 0xA5U;
    out[1] = 0xB6U;
    out[2] = (uint8_t)(dev_id >> 8);
    out[3] = (uint8_t)dev_id;
    out[4] = type;
    out[5] = (uint8_t)(cmd >> 8);
    out[6] = (uint8_t)cmd;
    out[7] = n;
    out[8] = PROTO_VERSION;
    for (i = 0U; i < n; i++) {
        out[9 + i] = (content != NULL) ? content[i] : 0U;
    }
    crc = crc16_modbus(out, (uint16_t)(9 + n));
    out[9 + n]  = (uint8_t)(crc >> 8);
    out[10 + n] = (uint8_t)crc;
    out[11 + n] = 0xB6U;
    out[12 + n] = 0xA5U;
    return flen;
}
/****************************End*****************************/
