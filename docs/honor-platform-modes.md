# 荣耀 YOYO 平台入口与 RoboPlanter 接入选型

核对日期：2026-10-24。平台页面以当前账号可见界面为准。

YOYO 平台把完整对话服务、能力工具、知识资料和 Skill 技能包分别管理。LLM 与 A2A 是智能体的创建方式；API 与 MCP 是插件的接入方式；独立 Skill 在“我的技能”中创建。它们处在不同层级，可以组成不同方案。

RoboPlanter 按用户“把 YOYO 作为大模型入口，再写 MCP 对接花盆”的方向，优先采用平台托管的 LLM 智能体接入自有 MCP 插件。当前本地实现与交互定义见 [YOYO 接入](../integrations/yoyo/README.md)。Skill 调用官方 BLE 工具保留为手机直连的备选，需要先确认长写与业务回包。端 MCP 已出现在控制台，但其开发协议、SDK 和与 Skill 的关联方式仍需要核实。

## 侧栏入口的职责

| 入口 | 用途 | RoboPlanter 中的用途 |
| --- | --- | --- |
| 首页 | 平台能力及活动导航 | 查找开发入口与赛事信息 |
| 创建智能体 | 选择 LLM 或 A2A 模式，创建完整对话服务 | 建立有角色设定、能力和资料的盆栽助手 |
| 我的智能体 | 管理已创建的智能体及其开发和发布流程 | 调整助手行为，配置插件和知识库 |
| 插件市场 | 查找可复用的能力插件 | 按需复用天气等外部能力，核对授权和费用 |
| 我的插件 | 创建和管理 API 或 MCP 插件 | 封装读取状态、浇水、灯带、打印等设备工具 |
| 技能市场 | 查找可使用的 Skill | 参考面向 YOYO 的任务型技能 |
| 我的技能 | 创建或导入 Skill 包，编辑文件和元数据，检测、测试、发布 | 编写 RoboPlanter 的调用条件、步骤和设备操作约束 |
| 我的知识库 | 管理供智能体检索的资料 | 植物养护说明、传感器解释、设备使用手册 |
| 模板广场 | 复制预配置的智能体模板 | 参考陪伴助手、智慧家居等配置，再按实际接口改造 |
| 文档中心 | 阅读平台、协议和开发流程说明 | 查证具体接入约束 |

当前模板广场可见“智慧家居语音助手”“支持设备自控”“儿童陪伴助手”等模板及“免费复制”入口。模板描述中的设备能力仍以对应接入条件为前提，不能据模板名称推断任意 XIAO 设备可直接控制。

来源：[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)、[智能体开发](https://developer.honor.com/cn/doc/guides/101617)、[插件配置](https://developer.honor.com/cn/doc/guides/101645)、[技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)。

## LLM 和 A2A 如何选择

当前“创建智能体”弹窗明确区分以下两种模式。

| 方式 | 对话服务由谁组织 | 开发工作 | 适合本项目的情况 |
| --- | --- | --- | --- |
| LLM 模式 | 平台托管的模型与智能体配置 | 配置提示词、插件、知识库；实现自有设备工具 | 希望 YOYO 作为入口，借助平台模型理解用户，再调用盆栽工具 |
| A2A 模式 | 接入的第三方对话服务 | 实现或适配对话接口、鉴权、会话和流式返回 | 已有完整智能体后端，或需要自行组织模型、记忆和工具编排 |

A2A 不要求自行训练模型，可以接入使用第三方模型的自建服务。它对接的是对话服务；连接 XIAO、水泵和打印机的设备通信仍需另行实现。LLM 模式“零代码”针对平台中的智能体配置，不表示自制硬件和服务端无需开发。

来源：[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)、[零代码智能体](https://developer.honor.com/cn/doc/guides/101711)、[A2A 配置](https://developer.honor.com/cn/doc/guides/101700)。

### 两类 A2A 文档需要分别适配

文档导航同时提供“标准 A2A 智能体接入”和“荣耀 A2A 智能体接入”。

- 标准 A2A 文档使用手机终端智能体生态接入标准 v1.0，基于 JSON-RPC 2.0，调用方法为 `message/stream`，使用消息、任务、上下文和 SSE 响应结构。
- 荣耀 A2A 文档描述自定义履约接口，请求包含 `requestId`、`messages`、`endpoint`、`session` 等，响应使用其规定的 `choices` 等字段及流式格式。

两者不能仅凭“A2A”名称互换；也不能把任意 OpenAI 兼容接口或任意 A2A SDK 的默认接口直接视为已兼容。应先确定平台选用的接入规范，再实现对应协议与鉴权。后台配置还涉及服务地址、模型及相关合规信息，工作量高于单独接入几个设备工具。

来源：[标准 A2A 接入](https://developer.honor.com/cn/doc/guides/101763)、[荣耀 A2A 接入](https://developer.honor.com/cn/doc/guides/101708)、[A2A 配置](https://developer.honor.com/cn/doc/guides/101700)。

## API 和 MCP 插件提供实际能力

API 插件封装已有 HTTP 接口；MCP 插件接入 MCP 服务提供的工具集合。平台智能体可以配置插件，由模型根据工具描述发起调用。MCP 本身不会自动把蓝牙命令转成设备操作，工具实现方仍要处理设备寻址、连接、返回结果和异常。

当前 API 创建表单支持 API 地址、请求头、通用参数及鉴权；鉴权菜单可见无需授权、SERVICE、AK/SK、OAuth、签名等方式。具体选项以目标服务的协议为准。

来源：[插件配置](https://developer.honor.com/cn/doc/guides/101645)、[MCP 插件](https://developer.honor.com/cn/doc/guides/101724)、[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)。

### 当前 MCP 有三种入口

| 类型 | 已确认的接入信息 | 对 RoboPlanter 的意义 |
| --- | --- | --- |
| StreamableHTTP | 官方文档提供远程 MCP 服务 URL 配置和测试流程 | 自建可被平台访问的服务，再通过设备通信通道访问 XIAO |
| SSE | 官方文档提供远程 SSE MCP 接入 | 适配采用这一传输方式的服务 |
| 端 MCP | 控制台要求 APP 包名、签名指纹、最低 APP 版本，可选荣耀 ROM 版本和环境变量 | 值得验证“手机 App 作为设备桥接层”的方向 |

端 MCP 表单的字段包括 `packageName`、`appAccessKey`、`minAppVersion`、`romVersion`。这证明当前平台存在与手机应用关联的 MCP 注册入口；不能据此推断 ESP32 可直接注册、任意 Android 服务可直接被调用，或手机锁屏后仍可长期运行。

公开 MCP 指南目前主要说明 StreamableHTTP 和 SSE。端 MCP 的 SDK、服务声明、调用协议、权限与支持机型尚未核实，因此“YOYO → 端 MCP App → BLE → XIAO”应作为待验证方案。此前把平台 MCP 概括为只有远程服务，应以本节修正。

来源：[MCP 公开指南](https://developer.honor.com/cn/doc/guides/101724)、[当前控制台中的创建插件表单](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)。

## 独立 Skill 负责任务流程

“我的技能”管理以 `SKILL.md` 为核心的技能包。当前创建页有文件编辑器，以及 `scripts`、`references`、`assets` 目录；基本信息包含描述、引导示例、`allowed tools`、执行环境、版本和 APIKey 配置开关。

`allowed tools` 是工具名称声明。当前页面用自由标签输入，展开未显示可选工具，也没有看到绑定“我的插件”中 MCP 插件的可见流程。因此，自建 MCP 插件如何供独立 Skill 调用，仍需具体文档或成功联调证明。这个界面观察也不能反向证明平台绝不支持关联。

同样，存在 `scripts` 目录和执行环境选项，不足以证明平台会运行任意 Python、Node 程序或常驻设备服务。需要由执行规范确定运行位置与限制。智能体提示词中的“技能”章节，也不等同于独立 Skill 包。

来源：[技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)、[当前控制台](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225)。

### 对盆栽已有明确依据的能力

官方技能案例提供 BLE 命令调用；`expect_response=true` 可以等待设备业务响应。这使 Skill 能请求实际湿度，再依据本次读数交互，而不只发送单向动作。

项目附件现有固件主要回复命令确认，详细状态仍打印到 USB 控制台。要完成真实交互，需要增加传感器回包、请求关联和设备本地动作控制。已有附件里的 `expect_response=false` 不适合直接沿用到状态查询。

公开能力说明对 MagicOS、YOYO 和蓝牙版本有要求，真机验收应按 Skill 对应版本执行。不要把 A2A 测试文档的较低版本门槛套用到 BLE Skill，也不要据 BLE 连接推断整个 YOYO 对话可离线运行。

来源：[技能开放能力](https://developer.honor.com/cn/doc/guides/101776)、[BLE 调用案例](https://developer.honor.com/cn/doc/guides/101781)、[本项目固件核对](feasibility-review.md)。

## 知识库提供参考资料

知识库可导入资料并绑定到智能体，用检索帮助回答植物养护、设备说明等问题。适合放植物品种、养护手册、传感器测量范围和打印模板说明。

当前湿度、设备是否在线、上次浇水是否成功属于实时状态，应通过工具向设备查询。知识库里的历史说明不能充当这些状态的权威来源。

来源：[创建知识库](https://developer.honor.com/cn/doc/guides/101642)、[配置知识库](https://developer.honor.com/cn/doc/guides/101646)。

## RoboPlanter 的实施建议

### BLE Skill 手机直连备选

```text
用户 → YOYO 主对话 → RoboPlanter Skill → 官方 BLE 工具
                                           ↕
                               XIAO ESP32-S3 Plus
                                  ├─ Grove 传感器
                                  ├─ 12V 水泵及驱动
                                  ├─ 灯带与 12V 打印机
                                  └─ Wi-Fi ↔ SenseCAP Indicator
```

验收流程是“问盆栽状态 → 返回真实采样值 → 用户明确请求后执行有时限的浇水 → 返回实际结果”。XIAO 的 HTTP/BLE 与 Indicator 的 Wi-Fi 状态界面已有独立固件。Indicator 已加入中文五页界面、原创表情、八主题、本地照片与番茄钟；YOYO 对话推送、主题/计时控制接口尚未实现。手机端 BLE 长写、业务回包仍待联调，不能凭固件编译通过认定 Skill 已打通。

### NFC 贴作为实体入口

用户已确认加入 NFC 贴，首版按“手机碰花盆上的无源标签”设计。标签保存盆栽入口链接和非秘密设备标识，实时状态与控制仍由应用及设备通信完成。标签无需连接 XIAO，也不改变 SenseCAP Indicator 的显示终端分工。

Android 支持读取 NDEF URI 并交给相应应用处理，因此可以先设计盆栽网页入口。指定 YOYO Skill 或智能体能否通过该入口直接唤起，尚无已核实的接入方式；不能把普通 NFC URL、端 MCP 注册和 Skill 唤起自动串成已完成的链路。默认读取标签只选择设备和打开入口，浇水、打印由后续交互发起。

来源：[Android NFC 基础](https://developer.android.com/develop/connectivity/nfc/nfc)、[本项目 NFC 设计](feasibility-review.md)。

### 自建 MCP 主线使用统一设备能力

```text
用户 → YOYO 入口 → LLM 模式智能体 → MCP 插件
                                        ↕
                                 自有设备服务
                                        ↕
                                 XIAO 与外设
```

这一路线符合“利用平台模型，自建 MCP 提供盆栽能力”的设想。远程服务到 XIAO 可以另行设计 MQTT 等设备通道；平台不能天然访问花盆的局域网地址。Indicator 可以与同一设备状态接口配合，不需默认承担 MCP 服务器职责。

上层工具围绕 `get_planter_state`、`water`、`stop_water`、`set_light`、`print_note`、`get_action_result` 实现。返回值区分请求已接受、动作已完成、拒绝和结果未知。BLE 与 MCP 可复用相同业务语义，具体传输编码由各自实现适配。YOYO 负责用户交互，Indicator 不承担替代 YOYO 的对话中枢职责。

如果随后取得端 MCP SDK 并验证调用，可评估用手机 App 桥接 XIAO，减少远程设备服务的依赖；这还不能视为已验证的替代架构。A2A 则留给需要接入完整第三方对话服务的阶段，不是当前硬件接入的前置条件。

### 下一步需要取得的证据

1. 端 MCP 的官方 SDK 或最小示例、服务声明方式、应用权限与机型版本要求。
2. 自建 MCP 工具与独立 Skill 的关联方法，以及一次实际成功调用。
3. 真机 BLE 状态查询、动作结果、断连及超时停止记录。
4. 赛事最终提交入口对 Skill、智能体和插件成果的具体要求；独立智能体或插件发布不能自行视为完成 Skill 提交。

本次核对覆盖公开文档和当前控制台的创建表单，未提交新智能体、插件或技能，也未进行手机到硬件的运行验证。
