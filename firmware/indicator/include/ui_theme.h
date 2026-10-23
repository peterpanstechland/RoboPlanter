#pragma once
#include <Arduino_GFX_Library.h>

namespace ui_theme {
constexpr uint8_t kCount = 8;
struct Theme {
  const char *name;
  uint16_t background, card, ink, muted, accent, second, face, outline;
};
const Theme &get(uint8_t id);
// Artwork is decorative companionship, never a sensor/plant-health diagnosis.
void background(Arduino_GFX &g, uint8_t id);
void face(Arduino_GFX &g, uint8_t id, int16_t cx, int16_t cy,
          int16_t radius, bool blink, uint32_t now);
}
