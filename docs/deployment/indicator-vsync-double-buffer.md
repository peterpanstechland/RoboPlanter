# Indicator D1：VSYNC 双缓冲修订

2026-10-24。用户确认[第一轮刷新中断修订](indicator-garden-ui.md)后“仍偶尔闪一下”。本次已构建并刷入完整帧双缓冲版本；串口与主机检查通过，用户随后反馈 **“暂时没有再闪，触摸正常”**。短期实屏观察通过，长期运行稳定性仍需后续使用验证。

## 改动与边界

原实现先画到 Canvas，再把 460,800 B 像素复制到 DMA 正在读取的显示缓冲区；复制期间存在读到半幅新内容的窗口。本次 Canvas 直接借用未显示的缓冲区，每帧完整重画，写回缓存后发布，VSYNC 中断才切换 DMA 链。中断完成重启后才交还旧显示缓冲区，取消整帧复制。

两个 480×480 RGB565 缓冲区仍占 921,600 B PSRAM，与原 Canvas 加显示缓冲区的总量相同；增加一条约 1,380 B 的内部 RAM 描述符链。提交与确认使用 32 位无锁原子操作。等待超过 100 ms 时不交还任何缓冲区，并停止界面绘制，避免继续写入归属不明的内存。

12 MHz 像素时钟、引脚、porch、屏幕方向、触摸映射、背光和 UI 绘图保持不变。IRAM/ROM 中断路径保留。游戏规则、存档格式及保存节奏不变，未清空 `rp-garden` 或用户设置；本次启动恢复薄荷枝叶阶段、中文和 80% 亮度。仅刷 ESP32 的 COM13，RP2040 与 XIAO 未操作。

双缓冲能消除 CPU 改写当前显示帧的窗口，但不能单独保证 Flash/PSRAM 总线在所有情况下都无争用；用户本轮短期观察未再出现闪屏。参考 [Espressif RGB LCD 双缓冲说明](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html)；本项目实际固定 SDK 仍是 Arduino 2.0.17 / ESP-IDF 4.4.7，并非升级到该文档版本。

## 已刷入的正常镜像

| 项目 | 结果 |
| --- | --- |
| 设备 | SenseCAP Indicator D1 / ESP32-S3 rev 0.2 / 8 MB Flash、8 MB OPI PSRAM |
| 串口 | COM13 CH340，VID:PID 1A86:7523 |
| 构建 / 上传 | `SUCCESS` / `Hash of data verified` |
| 文件 | `firmware/indicator/.pio/build/sensecap_indicator/firmware.bin` |
| 大小 | 1,324,672 B |
| SHA-256 | `63491D21FBB9807106C1FB38B358EEE43028245CB40A2C50B60337AE1F046349` |
| 静态 RAM | 47,776 / 327,680 B |
| 程序 Flash | 1,324,309 / 3,342,336 B |
| 本地日志 | `tmp/indicator-doublebuffer-{build,flash,boot}-20261024.log` |

启动确认显示、触摸、客户端任务和持久化均初始化成功，渲染器输出 `two framebuffers -> VSYNC swap; no full-frame copy`。正常镜像不含 `rp-lcd-test`、`LCD flash stress` 或压力测试函数。

正常镜像两轮自动存档分别在 60,294 ms、120,309 ms 完成，耗时 6,661 µs、6,673 µs。对应累计提交/交换为 421/421、844/844；交换超时、FIFO underflow、描述符错误及 `dma_no_eof` 均为 0，最大 VSYNC 间隔为 23,666 µs。短写入的 `rgb_frames=0` 仅表示它落在相邻两次 VSYNC 之间。

最终 ELF 中 `restartFrame` 位于 IRAM `0x40375424`，`onVsync` 位于 `0x403754E8`，状态位于 DRAM `0x3FC991A4`。反汇编确认原子操作内联为内存屏障与读写，ISR 外部调用仍仅涉及 IRAM 的计时器及 ROM 延时，未引入 Flash 中的原子辅助函数或 GDMA 控制调用。

## 存档与显示压力测试

独立 `sensecap_indicator_lcdtest` 镜像每次完整重绘网格和进度条，再提交画面、写入并读回 156 B 数据，共 96 次。测试函数只写 `rp-lcd-test`，结束清理该命名空间；诊断镜像随后恢复正常游戏行为。捕获的一轮结果：

```text
LCD flash stress: writes=96 failures=0 elapsed_ms=7891 longest_write_us=107842 frames=334 max_gap_us=23666 current_gap_us=4366 presents=96 swaps=96 timeouts=0 dma_udf=0 dma_desc=0 dma_no_eof=324
```

96 次提交均获 VSYNC 交换确认，96 次存档读回一致；最长写入约 108 ms，最大 VSYNC 间隔约 23.7 ms，未记录交换超时、FIFO underflow 或描述符错误。这验证了本轮握手、存档和中断连续性，不能证明全部像素始终无损。

`dma_no_eof` 是在 VSYNC 采样时未观察到 GDMA `TOTAL_EOF` 的次数，不是已确认的丢帧数，其具体原因尚未确定。描述符长度、对齐、尾链和 EOF 位已核查。[官方 4.4.7 RGB 驱动](https://github.com/espressif/esp-idf/blob/v4.4.7/components/esp_lcd/src/esp_lcd_rgb_panel.c)也在 VSYNC 重启 DMA 而不等待该标志；不能据此断言该计数必然正常。

压力日志为 `tmp/indicator-doublebuffer-stress-{build,flash,boot}-20261024.log`。测试后源码补充了“描述符错误零增量”的返回值条件，并重新构建成功，见 `tmp/indicator-doublebuffer-stress-final-build-20261024.log`；该单行判定修订未再次刷入诊断镜像，上述实测本身已记录 `dma_desc=0`。最终设备运行的是表中正常镜像。

## 主机检查

- 原生 UI 模型 99 项，养成模型/实际存档四场景分别 421、412、421、412 项断言通过，场景包含重复的共同断言。
- 实际 `main.cpp` 的 40 项交互、44 项成长样例、84 项主体固定/眨眼像素检查通过；新增 6 项模拟提交失败检查，确认失败后停止绘图、保留最后像素、仅记录一次故障并继续让出 CPU。
- 原生渲染导出 562 帧、9 个 GIF、17 张对照图；18 张设置图与 v2 逐像素一致，已有养成预览媒体也完全一致。最终源码哈希已更新，硬件驱动仅记录来源，主机未执行 LCD/DMA。

## 用户实屏反馈

用户对两项观察请求分别回复：

- 关于停在基本静止的设置页是否也会闪：“设置页面不会”。该问题发于双缓冲版本之前，回复未明确对应的固件版本，仅作为排查线索保留。
- 关于双缓冲版首页观察、游戏浇水及切换植物：“暂时没有再闪，触摸正常”。据此记录本轮短期显示与触摸检查通过，不推定所有游戏规则、掉电恢复或长时间运行已完成验收。

这次仅补充用户观察结果，没有重编译或再次刷写；设备镜像仍为上表版本。
