/************************************************************
 * 文件名: adc_app.c
 * 功能:   ADC 应用层实现 (原始值 -> 电压)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "adc_app.h"

#define ADC_VREF        3.3f       /* 参考电压 (按实际硬件调整) */
#define ADC_FULL_SCALE  4095.0f    /* 12位满量程 */

/************************************************************
 * 函数: adc_app_read
 * 描述: 读取两路点位并换算为电压
 * 参数: ch0_v - PC0(IN10)电压输出 (NULL跳过)
 *       ch1_v - PC1(IN11)电压输出 (NULL跳过)
 ************************************************************/
void adc_app_read(float *ch0_v, float *ch1_v)
{
    if (ch0_v != NULL) {
        *ch0_v = (float)adc_read_raw(ADC_CHANNEL_10) * ADC_VREF / ADC_FULL_SCALE;
    }
    if (ch1_v != NULL) {
        *ch1_v = (float)adc_read_raw(ADC_CHANNEL_11) * ADC_VREF / ADC_FULL_SCALE;
    }
	//CH2为PT100采样得到的温度
	//变量名：temp_C
}
/****************************End*****************************/
