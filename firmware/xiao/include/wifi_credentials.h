#pragma once
#include <cstdint>
#include <cstring>
namespace robo {
struct WifiCredentials {
  uint32_t version=1;
  char ssid[33]={}, password[65]={};
  bool valid() const {
    if(version!=1 || ssid[32] || password[64])return false;
    const size_t n=strlen(ssid),p=strlen(password);
    if(!n || n>32 || p>64 || (p && p<8))return false;
    for(size_t i=0;i<n;++i)if(uint8_t(ssid[i])<32 || uint8_t(ssid[i])==127)return false;
    for(size_t i=0;i<p;++i) {
      const unsigned char c=password[i];
      if(c<32 || c>126)return false;
      if(p==64 && !((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
    }
    return true;
  }
};
}
