/************************************************************
 * 文件名: ADC.c
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "ADC.h"

/************************************************************
 * 函数: adc_init，初始化 ADC0
 ************************************************************/
void adc_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);

    /* PC0 / PC1 配置为模拟输入 */
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_1 | GPIO_PIN_0);

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    adc_deinit();
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);

    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    adc_enable(ADC0);
    delay_1ms(1);
    adc_calibration_enable(ADC0);
}

/************************************************************
 * 函数: adc_read_raw，读取指定规则通道一次转换结果 (12位)
 ************************************************************/
uint16_t adc_read_raw(uint8_t channel)
{
    adc_routine_channel_config(ADC0, 0U, channel, ADC_SAMPLETIME_480);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);

    while (RESET == adc_flag_get(ADC0, ADC_FLAG_EOC)) {
        /* 等待转换完成 */
    }
    adc_flag_clear(ADC0, ADC_FLAG_EOC);

    return adc_routine_data_read(ADC0);
}
/****************************End*****************************/
