# 本机套件参考资料

项目依据用户提供的 `Seeed XIAO Robot Kit x Honor Robot Phone Getting Started.zip` 核对硬件。解压资料保留在本机 `reference/seeed-honor-robot-kit/`，不纳入 GitHub 代码仓库；其中包含原厂固件、CAD、图片与嵌套构建缓存，独立盆栽固件的编译不依赖这些文件。

如需在另一台电脑复查原始材料，将同一份 ZIP 解压到上述目录。原始资料保持原样，以下路径供定位：

| 材料 | 相对于 `reference/seeed-honor-robot-kit/` 的路径 |
| --- | --- |
| 入门说明 | `Seeed XIAO Robot Kit x Honor Robot Phone Getting Started.md` |
| V1.0 原理图 | `图片和附件/XIAO Robot Board_V1.0.pdf` |
| 原厂工程 | `firmware/XIAO-Robot-Kit/` |
| 原厂 BLE 实现 | `firmware/XIAO-Robot-Kit/main/ble_motor.c` |
| 原厂主程序 | `firmware/XIAO-Robot-Kit/main/main.c` |
| 电机引脚 | `firmware/XIAO-Robot-Kit/main/motor_control.c` |
| 加热器引脚 | `firmware/XIAO-Robot-Kit/main/imu_heater.c` |
| UART 遥测 | `firmware/XIAO-Robot-Kit/main/vofa_telemetry.c` |
| 套件 Skill 示例 | `图片和附件/XIAO Robot Kit机甲小车-SKILL.md` |

本项目的分析结论、接线依据和公开官方来源链接已保存在 [可行性核对](../docs/feasibility-review.md) 与 [接线文档](../docs/wiring.md)。项目代码位于 `firmware/`；不要把原厂底盘程序误当作盆栽固件烧录。
