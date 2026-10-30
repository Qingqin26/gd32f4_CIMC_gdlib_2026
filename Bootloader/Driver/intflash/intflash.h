/************************************************************
 * 文件名: intflash.h
 * 描述:   GD32F470 内部 Flash 读写驱动 (基于 FMC, 4KB 页擦除)
 *         GD32F470/F427/F425 才支持 4KB page erase, 可精确擦除单个页
 * 平台:   2026 CIMC GD32F470VE
 ************************************************************/
#ifndef __INTFLASH_H
#define __INTFLASH_H

#include "HeaderFiles.h"

/* 内部 Flash 页大小 (GD32F470 page erase 粒度 = 4KB) */
#define INTFLASH_PAGE_SIZE     0x1000U

/* 擦除一页 (page_addr 须 4KB 页对齐); 返回 0=成功 1=失败 */
uint8_t intflash_erase_page(uint32_t page_addr);

/* 读取内部 Flash (Flash 直接映射到地址空间 */
void    intflash_read(uint32_t addr, uint8_t *buf, uint32_t len);

/* 按字(4字节)写入内部 Flash (addr 须 4字节对齐; 调用前须先擦除目标页);
 * len 不足 4 字节倍数时, 末尾补 0xFF 凑齐一字; 返回 0=成功 1=失败 */
uint8_t intflash_write(uint32_t addr, const uint8_t *buf, uint32_t len);

#endif
/****************************End*****************************/
