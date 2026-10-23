# YOYO 主交互接入

RoboPlanter 的主入口是 YOYO：目标是用户说话，YOYO 理解意图，通过 MCP 查询或操作盆栽，再用设备返回结果回答。Indicator 是工位陪伴、状态显示与本地操作终端。当前采用“平台 LLM 智能体 + 自有 MCP”：云 MCP 已上线，真实设备网关尚未接通，云端所有写操作（包括停泵）未启用。BLE Skill 保留作手机近距离直连的备选。

## 文件

- [智能体提示词](agent-prompt.md)：配置到平台 LLM 智能体，描述角色、工具使用、动作结果和陪伴交互。
- [交互验收场景](acceptance-scenarios.json)：人工在 YOYO 上逐条试验；这些是预期行为，不是已经通过的模型测试。
- [MCP 桥接服务](../mcp/README.md)：六个实际工具及本机运行方式。
- [云 MCP 与本地网关](../cloud/README.md)：公网入口、鉴权、MQTT 查询链路和网关配置。
- [实际部署与验证记录](../cloud/INFRA-DEPLOYMENT.md)：公网服务、两个 MQTT 账号及 24 项实际 broker 权限检查。
- [独立 Skill 内容草稿](skill-draft/README.md)：本地草稿包及尚未完成的工具绑定依赖。
- [设备协议](../../docs/device-protocol.md)：XIAO 的状态、命令和结果查询。

## 调用关系与网络

```text
YOYO / LLM 草稿 → MCP 插件（等待审核与绑定）→ 云 MCP → EMQX
                                                 ↓
                                  本地双网络网关（待接通）
                                                 ↓
                                           XIAO HTTP
                                                 ↑
                                       SenseCAP Indicator
```

图中的箭头表示完整方案；平台绑定和真实设备链路还未完成。云 MCP 已部署到 `https://emqx.robomon.cn/roboplanter/mcp`，兼容 SSE 地址为 `https://emqx.robomon.cn/roboplanter/sse`，均要求 Bearer 鉴权。公网初始化、发现六个工具以及离线/禁写返回已验证；EMQX 的云端与网关两个独立账号已通过 24 项实际权限检查。健康检查通过不代表设备在线。

本地网关通过 MQTT 5/TLS 连接云端，同时通过 HTTP GET 访问 XIAO 的 `192.168.4.1`。网关电脑需要一条网络连接 XIAO 热点，另一条网络上网，例如 Wi-Fi + 有线网络。荣耀云不能直接访问这个私网地址，也不能使用用户电脑的 `127.0.0.1`。网关只支持读取状态与按请求编号查询结果；设备令牌只保存在本地。

**真实网关尚未接通，当前状态查询应返回 `unavailable`，动作结果查询可能为 `unknown`。所有云端动作，包括 `stop_water`，均返回 `rejected / cloud_writes_not_enabled / sent=false`，不会向设备发命令。** 本机 MCP 仍可用 stdio 或回环 HTTP 独立联调，但其动作能力不代表云端已启用。

平台当前状态：LLM 智能体草稿已创建；MCP 插件已导入全部六个工具，平台实际调用两个只读工具通过，并按用户授权提交团队内部发布（不上架市场），当前“审核中”。审核通过后才能在智能体中绑定。独立 Skill 仅为本地内容草稿。A2A 尚未实现，当前“LLM 智能体 + MCP”主路线不需要 A2A。端 MCP 的 SDK 与本地桥接协议尚未取得，本方案使用云 MCP。见[平台记录与截图](../../docs/deployment/honor-platform-status.md)。

## 平台配置顺序

1. 在“我的插件”中按官方 MCP 指南配置已上线的 MCP 地址和鉴权，检查实际发现的六个工具和参数。不要把 HTTP `/api/v1/state` 或本地回环地址填成平台服务地址。
2. 在已创建的 LLM 智能体草稿中绑定插件，并使用 `agent-prompt.md`。工具名称如被平台添加前缀，按实际工具名映射。
3. 先验证离线状态和禁写反馈：YOYO 应如实说明设备尚未接通，不能把六个工具可发现解释为动作可用。
4. 按云端说明配置本地双网络网关，刷写并连接真实 XIAO，再验证只读状态对话。未知、过期或未标定的数据必须保留原状态。
5. 后续补齐设备执行截止时间协议、持久化动作记录与真机验证后，才能另行启用云端写操作。语音成功必须同时有工具日志和设备反馈，不能只凭 YOYO 说“成功了”。

资料：[插件配置](https://developer.honor.com/cn/doc/guides/101645)、[MCP 接入指南](https://developer.honor.com/cn/doc/guides/101724)。平台的独立 Skill 与 LLM 智能体是不同入口；当前提示词供 LLM 智能体绑定 MCP 后使用，独立 Skill 的本地内容草稿不代表已在平台安装、绑定工具或发布。

## BLE Skill 备选的实际条件

荣耀的 `ble_send_command` 支持 `ack_type="ack"` 和 `expect_response=true`，分别控制 GATT 写确认和设备业务响应等待。当前固件特征支持 WRITE + NOTIFY，不能照搬附件中的 `no_ack` 与不等回包配置。设备名称应使用实际广播名称，不添加未公开的扫描工具。

官方文档同页对 `result_code` 的说明存在 0 与 200 两种成功值，真机需核实实际工具版本，并验证 `response` 中的设备业务结果；仅工具写入成功不能证明泵已完成。文档未给出自动协商 MTU、长写、跨调用保留连接或 APIKey 注入的明确保证。

现有 16 字符令牌下，`S|token|uv` 已为 21 字节，土壤查询 23 字节，两秒浇水命令约 31 字节，超过默认 ATT MTU 下常见的 20 字节载荷。必须先在手机上验证完整请求和响应，再生成可用 BLE Skill；本轮不通过删除设备鉴权或猜测秘钥变量名来绕过这个问题。来源：[官方蓝牙工具与参数](https://developer.honor.com/cn/doc/guides/101781)。

## 交互边界

NFC 贴用于选择盆栽和打开入口，不把碰触解释为浇水授权；指定 YOYO Skill/智能体的直达链接尚未验证。Indicator 已实现 480×480 中文陪伴界面：表情首页 → 今日状态 → 信息 → 照片 → 番茄钟，支持触摸横滑、八种主题与主题保存；照片需本地导入，番茄钟在屏幕本地运行。这些界面尚未真机验收，也没有 YOYO 控制主题、照片或番茄钟的工具。

打印机首版仅支持 ASCII；中文对话由 YOYO 完成。自动浇水、定时提醒、多设备长期记忆等后续功能不混入首版验收。
