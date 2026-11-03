# YOYO 0.4 工位交互联调记录

更新：2026-10-29。此记录区分设备侧验收、平台发布与手机端验收；三者不能互相替代。

## 当前结果

- XIAO 与 Indicator 新固件已写入应用分区，保留 Wi-Fi/NVS；RP2040 土壤程序已备份并加入提示音协议。
- 设备侧 HTTP 实测：60秒番茄钟开始、暂停、继续、结束通过；暂停期间剩余时间不变。当前结束回待机。
- “会议幸存签”已通过相同主控控制路径生成并发送；用户确认正常出纸和切纸。保留384点宽、128行图像分带、Logo、真实有效读数、三行待办和78牌正逆位。
- 自动打印默认为开；语音或屏幕均可设置，持久保存。自动场景每类五分钟内最多一次；显式手动打印不受自动开关影响。2026-10-29最后读取开关为开。
- SEN54 温湿度、PM2.5、VOC和距离持续有效；土壤仍是未校准ADC。水箱未标定，实际泵未接，泵关闭。
- 荣耀平台三个技能文件已更新，入口示例为“读取工位状态”“早上好，上班打卡”。平台之后显示“已上架 / 私有发布”，上架时间07:50:35。未把平台发布状态当成手机验收成功，也未取得明确的安全检测结论。
- 手机首次安全配对和 Skill 实时读取已经验收：2026-10-29用户确认输入 Indicator 上的六位码后配对成功；主控记录 connect_count=5、auth_mode=13（SC+MITM+bonding）、failure_reason=0、stack_success=true、paired=true、connected=true。用户随后在“RoboPlanter工位盆栽伙伴”说“读取工位状态”，确认返回土壤、温度和湿度。该验收基于下方记录的已部署固件，不是尚未刷入的提示改进版。
- **手机执行开关灯/打印/番茄钟、跨断开或重启重连、长中文写入、系统提醒仍待验收**。不能把一次实时读取当成全部控制通过。
- 电脑只读扫描确认 RoboPlanter-A994 广播及服务UUID存在；没有用电脑冒充手机完成绑定。

## 2026-11-03 配对恢复修复

手机丢失原绑定而 XIAO 保留登记时，原配对按钮不可点击。Indicator 现提供“修复手机配对”确认页；操作要求手机先断开。`repair_yoyo` 只在已鉴权的 HTTP 命令入口提供，不加入 BLE/Skill 命令表。

主控阻止修复期间的新连接，只移除已登记手机对应的 BLE bond；删除回调成功后清除 `rp-companion` 的 `phone` 项并开启120秒登记窗口。NVS删除失败时不开放新窗口；删除回调超时保持关闭并提示重启。Wi-Fi、标定、自动打印和打印机信息不清除。重复请求号返回原结果，不重开窗口。结果可用原请求号查询。

新增主机检查覆盖：在线连接拒绝改绑、删除失败保留原登记、删除期间禁止新连接、重复完成不延长窗口、登记超时失效、确认页取消/离线/旧固件/双击，以及发送前重新检查手机已断开。固件编译和界面检查不替代后续手机重配与实际打印验收。

本次两板均已刷入并校验，仅更新0x10000应用分区，保留 NVS。XIAO 原应用与两板设置已备份；Indicator 原应用与分区也已回读校验。刷后主控报告 `repair_supported=true`，旧绑定仍在；Indicator 恢复上传有效土壤数据，SEN54 温湿度有效。

| 固件 | SHA256 | 大小 |
| --- | --- | --- |
| XIAO 配对恢复版 | `81449d17e6d74e7a471be2bc3e564b65cc71e6bdb20b890c85b71c2aa3396d76` | 2,394,800 字节 |
| Indicator 配对恢复版 | `778c6e44530f884e911cdcdcc0b5378c14c9eb9a40fae874624f756e162e1050` | 2,443,600 字节 |

本机回退文件分别位于 `build/device-backups/xiao-20261103-pairing-repair/` 和 `build/device-backups/indicator-20261103-pairing-repair/`；包含私有数据，不提交到 Git。原应用为各目录的 `previous-app.bin`，新应用为 `repair-app.bin`。控制核心、网络设置回归均通过，配对相关客户端34项检查、设置界面89项检查通过；中英文恢复确认页已渲染检查。

## 关于新引入的配对码

首次系统配对是本次实现的设计选择，不是荣耀官方强制要求。目的在于不再把设备长期令牌或旧一次性凭据写进 Skill；不是要求每次对话都输入密码。

`ROBO_ENABLE_BONDED_YOYO` 默认0，本机联调配置为1。开启后仅接受公开的 V 协议入口，且每次连接必须通过 SC+MITM+bonding，并匹配设备登记的手机。首次登记窗口由已认证的本地 HTTP/Indicator 开启，有效120秒；PIN只在该窗口的已认证状态接口和屏幕出现，不进入Skill。登记地址与自动打印偏好保存在 rp-companion NVS。

本次“连不上”的直接反馈是手机已弹出输入框，但用户不知道从哪里查看配对码。六位码在 Indicator 的“设置 → 花盆连接 → YOYO 与场景小票”中，点击“开启两分钟配对”后显示；它不是手机锁屏密码，也不在聊天或 Skill 中。用户找到并输入该码后，主控认证与 Skill 读取均成功。之前失败码99（0x63）在本项目 ESP-IDF 4.4.7 中是配对应答超时：由[BTA错误码转换](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bt/host/bluedroid/bta/include/bta/bta_api.h)、[HCI基础值](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bt/host/bluedroid/stack/include/stack/hcidefs.h)和[SMP_RSP_TIMEOUT](https://github.com/espressif/esp-idf/blob/v4.4.7/components/bt/host/bluedroid/stack/include/stack/smp_api.h)对应得到。

### 已编译、未刷入的配对体验修订

为了保留刚验收成功的手机连接，本次后续改动仅编译和测试，未再次刷写两块开发板：

- Indicator按钮改为“显示六位配对码”，开启后明确提示“在手机输入下方六位码”，PIN放大为48px；成功后提示返回YOYO。当前设备仍显示旧按钮文字。
- 未绑定且两分钟窗口关闭时，在发起加密配对前拒绝连接，避免在已经不可能登记的状态下继续索要PIN。拒绝连接断开后恢复广播。
- 有效窗口内重复开启不更换PIN或延长窗口，避免输入中变码；已绑定手机仍须通过原有加密和身份验证。
- 已认证HTTP状态增加stage、link_connected、window_remaining_ms、disconnect_reason、encryption_error；日志不输出PIN、密钥或手机地址。
- 核心测试覆盖窗口关闭/到期、时间溢出、拒绝第二条连接、已登记手机仍须认证；两板编译和590帧界面渲染通过。

首次绑定是当前实现的设计选择；重连体验尚待单独实测。不应为绕过问题而开放无鉴权的水泵、打印或灯光控制。

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
