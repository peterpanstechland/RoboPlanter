# 工位搭子：公开展示、手册与复现包

2026-10-25。技术项目名保持 RoboPlanter，对外表达为「工位搭子」「你先下班，我负责发芽。」

## 发布入口

- 展示页：<https://peterpanstechland.github.io/roboplanter/>
- 项目手册：<https://peterpanstechland.github.io/docs/projects/2026/roboplanter/>
- 固件源码复现包：<https://peterpanstechland.github.io/roboplanter/downloads/roboplanter-starter.zip>
- 离线 Device Kit：<https://peterpanstechland.github.io/roboplanter/downloads/roboplanter-device-kit.zip>
- 下载哈希清单：<https://peterpanstechland.github.io/roboplanter/downloads/manifest.json>
- 站点 PR：<https://github.com/peterpanstechland/peterpanstechland.github.io/pull/49>
- 构建/部署：<https://github.com/peterpanstechland/peterpanstechland.github.io/actions/runs/40181805301>

本次站点合并提交为 `42d451be0f064f95f794226b10349f053a960c6a`，项目实现提交为 `205177b`。原文档站工作区的未提交改动没有带入本次发布。

## 新增内容

11 篇手册覆盖产品设计思考、接线/备份/烧录、荣耀平台、BLE、通用硬件、UI/UX、社媒脚本、证据路线、验收与排障。内容同步到站点两个语言路径；目前这些项目教程的正文均为中文，并非两套已翻译内容。

展示页使用本地托管的 Three.js 0.180.0，提供程序化 3D 角色、四种心情、模拟浇水、粒子定格环绕和 1200×1600 / 1600×900 PNG 导出。Gaussian falloff 点精灵不等于实物 3D Gaussian Splatting 重建。网页不连接或控制硬件。

Device Kit 提供显式 profile、三套样例、离线登记/检查/退役。XIAO 契约已有对应实现，通用 BLE 是实验模板，灌溉是规划。任何样例通过都不能启用真实动作或证明 YOYO 已绑定设备。

## 公共源码范围

经用户明确授权公开经筛选的固件源码复现包；完整开发仓库继续保持私有。`tools/public-starter-files.json` 逐文件列出 126 个公开源文件。拒绝本地配置、原始附件、Flash 备份、凭据目录、私钥和非占位配置；保留字体及驱动的第三方许可证。

最终源码包有 129 个 ZIP 成员（126 个源文件与 3 个引导/清单文件），580,783 字节：

```text
roboplanter-starter.zip
SHA-256 09e3902cc396855a4b9e12c8f7b567c4cfea36116f4f971ead1345a110cd147a

roboplanter-device-kit.zip
SHA-256 034ef4588a63bb10dce491438dad95ff139c9044902a63463685543741e7188f
```

生成工具不改变仓库可见性或引入统一开源许可证。公开包不包含编译镜像；使用者依照教程核对板型、构建、备份后再刷写。

## 验证与范围

- 新增 Device Kit 14 项、打包隐私/哈希/可复现/旧文件清理 5 项、本地 MCP 非字符串结果回归 1 项通过。
- 最终源码包独立解压后，267 项控制核心、99 项 UI 模型、14 项 Device Kit、5 项打包检查通过。
- Docusaurus 的 `en` 和 `zh-Hans` 全站构建通过；保留已有依赖的动态 require 警告，未修改全局断链校验策略。
- 浏览器检查桌面及 390px 布局、模拟浇水反馈与冷却、定格/恢复、PNG 下载和手册导航；导出图像尺寸已核对。未测实体手机的 GPU 帧率。
- 已只读核对荣耀当前技能基本信息及文件；未修改平台草稿、硬件门禁或发布状态。

本次未烧录任何板卡、未操作水泵或打印机、未证明手机 BLE 已打通。后续关键项仍是可信凭据注入、手机长写/通知与业务回包，以及实物台架验证。

## 部署验收

GitHub Actions `40181805301` 的 Build Docusaurus 与 Deploy to GitHub Pages 均为 `success`。已在浏览器打开正式展示页与项目手册，确认标题、3D 渲染、导航与 11 篇目录存在。

已从官网链接下载 `roboplanter-starter.zip`：580,783 字节，SHA-256 与上表相同，129 个 ZIP 成员完整性检查通过。Device Kit 的本地发布归档及清单哈希也已核对。浏览器下载过程中控制工具曾超时，但文件已落盘，随后由文件哈希完成确认；不把工具超时当作下载失败或成功的唯一依据。

## WS2812 增量发布（2026-10-25）

实现提交：`f4bfd9b`；长期运行相位补丁：`5ec394d`。公开教程现在共 12 篇（两个语言路径的正文仍均为中文）。

- 固件四模式：solid、breath、chase、rainbow；默认关闭，旧开关命令兼容。
- 灯帧独立 priority-1 任务，每 40 ms 更新；LED show 不持有泵控制器锁。亮度仍使用编译期限制；单灯 chase 退为 breath。
- HTTP/BLE、CLI、本地 MCP、Device Kit 描述同步；BLE 查询维持 `L:enabled:on`。YOYO Skill 硬件门禁与云写禁用未改变。
- 展示页加入五状态灯效预览，390px 下控件改为两列；预览及导出图仍是概念画面，不控制实体板。
- 318 项控制器、13,453 项灯帧模型、99 项 Indicator UI 检查通过。灯帧模型 ASan/UBSan 通过，环境不支持的 LeakSanitizer 未开启。
- MCP 21 项、CLI 11 项、灯效 bridge 6 项、Device Kit 15 项、结果校验 1 项、打包 5 项，共 59 项 Python 测试通过。MCP 使用隔离 Python 3.13.5 与项目固定依赖；传输测试只连接本机模拟设备。
- 双语言 Docusaurus 全站构建通过。网页桌面/390px 布局、五种模式循环和控制台错误已检查。
- 公开归档包含 130 个源文件与 3 个引导/清单文件；源文件逐项哈希校验通过。控制器与灯帧测试也已从归档独立解压编译执行。

本次最终归档：`roboplanter-starter.zip`，597,163 字节，SHA-256 `96f9e01bbc022a77e8bc6ab07cc6f6f323fac2ac51c0dee13846f90c8df7db92`。Device Kit SHA-256 `ee76869371d48460a286d3b43581453696bdc1cb0683c6f6c28ee68915389b56`。

站点更新 PR：[#50](https://github.com/peterpanstechland/peterpanstechland.github.io/pull/50)、[#51](https://github.com/peterpanstechland/peterpanstechland.github.io/pull/51)，分别合并为 `af2e5449aaf48cce25e96b3f564c3748c188656f`、`dc4c5ef1b26c7b6ecb02d2fbb201ddbf0954385f`。总览旧文案修正在 [#52](https://github.com/peterpanstechland/peterpanstechland.github.io/pull/52) 合并为 `b6c70139ff6e5fd782cc2c1043ec0ad7b1e6d721`；对应私有源码提交 `b3e7913`。最终 Pages 发布与新版 XIAO 编译结果另在下方记录。

没有向实体设备烧录、没有实际灯光或供电测试，也没有在荣耀手机上执行 BLE 命令。软件模型测试不能作为灯光观感、实体帧率或泵截止延迟的证据。

### 新版 XIAO 实际构建

固定 `espressif32@6.12.0`、官方框架 `3.20017.241212+sha.dcc1105b`（Arduino 2.0.17），框架下载经过官方摘要核对。在两个不含私有配置的 `/tmp` 源码副本构建，源代码与仓库一致，两组均无 warning/error：

- 默认全部禁用：RAM 79,740 / 327,680 B，Flash 1,361,633 / 6,291,456 B；应用镜像 1,362,000 B，SHA-256 `fea81fae2a4077d9451004d0bf674b4b050c688a5f78973255e8150c4f3b8f45`。
- 仅编译灯外设启用、24 颗灯：RAM 79,788 B，Flash 1,365,281 B；应用镜像 1,365,648 B，SHA-256 `97189ea0751924ea68314b19b92a83f012c92afd570f87fd54ecdc2b013d0e2f`。ELF 包含 `applyLight` / `lightTask`，动作总开关仍为 0。不是可直接用于操作真实灯带的已配对固件。

日志与机器可读检查记录保存在本机 `build/review/`，未进入公共归档。没有刷写设备或改变仓库默认配置。

### 最终线上核验

最终 GitHub Pages 工作流 [40183494004](https://github.com/peterpanstechland/peterpanstechland.github.io/actions/runs/40183494004) 的构建与部署均为 success，对应提交 `b6c70139ff6e5fd782cc2c1043ec0ad7b1e6d721`。

已从正式站点打开灯带教程、操作彩虹预览，并确认总览显示当前四模式能力。通过官网的“下载完整源码复现包”链接下载最终 ZIP，实收 597,163 字节、133 个 ZIP 成员；归档 SHA-256 与上述最终清单相同，全部 130 个源文件的哈希再次通过。线上预览截图保存在本机 `build/review/ws2812-live.jpg`。本轮本地预览服务已停止。
