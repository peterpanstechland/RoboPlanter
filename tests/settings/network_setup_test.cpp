#define RP_HOST_FIXTURE 1
#define PLANTER_DEVICE_ID "RoboPlanter-TEST"
#define PLANTER_SETUP_PASSWORD "setup-test-password"
#define PLANTER_WIFI_SSID "old-network"
#define PLANTER_WIFI_PASSWORD "old-test-password"
#define PLANTER_DEVICE_TOKEN "fixture-token-for-tests"
#include <iostream>
#include <stdexcept>
#include "../../firmware/indicator/src/device_settings.cpp"
#include "../../firmware/indicator/src/network_setup.cpp"
namespace planter {State fixture;State snapshot(){return fixture;}}
unsigned checks=0;
#define CHECK(c) do{++checks;if(!(c))throw std::runtime_error("line "+std::to_string(__LINE__));}while(0)
void connect() {device_settings::service();settingsTestNow+=200;device_settings::service();WiFi.connect();device_settings::service();}
int main(){try {
  using network_setup::Phase;
  CHECK(device_settings::begin());connect();CHECK(device_settings::planterNetworkReady());
  CHECK(network_setup::begin());CHECK(network_setup::supported());
  const auto original=Preferences::values["wifi"];
  settingsTestPostBody=R"({"device_id":"RoboPlanter-TEST","network":{"setup_open":true}})";
  CHECK(network_setup::start("new-network","new-test-password"));CHECK(!network_setup::start("other","other-password"));
  network_setup::service();CHECK(network_setup::snapshot().phase==Phase::JoiningSetup);
  connect();CHECK(WiFi.current=="RoboPlanter-TEST-Setup");CHECK(!device_settings::planterNetworkReady());
  CHECK(Preferences::values["wifi"]==original); // Temporary setup AP is never a boot network.
  settingsTestPostCode=-1;network_setup::service();CHECK(network_setup::snapshot().phase==Phase::WaitingXiao);
  const auto posts=settingsTestPosts.size();
  settingsTestStateBody=R"({"device_id":"RoboPlanter-TEST","network":{"request_id":"wifi-12345678","phase":"saved","connected":true,"ssid":"new-network","ip":"192.168.4.2"}})";
  network_setup::service();CHECK(settingsTestPosts.size()==posts); // Lost POST response is read back, never resent.
  CHECK(network_setup::snapshot().phase==Phase::JoiningNetwork);
  connect();CHECK(device_settings::planterNetworkReady());
  CHECK(!strcmp(device_settings::planterEndpoint().ssid,"new-network"));
  CHECK(!strcmp(device_settings::planterEndpoint().address,"http://192.168.4.2"));
  settingsTestStateBody=R"({"device_id":"RoboPlanter-WRONG"})";
  network_setup::service();CHECK(network_setup::snapshot().phase==Phase::Verifying);
  settingsTestNow+=1001;settingsTestStateBody=R"({"device_id":"RoboPlanter-TEST"})";
  network_setup::service();CHECK(network_setup::snapshot().phase==Phase::Done);
  CHECK(device_settings::restoreDefaultWifi());connect();CHECK(WiFi.current=="new-network");
  initialized=false;CHECK(device_settings::begin());CHECK(!strcmp(device_settings::wifiState().targetSsid,"new-network"));
  CHECK(!strcmp(device_settings::planterEndpoint().address,"http://192.168.4.2"));
  planter::fixture.commandBlocked=true;CHECK(!network_setup::start("blocked","valid-password"));
  std::cout<<checks<<" unified setup, lost response, identity and boot persistence checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
