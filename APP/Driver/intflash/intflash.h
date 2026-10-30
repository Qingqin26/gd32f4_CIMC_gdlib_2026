/************************************************************
 * 文件名: intflash.h
 * 功能:   GD32F470 内部 Flash 读写驱动 (基于 FMC, 4KB 页擦除)
 *         GD32F470/F427/F425 额外支持 4KB page erase, 可精确擦除参数页
 *         而不影响相邻区域(普通 GD32F4 只能按扇区擦)
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#ifndef __INTFLASH_H
#define __INTFLASH_H

#include "HeaderFiles.h"

/* 内部 Flash 页大小-参数页 */
#define INTFLASH_PAGE_SIZE     0x1000U

/* 擦除一页 */
uint8_t intflash_erase_page(uint32_t page_addr);

/* 读取内部 Flash */
void    intflash_read(uint32_t addr, uint8_t *buf, uint32_t len);

// 按字写入内部 Flash 
uint8_t intflash_write(uint32_t addr, const uint8_t *buf, uint32_t len);

#endif
/****************************End*****************************/
