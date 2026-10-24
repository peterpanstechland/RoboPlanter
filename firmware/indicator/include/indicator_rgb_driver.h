#pragma once
#include <cstdint>
#include <esp_lcd_panel_rgb.h>

// Board-owned ESP32-S3 scanout. This has its own state and exposes the buffer
// explicitly; it never reinterprets an opaque ESP-IDF or Arduino_GFX handle.
namespace indicator_rgb {
struct Stats {
  bool ready = false;
  uint32_t frames = 0, maxGapUs = 0, lastGapUs = 0, currentGapUs = 0;
};
bool begin(const esp_lcd_rgb_panel_config_t &config, bool vsyncIdleHigh,
           uint16_t **framebuffer);
Stats stats();
} // namespace indicator_rgb
