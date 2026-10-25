# 作物知识与真实周期设计发布记录

日期：2026-10-25。

## 本轮范围

- 私有源码提交：`7dc98f0`，已推送至原有 `codex/initial-sync` 分支；未更改仓库可见性。
- 增加两个手册章节：植物生长与陪伴养成、作物知识库与地区化方案。
- 增加 33 条含核验状态的来源记录、4 张美国州级定性示例卡、标准库离线校验与筛选程序。
- 同步产品叙事、社媒手册和展示页，说明现有屏幕的加速游戏与未来真实物候的差别。
- 来源正文、完整数据集和第三方模型代码均未打包；原始附件、设备备份、凭据和私人配置未公开。

## 验证

- `python3 -m unittest discover -s tests -p test_crop_knowledge.py -v`：17 项通过。
- 独立代码复核发现必填容器路径可能绕过缺失条件判断；已限制为明确叶字段，并补回归测试。
- `python3 -m unittest discover -s tests -p test_public_starter.py -v`：5 项通过。
- `knowledge.py validate`：33 条来源、4 张卡，`automatic_action:false`。
- 从实际公开 ZIP 解压至空临时目录，校验 141 个源码文件的全部 SHA-256，重新运行 17 项知识测试和校验命令均通过。
- 合成中国上海阳台番茄案例保留已观察的开花阶段，忽略预测果实记录，不匹配任何美国州级卡，不输出自动动作。
- 两个 Docusaurus locale 的完整构建成功；有既存 `vscode-languageserver-types` 动态 require 警告。新增章及链接通过站点构建检查。

## 公开制品

- 文档 PR：[53](https://github.com/peterpanstechland/peterpanstechland.github.io/pull/53)，已合并。
- PR 源提交：`eb4c1c10ce4fd26a21d1c08bb5616e2c1f1917f1`。
- 主分支合并提交：`8926a87c2916ca39706ae3c8687b16fbb65e2685`。
- Pages 构建：[40186063721](https://github.com/peterpanstechland/peterpanstechland.github.io/actions/runs/40186063721)，构建与部署均成功。
- 公开源码包：141 个源码文件，144 个 ZIP 成员，633515 字节。
- 源码 ZIP SHA-256：`ddeedd9235e163f35da92fc541a466b25b30366d06f75b3644272060304ebad2`。
- Device Kit ZIP 保持原值：`ee76869371d48460a286d3b43581453696bdc1cb0683c6f6c28ee68915389b56`。

- 浏览器打开 [作物知识库](https://peterpanstechland.github.io/docs/projects/2026/roboplanter/crop-knowledge/) 和 [植物生长与陪伴养成](https://peterpanstechland.github.io/docs/projects/2026/roboplanter/plant-companion/)，标题、正文、侧栏入口及新内容可见。
- 重载 [展示首页](https://peterpanstechland.github.io/roboplanter/)，确认知识库链接、真实养成入口及“33 条来源”的离线原型进度可见。
- 下载线上源码 ZIP，文件大小与 SHA-256 均与上述本地制品一致，包内 141 个源文件摘要再次全部通过。
- 在线页面截图：`build/review/crop-knowledge-live.png`，本地验收资料，不纳入公开源码白名单。

## 未实现与后续输入

Indicator 固件未改、未刷写；其现有养成仍为加速游戏。知识原型不验证输入观察者身份，不计算真实生长预测，不联网抓取，不接 YOYO，不生成药肥剂量或执行设备动作。卡片匹配仅供参考阅读，尚非完整季节、品种、城市、土壤和管理记录规则引擎。

首批真实地区、作物/品种、室内/阳台/温室/露地场景及种植起点仍待用户提供。需要逐条本地化审核与实物观察验证后，才能建立该组合的实际养护方案。没有下载完整图像数据集、训练识别模型、测得节水或增产结果。
