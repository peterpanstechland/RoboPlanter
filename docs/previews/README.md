# 界面与接线预览

当前 Indicator 使用 Worker Smart Planter 像素界面，本轮增加中英文、Wi-Fi 和亮度设置，并改善原生字号。以下预览直接运行固件绘图代码，网络和硬件边界在本机替换为离线状态，不控制设备。GIF 时间来自主机采样，不是实机拍摄或实测帧率；刷写与物理验收进度见[设置版本记录](../deployment/indicator-settings-ui.md)。

![像素盆栽新版首页动效](indicator-settings.gif)

[查看新版主页面总览](indicator-settings-main.png)与[设备设置页](indicator-settings-menu.png)。更新原生预览可运行 `tools/render-indicator-ui.ps1`，默认生成位置为本机 `build/indicator-preview-v2/`；使用说明见[原生渲染器](../../tests/render/README.md)。原 `build/indicator-preview/`、[第一版动画](indicator-worker.gif)和[第一版总览](indicator-worker-overview.png)保留为历史对照。

新版目录包含 `zh-main-overview.png`、`en-main-overview.png`、`zh-settings-overview.png`、`en-settings-overview.png`，覆盖中英文五页、主题、操作确认、设置、Wi-Fi 空列表、关于与键盘。17 项真实主界面交互断言覆盖原来的操作验证和设置防穿透；另有独立设置 UI、字体及模型测试。字体为 421 字符、五种原生字号。

设置截图中的后端、存储和背光是主机模拟；网络名仅用 `example` 样例，没有真实扫描、密码或外网连接。`manifest.json` 标记这些样例，并记录生产 UI、字体、渲染器和测试边界的来源哈希。预览中的 80% 亮度只是数值，不证明实体屏幕的亮度已验证。

## 接线与早期交互原型

下载仓库后，用浏览器打开以下独立 HTML。GitHub 文件页展示源码，不直接运行界面；当前未启用 GitHub Pages 或其他公开托管。

- [接线与通信示意图](roboplanter-wiring.html)：查看 XIAO、Indicator、传感器与供电连接。
- [旧版 Indicator 桌边伙伴](indicator-companion.html)：早期五页导航与番茄钟交互原型，其主题与画面已被新版像素界面替代。

预览不会连接或控制真实硬件；照片与读数保持未接入状态。真实接线以 [接线文档](../wiring.md) 为准，实机功能与照片导入步骤见 [Indicator UI](../indicator-ui.md)。

`source/` 保存可编辑的原始片段，外层 HTML 是独立浏览器包装。界面布局用于交互设计预览，实际屏幕字形、刷新速度和触摸效果仍需实机验收。
