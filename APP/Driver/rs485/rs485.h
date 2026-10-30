/************************************************************
 * 文件名: rs485.h
 * 功能:   RS485 半双工收发驱动
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __RS485_H
#define __RS485_H

#include "gd32f4xx.h"

/* ============= 硬件配置 ============= */
#define RS485_USART         USART1
#define RS485_BAUDRATE      19200U //初始化默认为19200

#define RS485_TX_PORT       GPIOD
#define RS485_TX_PIN        GPIO_PIN_5
#define RS485_RX_PORT       GPIOD
#define RS485_RX_PIN        GPIO_PIN_6
#define RS485_DE_PORT       GPIOE
#define RS485_DE_PIN        GPIO_PIN_8

#define rs485_dir_tx()      gpio_bit_set(RS485_DE_PORT,   RS485_DE_PIN)
#define rs485_dir_rx()      gpio_bit_reset(RS485_DE_PORT, RS485_DE_PIN)

#define RS485_RX_BUF_SIZE   64U

/* ============= 对外接口 ============= */
void rs485_init(void);                                       /* 初始化 GPIO + USART + 中断 */
void rs485_send(const uint8_t *data, uint16_t len);          /* 发送  */
uint16_t rs485_recv(uint8_t *buf, uint16_t maxlen);              /* 非阻塞读 */

/* 运行时修改 RS485 波特率 */
void rs485_set_baudrate(uint32_t baud);

#endif
/****************************End*****************************/
