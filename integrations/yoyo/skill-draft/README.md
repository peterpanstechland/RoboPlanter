# 独立荣耀 Skill：RoboPlanter 工位小芽

当前源码为 **1.0.1**，使用新标识 `roboplanter-workmate`。2026-11-03，旧入口在开发者平台重新上架后，手机仍显示已下架，因此按用户要求创建独立条目。1.0.1改为通过现有soil查询读取主控校准百分比，完整状态再查询env；无需修改固件或重新配对。平台发布状态与手机实测结果分别记录在[部署记录](../../../docs/deployment/honor-platform-status.md)，不能相互替代。

主线为 **YOYO → 官方 BLE 工具 → 已绑定 XIAO → 外设**。Indicator 通过局域网 HTTP 操作同一主控；打印机已改为 XIAO 直连 BLE，128 行位图和切纸经现场验收。新建技能不需要清除设备绑定，也不改变现有打印配置。

## 打包与平台配置

执行 `python tools/package_honor_skill.py`。为兼容现有发布脚本，输出文件仍为 `build/honor/roboplanter-companion-draft.zip`，包内名称与版本以 `SKILL.md` 为准。

只打包三份文件：根目录 `SKILL.md`、`references/ble-contract.md`、`references/acceptance.md`。无外层文件夹、设备令牌、Wi-Fi 密码、配对码、服务器配置或执行脚本。打包器继续阻断历史单次读取凭据；该检查不代替完整安全审查。

| 字段 | 内容 |
| --- | --- |
| Slug / 技能名称 | `roboplanter-workmate` |
| 展示名称 | `RoboPlanter工位小芽` |
| 源码版本 / 环境 | `1.0.1` / Android（最近发布记录：1.0.0已上架，1.0.1审核中） |
| 新 Skill ID | `96fbbd3c7947445f836919c40d33add0` |
| 头像 | `../assets/roboplanter-160.png` |
| 引导示例 | `你好，小芽`；`读取工位状态` |
| Allowed tools | `ble_send_command,create_schedule,create_alarm,check_schedule,skill_exit` |
| APIKey 开关 | 关闭；设备通过系统加密蓝牙绑定鉴权 |
| 发布范围 | 私有发布，不上架技能市场 |

## 功能与验收边界

- 默认一至两句自然聊天；普通问候不调用硬件。
- 读取实时状态、开关灯、控制番茄钟、查询动作结果。
- 打卡、午饭、会议、休息、低电量场景可自动打印；每次先查持久化开关，超时不自动重印。
- 明确的系统日程或闹钟由官方工具处理，以真实工具结果为准。
- 此技能尚未开放语音启动浇水；保留紧急停泵。硬件侧已完成的泵测试不自动扩展技能权限。

首次绑定在 Indicator 显示六位码后，于手机系统蓝牙中完成；不在聊天中填写或上传该码。2026-11-03用户确认新入口普通问候已通过；旧技能曾完成手机实时读取，新入口仍需独立验证读取和打印。已知的 XIAO / Indicator 打印成功不等于新技能手机端硬件调用已通过。

手机应使用开发者平台同一手机号对应的荣耀账号，在 **YOYO → 技能库 → 我的技能 → 私有技能** 安装新名称的条目，开新对话先说“你好”。若普通问候也失败，先排查技能安装入口，避免反复重配蓝牙。

依据：[技能创建管理](https://developer.honor.com/cn/doc/guides/101775)、[官方 BLE 案例](https://developer.honor.com/cn/doc/guides/101781)。
