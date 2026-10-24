# 荣耀 YOYO 独立 Skill 参赛主线与辅助接入

实查日期：2026-10-24。赛事要求已通过官方活动页的浏览器正文核实；平台功能以当前账号可见界面和对应官方文档为准。

**RoboPlanter 的参赛主线是独立 Skill：YOYO 主对话 → RoboPlanter Skill → 官方 `ble_send_command` → XIAO ESP32-S3 Plus。** SenseCAP Indicator 通过本地 Wi-Fi HTTP 与 XIAO 通信，承担表情、状态显示和触摸操作。已部署的 MCP 保留为辅助能力与后续远程查询链路；A2A 本阶段不需要。

## 比赛要求与交付状态

2026-10-24 实查的官方活动页明确写道：

> 赛道二选一，均需通过「荣耀开发者服务平台」报名并提交 Skill 服务，收到审核通过邮件，即入围成功，有机会评奖评优。

这项要求同时适用于硬件与软件赛道。仅有 LLM 智能体、MCP 插件或云服务上线，不能替代 Skill 服务提交，也不能据此认定已经入围。来源：[荣耀 Maker Challenge 创客先锋赛官方活动页](https://developer.honor.com/cn/activity/page/tg2026081119200701#navigation6)。

当前阶段分别记录如下：

| 项目 | 已有结果与边界 |
| --- | --- |
| 独立 Skill | 0.2.0已导入并保存平台草稿，声明官方BLE及退出工具，硬件门禁关闭。检测、测试、发布和比赛提交分别记录，当前不宣称参赛提交或审核通过 |
| XIAO 与 Indicator | 固件已编译，具有 BLE/HTTP 设备接口与本地屏幕功能；尚未烧录和完成真机联调 |
| 云 MCP | 已上线 HTTPS 服务，六个工具可发现，两个 MQTT 账号的 24 项实际 broker 权限检查通过；真实设备网关尚未接通 |
| 平台 MCP 插件 | 已导入六个工具，并在平台实际调用两个只读工具，收到离线/未知结果；已提交团队内部发布，当前审核中，尚待绑定 |
| LLM 智能体 | 已创建草稿，作为辅助对话方案，不作为独立 Skill 的替代交付 |
| A2A | 未实现，本阶段无需开发 |

独立 Skill 的硬件鉴权注入、手机端长写与业务回包仍未确认。草稿不得声称已经能读取或控制花盆。最新平台状态以[平台记录与截图](deployment/honor-platform-status.md)为准；云验证见[部署记录](../integrations/cloud/INFRA-DEPLOYMENT.md)。

## 独立 Skill 的设备调用链路

```text
用户 → YOYO 主对话 → RoboPlanter 独立 Skill → 官方 ble_send_command
                                                    ↕ BLE
                                           XIAO ESP32-S3 Plus
                                             ├─ 传感器
                                             ├─ 12V 水泵外部驱动
                                             ├─ 灯带与打印机
                                             └─ Wi-Fi HTTP ↔ SenseCAP Indicator
```

这是待真机验证的目标链路。验收顺序是“查询盆栽状态 → 返回本次真实采样及有效性 → 用户明确请求后执行有时限的动作 → 查询并说明实际结果”。低湿度、用户心情或碰触 NFC 标签都不是浇水授权。超时或结果未知时，保留请求编号查询，不重新发送动作，也不换新编号重试。

Indicator 已实现中文五页界面、原创表情、八主题、本地照片与番茄钟。它是本地显示与操作终端，不承担 YOYO 对话中枢或云 MCP 服务；YOYO 对话推送、主题和计时控制接口尚未实现。照片需本地导入后构建，番茄钟在屏幕上本地运行。相关实现见[Indicator UI](indicator-ui.md)与[设备协议](device-protocol.md)。

## 独立 Skill 的包与平台流程

“我的技能”独立管理以 `SKILL.md` 为核心的技能包。官方导入要求为不超过 50M 的 ZIP，根目录直接包含 `SKILL.md`；其 YAML 元数据必须有 `name` 和 `description`，同层不可存在重名文件或目录。`_meta.json` 不是必需，没有它时可由合法的 `name` 解析 slug。

创建时需补齐技能标识、名称、描述、执行环境、分类、版本与版本说明。`metadata`、APIKey 开关及 Allowed tools 不是无条件必填项，但**实际调用的全部工具必须声明**；Allowed tools 最多 20 个，每个不超过 64 字符。BLE 方案只使用已公开的真实工具名，不虚构扫描工具、MCP 名称或令牌变量。

平台要求进行安全检测后才可真机测试或发布；文档同时说明检测结果不影响正常发布，因此应写“完成安全检测”，不能改写为必须取得某个通过分数。草稿真机测试需要头像。发布进入人工审核，可选择市场发布或私有发布；保存草稿、平台发布审核与比赛入围审核是不同状态。

来源：[技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)。本地草稿入口：[独立 Skill 草稿说明](../integrations/yoyo/skill-draft/README.md)。

`allowed tools` 是工具名称声明，不能据此推定已绑定“我的插件”中的自建 MCP。当前公开资料及页面观察尚未确认独立 Skill 关联自建 MCP 的方法；这也不等于平台绝不支持。存在 `scripts` 目录或执行环境选项，同样不足以证明平台会运行任意 Python、Node 程序或常驻设备服务。智能体提示词中的“技能”章节也不等同于独立 Skill 包。

## 官方 BLE 能力与待验证边界

官方开放能力列出 `ble_send_command`，用于向已知 BLE 设备发送指令，调用前需要目标设备信息与支持的命令内容。对应环境为 MagicOS 10.0 及以上、YOYO 90.10.32.025 及以上，支持文档所列 BLE 5.0–5.4 文本传输。不要套用 A2A 文档的较低版本门槛，也不要据 BLE 连接推断整个 YOYO 对话可以离线运行。

官方案例的 `ack_type="ack"` 表示 GATT WriteWithResponse；`expect_response=true` 用于等待业务响应。前者不代表动作完成。项目当前特征采用 WRITE + NOTIFY，查询需要实际订阅并验证通知，不能沿用原机甲小车附件中不等待业务回包的配置。

本项目 XIAO 固件已增加传感器状态、按请求编号查询结果、设备本地动作限时，以及 BLE 连接会话隔离。它与原附件仅确认命令、将详细状态打印到 USB 的程序不同。编译通过仍不能证明荣耀手机端已经成功调用。

仍需核实：

- 如何在 Skill 中安全提供项目设备令牌。平台 APIKey 开关的存在没有给出已确认的注入语法；不得猜测变量名、删除设备鉴权或把真实令牌写入公开技能包。
- 手机端 MTU、长写或分片是否适用于实际调用。16 字符令牌下，现有请求就可能超过默认 ATT MTU 的 20 字节载荷；不能仅凭设备端设置 MTU 247 认定双方已协商成功。
- 通知订阅时序、完整业务回包、断连重连与超时行为。官方参数页对 `result_code` 的成功值存在 0 与 200 两种描述，需按实际工具响应并校验设备业务内容，不能只凭状态码报成功。

来源：[技能开放能力](https://developer.honor.com/cn/doc/guides/101776)、[BLE 调用案例](https://developer.honor.com/cn/doc/guides/101781)、[XIAO 固件及已知边界](../firmware/xiao/README.md)。以上未知项在确认前，应作为草稿的明确限制，不作可用承诺。

## 侧栏入口的职责

| 入口 | 用途 | RoboPlanter 中的定位 |
| --- | --- | --- |
| 首页 | 平台能力及活动导航 | 查找赛事报名和开发入口 |
| 我的技能 | 创建或导入 Skill，编辑、检测、测试、发布 | 参赛主交付入口，配置 BLE 调用流程与交互约束 |
| 技能市场 | 查找可使用的 Skill | 参考任务型技能及后续分发 |
| 创建智能体 / 我的智能体 | 创建和管理 LLM 或 A2A 对话服务 | 已有 LLM 草稿作为辅助方案；本阶段不建 A2A |
| 我的插件 / 插件市场 | 创建或复用 API、MCP 能力插件 | 管理已部署的辅助 MCP，不代替 Skill 服务 |
| 我的知识库 | 管理智能体检索资料 | 养护手册和设备说明，不存放伪装为实时值的数据 |
| 模板广场 | 复制预配置智能体模板 | 可参考配置，不能据模板名认定任意 XIAO 已可控制 |
| 文档中心 | 阅读接入协议与流程 | 查证工具、环境及发布限制 |

来源：[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)、[智能体开发](https://developer.honor.com/cn/doc/guides/101617)、[插件配置](https://developer.honor.com/cn/doc/guides/101645)、[技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)。

## MCP 与 LLM 是已保留的辅助方案

```text
LLM 智能体草稿 → MCP 插件（审核中，待绑定）→ 已部署云 MCP
                                                    ↕
                                                   EMQX
                                                    ↕
                                           本地网关（未接通）
                                                    ↕ HTTP GET
                                                  XIAO
```

API 插件封装 HTTP 接口，MCP 插件接入工具集合；官方文档明确的绑定位置是智能体配置页。本项目云 MCP 已部署至 `https://emqx.robomon.cn/roboplanter/mcp`，并提供 `/sse` 入口，使用 HTTPS + Bearer 鉴权。此项成果可用于后续远程查询和复用设备能力，不能视作独立 Skill 已接入或赛事已提交。

云端六工具为 `get_planter_state`、`water`、`stop_water`、`set_light`、`print_note`、`get_action_result`。当前真实网关未接入，只读查询应如实返回不可用或未知；所有云端动作，包括停泵，固定返回 `rejected / cloud_writes_not_enabled / sent=false`。本机固件存在动作接口不代表云端写入已开放。

辅助链路的网关需要同时连接 XIAO 热点与互联网。云端不能直接访问 `192.168.4.1`；Indicator 仍通过本地 Wi-Fi HTTP 与 XIAO 通信。BLE 与 MCP 可以保留一致的业务语义，但各自的鉴权、工具绑定与实物验证仍需独立完成。

来源：[插件配置](https://developer.honor.com/cn/doc/guides/101645)、[MCP 接入指南](https://developer.honor.com/cn/doc/guides/101724)、[云 MCP 与网关说明](../integrations/cloud/README.md)。

### 端 MCP 的能力仍待取得规范

控制台可见 StreamableHTTP、SSE 与端 MCP 三类入口。端 MCP 表单含 `packageName`、`appAccessKey`、`minAppVersion`、`romVersion` 等字段，证明有与手机应用关联的注册入口；不能据此认定 ESP32 可直接注册或任意 Android 服务可直接被 YOYO 调用。

端 MCP 的 SDK、服务声明、调用协议、权限及支持机型尚未核实。“手机 App 作为设备桥接层”可留待后续研究，不作为本轮 Skill 参赛主线的前置任务。来源：[MCP 公开指南](https://developer.honor.com/cn/doc/guides/101724)、[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)。

## LLM 与 A2A 的适用范围

LLM 模式由平台托管模型与配置，可绑定插件和知识库；A2A 模式接入第三方对话服务，需要实现对应接口、鉴权、会话和流式返回。A2A 不要求自行训练模型，但设备通信仍需另行实现。两者都是完整智能体的接入方式，不是 Skill 包格式。

官方还区分标准 A2A 与荣耀 A2A：前者使用手机终端智能体生态接入标准 v1.0、JSON-RPC 2.0 的 `message/stream`；后者使用自定义履约接口及 `requestId`、`messages`、`endpoint`、`session`、`choices` 等结构。两者不能互换，也不能把任意 A2A SDK 或 OpenAI 兼容接口视为已经兼容。

**本阶段不开发 A2A。** 只有后续确需接入完整第三方对话服务时，再评估其适用性。来源：[零代码智能体](https://developer.honor.com/cn/doc/guides/101711)、[标准 A2A](https://developer.honor.com/cn/doc/guides/101763)、[荣耀 A2A](https://developer.honor.com/cn/doc/guides/101708)、[A2A 配置](https://developer.honor.com/cn/doc/guides/101700)。

## 知识库与 NFC 的边界

知识库适合植物养护手册、传感器说明和设备使用方法。当前湿度、设备是否在线及上次浇水是否成功必须通过实时工具查询，历史资料不能代替设备状态。来源：[创建知识库](https://developer.honor.com/cn/doc/guides/101642)、[配置知识库](https://developer.honor.com/cn/doc/guides/101646)。

NFC 贴按手机读取的无源入口标签设计，保存非秘密设备标识或入口链接，不连接 XIAO。Android 可以处理 NDEF URI，但指定 YOYO Skill 的直达链接尚未验证；不能把普通 URL、端 MCP 和技能唤起推定为已打通。碰触标签仅用于选择设备或打开入口，不授权浇水和打印。来源：[Android NFC 基础](https://developer.android.com/develop/connectivity/nfc/nfc)、[本项目 NFC 设计](feasibility-review.md)。

## 下一步验收证据

1. 在“我的技能”建立独立 Skill 草稿，保留明确的当前能力限制；记录平台条目与保存结果。
2. 确认硬件鉴权的安全提供方式，以及真实手机上的 MTU/长写、通知与业务回包；未确认前不启用硬件能力承诺。
3. 完成 XIAO 与 Indicator 烧录、电气检查和真实只读查询，再逐项验证有本地限时的动作、结果查询与断连不重放。
4. 完成 Skill 的安全检测、真机测试及需要的发布流程；按赛事入口报名并提交 Skill 服务，单独记录提交结果与审核邮件。

本次已进行平台草稿创建、辅助 MCP 接入与内部发布提审，不能再概括为“未提交任何平台内容”。但独立 Skill 草稿、比赛 Skill 提交及手机到真实硬件的验证尚不能视为完成；收到赛事审核通过邮件之前，不宣称已入围。
