#pragma once
#include <Arduino.h>
#include "../../xiao/include/daily_ticket.h"

namespace planter {
struct Reading { float value = 0; bool valid = false; };
enum class NetworkFailure : uint8_t { None, Configuration, Http, Authentication, InvalidState };
struct PrintReceipt {
  char text[161] = {};
  bool requiresRaster = false;
  bool layout32 = false;
  bool isDaily = false;
  robo::DailyTicket daily;
};
struct State {
  bool received = false, wifi = false;
  uint32_t receivedMs = 0, sampleAgeMs = 0;
  uint32_t networkRevision = 0; // A new Wi-Fi selection invalidates old capability data.
  bool ageKnown = false;
  char deviceId[49] = {}, networkError[80] = {};
  NetworkFailure networkFailure = NetworkFailure::None;
  int16_t httpStatus = 0;
  Reading soilRaw, soilPercent, airRaw, uvRaw, temperature, humidity;
  bool soilCalibrationKnown = false, soilCalibrated = false;
  bool pumpKnown = false, pumpEnabled = false, pumpOn = false;
  bool remainingKnown = false;
  uint32_t remainingMs = 0;
  bool lightKnown = false, lightEnabled = false, lightOn = false;
  bool printerKnown = false, printerEnabled = false;
  bool printerRaster = false, printerAutocut = false;
  bool printerLayout32 = false;
  bool printerDaily = false;
  char printerStatus[25] = {};
  char requestId[33] = {}, commandStatus[25] = {}, commandError[80] = {};
  bool commandBlocked = false, uncertain = false;
  uint32_t commandStartedMs = 0;
};
enum class Action : uint8_t { Water, StopWater, Light, Print };
bool begin();
State snapshot();
bool fresh(const State &state, uint32_t now);
bool submit(Action action, bool lightOn = false, const PrintReceipt *receipt = nullptr);
void acknowledgeUncertain();
}
