# Indicator native pixel preview

Run from the repository root:

```powershell
./tools/render-indicator-ui.ps1
```

Requires Visual Studio C++ Build Tools, the Arduino_GFX dependency installed by
the Indicator PlatformIO build, and Python with Pillow. Use `-Python PATH` or
`-OutputDirectory PATH` to override defaults. This script does not build or flash
device firmware and never opens a serial port.

The harness includes the real `firmware/indicator/src/main.cpp` and
`settings_ui.cpp`, links the real theme, text and photo code plus generated
native-size glyphs, and compiles the installed
Arduino_GFX `Arduino_G.cpp`, `Arduino_GFX.cpp`, and `Arduino_Canvas.cpp` unchanged.
Only Arduino, Preferences, display hardware, PWM and network/settings backend boundaries are stubbed.
This keeps actual RGB565 clipping, shape rasterization, layout and font pixels.
The disconnected fixture leaves readings and actuator states unknown.

Output defaults to `build/indicator-preview-v2/`; the deployed v1 files in
`build/indicator-preview/` are preserved, and the tools reject that exact old
output directory. The new output includes all eight themes on every Chinese page,
theme menus, disabled hardware controls, water/print confirmations, timer phases, PNG animation frames,
eight GIFs, and contact sheets. The default theme also has English captures of
all five pages, the theme menu, controls and both confirmations. Both languages
include settings, an empty Wi-Fi list, About, a sample list and all four keyboard
modes. `manifest.json` records source hashes, language, sample-fixture flags,
capture times and the font format (421 characters at native 16/24/32/48/64 px).
The PNG pixels are lossless; GIF colors are palette limited. Start review with
`zh-main-overview.png`, `en-main-overview.png`, `zh-settings-overview.png`,
`en-settings-overview.png`, then `all-themes-*.png` and the eight GIFs; individual frames are only for close
inspection. The confirmation modal is forced open for layout review with actions
still disabled, not evidence that an offline user could open it normally.
GIFs sample 53 real code frames at 100 ms timestamps across the 5.3-second blink
cycle. This is host-simulated animation timing, not measured hardware FPS.

Before capture, 17 host interaction assertions exercise the actual firmware
`handle()` and `TouchGate`: two-step water/print confirmation, repeat taps, swipe
cancellation, stale-state rejection and direct STOP through an uncertain-command
lock, plus opening settings, hidden-action isolation, settings swipes, language
switching and invalid-touch cancellation. The original 11 action assertions are
retained. Only this isolated test phase enables a counting submit stub; the render
phase throws if any action is attempted. Network/device implementations remain
unexecuted, and no fake sensor values are placed in screenshots or firmware.

The settings backend is a clearly marked host-only sample fixture: its only
network name is the literal `example`. No real adapter scan, network request,
password, NVS store or production backend is used. An attempted connection throws
even at the mock boundary. Backlight changes update a number rather than hardware.
The independently tested production settings UI is still the code drawing and
handling each captured panel; its backend is the simulated part.

These are native code captures, not photographs of the physical display. They
cannot verify RGB scanout timing, PSRAM/DMA flicker, backlight, real touch input,
network behavior or device frame rate. Re-run after UI or generated-font edits.
`-CompileOnly` checks the native harness build without generating captures.
