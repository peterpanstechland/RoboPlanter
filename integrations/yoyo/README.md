# YOYO 主交互接入

RoboPlanter 的主入口是 YOYO 独立 Skill：目标是用户说话，YOYO 通过官方 `ble_send_command` 工具连接附近的 XIAO，再根据设备业务回包回答。Indicator 是工位陪伴、状态显示与本地操作终端，经 Wi-Fi HTTP 连接 XIAO。独立 Skill 不以 MCP 插件、云网关或 A2A 为前置条件。

独立 Skill 0.3.2 位于 `skill-draft/`，保留连贯闲聊、短句与少追问、普通换话题不退出，以及固定无效凭据的蓝牙联通诊断。平台根文件与公开源码统一，不再携带旧私有0.3.1临时读取值；正式读数与执行器权限关闭。手机历史验收的单次读取版为0.2.2，不能把草稿更新当作手机已升级。持续凭据下发和手机长写仍未确认。已有云 MCP、MQTT 与 LLM 智能体草稿保留为辅助；本机只读网关已完成真实查询上云，云端所有写操作仍未启用。

历史私有台架0.2.2曾经用户授权使用独立短期一次性只读凭据，完成手机土壤读取。该方式仍把凭据放进提示词，不能作为正式方案；0.3.2已移除平台正文中的此入口，旧值不得复用。历史验收证据见[平台记录](../../docs/deployment/honor-platform-status.md)。

## 文件

- [独立 Skill 草稿](skill-draft/README.md)：主线的 Skill 文件、上传材料及尚未验证的 BLE 条件。
- [辅助智能体提示词](agent-prompt.md)：供平台 LLM 智能体接入 MCP 后使用，不代替独立 Skill。
- [交互验收场景](acceptance-scenarios.json)：人工在 YOYO 上逐条试验；这些是预期行为，不是已经通过的模型测试。
- [MCP 桥接服务](../mcp/README.md)：六个实际工具及本机运行方式。
- [云 MCP 与本地网关](../cloud/README.md)：公网入口、鉴权、MQTT 查询链路和网关配置。
- [实际部署与验证记录](../cloud/INFRA-DEPLOYMENT.md)：公网服务、两个 MQTT 账号及 24 项实际 broker 权限检查。
- [设备协议](../../docs/device-protocol.md)：XIAO 的状态、命令和结果查询。

## 调用关系与网络

```text
YOYO 独立 Skill → 官方 ble_send_command → XIAO BLE
                                           ↓
                                  本地传感器与执行器
                                           ↑
SenseCAP Indicator → Wi-Fi HTTP → XIAO HTTP
```

图中的箭头表示目标调用关系，不能据草稿上传认定手机 BLE 已连通。手机 BLE 与 Indicator HTTP 最终进入同一套 XIAO 控制逻辑；Indicator 不转发 YOYO 的蓝牙命令。

辅助网络方案为 LLM 智能体 → MCP 插件 → 云 MCP → EMQX → 本地双网络网关 → XIAO HTTP。云 MCP 已部署到 `https://emqx.robomon.cn/roboplanter/mcp`，兼容 SSE 地址为 `https://emqx.robomon.cn/roboplanter/sse`，均要求 Bearer 鉴权。公网初始化、发现六个工具以及离线/禁写返回已验证；EMQX 的云端与网关两个独立账号已通过 24 项实际权限检查。健康检查通过不代表设备在线，也不证明独立 Skill 可以调用这些 MCP 工具。

本地网关通过 MQTT 5/TLS 连接云端，同时通过 HTTP GET 访问 XIAO 的 `192.168.4.1`。网关电脑需要一条网络连接 XIAO 热点，另一条网络上网，例如 Wi-Fi + 有线网络。荣耀云不能直接访问这个私网地址，也不能使用用户电脑的 `127.0.0.1`。网关只支持读取状态与按请求编号查询结果；设备令牌只保存在本地。

**2026-10-26 已通过本机网关从公网 MCP 读到 Indicator 真实土壤 ADC 2516，未校准且百分比为空。只有网关与设备在线时查询才可成功；电脑休眠或进程退出会离线。所有云端动作，包括 `stop_water`，仍返回 `rejected / cloud_writes_not_enabled / sent=false`。** 本机 MCP 仍可用 stdio 或回环 HTTP 独立联调，但其动作能力不代表云端已启用。

辅助平台的已完成记录：LLM 智能体草稿已创建；MCP 插件已导入全部六个工具，平台实际调用两个只读工具通过，并按用户授权提交团队内部发布（不上架市场），提交后处于“审核中”，尚未完成智能体绑定。独立 Skill 的上传进展另见[平台记录与截图](../../docs/deployment/honor-platform-status.md)。A2A 尚未实现，独立 Skill 主线不需要 A2A；端 MCP 的 SDK 与本地桥接协议尚未取得。

## 平台配置顺序

1. 在“我的技能”中上传 `skill-draft/` 的独立 Skill，填写平台资料并进行安全检测；0.2.0已保存，检测及后续状态见平台记录。
2. 按平台测试流程验证安装、自然语言唤起、陪伴对话和未接通设备时的如实反馈。文件上传或检测通过不等于 BLE 可用。
3. 确认平台提供的设备令牌安全注入方式，不将令牌写入 Skill 正文，也不猜测 APIKey 变量语法。`allowed-tools` 用于声明官方工具，不能凭填写自建 MCP 工具名认定已完成绑定。
4. 安全注入方式确认后，在真机验证 `ble_send_command` 的完整请求、长写、通知订阅与业务回包。先验证带鉴权的只读查询；未知、过期或未标定的数据必须保留原状态。
5. 只读链路和外设联调完成后，再验证明确授权的短时动作、软件限时关闭和结果查询。仅 GATT 写成功或 YOYO 说“成功了”不能证明设备执行完成。

资料：[Skill 创建与管理](https://developer.honor.com/cn/doc/guides/101775)、[技能开放能力](https://developer.honor.com/cn/doc/guides/101776)、[官方蓝牙工具与参数](https://developer.honor.com/cn/doc/guides/101781)。平台的独立 Skill 与 LLM 智能体是不同入口；[MCP 接入指南](https://developer.honor.com/cn/doc/guides/101724)描述的是辅助插件的接入，不能据此推定独立 Skill 能调用该插件。

## 独立 Skill 的 BLE 联调条件

荣耀的 `ble_send_command` 支持 `ack_type="ack"` 和 `expect_response=true`，分别控制 GATT 写确认和设备业务响应等待。当前固件特征支持 WRITE + NOTIFY，不能照搬附件中的 `no_ack` 与不等回包配置。设备名称应使用实际广播名称，不添加未公开的扫描工具。

官方文档同页对 `result_code` 的说明存在 0 与 200 两种成功值，真机需核实实际工具版本，并验证 `response` 中的设备业务结果；仅工具写入成功不能证明泵已完成。文档未给出自动协商 MTU、长写、跨调用保留连接或 APIKey 注入的明确保证。

现有 16 字符令牌下，`S|token|uv` 已为 21 字节，土壤查询 23 字节，动作还包含请求编号和参数，超过默认 ATT MTU 下常见的 20 字节载荷。固件已实现连接归属校验与长写片段处理，但手机工具是否支持仍须真机验证。当前可整理和上传 Skill 草稿，**在安全注入与完整收发得到确认前，不发送硬件命令**；不删除设备鉴权，也不猜测密钥变量名。来源：[官方蓝牙工具与参数](https://developer.honor.com/cn/doc/guides/101781)。

## 交互边界

NFC 贴用于选择盆栽和打开入口，不把碰触解释为浇水授权；指定 YOYO Skill/智能体的直达链接尚未验证。Indicator 已实现 480×480 中文陪伴界面：表情首页 → 今日状态 → 信息 → 照片 → 番茄钟，支持触摸横滑、八种主题与主题保存；照片需本地导入，番茄钟在屏幕本地运行。这些界面尚未真机验收，也没有 YOYO 控制主题、照片或番茄钟的工具。

打印机首版仅支持 ASCII；中文对话由 YOYO 完成。自动浇水、定时提醒、多设备长期记忆等后续功能不混入首版验收。
