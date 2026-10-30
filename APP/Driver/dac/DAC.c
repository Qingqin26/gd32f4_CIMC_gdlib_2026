/************************************************************
 * 文件名: dac.c
 * 功能:   DAC0 输出驱动 (PA4 = DAC0_OUT0)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "dac.h"

/************************************************************
 * 函数: dac_init
 ************************************************************/
void dac_init(void)
{
    rcu_periph_clock_enable(DAC_RCU);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(DAC_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC_PIN);

    dac_deinit(DAC0);
    dac_trigger_disable(DAC0, DAC_OUT0);              /* 关触发: 写数据即更新输出 */
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);
    dac_output_buffer_enable(DAC0, DAC_OUT0);         /* 输出缓冲(增强带载能力) */
    dac_enable(DAC0, DAC_OUT0);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, 0U);
}

// 函数: dac_set_raw，设置 12 位 DAC 输出值 (0~4095, 立即生效)
void dac_set_raw(uint16_t value)
{
    if (value > 4095U) {
        value = 4095U;
    }
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, value);
}
/****************************End*****************************/
