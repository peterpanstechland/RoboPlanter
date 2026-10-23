#include "ui_text.h"
#include "ui_glyphs.h"

namespace {
uint32_t next(const char *&p) {
  const uint8_t a = static_cast<uint8_t>(*p++);
  if (a < 0x80) return a;
  unsigned count = 0;
  uint32_t code = 0, minimum = 0;
  if (a >= 0xC2 && a <= 0xDF) { count = 1; code = a & 0x1F; minimum = 0x80; }
  else if (a >= 0xE0 && a <= 0xEF) { count = 2; code = a & 0x0F; minimum = 0x800; }
  else if (a >= 0xF0 && a <= 0xF4) { count = 3; code = a & 7; minimum = 0x10000; }
  else return '?';
  for (unsigned i = 0; i < count; ++i) {
    const uint8_t c = static_cast<uint8_t>(*p);
    if (c < 0x80 || c > 0xBF) return '?';
    code = (code << 6) | (c & 0x3F);
    ++p;
  }
  if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return '?';
  return code;
}
const ui_font::Glyph &glyph(uint32_t code) {
  size_t left = 0, right = ui_font::kCount;
  while (left < right) {
    const size_t middle = left + (right - left) / 2;
    if (ui_font::kGlyphs[middle].code < code) left = middle + 1;
    else right = middle;
  }
  if (left < ui_font::kCount && ui_font::kGlyphs[left].code == code) return ui_font::kGlyphs[left];
  return ui_font::kGlyphs[ui_font::kFallback];
}
}
namespace ui_text {
void draw(Arduino_GFX &gfx, int16_t x, int16_t y, const char *utf8,
          uint16_t color, uint8_t scale) {
  if (!utf8 || scale == 0 || scale > 4) return;
  const int16_t start = x;
  while (*utf8) {
    const uint32_t code = next(utf8);
    if (code == '\n') { x = start; y += 18 * scale; continue; }
    const auto &g = glyph(code);
    for (int row = 0; row < 16; ++row) {
      const uint16_t bits = g.rows[row];
      int col = 0;
      while (col < 16) {
        if (!(bits & (0x8000U >> col))) { ++col; continue; }
        const int begin = col++;
        while (col < 16 && (bits & (0x8000U >> col))) ++col;
        gfx.fillRect(x + begin * scale, y + row * scale, (col - begin) * scale, scale, color);
      }
    }
    x += g.advance * scale;
  }
}
int16_t width(const char *utf8, uint8_t scale) {
  if (!utf8 || scale == 0 || scale > 4) return 0;
  int32_t line = 0, widest = 0;
  while (*utf8) {
    const uint32_t code = next(utf8);
    if (code == '\n') { if (line > widest) widest = line; line = 0; }
    else line += glyph(code).advance * scale;
  }
  if (line > widest) widest = line;
  return static_cast<int16_t>(widest > 32767 ? 32767 : widest);
}
}  // namespace ui_text
