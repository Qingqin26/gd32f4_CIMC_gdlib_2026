/************************************************************
 * 文件名: protocol.c
 * 描述:   Bootloader 通信协议层
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#include "protocol.h"
#include "proto_hex.h"
#include "proto_frame.h"
#include "proto_crc.h"
#include "proto_cmd.h"

/* 参数区 */
#define PARAM_FLASH_ADDR    0x08010000U
#define PARAM_FLASH_MAGIC   0x43494D43U

static uint16_t s_dev_id = DEVICE_ID_DEFAULT;   /* 默认 0x0001 */

/* ASCII 字符累积缓冲 (至少容纳一整帧的十六进制字符) */
#define RXBUF_SIZE   (PROTO_FRAME_MAX * 2U + 16U)
static char     s_rx[RXBUF_SIZE];
static uint16_t s_rx_len = 0U;

static char up(char c)
{
    return (c >= 'a' && c <= 'f') ? (char)(c - 32) : c;
}
static uint8_t is_hex(char c)
{
    c = up(c);
    return (uint8_t)((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
}
static uint8_t nib(char c)
{
    c = up(c);
    if (c >= '0' && c <= '9') { return (uint8_t)(c - '0'); }
    if (c >= 'A' && c <= 'F') { return (uint8_t)(c - 'A' + 10); }
    return 0U;
}

/* 从接收缓冲丢弃(消费)前 cnt 个字符 */
static void rx_consume(uint16_t cnt)
{
    uint16_t i;
    if (cnt >= s_rx_len) { s_rx_len = 0U; return; }
    for (i = cnt; i < s_rx_len; i++) {
        s_rx[i - cnt] = s_rx[i];
    }
    s_rx_len = (uint16_t)(s_rx_len - cnt);
}

/* 起始标志 "A5B6", 返回起始索引或 -1 */
static int rx_find_start(void)
{
    uint16_t i;
    if (s_rx_len < 4U) { return -1; }
    for (i = 0U; (uint16_t)(i + 4U) <= s_rx_len; i++) {
        if (up(s_rx[i]) == 'A' && up(s_rx[i + 1U]) == '5' &&
            up(s_rx[i + 2U]) == 'B' && up(s_rx[i + 3U]) == '6') {
            return (int)i;
        }
    }
    return -1;
}

void protocol_init(void)
{
    s_rx_len = 0U;

    /* 读取设备ID: 参数区magic有效则取device_id, 否则用默认0x0001 */
    if (*(volatile uint32_t *)PARAM_FLASH_ADDR == PARAM_FLASH_MAGIC) {
        s_dev_id = *(volatile uint16_t *)(PARAM_FLASH_ADDR + 6U);
        if (s_dev_id == 0x0000U || s_dev_id == PROTO_BROADCAST_ID) {
            s_dev_id = DEVICE_ID_DEFAULT;
        }
    } else {
        s_dev_id = DEVICE_ID_DEFAULT;
    }
}

uint16_t protocol_dev_id(void)
{
    return s_dev_id;
}

//读取App持久化的RS485波特率
uint32_t protocol_saved_baudrate(void)
{
    uint32_t baud;

    if (*(volatile uint32_t *)PARAM_FLASH_ADDR != PARAM_FLASH_MAGIC) {
        return 19200U;
    }
    baud = *(volatile uint32_t *)(PARAM_FLASH_ADDR + 24U);
    if ((baud == 4800U) || (baud == 9600U) || (baud == 19200U) || (baud == 115200U)) {
        return baud;
    }
    return 19200U;
}

uint16_t protocol_build_ack(uint16_t cmd, uint8_t ok, uint8_t *resp, uint16_t cap)
{
    uint8_t  fb[PROTO_FRAME_MAX];
    uint16_t flen;

    if (resp == NULL) { return 0U; }

    if (ok != 0U) {
        uint8_t body = PROTO_ACK_OK;    /* 0xFF */
        flen = proto_frame_build(s_dev_id, FRAME_TYPE_ACK, cmd, &body, 1U, fb, sizeof(fb));
    } else {
        flen = proto_frame_build(s_dev_id, FRAME_TYPE_ERROR, cmd, NULL, 0U, fb, sizeof(fb));
    }
    if (flen == 0U) { return 0U; }
    return bytes_to_hex(fb, flen, (char *)resp, cap);
}

uint8_t protocol_feed_cmd(const uint8_t *data, uint16_t len, uint16_t *out_cmd)
{
    uint16_t i;

    if (data == NULL) { return 0U; }

    /* 1. 追加输入(仅十六进制字符) */
    for (i = 0U; i < len; i++) {
        char c = (char)data[i];
        if (is_hex(c)) {
            if (s_rx_len < RXBUF_SIZE) {
                s_rx[s_rx_len++] = c;
            } else {
                s_rx_len = 0U;   /* 溢出复位 */
            }
        }
    }

    /* 2. 循环提取完整帧 */
    for (;;) {
        int           start = rx_find_start();
        uint8_t       N;
        uint16_t      frame_chars, flen;
        uint8_t       fb[PROTO_FRAME_MAX];
        proto_frame_t fr;

        if (start < 0) {
            if (s_rx_len > 3U) { rx_consume((uint16_t)(s_rx_len - 3U)); }
            break;
        }
        if (start > 0) { rx_consume((uint16_t)start); }
        if (s_rx_len < 16U) { break; }            /* 不足以读到内容长度字段 */

        /* 内容长度 N = 第8字节(字节索引7) = ASCII 索引14,15 */
        N = (uint8_t)((nib(s_rx[14]) << 4) | nib(s_rx[15]));
        if (N > PROTO_PAYLOAD_MAX) {
            rx_consume(4U);                        /* 长度异常: 跳过本起始标志重找 */
            continue;
        }
        frame_chars = (uint16_t)((PROTO_FRAME_OVERHEAD + N) * 2U);
        if (s_rx_len < frame_chars) { break; }     /* 整帧未到齐 */

        flen = hex_to_bytes(s_rx, frame_chars, fb, sizeof(fb));
        rx_consume(frame_chars);

        if (proto_frame_parse(fb, flen, &fr) == PROTO_OK) {
            if (fr.dev_id == s_dev_id || fr.dev_id == PROTO_BROADCAST_ID) {
                if (fr.type == FRAME_TYPE_CMD) {
                    if (out_cmd != NULL) { *out_cmd = fr.cmd; }
                    return 1U;                      /* 命中一条命令, 剩余留待下次 */
                }
            }
        }
    }
    return 0U;
}
/****************************End*****************************/
