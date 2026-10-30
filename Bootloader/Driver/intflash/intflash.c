/************************************************************
 * 文件名: intflash.c
 * 描述:   GD32F470 内部 Flash 读写驱动实现 (FMC)
 *         - 擦除: fmc_page_erase 4KB 页擦除
 *         - 写入: fmc_word_program 按 32 位字编程
 *         - 读取: 内部 Flash 直接映射, 指针读取
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#include "intflash.h"

#define INTFLASH_CLEAR_FLAGS()  \
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | \
                   FMC_FLAG_PGMERR | FMC_FLAG_PGSERR | FMC_FLAG_RDDERR)

/************************************************************
 * 函数: intflash_erase_page
 * 描述: 擦除内部 Flash 一页
 * 参数: page_addr - 页起始地址
 * 返回: 0=成功, 1=失败
 ************************************************************/
uint8_t intflash_erase_page(uint32_t page_addr)
{
    fmc_state_enum state;

    fmc_unlock();                 /* 解锁 FMC */
    INTFLASH_CLEAR_FLAGS();
    state = fmc_page_erase(page_addr);
    fmc_lock();                   /* 操作完锁定 FMC */

    return (FMC_READY == state) ? 0U : 1U;
}

/************************************************************
 * 函数: intflash_read
 * 描述: 读取内部 Flash 数据(Flash 已映射到地址空间, 直接拷贝)
 ************************************************************/
void intflash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (buf == NULL) {
        return;
    }
    memcpy(buf, (const void *)addr, len);
}

/************************************************************
 * 函数: intflash_write
 * 描述: 按字写入内部 Flash(调用前须先擦除目标页)
 * 参数: addr - 目标地址(须4字节对齐)
 *       buf  - 源数据
 *       len  - 字节数(不足4字节倍数时末尾补0xFF)
 * 返回: 0=成功, 1=失败
 ************************************************************/
uint8_t intflash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t words = (len + 3U) / 4U;   /* 向上取整为整字 */
    uint32_t i;
    uint8_t  ret = 0U;

    if (buf == NULL) {
        return 1U;
    }

    fmc_unlock();
    INTFLASH_CLEAR_FLAGS();

    for (i = 0U; i < words; i++) {
        uint32_t w = 0xFFFFFFFFU;       /* 未满一字部分保持 0xFF(擦除态) */
        uint32_t nbyte = len - i * 4U;
        if (nbyte > 4U) {
            nbyte = 4U;
        }
        memcpy(&w, buf + i * 4U, nbyte);
        if (FMC_READY != fmc_word_program(addr + i * 4U, w)) {
            ret = 1U;                   /* 编程失败, 终止 */
            break;
        }
    }

    fmc_lock();
    return ret;
}
/****************************End*****************************/
