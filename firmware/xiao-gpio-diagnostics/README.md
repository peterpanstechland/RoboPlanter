# XIAO D8 / D9 空载输出诊断

仅用于 XIAO ESP32-S3 Plus / Robot Board 的 SERVO4/J14 信号 D8（GPIO7）或 SERVO2/J5 信号 D9（GPIO8）。先断电，将待测信号针上的超声波、打印机 TTL 或其他负载全部断开。此程序不是正式花盆固件，没有网络、蓝牙、传感器业务或 NVS 写入。

程序启动后将电机、加热和 GPIO42 水泵继电器保持关闭，D8 / D9 为输入。串口 115200 收到完整的 `TEST_D8_EMPTY` 或 `TEST_D9_EMPTY` 加换行后，对指定信号针分别以 Arduino 和 ESP-IDF 配置执行一次低／高／低读回；高电平各持续约 5ms，采集结束先恢复低电平再输出日志，随后将待测脚恢复为输入。不会自动循环测试，也不接受任意 GPIO 编号。

当 XIAO 已从底板拔下、仅 USB 供电且所有排针悬空时，还可使用 `TEST_D3_EMPTY` 对 GPIO4 作空载参照，以及 `PROBE_BARE_I2C` 对 GPIO5/6 上的 0x38、0x44、0x69、0x6a 作空地址应答检查。I²C 检查不写入传感器命令，结束后 GPIO5/6 恢复为输入。这两个扩展命令不能在未确认外设接线时使用。

`TEST_D7_EMPTY` 可检查空闲的 D7 / GPIO44（原 UART RX）。同样要求该信号针无外部连接；这是寻找替代测距引脚的空载检查，不会自动更改正式固件的引脚配置。诊断使用 USB CDC 串口，不使用 GPIO43/44 作串口通信。

日志的 `input` 是输入读回，`latch` 是输出寄存器位，`output_enabled` 是输出使能位；`mux` / `route` 保留引脚复用和矩阵选择，辅助区分配置错误和电气问题。IDF 路径还解除该引脚的 hold。输出寄存器为 1 而输入为 0 并不单独证明芯片损坏，仍需隔离 Robot Board 和外部线路后判断。

构建：`pio run -d firmware/xiao-gpio-diagnostics`。刷写前核对设备 MAC、分区和当前应用，备份 NVS。只将诊断应用写到 `0x10000`；完成后恢复原正式应用并核验，不使用擦除整片 Flash 的方式。
