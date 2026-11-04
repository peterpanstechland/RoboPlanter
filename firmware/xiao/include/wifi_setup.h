#pragma once
#include <Preferences.h>
#include <ESPmDNS.h>
#include "wifi_credentials.h"

// Called only by setup/loop/HTTP on the Arduino task. No radio/NVS work on
// BLE callbacks. Trial credentials replace the committed blob only after DHCP.
namespace wifi_setup {
Preferences storage;
robo::WifiCredentials committed, candidate;
bool available=false, trial=false, startPending=false, apOpen=false, mdns=false;
uint32_t started=0, offlineSince=0, retryAt=0, apUntil=0;
String setupSsid, deviceName;
char host[64]={};
char requestId[33]={};
const char* phase="idle";
const char* error="";
void begin(const String& id) {
  deviceName=id;setupSsid=id+"-Setup";strlcpy(host,id.c_str(),sizeof(host));
  for(char* p=host;*p;++p)if(*p>='A'&&*p<='Z')*p+=32;
  available=storage.begin("rp-network",false);
  strlcpy(committed.ssid,ROBO_STA_SSID,sizeof(committed.ssid));
  strlcpy(committed.password,ROBO_STA_PASSWORD,sizeof(committed.password));
  robo::WifiCredentials saved;
  if(available && storage.getBytesLength("wifi")==sizeof(saved) &&
     storage.getBytes("wifi",&saved,sizeof(saved))==sizeof(saved) && saved.valid())committed=saved;
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);
  WiFi.setHostname(host);
  if(committed.valid())WiFi.begin(committed.ssid,committed.password);
  started=offlineSince=millis();phase="connecting";
}
bool openWindow() {
  if(!strcmp(ROBO_AP_PASSWORD,"planter-demo"))return false;
  if(!apOpen) {
    WiFi.mode(WIFI_AP_STA);
    // Per-device AP password is provisioned privately in both boards.
    if(!WiFi.softAP(setupSsid.c_str(),ROBO_AP_PASSWORD,1,false,2))return false;
    apOpen=true;
  }
  apUntil=millis()+180000;return true;
}
bool submit(const char* id,const char* ssid,const char* password) {
  if(!available || trial || startPending || !ssid || !password || strlen(ssid)>32 || strlen(password)>64)return false;
  robo::WifiCredentials next;
  strlcpy(next.ssid,ssid,sizeof(next.ssid));strlcpy(next.password,password,sizeof(next.password));
  if(!next.valid())return false;
  candidate=next;strlcpy(requestId,id,sizeof(requestId));startPending=true;
  phase="queued";error="";return true;
}
void state(JsonObject out) {
  out["supported"]=true;out["storage"]=available;
  out["phase"]=phase;out["error"]=error;out["request_id"]=requestId;
  out["setup_open"]=apOpen;out["setup_ssid"]=setupSsid;
  out["connected"]=WiFi.status()==WL_CONNECTED;
  out["ssid"]=WiFi.status()==WL_CONNECTED?WiFi.SSID():String(committed.ssid);
  out["ip"]=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String("");
  out["hostname"]=host;
  out["remaining_ms"]=trial && uint32_t(millis()-started)<25000?25000-uint32_t(millis()-started):0;
}
void service() {
  const uint32_t now=millis();
  if(startPending) {
    startPending=false;trial=true;phase="connecting";started=now;
    WiFi.disconnect(false,false);WiFi.begin(candidate.ssid,candidate.password);
  }
  const bool connected=WiFi.status()==WL_CONNECTED;
  if(connected && !mdns) {
    mdns=MDNS.begin(host);
    if(mdns) {MDNS.addService("roboplanter","tcp",80);MDNS.addServiceTxt("roboplanter","tcp","device_id",deviceName);}
  }
  if(trial) {
    if(connected && WiFi.SSID()==candidate.ssid) {
      const bool saved=storage.putBytes("wifi",&candidate,sizeof(candidate))==sizeof(candidate);
      if(saved)committed=candidate;
      trial=false;phase=saved?"saved":"failed";error=saved?"":"storage_failed";
      memset(&candidate,0,sizeof(candidate));
      if(!saved){WiFi.disconnect(false,false);if(committed.valid())WiFi.begin(committed.ssid,committed.password);}
    } else if(now-started>=25000) {
      trial=false;phase="failed";error="wifi_connection_failed";memset(&candidate,0,sizeof(candidate));
      WiFi.disconnect(false,false);if(committed.valid())WiFi.begin(committed.ssid,committed.password);
      retryAt=now+30000;
    }
  }
  if(connected) {offlineSince=0;retryAt=0;if(!trial && !requestId[0])phase="connected";}
  else {
    if(!offlineSince)offlineSince=now;
    if(now-offlineSince>=30000 && !apOpen)openWindow();
    if(!trial && !retryAt)retryAt=now+30000;
    if(!trial && retryAt && int32_t(now-retryAt)>=0 && committed.valid()) {
      WiFi.disconnect(false,false);WiFi.begin(committed.ssid,committed.password);retryAt=now+30000;
    }
  }
  // Recovery AP stays available while offline; closes after the setup window
  // only once the device has a working station connection.
  if(apOpen && connected && !trial && int32_t(now-apUntil)>=0) {
    WiFi.softAPdisconnect(false);WiFi.mode(WIFI_STA);apOpen=false;
  }
}
}
