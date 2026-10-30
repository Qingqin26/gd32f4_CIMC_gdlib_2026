/************************************************************
 * 文件名: bootloader.h
 * 描述:   RS485 升级 Bootloader —— 内部 Flash 分区 / 升级流程接口
 * 平台:   2026 CIMC GD32F470VE (512KB Flash, 4KB/页)
 *
 * 内部 Flash 分区 (竞赛文档 2.5):
 *   Bootloader 区   0x08000000  64K   <- 本程序
 *   参数区          0x08010000   4K   (升级流程不擦写, 仅读device_id)
 *   App 区          0x08011000 128K
 *   App 备份区      0x08031000 128K
 *   固件暂存区      0x08051000 128K
 *
 * 升级数据流 (Bootloader 直连接收, ASCII-hex命令 + 裸bin):
 *   App 收到 0x0501 -> 写 RTC_BKP1=升级魔数 -> 软复位进 Bootloader
 *   Bootloader 判 RTC_BKP1: 是升级启动则打印10s倒计时等 0x0502
 *   0x0502 -> 擦暂存区, 收裸bin(切片256B), 空闲超时判收齐, 校验前4字节魔术字
 *            正确回 OK(0502), 错误回 ERROR
 *   0x0503 -> 先回 OK, 再"剥离4字节魔术字头 -> 备份当前App -> 写App -> 校验
 *            -> 失败回滚" -> 跳转 App
 *
 * Keil 配置:
 *   Bootloader: IROM1 = 0x08000000, Size = 0x10000 (64K)
 *   App:        IROM1 = 0x08011000, Size = 0x20000 (128K), 并设 SCB->VTOR=0x08011000
 ************************************************************/
#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "HeaderFiles.h"

/* ============= 内部 Flash 分区 ============= */
#define BL_FLASH_PAGE_SIZE    0x1000U        /* 4KB 页擦除粒度 */

#define BL_BOOT_ADDR          0x08000000U    /* Bootloader 区 64K  */
#define BL_BOOT_SIZE          0x00010000U
#define BL_PARAM_ADDR         0x08010000U    /* 参数区 4K */
#define BL_PARAM_SIZE         0x00001000U
#define BL_APP_ADDR           0x08011000U    /* App 区 128K */
#define BL_APP_SIZE           0x00020000U
#define BL_APP_BACKUP_ADDR    0x08031000U    /* App 备份区 128K */
#define BL_APP_BACKUP_SIZE    0x00020000U
#define BL_STAGING_ADDR       0x08051000U    /* 固件暂存区 128K */
#define BL_STAGING_SIZE       0x00020000U

#define BL_APP_MAX_SIZE       BL_APP_SIZE    /* App 固件最大长度 = App 区大小 */

/* App 向量表合法性判断: 初始栈顶应落在片内 SRAM (0x20000000 ~ +192KB) */
#define BL_SRAM_BASE          0x20000000U
#define BL_SRAM_SIZE          0x00050000U

/* ============= 固件包前置魔术字 ============= */
/* .bin 前 4 字节 = 5A A5 C3 3C; Bootloader 校验后剥离这4字节再写入 App 区 */
#define BL_FW_MAGIC_LEN       4U
#define BL_FW_MAGIC0          0x5AU
#define BL_FW_MAGIC1          0xA5U
#define BL_FW_MAGIC2          0xC3U
#define BL_FW_MAGIC3          0x3CU

/* ============= 升级请求启动标志 ============= */
/* App 处理 0x0501 时写入 BL_BKP_REG=BL_UPGRADE_MAGIC 再软复位;
 * Bootloader 读到则进入"升级请求启动"(10s倒计时)并清除。
 * 注意: RTC_BKP0 已被 App 的 RTC.c 占用(0x32F0标记), 这里用 RTC_BKP1。 */
#define BL_BKP_REG            RTC_BKP1
#define BL_UPGRADE_MAGIC      0x55AA0501U

/* ============= bl_apply_update 返回码 ============= */
#define BL_OK                 0U   /* 成功 */
#define BL_ERR_SIZE           1U   /* 固件长度非法 */
#define BL_ERR_MAGIC          2U   /* 魔术字不匹配 */
#define BL_ERR_BACKUP         3U   /* 备份当前 App 失败 */
#define BL_ERR_ERASE          4U   /* 擦除 App 区失败 */
#define BL_ERR_WRITE          5U   /* 写 App 区失败(已回滚) */
#define BL_ERR_VERIFY         6U   /* 写后回读校验不一致(已回滚) */

/* ============= OTA 会话上下文 (RAM) ============= */
typedef enum {
    BL_OTA_IDLE = 0,    /* 空闲 */
    BL_OTA_RECEIVING,   /* 正接收裸bin到暂存区 */
    BL_OTA_READY        /* bin已收齐且魔术字通过, 待 0x0503 */
} bl_ota_state_t;

typedef struct {
    bl_ota_state_t state;     /* 当前 OTA 状态 */
    uint32_t       fw_size;   /* App 镜像长度(=收到总长-4魔术字头) */
    uint32_t       received;  /* 已写入暂存区字节数(含魔术字头) */
} bl_ota_ctx_t;

extern bl_ota_ctx_t g_ota;

/* ============= 对外接口 ============= */
void     bootloader_main(void);                   /* Bootloader 主流程(不返回) */

uint8_t  bl_app_valid(void);                       /* App 区向量表是否合法 (1=有效) */
void     bl_jump_to_app(void);                     /* 跳转到 App (App 无效则直接返回) */

uint8_t  bl_staging_erase(void);                                              /* 擦除暂存区(128K) */
uint8_t  bl_staging_write(uint32_t offset, const uint8_t *data, uint32_t len);/* 写暂存区(相对偏移,须4字节对齐) */
uint8_t  bl_apply_update(uint32_t fw_size);                                   /* 暂存+4->备份->写App->校验->失败回滚 */

#endif
/****************************End*****************************/
