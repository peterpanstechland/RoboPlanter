# Indicator：Wi-Fi 与设置齿轮比例修订

2026-10-24，按用户“感觉被拉长”的反馈重绘两个图标，已编译、刷入 COM13 并正常启动。

- 原齿轮着墨为 14×16 格，改为对称的 14×14 格、八个短齿和镂空中心。
- 原 Wi-Fi 着墨为 14×13 格，改为 14×10 格的双弧线和底点。
- 顶部两图均用 32 px 绘制框，以 2 倍整数像素显示，中心纵坐标统一为 34。设置菜单的 Wi-Fi 也改为 32 px，保持原中心；信息页复用新字形。
- 设置按钮仍为 44×44 px，交互、养成、存档与已通过短期实屏观察的双缓冲驱动未修改。

原生预览输出在 `build/indicator-preview-icons/`。已目视检查首页和设置页；与上一版逐像素比较，首页变化 472 像素、设置页变化 648 像素，均位于图标区域，其他区域差异为 0。[首页预览](../previews/indicator-current-home.png)、[设置页预览](../previews/indicator-current-settings.png)来自固件绘制代码，非实机拍摄。

已有原生渲染检查全部通过：40 项交互、44 项成长样例、84 项位置与眨眼、6 项提交失败检查。没有为这次外观微调增加新测试。生成 562 帧、9 个 GIF、17 张对照图，已更新仓库内当前养成预览。

| 构建 / 刷写 | 结果 |
| --- | --- |
| 环境 | `sensecap_indicator`，Arduino 2.0.17 / ESP-IDF 4.4.7 |
| 设备 | D1 / ESP32-S3 rev 0.2，COM13 / CH340 1A86:7523 |
| 编译 / 上传 | `SUCCESS` / `Hash of data verified` |
| 应用大小 | 1,324,672 B |
| SHA-256 | `C971E783FEBB0B1B4EA2F47A3CBAE22C84A9952C36BECAA61FBD053BEEBD93E1` |
| RAM / 程序 Flash | 47,776 B / 1,324,309 B |
| 日志 | `tmp/indicator-icons-{render,build,flash,boot}-20261024.log` |

启动捕获 20 秒：显示、触摸、客户端任务和持久化初始化成功，无 panic；恢复用户选择的多肉种子、中文及 80% 亮度。RP2040 与 XIAO 未刷写。本轮图标外观的实屏偏好仍以用户反馈为准；先前双缓冲版的“暂时没有再闪，触摸正常”记录保留在[显示修订记录](indicator-vsync-double-buffer.md)。
