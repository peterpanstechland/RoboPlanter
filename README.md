# RoboPlanter 智能盆栽

基于用户提供的 **Seeed XIAO ESP32-S3 Plus + XIAO Robot Board** 套件，配合 **SenseCAP Indicator** 的智能盆栽。**YOYO 是主要的对话入口**：设计目标是理解用户的话，通过自有 MCP 查询和控制盆栽，再反馈真实结果。当前云 MCP 已上线，设备网关尚未接通，云端所有写操作（包括停泵）均未启用。XIAO 负责传感器与执行器；Indicator 负责同步显示、触摸和本地操作。两块板使用 Wi-Fi HTTP 通信，分别通过 USB/5V 供电，无需互接 UART/I²C。

这份实现面向压缩包内的 Plus 主控，不是 XIAO ESP32-S3 Sense。原始套件资料保留在本机 `reference/`，不随代码上传；[参考资料说明](reference/README.md) 记录恢复路径。新程序位于 `firmware/`。

## 从哪里开始

| 内容 | 入口 |
| --- | --- |
| YOYO 角色、工具调用与交互验收 | [integrations/yoyo/README.md](integrations/yoyo/README.md) |
| 自有 MCP 桥接服务与本机测试 | [integrations/mcp/README.md](integrations/mcp/README.md) |
| 已上线云 MCP、MQTT 与本地网关 | [integrations/cloud/README.md](integrations/cloud/README.md) |
| 云部署、鉴权与真实 broker 验证记录 | [integrations/cloud/INFRA-DEPLOYMENT.md](integrations/cloud/INFRA-DEPLOYMENT.md) |
| XIAO 固件、传感器与驱动开关 | [firmware/xiao/README.md](firmware/xiao/README.md) |
| Indicator 屏幕、触摸与网络客户端 | [firmware/indicator/README.md](firmware/indicator/README.md) |
| 工位陪伴 UI、八主题、照片与番茄钟 | [docs/indicator-ui.md](docs/indicator-ui.md) |
| 可交互接线图与主题界面预览 | [docs/previews/README.md](docs/previews/README.md) |
| 引脚、电源、Grove 转接与首次联调 | [docs/wiring.md](docs/wiring.md) |
| HTTP/BLE 协议、请求结果与命令行工具 | [docs/device-protocol.md](docs/device-protocol.md) |
| 荣耀 Skill、A2A、MCP 接入研究 | [docs/honor-platform-modes.md](docs/honor-platform-modes.md) |
| 附件与器材可行性分析 | [docs/feasibility-review.md](docs/feasibility-review.md) |

## 已实现

- XIAO 创建 `RoboPlanter-XXXX` 热点，提供带设备令牌的状态、命令和按请求编号查询结果的 HTTP 接口；同时提供 BLE 命令入口。
- 可选模拟土壤、空气质量、UV，以及明确为 SHT31 时的温湿度驱动。未知、未启用、过期或未标定的数据有明确状态，不使用模拟读数。
- 有限时长手动浇水、停泵、兼容 NeoPixel 灯带开关、ASCII 串口打印。泵由独立任务限时关闭；泵与打印互斥；同一次启动内的请求编号去重。
- Indicator 的 480×480 中文界面：表情首页 → 今日状态 → 信息 → 照片 → 番茄钟，支持左右滑动、八种主题与断电保留主题。显示真实状态及离线/过期/结果未知；独立硬件操作区发送手动操作，网络恢复时不自动重发动作。
- 标准库 Python 联调工具 `tools/device_cli.py`，以及控制核心和 HTTP 客户端测试。
- 基于官方 Python SDK 的六个 MCP 工具：读取状态、限时浇水、停泵、灯光开关、打印、查询动作结果。本机版固定设备地址、保管设备令牌，动作不自动重发；默认 stdio，HTTP 仅监听本机回环地址。
- 云 MCP 已部署至 `https://emqx.robomon.cn/roboplanter/mcp`，同时提供 SSE，使用 HTTPS + Bearer 鉴权。EMQX 的云端和网关两个独立账号已完成 **24 项实际 broker 权限检查**。云服务只转发状态与结果查询；四个动作工具（包括停泵）均返回 `rejected / cloud_writes_not_enabled / sent=false`。
- YOYO 的智能体提示词和 13 条人工交互验收场景：查状态、明确请求后浇水、开关灯、打印鼓励小票，以及离线和结果未知时的反馈。这些场景尚需在 YOYO 上人工验收。

所有传感器与执行器默认禁用。水泵和打印另需确认驱动/电平，本地执行动作还需启用动作开关并更换默认令牌。首版没有自动浇水策略或相机。照片需本地导入后重新构建；番茄钟由屏幕本地运行，尚无 YOYO 控制接口。

当前网络方案为云 MCP → MQTT → 本地双网络网关 → XIAO HTTP；网关需要同时连接 XIAO 热点和互联网。**真实设备网关尚未接通**，云端状态查询应返回不可用，不能据云服务健康或工具发现成功认定花盆在线。云端写操作需要后续设备截止时间协议与真机验证，不能通过启用本地动作开关开放。荣耀平台的 LLM 智能体草稿已创建；MCP 六工具已导入、两个只读工具的平台调用通过，插件已提交团队内部发布，处于审核中，待通过后绑定智能体。见[平台记录](docs/deployment/honor-platform-status.md)。独立 Skill 只有[本地内容草稿](integrations/yoyo/skill-draft/README.md)。A2A 尚未实现，当前主路线不需要 A2A。

## 配置与构建

需要 Python 3，首次构建需联网下载 PlatformIO 和固定版本的工具链。Windows PowerShell 在项目根目录执行：

```powershell
Copy-Item firmware/xiao/include/config.local.example.h firmware/xiao/include/config.local.h
Copy-Item firmware/indicator/include/local_config.example.h firmware/indicator/include/local_config.h
```

编辑两份本地配置，使令牌与热点密码相同。设备令牌使用随机的 16–64 个可打印字符，不含空白或 `|`；本地配置已加入 Git 忽略规则。Indicator 的 SSID 需填写 XIAO 启动串口输出的实际名称。

```powershell
.\tools\build.ps1 -Target all
# 指定 Python 路径时：
# .\tools\build.ps1 -Target xiao -Python 'C:\path\to\python.exe'
```

脚本将 PlatformIO 安装到项目 `.tools/python`，缓存放在根目录 `.pio`，以缩短 Windows 编译器命令行；不修改全局 PlatformIO 配置。两个独立工程也可使用对应目录内的 `platformio.ini` 构建。

输出分别位于：

- `firmware/xiao/.pio/build/xiao_robot_board/firmware.bin`
- `firmware/indicator/.pio/build/sensecap_indicator/firmware.bin`

构建不会烧录。首次上电先只验证两块板的 USB 供电、网络和显示，再逐项连接外设。Indicator 目标是 ESP32 的 CH340 接口，保留 RP2040 原固件；烧录前保留原厂程序恢复方式。完整步骤与电源要求见接线文档。

## 本机验证

2026-10-24：两份固件均编译成功；控制核心 **212 项检查通过**，Indicator UI 模型 **99 项检查通过**；Python 测试合计 **35 项通过**（HTTP 客户端 8 项、MCP 桥接 21 项、图片打包 6 项）。MCP 测试使用官方 SDK 完成 stdio 握手、发现六个工具，并逐个调用模拟设备；不连接真实硬件或荣耀账号。Indicator 空相册与包含一张测试照片的配置均已编译，最终产物为空相册。

本轮另新增并通过 **云入口 14 项测试 + MQTT 15 项测试**，覆盖鉴权、两种 MCP 传输、会话隔离、过期/retained 消息拒绝、新鲜度和云端禁写。公网服务及真实 broker 的验证另见[部署记录](integrations/cloud/INFRA-DEPLOYMENT.md)，不计作真机验收。

| 默认配置构建 | 静态 RAM | 程序 Flash / 分区容量 |
| --- | --- | --- |
| XIAO Robot Board | 77,884 B / 327,680 B（23.8%） | 1,361,121 B / 6,291,456 B（21.6%） |
| SenseCAP Indicator | 46,064 B / 327,680 B（14.1%） | 860,833 B / 3,342,336 B（25.8%） |

以上为编译器报告，不包含运行中的动态堆、网络缓冲或屏幕帧缓冲峰值。构建日志保存于本机 `tmp/xiao-build.log` 与 `tmp/indicator-ui-build.log`。默认 XIAO 构建也会解析所有可选驱动源码，但不代表外设已经通过实物验收。

```powershell
# 需要 Visual Studio C++ Build Tools；测试使用本机 C++ 编译器，不连接设备。
.\tools\test-core.ps1
.\tools\test-indicator-ui.ps1
# Python >=3.11；MCP 依赖只安装到本项目 .tools/mcp-python。
.\integrations\mcp\install.ps1
python -m unittest discover -s tests -p 'test_*.py' -v
# 云入口与 MQTT 测试，依赖见 integrations/cloud/README.md。
python -m unittest integrations.cloud.test_server integrations.cloud.test_mqtt -v
# 图片工具/测试需要 Pillow；常规固件构建无需 Pillow。
```

测试覆盖泵超时与计时回绕、重复请求与参数冲突、缓存满时停泵、默认禁用、打印互斥，以及客户端鉴权、拒绝重定向、失败动作不重发、MCP 工具调用和迟到回复不覆盖已完成结果。

尚未烧录或进行真机测量。显示批次/触摸方向、具体传感器 SKU、12V 负载电流、打印机协议和 TTL 电平仍需实物验收。BLE 入站长命令依赖手机端 MTU/长写支持，不能据编译通过认定 YOYO 链路已打通。NFC 贴按无源入口标签设计，不占 GPIO；直达指定 YOYO Skill 的链接尚未验证。
