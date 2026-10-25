# Landing page 一体机 3D 发布记录

日期：2026-10-25。

## 交付范围

将已生成的 Tripo 一体机静态外观模型放到公开 landing page 的 `#all-in-one` 区域。保留现有程序化角色、粒子、网页灯效与海报交互；新增旋转预览使用独立的本地 Three.js r170 iframe，避免与角色演示的 r180 混用。

默认只显示预览图；点击后加载 GLB。返回图片会把 iframe 置为 `about:blank` 并隐藏，释放该视窗。模型视窗支持重置和默认关闭的自动转台；页面隐藏或相应展示区不在视口时暂停渲染，弱动效偏好下禁用自动转台。模型下载失败时保留图片并提供重试。

公开资产只有模型、预览、限定字段的摘要清单和带原 MIT 许可的本地运行时，没有复制账户配置、Tripo CLI 本地状态、原始任务响应或临时下载地址。本轮没有调用 Tripo 生成或付费处理接口。

## 资产

- 模型：`web/experience/assets/all-in-one/model.glb`，44,940,768 字节，1,444,258 个三角形，GLB 2.0，三个内嵌图片，无外部纹理 URI。
- SHA-256：`3080403faa426b65489624fec1ef4b1d21d20ca450cc6a10ec5f422baa025cf4`，与原始 Tripo 文件一致。
- 预览：`web/experience/assets/all-in-one/preview.png`，319,246 字节。
- 固件源码复现包：141 个源码文件，633,979 字节；SHA-256 `c77c20ccadcbf0fce9c87364bca3fd9c5c46e1f0d93fd04b3695923ea7a4ed16`。该 ZIP 不包含大模型，仅同步本轮修改的公开手册。

## 验证

- 新增/修改 JavaScript 均通过 Node 语法检查；5 项 public starter 测试通过；`git diff --check` 通过。
- Docusaurus 两个 locale 完整构建成功；仅有既有 `vscode-languageserver-types` 动态 require 警告。
- 浏览器资源事件确认：初次加载页面没有请求 GLB，点击后才发生模型请求。
- 实际 GLB 在 landing iframe 中加载成功，重置与转台开关可操作。
- 模型请求被临时阻断时显示图片、错误说明和重试；恢复网络限制后重试可加载。测试阻断与缓存设置均已恢复。
- 390×844 视口测试无横向溢出；弱动效媒体设置生效后转台禁用，随后恢复默认视口和媒体设置。此项是布局模拟，不是实体手机性能验收。
- 返回图片的键盘操作确认 iframe 卸载、图片恢复；现有浇水文案、灯效及海报弹窗仍可用。
- 核对两个 Git 仓库都跟踪了所有 viewer 依赖。通用 `build/` 忽略规则原先排除了 vendored Three.js 运行时，已明确加入版本控制，避免本机有文件但 CI 缺失。

## 发布

- 私有源提交：`82d6837`、`cd26302`，已推送至原有 `codex/initial-sync`；未改变仓库可见性。
- 公开文档 PR：[54](https://github.com/peterpanstechland/peterpanstechland.github.io/pull/54)，已合并。
- 发布分支提交：`c991e192db14cafc6635f5cbeda6bc988246c52c`。
- 主分支合并：`6efcad3e86ee4c5ee571feb6c5538f26020ef255`。
- Pages 工作流：[40202603028](https://github.com/peterpanstechland/peterpanstechland.github.io/actions/runs/40202603028)，构建及部署均成功。
- 正式页：[一体机展示区](https://peterpanstechland.github.io/roboplanter/#all-in-one)。浏览器确认新文案与入口可见，点击后实际模型加载完成，重置与转台控件可用；重置视角操作成功。
- 回读线上 `assets/all-in-one/manifest.json`，与本地已验证的资产元数据完全一致。没有再次下载完整线上 GLB 计算摘要；线上模型已通过浏览器实际加载。
- 线上验收截图：`build/review/landing-tripo-live.jpg`，未加入公开资产白名单。

## 边界

这是 AI 生成的静态外观模型。没有制造 CAD、工程尺寸、可打印结构、可拆部件动画或真实灯带联动，也不代表现有样机外壳已经制造。约 45 MB 的原始模型按用户主动操作加载，尚未做面向手机网络与 GPU 的专项减面或性能认证。
