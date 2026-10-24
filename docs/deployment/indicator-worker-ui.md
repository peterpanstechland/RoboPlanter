# Indicator Worker 像素界面记录

2026-10-24，按用户提供的 Worker Smart Planter 参考图重做原版 SenseCAP Indicator 的界面。**当前已完成构建、上传哈希校验、20 秒启动观察及本机绘图/交互检查，实机画面、动效与操作复测待用户确认。** 此记录对应 Worker 像素版本；更早的首次刷写、Canvas 修复和完整 Flash 备份恢复步骤保留在[首次刷写记录](indicator-first-flash.md)。

## 当前镜像

| 项目 | 结果 |
| --- | --- |
| 目标 | 原版 Indicator ESP32-S3，8 MB Flash / 8 MB PSRAM |
| 环境 | `sensecap_indicator`；espressif32 6.12.0 / Arduino ESP32 2.0.17 |
| 构建 | `SUCCESS` |
| 上传 | CH340 COM13；`Hash of data verified`，`SUCCESS` |
| 应用文件 | `firmware/indicator/.pio/build/sensecap_indicator/firmware.bin` |
| 文件大小 | 862,032 B |
| SHA-256 | `505F5CAB5533B1BA54DBFB050C04A6739A6043BA6DCAF02709EEE94EEFC2ADBD` |
| 静态 RAM | 46,160 / 327,680 B（14.1%） |
| 程序 Flash | 861,665 / 3,342,336 B（25.8%） |
| 构建/上传日志 | `tmp/indicator-worker-flash-20261024.log` |
| 启动观察 | 20 秒，四项初始化均 ready，未见 panic；串口已关闭 |
| 启动日志 | `tmp/indicator-worker-boot-20261024.log` |
| 物理画面与触摸复测 | 待用户确认 |

COM13 是此次识别出的 ESP32 CH340 端口，之后操作仍须重新识别。RP2040 和 XIAO 未刷写。镜像仍为空相册和占位网络配置，没有以模拟传感器数据填充画面。

本次启动日志包含：

```text
Indicator init: display=ready touch=ready client_task=ready preferences=ready
Indicator renderer: offscreen canvas -> complete frame copy.
Indicator UI: Worker Smart Planter / pixel companion / animated.
```

20 秒观察已完整结束，窗口内未见 panic，随后关闭了串口。这证明当前 Worker 固件启动到初始化完成，不等同于用户确认画面、动画或触摸效果；网络任务 ready 也不表示花盆已连接。

## 界面与交互变化

主角改为代码原生绘制的像素 CRT 电脑盆栽：浅色机壳、黑色屏幕、可爱眼睛与腮红、顶部双嫩芽和底座。背景使用黑炭色，搭配奶油白卡片、鼠尾草绿、淡紫和水蓝；加入杯子、书堆、像素图标和纸条布局。八套配色保持同一角色家族：工位花园、紫色午后、薄荷微风、暖光书桌、夜航工位、莓果便签、苔绿书房、像素街机。

眨眼、叶摆、轻微呼吸、星点和杯口蒸汽由时间驱动，是装饰动画。更新间隔最低 100 ms，上限约 10 fps；实际刷新速度受绘制、PSRAM 访问及整帧复制耗时影响，没有声称实测达到固定帧率。普通静态页面按 1 秒刷新，触摸事件可即时重绘。

保留五页导航、主题持久化、照片和本地番茄钟。浇水和打印增加确认页，最后确认时重新检查新鲜度、网络与设备能力；取消或滑动关闭不发送命令。停泵无需二次确认。浇水按配置时长显示，不编造毫升；打印预览为实际 ASCII 便签内容，完成仍只表示串口字节发送结束。缺失读数保持未知，不照搬参考图中的水位、光照百分比、日期时间或手机在线状态。

继续使用离屏 Canvas：在 460,800 B 的独立 PSRAM 画布完成整帧，再复制至同样大小的 RGB 扫描缓冲。两个缓冲合计 921,600 B，属于运行时内存，不计入静态 RAM 数字。它不是 VSYNC 同步换帧。

## 已有验证及边界

- 99 项可移植 UI 模型检查通过，覆盖手势隔离、页边界、计时暂停/回绕与主题独立性。
- 11 项实际 `main.cpp` 交互处理断言通过，覆盖确认页取消/发送和相关交互边界。
- 346 字符的当前字库覆盖检查通过。
- 原生预览直接运行生产绘图代码和固定版本 Arduino_GFX 栅格算法，检查了八主题首页、主题缩略图、状态/信息/番茄钟/操作页、浇水与打印确认页，以及睁眼/闭眼帧；没有发现裁剪或缺失字形。
- 构建及 COM13 上传已成功，上传的数据哈希校验通过。
- 新版 20 秒启动观察完成，四项初始化为 ready，确认 Worker UI 标识，观察窗口内未见 panic。

预览输出位于本机 `build/indicator-preview/`，包含 `theme-0-face.png`、`all-themes-face.png`、`all-themes-themes.png` 和 `theme-0-work-garden.gif` 等。`manifest.json` 记录绘图来源文件 SHA-256、输出及限制。可按[界面说明](../indicator-ui.md)运行 `tools/render-indicator-ui.ps1` 重生成；这些构建输出不随 Git 提交。

PNG/GIF 使用未连接的未知设备状态；GIF 按主机 100 ms 采样导出，不代表实机帧率。此验证不模拟 LCD 时序、PSRAM/DMA、背光或触摸硬件，也不触发真实花盆动作。

旧界面的 Canvas 修复版已有用户“频闪消失，触摸正常”的实测反馈，保留为历史依据。当前 Worker 像素界面增加了新绘制内容和确认流程，需要独立复测；尚未把画面观感、动画稳定性、触摸操作或真实联网功能标为通过。

## 下一步实机检查

1. 确认默认像素电脑、嫩芽、杯子和书堆完整，中文清晰，动画无明显闪烁或撕裂。
2. 检查八种配色、主题缩略图和左右滑动；触摸抬起才响应，滑动不误点按钮。
3. 检查浇水/打印确认页取消、返回与滑动关闭；设备不可用时不能发送动作。
4. 检查番茄钟跨页、换主题后继续计时，主题重启后保留。
5. XIAO 配置完成后再验收真实读数、离线/过期与未知结果处理，以及安全台架上的动作执行。

完整刷前备份仍仅保存在本地忽略目录，不随当前固件发布，不需要重新读取或覆盖。恢复方法见[首次刷写记录](indicator-first-flash.md#从本地备份恢复-esp32)；本次界面升级没有执行恢复。
