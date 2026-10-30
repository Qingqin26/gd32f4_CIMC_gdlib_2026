/************************************************************
 * 文件名: proto_cmd.h
 * 功能:   通信协议常量 —— 帧类型 / 命令字 / 错误码 / 协议常量
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#ifndef __PROTO_CMD_H
#define __PROTO_CMD_H

/* ---------- 协议常量 ---------- */
#define PROTO_START_FLAG     0xA5B6U   /* 起始标志(大端)   */
#define PROTO_END_FLAG       0xB6A5U   /* 结束标志(大端)   */
#define PROTO_VERSION        0x02U     /* 协议版本(固定)   */
#define PROTO_BROADCAST_ID   0xFFFFU   /* 广播设备ID       */
#define DEVICE_ID_DEFAULT    0x0001U   /* 本机默认设备ID(可由 0x01A1 设置) */
#define PROTO_ACK_OK         0xFFU     /* 应答内容: OK     */
#define PROTO_ERR_CMD        0xEEEEU   /* 错误应答命令字   */

/* ---------- 帧类型 (4.2) ---------- */
#define FRAME_TYPE_CMD       0x01U     /* 上位机->设备 命令下发帧 */
#define FRAME_TYPE_ACK       0x02U     /* 设备->上位机 应答帧     */
#define FRAME_TYPE_HEARTBEAT 0x05U     /* 双向 心跳帧             */
#define FRAME_TYPE_ERROR     0xFFU     /* 设备->上位机 异常/错误帧 */

/* ---------- 命令字 (4.3, 高字节=功能模块 低字节=指令) ---------- */
/* 系统管理类 0x01 */
#define CMD_SYS_REBOOT          0x0101U  /* 设备重启           */
#define CMD_SYS_FACTORY         0x0102U  /* 恢复出厂(预留)     */
#define CMD_SYS_INFO            0x0103U  /* 查询设备信息(预留) */
#define CMD_SYS_FW_VER          0x0104U  /* 查询固件版本       */
#define CMD_SYS_SET_TIME        0x0105U  /* 设置设备时间       */
#define CMD_SYS_GET_TIME        0x0106U  /* 查询设备时间       */
#define CMD_SYS_GET_ID          0x0111U  /* 查询设备ID         */
#define CMD_SYS_GET_BAUD        0x0112U  /* 查询波特率         */
#define CMD_SYS_SET_ID          0x01A1U  /* 设置设备ID         */
#define CMD_SYS_SET_BAUD        0x01A2U  /* 设置波特率         */
/* 数据类 0x02 */
#define CMD_DATA_CH0            0x0201U  /* 查询CH0(ADC0:滑动变阻器/PC0) */
#define CMD_DATA_CH1            0x0202U  /* 查询CH1(ADC1:DAC回读/PC1)    */
#define CMD_DATA_PT100          0x0221U  /* 查询特定通道(外部ADC PT100)  */
#define CMD_DATA_SET_RATIO_CH0  0x0241U  /* 设置CH0变比        */
#define CMD_DATA_SET_RATIO_CH1  0x0242U  /* 设置CH1变比        */
#define CMD_DATA_SET_INTERVAL   0x0261U  /* 设置数据上报间隔   */
/* 控制类 0x03 */
#define CMD_CTRL_DAC_VOLT       0x0301U  /* 设置DAC输出电压    */
#define CMD_CTRL_REPORT_START   0x0302U  /* 定时自动上报开始(CH0/CH1) */
#define CMD_CTRL_REPORT_STOP    0x0303U  /* 定时自动上报停止   */
#define CMD_CTRL_SLEEP          0x03AAU  /* 进入睡眠模式       */
/* 参数配置类 0x04 */
#define CMD_CFG_READ_TH_ALL     0x0400U  /* 读阈值(批量CH0/CH1)*/
#define CMD_CFG_READ_TH_CH0     0x0401U  /* 读CH0阈值          */
#define CMD_CFG_READ_TH_CH1     0x0402U  /* 读CH1阈值          */
#define CMD_CFG_READ_TH_CH2     0x0403U  /* 读CH2阈值          */
#define CMD_CFG_WRITE_TH_CH0    0x0411U  /* 写CH0阈值          */
#define CMD_CFG_WRITE_TH_CH1    0x0412U  /* 写CH1阈值          */
#define CMD_CFG_WRITE_TH_CH2    0x0413U  /* 写CH2阈值          */
/* 系统升级类 0x05 */
#define CMD_OTA_REQUEST         0x0501U  /* 升级请求           */
#define CMD_OTA_PREPARE         0x0502U  /* 准备传输固件包     */
#define CMD_OTA_EXECUTE         0x0503U  /* 执行升级流程       */
/* 告警与日志类 0x06 */
#define CMD_ALARM_REPORT_EN     0x0601U  /* 是否主动上报告警   */
#define CMD_ALARM_QUERY         0x0602U  /* 查询告警记录       */
#define CMD_ALARM_CLEAR         0x0603U  /* 清除告警           */
#define CMD_LOG_QUERY           0x0604U  /* 查询操作日志(预留) */
#define CMD_LOG_CLEAR           0x0605U  /* 清除操作日志(预留) */
/* 特殊命令字 */
#define CMD_HEARTBEAT           0x8888U  /* 设备心跳/上电通知(帧类型0x05) */
#define CMD_DISCOVER            0xFFFFU  /* 上位机广播寻找设备(帧类型0x05) */

/* ---------- 帧解析返回码 ---------- */
#define PROTO_OK             0
#define PROTO_ERR_PARAM      1
#define PROTO_ERR_LENGTH     2
#define PROTO_ERR_HEAD       3
#define PROTO_ERR_TAIL       4
#define PROTO_ERR_VERSION    5
#define PROTO_ERR_CRC        6

#endif
/****************************End*****************************/
