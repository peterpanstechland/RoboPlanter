#pragma once
#include <cstring>
#include "printer_text.h"

namespace robo {
// One band is 48 bytes * 32 rows. Data is MSB first, 1 = black.
inline void renderPrinterLine(const std::string& text, const PrinterLine& line, uint8_t* pixels) {
  std::memset(pixels, 0, PrinterRowBytes * printer_font::kLineHeight);
  uint16_t x = PrinterMargin;
  size_t offset = line.begin;
  while (offset < line.end) {
    uint16_t code = 0;
    if (!nextPrinterCode(text, offset, code)) return; // Layout already validated.
    const auto* glyph = printer_font::find(code);
    if (!glyph) return;
    const uint8_t stride = (glyph->width+7)/8;
    for (uint8_t row = 0; row < glyph->height; ++row) {
      for (uint8_t col = 0; col < glyph->width; ++col) {
        if (!(printer_font::kBitmap[glyph->offset+row*stride+col/8] & (0x80 >> (col%8)))) continue;
        const int px = x + glyph->x + col, py = glyph->y + row;
        if (px >= 0 && px < PrinterWidth && py >= 0 && py < printer_font::kLineHeight)
          pixels[py*PrinterRowBytes+px/8] |= 0x80 >> (px%8);
      }
    }
    x += glyph->advance;
  }
}

constexpr size_t MaxReceiptRows = 2*PrinterMargin + MaxPrinterLines*printer_font::kLineHeight;
constexpr size_t MaxReceiptWireBytes = 2 + (MaxPrinterLines+2)*8 + MaxReceiptRows*PrinterRowBytes + 6;
// Zero denotes invalid text. Includes initialization, every raster header,
// top/bottom margins, feed and the optional cutter, for a baud-derived deadline.
inline size_t plannedReceiptBytes(const std::string& text, bool cut = false) {
  PrinterLayout layout;
  if (!printerTextLayout(text, layout)) return 0;
  const size_t rows = 2*PrinterMargin + layout.count*printer_font::kLineHeight;
  return 2 + (layout.count+2)*8 + rows*PrinterRowBytes + (cut ? 6 : 3);
}

// Only fixed firmware commands are emitted, after validating the full text.
// Never retry a write or reach the cutter after an earlier write failure.
template <typename Write>
bool writeReceipt(const std::string& text, Write write, bool cut = false) {
  PrinterLayout layout;
  if (!printerTextLayout(text, layout)) return false;
  if (!write(0x1B) || !write('@')) return false;
  const auto band = [&](const uint8_t* pixels, uint16_t rows) {
    const uint8_t header[] = {0x1D, 0x76, 0x30, 0x00, PrinterRowBytes, 0x00,
                             static_cast<uint8_t>(rows), static_cast<uint8_t>(rows>>8)};
    for (uint8_t byte : header) if (!write(byte)) return false;
    for (size_t i=0; i<PrinterRowBytes*rows; ++i) if (!write(pixels[i])) return false;
    return true;
  };
  uint8_t pixels[PrinterRowBytes*printer_font::kLineHeight] = {};
  if (!band(pixels, PrinterMargin)) return false;
  for (uint8_t line=0; line<layout.count; ++line) {
    renderPrinterLine(text, layout.lines[line], pixels);
    if (!band(pixels, printer_font::kLineHeight)) return false;
  }
  std::memset(pixels, 0, sizeof pixels);
  if (!band(pixels, PrinterMargin)) return false;
  // Exactly the supplied sample's feed, followed by optional GS V 0.
  const uint8_t feed[] = {0x1B, 0x64, 0x06};
  for (uint8_t byte : feed) if (!write(byte)) return false;
  if (cut) {
    const uint8_t cutter[] = {0x1D, 0x56, 0x00};
    for (uint8_t byte : cutter) if (!write(byte)) return false;
  }
  return true;
}
}
