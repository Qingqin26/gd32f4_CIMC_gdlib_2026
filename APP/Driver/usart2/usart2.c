/************************************************************
 * 文件名: usart2.c
 * 功能:   USART2 串口打印输出实现 (查询发送, 类 printf)
 * 平台:   2026 CIMC GD32F4xx
 * 硬件:   TX = PD8 (USART2_TX, AF7), RX = PD9 (USART2_RX, AF7)
 ************************************************************/
#include "usart2.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 接收环形缓冲区 (USART2_IRQHandler 写, 主循环读) */
static volatile uint8_t  s_rx_buf[USART2_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0U;
static volatile uint16_t s_rx_tail = 0U;

/************************************************************
 * 函数: usart2_init
 * 功能: 初始化 USART2 (GPIO + 115200 8N1)
 ************************************************************/
void usart2_init(void)
{
    /* 1. 使能时钟 */
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_USART2);

    /* 2. PD8/PD9 复用为 USART2 (AF7) */
    gpio_af_set(USART2_TX_PORT, GPIO_AF_7, USART2_TX_PIN);
    gpio_af_set(USART2_RX_PORT, GPIO_AF_7, USART2_RX_PIN);
    gpio_mode_set(USART2_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART2_TX_PIN);
    gpio_output_options_set(USART2_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART2_TX_PIN);
    gpio_mode_set(USART2_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART2_RX_PIN);

    /* 3. USART2 参数: 115200 8N1, 无校验 */
    usart_deinit(USART2_PERIPH);
    usart_baudrate_set(USART2_PERIPH, USART2_BAUDRATE);
    usart_word_length_set(USART2_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(USART2_PERIPH, USART_STB_1BIT);
    usart_parity_config(USART2_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART2_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART2_PERIPH, USART_CTS_DISABLE);
    usart_receive_config(USART2_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART2_PERIPH, USART_TRANSMIT_ENABLE);
    usart_enable(USART2_PERIPH);

    /* 4. 使能 RX 中断 (接收用), 优先级 3 */
    usart_interrupt_enable(USART2_PERIPH, USART_INT_RBNE);
    nvic_irq_enable(USART2_IRQn, 3U, 0U);
}

/************************************************************
 * 函数: usart2_send
 * 功能: 阻塞发送原始字节流
 * 参数: data - 数据指针, len - 字节数
 ************************************************************/
void usart2_send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) {
        return;
    }

    /* 逐字节发送, 每字节等待 TBE (发送缓冲空) */
    while (len--) {
        usart_data_transmit(USART2_PERIPH, (uint32_t)(*data++));
        while (usart_flag_get(USART2_PERIPH, USART_FLAG_TBE) == RESET) { /* wait */ }
    }

    /* 等待 TC, 确保最后一字节完全移出移位寄存器 */
    while (usart_flag_get(USART2_PERIPH, USART_FLAG_TC) == RESET) { /* wait */ }
}

//重定向
void usart2_printf(const char *fmt, ...)
{
    char     buf[USART2_PRINTF_BUF];
    uint16_t len;
    va_list  ap;

    /* 格式化到本地缓冲 (vsnprintf 防越界) */
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* 强制以 '\0' 结尾, 用实际长度发送 (不依赖 vsnprintf 返回值) */
    buf[USART2_PRINTF_BUF - 1] = '\0';
    len = (uint16_t)strlen(buf);
    if (len > 0U) {
        usart2_send((const uint8_t *)buf, len);
    }
}
/************************************************************
 * 函数: usart2_recv
 * 功能: 从接收环形缓冲读出数据
 ************************************************************/
uint16_t usart2_recv(uint8_t *buf, uint16_t maxlen)
{
    uint16_t n = 0U;

    if (buf == NULL || maxlen == 0U) {
        return 0U;
    }

    while (n < maxlen) {
        __disable_irq();
        if (s_rx_head == s_rx_tail) {
            __enable_irq();
            break;
        }
        buf[n++]  = s_rx_buf[s_rx_tail];
        s_rx_tail = (uint16_t)((s_rx_tail + 1U) % USART2_RX_BUF_SIZE);
        __enable_irq();
    }

    return n;
}

/************************************************************
 * USART2 中断服务: 接收到的字节压入环形缓冲
 ************************************************************/
void USART2_IRQHandler(void)
{
    uint8_t  ch;
    uint16_t next;

    if (usart_interrupt_flag_get(USART2_PERIPH, USART_INT_FLAG_RBNE) != RESET) {
        ch   = (uint8_t)usart_data_receive(USART2_PERIPH);
        next = (uint16_t)((s_rx_head + 1U) % USART2_RX_BUF_SIZE);

        if (next != s_rx_tail) {       /* 缓冲未满 */
            s_rx_buf[s_rx_head] = ch;
            s_rx_head = next;
        }
        /* 缓冲满则丢弃新字节 (避免覆盖未读数据) */
    }
}
/****************************End*****************************/
