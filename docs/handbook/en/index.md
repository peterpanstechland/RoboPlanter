# RoboPlanter: a desk companion that grows with you

> You clock out. I'll keep growing.

RoboPlanter combines a real plant, a pixel companion and a tiny paper ritual. Talk to YOYO on an HONOR Robot Phone, check calibrated soil moisture on a SenseCAP Indicator, and print a garden ticket with plant status, three handwritten to-dos and a randomly drawn tarot card.

Built by **Shi Pan / Peter Pan's Techland**. This is a working hardware prototype with a documented reproduction path, not an autonomous gardening product.

![RoboPlanter assembly visualization with an HONOR Robot Phone in its holder](/roboplanter/assets/assembly-hero.png)

*CAD assembly visualization, not a photograph. Phone face and deployed-gimbal imagery are composited from HONOR's official assets; the original Body8/9/16 manufacturing geometry is preserved. See [visual credits](/roboplanter/assets/assembly-hero-sources.json).*

## Start here

| Your goal | Read this |
| --- | --- |
| Understand the idea | [Project story](product-story.md) |
| Build your own | [Hardware quickstart](hardware-quickstart.md) |
| Check parts and pins | [Hardware catalog](hardware-catalog.md) · [Current wiring](hardware-wiring.md) |
| Connect YOYO | [Companion guide](yoyo-companion.md) · [Skill setup](honor-platform.md) |
| Develop an integration | [System API](system-api.md) · [BLE contract](ble-integration.md) |
| Calibrate and commission | [Sensor calibration](sensor-calibration.md) · [Acceptance checks](acceptance.md) |
| Understand the paper experience | [Daily tickets](daily-ticket.md) · [Printer guide](uart-printer.md) |
| Follow remaining work | [Roadmap and evidence](roadmap-evidence.md) |

[English landing page](https://peterpanstechland.github.io/roboplanter/en/) · [Public repository](https://github.com/peterpanstechland/RoboPlanter) · [Source ZIP](/roboplanter/downloads/roboplanter-starter.zip) · [Skill ZIP](/roboplanter/downloads/roboplanter-honor-skill.zip)

## Current evidence

The most recent field confirmations, on November 4, 2026, include YOYO reporting calibrated soil percentages close to the screen, reconnecting without clearing phone bonds, changing all 30 LEDs green → white → green, matching the screen's ticket preview to the printout with correct proportions and automatic cutting, and restoring moving-target ultrasonic readings on D7/GPIO44. The local Skill package is 1.0.3; this does not establish the version installed on the phone.

Voice-triggered scene printing, the complete phone focus-timer flow, the audible buzzer and the full Wi-Fi-change sequence still need their individual acceptance checks. Voice-started watering is not enabled in the current Skill.

