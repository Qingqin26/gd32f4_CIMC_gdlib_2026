/************************************************************
 * 文件名: led.c
 * 功能:   板载 LED 驱动实现 (LED0=PA6, LED1=PA7)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "led.h"

/************************************************************
 * 函数:   led_init
 * 功能:   初始化 2 个 LED 引脚 
 * 状态灯和采集灯
 ************************************************************/
void led_init(void)
{
    rcu_periph_clock_enable(LED_RCU);

    gpio_mode_set(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_ALL_PINS);
    gpio_output_options_set(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED_ALL_PINS);

    led_off(LED_ALL_PINS);
}
/****************************End*****************************/
