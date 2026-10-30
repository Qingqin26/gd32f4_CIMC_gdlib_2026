/************************************************************
 * 文件名: protocol.c
 * 功能:   通信协议层实现 (RS485, ASCII-hex 帧)
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "protocol.h"
#include "proto_hex.h"
#include "proto_frame.h"
#include "proto_crc.h"
#include "proto_cmd.h"
#include "adc_app.h"
#include "param_app.h"
#include "rs485.h"
#include "rtc_app.h"
#include "ad3344_app.h"
#include "systick.h"
#include "dac_app.h"
#include "RTC.h"

static uint8_t  g_reboot_req = 0U;     /* 设备重启请求位 */
static uint32_t g_baud_pending = 0U;  /* 待切换波特率 */
static uint8_t  s_report_en   = 0U;    /* 定时自动上报使能 */
static uint32_t s_report_last = 0U;    /* 上次自动上报时刻 */
static uint8_t  s_sleep_req   = 0U;    /* 睡眠请求 */
static uint8_t  s_was_over[2] = {0U, 0U}; /* 告警边沿检测 */

/* 固件版本号 2.0.1.0 */
#define FW_VER_MAJOR    2U
#define FW_VER_MINOR    0U
#define FW_VER_PATCH    1U
#define FW_VER_BUILD    0U

/* ASCII 接收累积缓冲 */
#define RXBUF_SIZE   (PROTO_FRAME_MAX * 2U + 16U)
static char     s_rx[RXBUF_SIZE];
static uint16_t s_rx_len = 0U;

static char up(char c)
{
    return (c >= 'a' && c <= 'f') ? (char)(c - 32) : c;
}
static uint8_t is_hex(char c)
{
    c = up(c);
    return (uint8_t)((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
}
static uint8_t nib(char c)
{
    c = up(c);
    if (c >= '0' && c <= '9') { return (uint8_t)(c - '0'); }
    if (c >= 'A' && c <= 'F') { return (uint8_t)(c - 'A' + 10); }
    return 0U;
}

/* 从接收缓冲丢弃前 cnt 个字符 */
static void rx_consume(uint16_t cnt)
{
    uint16_t i;
    if (cnt >= s_rx_len) { s_rx_len = 0U; return; }
    for (i = cnt; i < s_rx_len; i++) {
        s_rx[i - cnt] = s_rx[i];
    }
    s_rx_len = (uint16_t)(s_rx_len - cnt);
}

/* 查找起始标志 "A5B6" */
static int rx_find_start(void)
{
    uint16_t i;
    if (s_rx_len < 4U) { return -1; }
    for (i = 0U; (uint16_t)(i + 4U) <= s_rx_len; i++) {
        if (up(s_rx[i]) == 'A' && up(s_rx[i + 1U]) == '5' &&
            up(s_rx[i + 2U]) == 'B' && up(s_rx[i + 3U]) == '6') {
            return (int)i;
        }
    }
    return -1;
}

/* float -> 大端4字节 ，IEEE754 单精度 */
static void f32_to_be(float v, uint8_t *b)
{
    uint32_t u;
    memcpy(&u, &v, sizeof(u));      /* 取 float 位模式(小端机) */
    b[0] = (uint8_t)(u >> 24);
    b[1] = (uint8_t)(u >> 16);
    b[2] = (uint8_t)(u >> 8);
    b[3] = (uint8_t)u;
}

/* 大端4字节 -> float */
static float be_to_f32(const uint8_t *b)
{
    uint32_t u = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                 ((uint32_t)b[2] << 8)  | (uint32_t)b[3];
    float v;
    memcpy(&v, &u, sizeof(v));
    return v;
}

/* uint32 -> 大端4字节 */
static void u32_to_be(uint32_t u, uint8_t *b)
{
    b[0] = (uint8_t)(u >> 24);
    b[1] = (uint8_t)(u >> 16);
    b[2] = (uint8_t)(u >> 8);
    b[3] = (uint8_t)u;
}

/* 大端4字节 -> uint32 */
static uint32_t be_to_u32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  | (uint32_t)b[3];
}

/* 波特率映射码 -> 实际波特率值  */
static uint32_t baud_code_to_val(uint8_t code)
{
    switch (code) {
        case 0x11U: return 4800U;
        case 0x12U: return 9600U;
        case 0x13U: return 19200U;
        case 0x14U: return 115200U;
        default:    return 0U;
    }
}

/* 实际波特率值 -> 协议映射码(1字节) */
static uint8_t baud_val_to_code(uint32_t baud)
{
    switch (baud) {
        case 4800U:   return 0x11U;
        case 9600U:   return 0x12U;
        case 19200U:  return 0x13U;
        case 115200U: return 0x14U;
        default:      return 0x00U;
    }
}

/* 上报间隔映射码(协议1字节) -> 秒 */
static uint32_t interval_code_to_sec(uint8_t code)
{
    switch (code) {
        case 0x01U: return 1U;
        case 0x02U: return 3U;
        case 0x03U: return 5U;
        default:    return 0U;
    }
}

/* 组应答帧 -> ASCII-hex 写入 resp */
static uint16_t proto_reply(uint8_t type, uint16_t cmd,
                            const uint8_t *content, uint8_t n,
                            uint8_t *resp, uint16_t resp_max)
{
    uint8_t  fb[PROTO_FRAME_MAX];
    uint16_t flen = proto_frame_build(g_param.device_id, type, cmd, content, n, fb, sizeof(fb));
    if (flen == 0U) { return 0U; }
    return bytes_to_hex(fb, flen, (char *)resp, resp_max);
}

/* 定时上报数据帧(12字节): UTC时间戳(4) + CH0(4,float) + CH1(4,float), CH0/CH1均×变比 */
static uint16_t build_report_frame(uint8_t *resp, uint16_t resp_max)
{
    uint8_t  body[12];
    float    ch0 = 0.0f, ch1 = 0.0f;
    uint32_t ts  = rtc_app_get_timestamp();   /* UTC秒级时间戳 */

    adc_app_read(&ch0, &ch1);                 /* ch0=PC0电位器, ch1=PC1 DAC回读 */
    ch0 *= g_param.ratio_ch0;                 /* ×CH0变比 */
    ch1 *= g_param.ratio_ch1;                 /* ×CH1变比 */

    u32_to_be(ts,  &body[0]);                 /* [0..3]  时间戳(大端)     */
    f32_to_be(ch0, &body[4]);                 /* [4..7]  CH0(IEEE754大端) */
    f32_to_be(ch1, &body[8]);                 /* [8..11] CH1(IEEE754大端) */
    return proto_reply(FRAME_TYPE_ACK, CMD_CTRL_REPORT_START, body, 12U, resp, resp_max);
}

/* 告警(超阈值)检测与记录 */
static uint8_t bcd2dec(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}

/* 追加一条告警记录到 g_param 环形缓冲并持久化 */
static void alarm_record_add(uint8_t ch, float threshold, float value)
{
    rtc_parameter_struct t;
    alarm_record_t      *r;
    uint32_t             idx;

    rtc_current_time_get(&t);
    idx = g_param.alarm_count % PARAM_ALARM_MAX;
    r   = &g_param.alarms[idx];
    r->year      = (uint16_t)(2000U + bcd2dec(t.year));
    r->month     = bcd2dec(t.month);
    r->day       = bcd2dec(t.date);
    r->hour      = bcd2dec(t.hour);
    r->minute    = bcd2dec(t.minute);
    r->second    = bcd2dec(t.second);
    r->channel   = ch;
    r->threshold = threshold;
    r->value     = value;
    g_param.alarm_count++;
    (void)param_app_save();//保存到flash
}

/* 告警为 ASCII 行: "20YY-MM-DD HH:MM:SS | CHx | 阈值 | 实际值" + 换行 */
static uint16_t fmt_alarm_line(const alarm_record_t *r, uint8_t *out, uint16_t out_max)
{
    int n = sprintf((char *)out, "%04u-%02u-%02u %02u:%02u:%02u | CH%u | %.2f | %.2f\r\n",
                    (unsigned)r->year, (unsigned)r->month, (unsigned)r->day,
                    (unsigned)r->hour, (unsigned)r->minute, (unsigned)r->second,
                    (unsigned)r->channel, r->threshold, r->value);
    if (n < 0 || (uint16_t)n > out_max) { return 0U; }
    return (uint16_t)n;
}

/* 单通道边沿检测: 正常->超阈值 时记录 */
static uint16_t alarm_check_one(uint8_t ch, float value, float threshold,
                                uint8_t *out, uint16_t out_max)
{
    if (threshold > 0.0f && value > threshold) {
        if (s_was_over[ch] == 0U) {
            s_was_over[ch] = 1U;
            alarm_record_add(ch, threshold, value);
            if (g_param.alarm_report_en != 0U) {      /* 主动模式 */
                return fmt_alarm_line(&g_param.alarms[(g_param.alarm_count - 1U) % PARAM_ALARM_MAX], out, out_max);
            }
        }
    } else {
        s_was_over[ch] = 0U;
    }
    return 0U;
}

/************************************************************
 * 静态函数: proto_dispatch
 * 描述: 命令分发, 按命令字 switch 进各分支
 ************************************************************/
static uint16_t proto_dispatch(const proto_frame_t *fr, uint8_t *resp, uint16_t resp_max)
{
    switch (fr->cmd) {
        case CMD_DATA_CH0: {       /* 查询CH0 */
            float   v = 0.0f;
            uint8_t body[4];
            adc_app_read(&v, NULL); //adc读数据
            v *= g_param.ratio_ch0; //上报乘以变比后的值
            f32_to_be(v, body);

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_CH0, body, 4U, resp, resp_max);
        }

        // 按 系统/数据/控制/配置/升级/告警 几类命令逐个 case 处理。
        case CMD_OTA_REQUEST: {    /* 0x0501: 回OK后写升级标志并软重启进Bootloader */
            uint8_t ok = PROTO_ACK_OK;
            rcu_periph_clock_enable(RCU_PMU);     /* 确保PMU时钟+备份域写访问 */
            pmu_backup_write_enable();
            RTC_BKP1 = 0x55AA0501U;
            g_reboot_req = 1U;

            return proto_reply(FRAME_TYPE_ACK, CMD_OTA_REQUEST, &ok, 1U, resp, resp_max);
        }

        case CMD_SYS_REBOOT: {     /* 设备重启:下发空,应答OK;置请求位,待应答发出后复位 */
            uint8_t ok = PROTO_ACK_OK;
            g_reboot_req = 1U;

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_REBOOT, &ok, 1U, resp, resp_max);
        }

        case CMD_SYS_FW_VER: {     /* 查询固件版本:下发空,应答4字节版本号 */
            uint8_t body[4];
            body[0] = FW_VER_MAJOR;
            body[1] = FW_VER_MINOR;
            body[2] = FW_VER_PATCH;
            body[3] = FW_VER_BUILD;

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_FW_VER, body, 4U, resp, resp_max);
        }

        case CMD_SYS_SET_TIME: {   /* 0x0105 设置设备时间: 下发4字节UTC秒级时间戳(大端),校时后应答OK */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 4U || fr->content == NULL) { break; }                 /* 长度不足: 回错误帧 */
            if (rtc_app_set_timestamp(be_to_u32(fr->content)) != 0U) { break; } /* 校时失败: 回错误帧 */

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_SET_TIME, &ok, 1U, resp, resp_max);
        }

        case CMD_SYS_GET_TIME: {   /* 0x0106 读取设备时间: 无下发内容,应答4字节UTC秒级时间戳 */
            uint8_t body[4];
            u32_to_be(rtc_app_get_timestamp(), body);

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_GET_TIME, body, 4U, resp, resp_max);
        }

        case CMD_SYS_GET_ID: {     /* 查询设备ID: 应答 2字节当前ID(大端) */
            uint8_t body[2];
            body[0] = (uint8_t)(g_param.device_id >> 8);
            body[1] = (uint8_t)(g_param.device_id & 0xFFU);

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_GET_ID, body, 2U, resp, resp_max);
        }

        case CMD_SYS_SET_ID: {     /* 设置设备ID: 下发2字节新ID, 应答 OK(0xFF) */
            uint16_t new_id;
            uint8_t  ok = PROTO_ACK_OK;
            if (fr->len < 2U || fr->content == NULL) {
                break;             /* 内容长度不足: 交由 default 回错误应答帧 */
            }
            new_id = (uint16_t)(((uint16_t)fr->content[0] << 8) | fr->content[1]);
            if (new_id == 0x0000U || new_id == PROTO_BROADCAST_ID) {
                break;             /* 非法ID(0x0000未分配/0xFFFF广播): 回错误应答帧 */
            }
            g_param.device_id = new_id;
            (void)param_app_save();   /* 持久化新设备ID,保存到内部flash */

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_SET_ID, &ok, 1U, resp, resp_max);
        }

        case CMD_DATA_CH1: {       /* 查询CH1: 应答 IEEE754 float = 电压*变比(ratio_ch1) */
            float   v = 0.0f;
            uint8_t body[4];
            adc_app_read(NULL, &v);
            v *= g_param.ratio_ch1;
            f32_to_be(v, body);

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_CH1, body, 4U, resp, resp_max);
        }

        case CMD_DATA_PT100: {     /* 0x0221 查询温度通道: 应答 IEEE754 */
            float   t = ad3344_app_read_temp();
            uint8_t body[4];
            f32_to_be(t, body);

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_PT100, body, 4U, resp, resp_max);
        }

        case CMD_DATA_SET_RATIO_CH0: {   /* 设置CH0变比: 下发4字节float(大端),存Flash,应答OK */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 4U || fr->content == NULL) { break; }
            g_param.ratio_ch0 = be_to_f32(fr->content);
            if (param_app_save() != 0U) { break; }

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_SET_RATIO_CH0, &ok, 1U, resp, resp_max);
        }

        case CMD_DATA_SET_RATIO_CH1: {   /* 设置变比 */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 4U || fr->content == NULL) { break; }
            g_param.ratio_ch1 = be_to_f32(fr->content);
            if (param_app_save() != 0U) { break; }

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_SET_RATIO_CH1, &ok, 1U, resp, resp_max);
        }

        case CMD_DATA_SET_INTERVAL: {   /* 0x0261 设置数据上报间隔: */
            uint32_t sec;
            uint8_t  ok = PROTO_ACK_OK;

            if (fr->len < 1U || fr->content == NULL) { break; }   /* 长度不足: 回错误帧 */
            sec = interval_code_to_sec(fr->content[0]);           /* 映射码 -> 秒 */
            if (sec == 0U) { break; }                             /* 非法映射码: 回错误帧 */
            g_param.report_interval_s = sec;
            if (param_app_save() != 0U) { break; }                /* 持久化到内部Flash失败: 回错误帧 */

            return proto_reply(FRAME_TYPE_ACK, CMD_DATA_SET_INTERVAL, &ok, 1U, resp, resp_max);
        }

        case CMD_CTRL_REPORT_START: {   /* 0x0302 定时自动上报开始: 首次立即回一帧,之后由 protocol_report_poll 按间隔发送 */
            s_report_en   = 1U;
            s_report_last = g_tick_ms;

            return build_report_frame(resp, resp_max);   /* 首次应答一帧上报数据 */
        }

        case CMD_CTRL_REPORT_STOP: {    /* 0x0303 停止自动上报: 应答 OK */
            uint8_t ok = PROTO_ACK_OK;
            s_report_en = 0U;
            return proto_reply(FRAME_TYPE_ACK, CMD_CTRL_REPORT_STOP, &ok, 1U, resp, resp_max);
        }

        case CMD_CTRL_DAC_VOLT: {   /* 0x0301 设置DAC输出 */
            uint16_t raw;
            uint8_t  ok = PROTO_ACK_OK;
            if (fr->len < 2U || fr->content == NULL) { break; }   /* 长度不足: 回错误帧 */
            raw = (uint16_t)(((uint16_t)fr->content[0] << 8) | fr->content[1]);
            if (raw > 0x0FFFU) { break; }                         /* 超出12位范围: 回错误帧 */
            dac_app_set_raw(raw);                                 /* 立即输出 */

            return proto_reply(FRAME_TYPE_ACK, CMD_CTRL_DAC_VOLT, &ok, 1U, resp, resp_max);
        }

        case CMD_CTRL_SLEEP: {     /* 0x03AA 睡眠模式，应答OK后进深度睡眠,RTC闹钟10s唤醒,醒后回字符串 */
            uint8_t ok = PROTO_ACK_OK;
            s_sleep_req = 1U;      /* OK应答发出后由 protocol_sleep_poll 执行深睡 */

            return proto_reply(FRAME_TYPE_ACK, CMD_CTRL_SLEEP, &ok, 1U, resp, resp_max);
        }

        case CMD_ALARM_REPORT_EN: {  /* 0x0601 设置告警主动上报: 01=主动,02=不主动; 存Flash,应答OK */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 1U || fr->content == NULL) { break; }       /* 长度不足: 回错误帧 */
            if      (fr->content[0] == 0x01U) { g_param.alarm_report_en = 1U; }
            else if (fr->content[0] == 0x02U) { g_param.alarm_report_en = 0U; }
            else { break; }                                           /* 非法值: 回错误帧 */
            if (param_app_save() != 0U) { break; }                    /* 持久化失败: 回错误帧 */

            return proto_reply(FRAME_TYPE_ACK, CMD_ALARM_REPORT_EN, &ok, 1U, resp, resp_max);
        }

        case CMD_ALARM_QUERY: {      /* 0x0602 查询告警记录: 非帧ASCII,最近10条倒序;无则"empty" */
            uint16_t n, i, len = 0U;
            uint32_t idx;
            n = (g_param.alarm_count < PARAM_ALARM_MAX) ? (uint16_t)g_param.alarm_count : (uint16_t)PARAM_ALARM_MAX;
            if (n == 0U) {                                            /* 无告警: 回 "empty" */
                if (resp_max < 5U) { return 0U; }
                memcpy(resp, "empty", 5U);
                return 5U;
            }
            if (resp_max < 2U) { return 0U; }
            resp[len++] = '\r';
            resp[len++] = '\n';
            for (i = 0U; i < n; i++) { //排序
                idx  = (g_param.alarm_count - 1U - i) % PARAM_ALARM_MAX;
                len += fmt_alarm_line(&g_param.alarms[idx], resp + len, (uint16_t)(resp_max - len));
            }

            return len;
        }

        case CMD_ALARM_CLEAR: {      /* 0x0603 清除告警记录: 计数清零,存Flash,应答OK */
            uint8_t ok = PROTO_ACK_OK;
            float   c0 = 0.0f, c1 = 0.0f;
            g_param.alarm_count = 0U;

            adc_app_read(&c0, &c1);
            s_was_over[0] = (uint8_t)(g_param.threshold_ch0 > 0.0f && (c0 * g_param.ratio_ch0) > g_param.threshold_ch0);
            s_was_over[1] = (uint8_t)(g_param.threshold_ch1 > 0.0f && (c1 * g_param.ratio_ch1) > g_param.threshold_ch1);
            if (param_app_save() != 0U) { break; }

            return proto_reply(FRAME_TYPE_ACK, CMD_ALARM_CLEAR, &ok, 1U, resp, resp_max);
        }

        case CMD_CFG_WRITE_TH_CH0: {     /* 写CH0阈值: 4字节float(大端),存Flash,应答OK */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 4U || fr->content == NULL) { break; }
            g_param.threshold_ch0 = be_to_f32(fr->content);
            if (param_app_save() != 0U) { break; }

            return proto_reply(FRAME_TYPE_ACK, CMD_CFG_WRITE_TH_CH0, &ok, 1U, resp, resp_max);
        }

        case CMD_CFG_WRITE_TH_CH1: {     /* 写CH1阈值 */
            uint8_t ok = PROTO_ACK_OK;
            if (fr->len < 4U || fr->content == NULL) { break; }
            g_param.threshold_ch1 = be_to_f32(fr->content);
            if (param_app_save() != 0U) { break; }

            return proto_reply(FRAME_TYPE_ACK, CMD_CFG_WRITE_TH_CH1, &ok, 1U, resp, resp_max);
        }

        case CMD_CFG_READ_TH_ALL: {      /* 0x0400 读全部阈值 */
            uint8_t body[8];
            f32_to_be(g_param.threshold_ch0, &body[0]); 
            f32_to_be(g_param.threshold_ch1, &body[4]); 

            return proto_reply(FRAME_TYPE_ACK, CMD_CFG_READ_TH_ALL, body, 8U, resp, resp_max);
        }

        case CMD_CFG_READ_TH_CH0: {      /* 读CH0阈值: 应答4字节float(大端) */
            uint8_t body[4];
            f32_to_be(g_param.threshold_ch0, body);
            return proto_reply(FRAME_TYPE_ACK, CMD_CFG_READ_TH_CH0, body, 4U, resp, resp_max);
        }

        case CMD_CFG_READ_TH_CH1: {      /* 读CH1阈值 */
            uint8_t body[4];
            f32_to_be(g_param.threshold_ch1, body);
            return proto_reply(FRAME_TYPE_ACK, CMD_CFG_READ_TH_CH1, body, 4U, resp, resp_max);
        }

        case CMD_SYS_GET_BAUD: {   /* 0x0112 查询波特率: 应答1字节映射码(11/12/13/14) */
            uint8_t code = baud_val_to_code(g_param.baudrate);

            if (code == 0x00U) { break; } 
            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_GET_BAUD, &code, 1U, resp, resp_max);
        }

        case CMD_SYS_SET_BAUD: {   /* 0x01A2 设置波特率: 下发1字节映射码(11/12/13/14),先应答OK再延迟切换 */
            uint32_t baud;
            uint8_t  ok = PROTO_ACK_OK;

            if (fr->len < 1U || fr->content == NULL) { break; }   /* 长度不足: 回错误帧 */
            baud = baud_code_to_val(fr->content[0]);              /* 映射码 -> 波特率值 */
            if (baud == 0U) { break; }                            /* 非法映射码: 回错误帧 */
            g_param.baudrate = baud;
            if (param_app_save() != 0U) { break; }                /* 持久化到内部Flash失败: 回错误帧 */
            g_baud_pending = baud;   /* 先回OK,再由 protocol_baud_poll 切换USART */

            return proto_reply(FRAME_TYPE_ACK, CMD_SYS_SET_BAUD, &ok, 1U, resp, resp_max);
        }

        default:
            break;
    }
    return proto_reply(FRAME_TYPE_ERROR, PROTO_ERR_CMD, NULL, 0U, resp, resp_max);
}

/* 处理一整帧(字节): 解析+ID过滤+分发 */
static uint16_t proto_handle_one(const uint8_t *fb, uint16_t flen,
                                 uint8_t *resp, uint16_t resp_max)
{
    proto_frame_t fr;
    int r = proto_frame_parse(fb, flen, &fr);

    if (r != PROTO_OK) {
        if (s_report_en != 0U) { return 0U; }

        if (r == PROTO_ERR_HEAD || r == PROTO_ERR_PARAM) {
            return 0U;
        }
        
        return proto_reply(FRAME_TYPE_ERROR, PROTO_ERR_CMD, NULL, 0U, resp, resp_max);
    }

    if (fr.dev_id != g_param.device_id && fr.dev_id != PROTO_BROADCAST_ID) {
        return 0U;
    }
    /* 上报期间闸门(0x0302已开启): 仅停止上报(0x0303)有效*/
    if (s_report_en != 0U &&
        !(fr.type == FRAME_TYPE_CMD && fr.cmd == CMD_CTRL_REPORT_STOP)) {
        return 0U;
    }
    /* 心跳帧(0x05): 广播寻找设备(0xFFFF)回一次本机心跳(0x8888); 对方心跳等不回 */
    if (fr.type == FRAME_TYPE_HEARTBEAT) {
        if (fr.cmd == CMD_DISCOVER) {
            return proto_reply(FRAME_TYPE_HEARTBEAT, CMD_HEARTBEAT, NULL, 0U, resp, resp_max);
        }
        return 0U;
    }
    /* 命令下发帧(0x01): 正常分发处理 */
    if (fr.type == FRAME_TYPE_CMD) {
        return proto_dispatch(&fr, resp, resp_max);
    }
    
    return proto_reply(FRAME_TYPE_ERROR, PROTO_ERR_CMD, NULL, 0U, resp, resp_max);
}

void protocol_init(void)
{
    s_rx_len = 0U;
}

uint16_t protocol_rs485_feed(const uint8_t *data, uint16_t len,
                             uint8_t *resp, uint16_t resp_max)
{
    uint16_t i;
    uint16_t resp_len = 0U;

    if (data == NULL || resp == NULL) {
        return 0U;
    }

    /* 1. 追加输入 */
    for (i = 0U; i < len; i++) {
        char c = (char)data[i];
        if (is_hex(c)) {
            if (s_rx_len < RXBUF_SIZE) {
                s_rx[s_rx_len++] = c;
            } else {
                s_rx_len = 0U;   /* 溢出复位 */
            }
        }
    }

    /* 2. 循环提取完整帧并处理 */
    for (;;) {
        int start = rx_find_start();
        uint8_t  N;
        uint16_t frame_chars;
        uint8_t  fb[PROTO_FRAME_MAX];
        uint16_t flen;

        if (start < 0) {
            /* 未见起始标志: 仅保留末尾<=3字符(可能是半个A5B6) */
            if (s_rx_len > 3U) {
                rx_consume((uint16_t)(s_rx_len - 3U));
            }
            break;
        }
        if (start > 0) {
            rx_consume((uint16_t)start); 
        }
        if (s_rx_len < 16U) {
            break;
        }
        /* 报文长度 N = 第8字节(字节索引7) = ASCII 索引14,15 */
        N = (uint8_t)((nib(s_rx[14]) << 4) | nib(s_rx[15]));
        if (N > PROTO_PAYLOAD_MAX) {
            rx_consume(4U);
            continue;
        }
        frame_chars = (uint16_t)((PROTO_FRAME_OVERHEAD + N) * 2U);
        if (s_rx_len < frame_chars) {
            uint16_t j;
            uint8_t  resync = 0U;
            
            for (j = 16U; (uint16_t)(j + 4U) <= s_rx_len && (uint16_t)(j + 4U) < frame_chars; j += 2U) {
                if (up(s_rx[j]) == 'B' && up(s_rx[j + 1U]) == '6' &&
                    up(s_rx[j + 2U]) == 'A' && up(s_rx[j + 3U]) == '5') {
                    uint16_t ac = (uint16_t)(j + 4U);
                    flen = hex_to_bytes(s_rx, ac, fb, sizeof(fb));
                    if (resp_len < resp_max) {
                        resp_len += proto_handle_one(fb, flen, resp + resp_len,
                                                     (uint16_t)(resp_max - resp_len));
                    }
                    rx_consume(ac);
                    resync = 1U;
                    break;
                }
            }
            if (resync != 0U) { continue; }

            for (j = 2U; (uint16_t)(j + 4U) <= s_rx_len; j++) {
                if (up(s_rx[j]) == 'A' && up(s_rx[j + 1U]) == '5' &&
                    up(s_rx[j + 2U]) == 'B' && up(s_rx[j + 3U]) == '6') {
                    rx_consume(j);
                    resync = 1U;
                    break;
                }
            }
            if (resync != 0U) { continue; }
            break;
        }
        /* ASCII 还原为字节并处理 */
        flen = hex_to_bytes(s_rx, frame_chars, fb, sizeof(fb));
        if (resp_len < resp_max) {
            resp_len += proto_handle_one(fb, flen,
                                         resp + resp_len,
                                         (uint16_t)(resp_max - resp_len));
        }
        rx_consume(frame_chars);
    }

    return resp_len;
}
/* 设备重启轮询 */
/* 收到 0x0101 时已置 g_reboot_req,这里延时确保应答上线后再系统复位 */
void protocol_reboot_poll(void)
{
    if (g_reboot_req != 0U) {
        delay_1ms(5U);
        NVIC_SystemReset();     /* CMSIS 内核系统复位 */
    }
}

/* 构造本机心跳帧 */
uint16_t protocol_build_heartbeat(uint8_t *resp, uint16_t resp_max)
{
    return proto_reply(FRAME_TYPE_HEARTBEAT, CMD_HEARTBEAT, NULL, 0U, resp, resp_max);
}
/* 波特率延迟切换 */
void protocol_baud_poll(void)
{
    if (g_baud_pending != 0U) {
        delay_1ms(2U);                          /* 余量:确保总线空闲 */
        rs485_set_baudrate(g_baud_pending);     /* 切到新波特率 */
        g_baud_pending = 0U;
    }
}

/* 定时自动上报轮询 */
uint16_t protocol_report_poll(uint8_t *resp, uint16_t resp_max)
{
    uint32_t interval_ms;

    if (s_report_en == 0U || resp == NULL) {
        return 0U;
    }
    interval_ms = g_param.report_interval_s * 1000U;
    if (interval_ms == 0U) {
        interval_ms = 1000U;              /* 防御: 间隔为0按1s */
    }
    if ((uint32_t)(g_tick_ms - s_report_last) < interval_ms) {
        return 0U;                        /* 未到间隔 */
    }
    s_report_last = g_tick_ms;
    return build_report_frame(resp, resp_max);
}

/* 查询定时自动上报 是否启动*/
uint8_t protocol_report_is_active(void)
{
    return s_report_en;
}

/* 睡眠: 收到0x03AA并发出OK后,在此进入深度睡眠;10s后RTC闹钟唤醒并回复字符串 */
void protocol_sleep_poll(void)
{
    if (s_sleep_req != 0U) {
        s_sleep_req = 0U;
        delay_1ms(5U);                       /* 确保OK应答帧已发完 */
        OLED_DisPlay_Off();                  /* 进入睡眠前关闭OLED显示(省电) */
        mcu_deepsleep_with_rtc(10U);         /* 深睡10s,唤醒后已恢复时钟 */
        OLED_DisPlay_On();                   /* 睡眠结束后重新打开OLED显示 */

        rs485_send((const uint8_t *)"instrument wakeup", 17U);  /* 唤醒后回复 */
    }
}

/* 告警: 读CH0/CH1判超阈值 */
uint16_t protocol_alarm_poll(uint8_t *resp, uint16_t resp_max)
{
    float    c0 = 0.0f, c1 = 0.0f;
    uint16_t body = 0U;

    if (resp == NULL || resp_max < 3U) { return 0U; }
    adc_app_read(&c0, &c1);
    c0 *= g_param.ratio_ch0;                     /* CH0 = 电位器电压 × 变比 */
    c1 *= g_param.ratio_ch1;                     /* CH1 = DAC回读电压 × 变比 */
    body += alarm_check_one(0U, c0, g_param.threshold_ch0, resp + 2U + body, (uint16_t)(resp_max - 2U - body));
    body += alarm_check_one(1U, c1, g_param.threshold_ch1, resp + 2U + body, (uint16_t)(resp_max - 2U - body));
    if (s_report_en != 0U) { return 0U; }
    if (body == 0U) { return 0U; }               /* 无告警 */
    resp[0] = '\r';
    resp[1] = '\n';

    return (uint16_t)(body + 2U);
}

/****************************End*****************************/
