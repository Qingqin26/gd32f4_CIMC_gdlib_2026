/************************************************************
 * 文件名: proto_crc.c
 * 功能:   CRC-16-Modbus 实现
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "proto_crc.h"

uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t  j;

    if (data == NULL) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (j = 0U; j < 8U; j++) {
            if (crc & 0x0001U) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            } else {
                crc = (uint16_t)(crc >> 1);
            }
        }
    }
    return crc;
}
/****************************End*****************************/
