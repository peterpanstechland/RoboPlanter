#include "settings_ui.h"
#include "device_settings.h"
#include "planter_presentation.h"
#include "connection_presentation.h"
#include "indicator_board.h"
#include "ui_text.h"
#include "ui_theme.h"
#include <cstdio>
#include <cstring>

namespace settings_ui {
namespace {
using device_settings::Language;
using device_settings::WifiError;
using device_settings::WifiPhase;
enum class Page : uint8_t { Menu, Wifi, Password, About, Planter, Yoyo, Calibration, Soil, Tank, ResetCalibration, AutoLight, RepairYoyo };
struct Rect { int16_t x, y, w, h; };
constexpr Rect kChinese{24, 117, 208, 48}, kEnglish{248, 117, 208, 48};
constexpr Rect kWifi{16, 181, 216, 76}, kLightSettings{248,181,216,76}, kPlanter{16, 394, 216, 54}, kAbout{248, 394, 216, 54};
constexpr Rect kCompanion{24,332,208,60}, kCalibration{248,332,208,60}, kPairPhone{24,174,432,54}, kAutoPrint{24,316,432,54};
constexpr Rect kRepairConfirm{24,390,208,64},kRepairCancel{248,390,208,64};
constexpr Rect kRepairLink{24,386,208,48},kYoyoCheck{248,386,208,48};
constexpr Rect kPairedNetwork{24, 411, 208, 52}, kNetworkSettings{248, 411, 208, 52};
constexpr Rect kMinus{24, 321, 56, 56}, kSlider{96, 321, 288, 56}, kPlus{400, 321, 56, 56};
constexpr Rect kRefresh{16, 422, 100, 48}, kPrevious{124, 422, 64, 48};
constexpr Rect kNext{196, 422, 64, 48}, kDefault{268, 422, 196, 48};
constexpr Rect kModes[] = {{24, 422, 102, 48}, {134, 422, 102, 48},
                           {244, 422, 102, 48}, {354, 422, 102, 48}};
bool opened = false, scanWasRunning = false, listLoaded = false;
bool settingsRejected = false, secureNetwork = true;
Page page = Page::Menu;
uint8_t networkPage = 0, keyboardMode = 0, listCount = 0;
device_settings::Network list[device_settings::kMaxNetworks];
char selectedSsid[33] = {}, password[65] = {};
size_t passwordLength = 0;
WifiError joinError = WifiError::None;

const char *tr(const char *zh, const char *en) {
  return device_settings::getLanguage() == Language::English ? en : zh;
}
void wipePassword() {
  volatile char *p = password;
  for (size_t i = 0; i < sizeof(password); ++i) p[i] = 0;
  passwordLength = 0;
  joinError = WifiError::None;
}
bool inside(const Rect &r, int32_t x, int32_t y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
bool tapped(const Rect &r, const ui::TouchGate &touch) {
  return inside(r, touch.startX(), touch.startY()) && inside(r, touch.lastX(), touch.lastY());
}
Rect networkRect(unsigned row) { return {16, static_cast<int16_t>(160 + row * 50), 448, 48}; }
Rect keyRect(unsigned key) {
  return {static_cast<int16_t>(24 + key % 8 * 54), static_cast<int16_t>(226 + key / 8 * 48), 54, 48};
}
// Keep connection SSID bytes untouched. Only its bounded display copy is
// sanitized/truncated, and never split a valid UTF-8 code point.
void fitted(char *out, size_t capacity, const char *text, int16_t maxWidth, uint8_t size) {
  size_t used = 0;
  const int16_t dots = ui_text::widthSized("...", size);
  const bool truncate = ui_text::widthSized(text, size) > maxWidth;
  int16_t advance = 0;
  while (*text && used + 5 < capacity) {
    const uint8_t lead = static_cast<uint8_t>(*text);
    size_t count = lead < 0x80 ? 1 : lead >= 0xC2 && lead <= 0xDF ? 2 :
                   lead >= 0xE0 && lead <= 0xEF ? 3 : lead >= 0xF0 && lead <= 0xF4 ? 4 : 0;
    bool valid = count != 0 && lead >= 32 && lead != 127;
    for (size_t i = 1; valid && i < count; ++i)
      valid = text[i] && (static_cast<uint8_t>(text[i]) & 0xC0) == 0x80;
    char unit[5] = {};
    if (valid) memcpy(unit, text, count);
    else { unit[0] = '?'; count = 1; }
    const int16_t width = ui_text::widthSized(unit, size);
    if (advance + width > maxWidth - (truncate ? dots : 0)) break;
    const size_t bytes = strlen(unit);
    memcpy(out + used, unit, bytes); used += bytes; advance += width; text += count;
  }
  if (*text && used + 3 < capacity) { memcpy(out + used, "...", 3); used += 3; }
  out[used] = 0;
}
void text(int16_t x, int16_t y, const char *value, uint16_t color,
          uint8_t size = 24, int16_t maxWidth = 432) {
  char visible[192]; fitted(visible, sizeof(visible), value, maxWidth, size);
  ui_text::drawSized(indicator::display(), x, y, visible, color, size);
}
void button(const Rect &r, const char *label, const ui_theme::Theme &t,
            bool selected = false, bool enabled = true, uint8_t size = 24) {
  auto &g = indicator::display();
  const uint16_t fill = selected ? t.accent : t.surface;
  const uint16_t ink = selected ? t.cardInk : enabled ? t.ink : t.muted;
  g.fillRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 9, fill);
  g.drawRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 9, selected ? t.accent : t.outline);
  char visible[96]; fitted(visible, sizeof(visible), label, r.w - 8, size);
  const int16_t width = ui_text::widthSized(visible, size);
  ui_text::drawSized(g, r.x + (r.w - width) / 2, r.y + (r.h - size) / 2 - 1, visible, ink, size);
}
const char *errorText(WifiError error) {
  switch (error) {
    case WifiError::InvalidInput: return tr("输入格式不正确", "Check network details");
    case WifiError::Busy: return tr("正在处理，请稍候", "Busy. Please wait");
    case WifiError::AuthFailed: return tr("密码错误或认证失败", "Authentication failed");
    case WifiError::NetworkNotFound: return tr("未找到网络", "Network not found");
    case WifiError::Timeout: return tr("连接超时，稍后自动重连", "Timed out; auto-retry soon");
    case WifiError::Disconnected: return tr("连接已断开", "Disconnected");
    case WifiError::ScanFailed: return tr("扫描失败，请刷新", "Scan failed. Refresh");
    case WifiError::StorageUnavailable: return tr("设置未能保存", "Settings could not be saved");
    case WifiError::ConnectionFailed: return tr("连接失败，稍后自动重连", "Failed; auto-retry soon");
    default: return "";
  }
}
const char *phaseText(const device_settings::WifiSnapshot &state) {
  if (!state.ready) return tr("网络服务尚未就绪", "Network service unavailable");
  if (state.scanning) return tr("正在扫描附近网络", "Scanning nearby networks");
  switch (state.phase) {
    case WifiPhase::Connecting: return tr("正在连接", "Connecting");
    case WifiPhase::Connected: return tr("已连接 Wi-Fi", "Wi-Fi connected");
    case WifiPhase::Failed: return tr("未能连接", "Not connected");
    default: return tr("尚未连接", "Not connected");
  }
}
void syncList(const device_settings::WifiSnapshot &state) {
  if (state.scanning) { scanWasRunning = true; return; }
  if (!listLoaded || scanWasRunning) {
    listCount = state.networkCount < device_settings::kMaxNetworks ? state.networkCount : device_settings::kMaxNetworks;
    for (size_t i = 0; i < listCount; ++i) list[i] = state.networks[i];
    networkPage = 0; listLoaded = true; scanWasRunning = false;
  }
}
void beginScan() {
  if (device_settings::requestScan()) { scanWasRunning = true; listCount = 0; networkPage = 0; }
}
void showWifi() {
  wipePassword(); page = Page::Wifi; listLoaded = false; networkPage = 0;
  const auto state = device_settings::wifiState();
  syncList(state);
  if (state.ready && state.phase != WifiPhase::Connecting && !state.scanning && !state.networkCount) beginScan();
}
bool validPassword() {
  if (!secureNetwork) return passwordLength == 0;
  if (passwordLength >= 8 && passwordLength <= 63) return true;
  if (passwordLength != 64) return false;
  for (size_t i = 0; i < passwordLength; ++i) {
    const char c = password[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}
char keyCharacter(unsigned index) {
  const char *characters = keyboardMode == 0 ? "abcdefghijklmnopqrstuvwxyz" :
                           keyboardMode == 1 ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" :
                           keyboardMode == 2 ? "0123456789!@#$%^&*()-_=+[]{};:" : "'\",.<>/?\\|`~";
  if (index == 30) return ' ';
  return index < strlen(characters) ? characters[index] : 0;
}
void drawMenu(const ui_theme::Theme &t) {
  auto &g = indicator::display();
  text(24, 82, tr("界面语言", "Language"), t.ink);
  button(kChinese, "中文", t, device_settings::getLanguage() == Language::Chinese);
  button(kEnglish, "English", t, device_settings::getLanguage() == Language::English);
  g.fillRoundRect(kWifi.x, kWifi.y, kWifi.w, kWifi.h, 12, t.surface);
  ui_theme::icon(g, ui_theme::Icon::Wifi, 31, 198, 32, t.accent);
  text(78, 193, "Wi-Fi", t.ink);
  const auto state = device_settings::wifiState();
  text(30, 226, phaseText(state), t.muted, 16, 188);
  const auto planterState=planter::snapshot();
  g.fillRoundRect(kLightSettings.x,kLightSettings.y,kLightSettings.w,kLightSettings.h,12,t.surface);
  text(264,193,tr("灯光设置","Light settings"),t.ink,24,184);
  text(264,226,planterState.autoLightKnown ? (planterState.autoLight.enabled ? (planterState.autoLightPaused ? tr("自动已暂停","Auto paused") : tr("自动模式开启","Auto enabled")) : tr("手动模式","Manual mode")) : tr("等待读取","Waiting for data"),t.muted,16,184);
  g.fillRoundRect(16, 273, 448, 110, 12, t.surface);
  text(32, 284, tr("屏幕亮度", "Brightness"), t.ink);
  char value[8]; snprintf(value, sizeof(value), "%u%%", indicator::brightness());
  text(360, 284, value, t.accent, 24, 88);
  button(kMinus, "-", t); button(kPlus, "+", t);
  const int16_t knob = static_cast<int16_t>(96 + (indicator::brightness() - 5) * 287 / 95);
  g.fillRoundRect(96, 344, 288, 10, 5, t.outline);
  g.fillRoundRect(96, 344, knob - 96 + 1, 10, 5, t.accent);
  g.fillCircle(knob, 349, 10, t.accent);
  button(kPlanter, tr("花盆连接", "Planter connection"), t, false, true, 16);
  button(kAbout, tr("关于设备", "About device"), t, false, true, 16);
  if (settingsRejected || state.error == WifiError::StorageUnavailable)
    text(24, 455, tr("设置未能保存", "Settings could not be saved"), t.muted, 16);
}
void drawWifi(const ui_theme::Theme &t) {
  const auto state = device_settings::wifiState();
  syncList(state);
  text(24, 82, phaseText(state), t.ink, 24);
  const char *ssid = state.phase == WifiPhase::Connected ? state.connectedSsid : state.targetSsid;
  text(24, 116, ssid[0] ? ssid : tr("仅支持 2.4 GHz 网络", "2.4 GHz networks only"), t.muted, 16);
  if (state.error != WifiError::None) text(24, 138, errorText(state.error), t.accent, 16);
  else text(24, 138, state.saved ? tr("已保存，上电自动连接", "Saved. Reconnects on startup") :
            tr("连接成功后自动保存", "Saves after a successful connection"), t.muted, 16);
  for (unsigned row = 0; row < 5; ++row) {
    const unsigned index = networkPage * 5 + row;
    if (index >= listCount || state.scanning) break;
    const auto r = networkRect(row); const auto &network = list[index];
    auto &g = indicator::display();
    g.fillRoundRect(r.x, r.y, r.w, r.h, 9, t.surface);
    text(30, r.y + 9, network.ssid, t.ink, 24, 320);
    text(366, r.y + 15, network.secure ? tr("加密", "Secured") : tr("开放", "Open"), t.muted, 16, 82);
  }
  if (!state.scanning && !listCount)
    text(32, 232, tr("暂无网络，点击刷新", "No networks. Tap Refresh"), t.muted, 24, 416);
  else if (state.scanning)
    text(32, 232, tr("扫描中，请稍候", "Scanning. Please wait"), t.muted, 24, 416);
  button(kRefresh, tr("刷新", "Refresh"), t, false, state.ready && !state.scanning, 16);
  button(kPrevious, "<", t, false, networkPage > 0 && !state.scanning);
  button(kNext, ">", t, false, listCount > 5 && networkPage == 0 && !state.scanning);
  button(kDefault, tr("连接花盆网络", "Planter network"), t, false, state.ready, 16);
}
void drawPassword(const ui_theme::Theme &t) {
  text(24, 82, selectedSsid, t.ink, 24);
  text(24, 116, tr("网络密码", "Network password"), t.muted, 16);
  auto &g = indicator::display();
  g.fillRoundRect(24, 143, 432, 52, 10, t.surface);
  if (!secureNetwork) text(40, 157, tr("开放网络，无需密码", "Open network: no password"), t.muted, 16, 344);
  else if (!passwordLength) text(40, 157, tr("点击下方键盘输入", "Enter using the keys below"), t.muted, 16, 344);
  else {
    char stars[25]; const size_t count = passwordLength < 24 ? passwordLength : 24;
    memset(stars, '*', count); stars[count] = 0;
    text(40, 153, stars, t.ink, 24, 340);
  }
  char count[8]; snprintf(count, sizeof(count), "%u", static_cast<unsigned>(passwordLength));
  if (secureNetwork) text(408, 159, count, t.muted, 16, 36);
  text(24, 204, joinError != WifiError::None ? errorText(joinError) : secureNetwork ? tr("8–63 位 ASCII，或 64 位十六进制", "8-63 ASCII characters, or 64 hex digits") :
       tr("连接后仍需单独确认花盆状态", "Check the planter link after connecting"), t.muted, 16);
  for (unsigned i = 0; i < 32; ++i) {
    const auto r = keyRect(i);
    if (i == 31) button(r, tr("删", "Del"), t, false, secureNetwork, 16);
    else if (i == 30) button(r, tr("空格", "Space"), t, false, secureNetwork, 16);
    else {
      const char c = keyCharacter(i);
      if (c) { const char key[2] = {c, 0}; button(r, key, t, false, secureNetwork); }
    }
  }
  button(kModes[0], "abc", t, keyboardMode == 0, secureNetwork);
  button(kModes[1], "ABC", t, keyboardMode == 1, secureNetwork);
  button(kModes[2], keyboardMode == 3 ? "#+= 2" : "#+= 1", t, keyboardMode >= 2, secureNetwork, 16);
  button(kModes[3], tr("连接", "Join"), t, validPassword(), validPassword());
}
void drawAbout(const ui_theme::Theme &t) {
  auto &g = indicator::display();
  g.fillRoundRect(16, 82, 448, 362, 14, t.surface);
  text(32, 102, "SenseCAP Indicator D1", t.ink);
  text(32, 150, tr("主控与协处理器", "Main and companion processors"), t.muted, 16, 416);
  text(32, 180, "ESP32-S3 + RP2040", t.ink, 24, 416);
  text(32, 229, tr("无内置 CO2 / VOC 传感器", "No built-in CO2 / VOC sensors"), t.ink, 24, 416);
  text(32, 269, tr("温湿度需要外接传感器", "Temperature / humidity need an external sensor"), t.muted, 16, 416);
  text(32, 312, tr("内置无源蜂鸣器，支持提示音", "Built-in passive buzzer for alert tones"), t.ink, 24, 416);
  text(32, 355, tr("其他 Wi-Fi 不等于已连接花盆", "Joining Wi-Fi does not connect the planter"), t.muted, 16, 416);
  text(32, 389, tr("校准数据另存于花盆主控", "Sensor calibration is saved on the planter"), t.muted, 16, 416);
}
void drawPlanter(const ui_theme::Theme &t, uint32_t now) {
  const auto wifi = device_settings::wifiState();
  const auto endpoint = device_settings::planterEndpoint();
  const auto state = planter::snapshot();
  const bool english = device_settings::getLanguage() == Language::English;
  const auto status = endpoint.configured ? planter_presentation::link(state, wifi, now) : planter_presentation::Link::Configuration;
  auto &g=indicator::display();
  const bool live=planter::fresh(state,now);
  const bool wifiOk=wifi.phase==WifiPhase::Connected && wifi.planterNetwork;
  const auto phone=connection_ui::phonePhase(state,now);
  char detail[112];
  auto row=[&](int y,const char* label,const char* value,const char* note,bool good) {
    g.fillRoundRect(24,y,432,76,9,t.surface);
    g.fillCircle(40,y+17,4,good?t.accent:t.muted);
    text(52,y+7,label,t.muted,16,392);
    text(36,y+28,value,t.ink,24,408);
    text(36,y+55,note,t.muted,16,408);
  };
  const char* network=wifi.connectedSsid[0]?wifi.connectedSsid:wifi.targetSsid;
  if(wifi.phase==WifiPhase::Connecting)
    snprintf(detail,sizeof(detail),tr("第 %u 次尝试 · %lu 秒 / 20 秒","Attempt %u · %lu / 20 seconds"),wifi.attempt,static_cast<unsigned long>((now-wifi.attemptStartedMs)/1000));
  else if(wifi.retryAtMs && int32_t(wifi.retryAtMs-now)>0)
    snprintf(detail,sizeof(detail),tr("%s · %lu 秒后自动重试","%s · retry in %lus"),errorText(wifi.error),static_cast<unsigned long>((wifi.retryAtMs-now+999)/1000));
  else snprintf(detail,sizeof(detail),"%s",wifiOk?(wifi.saved?tr("网络已保存，断电后自动连接","Saved network; reconnects after power-on"):tr("已联网，但网络设置未保存","Connected; Wi-Fi settings not saved")):
    wifi.phase==WifiPhase::Connected?tr("请在网络设置连接花盆网络","Select the planter network in Wi-Fi settings"):errorText(wifi.error));
  row(82,"01  Indicator > Wi-Fi",network[0]?network:phaseText(wifi),detail,wifiOk);
  if(live)snprintf(detail,sizeof(detail),tr("%lu 秒前更新 · HTTP %d","Updated %lus ago · HTTP %d"),static_cast<unsigned long>((now-state.receivedMs)/1000),state.httpStatus);
  else if(status==planter_presentation::Link::Authentication)snprintf(detail,sizeof(detail),"%s",tr("屏幕与主控密钥不一致，重配手机无效","Screen / XIAO token mismatch; check config"));
  else if(status==planter_presentation::Link::WrongNetwork)snprintf(detail,sizeof(detail),"%s: %s",tr("目标网络","Target Wi-Fi"),endpoint.ssid);
  else snprintf(detail,sizeof(detail),"%s%s",state.checking?tr("检查中 · ","Checking · "):"",endpoint.address[0]?endpoint.address:"--");
  row(164,"02  Indicator > XIAO / HTTP",planter_presentation::linkText(status,english),detail,live);
  row(246,"03  Robot Phone > XIAO / BLE",connection_ui::title(phone,english),
    live&&state.yoyoPaired?tr("无需常连；说“读取工位状态”验证","On demand: say Read desk status to verify"):
    tr("手机连主控；屏幕只负责显示配对码","Phone pairs with XIAO; screen shows its code"),live&&state.yoyoPaired);
  button(kCompanion, tr("YOYO 与场景小票", "YOYO / receipts"), t, false, true, 16);
  button(kCalibration, tr("传感器校准", "Sensor calibration"), t, false, true, 16);
  button(kPairedNetwork,wifiOk?(state.checking?tr("正在检查","Checking"):tr("立即检查链路","Check link now")):
    wifi.phase==WifiPhase::Connected?tr("连接花盆网络","Use planter Wi-Fi"):tr("重新连接 Wi-Fi","Reconnect Wi-Fi"),t,false,
    wifi.ready && wifi.phase!=WifiPhase::Connecting && !wifi.scanning && !state.checking && !state.commandBlocked,16);
  button(kNetworkSettings, tr("无线网络设置", "Wi-Fi settings"), t, false, true, 16);
}
void drawYoyo(const ui_theme::Theme &t,uint32_t now) {
  const auto state=planter::snapshot();
  const bool live=planter::fresh(state,now),ready=live && !state.commandBlocked;
  const bool english=device_settings::getLanguage()==Language::English;
  const auto phase=connection_ui::phonePhase(state,now);
  text(24,90,connection_ui::title(phase,english),t.ink,24);
  text(24,130,connection_ui::hint(phase,english),t.muted,16);
  button(kPairPhone,state.commandBlocked?tr("正在提交，请稍候","Request pending"):
    state.yoyoRepairing||state.yoyoReconnecting?tr("正在处理连接","Working on connection"):
    state.yoyoPaired?tr("重连手机（保留绑定）","Reconnect; keep bond"):
    connection_ui::secondsLeft(state,now)?tr("配对窗口已开启","Pairing window open"):tr("显示六位配对码","Show pairing code"),t,false,
    ready && state.yoyoEnabled && !state.yoyoRepairing && !state.yoyoReconnecting &&
    (state.yoyoPaired?state.yoyoReconnectSupported:!state.yoyoLinkConnected&&!connection_ui::secondsLeft(state,now)),24);
  if(connection_ui::showPin(state,now)) {
    char pin[24];snprintf(pin,sizeof(pin),"PIN  %06lu",static_cast<unsigned long>(state.yoyoPin));text(80,240,pin,t.accent,48);
    char countdown[64];snprintf(countdown,sizeof(countdown),tr("剩余 %lu 秒 · 仅在手机系统蓝牙输入","%lus left · enter in phone Bluetooth"),static_cast<unsigned long>(connection_ui::secondsLeft(state,now)));
    text(24,292,countdown,t.muted,16);
  } else {
    text(24,244,!live?tr("主控离线，不显示旧码或旧连接状态","XIAO offline; old code is hidden"):
      state.yoyoNotifications?tr("手机已订阅回复通道","Phone subscribed to replies"):
      state.yoyoConnected?tr("蓝牙已加密，等待工具订阅回复","Encrypted link; waiting for tool subscription"):
      state.yoyoPaired?tr("已保存绑定，无需再次输入配对码","Bond saved; no code needed on reconnect"):
      tr("六位码在此显示，两分钟有效","Code appears here for two minutes"),t.muted,16);
    char detail[112];
    if(live && state.yoyoToolSeen)snprintf(detail,sizeof(detail),tr("最近请求 %lu 秒前 · 本次连接 %lu 条","Last request %lus ago · session count %lu"),
      static_cast<unsigned long>((uint64_t(state.yoyoToolAgeMs)+now-state.receivedMs)/1000),static_cast<unsigned long>(state.yoyoToolCount));
    else snprintf(detail,sizeof(detail),"%s",tr("说“你好”不会验证硬件连接","Saying hello does not test hardware"));
    if(live && phase==connection_ui::PhonePhase::Failed)snprintf(detail,sizeof(detail),"%s",connection_ui::failureHint(state,english));
    if(live && !strcmp(state.yoyoStage,"repair_restart_required"))snprintf(detail,sizeof(detail),"%s",tr("修复超时，请重启主控后检查","Repair timed out; restart XIAO, then check"));
    if(live && (!strcmp(state.commandStatus,"failed")||!strcmp(state.commandStatus,"rejected")||state.uncertain))
      snprintf(detail,sizeof(detail),"%s",state.uncertain?tr("请求结果未知，先检查，不自动重发","Outcome unknown; check before retry"):state.commandError);
    text(24,278,detail,t.muted,16);
  }
  button(kAutoPrint,state.autoPrint?tr("场景自动打印：开","Scene auto-print: ON"):tr("场景自动打印：关","Scene auto-print: OFF"),t,state.autoPrint,ready&&state.autoPrintKnown,24);
  button(kRepairLink,tr("重新绑定手机","Rebind phone"),t,false,ready&&state.yoyoPaired&&state.yoyoRepairSupported&&!state.yoyoRepairing&&!state.yoyoReconnecting,16);
  button(kYoyoCheck,tr("检查链路","Check link"),t,false,true,16);
  text(24,447,tr("重连保留设置；重新绑定需确认","Reconnect keeps settings; rebind needs confirmation"),t.muted,16);
}
void drawRepairYoyo(const ui_theme::Theme &t,uint32_t now) {
  const auto state=planter::snapshot();
  text(24,94,tr("重新绑定这部手机","Pair this phone again"),t.ink,24);
  text(24,145,tr("将清除花盆保存的旧手机绑定。","Remove the planter's old phone bond."),t.muted,16);
  text(24,183,tr("请先在手机忽略 RoboPlanter，","Forget RoboPlanter on the phone,"),t.ink,16);
  text(24,214,tr("并暂时关闭手机蓝牙。","then turn phone Bluetooth off."),t.ink,16);
  text(24,258,tr("确认后显示新码，两分钟内完成配对。","Confirm for a new two-minute code."),t.muted,16);
  text(24,290,tr("Wi-Fi、标定和打印机设置都会保留。","Wi-Fi, sensors and printer are kept."),t.muted,16);
  const bool ready=planter::fresh(state,now) && !state.commandBlocked && state.yoyoEnabled && state.yoyoRepairSupported && state.yoyoPaired && !state.yoyoLinkConnected && !state.yoyoRepairing;
  text(24,341,state.yoyoLinkConnected?tr("等待手机断开蓝牙连接","Waiting for phone to disconnect"):
    ready?tr("已就绪，确认后可重新打开手机蓝牙","Ready. Turn Bluetooth on after confirming"):
    tr("正在等待花盆状态","Waiting for planter status"),t.muted,16);
  button(kRepairConfirm,tr("确认重新配对","Confirm repair"),t,true,ready,24);
  button(kRepairCancel,tr("取消","Cancel"),t,false,true,24);
}
#include "calibration_ui.h"
#include "auto_light_ui.h"

void brightness(uint8_t value) {
  indicator::setBrightness(value);
  settingsRejected = !device_settings::saveBrightness(indicator::brightness());
}
}

bool isOpen() { return opened; }
void open() { wipePassword(); opened = true; page = Page::Menu; settingsRejected = false; }
void openConnection() { open(); page=Page::Planter; planter::requestRefresh(); }
void close() { wipePassword(); calibrationCapture.cancel(); selectedSsid[0] = 0; opened = false; page = Page::Menu; }
void back() {
  wipePassword();
  calibrationCapture.cancel();
  if (page == Page::Menu) close();
  else if (page == Page::ResetCalibration) page = calibrationSoil ? Page::Soil : Page::Tank;
  else if (page == Page::Soil || page == Page::Tank) page = Page::Calibration;
  else if (page == Page::Calibration) page = Page::Planter;
  else if(page==Page::Yoyo)page=Page::Planter;
  else if(page==Page::RepairYoyo)page=Page::Yoyo;
  else if (page == Page::Password) { selectedSsid[0] = 0; page = Page::Wifi; }
  else page = Page::Menu;
}
const char *title() {
  switch (page) {
    case Page::Wifi: return tr("无线网络", "Wi-Fi");
    case Page::Password: return tr("连接网络", "Join network");
    case Page::About: return tr("关于设备", "About device");
    case Page::Yoyo: return tr("YOYO 伙伴", "YOYO companion");
    case Page::RepairYoyo: return tr("修复手机配对", "Repair pairing");
    case Page::Planter: return tr("花盆连接", "Planter connection");
    case Page::Calibration: return tr("传感器校准", "Sensor calibration");
    case Page::Soil: return tr("土壤干湿校准", "Soil calibration");
    case Page::Tank: return tr("水箱水位校准", "Tank calibration");
    case Page::ResetCalibration: return tr("清除校准", "Reset calibration");
    case Page::AutoLight: return tr("自动灯光", "Automatic lighting");
    default: return tr("设备设置", "Settings");
  }
}
void draw(uint32_t now, uint8_t themeId) {
  if (!opened) return;
  const auto &t = ui_theme::get(themeId);
  indicator::display().fillRect(0, 76, 480, 400, t.background);
  switch (page) {
    case Page::Menu: drawMenu(t); break;
    case Page::Wifi: drawWifi(t); break;
    case Page::Password: drawPassword(t); break;
    case Page::About: drawAbout(t); break;
    case Page::Planter: drawPlanter(t, now); break;
    case Page::Yoyo: drawYoyo(t,now);break;
    case Page::RepairYoyo:drawRepairYoyo(t,now);break;
    case Page::Calibration: drawCalibrationMenu(t, now); break;
    case Page::Soil: case Page::Tank: drawCalibrationEditor(t, now); break;
    case Page::ResetCalibration: drawCalibrationReset(t); break;
    case Page::AutoLight: drawAutoLight(t,now); break;
  }
}
bool handle(ui::Gesture gesture, const ui::TouchGate &touch, uint32_t now) {
  if (!opened) return false;
  if (gesture != ui::Gesture::Tap) return true;
  if(page==Page::AutoLight) { handleAutoLight(touch,now); return true; }
  if (handleCalibration(touch, now)) return true;
  if (page == Page::Menu) {
    if (tapped(kChinese, touch)) settingsRejected = !device_settings::setLanguage(Language::Chinese);
    else if (tapped(kEnglish, touch)) settingsRejected = !device_settings::setLanguage(Language::English);
    else if (tapped(kWifi, touch)) showWifi();
    else if(tapped(kLightSettings,touch))openAutoLight();
    else if (tapped(kAbout, touch)) page = Page::About;
    else if (tapped(kPlanter, touch)) page = Page::Planter;
    else if (tapped(kMinus, touch)) brightness(indicator::brightness() > 10 ? indicator::brightness() - 5 : 5);
    else if (tapped(kPlus, touch)) brightness(indicator::brightness() < 95 ? indicator::brightness() + 5 : 100);
    else if (tapped(kSlider, touch)) brightness(static_cast<uint8_t>(5 + (touch.lastX() - 96) * 95 / 287));
  } else if(page==Page::RepairYoyo) {
    const auto state=planter::snapshot();
    if(tapped(kRepairCancel,touch))page=Page::Yoyo;
    else if(tapped(kRepairConfirm,touch) && planter::fresh(state,now) && !state.commandBlocked && state.yoyoEnabled && state.yoyoRepairSupported && state.yoyoPaired && !state.yoyoLinkConnected && !state.yoyoRepairing) {
      if(planter::submit(planter::Action::RepairYoyo))page=Page::Yoyo;
    }
  } else if(page==Page::Yoyo) {
    const auto state=planter::snapshot();
    if(tapped(kYoyoCheck,touch)){page=Page::Planter;planter::requestRefresh();return true;}
    if(planter::fresh(state,now) && !state.commandBlocked) {
      if(tapped(kRepairLink,touch) && state.yoyoPaired && state.yoyoRepairSupported && !state.yoyoRepairing && !state.yoyoReconnecting)page=Page::RepairYoyo;
      else if(tapped(kPairPhone,touch) && state.yoyoEnabled && !state.yoyoRepairing && !state.yoyoReconnecting) {
        if(state.yoyoPaired) {if(state.yoyoReconnectSupported)planter::submit(planter::Action::ReconnectYoyo);}
        else if(!state.yoyoLinkConnected && !connection_ui::secondsLeft(state,now))planter::submit(planter::Action::PairYoyo);
      }
      else if(tapped(kAutoPrint,touch) && state.autoPrintKnown)planter::submit(planter::Action::AutoPrint,!state.autoPrint);
    }
  } else if (page == Page::Planter) {
    if(tapped(kCompanion,touch))page=Page::Yoyo;
    else if(tapped(kCalibration,touch))page=Page::Calibration;
    else if (tapped(kNetworkSettings, touch)) showWifi();
    else if (tapped(kPairedNetwork, touch) && device_settings::wifiState().ready) {
      const auto wifi=device_settings::wifiState();
      const auto state=planter::snapshot();
      if(!state.commandBlocked && !state.checking && !wifi.scanning && wifi.phase!=WifiPhase::Connecting) {
        if(wifi.phase==WifiPhase::Connected && wifi.planterNetwork)planter::requestRefresh();
        else if(wifi.phase==WifiPhase::Connected)device_settings::restoreDefaultWifi();
        else device_settings::requestReconnect();
      }
    }
  } else if (page == Page::Wifi) {
    const auto state = device_settings::wifiState();
    if (tapped(kRefresh, touch) && state.ready && !state.scanning) beginScan();
    else if (tapped(kDefault, touch) && state.ready) { wipePassword(); device_settings::restoreDefaultWifi(); }
    else if (tapped(kPrevious, touch) && networkPage && !state.scanning) --networkPage;
    else if (tapped(kNext, touch) && networkPage == 0 && listCount > 5 && !state.scanning) ++networkPage;
    else if (!state.scanning) {
      for (unsigned row = 0; row < 5; ++row) {
        const unsigned index = networkPage * 5 + row;
        if (index < listCount && tapped(networkRect(row), touch)) {
          wipePassword(); memcpy(selectedSsid, list[index].ssid, sizeof(selectedSsid)); selectedSsid[32] = 0;
          secureNetwork = list[index].secure; keyboardMode = 0; page = Page::Password; break;
        }
      }
    }
  } else if (page == Page::Password) {
    if (tapped(kModes[3], touch) && validPassword()) {
      if (device_settings::requestConnect(selectedSsid, password)) {
        wipePassword(); selectedSsid[0] = 0; page = Page::Wifi;
      } else {
        joinError = device_settings::wifiState().error;
        if (joinError == WifiError::None) joinError = WifiError::ConnectionFailed;
      }
    } else if (secureNetwork) {
      if (tapped(kModes[0], touch)) keyboardMode = 0;
      else if (tapped(kModes[1], touch)) keyboardMode = 1;
      else if (tapped(kModes[2], touch)) keyboardMode = keyboardMode == 2 ? 3 : 2;
      else for (unsigned i = 0; i < 32; ++i) if (tapped(keyRect(i), touch)) {
        if (i == 31) { if (passwordLength) password[--passwordLength] = 0; }
        else if (passwordLength < 64) {
          const char c = keyCharacter(i);
          if (c) { password[passwordLength++] = c; password[passwordLength] = 0; }
        }
        break;
      }
    }
  }
  return true;
}
}  // namespace settings_ui
