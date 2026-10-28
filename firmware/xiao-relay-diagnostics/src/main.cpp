// BENCH ONLY: relay contact terminals must be disconnected from the pump/12V.
// No Wi-Fi, BLE, printer, or automatic boot pulse. Restore production after test.
#include <Arduino.h>
#include <cstring>
#include <initializer_list>
constexpr int RelayPin = 42; // D15 / SERVO1 / J1 pin1
bool used = false, active = false;
uint32_t started = 0;
char command[32]; size_t usedBytes = 0;
void setup() {
  digitalWrite(RelayPin, LOW); pinMode(RelayPin, OUTPUT);
  for (int pin : {2, 3, 39, 11, 12}) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
  // Printer TX remains idle, including during the bench test.
  digitalWrite(7, HIGH); pinMode(7, OUTPUT);
  Serial.begin(115200);
}
void loop() {
  if (active && uint32_t(millis() - started) >= 300) {
    digitalWrite(RelayPin, LOW); active = false; Serial.println("RELAY_OFF_300MS");
  }
  if (!active && Serial.available()) {
    const char c = char(Serial.read());
    if (c == '\n') {
      command[usedBytes] = 0; usedBytes = 0;
      if (!strcmp(command, "STATUS")) Serial.println(used ? "RELAY_OFF_TEST_USED" : "RELAY_OFF_READY");
      else if (!strcmp(command,"RELAY_TEST_300MS") && !used) {
        used = true; started = millis(); digitalWrite(RelayPin, HIGH);
        active = true; Serial.println("RELAY_ON");
      } else Serial.println("REJECTED");
    } else if (c != '\r') {
      if (usedBytes < sizeof(command)-1) command[usedBytes++] = c;
      else { usedBytes = 0; used = true; Serial.println("REJECTED_OVERFLOW"); }
    }
  }
  delay(1);
}
