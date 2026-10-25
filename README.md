# RoboPlanter · 工位搭子

> 你先下班，我负责发芽。

一盆有点嘴硬的绿，和一套面向用户自带硬件的 YOYO 接入实践。先把一张工位上的照料与陪伴做好，再逐步验证设施农业与智能灌溉所需的能力。

[互动展示与工位海报](https://peterpanstechland.github.io/roboplanter/) · [完整项目手册](https://peterpanstechland.github.io/docs/projects/2026/roboplanter/) · [本地手册源文件](docs/handbook/index.md) · [自带硬件 Device Kit](integrations/device-kit/README.md)

**三个体验各有边界：** 真实 XIAO 负责传感器和动作；Indicator 提供本地像素养成；网页 3D 角色、粒子、心情和海报是概念演示，不连接设备。当前没有商品售卖、通用即插即用或已上线农业控制的承诺。

2026-10-25 新增 11 篇手册与可运行的离线 Device Kit：覆盖产品设计、硬件接线/备份/烧录、荣耀平台、BLE、设备契约、排障、验收、社媒拍摄与扩展路线。Device Kit 提供严格 profile 校验、设备登记、离线遥测检查和退役；它不读取令牌、不连接设备、不发送命令。当前固件源码仓库为私有；公开资料和下载不改变仓库可见性。

基于用户提供的 **Seeed XIAO ESP32-S3 Plus + XIAO Robot Board** 套件，配合 **SenseCAP Indicator D1** 的智能盆栽。**YOYO 独立 Skill 是主要的对话入口**：目标路径为 Skill → 官方 `ble_send_command` → XIAO BLE，再根据设备业务回包反馈真实结果。XIAO 负责传感器与执行器；Indicator 负责同步显示、触摸和本地操作，经 Wi-Fi HTTP 连接 XIAO。两块板分别通过 USB/5V 供电，无需互接 UART/I²C。

独立 Skill 0.2.0已导入并保存荣耀平台草稿，尚未完成真机端到端验证。硬件令牌安全注入和手机工具长写未验证，当前 Skill 不发送硬件命令。已有 MCP/MQTT 保留为辅助接入，不是 Skill 主线的前置条件；A2A 不需要。

这份实现面向压缩包内的 Plus 主控，不是 XIAO ESP32-S3 Sense。原始套件资料保留在本机 `reference/`，不随代码上传；[参考资料说明](reference/README.md) 记录恢复路径。新程序位于 `firmware/`。

## 从哪里开始

| 内容 | 入口 |
| --- | --- |
| YOYO 独立 Skill、BLE 条件与交互验收 | [integrations/yoyo/README.md](integrations/yoyo/README.md) |
| 辅助 MCP 桥接服务与本机测试 | [integrations/mcp/README.md](integrations/mcp/README.md) |
| 辅助云 MCP、MQTT 与本地网关 | [integrations/cloud/README.md](integrations/cloud/README.md) |
| 云部署、鉴权与真实 broker 验证记录 | [integrations/cloud/INFRA-DEPLOYMENT.md](integrations/cloud/INFRA-DEPLOYMENT.md) |
| XIAO 固件、传感器与驱动开关 | [firmware/xiao/README.md](firmware/xiao/README.md) |
| Indicator 屏幕、触摸与网络客户端 | [firmware/indicator/README.md](firmware/indicator/README.md) |
| Indicator 首次刷写、启动结果与本地备份恢复 | [docs/deployment/indicator-first-flash.md](docs/deployment/indicator-first-flash.md) |
| Worker 像素界面第一版的构建与刷写记录 | [docs/deployment/indicator-worker-ui.md](docs/deployment/indicator-worker-ui.md) |
| 中英文、Wi-Fi 与亮度设置版本的验证进度 | [docs/deployment/indicator-settings-ui.md](docs/deployment/indicator-settings-ui.md) |
| 工位陪伴 UI、设备设置、八主题与番茄钟 | [docs/indicator-ui.md](docs/indicator-ui.md) |
| 四种植物精灵、照料、成长与本地存档 | [docs/indicator-garden.md](docs/indicator-garden.md) |
| 可交互接线图与主题界面预览 | [docs/previews/README.md](docs/previews/README.md) |
| 引脚、电源、Grove 转接与首次联调 | [docs/wiring.md](docs/wiring.md) |
| HTTP/BLE 协议、请求结果与命令行工具 | [docs/device-protocol.md](docs/device-protocol.md) |
| 荣耀 Skill、A2A、MCP 接入研究 | [docs/honor-platform-modes.md](docs/honor-platform-modes.md) |
| 附件与器材可行性分析 | [docs/feasibility-review.md](docs/feasibility-review.md) |

## 已实现

- XIAO 创建 `RoboPlanter-XXXX` 热点，提供带设备令牌的状态、命令和按请求编号查询结果的 HTTP 接口；同时提供 BLE 命令入口。
- 可选模拟土壤、空气质量、UV，以及明确为 SHT31 时的温湿度驱动。未知、未启用、过期或未标定的数据有明确状态，不使用模拟读数。
- 有限时长手动浇水、停泵、兼容 NeoPixel 灯带开关、ASCII 串口打印。泵由独立任务限时关闭；泵与打印互斥；同一次启动内的请求编号去重。
- Indicator 的 480×480 Worker 像素界面：黑炭底、奶油白卡片和八种配色，保留首页 → 今日状态 → 信息 → 照片 → 番茄钟五页。首页加入薄荷、九层塔、蔷薇和多肉精灵，从种子逐步萌芽、长叶、开花和成熟；四株独立存档，只有当前选中者累计在线时间，断电暂停。
- 植物精灵提供生命条、游戏心情、成长值和工位氛围选择。虚拟浇水、施肥、除草与陪伴有冷却，25 分钟专注完成后为当前植物奖励一次。游戏数据不代表传感器或真实植物健康，照料按钮不会驱动水泵。真实设备状态及离线/过期/结果未知继续显示在今日状态；真实浇水和打印仍需确认，停泵无需二次确认，网络恢复不自动重发动作。规则见[养成说明](docs/indicator-garden.md)。
- Indicator 设备设置：中英文切换并保存、2.4 GHz Wi-Fi 扫描与屏幕密码键盘、恢复默认网络、5–100% 背光亮度。五种原生字号改善大字清晰度。本机 D1 不含内置 CO₂/VOC，温湿度需外接；当前未读取 RP2040 传感器。连上 Wi-Fi 不代表已经连上花盆，设置界面也不会触发硬件动作。
- 标准库 Python 联调工具 `tools/device_cli.py`，以及控制核心和 HTTP 客户端测试。
- 基于官方 Python SDK 的六个 MCP 工具：读取状态、限时浇水、停泵、灯光开关、打印、查询动作结果。本机版固定设备地址、保管设备令牌，动作不自动重发；默认 stdio，HTTP 仅监听本机回环地址。
- 云 MCP 已部署至 `https://emqx.robomon.cn/roboplanter/mcp`，同时提供 SSE，使用 HTTPS + Bearer 鉴权。EMQX 的云端和网关两个独立账号已完成 **24 项实际 broker 权限检查**。云服务只转发状态与结果查询；四个动作工具（包括停泵）均返回 `rejected / cloud_writes_not_enabled / sent=false`。
- YOYO独立Skill 0.2.0已保存平台草稿，包含工位陪伴流程、BLE协议和验收说明，另有白名单打包工具；辅助MCP智能体提示词及13条人工验收场景保留。草稿上传不代表BLE已接通或硬件动作已验收。

所有传感器与执行器默认禁用。水泵和打印另需确认驱动/电平，本地执行动作还需启用动作开关并更换默认令牌。首版没有自动浇水策略或相机。照片需本地导入后重新构建；番茄钟由屏幕本地运行，尚无 YOYO 控制接口。

主线计划由[独立 Skill](integrations/yoyo/skill-draft/README.md)经官方 BLE 工具直接连接附近的 XIAO，不经过云 MCP 或 Indicator。辅助网络方案为云 MCP → MQTT → 本地双网络网关 → XIAO HTTP；该网关需要同时连接 XIAO 热点和互联网。**真实设备网关尚未接通**，云端状态查询应返回不可用，不能据云服务健康或工具发现成功认定花盆在线。云端写操作（包括停泵）均禁用，需要后续设备截止时间协议与真机验证，不能通过启用本地动作开关开放。

辅助平台的实际进展保留：LLM 智能体草稿已创建；MCP 六工具已导入、两个只读工具的平台调用通过，插件已提交团队内部发布，提交后处于审核中，尚未绑定智能体。这些结果不等于独立 Skill 已获得 MCP 工具调用能力。独立 Skill 本轮已另建草稿；最新进展见[平台记录](docs/deployment/honor-platform-status.md)。

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

构建脚本不会烧录。Indicator 已于 2026-10-24 经 ESP32 的 CH340 接口完成首次刷写，刷前保存并校验了本地 8 MB 备份。用户反馈首版频闪，随后确认旧界面的 Canvas 修复版“频闪消失，触摸正常”。Worker 像素第一版已构建并上传校验成功；见[第一版记录](docs/deployment/indicator-worker-ui.md)。中英文、Wi-Fi 和亮度设置 v2 也已刷写，用户随后确认三项正常，详见[设置版本记录](docs/deployment/indicator-settings-ui.md)。养成 v3 在第一轮刷新中断修订后仍收到偶发闪屏反馈；当前已刷入 VSYNC 双缓冲版本，完成 96 次帧交换与存档读回，用户随后确认“暂时没有再闪，触摸正常”，见[双缓冲修订记录](docs/deployment/indicator-vsync-double-buffer.md)、[养成历史记录](docs/deployment/indicator-garden-ui.md)和[玩法说明](docs/indicator-garden.md)。RP2040 原固件未操作，XIAO 尚未刷写；备份与恢复方法见[首次刷写记录](docs/deployment/indicator-first-flash.md)。

## 本机验证

2026-10-24 图标微调：Wi-Fi 使用宽扁的双弧线；设置齿轮按后续反馈改为原生细描边，外轮廓和中心小圆之间留白。顶部保持居中对齐，镜像与检查结果见[图标修订记录](docs/deployment/indicator-icons.md)。

本机验证覆盖控制核心、Indicator 页面与实际手势处理、独立设置 UI、网络设置后端、字体、照片打包、HTTP 客户端及 MCP 桥接。养成 v3 新增成长门槛、冷却、独立植物进度、专注奖励和本地存档检查，并扩展原生预览；各版具体验证结果以相应版本记录为准。MCP 测试使用官方 SDK 完成 stdio 握手、发现六个工具，并逐个调用模拟设备；不连接真实硬件或荣耀账号。模拟网络测试不计作实物连网验收。

Indicator 曾分别编译空相册与一张测试照片配置，本轮保持空相册。字库采用 16/24/32/48/64 px 五种原生字号，按当前界面文案生成字符子集。原生 PNG/GIF 预览运行实际固件绘图代码与固定版本 Arduino_GFX 栅格算法；养成版本默认输出为 `build/indicator-preview-v3/`，旧 `build/indicator-preview/` 与 `build/indicator-preview-v2/` 保留。预览使用未知真实花盆状态，以及明确标记的游戏进度和设置样例，不能代替真实背光、网络或屏幕时序验证。

本轮另新增并通过 **云入口 14 项测试 + MQTT 15 项测试**，覆盖鉴权、两种 MCP 传输、会话隔离、过期/retained 消息拒绝、新鲜度和云端禁写。公网服务及真实 broker 的验证另见[部署记录](integrations/cloud/INFRA-DEPLOYMENT.md)，不计作真机验收。

| 已记录构建 | 静态 RAM | 程序 Flash / 分区容量 |
| --- | --- | --- |
| XIAO Robot Board | 78,172 B / 327,680 B（23.9%） | 1,361,145 B / 6,291,456 B（21.6%） |
| SenseCAP Indicator（Worker 第一版历史记录） | 46,160 B / 327,680 B（14.1%） | 861,665 B / 3,342,336 B（25.8%） |

以上为编译器报告，不包含运行中的动态堆、网络缓冲或屏幕帧缓冲峰值。Indicator 在扫描缓冲之外保留 460,800 B 的 PSRAM 离屏画布。表中的 Worker 第一版镜像为 862,032 B，日志为 `tmp/indicator-worker-flash-20261024.log`，不是后续设置版本的指标；新版本尺寸、哈希与刷写状态见[设置版本记录](docs/deployment/indicator-settings-ui.md)。XIAO 构建日志保存于本机 `tmp/xiao-build.log`。默认 XIAO 构建也会解析所有可选驱动源码，但不代表外设已经通过实物验收。

```powershell
# 需要 Visual Studio C++ Build Tools；测试使用本机 C++ 编译器，不连接设备。
.\tools\test-core.ps1
.\tools\test-indicator-ui.ps1
.\tools\test-garden.ps1
.\tools\test-settings-ui.ps1
.\tools\test-indicator-font.ps1
# Python >=3.11；MCP 依赖只安装到本项目 .tools/mcp-python。
.\integrations\mcp\install.ps1
python -m unittest discover -s tests -p 'test_*.py' -v
# 云入口与 MQTT 测试，依赖见 integrations/cloud/README.md。
python -m unittest integrations.cloud.test_server integrations.cloud.test_mqtt -v
# 图片工具/测试需要 Pillow；常规固件构建无需 Pillow。
```

测试覆盖泵超时与计时回绕、重复请求与参数冲突、缓存满时停泵、默认禁用、打印互斥，以及客户端鉴权、拒绝重定向、失败动作不重发、MCP 工具调用和迟到回复不覆盖已完成结果。

旧界面的 Canvas 修复版已有用户“频闪消失，触摸正常”反馈；Worker 第一版也已上传校验，启动观察未见 panic。v2 的字体、背光和 Wi-Fi 扫描已得到用户确认；这些历史结果不自动算作 v3 养成、存档和长时间稳定性的验收。动画更新间隔为 100 ms，上限约 10 fps，实际速度还受绘制和等待帧边界耗时影响。当前双缓冲实现不再整帧复制到正在显示的缓冲区。当前为空相册，XIAO 尚未刷写，真实花盆连接、传感器与执行器尚未验收。具体传感器 SKU、12V 负载电流、打印机协议和 TTL 电平仍需实物检查。BLE 入站长命令依赖手机端 MTU/长写支持，不能据编译或 Indicator 启动成功认定 YOYO 链路已打通。NFC 贴按无源入口标签设计，不占 GPIO；直达指定 YOYO Skill 的链接尚未验证。
