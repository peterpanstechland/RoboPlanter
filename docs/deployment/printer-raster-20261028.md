# 2026-10-28 中文小票与切刀联调

实现提交：`378b037`。协议依据用户提供的 [escpos-sample](https://github.com/peterpanstechland/escpos-sample/tree/60f85d4e8cbcce8956528c7a2d1d0344aa2adf55)。使用说明见 [UART 打印教程](../handbook/uart-printer.md)。本记录不包含设备令牌或 Wi-Fi 凭据。

## 软件检查

- XIAO 默认禁用外设、现场私有配置（D8 UART、115200 波特、切刀开启、10 颗灯带）均构建成功。现场版本 RAM 80,828 bytes，Flash 使用 1,966,917 bytes。
- 核心控制/光栅编码测试 493 项；字体覆盖、像素一致性和可复现生成测试 3 项通过。7,540 字符字库，384 点宽，32 行分带，最多 160 UTF-8 字节、8 行正文。
- 本地 MCP 27 项测试及 6 项灯效保护回归通过；配对工具 6 项、公开源码包 5 项测试通过。
- Indicator 设置 118 项、启动失败 84 项、STOP 隔离 50 项、打印/网络 144 项、设置界面 43 项检查通过。主程序离线渲染、帧交接和字体检查通过，中英小票和连接诊断页已检查无溢出。
- UART 写入或排空失败锁定后续打印；`completed`、`bytes_sent` 仅表示软件发送完成。缺纸、过热、机械完成状态尚未实现，不能从软件回包推断切纸成功。

## XIAO 实机

- 核对 ESP32-S3 Plus 身份和 16MB 分区后，完整备份 16MB。备份 SHA-256：`343cfccf8e512f70f3898edbb60ca9e05362871b11384c712575229e63f53650`。
- 确认 app0 已使用、app1 为空，仅向 `0x10000` 写入应用，不写 bootloader、分区表、NVS 或其他分区。写入后 esptool 哈希校验成功。
- 应用文件 1,967,280 bytes，SHA-256：`28bcbdd106827aa1683a0e3fd5c71666bd737c74868abaa4e3efb22fb3f68517`。私有固件和备份保留在 Git 忽略的 `build/device-backups/xiao-20261028-printer/`。
- 启动后原 Wi-Fi、设备令牌正常；鉴权状态返回 `text_format:utf8-raster`、`paper_width_dots:384`、`autocut:true`、`feedback:unavailable`。灯带能力仍启用，水泵仍禁用，Indicator 土壤中继配置保留。
- 请求 `cn-cut-7a8235d56f144529` 发送中文测试小票，记录到 `accepted → completed`。用户随后发现接线有误，明确要求接好后再试；第一轮不计为物理打印通过。
- 用户授权的第二轮使用新请求 `cn-cut-4fddca30708d4fa9`，同样记录到 `accepted → completed`。没有自动重发；用户明确确认“中文清晰，已自动切断”，本轮中文出纸和切刀通过。

## Indicator 实机

本轮核对为 ESP32-S3、8MB Flash。高速完整备份遇串口包噪声后停止，改为低速分块读取。前 2MB 已保存，包含 bootloader、分区表、NVS 和完整原应用；本轮没有得到完整 8MB 备份。2026-10-24 的完整备份仍保留。

原应用已核对：土壤中继启用、双帧缓冲、未编入照片。NVS 包含中文、80% 亮度、已配对 Wi-Fi、主题和植物存档。私有连接配置通过一次鉴权设备身份查询生成，保留 `PLANTER_ENABLE_LOCAL_SOIL=1`；私有构建成功（RAM 48,056、Flash 1,350,601 bytes）。

- 备份中原 app 的内部 checksum 和 validation hash 均有效，设备 `0x10000..0x1FFFFF` 与备份通过 `verify_flash` 校验。新应用擦写范围 `0x10000..0x159FFF` 完全在本次备份范围内。
- 115200 波特率仅写入新应用，1,350,960 bytes，esptool 哈希校验成功。应用 SHA-256：`404b9fed5bc269699e1ab41af25272179c7722b9501d4424e66b21305aae7ec1`。
- 启动后 `display/touch/client_task/preferences=ready`，双帧缓冲 VSYNC 正常启用。中文、80% 亮度、原植物存档 `species=0 / stage=2` 保留，RP2040 未写入。
- 实际固件及各目标文件均核对包含新的配对字段；私有值未输出。随后串口出现 `Indicator soil relay: HTTP 200`，XIAO 实际返回新的 `indicator_rp2040_grove_adc` 原始读数，证明两板鉴权链路工作。读数尚未标定，不解释为湿度百分比。
- 屏幕产生的 `screen-` 命令已出现在 XIAO 的完成记录中。用户随后确认从 Indicator 发起打印后正常出纸、切纸，端到端链路通过；同时反馈文字看起来被压缩，排版效果待进一步核对，不能把链路通过等同于视觉验收通过。
