/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：led.c
 * 平台: 2025CIMC IHD-V04
************************************************************/

/************************* 头文件 *************************/

#include "LED.h"

/************************ 全局变量定义 ************************/


/************************************************************ 
 * Function :       LED_Init
 * Comment  :       用于初始化LED端口
 * Parameter:       null
 * Return   :       null
************************************************************/

void LED_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);

	/* PA4~PA7 统一配成推挽输出, 上电默认拉低(灯灭) */
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
	gpio_bit_reset(GPIOA, GPIO_PIN_4);

	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
	gpio_bit_reset(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
}

/****************************End*****************************/

