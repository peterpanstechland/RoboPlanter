#include <iostream>
#include <stdexcept>
#include <ArduinoJson.h>
#include <WiFi.h>
#define ROBO_STA_SSID "old-network"
#define ROBO_STA_PASSWORD "old-test-password"
#define ROBO_AP_PASSWORD "setup-test-password"
#include "../../firmware/xiao/include/wifi_setup.h"
unsigned checks=0;
#define CHECK(c) do {++checks;if(!(c))throw std::runtime_error("line "+std::to_string(__LINE__));}while(0)
int main() {try {
  using namespace wifi_setup;
  begin("RoboPlanter-TEST");CHECK(!strcmp(committed.ssid,"old-network"));CHECK(!Preferences::values.count("wifi"));
  CHECK(!submit("invalid","network","short"));
  CHECK(submit("first","new-network","new-test-password"));CHECK(!submit("second","other","other-password"));
  service();CHECK(trial && WiFi.attempted=="new-network");CHECK(!Preferences::values.count("wifi"));
  WiFi.connect();service();CHECK(!trial && !strcmp(phase,"saved"));CHECK(Preferences::values.count("wifi"));
  const auto saved=Preferences::values["wifi"];
  begin("RoboPlanter-TEST");CHECK(!strcmp(committed.ssid,"new-network"));
  CHECK(submit("bad","bad-network","bad-password"));service();CHECK(trial);
  settingsTestNow+=25001;service();CHECK(!trial && !strcmp(phase,"failed"));
  CHECK(Preferences::values["wifi"]==saved);CHECK(WiFi.attempted=="new-network");
  CHECK(submit("disk","another-network","another-password"));service();Preferences::available=false;
  WiFi.connect();service();CHECK(!strcmp(error,"storage_failed"));CHECK(Preferences::values["wifi"]==saved);
  CHECK(WiFi.attempted=="new-network");Preferences::available=true;
  WiFi.link=WL_DISCONNECTED;settingsTestNow+=31000;service();settingsTestNow+=31000;service();CHECK(apOpen);
  StaticJsonDocument<768> json;state(json.to<JsonObject>());String body;serializeJson(json,body);
  CHECK(body.find("password")==String::npos);CHECK(body.find("new-test-password")==String::npos);
  CHECK(!strcmp(json["hostname"]|"","roboplanter-test"));
  std::cout<<checks<<" XIAO network trial, persistence, rollback and recovery checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
