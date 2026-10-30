/************************************************************
 * 文件名: usart2.h
 * 功能:   USART2 串口打印输出 
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __USART2_H
#define __USART2_H

#include "gd32f4xx.h"

/* ============= 硬件配置 ============= */
#define USART2_PERIPH       USART2
#define USART2_BAUDRATE     115200U

#define USART2_TX_PORT      GPIOD
#define USART2_TX_PIN       GPIO_PIN_8
#define USART2_RX_PORT      GPIOD
#define USART2_RX_PIN       GPIO_PIN_9

/* printf 格式化单次发送的最大字节数 */
#define USART2_PRINTF_BUF   128U

/* 接收环形缓冲大小 (中断接收用) */
#define USART2_RX_BUF_SIZE  64U

/* ============= 对外接口 ============= */
void usart2_init(void);                                /* 初始化 GPIO + USART2 */
void usart2_send(const uint8_t *data, uint16_t len);   /* 发送原始字节流 */
void usart2_printf(const char *fmt, ...);              /* 格式化打印输出 (类 printf) */
uint16_t usart2_recv(uint8_t *buf, uint16_t maxlen);   /* 读接收缓冲, 返回实际字节数 (0=空) */

#endif
/****************************End*****************************/
