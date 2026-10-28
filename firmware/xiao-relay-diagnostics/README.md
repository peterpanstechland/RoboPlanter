# SERVO1 relay bench firmware

For XIAO ESP32-S3 Plus / Robot Board V1.0 and Grove Relay v1.2 only.
Disconnect both relay load terminals from the pump/12V before using it.
GPIO42 (SERVO1/J1 pin1, D15) is the active-high relay signal; use the module's
rated supply and common logic ground. SERVO4/GPIO7 belongs to the printer.

This diagnostic application has no Wi-Fi/BLE or automatic boot pulse. GPIO42
starts LOW. On the native USB serial port at 115200 baud:

- `STATUS` + newline returns `RELAY_OFF_READY`.
- `RELAY_TEST_300MS` + newline permits one 300ms pulse per boot, with `RELAY_ON`
  followed by `RELAY_OFF_300MS`. Subsequent tests in that boot are rejected.
- `STATUS` afterwards returns `RELAY_OFF_TEST_USED`.

Those messages verify software GPIO handling, not physical contact closure.
Observe the relay LED/click separately. Preserve the previous application and NVS,
verify board identity before flashing, and restore the production application
immediately after the test. Do not use this application with a loaded pump.
The production firmware retains tank calibration, low-water/invalid-reading
interlocks and its own independent pump deadline; this bench application is not
a way to bypass those protections in normal use.
