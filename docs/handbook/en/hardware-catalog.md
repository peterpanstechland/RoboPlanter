# Hardware and software bill of materials

The supported controller is the **XIAO ESP32-S3 Plus**, not the XIAO Sense camera variant. The display used for field testing is the original **SenseCAP Indicator D1**, not a D1S or D1Pro.

| Hardware | Quantity | Job |
| --- | --- | --- |
| [HONOR Robot Phone](https://www.honor.com/global/phones/honor-robot-phone/) | 1 | YOYO conversation and the Skill's official BLE tool |
| [Seeed XIAO ESP32-S3 Plus](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html) | 1 | BLE/HTTP state and actuator controller |
| XIAO Robot Board V1.0 | 1 | Carrier and peripheral connectors |
| [SenseCAP Indicator D1](https://www.seeedstudio.com/SenseCAP-Indicator-D1-p-5643.html) | 1 | Touch UI; internal RP2040 samples soil |
| Grove SEN54 module/adapter | 1 | Temperature, humidity, particulate matter and VOC index |
| Capacitive analog soil-moisture probe | 1 | Relative wet/dry measurement; must support 3.3 V operation |
| Grove Light Sensor v1.2 | 1 | Ambient-light ADC input |
| Grove Ultrasonic Ranger v2.0 | 1 | Single-signal water-surface distance |
| Grove Relay v1.2 | 1 | Switches the pump's independent supply |
| 12 V DC pump | 1 | Prototype label: 150 mA; verify startup requirements |
| Grove WS2813 strip | 30 pixels | Online status and white fill-light indication |
| EM5826H thermal printer with BLE and cutter | 1 | 58 mm tickets; independent supply as specified by its label |
| Printed Body8, Body9 and Body16 | 1 each | Shell/support, front panel, lid/plant insert |
| Water tank, tubing, thermal paper, wiring, fasteners, suitable supplies and USB data cables | As needed | Mechanical, liquid and electrical assembly |

SEN54 replaces the earlier DHT20 and analog Air Quality module. The light sensor replaces the earlier UV sensor. SEN54 does **not** measure CO₂ or NOx. The D1's own hardware must not be confused with sensor-equipped Indicator variants.

## Software

| Dependency | Project version / role |
| --- | --- |
| PlatformIO Core | 6.1.18 in the documented setup |
| Espressif32 platform | 6.12.0; Arduino-ESP32 2.0.17 |
| ArduinoJson | 6.21.5 |
| Adafruit NeoPixel | 1.12.3; the WS2813 path also uses the project's RMT implementation |
| GFX Library for Arduino | 1.4.7 for the Indicator |
| MicroPython on RP2040 | Follow the tested Indicator RP2040 instructions |
| YOYO Skill | Local package 1.0.3; platform publication is a separate step |
| Python 3 | Packaging, build helpers and host checks |

The BOM intentionally has no guessed prices, unmeasured pump flow rate or universal fastener sizes. Read the [pin and power table](hardware-wiring.md) before ordering adapters.

