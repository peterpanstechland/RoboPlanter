#pragma once
#include <Arduino_GFX_Library.h>

namespace ui_theme {
constexpr uint8_t kCount = 8;
struct Theme {
  const char *name;
  uint16_t background, card, ink, muted, accent, second, face, outline;
  // Light-card text uses these rather than the light-on-dark ink.
  uint16_t cardInk, cardMuted, surface;
};
enum class Icon : uint8_t {
  Water, Temp, Humidity, Light, Print, Yoyo, Settings, Leaf, Clock, Wifi, Back
};
const Theme &get(uint8_t id);
// Artwork is decorative companionship, never a sensor/plant-health diagnosis.
void background(Arduino_GFX &g, uint8_t id);
void face(Arduino_GFX &g, uint8_t id, int16_t cx, int16_t cy,
          int16_t radius, bool blink, uint32_t now);
// Monochrome 16x16 pixel glyph, scaled into a square with top-left x/y.
void icon(Arduino_GFX &g, Icon kind, int16_t x, int16_t y,
          int16_t size, uint16_t color);
// Home props: x28..124 and x356..452, y286..355; desk line at y355.
void deskScene(Arduino_GFX &g, uint8_t id, uint32_t now);
}
