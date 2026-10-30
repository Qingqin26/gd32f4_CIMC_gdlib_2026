/************************************************************
 * 文件名: rs485.c
 * 描述:   RS485 驱动实现
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#include "rs485.h"

static volatile uint8_t  s_rx_buf[RS485_RX_BUF_SIZE];//环形缓冲区
static volatile uint16_t s_rx_head = 0U;
static volatile uint16_t s_rx_tail = 0U;

void rs485_init(void)
{
    /* 1. 使能时钟 */
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_USART1);

    /* 2. PD5/PD6 复用为 USART1 (AF7) */
    gpio_af_set(RS485_TX_PORT, GPIO_AF_7, RS485_TX_PIN);
    gpio_af_set(RS485_RX_PORT, GPIO_AF_7, RS485_RX_PIN);
    gpio_mode_set(RS485_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_TX_PIN);
    gpio_output_options_set(RS485_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_TX_PIN);
    gpio_mode_set(RS485_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_RX_PIN);

    /* 3. PE8 方向控制 - 推挽输出, 默认低电平 (接收模式) */
    gpio_mode_set(RS485_DE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_DE_PIN);
    gpio_output_options_set(RS485_DE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_DE_PIN);
    rs485_dir_rx();

    /* 4. USART1 配置: 19200 8N1, 无流控 */
    usart_deinit(RS485_USART);
    usart_baudrate_set(RS485_USART, RS485_BAUDRATE);
    usart_word_length_set(RS485_USART, USART_WL_8BIT);
    usart_stop_bit_set(RS485_USART, USART_STB_1BIT);
    usart_parity_config(RS485_USART, USART_PM_NONE);
    usart_hardware_flow_rts_config(RS485_USART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(RS485_USART, USART_CTS_DISABLE);
    usart_receive_config(RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS485_USART, USART_TRANSMIT_ENABLE);
    usart_enable(RS485_USART);

    /* 5. 使能 RX 中断 (优先级 2, 低于 SysTick 的 0) */
    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);
    nvic_irq_enable(USART1_IRQn, 2U, 0U);
}

void rs485_send(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) {
        return;
    }

    /* 切到发送方向 */
    rs485_dir_tx();

    /* 逐字节发送, 每字节等待 TBE (发送缓冲区空) */
    while (len--) {
        usart_data_transmit(RS485_USART, (uint32_t)(*data++));
        while (usart_flag_get(RS485_USART, USART_FLAG_TBE) == RESET) { /* wait */ }
    }

    /* 关键: 等待 TC (Transmission Complete) 确认最后一字节移出移位寄存器
       否则提前切回 RX 会导致最后一字节被截断 */
    while (usart_flag_get(RS485_USART, USART_FLAG_TC) == RESET) { /* wait */ }

    /* 切回接收方向 */
    rs485_dir_rx();
}

uint16_t rs485_recv(uint8_t *buf, uint16_t maxlen)
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
        s_rx_tail = (uint16_t)((s_rx_tail + 1U) % RS485_RX_BUF_SIZE);
        __enable_irq();
    }

    return n;
}

//USART1 中断服务: 接收到的字节压入环形 buffer
void USART1_IRQHandler(void)
{
    uint8_t  ch;
    uint16_t next;

    if (usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE) != RESET) {
        ch   = (uint8_t)usart_data_receive(RS485_USART);
        next = (uint16_t)((s_rx_head + 1U) % RS485_RX_BUF_SIZE);

        if (next != s_rx_tail) {
            /* buffer 未满 */
            s_rx_buf[s_rx_head] = ch;
            s_rx_head = next;
        }
        /* buffer 满则丢弃该字节 (避免覆盖未读数据) */
    }
}

/************************************************************
 * 函数: rs485_set_baudrate，运行时修改 RS485(USART1) 波特率
 ************************************************************/
void rs485_set_baudrate(uint32_t baud)
{
    usart_disable(RS485_USART);
    usart_baudrate_set(RS485_USART, baud);
    usart_enable(RS485_USART);
}

/****************************End*****************************/
