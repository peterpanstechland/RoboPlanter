// Compile the production UI translation unit without editing it or its artwork.
// Hardware/network definitions below are disconnected host-only boundaries.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../../firmware/indicator/src/main.cpp"
// A unity translation unit also matches the library's common _data16 variable
// declaration without changing its source or relaxing linker diagnostics.
#include "../../firmware/indicator/src/ui_theme.cpp"
#include "../../firmware/indicator/src/ui_text.cpp"
#include "../../firmware/indicator/src/ui_photos.cpp"
#include <Arduino_G.cpp>
#include <Arduino_GFX.cpp>
#include <canvas/Arduino_Canvas.cpp>

namespace {
Arduino_Canvas hostCanvas(480, 480, nullptr);
planter::State hostState; // Deliberately unknown: no sensor or actuator fixtures.
unsigned presentCount = 0;
bool interactionChecks = false;
std::vector<planter::Action> submitted;
unsigned checked = 0;
}
namespace indicator {
bool begin() { return hostCanvas.begin(GFX_SKIP_OUTPUT_BEGIN); }
Arduino_GFX &display() { return hostCanvas; }
void present() { ++presentCount; }
bool touchReady() { return true; }
TouchStatus readTouchStatus(int16_t &, int16_t &) { return TouchStatus::Up; }
bool readTouch(int16_t &, int16_t &) { return false; }
}
namespace planter {
bool begin() { return true; }
State snapshot() { return hostState; }
bool fresh(const State &state, uint32_t now) {
  // Same boundary contract as planter_client.cpp; no transport is linked.
  return state.received && state.wifi && state.ageKnown &&
      static_cast<uint64_t>(now - state.receivedMs) + state.sampleAgeMs <= kStaleMs;
}
bool submit(Action action, bool) {
  if (!interactionChecks) throw std::runtime_error("Rendering must never submit a hardware action");
  submitted.push_back(action);
  return true;
}
void acknowledgeUncertain() { throw std::runtime_error("Rendering must not modify command state"); }
}
namespace {
struct Capture { std::string name, kind, page; unsigned theme, time; };
std::vector<Capture> captures;
void writeFrame(const std::filesystem::path &output, const Capture &capture) {
  renderNow = capture.time;
  draw(hostState, capture.time);
  const uint16_t *pixels = hostCanvas.getFramebuffer();
  std::ofstream image(output / (capture.name + ".ppm"), std::ios::binary);
  if (!image) throw std::runtime_error("Unable to open image output");
  image << "P6\n480 480\n255\n";
  for (size_t i = 0; i < 480U * 480; ++i) {
    const uint16_t c = pixels[i];
    const uint8_t r = static_cast<uint8_t>((c >> 11) & 31);
    const uint8_t g = static_cast<uint8_t>((c >> 5) & 63);
    const uint8_t b = static_cast<uint8_t>(c & 31);
    const char rgb[3] = {static_cast<char>((r << 3) | (r >> 2)),
                         static_cast<char>((g << 2) | (g >> 4)),
                         static_cast<char>((b << 3) | (b >> 2))};
    image.write(rgb, 3);
  }
  if (!image) throw std::runtime_error("Image output failed");
  captures.push_back(capture);
}
void resetPage(uint8_t themeId, uint8_t page) {
  model = ui::Model();
  model.selectTheme(themeId);
  controlsOpen = false;
  confirmOpen = false;
  for (uint8_t i = 0; i < page; ++i) model.nextPage();
}
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
  ++checked;
}
void tapRect(const Rect &rect, const State &state, uint32_t now) {
  const int16_t x = rect.x + rect.w / 2, y = rect.y + rect.h / 2;
  handle(touch.update(ui::TouchInput::Down, x, y, now), state, now);
  handle(touch.update(ui::TouchInput::Up, 0, 0, now + 20), state, now + 20);
}
void interactionTests() {
  interactionChecks = true;
  State available;
  // Test-only connectivity/capability flags, never sensor readings or screenshots.
  available.received = available.wifi = available.ageKnown = true;
  available.receivedMs = 1000;
  available.pumpEnabled = available.printerEnabled = true;
  for (unsigned button : {0U, 3U}) {
    resetPage(0, 1); controlsOpen = true; touch = ui::TouchGate(); submitted.clear();
    tapRect(kHardware[button], available, 1100);
    require(confirmOpen && submitted.empty(), "First water/print tap must only open confirmation");
    tapRect(kConfirmGo, available, 1200);
    require(!confirmOpen && submitted.size() == 1 && submitted[0] == kActions[button],
            "Second tap must submit the selected command exactly once");
    tapRect(kConfirmGo, available, 1300);
    require(submitted.size() == 1, "A further tap outside an action cannot resubmit");
  }
  resetPage(0, 1); controlsOpen = true; touch = ui::TouchGate(); submitted.clear();
  tapRect(kHardware[0], available, 1100);
  handle(touch.update(ui::TouchInput::Down, 350, 220, 1200), available, 1200);
  handle(touch.update(ui::TouchInput::Down, 120, 220, 1250), available, 1250);
  handle(touch.update(ui::TouchInput::Up, 0, 0, 1280), available, 1280);
  require(!confirmOpen && controlsOpen && submitted.empty(), "Swipe must cancel confirmation without sending");
  tapRect(kHardware[0], available, 1400);
  tapRect(kConfirmGo, available, 7000);
  require(confirmOpen && submitted.empty(), "State expiring on confirmation must block the send");
  tapRect(kConfirmCancel, available, 7100);
  require(!confirmOpen && submitted.empty(), "Cancel must never send");
  available.commandBlocked = available.uncertain = true;
  tapRect(kHardware[1], available, 7200);
  require(submitted.size() == 1 && submitted[0] == planter::Action::StopWater && !confirmOpen,
          "STOP must remain direct through stale/unknown-command lock when connected and enabled");
  submitted.clear(); resetPage(0, 1); controlsOpen = true;
  tapRect(kHardware[0], State(), 7300);
  tapRect(kHardware[3], State(), 7400);
  require(!confirmOpen && submitted.empty(), "Unknown disconnected capabilities must block water/print");
  interactionChecks = false;
  std::cout << "PASS: " << checked << " real handle/TouchGate interaction assertions.\n";
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Usage: indicator_render OUTPUT_DIRECTORY");
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    setup();
    if (!displayReady) throw std::runtime_error("Host canvas allocation failed");
    interactionTests();
    const char *pages[] = {"face", "today", "info", "photos", "focus"};
    for (uint8_t themeId = 0; themeId < ui_theme::kCount; ++themeId) {
      const std::string prefix = "theme-" + std::to_string(themeId) + "-";
      for (uint8_t page = 0; page < 5; ++page) {
        resetPage(themeId, page);
        writeFrame(output, {prefix + pages[page], "screen", pages[page], themeId, 1000});
      }
      resetPage(themeId, 0); model.openThemes();
      writeFrame(output, {prefix + "themes", "screen", "themes", themeId, 1000});
      resetPage(themeId, 1); controlsOpen = true;
      writeFrame(output, {prefix + "controls", "screen", "controls", themeId, 1000});
      confirmOpen = true; confirmAction = planter::Action::Water;
      writeFrame(output, {prefix + "confirm-water", "screen", "confirm-water", themeId, 1000});
      confirmAction = planter::Action::Print;
      writeFrame(output, {prefix + "confirm-print", "screen", "confirm-print", themeId, 1000});
      resetPage(themeId, 0);
      for (unsigned frame = 0; frame < 53; ++frame) {
        char number[8]; std::snprintf(number, sizeof(number), "%03u", frame);
        writeFrame(output, {prefix + "animation-" + number, "animation", "face", themeId, frame * 100});
      }
    }
    resetPage(0, 4); model.timer().start(1000);
    writeFrame(output, {"timer-running", "screen", "focus", 0, 61100});
    model.timer().pause(61100);
    writeFrame(output, {"timer-paused", "screen", "focus", 0, 62000});
    model.timer().reset(); model.timer().start(0); model.timer().tick(1500000);
    writeFrame(output, {"timer-completed", "screen", "focus", 0, 1500000});
    std::ofstream manifest(output / "captures.json");
    manifest << "{\"width\":480,\"height\":480,\"state\":\"disconnected_unknown\",\"interaction_assertions\":"
             << checked << ",\"frame_duration_ms\":100,\"captures\":[\n";
    for (size_t i = 0; i < captures.size(); ++i) {
      const auto &c = captures[i];
      if (i) manifest << ",\n";
      manifest << "{\"name\":\"" << c.name << "\",\"kind\":\"" << c.kind
               << "\",\"page\":\"" << c.page << "\",\"theme\":" << c.theme
               << ",\"time_ms\":" << c.time << "}";
    }
    manifest << "\n]}\n";
    std::cout << "Rendered " << captures.size() << " actual firmware frames at 480 x 480; no device I/O.\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
