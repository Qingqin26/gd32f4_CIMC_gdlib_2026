/************************************************************
 * 文件：HeaderFiles.h
 * 平台: 2026 CIMC GD32F470VE
 * 描述: Bootloader 公共头文件
************************************************************/
#ifndef __HEADERFILES_H
#define __HEADERFILES_H

/************************* 头文件 *************************/
#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "systick.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Function.h"

/* ===== 模块 ===== */
#include "LED.h"          /* LED 状态 */
#include "rs485.h"        /* RS485 = USART1 */
#include "usart2.h"       /* 调试 = USART2 */
#include "intflash.h"     /* 内部 Flash 读写 */
#include "protocol.h"     /* 通信协议层 */
#include "bootloader.h"   /* Bootloader 流程 */
#include "OLED.h"

#endif
/****************************End*****************************/
