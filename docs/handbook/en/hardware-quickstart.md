# Build RoboPlanter from the first boot to a real reading

Build one link at a time: controller → screen → sensors → calibration → phone → printer → pump. This order makes faults observable rather than turning the whole assembly into a black box.

## 1. Prepare the hardware

Use the [BOM](hardware-catalog.md) and [current wiring](hardware-wiring.md). The XIAO project targets ESP32-S3 Plus with 16 MB flash and 8 MB OPI PSRAM. The Indicator firmware targets the original 480×480 D1 display, ST7701 panel and touch address 0x48. Similar product names do not establish binary compatibility.

Disconnect pump loads, motors, heaters and other actuators for initial flashing. Use a known data cable and the correct device USB port; preserve existing firmware and configuration before replacing a working device. Do not connect both carrier and XIAO power inputs without checking the carrier's power design.

## 2. Get and configure the source

Clone the [public repository](https://github.com/peterpanstechland/RoboPlanter) or unpack the [source kit](/roboplanter/downloads/roboplanter-starter.zip). The [download manifest](/roboplanter/downloads/manifest.json) contains archive hashes, and the kit contains a file-level source manifest.

Install Python 3.10+ and the documented toolchain. A separate Python environment avoids changing system packages:

```powershell
py -3 -m venv .tools\build-env
.tools\build-env\Scripts\python.exe -m pip install platformio==6.1.18 esptool==4.9.0
```

Copy the examples only when private configuration files do not already exist:

```powershell
Copy-Item firmware/xiao/include/config.local.example.h firmware/xiao/include/config.local.h
Copy-Item firmware/indicator/include/local_config.example.h firmware/indicator/include/local_config.h
```

Create your own network and device-authentication settings. Keep actuators disabled until their physical wiring and guards have been checked. Example names, addresses and calibration values are not your device configuration.

## 3. Build and flash each controller

```powershell
.tools\build-env\Scripts\python.exe -m platformio run -d firmware/xiao -e xiao_robot_board
.tools\build-env\Scripts\python.exe -m platformio run -d firmware/indicator -e sensecap_indicator
```

These commands build, but do not flash. Identify and verify each serial port, make a device backup, then follow the [XIAO firmware instructions](../../../firmware/xiao/README.md) and [Indicator instructions](../../../firmware/indicator/README.md) for the corresponding upload. Do not overwrite Wi-Fi credentials, HTTP tokens or calibration records from an existing setup.

For the Indicator's soil path, install the [RP2040 responder](../../../firmware/indicator-rp2040-diagnostics/README.md) and explicitly enable `soil_uart.serve()` from its MicroPython `main.py`. This is separate from flashing the Indicator ESP32.

## 4. Connect the two boards

Use the Indicator's Wi-Fi control and the two-device network setup workflow. The boards should join the same 2.4 GHz network. A Wi-Fi connection alone does not prove that HTTP authentication and device identity match: inspect the screen's planter-connection status.

Read state first. Add SEN54, soil, light and ultrasonic inputs one at a time. Cover/uncover the light sensor; move a flat ultrasonic target; check that temperature/humidity respond. Missing, stale or uncalibrated inputs must not become fabricated percentages. Save actual [wet/dry and tank reference points](sensor-calibration.md).

## 5. Add the interaction loop

Pair the **phone with XIAO**, using the current six-digit code shown on the Indicator. Install your [Skill](honor-platform.md), then test a greeting followed by a real state query. Normal reconnection should preserve the bond.

Bring up [BLE printing](uart-printer.md) next. Print one short test, check proportions, then compare the frozen preview with one actual ticket and its cut. Add the 30-pixel status/fill-light strip and its saved thresholds.

## 6. Commission the liquid circuit last

First verify relay activation with its load terminals empty. Check the pump's supply and startup current, install tubing, prime or immerse it as its design requires, and use a receiving container. Measure actual full, stop and empty distances in the installed tank. Verify stop behavior, not just startup. The original prototype passed one 0.5-second outflow-and-stop test; that is not a flow calibration or a continuous-duty test.

Print Body8, Body9 and Body16, one each, and check slicing, fits and liquid/electronics separation on your machine. Body20 is a reference volume, not a printable part. Follow the [acceptance list](acceptance.md) before calling the reproduction complete.

