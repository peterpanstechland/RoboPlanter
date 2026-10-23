#pragma once
#include <Arduino_GFX_Library.h>

// Compact UTF-8 bitmap subset, generated from OFL Noto Sans SC.
// Top-left origin; 16 px line height, or 32 px at scale=2.
namespace ui_text {
void draw(Arduino_GFX &gfx, int16_t x, int16_t y, const char *utf8,
          uint16_t color, uint8_t scale = 1);
int16_t width(const char *utf8, uint8_t scale = 1);
}
