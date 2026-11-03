# XIAO D8 / D9 空载输出诊断

仅用于 XIAO ESP32-S3 Plus / Robot Board 的 SERVO4/J14 信号 D8（GPIO7）或 SERVO2/J5 信号 D9（GPIO8）。先断电，将待测信号针上的超声波、打印机 TTL 或其他负载全部断开。此程序不是正式花盆固件，没有网络、蓝牙、传感器业务或 NVS 写入。

程序启动后将电机、加热和 GPIO42 水泵继电器保持关闭，D8 / D9 为输入。串口 115200 收到完整的 `TEST_D8_EMPTY` 或 `TEST_D9_EMPTY` 加换行后，对指定信号针分别以 Arduino 和 ESP-IDF 配置执行一次低／高／低读回；高电平各持续约 5ms，采集结束先恢复低电平再输出日志，随后将待测脚恢复为输入。不会自动循环测试，也不接受任意 GPIO 编号。

日志的 `input` 是输入读回，`latch` 是输出寄存器位，`output_enabled` 是输出使能位；`mux` / `route` 保留引脚复用和矩阵选择，辅助区分配置错误和电气问题。IDF 路径还解除该引脚的 hold。输出寄存器为 1 而输入为 0 并不单独证明芯片损坏，仍需隔离 Robot Board 和外部线路后判断。

构建：`pio run -d firmware/xiao-gpio-diagnostics`。刷写前核对设备 MAC、分区和当前应用，备份 NVS。只将诊断应用写到 `0x10000`；完成后恢复原正式应用并核验，不使用擦除整片 Flash 的方式。
