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
