/************************************************************
 * 文件：Function.c
 * 描述: 系统初始化与主循环, 负责串口命令处理与定时上报的调度
 * 平台: 2026CIMC 202602
 * 版本: 2026/4/19
************************************************************/

/************************* 头文件 *************************/
#include "Function.h"
/************************* 宏定义 *************************/


/************************* 变量定义 *************************/
uint8_t ucLed[2] = {0, 0};    // 两个LED的逻辑状态, 1=亮 0=灭


/*逻辑函数*/
/************************* 定义函数 *************************/

/************************************************************
 * Comment  :       上电初始化, 依次拉起各个外设
************************************************************/
void system_init(void)
{
	systick_config();     // SysTick 1kHz
	led_init();           // LED
	param_app_init();     // 内部Flash
	OLED_Init();          // OLED
	rs485_init();         // RS485
	rs485_set_baudrate(g_param.baudrate);   // 持久化波特率
	usart2_init();        // 调试串口
	ad3344_app_init();    // PT100温度采集
	RTC_Init();           // RTC实时时钟: LXTAL
	spi_flash_init();     // 外部Flash GD25Q40E
	protocol_init();      // 协议层初始化
	adc_init();           // ADC: PC1/PC2
	dac_init();           // DAC: PA4 输出
	dac_app_set_voltage(1.650f);  // 开机DAC输出电压1.65V
}


/************************************************************
 * Comment  :       主循环
************************************************************/
void user_function(void)
{
	uint8_t  rs485_buf[32];
	uint16_t rs485_len;
	uint32_t last_tick = 0;
	uint8_t  oled_rep_state = 0xFFU;  // OLED状态行缓存, 0xFF=强制首次刷新
	uint8_t  rsp_buf[512];            // RS485应答缓冲
	float    voltage_V, resistance_ohm, temp_C;

	/* OLED开机界面 */
	OLED_Clear();
	OLED_ShowString(0,  0, (u8*)"2026837587 ", 16);
	OLED_ShowString(0, 16, (u8*)"IDLE  ", 16);
	OLED_Refresh();

	/* 上电主动发一帧0x8888心跳, 通知上位机设备已上线(复位后同样会发) */
	{
		uint16_t hb_len = protocol_build_heartbeat(rsp_buf, sizeof(rsp_buf));
		if (hb_len > 0U) {
			rs485_send(rsp_buf, hb_len);
		}
	}

	while(1)
	{
		led_disp(ucLed);

		/* OLED状态行: 自动上报时显示AutoSample, 否则IDLE */
		{
			uint8_t rep_active = protocol_report_is_active();
			ucLed[1] = rep_active;            // PA7采集指示灯
			if (rep_active != oled_rep_state) {
				oled_rep_state = rep_active;
				OLED_ShowString(0, 16, (u8*)(rep_active ? "AutoSample" : "IDLE      "), 16);
				OLED_Refresh();
			}
		}

		/* 读取PT100的电压/电阻/温度, 并做告警检测 */
		if ((g_tick_ms - last_tick) >= 1000U) {
			last_tick = g_tick_ms;
			ucLed[0] = (uint8_t)(!ucLed[0]);  // PA6系统心跳灯, 每秒翻转一次

			ad3344_app_read_all(&voltage_V, &resistance_ohm, &temp_C);

			/* CH0/CH1超阈值时边沿触发告警, 主动模式下发字符串 */
			{
				uint16_t al_len = protocol_alarm_poll(rsp_buf, sizeof(rsp_buf));
				if (al_len > 0U) { rs485_send(rsp_buf, al_len); }
			}
		}

		/* RS485收到数据后在协议层解析 */
		rs485_len = rs485_recv(rs485_buf, sizeof(rs485_buf));
		if (rs485_len > 0U) {
			uint16_t rsp_len = protocol_rs485_feed(rs485_buf, rs485_len,
			                                       rsp_buf, sizeof(rsp_buf));
			if (rsp_len > 0U) {
				rs485_send(rsp_buf, rsp_len);
			}
			
			protocol_baud_poll();     // 波特率延迟切换
			protocol_reboot_poll();   // 设备重启
			protocol_sleep_poll();    // 进入深度睡眠, 10s后唤醒回字符串
		}

		/* 定时自动上报 */
		{
			uint16_t rep_len = protocol_report_poll(rsp_buf, sizeof(rsp_buf));
			if (rep_len > 0U) {
				rs485_send(rsp_buf, rep_len);
			}
		}
	}
}
/****************************End*****************************/
