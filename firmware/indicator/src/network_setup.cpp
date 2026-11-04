#include "network_setup.h"
#include "config.h"
#include "device_settings.h"
#include "planter_client.h"
#include "../../xiao/include/wifi_credentials.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
namespace network_setup {
namespace {
SemaphoreHandle_t mutex=nullptr;
QueueHandle_t requests=nullptr;
State current;
robo::WifiCredentials target;
char requestId[33]={};
uint32_t phaseAt=0,nextCheck=0,discoveryAt=0;
bool mdns=false;
void phase(Phase p,const char* error="") {
  xSemaphoreTake(mutex,portMAX_DELAY);current.phase=p;current.startedMs=millis();
  strlcpy(current.error,error,sizeof(current.error));xSemaphoreGive(mutex);phaseAt=millis();nextCheck=0;
  if(!busy(p))memset(&target,0,sizeof(target));
}
bool call(const char* origin,const char* path,const String* body,JsonDocument& result,int* httpStatus=nullptr) {
  WiFiClient client;HTTPClient http;http.setConnectTimeout(1000);http.setTimeout(1500);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if(!http.begin(client,String(origin)+path))return false;
  http.addHeader("X-Device-Token",PLANTER_DEVICE_TOKEN);
  int code;
  if(body){http.addHeader("Content-Type","application/json");code=http.POST(*body);}else code=http.GET();
  if(httpStatus)*httpStatus=code;
  if(code<200 || code>=300 || http.getSize()<0 || http.getSize()>4096){http.end();return false;}
  const String value=http.getString();http.end();
  return !deserializeJson(result,value) && !strcmp(result["device_id"]|"",PLANTER_DEVICE_ID);
}
void fail(const char* error) {device_settings::restoreDefaultWifi();phase(Phase::Failed,error);}
}
bool busy(Phase p){return p>=Phase::Opening && p<=Phase::Verifying;}
bool supported(){return PLANTER_DEVICE_ID[0] && strlen(PLANTER_SETUP_PASSWORD)>=8 && strcmp(PLANTER_SETUP_PASSWORD,"planter-demo");}
bool begin() {
  mutex=xSemaphoreCreateMutex();requests=xQueueCreate(1,sizeof(robo::WifiCredentials));
  if(mutex && requests)return true;
  if(mutex)vSemaphoreDelete(mutex);if(requests)vQueueDelete(requests);
  mutex=nullptr;requests=nullptr;return false;
}
State snapshot(){if(!mutex)return State();xSemaphoreTake(mutex,portMAX_DELAY);auto s=current;xSemaphoreGive(mutex);return s;}
bool start(const char* ssid,const char* password) {
  if(!mutex || !requests || !supported() || !ssid || !password || strlen(ssid)>32 || strlen(password)>64)return false;
  robo::WifiCredentials value;strlcpy(value.ssid,ssid,sizeof(value.ssid));strlcpy(value.password,password,sizeof(value.password));
  if(!value.valid())return false;
  const auto planter=planter::snapshot();
  if(planter.commandBlocked || planter.pumpOn)return false;
  xSemaphoreTake(mutex,portMAX_DELAY);
  const bool ok=!busy(current.phase) && xQueueSend(requests,&value,0)==pdTRUE;
  if(ok){current.phase=Phase::Opening;current.startedMs=millis();current.error[0]=0;}
  xSemaphoreGive(mutex);memset(&value,0,sizeof(value));return ok;
}
void service() {
  if(!mutex || !requests || !supported())return;
  const uint32_t now=millis();
  if(xQueueReceive(requests,&target,0)==pdTRUE) {
    phase(Phase::Opening);
    snprintf(requestId,sizeof(requestId),"wifi-%08lx",static_cast<unsigned long>(esp_random()));
    // Online device explicitly opens its window. Offline device exposes the
    // recovery AP after 30 seconds; no credentials are sent to other networks.
    if(device_settings::planterNetworkReady()) {
      DynamicJsonDocument response(1024);String empty="{}";
      const auto endpoint=device_settings::planterEndpoint();
      call(endpoint.address,"/api/v1/wifi/setup",&empty,response);
    }
    const String setup=String(PLANTER_DEVICE_ID)+"-Setup";
    if(!device_settings::requestTemporaryConnect(setup.c_str(),PLANTER_SETUP_PASSWORD)){phase(Phase::Failed,"network_busy");return;}
    phase(Phase::JoiningSetup);
  }
  const Phase p=snapshot().phase;
  if(!busy(p)) {
    // Resolve only on the user-established trusted network; discovery never
    // grants trust to an arbitrary office/cafe Wi-Fi.
    if(device_settings::planterNetworkReady() && now-discoveryAt>15000 &&
       planter::snapshot().networkFailure==planter::NetworkFailure::Http) {
      discoveryAt=now;if(!mdns)mdns=MDNS.begin((String("indicator-")+PLANTER_DEVICE_ID).c_str());
      char host[64];strlcpy(host,PLANTER_DEVICE_ID,sizeof(host));
      for(char* cursor=host;*cursor;++cursor)if(*cursor>='A'&&*cursor<='Z')*cursor+=32;
      const IPAddress ip=MDNS.queryHost(host,800);
      if(uint32_t(ip))device_settings::updatePlanterAddress((String("http://")+ip.toString()).c_str());
    }
    return;
  }
  if(now-phaseAt>90000){fail("setup_timeout");return;}
  const auto wifi=device_settings::wifiState();
  const String setup=String(PLANTER_DEVICE_ID)+"-Setup";
  if(p==Phase::JoiningSetup) {
    if(wifi.phase!=device_settings::WifiPhase::Connected || strcmp(wifi.connectedSsid,setup.c_str()))return;
    phase(Phase::Sending);
    DynamicJsonDocument doc(384);doc["request_id"]=requestId;doc["ssid"]=target.ssid;doc["password"]=target.password;
    String body;serializeJson(doc,body);DynamicJsonDocument response(1024);
    // Exactly one POST. A missing response is reconciled by request ID below.
    int status=0;
    call("http://192.168.4.1","/api/v1/wifi",&body,response,&status);
    if(status>=300){fail(status==401?"device_authentication_failed":"xiao_rejected_settings");return;}
    phase(Phase::WaitingXiao);return;
  }
  if(p==Phase::WaitingXiao) {
    if(now<nextCheck)return;nextCheck=now+1000;
    if(wifi.phase!=device_settings::WifiPhase::Connected || strcmp(wifi.connectedSsid,setup.c_str()))return;
    DynamicJsonDocument response(1024);
    if(!call("http://192.168.4.1","/api/v1/wifi",nullptr,response))return;
    const auto network=response["network"];
    if(strcmp(network["request_id"]|"",requestId))return;
    if(!strcmp(network["phase"]|"","failed")){fail(network["error"]|"xiao_wifi_failed");return;}
    if(!strcmp(network["phase"]|"","saved") && network["connected"]==true && !strcmp(network["ssid"]|"",target.ssid)) {
      IPAddress ip;if(!ip.fromString(network["ip"]|"")){fail("invalid_device_address");return;}
      const String origin=String("http://")+ip.toString();
      if(!device_settings::configurePlanterNetwork(target.ssid,target.password,origin.c_str())){fail("screen_network_busy");return;}
      phase(Phase::JoiningNetwork);
    }
    return;
  }
  if(p==Phase::JoiningNetwork && device_settings::planterNetworkReady())phase(Phase::Verifying);
  if(snapshot().phase==Phase::Verifying && now>=nextCheck) {
    nextCheck=now+1000;DynamicJsonDocument response(4096);
    const auto endpoint=device_settings::planterEndpoint();
    if(call(endpoint.address,"/api/v1/state",nullptr,response))phase(Phase::Done);
  }
}
}
