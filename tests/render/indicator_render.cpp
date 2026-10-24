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
#include "../../firmware/indicator/src/settings_ui.cpp"
#include <Arduino_G.cpp>
#include <Arduino_GFX.cpp>
#include <canvas/Arduino_Canvas.cpp>

// Sample-only settings backend. No production Wi-Fi, OS, or NVS code is linked.
// The only network name used by capture fixtures is the literal "example".
namespace host_fixture {
device_settings::Language language = device_settings::Language::Chinese;
device_settings::WifiSnapshot wifi;
uint8_t savedBrightness = 80, appliedBrightness = 100;
unsigned scans = 0;
void resetWifi() {
  wifi = device_settings::WifiSnapshot();
  wifi.ready = wifi.persistenceAvailable = true;
}
}

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
void setBrightness(uint8_t value) { host_fixture::appliedBrightness = constrain(value, 5, 100); }
uint8_t brightness() { return host_fixture::appliedBrightness; }
bool touchReady() { return true; }
TouchStatus readTouchStatus(int16_t &, int16_t &) { return TouchStatus::Up; }
bool readTouch(int16_t &, int16_t &) { return false; }
}
namespace device_settings {
bool begin() { host_fixture::resetWifi(); return true; }
Language getLanguage() { return host_fixture::language; }
bool setLanguage(Language value) { host_fixture::language = value; return true; }
uint8_t getBrightness() { return host_fixture::savedBrightness; }
bool saveBrightness(uint8_t value) { host_fixture::savedBrightness = constrain(value, 5, 100); return true; }
WifiSnapshot wifiState() { return host_fixture::wifi; }
bool requestScan() { ++host_fixture::scans; host_fixture::wifi.scanning = true; return true; }
bool requestConnect(const char *, const char *) {
  throw std::runtime_error("Capture must not request a connection, even through the mock");
}
bool restoreDefaultWifi() { host_fixture::resetWifi(); return true; }
void service() { throw std::runtime_error("The native renderer must not run a network worker"); }
uint32_t networkRevision() { return 0; }
bool planterNetworkReady() { return false; }
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
struct Capture {
  std::string name, kind, page;
  unsigned theme, time;
  std::string language = "zh", fixture = "none";
};
std::vector<Capture> captures;
void writeFrame(const std::filesystem::path &output, Capture capture) {
  capture.language = device_settings::getLanguage() == device_settings::Language::English ? "en" : "zh";
  if (host_fixture::wifi.networkCount) capture.fixture = "sample_network_example";
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
  settings_ui::close();
  host_fixture::resetWifi();
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

  // Exercise the production main dispatcher, not a copy of settings routing.
  available.commandBlocked = available.uncertain = false;
  resetPage(0, 1); controlsOpen = confirmOpen = true; touch = ui::TouchGate();
  confirmAction = planter::Action::Water;
  tapRect(kSettings, available, 1100);
  require(settings_ui::isOpen() && !controlsOpen && !confirmOpen && submitted.empty(),
          "Opening settings must close both action layers without submitting");
  // A stale underlying modal must still be unreachable behind settings.
  controlsOpen = confirmOpen = true;
  tapRect(kConfirmGo, available, 1200);
  require(settings_ui::isOpen() && submitted.empty(), "Settings must swallow a tap over the hidden confirmation button");
  settings_ui::open(); confirmOpen = false;
  tapRect(kHardware[1], available, 1300);
  require(submitted.empty(), "Settings must swallow a tap over the hidden direct STOP button");
  const auto oldPage = model.page();
  handle(touch.update(ui::TouchInput::Down, 400, 250, 1400), available, 1400);
  handle(touch.update(ui::TouchInput::Down, 100, 250, 1450), available, 1450);
  handle(touch.update(ui::TouchInput::Up, 0, 0, 1480), available, 1480);
  require(settings_ui::isOpen() && model.page() == oldPage && submitted.empty(),
          "A settings swipe cannot navigate the hidden page or submit an action");
  settings_ui::open();
  tapRect({248, 117, 208, 48}, available, 1500);
  require(device_settings::getLanguage() == device_settings::Language::English && submitted.empty(),
          "Changing language must work through main without reaching hardware actions");
  handle(touch.update(ui::TouchInput::Down, 100, 140, 1600), available, 1600);
  handle(touch.update(ui::TouchInput::Invalid, 0, 0, 1620), available, 1620);
  handle(touch.update(ui::TouchInput::Up, 0, 0, 1640), available, 1640);
  require(device_settings::getLanguage() == device_settings::Language::English && submitted.empty(),
          "Invalid touch cannot become a language tap or underlying action");
  resetPage(0, 0); touch = ui::TouchGate(); device_settings::setLanguage(device_settings::Language::Chinese);
  indicator::setBrightness(80); device_settings::saveBrightness(80);
  interactionChecks = false;
  std::cout << "PASS: " << checked << " real handle/TouchGate interaction assertions.\n";
}

void captureScreens(const std::filesystem::path &output, uint8_t themeId, const std::string &prefix) {
  const char *pages[] = {"face", "today", "info", "photos", "focus"};
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
}

void captureSettings(const std::filesystem::path &output, const std::string &language) {
  const std::string prefix = language + "-settings-";
  resetPage(0, 0); touch = ui::TouchGate();
  settings_ui::open();
  writeFrame(output, {prefix + "menu", "screen", "settings-menu", 0, 1000});
  tapRect({16, 181, 448, 76}, hostState, 1100);
  // Complete the simulated scan with zero results. No real adapter was queried.
  host_fixture::wifi.scanning = false;
  writeFrame(output, {prefix + "wifi-empty", "screen", "settings-wifi-empty", 0, 1200});
  settings_ui::open();
  tapRect({16, 394, 448, 54}, hostState, 1300);
  writeFrame(output, {prefix + "about", "screen", "settings-about", 0, 1400});
  settings_ui::open();
  host_fixture::wifi.networkCount = 1;
  std::strcpy(host_fixture::wifi.networks[0].ssid, "example");
  host_fixture::wifi.networks[0].secure = true;
  host_fixture::wifi.networks[0].rssi = -60;
  tapRect({16, 181, 448, 76}, hostState, 1500);
  writeFrame(output, {prefix + "wifi-sample", "screen", "settings-wifi-sample", 0, 1600});
  tapRect({16, 160, 448, 48}, hostState, 1700);
  writeFrame(output, {prefix + "keyboard", "screen", "settings-keyboard", 0, 1800});
  tapRect({134, 422, 102, 48}, hostState, 1900);
  writeFrame(output, {prefix + "keyboard-upper", "screen", "settings-keyboard-upper", 0, 2000});
  tapRect({244, 422, 102, 48}, hostState, 2100);
  writeFrame(output, {prefix + "keyboard-symbols", "screen", "settings-keyboard-symbols", 0, 2200});
  tapRect({244, 422, 102, 48}, hostState, 2300);
  writeFrame(output, {prefix + "keyboard-symbols-2", "screen", "settings-keyboard-symbols-2", 0, 2400});
  settings_ui::close();
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
    for (uint8_t themeId = 0; themeId < ui_theme::kCount; ++themeId) {
      const std::string prefix = "theme-" + std::to_string(themeId) + "-";
      captureScreens(output, themeId, prefix);
      resetPage(themeId, 0);
      for (unsigned frame = 0; frame < 53; ++frame) {
        char number[8]; std::snprintf(number, sizeof(number), "%03u", frame);
        writeFrame(output, {prefix + "animation-" + number, "animation", "face", themeId, frame * 100});
      }
    }
    captureSettings(output, "zh");
    device_settings::setLanguage(device_settings::Language::English);
    captureScreens(output, 0, "en-theme-0-");
    captureSettings(output, "en");
    device_settings::setLanguage(device_settings::Language::Chinese);
    resetPage(0, 4); model.timer().start(1000);
    writeFrame(output, {"timer-running", "screen", "focus", 0, 61100});
    model.timer().pause(61100);
    writeFrame(output, {"timer-paused", "screen", "focus", 0, 62000});
    model.timer().reset(); model.timer().start(0); model.timer().tick(1500000);
    writeFrame(output, {"timer-completed", "screen", "focus", 0, 1500000});
    std::ofstream manifest(output / "captures.json");
    manifest << "{\"width\":480,\"height\":480,\"state\":\"disconnected_unknown\",\"interaction_assertions\":"
             << checked << ",\"settings_backend\":\"host_sample_fixture_no_network\",\"frame_duration_ms\":100,\"captures\":[\n";
    for (size_t i = 0; i < captures.size(); ++i) {
      const auto &c = captures[i];
      if (i) manifest << ",\n";
      manifest << "{\"name\":\"" << c.name << "\",\"kind\":\"" << c.kind
               << "\",\"page\":\"" << c.page << "\",\"theme\":" << c.theme
               << ",\"time_ms\":" << c.time << ",\"language\":\"" << c.language
               << "\",\"fixture\":\"" << c.fixture << "\"}";
    }
    manifest << "\n]}\n";
    std::cout << "Rendered " << captures.size() << " actual firmware frames at 480 x 480; no device I/O.\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
