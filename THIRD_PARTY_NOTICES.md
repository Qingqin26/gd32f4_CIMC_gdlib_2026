# 第三方软件与材料声明

本仓库包含第三方软件和可能由赛事组织方提供的模板。根目录的 MIT License 仅适用于没有单独版权或许可声明的本项目原创部分，不替代下列内容原有的许可条款。

## 主要第三方组件

| 组件 | 仓库位置 | 权利人/来源 | 许可依据 |
| --- | --- | --- | --- |
| Arm CMSIS | `APP/CMSIS/`、`Bootloader/CMSIS/` | Arm Limited | 以源文件中的版权和许可声明为准 |
| GD32F4xx 标准外设库 | `APP/Library/GD32F4xx_standard_peripheral/`、`Bootloader/Library/GD32F4xx_standard_peripheral/` | GigaDevice Semiconductor Inc. | 以源文件中的版权和许可声明为准 |
| GD32 USB Library | `APP/Library/GD32F4xx_usb_library/`、`Bootloader/Library/GD32F4xx_usb_library/` | GigaDevice Semiconductor Inc. | 以源文件中的版权和许可声明为准 |
| FatFs | `APP/Library/Third_Party/fat_fs/`、`Bootloader/Library/Third_Party/fat_fs/` | ChaN | 以该目录 README 和源文件声明为准 |
| GD30AD3344 驱动 | 工程内相关 Driver/Library 文件 | GigaDevice 或原始供应方 | 以源文件中的版权和许可声明为准 |
| CIMC 赛事模板 | 含 CIMC 版权头或由赛事资料提供的文件 | 原赛事材料权利人 | 以赛事规则、材料声明和源文件版权头为准 |

## 使用与再分发

- 不得删除或篡改第三方源文件已有的版权、作者和许可声明。
- 修改第三方文件时，应清楚标明修改内容和日期，并继续遵守原许可。
- 若某文件的许可状态不明确，不应仅根据本仓库的 MIT License 推定其可再分发或商用。
- 后续加入的赛题 PDF、原理图、数据手册、图片和其他文档，必须在 `docs/README.md` 中注明来源、版本与授权状态。

本文件用于帮助识别第三方内容，不构成法律意见。若本文件与具体源文件的许可声明冲突，以具体文件声明和适用协议为准。
