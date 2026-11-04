#pragma once
#include "planter_client.h"
#include <cstring>

// Presentation is derived from a fresh XIAO snapshot, never a cached PIN or
// the mere presence of a saved bond. All timestamps use unsigned subtraction.
namespace connection_ui {
enum class PhonePhase { Offline, Disabled, Repairing, Reconnecting, Failed,
  Pairing, Expired, Unpaired, Authenticating, ToolSeen, Secure, PairedIdle };
inline uint32_t secondsLeft(const planter::State& s, uint32_t now) {
  const uint32_t elapsed=now-s.receivedMs;
  return s.yoyoPairing && s.yoyoWindowMs>elapsed ? (s.yoyoWindowMs-elapsed+999)/1000 : 0;
}
inline bool showPin(const planter::State& s, uint32_t now) {
  return planter::fresh(s,now) && !s.yoyoPaired && secondsLeft(s,now)>0 &&
    s.yoyoPin>=100000 && s.yoyoPin<=999999;
}
inline PhonePhase phonePhase(const planter::State& s, uint32_t now) {
  if(!planter::fresh(s,now))return PhonePhase::Offline;
  if(!s.yoyoEnabled)return PhonePhase::Disabled;
  if(!strcmp(s.yoyoStage,"repair_restart_required"))return PhonePhase::Failed;
  if(s.yoyoRepairing)return PhonePhase::Repairing;
  if(s.yoyoReconnecting)return PhonePhase::Reconnecting;
  if(s.yoyoConnected)return s.yoyoToolSeen?PhonePhase::ToolSeen:PhonePhase::Secure;
  if(!strcmp(s.yoyoStage,"pairing_timeout") || !strcmp(s.yoyoStage,"authentication_failed") ||
     !strcmp(s.yoyoStage,"encryption_start_failed") || !strcmp(s.yoyoStage,"security_mode_rejected") ||
     !strcmp(s.yoyoStage,"enrollment_rejected") || !strcmp(s.yoyoStage,"repair_failed") ||
     !strcmp(s.yoyoStage,"reconnect_failed"))return PhonePhase::Failed;
  if(s.yoyoLinkConnected)return PhonePhase::Authenticating;
  if(s.yoyoPaired)return PhonePhase::PairedIdle;
  if(secondsLeft(s,now))return PhonePhase::Pairing;
  if(s.yoyoPairing || !strcmp(s.yoyoStage,"ready") || !strcmp(s.yoyoStage,"window_closed"))return PhonePhase::Expired;
  return PhonePhase::Unpaired;
}
inline const char* title(PhonePhase p,bool en) {
  switch(p) {
    case PhonePhase::Offline:return en?"Check screen / XIAO link first":"先恢复屏幕与主控连接";
    case PhonePhase::Disabled:return en?"Phone service unavailable":"主控手机服务未启用";
    case PhonePhase::Repairing:return en?"Removing the old phone bond":"正在清除旧手机绑定";
    case PhonePhase::Reconnecting:return en?"Releasing the phone link":"正在释放手机连接";
    case PhonePhase::Failed:return en?"Phone connection failed":"手机连接未完成";
    case PhonePhase::Pairing:return en?"Waiting for phone pairing":"等待手机输入配对码";
    case PhonePhase::Expired:return en?"Pairing code expired":"配对码已过期";
    case PhonePhase::Unpaired:return en?"Pair your phone once":"首次连接：绑定手机";
    case PhonePhase::Authenticating:return en?"Verifying the phone":"已发现手机，正在认证";
    case PhonePhase::ToolSeen:return en?"YOYO request reached XIAO":"YOYO 请求已到主控";
    case PhonePhase::Secure:return en?"Phone connected; awaiting Skill":"手机已连接，等待技能请求";
    default:return en?"Phone saved; reconnect on demand":"已绑定，手机按需连接";
  }
}
inline const char* hint(PhonePhase p,bool en) {
  switch(p) {
    case PhonePhase::Offline:return en?"Tap Check link to see the failing step":"点下方检查链路，查看断在哪一段";
    case PhonePhase::Disabled:return en?"Check XIAO configuration / firmware":"检查 XIAO 固件与手机服务配置";
    case PhonePhase::Repairing:return en?"Keep XIAO powered; a new code will appear":"保持主控上电，完成后自动显示新码";
    case PhonePhase::Reconnecting:return en?"Your saved phone bond is kept":"保留手机绑定，完成后打开小芽技能";
    case PhonePhase::Failed:return en?"Retry connection first; rebind only if needed":"先重连；仍失败时再重新绑定手机";
    case PhonePhase::Pairing:return en?"Phone Bluetooth > RoboPlanter-A994":"手机设置 > 蓝牙 > RoboPlanter-A994";
    case PhonePhase::Expired:return en?"Open a new window; use the new code":"重新开启窗口，使用新码完成配对";
    case PhonePhase::Unpaired:return en?"Tap Show code, then open phone Bluetooth":"先显示配对码，再打开手机系统蓝牙";
    case PhonePhase::Authenticating:return en?"Complete the prompt on your phone":"查看手机配对弹窗，完成认证";
    case PhonePhase::ToolSeen:return en?"Read the actual result in YOYO":"在手机查看实际结果；闲置断开属正常";
    default:return en?"Open Xiao Ya and say: Read desk status":"打开工位小芽，说：读取工位状态";
  }
}
inline const char* failureHint(const planter::State& s,bool en) {
  if(!strcmp(s.yoyoStage,"pairing_timeout"))return en?"Pairing timed out; check the phone prompt":"手机配对超时，请检查手机弹窗";
  if(!strcmp(s.yoyoStage,"reconnect_failed"))return en?"Could not release link; turn phone Bluetooth off":"连接未释放，请先关闭手机蓝牙";
  if(!strcmp(s.yoyoStage,"repair_failed") || !strcmp(s.yoyoStage,"repair_restart_required"))return en?"Bond removal failed; restart XIAO, then check":"旧绑定未清除，请重启主控后检查";
  if(!strcmp(s.yoyoStage,"security_mode_rejected"))return en?"Phone did not use secure pairing":"手机未使用加密配对，请重新绑定";
  if(!strcmp(s.yoyoStage,"enrollment_rejected"))return en?"This phone was not accepted as the owner":"未接受这部手机，请核对已有绑定";
  if(!strcmp(s.yoyoStage,"encryption_start_failed"))return en?"Encryption failed to start; reconnect":"加密未能启动，请重连手机";
  return en?"Phone authentication failed; reconnect first":"手机认证失败，先尝试保留绑定重连";
}
}
