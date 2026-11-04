#pragma once
#include <cstdint>
namespace network_setup {
enum class Phase : uint8_t {Idle,Opening,JoiningSetup,Sending,WaitingXiao,JoiningNetwork,Verifying,Done,Failed,Unknown};
struct State {Phase phase=Phase::Idle;uint32_t startedMs=0;char error[64]={};};
bool begin();
bool supported();
bool busy(Phase phase);
State snapshot();
bool start(const char* ssid,const char* password);
void service(); // HTTP worker only; never called by display task.
}
