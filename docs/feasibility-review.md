# RoboPlanter：套件与接入可行性核对

核对日期：2026-10-24。范围：用户提供的入门 ZIP、其中的 ESP-IDF 固件、V1.0 原理图，以及 Seeed / 荣耀官方文档。本文件记录原始资料的静态分析；原工程未重新编译或烧录，未测量供电或进行真机联调。

后续实现已在独立的 `firmware/xiao/` 与 `firmware/indicator/` 中开始，采用 XIAO 热点 + Indicator HTTP 客户端。具体实现和本机验证以 [项目说明](../README.md) 为准，接线以 [接线文档](wiring.md) 为准；下文对原套件“需新增”的描述不代表独立新固件仍未实现。

## 结论

一块 XIAO ESP32-S3 Plus 具备承担土壤及环境采集、水泵控制、灯带和串口热敏打印机的资源条件。用户已确定使用 SenseCAP Indicator 做显示与触摸交互；它具有自己的处理器，应作为独立交互终端开发，系统形态为“XIAO 控制器 + SenseCAP Indicator”。需要修改现有固件、核对实际 PCB 版本，并按外设型号设计供电。这是架构可行性判断，不是整机运行验证。

“一块主控”指花盆内的控制器数量。YOYO 的对话和大模型推理仍由手机 / 平台承担；若采用荣耀官方远程 MCP 插件路线，还需要可访问的远程服务。当前控制台还提供关联手机 App 的“端 MCP”入口，其 SDK 与设备桥接方式尚待核实。现有 ZIP 不包含 MCP 服务、摄像头驱动或盆栽业务。

摄像头需单独选型：Seeed 官方明确说明 XIAO ESP32-S3 Plus 的 B2B 连接器不兼容 Sense 的即插式摄像头扩展板，不能按“Plus 直接加 Sense 摄像头”的方式搭建。此限制不等于 ESP32-S3 芯片完全不能连接其他摄像头。

## 用户已确认的器材

- 热敏打印机：12V 供电，USB + TTL 接口；型号、峰值电流、TTL 电平、协议与波特率未确认。
- 水泵：12V；型号、额定 / 启动 / 堵转电流与流量未确认。
- Grove 电容式土壤传感器：用户称土壤温湿度；具体 SKU 与实际测量项未确认。
- Grove 灯带：具体型号、数量、供电电压未确认。
- Grove Air Quality Sensor v1.3。
- Grove Temperature & Humidity Sensor：具体芯片 / 型号未确认。
- Grove UV Sensor v1.1。
- 显示终端：已确定为 SenseCAP Indicator；具体子型号未确认。
- NFC 贴：用户确认新增；先按手机碰触花盆上的无源标签设计，具体标签型号与写入内容待定。

以上更新优先于早期“小型 I²C 屏幕 / 未定电压水泵”的设想。

## 已有项目能复用什么

内层工程位于 `reference/seeed-honor-robot-kit/firmware/XIAO-Robot-Kit/`。

- ESP-IDF 工程与 NimBLE BLE 外设通信。
- USB 命令行，以及 BLE 与 USB 共享的命令分发机制。
- NVS 参数保存、设备命名。
- RGB 灯带的 RMT 驱动。
- I²C IMU 与 ADC 电池采集的实现参考。
- 电机驱动参考，但必须先解决下面的引脚版本冲突。

工程 README 声明已验证环境为 ESP-IDF v6.0.2；`sdkconfig.defaults` 配置 16 MB Flash 与 Octal PSRAM，README 声明已验证 8 MB PSRAM。该环境声明来自附件，未在本机重新验证。

原工程主用途是小车：轮速闭环、编码器、机械臂、舞蹈、IMU 恒温与遥测等并非盆栽的必需功能。建议保留原件作为参考，另建精简的 RoboPlanter 固件，避免继承与盆栽无关的启动动作及外设占用。

## 外设可行性与条件

以下是候选分配，不是可直接施工的接线表。打印机、水泵已确认 12V，但尚缺准确型号和电流等参数，也未确认用户实物 PCB 版本。所有外设不必一次全部接入。

| 功能 | 候选接口 | 已确认的限制与需要的修改 |
| --- | --- | --- |
| 土壤湿度 | D0 / GPIO1，Servo3 的信号脚；或选择 I²C 传感器 | GPIO1 在当前固件中未作为两路舵机信号使用。传感器供电和输出电压需与 ADC 匹配；不能把舵机接口的供电脚当成已确认的 3.3V。读数需要干湿标定，不应直接称为准确的土壤含水率。 |
| 12V 水泵 | D8 / GPIO7 控制外部 12V 功率驱动，或另选未占用 GPIO | 现有 Robot Board 电机输出为 5V，不能用于这只 12V 水泵。需要接受 3.3V 控制信号的合适 MOSFET / 驱动模块，按泵电流选型，并根据泵内部结构设计感性负载保护；普通有刷直流水泵需续流保护。上电默认关闭。 |
| RGB 灯带 | Grove RGB，D13 / GPIO40 | 原理图、教程和 `rgb_led.c` 在此一致；可复用 RMT 驱动。灯珠数量、协议、亮度及电源余量需按型号确定。 |
| 12V 热敏打印机 | 优先用 TTL 串口，D6 / GPIO43 TX、D7 / GPIO44 RX | 当前被 VOFA UART0 遥测占用，必须先移除或迁移。12V 供电不代表串口是 12V，也不能据“TTL”断定是 3.3V。确认逻辑电平、协议、波特率、汉字编码 / 字库与峰值电流；打印机单独接 12V 电源支路。USB 打印路径涉及主机驱动，首版不优先。 |
| Grove Air Quality v1.3 | 模拟 ADC 输入 | 官方支持 3.3V / 5V；宜按最终 ADC 接口统一电平。用途是宽谱气体变化的定性参考，不能将读数包装成 CO₂ ppm、PM2.5 或某一气体的准确浓度。 |
| Grove UV v1.1 | 若为 GUVA-S12D 版本，则为模拟 ADC 输入 | 官方 UV Sensor 文档给出紫外响应；需核对该 v1.1 的实物型号。可作为 UV 趋势附加信息，不能直接替代植物可见光照度或 PAR 测量。 |
| Grove 温湿度 | 依型号为单总线数字 GPIO 或 I²C | DHT11 与 SHT31 等产品的接口不同，不应统一假定为 I²C。I²C 方案可使用 D4 / GPIO5、D5 / GPIO6；板载 IMU 地址 0x6A。 |
| SenseCAP Indicator | 独立终端程序，通过 Wi-Fi 获取设备状态并发送操作请求 | 不能当普通 I²C 屏直接驱动。首版建议使用局域网 HTTP 状态与操作接口；远程 MCP 架构可与设备服务共享 MQTT 状态。网络和 UI 均需新增实现。 |
| NFC 贴 | 手机读取无源 NDEF 标签 | 用作盆栽身份与交互入口；这种方案无需接入 XIAO，不占 GPIO，也不需要给贴纸单独供电。标签内容不能代替实时传感器读数；直接唤起指定 YOYO Skill 的方式待验证。 |
| 摄像头 | 待定 | Plus 不兼容 Sense 插拔式摄像头板。手机拍摄、独立视觉模块或重新设计的摄像头接口均需另外验证，不能视作当前固件已有能力。 |

GPIO10 / D16 已用于电池分压采样，不应直接当作空闲土壤湿度输入。若增加另一路 ESP-IDF ADC 采样，需要统一管理 ADC 单元，而不是盲目重复创建同一个 ADC 单元。

若电容土壤模块是官方 Grove Capacitive Moisture Sensor (Corrosion Resistant)，它只有模拟土壤湿度输出，不包含土壤温度；其读数是定性干湿参考。需要核对用户手中模块后才决定是否增加土壤温度测量。

如果土壤、空气质量和 UV 都是模拟输出，将至少需要三个模拟采样通道。若保留小车电机、编码器和舵机，ADC 引脚会更紧张。可以释放不用的小车外设，或选用经规格核对的外部多通道 ADC；普通 Grove Hub 只是并联连接，不能把三个不同模拟输出合并为一个 ADC 输入。

## 按现有 12V 器材规划供电

```text
12V 电源（容量按实际负载核算）
  ├─ 12V 打印机电源支路
  ├─ 12V 水泵电源支路 + 外部驱动
  └─ 12V → 稳压 5V 降压模块
       ├─ Robot Board 指定 5V 输入
       ├─ 显示终端规定的 5V / USB 输入
       └─ 灯带（仅当确认为 5V 型号）

传感器逻辑 / ADC：按模块规格配置 3.3V 供电和信号电平
```

不要把 12V 直接接到 XIAO、Grove、屏幕或 Robot Board 的 USB 5V 入口；也不要因为载板有“7.4V 电池输入”，就默认可以改接 12V。按 12V 器材设计独立供电支路与公共参考地，先确认电流后再定适配器、线径、连接器和降压模块规格。

供电预算需覆盖打印机峰值、水泵启动、显示终端及灯带负载；本次没有这些电流数据，因此不指定未经验证的适配器安培数。软件可先避免打印与泵启动同时发生，但这不能替代足够的硬件供电能力。

## 已确定的显示终端：SenseCAP Indicator

用户于 2026-10-24 确认采用 SenseCAP Indicator。后续界面与通信按此设备设计。

| 项目 | 确定的分工与建议 |
| --- | --- |
| 终端硬件 | ESP32-S3 + RP2040，3.95 英寸 480×480 电容触摸屏 |
| Indicator 职责 | 盆栽表情、植物与环境状态、触摸操作、打印与浇水的结果反馈 |
| XIAO 职责 | 传感器采样、水泵本地控制、灯带、打印队列与真实设备状态 |
| 两端通信 | 局域网 Wi-Fi + HTTP；接口与两端固件已实现，尚待实机联调 |
| 首版界面 | 表情首页 → 今日状态 → 信息 → 照片 → 番茄钟；八主题切换，离线或数据过期时明确显示状态 |
| 供电 | 按终端规定使用 5V / USB 电源，12V 主电源需先降压 |

Indicator 的内置传感器随子型号不同，不能默认所有版本都有 CO₂、TVOC 或温湿度测量。具体子型号待确认，不影响采用其作为显示与触摸终端的决定。内部双 MCU 已使用 UART 通信，不应随意复用该内部串口。

终端可以显示数据并提交操作请求，泵的超时停止与状态判断仍应留在 XIAO 本地。终端关闭、Wi-Fi 断开不应导致泵持续运行。

## 新增 NFC 碰一碰入口

用户于 2026-10-24 确认增加 NFC 贴。当前设计假设为“手机碰花盆上的标签”，由手机读取标签；花盆不需要因此增加 NFC 读卡模块。

建议标签写入 NDEF 格式的 HTTPS 入口，关联一个盆栽标识。手机识别后可进入对应的盆栽页面，再查询真实状态或发起操作。页面与设备映射需要开发；静态标签本身不会更新湿度，也不能仅凭标签内容自动完成账号授权或 BLE 配对。标签只承载入口和非秘密标识，控制鉴权留在应用或服务中。

首版交互目标为“碰一碰 → 识别这盆植物 → 查看状态或选择浇水、打印 → 设备执行 → 手机与 Indicator 显示结果”。单纯读取标签不直接执行浇水。

Android 官方支持根据 NDEF URI 将标签交给相应应用处理；是否自动打开、是否需要用户点击及锁屏行为取决于手机和系统版本，需要在参赛荣耀手机上测试。普通 URL 标签不能直接等同于指定 YOYO Skill 的启动入口，目前未核实这种直达方式。若之后取得官方启动链接或接入规范，再把入口接到 YOYO。

标签贴装在方便手机接近的位置，最终结构装配后验收读取稳定性。若用户实际意图是让花盆识别 NFC 卡片或道具，则需要另行设计 NFC 读卡硬件，与当前无源贴纸方案不同。

来源：[Android NFC 基础与标签分发](https://developer.android.com/develop/connectivity/nfc/nfc)、[无源 NFC 标签说明](https://developer.android.com/reference/android/nfc/Tag)。

## 必须先处理的附件内部矛盾

### M1 与 IMU 加热器的引脚发生互换

| 来源 | M1 两个控制脚 | IMU 加热器控制脚 |
| --- | --- | --- |
| 附带 V1.0 原理图（第 5–6 页） | D1 / GPIO2、D2 / GPIO3 | D12 / GPIO39 |
| 内层固件 | GPIO2、GPIO39 | GPIO3 |

代码证据：`main/motor_control.c:29` 使用 GPIO2 / GPIO39；`main/imu_heater.c:15` 的注释明确提到 V0.9，随后配置 GPIO3。外层入门文档也把 M1 写成 D1 / D2。

这表明代码和附图可能对应不同硬件版本。尚未确认用户实际板子的版本，因此不能单方面宣布其中一个定义适用于实物。原程序启动时会初始化电机，并在 IMU 正常时启动加热器，更应在确认之前避免直接运行原厂二进制或把 M1 改接水泵。

另一个待核实点：电源示意照片中 Servo4 丝印与教程 / 原理图对该口供电的表述不完全一致。候选 GPIO 分配仅引用信号脚，不据此承诺该接口供电电压。

### UART 已有遥测输出

`main/vofa_telemetry.c:11` 起配置 UART0 / GPIO43 / GPIO44，`main/speed_control.c:302` 初始化该遥测。打印机接入必须释放该串口，否则可能发生驱动冲突或把遥测文本误送入打印机。

### BLE 读写能力不等于已经回传传感器数据

`main/ble_motor.c` 已提供 READ、WRITE、WRITE_NO_RSP 和 NOTIFY，但执行命令后默认返回 `OK|命令|$` 或错误状态。`main/main.c:1159` 的 `status` 命令把详细状态打印到控制台，没有将这些数据变成 BLE 业务响应。

因此“手机拿不到盆栽数据”不能仅归因于 YOYO：设备侧也必须实现数据采集、编码和回包。

现有命令缓冲区为 64 字节（可接收长度小于 64），状态缓冲区为 48 字节（有效字符串最多 47）。长中文打印内容、JSON、图片不能直接套用当前短指令协议。需要定义分包、长度限制与结果关联，或通过短命令触发设备端已有的打印模板。

## YOYO、Skill 与 MCP 的边界

### 已有官方依据的 BLE Skill 路线

```text
用户 → YOYO / RoboPlanter Skill
                 ↕ ble_send_command
       XIAO ESP32-S3 Plus
          ├─ 湿度 / 水泵 / 灯带 / 打印机
          └─ Wi-Fi ↔ SenseCAP Indicator
```

荣耀《技能开放案例》（101781）明确说明：`ble_send_command` 的 `expect_response=true` 会等待设备通过通知、指示或读取返回业务数据，并在 `response` 中返回原始字符串。官方测距示例展示了发出请求后获得测量数据。

`ack_type=ack` 只是 BLE 写确认，不能替代业务数据回包，也不能证明水泵动作已完成。包内小车 Skill 统一设置 `expect_response=false`，适用于它的单向动作示例；盆栽查询不能原封不动沿用这个设置。

原型应先证明：读取真实湿度 → YOYO 能复述这次采样值 → 下发有本地时限的泵命令 → 设备回报结果。屏幕、灯带和打印在这个闭环上逐步加入。

### 用户希望采用的自建 MCP 路线

荣耀《MCP 插件》（101724）支持 Streamable HTTP / SSE，配置的是远程 MCP 服务地址；《配置插件》（101712）说明智能体可添加 MCP 插件。

2026-10-24 的实际控制台还提供第三种“端 MCP”，要求填写 APP 包名、签名指纹、最低 APP 版本，并可配置荣耀 ROM 版本。公开远程 MCP 指南不能代表当前平台的全部接入方式。端 MCP 的 SDK、运行协议及是否可由独立 Skill 调用仍待核实，不能直接假定 ESP32 可注册为端 MCP。完整入口关系与选型见[荣耀平台接入研究](honor-platform-modes.md)。

可考虑的架构是：

```text
YOYO 智能体 → 远程 MCP 服务 → 设备通信通道 → XIAO → 外设
```

其中“远程服务到设备”的连接仍需实现，例如设备通过 Wi-Fi 主动连接后端。局域网中 ESP32 的地址不能直接视为荣耀平台可以访问的远程服务。现有项目没有 Wi-Fi 业务通信或 MCP 服务实现。

目前查阅的技能文档未证明：把任意自建 MCP 工具名写入参赛 Skill 的 `allowed-tools`，就能直接调用。MCP 插件接入智能体与比赛 Skill 提交不能自动视作同一流程，需要进一步用开发者账号验证或向赛事方确认。

保留自建 MCP 方向，同时将 BLE Skill 作为已有文档支撑的参赛集成选项。单主控控制外设的可行性不依赖现在就选定其中一种上层接入方式。

## 固件改造范围与验收顺序

1. 确认实物 PCB 版本与外设型号，建立独立的板级引脚配置；启动时让泵保持关闭，去掉原小车自动动作及无关加热功能。
2. 实现湿度采样与校准、统一设备状态，以及 BLE 请求 / 响应。验证返回的是真实新采样值，不是 `OK` 回显。
3. 为水泵实现设备端计时停止、最大单次运行时间和重复请求处理。不能依赖模型稍后再发一条 `stop`；具体时限由水泵实测流量决定。
4. 接入灯带与打印队列，并为独立显示终端实现状态接口和 UI。打印任务不得阻塞 BLE 通信或泵停止控制；先验证短文本，再验证中文和长内容。若采用 MQTT，必须部署双方可访问的 broker；局域网 HTTP 方案则需明确设备地址发现与断线重连。
5. 在具体荣耀手机与 YOYO 版本上验证 Skill 工具调用、业务回包和断连行为。查询类请求与产生动作的请求分别验证重试规则。
6. 单独验证摄像头路径及 MCP 接入；在上述基础功能已经稳定之后再纳入演示。
7. 写入并验证 NFC 入口：目标手机能识别标签、进入正确盆栽页面，重复碰触不重复执行动作；指定 YOYO Skill 的直接唤起需独立验证。

拟新增的接口可包括 `get_state`、`water`、`stop_water`、`set_light`、`print_note`、`set_display`。这些是设计建议，不是附件里已经存在的命令。

当前仍缺：实物 PCB 版本、摄像头方案、打印机型号 / TTL 电平 / 峰值电流、水泵型号 / 电流 / 流量、土壤和温湿度传感器具体型号、灯带规格、Indicator 子型号，以及是否保留原小车底盘。打印机和水泵的额定电压已确认为 12V，显示终端已确定为 SenseCAP Indicator。

## 证据索引

本地来源：套件入门说明、V1.0 原理图、原厂固件 README、BLE 实现、主程序、电机/加热器引脚、UART 遥测、小车 Skill 示例。完整文件定位见[本机参考资料索引](../reference/README.md)。原始套件保留在本机，不随 GitHub 代码仓库上传。

官方在线来源：

- [Seeed XIAO ESP32-S3 系列规格与摄像头兼容性](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [荣耀技能创建和管理](https://developer.honor.com/cn/doc/guides/101775)
- [荣耀技能开放能力](https://developer.honor.com/cn/doc/guides/101776)
- [荣耀技能开放案例：BLE 请求与业务响应](https://developer.honor.com/cn/doc/guides/101781)
- [荣耀 MCP 插件](https://developer.honor.com/cn/doc/guides/101724)
- [荣耀配置插件](https://developer.honor.com/cn/doc/guides/101712)
- [Grove Air Quality Sensor v1.3](https://wiki.seeedstudio.com/Grove-Air_Quality_Sensor_v1.3/)
- [Grove UV Sensor](https://wiki.seeedstudio.com/Grove-UV_Sensor/)
- [Grove 电容式土壤湿度传感器](https://wiki.seeedstudio.com/Grove-Capacitive_Moisture_Sensor-Corrosion-Resistant/)
- [Grove DHT11 温湿度](https://wiki.seeedstudio.com/Grove-TemperatureAndHumidity_Sensor/)
- [Grove SHT31 温湿度](https://wiki.seeedstudio.com/Grove-TempAndHumi_Sensor-SHT31/)
- [SenseCAP Indicator 规格](https://wiki.seeedstudio.com/Sensor/SenseCAP/SenseCAP_Indicator/Get_started_with_SenseCAP_Indicator/)
- [SenseCAP Indicator 硬件架构](https://wiki.seeedstudio.com/SenseCAP_Indicator_Dive_into_the_Hardware/)
- [SenseCAP Indicator 子型号与原生固件](https://wiki.seeedstudio.com/SenseCAP_Indicator_Native_Firmware/)
- [SenseCAP Indicator MQTT 面板案例](https://wiki.seeedstudio.com/SenseCAP_Indicator_Application_Home_Assistant/)

输入 ZIP 的 SHA-256：`E339AFF65BB53A195CE2390877280422C9C97D01B9846DADE68ECA4542CCDBDA`。
