# RoboPlanter 独立 Skill 内容草稿

这是符合荣耀导入结构的内容草稿，**不是已连接硬件的 Skill**。它与上一级 `agent-prompt.md` 的用途不同：后者用于绑定 MCP 插件的 LLM 智能体；本目录用于“我的技能”的独立 Skill 草稿。

## 包内容

- 源文件：`SKILL.md`，YAML 中包含 `name` 和 `description`。
- 导入包：项目根目录下的 `build/honor/roboplanter-companion-draft.zip`，根目录仅包含 `SKILL.md`，无额外外层目录。该构建目录已被 Git 忽略。
- 未填写 `allowed-tools`、metadata、APIKey 注入语法或任何设备令牌；这些不是为了导入内容草稿而编造的必填项。

## 保存为草稿

在荣耀“我的技能”中导入 ZIP，补齐当前表单要求的基本信息，例如执行环境、实际可选分类、版本（可用 `0.1.0`）和版本说明。无 `_meta.json` 时，slug 可由合法的 `name` 解析。以列表中出现相应草稿条目作为保存成功依据。

本目录生成不包含平台上传、创建条目、安全检测、真机测试或发布。后续若保存平台草稿，应保留文中当前能力限制；发布和测试另按平台流程执行，不能把导入成功称为接入完成。

## 仍缺的依赖

1. 平台如何将自建 MCP 工具关联给独立 Skill 的官方方法或成功验证；仅填写六个工具名称不等于绑定。
2. 到达真实 XIAO 的设备链路。云 MCP 已上线并通过荣耀控制台调用，但真实本地网关尚未接入，不能把云端可访问当作花盆已在线；端 MCP 的 SDK / 调用协议仍需核实。
3. 六个真实工具的发现、参数、鉴权和结果回传，以及手机到设备的只读与动作验收。

只有完成这些条件后，才修订 `SKILL.md`，使用实际绑定名称声明工具并启用设备流程。BLE 直连是另一条备选路线，仍需验证消息长度、业务回包和设备凭证处理；不能通过删除设备鉴权或猜测 APIKey 变量来绕过。

依据：[荣耀技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)、[官方 BLE 工具案例](https://developer.honor.com/cn/doc/guides/101781)、项目 `docs/honor-platform-modes.md` 与 `integrations/yoyo/README.md`。
