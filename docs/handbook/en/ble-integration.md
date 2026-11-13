# Integrate the official YOYO BLE tool

The local path is **YOYO → official BLE tool → XIAO**, not YOYO → Indicator and not a mandatory cloud proxy. Read the [system API](system-api.md) for UUIDs, request shape and commands.

## Establish trust before commands

Open a short pairing window on the screen, then use the phone's system Bluetooth settings and the current six-digit code. The XIAO records the encrypted owner bond. SKILL.md contains public protocol parameters and behavior rules; it must not contain permanent tokens or temporary diagnostic passwords.

For another XIAO, update the device name consistently in the Skill and configuration. Do not copy the sample A994 identity and expect it to address new hardware.

## Validate in layers

1. Open the Skill and say hello. This checks the platform entry, not hardware connectivity.
2. Read `V|S|soil`. Confirm the business response and its validity/calibration state.
3. Read temperature/humidity separately, then combine only valid fields.
4. Test one light action with a fresh request ID and inspect its result.
5. Test focus control and printing individually. Confirm physical effects where relevant.
6. Reconnect without deleting the bond, then repeat a read.

Observe both sides. “Phone connected” proves a link; “YOYO request reached controller” proves processing reached XIAO; neither alone proves paper emerged or a relay switched.

The 3-second tool response timeout is not a promise that a full ticket is mechanically complete in 3 seconds. Actions can return acceptance and be queried later. A response lost in transit must not cause a duplicate print or watering action.

Legacy soil/desk encodings are documented in the [BLE contract](../../../integrations/yoyo/skill-draft/references/ble-contract.md). Use the named calibrated percentage, never positional guesswork on a raw ADC integer. Keep the reference file aligned with the installed firmware and Skill.

