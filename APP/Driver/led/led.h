/************************************************************
 * 文件名: led.h
 * 功能:   板载 LED 驱动 (2个 LED, 高电平点亮)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __LED_H
#define __LED_H

#include "gd32f4xx.h"

/* LED 端口与时钟 */
#define LED_PORT        GPIOA
#define LED_RCU         RCU_GPIOA

/* LED 引脚定义 */
#define LED0_PIN        GPIO_PIN_6
#define LED1_PIN        GPIO_PIN_7
#define LED_ALL_PINS    (LED0_PIN | LED1_PIN)

/* LED 控制宏(高电平点亮: set=ON, reset=OFF) */
#define led_on(pin)     gpio_bit_set(LED_PORT,   (pin))
#define led_off(pin)    gpio_bit_reset(LED_PORT, (pin))
#define led_toggle(pin) gpio_bit_toggle(LED_PORT, (pin))

/* 对外接口 */
void led_init(void);

#endif
/****************************End*****************************/
