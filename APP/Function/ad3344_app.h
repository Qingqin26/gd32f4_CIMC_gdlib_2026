/************************************************************
 * 文件名: ad3344_app.h
 * 功能:   GD30AD3344 ADC + PT100 测温 应用层
 *         - SPI3 端口实现 (供 gd30ad3344.c 调用)
 *         - PT100 温度计算 (Callendar-Van Dusen 二次方反解)
 * 平台:   2026 CIMC GD32F4xx
 * 硬件:   SCK  = PE12  (SPI3_SCK,  AF5)
 *         MISO = PE13  (SPI3_MISO, AF5)
 *         MOSI = PE14  (SPI3_MOSI, AF5)
 *         CS   = PE9   (GPIO 输出, 软件控制)
 * 配置:   差分 AIN0-AIN1, PGA 2.048V (gain 2), 1mA 激励, 100SPS
 ************************************************************/
#ifndef __AD3344_APP_H
#define __AD3344_APP_H

#include "gd32f4xx.h"

/* ============= SPI3 + CS 引脚 ============= */
#define AD3344_SPI              SPI3
#define AD3344_SPI_RCU          RCU_SPI3
#define AD3344_GPIO_PORT        GPIOE
#define AD3344_GPIO_RCU         RCU_GPIOE

#define AD3344_SCK_PIN          GPIO_PIN_12
#define AD3344_MISO_PIN         GPIO_PIN_13
#define AD3344_MOSI_PIN         GPIO_PIN_14
#define AD3344_CS_PIN           GPIO_PIN_9

/* ============= CS 控制宏 (供 gd30ad3344.c 调用) ============= */
#define SPI_CLR_CS()    gpio_bit_reset(AD3344_GPIO_PORT, AD3344_CS_PIN)
#define SPI_SET_CS()    gpio_bit_set(AD3344_GPIO_PORT,   AD3344_CS_PIN)

/* ============= SPI 16位收发 (供 gd30ad3344.c 调用) ============= */
uint16_t ad3344_spi_txrx16bit(uint16_t data);

/* ============= 应用层 API ============= */
void    ad3344_app_init(void);          /* 初始化 SPI + AD3344 */
int16_t ad3344_app_read_raw(void);      /* 读取一次 ADC raw 值 */
float   ad3344_app_read_temp(void);     /* 读取一次 PT100 温度 (摄氏度) */

/* 一次读取，返回电压(V)/电阻(欧)/温度(C), 指针可为 NULL 跳过 */
void    ad3344_app_read_all(float *voltage_V, float *resistance_ohm, float *temp_C);

#endif
/****************************End*****************************/
