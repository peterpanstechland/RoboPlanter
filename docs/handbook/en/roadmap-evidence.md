# Implementation progress and field evidence

Documentation prepared November 13, 2026. The latest device confirmations recorded for this release are from November 4; writing an English page does not create a new physical acceptance result.

| Area | Evidence / remaining work |
| --- | --- |
| YOYO entry | Replacement private entry opened and answered a greeting |
| Calibrated phone read | User confirmed the soil percentage was close to the Indicator |
| Reconnect | Ordinary reconnect preserved the phone bond; state reads returned and the screen showed the request reaching XIAO |
| Environment | SEN54 temperature/humidity/PM2.5/VOC, light changes and Indicator soil upload checked |
| Ultrasonic | Moved to J10 RX/D7/GPIO44; target movement changed readings and the screen remained stable |
| LEDs | All 30 pixels green → white fill light → green physically confirmed |
| Printing | XIAO BLE and screen-triggered preview-to-paper path confirmed; proportions, full content and automatic cut passed |
| Pump | Installed prototype previously produced water for a 0.5-second command and stopped; installed tank references still require checking after probe movement |
| UI | Touch, language, brightness, themes, growth features and stable display checked |
| Wi-Fi setup | Shared setup flow implemented; the complete real network-change sequence still needs acceptance |
| Focus and scenes | Device-side implementation exists; the full phone scene-print/focus matrix and audible buzzer remain to be checked |
| NFC / A2A | A specific Skill deep link is not yet verified; A2A is not implemented |

The local Skill is **1.0.3**. This is not a claim about the current published or installed platform version. The current Skill does not expose voice-started watering.

A transient GPIO fault was investigated but its damaged component, if any, was not conclusively identified. Restoring valid readings on D7 does not prove that the earlier D8/D9 pins were physically damaged.

The [public repository](https://github.com/peterpanstechland/RoboPlanter) preserves implementation and chronological field notes. Use those records when changing a claim, and distinguish software completion, tool response and observed physical behavior.

