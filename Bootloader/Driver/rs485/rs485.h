/************************************************************
 * 文件名: rs485.h
 * 描述:   RS485 半双工收发驱动 (SP3485 + USART1) —— Bootloader 升级通道
 * 平台:   2026 CIMC GD32F470VE
 * 硬件:   TX = PD5  (USART1_TX, AF7)
 *         RX = PD6  (USART1_RX, AF7)
 *         DE = PE8  (方向控制, 高=发送, 低=接收)
 * 参数:   19200 / 8N1 / 无流控
 ************************************************************/
#ifndef __RS485_H
#define __RS485_H

#include "gd32f4xx.h"

/* ============= 硬件配置 ============= */
#define RS485_USART         USART1
#define RS485_BAUDRATE      19200U      /* Bootloader 固定初始波特率 19200 */

#define RS485_TX_PORT       GPIOD
#define RS485_TX_PIN        GPIO_PIN_5
#define RS485_RX_PORT       GPIOD
#define RS485_RX_PIN        GPIO_PIN_6
#define RS485_DE_PORT       GPIOE
#define RS485_DE_PIN        GPIO_PIN_8

/* 方向控制宏 (SP3485: DE+RE短接, 高=发送, 低=接收) */
#define rs485_dir_tx()      gpio_bit_set(RS485_DE_PORT,   RS485_DE_PIN)
#define rs485_dir_rx()      gpio_bit_reset(RS485_DE_PORT, RS485_DE_PIN)

/* 接收环形缓冲区大小 */
#define RS485_RX_BUF_SIZE   512U  /* bootloader: 裸固件接收期增大防溢出 */

/* ============= 对外接口 ============= */
void     rs485_init(void);                              /* 初始化 GPIO + USART + 中断 */
void     rs485_send(const uint8_t *data, uint16_t len); /* 发送数据 (内部自动切方向) */
uint16_t rs485_recv(uint8_t *buf, uint16_t maxlen);     /* 读接收 (返回实际字节数, 0=无数据) */

/* 运行时修改 RS485 波特率(disable->set->enable); Bootloader 默认 19200, 通常不调用 */
void     rs485_set_baudrate(uint32_t baud);

#endif
/****************************End*****************************/
