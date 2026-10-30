# APP

主应用程序运行于 GD32F470VE，链接地址为 `0x08011000`，预留空间为 128 KiB。系统启动后完成外设初始化，并通过 RS485 协议处理数据采集、参数配置、DAC 控制和固件升级请求。

## 关键入口

- `User/main.c`：应用入口和主循环
- `Protocol/`：RS485 帧解析、命令处理与响应
- `Function/`：采集、参数、RTC 等业务逻辑
- `Driver/`：ADC、DAC、GD30AD3344、OLED、RS485、Flash 等驱动
- `Project/GD32_CIMC_GdLib_Code.uvprojx`：Keil MDK 工程
- `Project/Objects/GD32_CIMC_GdLib_Code.hex`：仓库保留的参考固件

## 构建与烧录

使用 Keil MDK 打开工程并执行 Build。APP 必须链接并烧录到 `0x08011000`，不能覆盖位于 `0x08000000` 的 Bootloader 或位于 `0x08010000` 的参数区。

完整开发环境、Flash 布局和固件校验值见仓库根目录的 [`README.md`](../README.md)。
