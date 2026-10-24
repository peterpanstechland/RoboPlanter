# Indicator D1 植物精灵养成版

2026-10-24：养成 v3 初版收到用户“偶尔会抖动”“不确定，偶尔闪一下”的反馈。下文记录第一轮固定主体与 IRAM 刷新中断修订，其编译、上传、启动和 96 次存档压力测试通过；**用户随后反馈“仍偶尔闪一下”，因此第一轮未解决实屏问题。** 后续改用 VSYNC 双缓冲，最新结果见[双缓冲修订记录](indicator-vsync-double-buffer.md)。v2 的字体、亮度调整、Wi-Fi 扫描已有用户“三项都正常”的反馈，单独保存在[设置版本记录](indicator-settings-ui.md)。

## 第一轮修订镜像（历史记录）

| 项目 | 结果 |
| --- | --- |
| 设备 | SenseCAP Indicator D1，ESP32-S3 rev 0.2，8 MB Flash / 8 MB PSRAM |
| 串口 | CH340 COM13，VID:PID 1A86:7523；上传前重新识别 |
| 构建 / 上传 | `SUCCESS` / `Hash of data verified` |
| 应用文件 | `firmware/indicator/.pio/build/sensecap_indicator/firmware.bin` |
| 文件大小 | 1,333,312 B |
| SHA-256 | `31D1FFBAD1D52221F834680041C5B24E74665E6D44117BC993505E380472F492` |
| 静态 RAM | 47,832 / 327,680 B（14.6%） |
| 程序 Flash | 1,332,949 / 3,342,336 B（39.9%） |
| 构建日志 | `tmp/indicator-garden-stability-build-20261024.log` |
| 上传日志 | `tmp/indicator-garden-stability-flash-20261024.log` |
| 启动日志 | `tmp/indicator-garden-stability-boot-20261024.log` |

正常使用版启动后观察 130 秒，未见 panic，捕获结束后串口已关闭。日志确认：

```text
Indicator init: display=ready touch=ready client_task=ready preferences=ready
Indicator UI: Worker Smart Planter v3 / growing plant spirit / zh-en settings.
Indicator garden: storage=ready species=0 stage=1
Indicator settings: language=en brightness=80% storage=ready
```

本次重启恢复了已有薄荷萌芽阶段、英文与 80% 背光；未把主机预览的成熟样例写进设备，也未清空用户存档。初版镜像 SHA-256 为 `4E79A19A09873E8D397110CD238FD8CA286F1B4FC0DD85740AA10E1057310974`，初版上传/20 秒启动日志保留为 `tmp/indicator-garden-{flash,boot}-20261024.log`。

原有离屏 Canvas、12 MHz 像素时钟、RGB 引脚/时序、触摸映射和背光实现保留；底层 RGB 刷新中断改为本项目维护的实现。RP2040、XIAO 未刷写；完整原机备份仍留本地忽略目录，恢复方法见[首次刷写记录](indicator-first-flash.md)。

## 偶发闪屏修订与压力测试

种子和整株植物的离散位移已取消，主体保持固定；双眨眼改为每 5.3 秒一次、持续 260 ms，保留星光、种籽和照料庆祝粒子。主机新增 84 项位置/眨眼像素断言，覆盖四种植物的五阶段及原位移时间点。

在当前 Arduino ESP32 2.0.17 / ESP-IDF 4.4.7 中，预编译 RGB 驱动没有启用缓存关闭期间可用的中断，且其帧重启路径调用位于 Flash 的 GDMA 函数。NVS 写入可能推迟 VSYNC 帧重启。项目新增 `indicator_rgb_driver.cpp`，按[官方 4.4.7 驱动](https://github.com/espressif/esp-idf/blob/v4.4.7/components/esp_lcd/src/esp_lcd_rgb_panel.c)的单次 DMA 链/VSYNC 重启流程实现，整段中断调用链位于 IRAM/ROM，状态和 DMA 描述符放内部 RAM。通过 `Arduino_RGB_Display::begin()` 子类接入，不转换第三方私有对象结构。SDK 版本有编译期检查，附完整 Apache-2.0 许可证。

最终正常镜像的链接与反汇编检查确认：`restartFrame` 在 `0x40375428`、`onVsync` 在 `0x403754E4`，状态位于内部 DRAM `0x3FC99140`；外部调用仅涉及 IRAM 中的 `esp_timer_get_time`/systimer HAL，以及 ROM 延时。没有在中断内访问 PSRAM 像素、写日志、分配内存或调用原来的 Flash GDMA 控制函数。

独立 `sensecap_indicator_lcdtest` 构建的压力测试函数只在 `rp-lcd-test` 命名空间写入并读回 96 个变化的 156 B 数据块，结束时清理该命名空间；不覆盖 `rp-garden` 或用户设置。完整诊断镜像在该函数结束后仍恢复正常 UI/网络行为。记录的一轮结果：

```text
LCD flash stress: writes=96 failures=0 elapsed_ms=5359 longest_write_us=105862 frames=226 max_gap_us=23664 current_gap_us=15834
```

最长写入约 106 ms，整个测试中的最大 VSYNC 间隔约 23.7 ms，没有与长写入对应的刷新中断停顿。日志位于 `tmp/indicator-lcd-stress-{build,flash,boot}-20261024.log`。这些数字验证刷新中断连续性，不能代替肉眼检查像素是否闪动；Flash/PSRAM 共享总线带宽仍需实机观察。

测试完成后已刷回默认 `sensecap_indicator`。正常镜像检查确认不含压力测试入口/命名空间字符串；正常存档仍按操作后约 2 秒、运行每 60 秒保存，并仅输出存档耗时及刷新统计，不记录存档内容或网络凭据。

正常使用版又完成两轮自动检查点：运行到 60,303 ms 和 120,314 ms 时分别用 6,416 µs、6,399 µs 成功存档，观测到的最大 VSYNC 间隔为 23,847 µs。两次写入没有对应的帧重启停顿；`rgb_frames=0` 仅表示一次短写入落在相邻两个 VSYNC 之间，不能单独判作卡帧。

## 变化与验证

四种植物拥有独立进度、五个阶段与不同造型。主页增加精灵生命、心情、成长余量；照料页提供虚拟浇水、施肥、除草与陪伴，氛围页由用户选择工位心情。25 分钟专注完成给当前植物一次成长奖励。具体门槛、冷却与存档规则见[养成说明](../indicator-garden.md)。游戏动作不调用真实执行器。

- 模型与实际存档代码通过四类主机测试：normal 421、loaded 412、corrupt 421、unavailable 412 项断言；涵盖四株隔离、成长双门槛、冷却、时间回绕、掉电恢复、保存失败与每字节损坏校验。各场景包含重复执行的共同断言，不把相加结果当作独立功能数。
- 实际 `main.cpp` 的 40 项交互检查通过，覆盖原有硬件确认/STOP/设置，以及游戏面板隔离、选择、照料和专注奖励边界；44 项成长阶段样例与 84 项固定位置/眨眼像素校验通过。
- 493 字符在 16/24/32/48/64 px 的覆盖检查通过；6 项 Python 字体测试与 40 项原生字体检查通过。
- 原生预览生成 562 帧、9 个 GIF、17 张对照图。已查看中英页面、20 个物种/阶段组合与八套主题，来源哈希与当前代码一致。18 张设置预览与 v2 逐像素一致。
- 设备构建使用 Arduino ESP32 2.0.17。修复了主机 C++17 与嵌入式 C++11 对结构体初始化的差异，最终目标编译通过。

预览在 `build/indicator-preview-v3/`。`garden-growth.gif` 是每阶段 800 ms 的图鉴，实际成长使用分钟级在线时间和成长值，不以动画播放速度推进。主机预览替换存储、无线和硬件边界，不能证明真实 LCD 帧率、NVS 写入或外设动作。

## 待实机检查

1. 首页种子、生命条、心情与动作反馈是否正常，原设置与翻页是否保持可用。
2. 游戏浇水后成长值增加 8、重复点击有冷却；种子不能施肥，没有杂草时不重复奖励。
3. 切换四株再切回，进度是否保留；操作后等待至少 2 秒，再重启核对存档。
4. 第一株有效照料并累计在线 5 分钟后萌芽；完成 25 分钟专注只奖励一次，休息计时不奖励。
5. 长时间运行的画面稳定性，以及虚拟照料与今日状态真实硬件控制的隔离。

当前游戏存档只在 Indicator 本机，尚未通过 YOYO Skill/MCP 同步游戏操作。真实传感器、XIAO 和执行器链路按原项目验收计划继续验证。
