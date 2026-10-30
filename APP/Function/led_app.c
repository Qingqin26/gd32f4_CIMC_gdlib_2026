/************************************************************
 * 文件名: led_app.c
 * 功能:   LED 应用层实现 (状态数组 -> GPIO刷新, 脏标志优化)
 *         底层初始化/控制宏见 Driver/led
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "led_app.h"

/************************************************************
 * 函数: led_disp
 ************************************************************/
void led_disp(uint8_t *ucLed)
{
    uint8_t        temp     = 0x00U;
    static uint8_t temp_old = 0xFFU;       /* 初值0xFF强制首次刷新 */

    if (ucLed[0]) { temp |= 0x01U; }
    if (ucLed[1]) { temp |= 0x02U; }

    if (temp == temp_old) {
        return;
    }

    /* 高电平点亮: bit=1 -> SET(3.3V), bit=0 -> RESET(0V) */
    gpio_bit_write(LED_PORT, LED0_PIN, (temp & 0x01U) ? SET : RESET);
    gpio_bit_write(LED_PORT, LED1_PIN, (temp & 0x02U) ? SET : RESET);

    temp_old = temp;
}
/****************************End*****************************/
