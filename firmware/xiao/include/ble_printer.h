#pragma once
#include <cstdint>

// Single print worker owns job functions. GATT events and HTTP snapshots are
// synchronized internally; this client never changes the YOYO security policy.
namespace printer_ble {
struct State {
  bool connected = false, ready = false, connecting = false;
  uint16_t mtu = 23;
  uint32_t bytes = 0, elapsedMs = 0;
  const char* phase = "idle";
  const char* error = "";
};
bool begin();
bool isPeer(const uint8_t* address);
bool startJob();
bool writeByte(uint8_t value);
bool finishJob();
void abortJob();
State snapshot();
}
