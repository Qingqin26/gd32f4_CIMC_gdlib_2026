# gd32f4_CIMC_gdlib_2026

基于 GD32F470VE 的 2026 年西门子杯中国智能制造挑战赛（CIMC）工业嵌入式系统开发方向参赛固件，包含应用程序与 Bootloader。

**参赛队伍编号：`2026837587`**

> [!IMPORTANT]
> 本仓库是竞赛项目的工程归档与交流版本，不代表赛事主办方或芯片厂商的官方实现。烧录前请核对目标芯片、Flash 分区和硬件版本。

## 功能概览

- GD32F470VE 裸机应用程序
- RS485 半双工通信与自定义帧协议
- 内置 ADC、DAC 和 GD30AD3344 外部 ADC 支持
- RTC、参数持久化和 OLED 状态显示
- 独立 Bootloader 与 RS485 固件升级流程
- APP、备份区和升级暂存区的 Flash 分区管理

## 仓库结构

```text
.
|-- APP/                 # 主应用程序
|   |-- Driver/          # 板级与外设驱动
|   |-- Function/        # 业务逻辑
|   |-- Protocol/        # 通信协议
|   |-- Project/         # Keil MDK 工程
|   `-- User/            # 应用入口
|-- Bootloader/          # 引导与固件升级程序
|   |-- Driver/
|   |-- Function/
|   |-- Protocol/
|   |-- project/         # Keil MDK 工程
|   `-- User/
|-- docs/                # 赛题、硬件、协议和图片资料
`-- .github/             # Issue 与 Pull Request 模板
```

## 开发环境

本项目原始开发环境如下：

- MCU：GigaDevice GD32F470VE（Arm Cortex-M4）
- IDE：Keil MDK / uVision 5.41
- Device Family Pack：工程文件引用 `GigaDevice.GD32F4xx_DFP 3.0.0`；原开发记录为 `3.0.3`
- 编译器：Arm Compiler 5（工程工具链标识为 `ARM-ADS`）

不同版本的 Keil 或 DFP 可能会更新工程配置。首次打开工程后，请确认目标器件、编译器和下载算法没有被自动替换。

## 构建

1. 安装 Keil MDK，并在 Pack Installer 中安装 GD32F4xx DFP。
2. 打开 `Bootloader/project/GD32_CIMC_GdLib_Bootloader.uvprojx`，执行 Build。
3. 打开 `APP/Project/GD32_CIMC_GdLib_Code.uvprojx`，执行 Build。
4. 检查 Build Output，确保链接地址与下表一致。

构建产物默认位于各工程的 `Objects/` 目录。仓库保留当前比赛版本的 HEX 参考固件，其他目标文件、链接文件和构建日志仍由 `.gitignore` 排除。正式版本仍建议通过 GitHub Releases 发布，并附带版本号、构建环境和校验值。

Keil 的 `.uvoptx` 文件也随工程保留，用于复现原有调试与下载配置。不同电脑首次打开工程时，Keil 可能会更新其中的本地选项；提交前应确认变更是否确有必要。

## Flash 布局

| 区域 | 起始地址 | 大小 | 用途 |
| --- | --- | ---: | --- |
| Bootloader | `0x08000000` | 64 KiB | 启动、校验和升级 |
| 参数区 | `0x08010000` | 4 KiB | 设备参数与设备 ID |
| APP | `0x08011000` | 128 KiB | 主应用程序 |
| APP 备份区 | `0x08031000` | 128 KiB | 固件备份 |
| 固件暂存区 | `0x08051000` | 128 KiB | 升级数据暂存 |

> [!WARNING]
> APP 不是从 Flash 默认基址启动。不要将 APP 链接或烧录到 `0x08000000`，否则会覆盖 Bootloader。参数区也不应随整片擦除流程被意外改写。

建议先烧录 Bootloader，再将 APP 烧录至 `0x08011000`。具体下载设置以两个 Keil 工程中的 IROM 配置为准。

## 预编译固件

仓库中的 HEX 文件是当前开源版本的参考构建产物：

| 固件 | 文件 | 烧录地址 | SHA-256 |
| --- | --- | --- | --- |
| Bootloader | [`Bootloader/project/Objects/CIMC_GD32_Template.hex`](Bootloader/project/Objects/CIMC_GD32_Template.hex) | `0x08000000` | `E28EBDB910C50697E6E44AE3DD4F43ED39EB30FA59ED1BDF401D7663B890A3C9` |
| APP | [`APP/Project/Objects/GD32_CIMC_GdLib_Code.hex`](APP/Project/Objects/GD32_CIMC_GdLib_Code.hex) | `0x08011000` | `DD53CAC305A5AAF9ADBEBDD38083363CDDC3212A9FDC49C5A0D9DAFC288990CA` |

SHA-256 对应当前仓库内文件；重新构建固件后需要同步更新本表。烧录前仍应核对硬件版本和链接地址。

## 文档与赛题材料

后续 PDF 赛题、硬件原理图、协议说明和图片统一放在 [`docs/`](docs/README.md)，不要散放在源码目录。提交第三方或赛事材料前，请确认版权方允许公开再分发，并在文档索引中记录来源和版本。

较大的二进制文档会快速增大 Git 历史。单个文件较大或经常更新时，建议使用 Git LFS 或放到 GitHub Release。

## 已知限制

- 本项目面向特定竞赛硬件，未验证其他 GD32F4 型号或开发板。
- 项目当前没有自动化硬件测试；修改 Flash、时钟、通信或升级代码后应在实物上回归验证。
- 部分源文件来自厂商 SDK、CMSIS、FatFs 或赛事模板，不能视为本项目原创代码。

## 许可证

除源文件、目录或第三方声明另有规定外，本项目原创代码采用 [MIT License](LICENSE)。ARM CMSIS、GigaDevice 固件库、FatFs 及其他第三方内容继续适用各自的版权和许可条款，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

赛事题面、原理图、芯片手册等后续加入的文档不因存放在本仓库而自动采用 MIT 许可证；其权利归原版权方所有。

## 贡献与安全

提交问题或代码前请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。涉及潜在安全问题或固件升级缺陷时，请按 [SECURITY.md](SECURITY.md) 中的方式报告，避免直接公开可利用细节。
