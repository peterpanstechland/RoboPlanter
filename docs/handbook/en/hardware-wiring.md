# Current hardware wiring

**Baseline: field-confirmed November 4, 2026.** This table supersedes the historical D8 UART-printer and D8/D9 ultrasonic arrangements. Connector labels refer to XIAO Robot Board V1.0.

![Current functional connection schematic, with power and signal notes](/roboplanter/assets/current-wiring-en.png)

| Peripheral | Signal | Power and notes |
| --- | --- | --- |
| SEN54 | IIC1/J11 or IIC2/J12: SDA = XIAO GPIO5, SCL = GPIO6; address 0x69 | Separate regulated **5 V** VCC; shared GND; I²C signals and pull-ups remain **3.3 V** |
| Ultrasonic Ranger v2.0 | Yellow SIG → UART/J10 **RX/D7/GPIO44** | Separate 3.3 V and GND; white unconnected. One bidirectional SIG, not separate trigger/echo |
| Relay v1.2 | SERVO1/J1 signal, D15/GPIO42 | Confirm module supply; use normally open load contacts for the independent pump circuit |
| Light Sensor v1.2 | SERVO3/J13 signal, D0/GPIO1 | Separate 3.3 V and GND; white unconnected |
| 30-pixel WS2813 | RGB/J7 signal, D13/GPIO40 → DIN through carrier buffer | Suitable 5 V supply and GND; prototype brightness ceiling 24/255 |
| Soil probe | Indicator Grove ADC yellow → **RP2040 GPIO27** | 3.3 V and GND; white/GPIO26 unused |
| EM5826H | **XIAO → printer over BLE** | Independent supply. Printer USB and every TTL signal wire disconnected in the current build |
| Indicator buzzer | Internal RP2040 GPIO19 | Passive buzzer; not a voice speaker |

M2/J9 is not used. SERVO4/D8 is currently unused. UART TX/GPIO43 stays disconnected: ROM boot logs can otherwise print as unwanted text. GPIO44 is now the ultrasonic signal; it cannot simultaneously receive printer TX.

## The three-device link

```text
Robot Phone / YOYO <--- encrypted BLE ---> XIAO
Indicator ESP32   <--- Wi-Fi + HTTP ----> XIAO --- BLE ---> printer
Soil probe ---> Indicator RP2040 --- internal UART ---> Indicator ESP32
```

The Indicator and XIAO use the same 2.4 GHz network and have separate power inputs. They do not need an external UART, I²C or USB data connection between them. MQTT and a cloud server are not required for this local path.

## Power details that matter

The Robot Board I²C power pin is 3.3 V. Split the SEN54 red wire out to a verified 5 V supply; do not feed 5 V into that pin. Use 3.3 V-compatible signal pull-ups or suitable level conversion. The two I²C sockets share GPIO5/6 with the carrier's onboard bus; they cannot be independently repurposed as UARTs.

The pump's 12 V positive goes through the relay's normally open contacts to pump positive. Pump negative returns to its supply negative. The relay has two load terminals, not a third NC terminal. Match the supply, contacts and transient protection to the actual motor. Never put 12 V on a Grove or servo supply pin. Power down before changing wires, and verify the carrier's supply arrangements before combining USB and external supplies.

Internal soil UART: RP2040 TX16 → Indicator ESP32 RX20, RP2040 RX17 ← ESP32 TX19, 115200 baud. These are **internal** links, not instructions to connect XIAO and Indicator UARTs.

After remounting the ultrasonic probe, recheck [tank calibration](sensor-calibration.md). A changed physical height invalidates assumptions about old reference distances.

