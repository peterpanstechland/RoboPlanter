#pragma once
#include <cstddef>
#include <cstdint>

namespace robo {
constexpr uint32_t UltrasonicTimeoutUs = 25000;
constexpr size_t UltrasonicMaxEdges = 8;
struct UltrasonicEdge { uint32_t elapsedUs; bool high; };
struct UltrasonicPulse { bool complete = false; uint32_t widthUs = 0; };

// Edges are recorded only after the trigger's falling edge, before releasing
// the shared SIG wire. Ignore an initial LOW observation, never invent a rise.
inline UltrasonicPulse decodeUltrasonicEdges(const UltrasonicEdge* edges,
                                             size_t count, bool overflow) {
  if (overflow || count > UltrasonicMaxEdges) return {};
  uint32_t rise = 0, previous = 0;
  bool seenRise = false;
  for (size_t i = 0; i < count; ++i) {
    const auto& edge = edges[i];
    if (edge.elapsedUs > UltrasonicTimeoutUs || (i && edge.elapsedUs < previous)) return {};
    previous = edge.elapsedUs;
    if (!seenRise && edge.high) { rise = edge.elapsedUs; seenRise = true; }
    else if (seenRise && !edge.high) {
      if (edge.elapsedUs == rise) return {};
      UltrasonicPulse pulse; pulse.complete = true; pulse.widthUs = edge.elapsedUs - rise;
      return pulse;
    }
  }
  return {};
}
} // namespace robo
