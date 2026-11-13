# RoboPlanter · YOYO Desk Garden Companion

[中文](README.md) · **English** · [English experience](https://peterpanstechland.github.io/roboplanter/en/) · [Build guide](https://peterpanstechland.github.io/docs/projects/2026/roboplanter/)

> You clock out. I'll keep growing.

A real plant, a pixel companion and a small paper ritual. RoboPlanter connects HONOR YOYO conversation with a Seeed XIAO controller, a SenseCAP Indicator and an EM5826H BLE thermal printer. Its blocky enclosure holds the phone, a touch display and the plant, with tickets emerging below.

## Architecture

```text
Robot Phone / YOYO <---- encrypted BLE ----> XIAO ESP32-S3 Plus + Robot Board
Indicator D1      <---- Wi-Fi / HTTP ------> XIAO ---- BLE ----> EM5826H
  RP2040 GPIO27 <---- soil probe
  RP2040 <---- internal UART ----> Indicator ESP32
```

The phone pairs with **XIAO**, not the Indicator. The two boards have separate power and no external data wire between them. Cloud MQTT/MCP is optional; A2A is not implemented.

## What is working?

The November 4, 2026 field confirmations include calibrated soil percentages returned by YOYO, bond-preserving reconnect, 30 LEDs changing green → white → green, D7/GPIO44 ultrasonic readings responding to target movement, and seven-page screen previews matching printed tickets with normal proportions and automatic cutting. SEN54, ambient light, soil upload, screen language/brightness/themes and an earlier 0.5-second pump outflow-and-stop test have also been checked.

The full phone scene-print/focus matrix, audible buzzer, complete Wi-Fi-change sequence and a specific NFC Skill entry still need acceptance. The current Skill does **not** expose voice-started watering. Local Skill 1.0.3 is not evidence of the platform-installed version.

## Reproduce it

1. Read the [BOM](docs/handbook/en/hardware-catalog.md) and [current wiring](docs/handbook/en/hardware-wiring.md).
2. Follow [hardware quickstart](docs/handbook/en/hardware-quickstart.md). Use your own private configuration; examples leave actuators disabled.
3. Set up [YOYO](docs/handbook/en/honor-platform.md), then validate the [BLE contract](docs/handbook/en/ble-integration.md).
4. Calibrate actual substrate and tank references, then test one actuator at a time.
5. Check the [acceptance list](docs/handbook/en/acceptance.md) before treating your build as commissioned.

Print Body8, Body9 and Body16, one each. Body20 is a reference volume, not a printed part. The current ultrasonic signal is J10 RX/D7/GPIO44. Printer USB/TTL are disconnected; the printer uses BLE. SEN54 needs independent 5 V with 3.3 V I²C signals. Never feed a 12 V pump/printer supply into Grove pins.

[Engineering API](docs/handbook/en/system-api.md) · [Daily-ticket design](docs/handbook/en/daily-ticket.md) · [Evidence and limits](docs/handbook/en/roadmap-evidence.md)

## Source and credits

By **Shi Pan / Peter Pan's Techland**. Public source availability does not introduce a new blanket software license; third-party notices remain with their files. Credential files, device backups and original vendor bundles are excluded. The assembly hero is a CAD visualization using official HONOR phone-image textures, not a product photograph. Printr inspired the idea of small printed daily rituals.
