# 荣耀平台接入记录

2026-10-24，开发者账号 Peter。控制台入口：[YOYO 智能体平台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)。

| 项目 | 当前结果 |
| --- | --- |
| LLM 智能体 | `RoboPlanter盆栽伙伴` 已创建并保存草稿；指令、开场白、引导示例和原创头像已配置。 |
| 智能体 ID | `661af41a3ab74a17b70b0357052e0773`，版本 1。 |
| MCP 插件 | `RoboPlanter盆栽工具` 已提交团队内部发布，平台显示“审核中”；未勾选发布到插件市场。 |
| 插件 ID | `182dbe5d082142d8b193837c47d768a2`，版本 1。 |
| 工具导入 | 专用 Bearer 鉴权成功，平台显示“调试通过”，六个工具全部导入。 |
| 平台调用 | `get_planter_state` 与 `get_action_result` 均实际调试通过，分别响应 `unavailable` 与 `unknown`，原因为 `gateway_unavailable`；均为只读。 |
| 独立 Skill | 本地内容草稿及 ZIP 已准备；未上传，未与工具绑定。它不是 LLM 智能体提示词的另一个名称。 |
| A2A | 未实现、未注册；当前采用 LLM + MCP，不需要额外建立 A2A 服务。 |
| 智能体绑定 | 草稿插件未出现在“我的插件”绑定列表；按官方流程，等待插件审核通过后绑定。智能体自身仍是草稿。 |
| 发布与真机 | 插件团队内部发布审核中，未上架市场；未通过 YOYO 手机端与真实花盆端到端验证。 |

服务地址为 `https://emqx.robomon.cn/roboplanter/mcp`。MCP 专用 token 只应通过平台鉴权配置传递，不放入插件描述、提示词、工具参数或技能包；EMQX 管理员密码、MQTT 密码与 XIAO 令牌不提交荣耀。

云端已通过公网鉴权、初始化与六工具发现，荣耀控制台也已完成状态工具调用。当前无真实网关，状态返回 unavailable，四个动作工具全部拒绝且 `sent=false`。平台调用成功不等同于设备已连接。

界面截图：[智能体草稿](honor-agent-draft.png)、[已导入六个工具](honor-mcp-six-tools.png)、[插件审核中](honor-plugin-in-review.png)。完整服务器与 MQTT 验证见 [基础设施记录](../../integrations/cloud/INFRA-DEPLOYMENT.md)。

本地 `.env.cloud` 一度仅 Peter 与 SYSTEM 可读。经用户明确授权，临时给 `PeterWIN\CodexSandboxOffline` 添加只读权限，用于将 MCP token 填入荣耀鉴权框；读取完成即撤销。凭据未写入本记录或截图。插件请求参数的默认 token 仍留空，调试时通过专用鉴权字段输入。

团队内部发布与发布声明分别经用户明确确认，提交前保持“允许发布到插件市场”未勾选。页面最后确认结果为“审核中”，不是“审核通过”。官方说明：[智能体配置插件](https://developer.honor.com/cn/doc/guides/101645)、[插件介绍](https://developer.honor.com/en/doc/guides/101637)。
