/************************************************************
 * 文件名: param_app.c
 * 功能:   设备参数持久化管理
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#include "param_app.h"
#include "proto_crc.h"      /* 复用 crc16_modbus 计算参数校验 */

device_param_t g_param;

/************************************************************
 * 函数: param_app_load_default
 * 描述: 填入出厂默认值 (仅写内存 g_param, 不落 Flash)
 ************************************************************/
void param_app_load_default(void)
{
    memset(&g_param, 0, sizeof(g_param));
    g_param.magic         = PARAM_MAGIC;
    g_param.version       = PARAM_VERSION;
    g_param.device_id     = 0x0001U;       /* 默认设备ID */
    g_param.ratio_ch0     = 1.0f;          /* 默认变比 1.0 */
    g_param.ratio_ch1     = 1.0f;
    g_param.threshold_ch0 = 0.0f;          /* 默认阈值 0(未设置) */
    g_param.threshold_ch1 = 0.0f;
    g_param.baudrate      = 19200U;       /* 默认 RS485 波特率 */
    g_param.report_interval_s = 1U;        /* 默认上报间隔 1s (映射码01) */
    g_param.alarm_report_en = 0U;          /* 默认不主动上报告警(仅存储,需查询) */
    g_param.alarm_count   = 0U;
}

// 函数: param_calc_crc (静态)，计算 g_param 的 CRC, 范围=结构体除末尾 crc 字段外的全部字节
static uint16_t param_calc_crc(void)
{
    return crc16_modbus((const uint8_t *)&g_param,
                        (uint16_t)(sizeof(device_param_t) - sizeof(g_param.crc)));
}

// 函数: param_app_init，上电从内部 Flash 读回参数并校验; 失败则恢复默认值
void param_app_init(void)
{
    intflash_read(PARAM_FLASH_ADDR, (uint8_t *)&g_param, sizeof(device_param_t));

    /* 魔数/版本/CRC 任一不符: 视为首次上电或数据损坏, 恢复默认 */
    if ((g_param.magic != PARAM_MAGIC) ||
        (g_param.version != PARAM_VERSION) ||
        ((uint16_t)g_param.crc != param_calc_crc())) {
        param_app_load_default();
    }
}

// 函数: param_app_save，重算 CRC -> 擦参数页 -> 整体写回
uint8_t param_app_save(void)
{
    g_param.magic   = PARAM_MAGIC;
    g_param.version = PARAM_VERSION;
    g_param.crc     = param_calc_crc();    /* 写前重算校验 */

    if (intflash_erase_page(PARAM_FLASH_ADDR) != 0U) {
        return 1U;                         /* 擦除失败 */
    }
    return intflash_write(PARAM_FLASH_ADDR, (const uint8_t *)&g_param,
                          sizeof(device_param_t));
}
/****************************End*****************************/
