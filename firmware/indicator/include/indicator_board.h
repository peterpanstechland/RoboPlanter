#pragma once
#include <Arduino_GFX_Library.h>
namespace indicator {
bool begin();
Arduino_GFX &display();
// Compose off-screen, then copy the finished frame to the RGB scanout buffer.
void present();
// Scanout diagnostics. Counters are sampled without blocking the VSYNC ISR.
// Compare frames before/after a storage operation; maxGapUs is since boot.
struct DisplayStats {
  bool ready = false;
  uint32_t frames = 0, maxGapUs = 0, lastGapUs = 0, currentGapUs = 0;
};
DisplayStats displayStats();
// Backlight duty in percent, clamped to 5..100. May be set before begin();
// the requested duty is applied once the display is ready. Caller persists it.
void setBrightness(uint8_t percent);
uint8_t brightness();
enum class TouchStatus : uint8_t { Up, Down, Invalid };
// Invalid includes an I2C fault, multiple contacts, or malformed coordinates.
// Callers must cancel a pending gesture on Invalid, not treat it as release.
TouchStatus readTouchStatus(int16_t &x, int16_t &y);
// Legacy wrapper: use readTouchStatus for any release-triggered interaction.
bool readTouch(int16_t &x, int16_t &y);
bool touchReady();
}
