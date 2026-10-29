# YOYO 0.4 工位交互联调记录

更新：2026-10-29。此记录区分设备侧验收、平台发布与手机端验收；三者不能互相替代。

## 当前结果

- XIAO 与 Indicator 新固件已写入应用分区，保留 Wi-Fi/NVS；RP2040 土壤程序已备份并加入提示音协议。
- 设备侧 HTTP 实测：60秒番茄钟开始、暂停、继续、结束通过；暂停期间剩余时间不变。当前结束回待机。
- “会议幸存签”已通过相同主控控制路径生成并发送；用户确认正常出纸和切纸。保留384点宽、128行图像分带、Logo、真实有效读数、三行待办和78牌正逆位。
- 自动打印默认为开；语音或屏幕均可设置，持久保存。自动场景每类五分钟内最多一次；显式手动打印不受自动开关影响。2026-10-29最后读取开关为开。
- SEN54 温湿度、PM2.5、VOC和距离持续有效；土壤仍是未校准ADC。水箱未标定，实际泵未接，泵关闭。
- 荣耀平台三个技能文件已更新，入口示例为“读取工位状态”“早上好，上班打卡”。平台之后显示“已上架 / 私有发布”，上架时间07:50:35。未把平台发布状态当成手机验收成功，也未取得明确的安全检测结论。
- 手机曾报“读取技能文件异常”。用户也报告已连接，但主控最新诊断仍为connect_count=0、paired=false。因此**手机实际BLE控制、跨断开重连、长中文写入、系统提醒尚未验收**。
- 电脑只读扫描确认 RoboPlanter-A994 广播及服务UUID存在；没有用电脑冒充手机完成绑定。

## 关于新引入的配对码

首次系统配对是本次实现的设计选择，不是荣耀官方强制要求。目的在于不再把设备长期令牌或旧一次性凭据写进 Skill；不是要求每次对话都输入密码。

`ROBO_ENABLE_BONDED_YOYO` 默认0，本机联调配置为1。开启后仅接受公开的 V 协议入口，且每次连接必须通过 SC+MITM+bonding，并匹配设备登记的手机。首次登记窗口由已认证的本地 HTTP/Indicator 开启，有效120秒；PIN只在该窗口的已认证状态接口和屏幕出现，不进入Skill。登记地址与自动打印偏好保存在 rp-companion NVS。

用户对新增配对码提出异议，已停止反复要求配对。该接入体验和真实手机兼容性仍待确认，不能称为“无感接入已完成”。不应为绕过问题而开放无鉴权的水泵、打印或灯光控制；平台APIKey字段也不等于已验证的安全注入方案。

## 本次新增接口

- `focus_start` + `duration_ms`：60000–7200000；运行或暂停中不能重新覆盖。
- `focus_pause` / `focus_resume` / `focus_reset`：共用主控计时。复用请求号去重，停电不恢复计时。
- `auto_print` + boolean `on`：持久开关。
- `print_scene` + `effect` + `text` + boolean `on`：effect为checkin/lunch/meetings/rest/lowbattery；text最多42个UTF-8字节且适配一行；on=true自动、false显式手动。
- `pair_yoyo`：仅已认证HTTP入口可调用；已登记或当前有连接时不重新绑定。不通过Skill发起。
- GET state 增加focus、auto_print、yoyo；原传感器、打印和水泵门禁保留。
- BLE `V|S|desk` 一次返回 K:土壤原始值:十分之一摄氏度:空气湿度；x为无效。另支持soil/env/ambient/pm25/voc/tank/focus/auto等查询。
- BLE完整格式与错误码见[协议](../../integrations/yoyo/skill-draft/references/ble-contract.md)。旧HTTP令牌不会写入Skill。

Indicator经HTTP订阅同一主控状态。远端启动计时会在不打断设置、硬件确认或养成页面时显示计时页；恢复后同步剩余时间。新页面“设置 → 花盆连接 → YOYO 与场景小票”提供首配和自动打印开关。非5/15/25分钟计时不高亮错误的预设。

## 蜂鸣器

[Seeed官方文档](https://wiki.seeedstudio.com/SenseCAP_Indicator_RP2040_Buzzer/)确认内置无源蜂鸣器位于RP2040 GPIO19。它能发提示音，不是播放YOYO语音的扬声器。当前提示音默认关闭，可在番茄钟页打开并保存。

屏幕通过内部UART发 `B|focus\n`，RP2040产生2400Hz、180ms提示音，两秒内限一次；非阻塞，不中断土壤应答，程序退出时关闭PWM。程序已写入并启动，用户听音验收尚未完成。

## 提醒及入口

Skill 使用[荣耀官方开放能力](https://developer.honor.com/en/doc/guides/101776)的 create_schedule / create_alarm / check_schedule，不以持续聊天替代系统定时。10:30须明确日期、时区、是否已过；只在系统确认提醒建立后报告成功。提醒本身不自动唤起AI、打印或后台点外卖。

[荣耀技能管理文档](https://developer.honor.com/en/doc/guides/101775)说明安装指令中的下载链接有效期10分钟。手机加载异常需要核对发布版本和新安装入口，不能直接判断为蓝牙故障。NFC仍只连接到实际配置的入口，未新增或编造官方深链接。

## 本地验证与恢复

- `tools/test-core.ps1`：控制、图像打印、78牌、DHT20、SEN54/水箱、共享计时、场景去重/开关、配对窗口/拒绝其他手机通过。
- `tools/test-device-settings.ps1`：网络、队列、打印及新增23项共享状态/计时入队/过期拒绝检查通过。
- 实际界面渲染590帧，含配对/已配对画面；触摸、固定锚点、换帧失败保护继续通过。
- `tests.test_honor_skill`：包使用公开V协议，不含旧单次读凭据。
- 编译与刷写不替代手机实测。最新测试记录保存在本机tmp；下列备份不含在Git中。

当前XIAO应用：build/device-backups/xiao-20261029-companion-diag/daily-app.bin，SHA256 36657a51e401f4d1a8ed5614667c9cd726cbe9402b8ae240cf64092fd148d77b。

当前Indicator应用：build/device-backups/indicator-20261029-companion/daily-app.bin，SHA256 9d68041a261132327a0c6945b09695aa51c110981310d0e084298db32c7e3fbe。源码追加了自定义分钟数预设高亮修正，需下次刷写同步。

RP2040：build/device-backups/rp2040-20261029-companion，保存旧main.py和soil_uart.py；新soil_uart.py SHA256 81b2d4831019a023665c798b5d738150a1393144625c20ac850585e21a27f9ae。

回退到本次改动前：XIAO xiao-20261028-tank/daily-app.bin；Indicator indicator-20261028-sen54/daily-app.bin。先核对芯片MAC和现有应用哈希，只写0x10000应用分区，不擦整片Flash。
