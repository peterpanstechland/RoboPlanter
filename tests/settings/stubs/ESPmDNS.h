#pragma once
#include "WiFi.h"
struct TestMDNS {
  bool begin(const char*){return true;}
  void addService(const char*,const char*,int){}
  void addServiceTxt(const char*,const char*,const char*,const String&){}
  IPAddress queryHost(const char*,uint32_t){return {};}
};
inline TestMDNS MDNS;
