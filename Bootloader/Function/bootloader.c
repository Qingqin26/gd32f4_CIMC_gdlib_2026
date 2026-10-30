/************************************************************
 * 文件名: bootloader.c
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#include "bootloader.h"
#include "protocol.h"
#include "proto_cmd.h"

#define BL_WAIT_UPGRADE_MS      10000U   /* 升级请求启动*/
#define BL_WAIT_NORMAL_MS       5000U    /* 等待5s */
#define BL_BIN_FIRST_TIMEOUT_MS 8000U    /* 等待首个字节上限 */
#define BL_BIN_IDLE_MS          300U     /* 接收 */
#define BL_EXEC_TIMEOUT_MS      30000U

/* 全局 OTA*/
bl_ota_ctx_t g_ota = { BL_OTA_IDLE, 0U, 0U };

/* "升级请求启动"打印3s一次 */
static uint8_t s_log_on = 0U;
#define BL_LOG(...)   do { if (s_log_on) { usart2_printf(__VA_ARGS__); } } while (0)

/* 向上位机(RS485)直发ASCII字符串 */
static void bl_rs485_puts(const char *s)
{
    if (s != NULL) { rs485_send((const uint8_t *)s, (uint16_t)strlen(s)); }
}

typedef void (*app_entry_t)(void);

static uint8_t  s_blk[256];
static uint16_t s_blk_len = 0U;
static uint32_t s_wr_off  = 0U;

//Flash 区域操作
/* 按 4KB 页擦除一段区域; 0=成功 1=失败 */
static uint8_t bl_region_erase(uint32_t addr, uint32_t size)
{
    uint32_t a;
    for (a = addr; a < addr + size; a += BL_FLASH_PAGE_SIZE) {
        if (intflash_erase_page(a) != 0U) {
            return 1U;
        }
    }
    return 0U;
}

/* 把 src 区域拷到 dst 区域(调用前须先擦除 dst); 源/目标皆内部Flash, 安全 */
static uint8_t bl_region_copy(uint32_t dst, uint32_t src, uint32_t size)
{
    return intflash_write(dst, (const uint8_t *)src, size);
}

// App 跳转 / 合法性
uint8_t bl_app_valid(void)
{
    uint32_t sp = *(volatile uint32_t *)BL_APP_ADDR;
    if ((sp >= BL_SRAM_BASE) && (sp <= (BL_SRAM_BASE + BL_SRAM_SIZE))) {
        return 1U;
    }
    return 0U;
}

void bl_jump_to_app(void)
{
    uint32_t    sp, pc, i;
    app_entry_t entry;

    if (bl_app_valid() == 0U) {
        return;
    }

    sp    = *(volatile uint32_t *)(BL_APP_ADDR);
    pc    = *(volatile uint32_t *)(BL_APP_ADDR + 4U);
    entry = (app_entry_t)pc;

    __disable_irq();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    for (i = 0U; i < 8U; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    SCB->VTOR = BL_APP_ADDR;
    __DSB();
    __ISB();

    __enable_irq();

    __set_MSP(sp);
    entry();

    while (1) { }
}

// 暂存区 / 升级落地
uint8_t bl_staging_erase(void)
{
    g_ota.received = 0U;
    return bl_region_erase(BL_STAGING_ADDR, BL_STAGING_SIZE);
}

uint8_t bl_staging_write(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (data == NULL) { return 1U; }
    if ((offset + len) > BL_STAGING_SIZE) { return 1U; }
    if (intflash_write(BL_STAGING_ADDR + offset, data, len) != 0U) { return 1U; }
    if ((offset + len) > g_ota.received) { g_ota.received = offset + len; }
    return 0U;
}

/* 升级落地: 暂存区跳过4字节魔术字 -> 备份当前App -> 擦写App -> 回读校验 -> 失败回滚 */
uint8_t bl_apply_update(uint32_t fw_size)
{
    const uint8_t *src = (const uint8_t *)(BL_STAGING_ADDR + BL_FW_MAGIC_LEN);

    if ((fw_size == 0U) || (fw_size > BL_APP_MAX_SIZE)) {
        return BL_ERR_SIZE;
    }

    /* 1. 备份当前 App (整 128K) 到备份区 */
    if (bl_region_erase(BL_APP_BACKUP_ADDR, BL_APP_BACKUP_SIZE) != 0U) {
        return BL_ERR_BACKUP;
    }
    if (bl_region_copy(BL_APP_BACKUP_ADDR, BL_APP_ADDR, BL_APP_SIZE) != 0U) {
        return BL_ERR_BACKUP;
    }

    /* 2. 擦 App 区 -> 写入(暂存区跳过魔术字头) */
    if (bl_region_erase(BL_APP_ADDR, BL_APP_SIZE) != 0U) {
        return BL_ERR_ERASE;
    }
    if (intflash_write(BL_APP_ADDR, src, fw_size) != 0U) {
        (void)bl_region_erase(BL_APP_ADDR, BL_APP_SIZE);
        (void)bl_region_copy(BL_APP_ADDR, BL_APP_BACKUP_ADDR, BL_APP_SIZE);
        return BL_ERR_WRITE;
    }

    /* 3. 回读校验(memcmp); 失败则从备份区回滚 */
    if (memcmp((const void *)BL_APP_ADDR, (const void *)src, fw_size) != 0) {
        (void)bl_region_erase(BL_APP_ADDR, BL_APP_SIZE);
        (void)bl_region_copy(BL_APP_ADDR, BL_APP_BACKUP_ADDR, BL_APP_SIZE);
        return BL_ERR_VERIFY;
    }

    return BL_OK;
}

// 裸bin接收累积                                  */
static void bin_recv_reset(void)
{
    s_blk_len = 0U;
    s_wr_off  = 0U;
    g_ota.received = 0U;
}

/* 追加裸字节: 满256字节(4字节对齐)则刷入暂存区; 0=成功 1=失败 */
static uint8_t bin_recv_push(const uint8_t *d, uint16_t n)
{
    uint16_t i;
    for (i = 0U; i < n; i++) {
        s_blk[s_blk_len++] = d[i];
        if (s_blk_len == (uint16_t)sizeof(s_blk)) {
            if (bl_staging_write(s_wr_off, s_blk, s_blk_len) != 0U) {
                return 1U;
            }
            s_wr_off += s_blk_len;
            s_blk_len = 0U;
        }
    }
    return 0U;
}

/* 刷入末尾不足256的残块; *total 返回收到的总字节数; 0=成功 1=失败 */
static uint8_t bin_recv_finish(uint32_t *total)
{
    if (s_blk_len > 0U) {
        if (bl_staging_write(s_wr_off, s_blk, s_blk_len) != 0U) {
            return 1U;
        }
        s_wr_off += s_blk_len;
        s_blk_len = 0U;
    }
    if (total != NULL) { *total = s_wr_off; }
    return 0U;
}

//升级交互流程                                   */
/* 等待窗口: 轮询RS485命令帧; verbose时打印10s倒计时。
 * 收到 0x0502 返回1(进入接收); 收到 0x0501 回OK并继续等; 超时返回0 */
static uint8_t bl_wait_window(uint32_t timeout_ms, uint8_t verbose)
{
    uint8_t  rx[256];
    uint8_t  rsp[64];
    uint16_t n, cmd, rl;
    uint32_t elapsed = 0U;
    uint32_t next_mark = 0U;

    while (elapsed < timeout_ms) {
        /* 倒计时打印: 剩余 10/7/4/1 秒 (每3秒) */
        if (verbose && (elapsed == next_mark)) {
            char     line[48];
            uint32_t remain_s = (timeout_ms - elapsed) / 1000U;
            (void)snprintf(line, sizeof(line), "wait for start Application(%us)......\r\n", (unsigned)remain_s);
            bl_rs485_puts(line);   /* 倒计时提示走RS485给上位机 */
            next_mark += 3000U;
        }

        n = rs485_recv(rx, sizeof(rx));
        if (n > 0U) {
            if (protocol_feed_cmd(rx, n, &cmd) != 0U) {
                if (cmd == CMD_OTA_PREPARE) {        /* 0x0502: 进入接收(应答延后到收齐) */
                    return 1U;
                } else if (cmd == CMD_OTA_REQUEST) { /* 0x0501: 防御性, 回OK并继续等 */
                    rl = protocol_build_ack(CMD_OTA_REQUEST, 1U, rsp, sizeof(rsp));
                    if (rl > 0U) { rs485_send(rsp, rl); }
                }
                /* 其它命令: 忽略 */
            }
        } else {
            delay_1ms(1U);
            elapsed++;
        }
    }
    return 0U;
}

/* 升级交互: 接收裸bin -> 校验魔术字 -> 回0502应答 -> 等0503 -> 搬运 */
static uint8_t bl_do_upgrade(void)
{
    uint8_t        rx[256];
    uint8_t        rsp[64];
    uint16_t       n, cmd, rl;
    uint32_t       idle = 0U, waited = 0U, total = 0U;
    const uint8_t *sp = (const uint8_t *)BL_STAGING_ADDR;

    BL_LOG("[BL] receiving firmware (staging pre-erased)...\r\n");
    bin_recv_reset();

    g_ota.state = BL_OTA_RECEIVING;
    for (;;) {
        n = rs485_recv(rx, sizeof(rx));
        if (n > 0U) {
            if (bin_recv_push(rx, n) != 0U) {
                BL_LOG("[BL] staging write fail\r\n");
                rl = protocol_build_ack(CMD_OTA_PREPARE, 0U, rsp, sizeof(rsp));
                if (rl > 0U) { rs485_send(rsp, rl); }
                g_ota.state = BL_OTA_IDLE;
                return 1U;   /* 写失败回ERROR, 继续等下一个0x0502 */
            }
            idle = 0U;
        } else {
            delay_1ms(1U);
            if (g_ota.received == 0U) {
                waited++;
                if (waited >= BL_BIN_FIRST_TIMEOUT_MS) {
                    BL_LOG("[BL] no firmware data, abort\r\n");
                    g_ota.state = BL_OTA_IDLE;
                    return 0U;
                }
            } else {
                idle++;
                if (idle >= BL_BIN_IDLE_MS) {
                    break;
                }
            }
        }
    }
    (void)bin_recv_finish(&total);
    BL_LOG("[BL] received %u bytes\r\n", (unsigned)total);
    //校验
    if ((total < BL_FW_MAGIC_LEN) ||
        (sp[0] != BL_FW_MAGIC0) || (sp[1] != BL_FW_MAGIC1) ||  //5A A5 C3 3C
        (sp[2] != BL_FW_MAGIC2) || (sp[3] != BL_FW_MAGIC3)) {
        BL_LOG("[BL] magic check FAIL\r\n");
        rl = protocol_build_ack(CMD_OTA_PREPARE, 0U, rsp, sizeof(rsp));   /* ERROR */
        if (rl > 0U) { rs485_send(rsp, rl); }
        g_ota.state = BL_OTA_IDLE;
        return 1U;
    }
    g_ota.fw_size = total - BL_FW_MAGIC_LEN;
    g_ota.state   = BL_OTA_READY;

    /* 魔术字正确 -> 回 0x0502 OK */
    rl = protocol_build_ack(CMD_OTA_PREPARE, 1U, rsp, sizeof(rsp));
    if (rl > 0U) { rs485_send(rsp, rl); }
    BL_LOG("[BL] magic OK, fw=%u bytes, wait EXECUTE(0x0503)\r\n", (unsigned)g_ota.fw_size);

    waited = 0U;
    for (;;) {
        n = rs485_recv(rx, sizeof(rx));
        if (n > 0U) {
            if (protocol_feed_cmd(rx, n, &cmd) != 0U) {
                if (cmd == CMD_OTA_EXECUTE) {       /* 0x0503 */
                    uint8_t r;
                    rl = protocol_build_ack(CMD_OTA_EXECUTE, 1U, rsp, sizeof(rsp));
                    if (rl > 0U) { rs485_send(rsp, rl); }   /* 先回 OK */

                    BL_LOG("[BL] applying (backup -> write -> verify)...\r\n");
                    r = bl_apply_update(g_ota.fw_size);     /* 再搬运 */
                    BL_LOG("[BL] apply ret=%u\r\n", (unsigned)r);
                    g_ota.state = BL_OTA_IDLE;
                    return 0U;   /* 升级结束 -> 跳App(新固件/回滚旧固件) */
                }
            }
        } else {
            delay_1ms(1U);
            waited++;
            if (waited >= BL_EXEC_TIMEOUT_MS) {
                BL_LOG("[BL] no EXECUTE cmd, jump App\r\n");
                g_ota.state = BL_OTA_IDLE;
                return 0U;   /* 等0503超时 -> 跳App */
            }
        }
    }
}

// 启动标志 / 主流程
/* 读取并清除"升级请求启动"标志(RTC_BKP1); 返回 1=升级请求启动 0=普通上电 */
static uint8_t bl_check_upgrade_flag(void)
{
    uint8_t up;

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();          /* 使能备份域寄存器访问 */

    up = (BL_BKP_REG == BL_UPGRADE_MAGIC) ? 1U : 0U;
    if (up != 0U) {
        BL_BKP_REG = 0U;                /* 清除, 避免下次仍判为升级启动 */
    }
    return up;
}

void bootloader_main(void)
{
    uint8_t upgrade_boot = bl_check_upgrade_flag();

    s_log_on = upgrade_boot;            /* 普通上电静默, 升级启动才打印 */

    if (upgrade_boot != 0U) {
        bl_rs485_puts("using command to interrupt start Application\r\n");
    }

    //OLED显示
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0,  0, (u8*)"2026837587 ", 16);
    OLED_ShowString(0, 16, (u8*)"Bootloader ", 16);
    OLED_Refresh();

    if (upgrade_boot != 0U) {
        (void)bl_staging_erase();
        //升级会话
        for (;;) {
            if (bl_wait_window(BL_WAIT_UPGRADE_MS, 1U) == 0U) {
                break;   /* 倒计时超时无0x0502 -> 跳App */
            }
            if (bl_do_upgrade() == 0U) {
                break;   /* 升级执行/无固件/0503超时 -> 跳App */
            }
            (void)bl_staging_erase();
        }
        BL_LOG("[BL] jump to App @0x%08X\r\n", BL_APP_ADDR);
    } else {
        //上电485
        if (bl_wait_window(BL_WAIT_NORMAL_MS, 0U) != 0U) {
            (void)bl_staging_erase();
            (void)bl_do_upgrade();
        }
    }

    /* 跳转 App */
    bl_jump_to_app();

    /* App无效 */
    s_log_on = 1U;
    BL_LOG("[BL] App invalid, stay in bootloader for upgrade\r\n");
    while (1) {
        uint8_t  rx[256];
        uint8_t  rsp[64];
        uint16_t n, cmd, rl;
        //LED4_ON();  delay_1ms(100U);//debug
        //LED4_OFF(); delay_1ms(100U);
        n = rs485_recv(rx, sizeof(rx));
        if (n > 0U) {
            if (protocol_feed_cmd(rx, n, &cmd) != 0U) {
                if (cmd == CMD_OTA_PREPARE) {
                    (void)bl_staging_erase();
                    (void)bl_do_upgrade();
                    bl_jump_to_app();
                } else if (cmd == CMD_OTA_REQUEST) {
                    rl = protocol_build_ack(CMD_OTA_REQUEST, 1U, rsp, sizeof(rsp));
                    if (rl > 0U) { rs485_send(rsp, rl); }
                }
            }
        }
    }
}
/****************************End*****************************/
