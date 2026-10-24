# SenseCAP Indicator 首次刷写记录

2026-10-24，经用户明确授权，已将 RoboPlanter 屏幕固件刷入本机 SenseCAP Indicator 的 ESP32-S3。备份、上传校验与启动串口观察已完成。用户反馈首版“显示有频闪，触摸正常”；随后已刷入离屏 Canvas 修复版并确认启动正常，**用户复测确认“频闪消失，触摸正常”**。真实花盆联网操作尚未验收。

## 设备与操作范围

| 项目 | 本次记录 |
| --- | --- |
| 目标芯片 | ESP32-S3，revision 0.2 |
| 存储 | 8 MB Flash、8 MB PSRAM |
| ESP32 接口 | CH340，COM13 |
| RP2040 接口 | COM14，未操作 |
| XIAO | 未刷写 |
| 固件环境 | `sensecap_indicator`，PlatformIO `espressif32@6.12.0` / Arduino ESP32 2.0.17 |
| 上传方式 | PlatformIO `-t upload --upload-port COM13` |

COM 号仅记录这次连接，重新插拔或换电脑后必须重新识别 CH340 对应的 ESP32 接口，不能据此固定使用 COM13。此次操作没有刷写 RP2040，也没有建立 Indicator 与 XIAO 的真实连接。

## 刷前备份与上传结果

刷前读取了 ESP32 的完整 8 MB Flash，包含原有应用、分区和持久化数据。最初以 921600 波特读取时出现通信噪声而失败，改用 460800 后成功。随后对备份执行 `verify_flash`，设备 Flash 与备份的摘要一致。

| 备份 / 应用版本 | 大小 | SHA-256 |
| --- | --- | --- |
| `build/device-backups/indicator-20261024/pre-roboplanter-8mb.bin` | 8,388,608 B | `CF7598F223939E2DEE07DD7A6DBF5F238EC32283354E6611779B0EC0979553E7` |
| 首版应用（历史记录，原构建路径已被新版覆盖） | 862,608 B | `C35C801D1A98C5DE677201C6DFACB42AD4506CE4507B7BEA35237197D99A05E9` |
| Canvas 修复版：`firmware/indicator/.pio/build/sensecap_indicator/firmware.bin` | 865,664 B | `FE8ACC02FE609602A7C70230BD2B589BFB3D1651162C6E7D4E7FC51912D17800` |

首版 PlatformIO 上传的各段镜像均通过设备端哈希校验，随后 Canvas 修复版上传也返回 `SUCCESS`。首版编译报告为静态 RAM 46,064 B、程序 Flash 862,241 B；新版为静态 RAM 46,152 / 327,680 B（14.1%），程序 Flash 865,293 / 3,342,336 B（25.9%）。新版镜像摘要由当前构建文件重新计算。程序 Flash 指标与带镜像头、填充等内容的 `.bin` 文件大小不同；静态 RAM 不包含运行中的帧缓冲、网络缓冲和动态堆。

该完整备份仅存本地，已由仓库的 `build/` 忽略规则排除。它可能包含原设备配置和凭据，**不要提交 Git、上传公开附件或纳入发布包**。仓库中的本记录和摘要不包含备份内容；克隆仓库无法恢复这个本地文件。备份对应的是“本次刷写前的设备状态”，不声称是可分发的干净原厂镜像。

## 启动观察

首版刷写后通过 CH340 以 115200 波特观察约 20 秒，记录到：

```text
RoboPlanter Indicator boot: flash=8388608 psram=8386279
Indicator init: display=ready touch=ready client_task=ready preferences=ready
RoboPlanter: release-only UI; decorative expressions; independent local focus timer.
```

首版观察窗口内未见 panic。用户随后确认“触摸正常”，同时指出“显示有频闪”。`display=ready` 表示显示初始化函数返回成功，`touch=ready` 表示固定地址 0x48 的触摸初始化成功；初始化日志本身不证明实际画面方向、颜色、稳定性或触摸坐标准确。`client_task=ready` 只表示网络任务创建成功，不表示已经连接花盆；`preferences=ready` 表示偏好存储已打开。

针对频闪，随后改为在独立 PSRAM Canvas 上完成一整帧，再复制到 RGB 扫描缓冲并执行缓存写回。旧实现每次在扫描缓冲上直接清屏和逐步重绘；GFX 1.4.7 的 `auto_flush=false` 不会隔离这些中间步骤。新增画布占用 460,800 B，连同扫描缓冲共 921,600 B，不计入静态 RAM 报告。这是整帧复制，不是 VSYNC 同步换帧，不能仅凭代码变更宣布完全消除频闪或撕裂。

Canvas 修复版已重新刷入 COM13，上传 `SUCCESS`。新版 20 秒启动观察已完整结束，观察窗口内未见 panic，随后关闭了串口。日志仍报告四项 `ready`，并新增：

```text
Indicator renderer: offscreen canvas -> complete frame copy.
```

用户复测新版后明确回复“频闪消失，触摸正常”。该反馈确认本次观察到的频闪问题已消除、触摸工作正常；未据此把长时间稳定性、全部页面操作或真实联网功能标为通过。

本次使用默认占位网络配置和空相册，没有写入真实 XIAO 的连接配置。屏幕离线时仍可检查五页导航、八种主题和本地番茄钟；“正在连接花盆”与缺失读数属于此配置下的预期状态。启动诊断不打印 Wi-Fi 密码或设备令牌。

本地日志：

- `tmp/indicator-flash-20261024.log`：首版构建与上传。
- `tmp/indicator-boot-20261024.log`：首版启动串口观察。
- `tmp/indicator-buffered-flash-20261024.log`：Canvas 修复版构建与上传。
- `tmp/indicator-buffered-boot-20261024.log`：Canvas 修复版启动串口观察。

## 待完成的实物检查

1. 用户已确认 Canvas 修复版频闪消失；后续检查长时间显示稳定性、中文可读性和各页完整性。
2. 用户已确认新版触摸正常；后续逐项检查左上返回、右上主题、主题格和底部按钮位置，以及抬手响应、滑动不误点按钮。
3. 验证五页导航、空相册提示、主题切换与重启后保留主题。
4. 验证番茄钟开始、暂停、恢复和重置；换页或换主题不影响计时。
5. XIAO 刷写并配置真实网络后，再验证状态、离线/过期提示、未知结果处理和硬件动作。泵限时与 STOP 的实物测试应先使用安全台架负载。

新版频闪与触摸已有用户实测确认，其余上述专项检查尚未标为通过。固定 ST7701/0x48 配置已在本机启动，但其他 Indicator 屏幕批次仍可能需要不同配置，不能据本次结果推断所有批次兼容。

## 从本地备份恢复 ESP32

以下是后续需要恢复时的手动步骤，本次没有执行恢复。恢复会覆盖 ESP32 的完整 Flash，包括 RoboPlanter 程序及此后保存的偏好，恢复到备份时的状态；不恢复 RP2040。先确认目标仍是同一台 Indicator 的 ESP32，并关闭占用其串口的监视器。

在项目根目录使用已准备好的本地工具链执行。将 `$rpPort` 设置为本次重新识别的 CH340 端口；`$rpPython` 可替换为可用 Python 的完整路径。下例先检查文件大小和摘要，防止误用应用镜像或损坏备份。已通过本地 esptool 4.9.0 的参数定义与 `verify_flash --help` 确认，`verify_flash` 支持以下三个 `keep` 参数；此参数检查没有连接串口或执行恢复。

```powershell
$rpPython = 'python'
$rpPort = 'COM13' # 重新识别 ESP32 CH340 后再填写；不要使用 RP2040 接口。
$rpBackup = Join-Path (Get-Location) 'build/device-backups/indicator-20261024/pre-roboplanter-8mb.bin'
$rpExpected = 'CF7598F223939E2DEE07DD7A6DBF5F238EC32283354E6611779B0EC0979553E7'
if ((Get-Item -LiteralPath $rpBackup).Length -ne 8388608) { throw 'Backup size mismatch' }
if ((Get-FileHash -LiteralPath $rpBackup -Algorithm SHA256).Hash -ne $rpExpected) {
    throw 'Backup SHA-256 mismatch'
}

$rpEsptool = Join-Path (Get-Location) '.pio/packages/tool-esptoolpy/esptool.py'
$rpPreviousPythonPath = $env:PYTHONPATH
try {
    $env:PYTHONPATH = Join-Path (Get-Location) '.tools/python'
    & $rpPython $rpEsptool --chip esp32s3 --port $rpPort --baud 460800 write_flash --flash_mode keep --flash_freq keep --flash_size keep 0x0 $rpBackup
    if ($LASTEXITCODE -ne 0) { throw 'Restore failed; do not claim success' }
    & $rpPython $rpEsptool --chip esp32s3 --port $rpPort --baud 460800 verify_flash --flash_mode keep --flash_freq keep --flash_size keep 0x0 $rpBackup
    if ($LASTEXITCODE -ne 0) { throw 'Restore verification failed' }
} finally {
    $env:PYTHONPATH = $rpPreviousPythonPath
}
```

`keep` 保留备份中的 Flash 参数；完整备份从地址 `0x0` 写入，不能把 `firmware.bin` 应用镜像代入此恢复命令。无需额外执行全片擦除。若串口连接不稳定，可将两条命令均降为 115200 后重试；连接或校验失败时不能认定已恢复。校验通过后重新上电，再观察原有程序的实际启动与画面。
