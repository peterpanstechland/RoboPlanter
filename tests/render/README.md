# Indicator native pixel preview

Run from the repository root:

```powershell
./tools/render-indicator-ui.ps1
```

Requires Visual Studio C++ Build Tools, the Arduino_GFX dependency installed by
the Indicator PlatformIO build, and Python with Pillow. Use `-Python PATH` or
`-OutputDirectory PATH` to override defaults. This script does not build or flash
device firmware and never opens a serial port.

The harness includes the real `firmware/indicator/src/main.cpp`, links the real
theme, text, photo code and generated Chinese glyphs, and compiles the installed
Arduino_GFX `Arduino_G.cpp`, `Arduino_GFX.cpp`, and `Arduino_Canvas.cpp` unchanged.
Only Arduino, Preferences, display hardware and network boundaries are stubbed.
This keeps actual RGB565 clipping, shape rasterization, layout and font pixels.
The disconnected fixture leaves readings and actuator states unknown.

Output is under `build/indicator-preview/`: all eight themes on every page,
theme menus, disabled hardware controls, water/print confirmations, timer phases, PNG animation frames,
eight GIFs, and contact sheets. `manifest.json` records source hashes and capture
times. The PNG pixels are lossless; GIF colors are palette limited. Start review
with `all-themes-*.png` and the eight GIFs; individual frames are only for close
inspection. The confirmation modal is forced open for layout review with actions
still disabled, not evidence that an offline user could open it normally.
GIFs sample 53 real code frames at 100 ms timestamps across the 5.3-second blink
cycle. This is host-simulated animation timing, not measured hardware FPS.

Before capture, a few host interaction assertions exercise the actual firmware
`handle()` and `TouchGate`: two-step water/print confirmation, repeat taps, swipe
cancellation, stale-state rejection and direct STOP through an uncertain-command
lock. Only this isolated test phase enables a counting submit stub; the render
phase throws if any action is attempted. Network/device implementations remain
unexecuted, and no fake sensor values are placed in screenshots or firmware.

These are native code captures, not photographs of the physical display. They
cannot verify RGB scanout timing, PSRAM/DMA flicker, backlight, real touch input,
network behavior or device frame rate. Re-run after UI or generated-font edits.
