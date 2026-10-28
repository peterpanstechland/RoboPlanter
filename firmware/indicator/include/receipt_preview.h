#pragma once
#include "../../xiao/include/printer_receipt.h"

namespace receipt_preview {
// Fit the complete physical page without changing its aspect ratio. The same
// raster bands drive both this preview and the printer; no second font/layout.
template<class Canvas>
bool draw(Canvas& g, const char* text, int x, int y, int width, int height,
          uint16_t paper, uint16_t ink) {
  const std::string value(text);
  robo::PrinterLayout layout;
  if (!robo::printerTextLayout(value, layout)) return false;
  const int rows = robo::printerLayoutRows(layout);
  const int fittedWidth = width*rows > height*robo::PrinterWidth ? height*robo::PrinterWidth/rows : width;
  const int fittedHeight = (rows*fittedWidth+robo::PrinterWidth-1)/robo::PrinterWidth;
  x += (width-fittedWidth)/2;
  y += (height-fittedHeight)/2;
  g.fillRect(x,y,fittedWidth,fittedHeight,paper);
  uint8_t pixels[robo::PrinterRowBytes*robo::PrinterBandRows];
  for (int start=0; start<rows; start+=robo::PrinterBandRows) {
    const int count = rows-start<robo::PrinterBandRows ? rows-start : robo::PrinterBandRows;
    robo::renderPrinterBand(value,layout,static_cast<uint16_t>(start),static_cast<uint8_t>(count),pixels);
    for (int row=0; row<count; ++row) {
      int col=0;
      const int dy = y+(start+row)*fittedWidth/robo::PrinterWidth;
      while (col<robo::PrinterWidth) {
        const auto black = [&](int c) { return pixels[row*robo::PrinterRowBytes+c/8] & (0x80>>(c%8)); };
        if (!black(col)) { ++col; continue; }
        const int first=col++;
        while (col<robo::PrinterWidth && black(col)) ++col;
        const int left=first*fittedWidth/robo::PrinterWidth;
        const int right=(col*fittedWidth+robo::PrinterWidth-1)/robo::PrinterWidth;
        g.drawFastHLine(x+left,dy,right-left,ink);
      }
    }
  }
  return true;
}
}
