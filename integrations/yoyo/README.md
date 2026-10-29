# YOYO 独立 Skill 接入

当前主线为 **Skill 0.4.0 → 官方 ble_send_command → 已绑定的 XIAO BLE**。2026-10-29 用户确认配对成功，并确认“读取工位状态”返回土壤、温度和湿度。Indicator 经同一局域网 HTTP 独立连接 XIAO，不转发手机蓝牙。

- [日常使用、首次配对和话术](../../docs/handbook/yoyo-companion.md)
- [平台创建、导入和发布教程](../../docs/handbook/honor-platform.md)
- [当前BLE协议与验收](../../docs/handbook/ble-integration.md)
- [Skill源文件](skill-draft/SKILL.md) · [业务协议](skill-draft/references/ble-contract.md) · [验收用例](skill-draft/references/acceptance.md)

首次在 Indicator“设置 → 花盆连接 → YOYO 与场景小票 → 开启两分钟配对”取得六位码，再到手机系统蓝牙选择自己的设备，输入屏幕上的码。采用SC+MITM加密与绑定是本项目的设计，并非荣耀对所有Skill的强制要求。技能文件不含配对码、HTTP令牌或单次读取密钥。

## 当前能力与验收边界

支持实时状态查询、开关灯、共享番茄钟、五类场景日签、自动打印开关、结果查询，以及官方日程/闹钟入口。设备侧计时和会议日签已测试；手机灯光、计时、自动场景小票、提醒和重启重连仍需端到端验收。水泵未完成负载接线和水箱标定，当前禁用。只能根据业务回包报告结果，不能将BLE写确认或UART发送完成说成实物成功。

自动日签默认开启，可在Indicator或语音中关闭并保存；一次对话最多一张，同场景五分钟冷却，超时不自动补打。土壤未标定保留原始值。NFC仅能使用已配置的真实入口，直达Skill深链接未验证。

## 打包与检查

```bash
python tools/package_honor_skill.py
python -m unittest discover -s tests -p test_honor_skill.py -v
```

打包只包括SKILL.md和两个references文件，输出到build/honor。用于其他设备时先修改广播名称并完成本机配对；不改变UUID或引入明文密钥。上传流程见平台教程。

辅助MCP/MQTT已完成只读服务与网关验证，仍取决于网关在线，云端控制禁用。它不是当前BLE Skill的前置条件。历史插件审核记录保留在部署目录；A2A未实现。
