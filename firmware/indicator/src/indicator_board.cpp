#include "indicator_board.h"
#include <Wire.h>

// Original SenseCAP Indicator ST7701 panel. Pin facts and register mapping:
// Seeed-Solution/SenseCAP_Indicator_ESP32 @77edb8d2, see README sources.
// Only expander port 0 LCD/touch bits are changed; RP2040 reset is port 1.
namespace {
constexpr uint8_t kExpander = 0x20;
constexpr uint8_t kTouch = 0x48;
constexpr uint8_t kCs = 1U << 4;
constexpr uint8_t kLcdReset = 1U << 5;
constexpr uint8_t kTouchIrq = 1U << 6;
constexpr uint8_t kTouchReset = 1U << 7;
uint8_t outputs = 0;
bool i2cHealthy = true;
bool touchOk = false;

bool readBytes(uint8_t addr, uint8_t reg, uint8_t *data, size_t count) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, static_cast<uint8_t>(count)) != count) return false;
  for (size_t i = 0; i < count; ++i) data[i] = Wire.read();
  return true;
}
bool writeRegister(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}
void setOutputs(uint8_t mask, bool high) {
  outputs = high ? outputs | mask : outputs & ~mask;
  if (!writeRegister(kExpander, 0x02, outputs)) i2cHealthy = false;
}

// Arduino_SWSPI supplies the nine-bit command/data stream; chip select lives
// on PCA9535 P04, not ESP32 GPIO4. The inherited CS is deliberately undefined.
class ExpanderSpi : public Arduino_SWSPI {
 public:
  ExpanderSpi() : Arduino_SWSPI(GFX_NOT_DEFINED, GFX_NOT_DEFINED, 41, 48) {}
  void beginWrite() override {
    Arduino_SWSPI::beginWrite();
    setOutputs(kCs, false);
  }
  void endWrite() override {
    setOutputs(kCs, true);
    Arduino_SWSPI::endWrite();
  }
};
ExpanderSpi spi;
Arduino_ESP32RGBPanel panel(
    18, 17, 16, 21,
    4, 3, 2, 1, 0,
    10, 9, 8, 7, 6, 5,
    15, 14, 13, 12, 11,
    1, 10, 8, 50, 1, 10, 8, 20);
Arduino_RGB_Display scanout(480, 480, &panel, 0, false, &spi, GFX_NOT_DEFINED,
                       st7701_type1_init_operations,
                       sizeof(st7701_type1_init_operations));
// RGB DMA keeps scanning its framebuffer even with auto_flush=false. Drawing
// the background directly there exposes each intermediate clear/paint step.
// Keep composition in a separate PSRAM buffer and copy only a complete frame.
Arduino_Canvas canvas(480, 480, &scanout);

// The Seeed Arduino tutorial's full example adds these two orientation bits
// to Arduino_GFX's type1 table and maps touch as (479-x, 479-y).
const uint8_t kOrientation[] = {
    BEGIN_WRITE, WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    WRITE_C8_D8, 0xC7, 0x04,
    WRITE_COMMAND_8, 0xFF,
    WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    WRITE_C8_D8, 0x36, 0x10, END_WRITE};
}

namespace indicator {
bool begin() {
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);
  if (!psramFound()) return false;
  Wire.begin(39, 40, 400000);
  Wire.setTimeOut(20);
  uint8_t direction;
  if (!readBytes(kExpander, 0x02, &outputs, 1) ||
      !readBytes(kExpander, 0x06, &direction, 1)) return false;
  // Preserve all other outputs and directions, especially port 1 RP2040.
  outputs = (outputs | kCs) & ~(kLcdReset | kTouchReset);
  if (!writeRegister(kExpander, 0x02, outputs)) return false;
  direction = (direction & ~(kCs | kLcdReset | kTouchReset)) | kTouchIrq;
  if (!writeRegister(kExpander, 0x06, direction)) return false;
  delay(10);
  setOutputs(kLcdReset | kTouchReset, true);
  delay(120);
  if (!canvas.begin(12000000L)) return false;
  spi.batchOperation(kOrientation, sizeof(kOrientation));
  if (!i2cHealthy) return false;
  canvas.fillScreen(0);
  present();
  digitalWrite(45, HIGH);
  uint8_t probe;
  touchOk = readBytes(kTouch, 0x02, &probe, 1);
  // FT5x06 values from Seeed's ft5x06_init; no address scanning/fallback.
  const uint8_t settings[][2] = {
      {0x80,70},{0x81,60},{0x82,16},{0x83,60},{0x84,10},
      {0x85,20},{0x87,2},{0x88,12},{0x89,40}};
  if (touchOk) {
    for (const auto &setting : settings)
      touchOk = writeRegister(kTouch, setting[0], setting[1]) && touchOk;
  }
  return true;
}
Arduino_GFX &display() { return canvas; }
void present() {
  canvas.flush();
  // Arduino_Canvas copies to its output but does not call output.flush().
  scanout.flush();
}
bool touchReady() { return touchOk; }
TouchStatus readTouchStatus(int16_t &x, int16_t &y) {
  if (!touchOk) return TouchStatus::Invalid;
  uint8_t data[5];
  if (!readBytes(kTouch, 0x02, data, sizeof(data))) return TouchStatus::Invalid;
  const uint8_t points = data[0] & 0x0F;
  if (points == 0) return TouchStatus::Up;
  if (points != 1) return TouchStatus::Invalid;
  const uint8_t event = data[1] >> 6;
  if (event == 1) return TouchStatus::Up;
  if (event == 3) return TouchStatus::Invalid;
  const uint16_t rawX = ((data[1] & 0x0F) << 8) | data[2];
  const uint16_t rawY = ((data[3] & 0x0F) << 8) | data[4];
  if (rawX >= 480 || rawY >= 480) return TouchStatus::Invalid;
  x = 479 - rawX;
  y = 479 - rawY;
  return TouchStatus::Down;
}
bool readTouch(int16_t &x, int16_t &y) { return readTouchStatus(x, y) == TouchStatus::Down; }
}
