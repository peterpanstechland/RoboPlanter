#include "lcd_diagnostics.h"
#if defined(ARDUINO) && defined(RP_LCD_FLASH_STRESS)
#include <Arduino.h>
#include <Preferences.h>
#include <cstring>
#include "indicator_board.h"

namespace lcd_diagnostics {
void runFlashStress() {
  // Enough changing blobs to cross NVS page boundaries and exercise garbage
  // collection. No writes to the user's rp-garden/rp-settings/rp-ui namespaces.
  Preferences scratch;
  if (!scratch.begin("rp-lcd-test", false)) {
    Serial.println("LCD flash stress: scratch namespace unavailable");
    return;
  }
  delay(300);
  const auto before = indicator::displayStats();
  const uint32_t started = millis();
  uint32_t longestWriteUs = 0;
  unsigned failures = 0;
  uint8_t bytes[156], restored[sizeof(bytes)];
  Serial.println("LCD flash stress: begin 96 scratch writes");
  for (unsigned i = 0; i < 96; ++i) {
    for (size_t j = 0; j < sizeof(bytes); ++j)
      bytes[j] = static_cast<uint8_t>(i + j * 17U);
    const uint32_t startUs = micros();
    const bool saved = scratch.putBytes("probe", bytes, sizeof(bytes)) == sizeof(bytes);
    const uint32_t durationUs = micros() - startUs;
    if (durationUs > longestWriteUs) longestWriteUs = durationUs;
    if (!saved || scratch.getBytes("probe", restored, sizeof(restored)) != sizeof(restored) ||
        std::memcmp(bytes, restored, sizeof(bytes)) != 0) ++failures;
    delay(37); // Vary the write phase relative to the ~24 ms RGB frame.
  }
  if (!scratch.clear()) ++failures;
  scratch.end();
  delay(300);
  const auto after = indicator::displayStats();
  Serial.printf("LCD flash stress: writes=96 failures=%u elapsed_ms=%lu longest_write_us=%lu frames=%lu max_gap_us=%lu current_gap_us=%lu\n",
                failures, static_cast<unsigned long>(millis() - started),
                static_cast<unsigned long>(longestWriteUs),
                static_cast<unsigned long>(after.frames - before.frames),
                static_cast<unsigned long>(after.maxGapUs),
                static_cast<unsigned long>(after.currentGapUs));
  // Diagnostic numbers measure IRQ continuity, not visible pixel correctness.
}
}
#endif
