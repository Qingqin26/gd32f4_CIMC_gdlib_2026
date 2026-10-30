# Bootloader

Bootloader 运行于 GD32F470VE 的 Flash 起始区域，负责 APP 有效性检查、跳转、备份恢复以及通过 RS485 接收升级固件。

## 关键入口

- `User/main.c`：启动入口
- `Function/bootloader.c`：启动判断、升级和 APP 跳转流程
- `Protocol/`：RS485 升级协议与 HEX 数据处理
- `Driver/`：Flash、OLED、RS485 等底层驱动
- `project/GD32_CIMC_GdLib_Bootloader.uvprojx`：Keil MDK 工程
- `project/Objects/CIMC_GD32_Template.hex`：仓库保留的参考固件

## 构建与烧录

使用 Keil MDK 打开工程并执行 Build。Bootloader 链接并烧录到 `0x08000000`，预留空间为 64 KiB。升级或整片擦除前应保护 `0x08010000` 的参数区，并确认 APP 位于 `0x08011000`。

完整开发环境、Flash 布局和固件校验值见仓库根目录的 [`README.md`](../README.md)。
