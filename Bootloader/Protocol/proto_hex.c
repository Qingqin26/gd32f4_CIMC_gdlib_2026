/************************************************************
 * 文件名: proto_hex.c
 ************************************************************/
#include "proto_hex.h"

static const char HEX_UP[] = "0123456789ABCDEF";

static uint8_t hex_nibble(char c)
{
    if (c >= '0' && c <= '9') { return (uint8_t)(c - '0'); }
    if (c >= 'A' && c <= 'F') { return (uint8_t)(c - 'A' + 10); }
    if (c >= 'a' && c <= 'f') { return (uint8_t)(c - 'a' + 10); }
    return 0xFFU;
}

//字节数组 -> 大写十六进制ASCII字符串
uint16_t bytes_to_hex(const uint8_t *bytes, uint16_t n, char *out, uint16_t out_max)
{
    uint16_t i;
    uint16_t k = 0U;

    if (bytes == NULL || out == NULL) {
        return 0U;
    }

    for (i = 0U; i < n; i++) {
        if ((uint16_t)(k + 2U) > out_max) {        /* 放不下完整一字节(2字符)则停 */
            break;
        }
        out[k++] = HEX_UP[(bytes[i] >> 4) & 0x0FU];
        out[k++] = HEX_UP[bytes[i] & 0x0FU];
    }
    return k;
}

//描述: 十六进制ASCII字符串 -> 字节数组
uint16_t hex_to_bytes(const char *hex, uint16_t hex_len, uint8_t *out, uint16_t out_max)
{
    uint16_t i;
    uint16_t k = 0U;
    uint8_t  hi, lo;

    if (hex == NULL || out == NULL) {
        return 0U;
    }
    if ((hex_len & 1U) != 0U) {                    /* 必须偶数长度 */
        return 0U;
    }

    for (i = 0U; i < hex_len; i += 2U) {
        if (k >= out_max) {                        /* 输出缓冲已满 */
            break;
        }
        hi = hex_nibble(hex[i]);
        lo = hex_nibble(hex[i + 1U]);
        if (hi == 0xFFU || lo == 0xFFU) {          /* 含非法字符 */
            return 0U;
        }
        out[k++] = (uint8_t)((hi << 4) | lo);
    }
    return k;
}
/****************************End*****************************/
