# Thermal printing: the current BLE path

The URL retains its historical `uart-printer` name for existing links. The current prototype uses **XIAO directly connected to EM5826H over BLE**, not UART, a computer relay or a phone-to-printer connection.

## Connect and test

Power the printer from its specified independent supply. Disconnect its USB and TTL signal wires for the current configuration. On the tested module, double-press FEED after power-up to enable Bluetooth. Do not let another phone or computer occupy its connection while XIAO is testing it.

First print one short ticket and observe the paper. Then open Indicator → Today's status → Hardware actions → Print note, review the seven pages and confirm once. Check that the print matches the preview, text and graphics are not flattened, the whole ticket arrives and the cutter operates.

The firmware uses service `0x18F0` and write characteristic `0x2AF1`, sends ESC/POS bitmaps at 384 dots across, and uses the tested **128-row raster band** path. The 32-row A/B sample printed with compressed vertical proportions on this printer. That model-specific finding should not be replaced with a generic assumption about all ESC/POS printers.

## Why the transport changed

The original UART0 TX/GPIO43 printed ESP32 ROM logs at boot. Moving application UART output to D8 avoided those early logs, and that path initially worked. Later, USB still printed correctly but repeated TTL tests with XIAO and a Debug Probe produced no output. This localized the problem without establishing which component was physically faulty. BLE provided a tested alternative for the demonstration.

The former printer RX/GPIO44 wire is now removed: GPIO44 is the ultrasonic signal. Do not restore a historical UART wiring diagram alongside the new sensor arrangement.

## Completion and recovery

XIAO BLE and Indicator-triggered full tickets have been physically confirmed, with normal proportions and automatic cutting. Software `bytes_sent` does not measure paper presence, printhead temperature or blade position. On uncertain completion, inspect the original request result and the printer before retrying. Restart/re-enable the printer's Bluetooth if it is unavailable; avoid sending a long bitmap repeatedly into an uncertain connection.

See [daily-ticket design](daily-ticket.md) for freezing the random draw and [troubleshooting](troubleshooting.md) for layer-by-layer isolation.

