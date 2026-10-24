# 界面与接线预览

当前 Indicator 使用 Worker Smart Planter 像素界面。以下预览直接运行固件绘图代码，网络和硬件边界在本机替换为离线状态，不控制设备。GIF 时间来自主机采样，不是实机拍摄或实测帧率。

![像素盆栽首页动效](indicator-worker.gif)

[查看九屏总览](indicator-worker-overview.png)。更新原生预览可运行 `tools/render-indicator-ui.ps1`，生成位置为本机 `build/indicator-preview/`；使用说明见 [原生渲染器](../../tests/render/README.md)。

## 接线与早期交互原型

下载仓库后，用浏览器打开以下独立 HTML。GitHub 文件页展示源码，不直接运行界面；当前未启用 GitHub Pages 或其他公开托管。

- [接线与通信示意图](roboplanter-wiring.html)：查看 XIAO、Indicator、传感器与供电连接。
- [旧版 Indicator 桌边伙伴](indicator-companion.html)：早期五页导航与番茄钟交互原型，其主题与画面已被新版像素界面替代。

预览不会连接或控制真实硬件；照片与读数保持未接入状态。真实接线以 [接线文档](../wiring.md) 为准，实机功能与照片导入步骤见 [Indicator UI](../indicator-ui.md)。

`source/` 保存可编辑的原始片段，外层 HTML 是独立浏览器包装。界面布局用于交互设计预览，实际屏幕字形、刷新速度和触摸效果仍需实机验收。
