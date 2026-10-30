/************************************************************
 * 版权：2025CIMC Copyright
 * 文件：RTC.c
 * 作者: Qiao Qin @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Qiao Qin     2025/4/20     V0.01    original
************************************************************/

#include "RTC.h"

#define RTC_CLOCK_SOURCE_LXTAL      /* 定义RTC时钟源为外部低速晶振 */
#define BKP_VALUE    0x32F0

rtc_parameter_struct   rtc_initpara;
rtc_alarm_struct       rtc_alarm;
__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;

static uint8_t rtc_dec2bcd(uint8_t val);   /* 十进制转BCD（内部使用，前向声明） */

/*!
    \brief      RTC配置函数：首次上电写入默认时间 2026-06-05 00:00:00
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_Init(void)
{
    /* 使能PMU时钟 */
    rcu_periph_clock_enable(RCU_PMU);
    /* 使能对RTC寄存器的访问 */
    pmu_backup_write_enable();

    rtc_pre_config();
    /* 获取RTC时钟入口选择 */
    RTCSRC_FLAG = GET_BITS(RCU_BDCTL, 8, 9);

    /* 检查RTC是否已配置过 */
    if((BKP_VALUE != RTC_BKP0) || (0x00 == RTCSRC_FLAG)){
        /* 备份寄存器校验失败或时钟源未配置（首次上电/Vbat掉电）：
           写入默认时间并打标记，下次上电不再重写（若LXTAL有电池则保持） */
        rtc_setup(26, 6, 5, 0, 0, 0);
        RTC_BKP0 = BKP_VALUE;
    }
    rcu_all_reset_flag_clear();
}

void rtc_pre_config(void)
{
    #if defined (RTC_CLOCK_SOURCE_IRC32K)
          rcu_osci_on(RCU_IRC32K);
          rcu_osci_stab_wait(RCU_IRC32K);
          rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

          prescaler_s = 0x13F;
          prescaler_a = 0x63;
    #elif defined (RTC_CLOCK_SOURCE_LXTAL)
          rcu_osci_on(RCU_LXTAL);
          rcu_osci_stab_wait(RCU_LXTAL);
          rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

          prescaler_s = 0xFF;
          prescaler_a = 0x7F;
    #else
    #error RTC clock source should be defined.
    #endif /* RTC_CLOCK_SOURCE_IRC32K */

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

/*!
    \brief      设置RTC完整日期与时间
    \param[in]  tmp_year/tmp_month/tmp_day/tmp_hh/tmp_mm/tmp_ss : 十进制日期时间
    \retval     0=成功, 1=失败
*/
uint8_t rtc_setup(uint32_t tmp_year, uint32_t tmp_month, uint32_t tmp_day,
                  uint32_t tmp_hh, uint32_t tmp_mm, uint32_t tmp_ss)
{
    ErrStatus error_status = ERROR;

    /* 设置预分频 */
    rtc_initpara.factor_asyn = prescaler_a;
    rtc_initpara.factor_syn  = prescaler_s;

    /* BCD转换后填入日期时间参数 */
    rtc_initpara.year   = rtc_dec2bcd(tmp_year);
    rtc_initpara.month  = rtc_dec2bcd(tmp_month);
    rtc_initpara.date   = rtc_dec2bcd(tmp_day);
    rtc_initpara.hour   = rtc_dec2bcd(tmp_hh);
    rtc_initpara.minute = rtc_dec2bcd(tmp_mm);
    rtc_initpara.second = rtc_dec2bcd(tmp_ss);

    /* 固定格式参数 */
    rtc_initpara.day_of_week   = RTC_SATURDAY;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm         = RTC_AM;

    /* 写入RTC硬件 */
    error_status = rtc_init(&rtc_initpara);
    if (ERROR == error_status) return 1;

    return 0;
}

/**
 * @brief       十进制转换为BCD码
 * @param       val : 要转换的十进制数
 * @retval      BCD码
 */
static uint8_t rtc_dec2bcd(uint8_t val)
{
    uint8_t bcdhigh = 0;

    while (val >= 10)
    {
        bcdhigh++;
        val -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | val);
}

/*!
    \brief      仅设置时分秒
    \retval     0=成功, 1=失败
*/
uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec)
{
    ErrStatus error_status = ERROR;

    rtc_initpara.hour   = rtc_dec2bcd(hour);
    rtc_initpara.minute = rtc_dec2bcd(min);
    rtc_initpara.second = rtc_dec2bcd(sec);

    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化RTC寄存器 */

    if (ERROR == error_status) return 1;

    return 0;
}

/*!
    \brief      仅设置年月日
    \retval     0=成功, 1=失败
*/
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date)
{
    ErrStatus error_status = ERROR;

    rtc_initpara.year  = rtc_dec2bcd(year);
    rtc_initpara.month = rtc_dec2bcd(month);
    rtc_initpara.date  = rtc_dec2bcd(date);

    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化RTC寄存器 */

    if (ERROR == error_status) return 1;

    return 0;
}

/************************************************************
 * 函数: rtc_alarm_after_seconds (静态)
 * 描述: 配置 RTC_ALARM0 于 当前时间+sec 触发 (mask日期, 只比 时:分:秒)
 ************************************************************/
static void rtc_alarm_after_seconds(uint32_t sec)
{
    rtc_parameter_struct now;
    rtc_alarm_struct     al;
    uint32_t h, m, s;

    rtc_current_time_get(&now);                 /* 读当前时间(BCD) */
    s = (uint32_t)(((now.second >> 4) * 10U) + (now.second & 0x0FU));
    m = (uint32_t)(((now.minute >> 4) * 10U) + (now.minute & 0x0FU));
    h = (uint32_t)(((now.hour   >> 4) * 10U) + (now.hour   & 0x0FU));

    s += sec;                                   /* 加偏移并逐级进位 */
    m += s / 60U;  s %= 60U;
    h += m / 60U;  m %= 60U;
    h %= 24U;

    rtc_alarm_disable(RTC_ALARM0);
    al.alarm_mask      = RTC_ALARM_DATE_MASK;   /* 不比较日期, 只匹配 时:分:秒 */
    al.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    al.alarm_day       = 0x01U;                 /* 已被mask, 任意值 */
    al.alarm_hour      = rtc_dec2bcd((uint8_t)h);
    al.alarm_minute    = rtc_dec2bcd((uint8_t)m);
    al.alarm_second    = rtc_dec2bcd((uint8_t)s);
    al.am_pm           = RTC_AM;
    rtc_alarm_config(RTC_ALARM0, &al);

    rtc_flag_clear(RTC_FLAG_ALRM0);
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);
}

/************************************************************
 * 函数: mcu_deepsleep_with_rtc
 * 描述: 进入深度睡眠, 由 RTC闹钟(sec秒后)唤醒; 唤醒后已恢复系统时钟
 ************************************************************/
void mcu_deepsleep_with_rtc(uint32_t sec)
{
    /* 1. 设置 RTC 闹钟 = 当前时间 + sec */
    rtc_alarm_after_seconds(sec);

    /* 2. RTC闹钟对应 EXTI line17, 配为中断+上升沿, 使能NVIC(深睡唤醒源) */
    exti_flag_clear(EXTI_17);
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_Alarm_IRQn, 2U, 0U);

    /* 3. 进入深度睡眠(WFI), CPU停在此处直到闹钟唤醒 */
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_DISABLE, WFI_CMD);

    /* ===== 闹钟唤醒后从此继续 ===== */
    /* 4. 深睡退出后时钟切回IRC16M, 重配回 240MHz PLL(否则USART波特率错乱) */
    SystemInit();

    /* 5. 关闭闹钟与清标志 */
    rtc_alarm_disable(RTC_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_flag_clear(EXTI_17);
}
