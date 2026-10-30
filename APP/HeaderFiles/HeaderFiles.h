/************************************************************
 * 文件：Headerfiles.h
 * 平台: 2025 CIMC
************************************************************/
#ifndef __HEADERFILES_H
#define __HEADERFILES_H

/************************* 头文件 *************************/

#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Function.h"     // 执行函数

/*以下为自定义头文件*/
#include "led.h"   // LED驱动
#include "led_app.h"  // LED应用层
#include "OLED.h"      // OLED显示屏
#include "rs485.h" // RS485
#include "usart2.h" // USART2 串口
#include "ad3344_app.h" // AD3344+PT100
#include "RTC.h"        // RTC实时时钟 LXTA
#include "rtc_app.h"   // RTC业务层
#include "protocol.h"  // 通信协议层
#include "ADC.h"        // ADC驱动: PC1/PC2
#include "adc_app.h"    // ADC应用: ch0/ch1
#include "dac.h"        // DAC驱动: PA4 输出
#include "dac_app.h"    // DAC应用: 电压设置
//#include "USART0.h"//串口头文件
//#include "adc.h"//adc配置头文件
//#include "tfcard.h"
//#include "timer.h"
#include "spiflash.h" // 外部flash GD25Q40E 

#include "intflash.h"  // 内部Flash读写
#include "param_app.h" // 参数持久化

//#include "sdcard.h"
//#include "diskio.h"
//#include "ffconf.h"
//#include "integer.h"

#endif

/****************************End*****************************/
