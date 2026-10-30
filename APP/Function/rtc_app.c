/************************************************************
 * 文件名: rtc_app.c
 * 描述:   RTC 业务层实现 (由 Driver/rtc/RTC.c 移植应用接口而来)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "rtc_app.h"

/************************************************************
 * 函数: rtc_app_get_time_str
 * 描述: 读取当前 RTC 时间并格式化为字符串
 * 输出: "Time:20YY-MM-DD HH:MM:SS\r\n"
 ************************************************************/
void rtc_app_get_time_str(char *buf)
{
    rtc_parameter_struct t;

    rtc_current_time_get(&t);
    sprintf(buf, "Time:20%02x-%02x-%02x %02x:%02x:%02x\r\n",
            t.year, t.month, t.date, t.hour, t.minute, t.second);
}

/************************************************************
 * 函数: rtc_app_set_time
 * 描述: 设置 RTC 日期时间
 * 参数: year   - 公历年(如2026, 内部取后两位)
 *       month/day/hour/minute/second - 月/日/时/分/秒
 ************************************************************/
uint8_t rtc_app_set_time(uint32_t year, uint32_t month, uint32_t day,
                         uint32_t hour, uint32_t minute, uint32_t second)
{
    return rtc_setup(year % 100U, month, day, hour, minute, second);
}

/* BCD -> 十进制 (RTC寄存器为BCD格式) */
static uint8_t bcd2dec(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}

/* UTC秒级时间戳 -> 年/月/日/时/分/秒 */
static void ts_to_utc(uint32_t ts, uint16_t *yy, uint8_t *mo, uint8_t *dd,
                      uint8_t *hh, uint8_t *mi, uint8_t *ss)
{
    static const uint8_t mdays[12] = {31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U};
    uint32_t days = ts / 86400U;    /* 自1970-01-01起的整天数 */
    uint32_t rem  = ts % 86400U;    /* 当天剩余秒数         */
    uint32_t y    = 1970U;
    uint8_t  leap = 0U, dim, m;

    *hh = (uint8_t)(rem / 3600U);
    *mi = (uint8_t)((rem % 3600U) / 60U);
    *ss = (uint8_t)(rem % 60U);

    for (;;) {                      /* 逐年扣减定位年份 */
        leap = (uint8_t)(((y % 4U) == 0U && (y % 100U) != 0U) || (y % 400U) == 0U);
        if (days < (leap ? 366U : 365U)) { break; }
        days -= (leap ? 366U : 365U);
        y++;
    }
    *yy = (uint16_t)y;

    for (m = 0U; m < 12U; m++) {     /* 逐月扣减定位月/日 */
        dim = mdays[m];
        if (m == 1U && leap != 0U) { dim = 29U; }
        if (days < dim) { break; }
        days -= dim;
    }
    *mo = (uint8_t)(m + 1U);
    *dd = (uint8_t)(days + 1U);
}

/* 年/月/日/时/分/秒 -> UTC秒级时间戳 (ts_to_utc 的逆运算) */
static uint32_t utc_to_ts(uint16_t yy, uint8_t mo, uint8_t dd,
                          uint8_t hh, uint8_t mi, uint8_t ss)
{
    static const uint16_t cum[12] = {0U,31U,59U,90U,120U,151U,181U,212U,243U,273U,304U,334U};
    uint32_t days = 0U;
    uint16_t y;
    uint8_t  leap;

    for (y = 1970U; y < yy; y++) {            /* 累加整年天数 */
        leap = (uint8_t)(((y % 4U) == 0U && (y % 100U) != 0U) || (y % 400U) == 0U);
        days += leap ? 366U : 365U;
    }
    days += cum[mo - 1U];                      /* 当年到月初的天数 */
    leap = (uint8_t)(((yy % 4U) == 0U && (yy % 100U) != 0U) || (yy % 400U) == 0U);
    if (mo > 2U && leap != 0U) { days += 1U; } /* 当年闰且已过2月,补1天 */
    days += (uint32_t)(dd - 1U);
    return days * 86400U + (uint32_t)hh * 3600U + (uint32_t)mi * 60U + (uint32_t)ss;
}

/************************************************************
 * 函数: rtc_app_set_timestamp
 * 功能: 用UTC秒级时间戳校时 (内部转日历后调 rtc_app_set_time)
 ************************************************************/
uint8_t rtc_app_set_timestamp(uint32_t ts)
{
    uint16_t yy;
    uint8_t  mo, dd, hh, mi, ss;
    ts_to_utc(ts, &yy, &mo, &dd, &hh, &mi, &ss);
    return rtc_app_set_time(yy, mo, dd, hh, mi, ss);  /* 复用:内部对年取后两位 */
}

/************************************************************
 * 函数: rtc_app_get_timestamp
 * 功能: 读取当前RTC时间(BCD)并转为UTC秒级时间戳
 ************************************************************/
uint32_t rtc_app_get_timestamp(void)
{
    rtc_parameter_struct t;
    uint16_t yy;
    uint8_t  mo, dd, hh, mi, ss;

    rtc_current_time_get(&t);
    yy = (uint16_t)(2000U + bcd2dec(t.year));   /* 年为2位,补全为20YY */
    mo = bcd2dec(t.month);
    dd = bcd2dec(t.date);
    hh = bcd2dec(t.hour);                        /* 24小时制,无AM/PM位 */
    mi = bcd2dec(t.minute);
    ss = bcd2dec(t.second);
    return utc_to_ts(yy, mo, dd, hh, mi, ss);
}

/****************************End*****************************/
