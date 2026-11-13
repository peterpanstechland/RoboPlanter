# Create and install the YOYO Skill

RoboPlanter uses a **Skill** as its phone-side interface contract. A separate LLM-agent/MCP experiment exists in the repository, but it is not a prerequisite for the current BLE workflow. A2A is not implemented.

## Package contents

The [downloadable Skill ZIP](/roboplanter/downloads/roboplanter-honor-skill.zip) contains three files:

```text
SKILL.md
references/ble-contract.md
references/acceptance.md
```

The local package is version 1.0.3. Its instructions are currently Chinese; the English handbook explains the same interfaces. Firmware UI language, Skill prompt language and platform-installed version are independent choices.

SKILL.md establishes a warm, brief voice, session continuity, intent-to-command mapping, pairing recovery, calibrated-data handling, scene-print preferences and action-result checks. The reference files specify transport semantics and acceptance criteria. It is a prompt and tool contract, not a resident device driver or a substitute for firmware-side validation.

## Setup workflow

1. Sign into [HONOR Developers](https://developer.honor.com/cn/manageCenter/app/E00225?~id=225) and open the YOYO platform's skills area.
2. Import the ZIP or create the same root and reference-file structure. Match the target device name to your actual XIAO.
3. Configure the platform's supported official tools. Hardware requests use `ble_send_command`; scheduling and exiting use the declared platform tools only where appropriate.
4. Run the platform's current validation and security checks. A button becoming enabled is not a security certification or a hardware acceptance result.
5. Use the platform's private-release/test workflow, then install or update the correct entry on the authorized phone.
6. Pair the phone with XIAO and follow [layered BLE validation](ble-integration.md).

Do not place Wi-Fi, HTTP, MQTT or cloud tokens in a Skill prompt. The current hardware authorization comes from system BLE pairing and encryption.

## Publication is a separate state

The replacement private entry is named RoboPlanter工位小芽. Its greeting worked after an older entry remained unavailable on the phone. The most recent local package update was not evidence of a corresponding platform release; check both the developer console and installed phone entry before comparing versions.

If a greeting itself fails, inspect publication/installation before resetting device bonds. If greeting works but reads fail, investigate the [connection chain](troubleshooting.md).

