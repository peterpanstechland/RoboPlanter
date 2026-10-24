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
| 独立 Skill | `RoboPlanter工位盆栽伙伴` 已导入并保存0.2.0草稿；Android、智能家居/环境、原创头像与两条引导语已配置。声明官方 `ble_send_command,skill_exit`，当前禁止硬件调用，只允许对话与退出。 |
| Skill ID | `6ea58dd2d841465fbcf6689d3814d30e`，slug `roboplanter-companion`，平台内部skillVersion=1。 |
| Skill检测与发布 | 已运行安全检测并刷新进度，界面已开放“真机测试/发布”按钮；未记录检测评分，不把按钮可用解释为硬件测试通过。返回“我的技能”已确认条目状态为“草稿”。未提交Skill发布或比赛作品。 |
| A2A | 未实现、未注册；独立 Skill + 官方BLE工具主线不需要A2A服务。 |
| 智能体绑定 | 草稿插件未出现在“我的插件”绑定列表；按官方流程，等待插件审核通过后绑定。智能体自身仍是草稿。 |
| 发布与真机 | 插件团队内部发布审核中，未上架市场；未通过 YOYO 手机端与真实花盆端到端验证。 |

服务地址为 `https://emqx.robomon.cn/roboplanter/mcp`。MCP 专用 token 只应通过平台鉴权配置传递，不放入插件描述、提示词、工具参数或技能包；EMQX 管理员密码、MQTT 密码与 XIAO 令牌不提交荣耀。

本轮根据官方活动页“两赛道均提交Skill服务”的要求，将独立Skill改为参赛主线：YOYO → 官方BLE工具 → XIAO；Indicator保持本地Wi-Fi HTTP。已有云服务和插件保留为辅助，不再等待插件审核来推进Skill。Skill的`hardware_ready:false`是提示词行为限制，不是已验证的平台强制权限开关；固件执行器仍默认禁用。没有向平台上传设备令牌，没有把自建MCP名称当成Skill工具。

Skill文件包含根目录`SKILL.md`和两份`references`，平台读取到两个工具、0.2.0版本及`hardware_ready:false`；Android为唯一选定运行环境。[技能配置截图](honor-skill-draft.png)。头像及两条引导语已保存；未添加其他测试账号，未触发手机硬件测试。当前欠缺设备鉴权安全注入、手机MTU/长写与NOTIFY回包、实际烧录及外设联调。

云端已通过公网鉴权、初始化与六工具发现，荣耀控制台也已完成状态工具调用。当前无真实网关，状态返回 unavailable，四个动作工具全部拒绝且 `sent=false`。平台调用成功不等同于设备已连接。

界面截图：[智能体草稿](honor-agent-draft.png)、[已导入六个工具](honor-mcp-six-tools.png)、[插件审核中](honor-plugin-in-review.png)。完整服务器与 MQTT 验证见 [基础设施记录](../../integrations/cloud/INFRA-DEPLOYMENT.md)。

本地 `.env.cloud` 一度仅 Peter 与 SYSTEM 可读。经用户明确授权，临时给 `PeterWIN\CodexSandboxOffline` 添加只读权限，用于将 MCP token 填入荣耀鉴权框；读取完成即撤销。凭据未写入本记录或截图。插件请求参数的默认 token 仍留空，调试时通过专用鉴权字段输入。

团队内部发布与发布声明分别经用户明确确认，提交前保持“允许发布到插件市场”未勾选。页面最后确认结果为“审核中”，不是“审核通过”。官方说明：[智能体配置插件](https://developer.honor.com/cn/doc/guides/101645)、[插件介绍](https://developer.honor.com/en/doc/guides/101637)。
