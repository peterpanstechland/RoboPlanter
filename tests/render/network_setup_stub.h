#pragma once
#include "network_setup.h"
namespace network_setup {
inline bool enabled=false;
inline State fixture;
inline unsigned starts=0;
bool supported(){return enabled;}
bool busy(Phase p){return p>=Phase::Opening && p<=Phase::Verifying;}
State snapshot(){return fixture;}
bool start(const char*,const char*){++starts;fixture.phase=Phase::JoiningSetup;return true;}
bool begin(){return true;}
void service(){}
}
