#pragma once
#include <Arduino_GFX_Library.h>
namespace indicator {
bool begin();
Arduino_GFX &display();
// Compose off-screen, then copy the finished frame to the RGB scanout buffer.
void present();
enum class TouchStatus : uint8_t { Up, Down, Invalid };
// Invalid includes an I2C fault, multiple contacts, or malformed coordinates.
// Callers must cancel a pending gesture on Invalid, not treat it as release.
TouchStatus readTouchStatus(int16_t &x, int16_t &y);
// Legacy wrapper: use readTouchStatus for any release-triggered interaction.
bool readTouch(int16_t &x, int16_t &y);
bool touchReady();
}
