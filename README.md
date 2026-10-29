# RoboPlanter · 工位搭子

> 你先下班，我负责发芽。

一盆能聊天、看工位环境、陪你专注并打印植物塔罗日签的桌面搭子。主控是 **Seeed XIAO ESP32-S3 Plus + XIAO Robot Board V1.0**，屏幕是 **SenseCAP Indicator D1**。这是当前台架实现；网页角色与接线动效是教学展示，不连接真实设备。

[项目手册](https://peterpanstechland.github.io/docs/projects/2026/roboplanter/) · [互动展示与三维接线](https://peterpanstechland.github.io/roboplanter/#wiring-lab) · [最新接线总表](docs/wiring.md) · [YOYO 使用指南](docs/handbook/yoyo-companion.md)

## 当前状态 · 2026-10-29

| 功能 | 真实验收状态 |
| --- | --- |
| YOYO Skill 0.4.0 | 已私有上架；手机加密配对成功，“读取工位状态”返回土壤、温度、湿度 |
| 环境采集 | SEN54 温湿度/PM2.5/VOC、环境光、超声波距离与 Indicator 土壤上传已联调 |
| 小票 | 中文、Logo、状态、三行待办、78张塔罗正逆位；128行图像比例正常，七页预览与实印一致，自动切纸通过 |
| 灯带 | WS2813，10颗、亮度24/255；本地亮灭/灯效已验证，手机开关灯待验收 |
| 番茄钟 | 主控共享计时，设备接口开始/暂停/继续/结束已验证；手机控制和蜂鸣器听音待验收 |
| 场景日签 | 打卡/午饭/会议/休息/低电量；默认自动打印，可持久关闭。设备侧会议小票已出纸，手机场景调用待验收 |
| 水箱与水泵 | 距离测量、继电器空载300ms吸合后释放已验证；水箱未完成，负载端空着，浇水禁用 |
| 设备设置 | 中英文、亮度、Wi-Fi保存；植物养成、换主题、触摸及双缓冲显示已验收 |

最新源码还包含更明确的“显示六位配对码”提示和大字号配对码，已编译但尚未再次刷入；不能据源码或文档更新认定设备已运行这版界面。历史版本、固件哈希与待测项目见[进度与证据](docs/handbook/roadmap-evidence.md)。

## 连接关系

```text
Robot Phone / YOYO Skill ── 加密 BLE ── XIAO Robot Board
                                              ↑
Indicator D1 ── 同一2.4GHz路由器 / 鉴权HTTP ─────┘
    └─ RP2040 Grove ADC：土壤传感器
```

两块板分别USB/5V供电，无需互接UART或I²C。首次手机配对在 Indicator 的“设置 → 花盆连接 → YOYO 与场景小票 → 开启两分钟配对”查看六位码。正常绑定后的重连无需重新输入；重启重连仍待完整实测。BLE 不在技能文件存放令牌；HTTP 使用本地私有配置。

当前外设分配：SEN54接IIC、独立5V且信号3.3V；SERVO1为继电器GPIO42，SERVO2为超声波GPIO8，SERVO3为环境光GPIO1，SERVO4为打印TX GPIO7，打印返回接原UART RX GPIO44；RGB为灯带GPIO40。土壤在Indicator GPIO27。**M2不使用，GPIO43不接打印机，继电器负载端仍留空。** 逐线供电和插座编号见[接线总表](docs/wiring.md)。

SEN54已取代DHT20和旧Air Quality；UV已换成Light v1.2。SEN54不提供CO₂或NOx。土壤未标定只报告原始值，超声波未标定只报告距离，游戏血条不代表真实植物健康。

## 从哪里开始

| 目标 | 文档 |
| --- | --- |
| 首次安装、备份与构建 | [硬件从零上手](docs/handbook/hardware-quickstart.md) |
| 接线、供电和当前器材 | [硬件连接](docs/wiring.md) |
| Robot Phone入口、配对、自然对话与场景 | [YOYO陪伴与控制](docs/handbook/yoyo-companion.md) |
| 创建和上传Skill | [荣耀平台教程](docs/handbook/honor-platform.md) |
| BLE配对、命令与回包 | [BLE接入](docs/handbook/ble-integration.md) · [完整设备协议](docs/device-protocol.md) |
| 中文打印与日签 | [打印机教程](docs/handbook/uart-printer.md) · [日签设计](docs/handbook/daily-ticket.md) |
| 固件与界面 | [XIAO](firmware/xiao/README.md) · [Indicator](firmware/indicator/README.md) · [植物养成](docs/indicator-garden.md) |
| 检查、故障与未完成项 | [验收](docs/handbook/acceptance.md) · [排障](docs/handbook/troubleshooting.md) · [进度](docs/handbook/roadmap-evidence.md) |
| 自带硬件与农业知识原型 | [离线Device Kit](integrations/device-kit/README.md) · [物候设计](docs/handbook/plant-companion.md) · [知识库](docs/handbook/crop-knowledge.md) |

## 配置与构建

新设备只从示例创建私有配置，不覆盖已经配好的文件。传感器/执行器按实际接线逐项启用；不要照搬这台样机的设备名、地址或绑定。

```powershell
Copy-Item firmware/xiao/include/config.local.example.h firmware/xiao/include/config.local.h
Copy-Item firmware/indicator/include/local_config.example.h firmware/indicator/include/local_config.h
.\tools\build.ps1 -Target all
```

需要Python3和首次联网安装的固定版本PlatformIO工具链。配置详见硬件教程：两板可接同一路由器，Indicator的配对SSID、主控地址和HTTP令牌必须匹配。Wi-Fi设置写入设备闪存，断电保留；屏幕换网不会自动改写受信任的花盆地址/令牌。

构建脚本只构建、不烧录。私有配置、设备备份、密钥和本地日志不提交。刷写前按教程核对芯片/端口并保存备份，当前已连接设备不需要因阅读此文档重新刷写。

## 辅助接入与边界

[本机MCP](integrations/mcp/README.md)和[云MCP/MQTT](integrations/cloud/README.md)用于辅助查询，YOYO本地BLE不依赖云端。曾经通过本地网关读到真实土壤；可用性取决于网关与设备在线。云端写操作仍禁用。A2A未实现，NFC直达Skill入口尚待验证。

本仓库保持原有可见性；公开下载来自逐文件白名单，不包含私有配置、备份或套件原始附件。源码可读不等于新增项目开源许可。原始资料说明见[reference](reference/README.md)，测试入口见[核心测试](tests/core/README.md)和[手册验收](docs/handbook/acceptance.md)。
