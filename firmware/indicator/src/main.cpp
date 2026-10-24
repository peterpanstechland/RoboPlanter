#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "config.h"
#include "indicator_board.h"
#include "planter_client.h"
#include "ui_model.h"
#include "ui_photos.h"
#include "ui_text.h"
#include "ui_theme.h"

namespace {
using planter::State;
bool displayReady=false, clientReady=false, controlsOpen=false, prefsReady=false;
ui::Model model;
ui::TouchGate touch;
Preferences preferences;
size_t photoIndex=0;
constexpr uint16_t kWarning=0xD360, kDanger=0xB928;
struct Rect { int16_t x,y,w,h; };
constexpr Rect kBack{16,12,48,44}, kTheme{340,12,124,44};
constexpr Rect kControls{24,368,432,52}, kDismiss{344,76,112,44};
constexpr Rect kHardware[]={{24,150,210,86},{246,150,210,86},{24,250,210,86},{246,250,210,86}};
constexpr planter::Action kActions[]={planter::Action::Water,planter::Action::StopWater,planter::Action::Light,planter::Action::Print};
constexpr Rect kTimerPresets[]={{24,314,136,44},{172,314,136,44},{320,314,136,44}};
constexpr Rect kTimerToggle{24,374,278,58}, kTimerReset{316,374,140,58};
constexpr Rect kPhotoPrevious{24,391,64,44}, kPhotoNext{392,391,64,44};
const ui_theme::Theme &theme() { return ui_theme::get(model.theme()); }
uint16_t contrastInk(uint16_t color) {
  // Choose the higher-contrast black/white ink using sRGB relative luminance.
  auto linear=[](float c){return c<=0.04045f?c/12.92f:powf((c+0.055f)/1.055f,2.4f);};
  const float luminance=0.2126f*linear(((color>>11)&31)/31.0f)+
      0.7152f*linear(((color>>5)&63)/63.0f)+0.0722f*linear((color&31)/31.0f);
  return luminance>0.179f?0x0000:0xFFFF;
}
bool inside(const Rect &r,int32_t x,int32_t y) {return x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;}
bool tapped(const Rect &r) {return inside(r,touch.startX(),touch.startY()) && inside(r,touch.lastX(),touch.lastY());}
void label(int16_t x,int16_t y,const char *s,uint16_t color,uint8_t scale=1) {
  ui_text::draw(indicator::display(),x,y,s,color,scale);
}
void centered(int16_t y,const char *s,uint16_t color,uint8_t scale=1) {
  label((480-ui_text::width(s,scale))/2,y,s,color,scale);
}
void clipped(int16_t x,int16_t y,const char *s,int16_t maxWidth,uint16_t color) {
  // Do not split a UTF-8 codepoint or paint over another card.
  char out[96]={};size_t n=0,i=0;
  while(s[i] && n<sizeof(out)-5) {
    const uint8_t lead=static_cast<uint8_t>(s[i]);
    size_t bytes=lead<0x80?1:(lead>=0xC2 && lead<=0xDF)?2:
        (lead>=0xE0 && lead<=0xEF)?3:(lead>=0xF0 && lead<=0xF4)?4:0;
    bool valid=bytes!=0;
    for(size_t j=1;valid && j<bytes;++j) {
      const uint8_t c=static_cast<uint8_t>(s[i+j]);
      if(c==0 || (c&0xC0)!=0x80)valid=false;
    }
    if(!valid){bytes=1;out[n]='?';}
    else memcpy(out+n,s+i,bytes);
    out[n+bytes]=0;
    if(ui_text::width(out)>maxWidth-14) {out[n]=0;strcat(out,"..");break;}
    n+=bytes;i+=bytes;
  }
  label(x,y,out,color);
}
void pill(const Rect &r,const char *s,uint16_t fill,uint16_t ink,bool outline=false) {
  auto &g=indicator::display();
  g.fillRoundRect(r.x,r.y,r.w,r.h,14,fill);
  if(outline)g.drawRoundRect(r.x,r.y,r.w,r.h,14,theme().accent);
  label(r.x+(r.w-ui_text::width(s))/2,r.y+(r.h-16)/2,s,ink);
}
const char *connection(const State &s,uint32_t now) {
  if(!clientReady)return "连接任务未就绪";
  if(!s.received)return s.wifi?"等待花盆回应":"正在连接花盆";
  if(!s.wifi)return "离线 · 显示上次读数";
  if(!planter::fresh(s,now))return "读数已过期";
  return "花盆已连接";
}
void statusStrip(const State &s,uint32_t now) {
  const bool fresh=planter::fresh(s,now);
  auto &g=indicator::display();
  g.fillRoundRect(24,437,432,29,12,theme().card);
  g.fillCircle(38,451,4,fresh?theme().accent:kWarning);
  label(50,443,connection(s,now),fresh?theme().ink:theme().muted);
  if(!indicator::touchReady())label(280,443,"触摸未就绪",kWarning);
  else if(model.page()!=ui::Page::Face) {
    const uint8_t p=static_cast<uint8_t>(model.page());
    for(uint8_t i=0;i<5;++i)g.fillCircle(390+i*11,451,i==p?4:2,i==p?theme().accent:theme().muted);
  }
}
void header() {
  auto &g=indicator::display();
  if(model.page()!=ui::Page::Face || controlsOpen || model.themesOpen()) {
    g.fillRoundRect(kBack.x,kBack.y,kBack.w,kBack.h,12,theme().card);
    g.drawLine(43,22,32,32,theme().ink);g.drawLine(32,32,43,42,theme().ink);
  } else {g.fillCircle(35,32,18,theme().accent);g.fillCircle(30,29,2,theme().card);g.fillCircle(40,29,2,theme().card);g.drawLine(30,37,40,37,theme().card);}
  label(78,24,"RoboPlanter",theme().ink);
  pill(kTheme,model.themesOpen()?"收起主题":"换个心情",theme().card,theme().ink,true);
}
void title(const char *s,const char *subtitle) {label(24,72,s,theme().ink,2);if(subtitle)label(24,109,subtitle,theme().muted);}
void facePage(const State &s,uint32_t now) {
  title("工位小搭子","摸摸我，看看今天");
  const uint32_t blinkPhase=now%5300U;
  const bool blinking=blinkPhase<150 || (blinkPhase>350 && blinkPhase<440);
  ui_theme::face(indicator::display(),model.theme(),240,260,100,blinking,now);
  pill({92,379,296,44},"点击小脸 · 今日状态",theme().card,theme().ink);
  // The face remains playful offline. It is decorative, not a health claim.
  statusStrip(s,now);
}
void readingCard(const Rect &r,const char *name,const planter::Reading &reading,
                 const char *unit,bool fresh,const char *invalid=nullptr) {
  auto &g=indicator::display();g.fillRoundRect(r.x,r.y,r.w,r.h,18,theme().card);
  label(r.x+16,r.y+12,name,theme().muted);
  char value[32];
  if(reading.valid)snprintf(value,sizeof(value),"%.1f%s",reading.value,unit);
  else strlcpy(value,"--",sizeof(value));
  label(r.x+16,r.y+36,value,fresh && reading.valid?theme().ink:theme().muted,2);
  label(r.x+16,r.y+77,!reading.valid?(invalid?invalid:"暂时没有有效读数"):!fresh?"上次读数 · 已过期":"设备有效读数",fresh && reading.valid?theme().accent:theme().muted);
}
void todayPage(const State &s,uint32_t now) {
  title("今日状态","真实读数，慢慢照顾");
  const bool fresh=planter::fresh(s,now);
  planter::Reading soil=s.soilPercent;
  if(!s.soilCalibrationKnown || !s.soilCalibrated)soil.valid=false;
  readingCard({24,138,210,104},"土壤湿度",soil,"%",fresh,!s.soilCalibrationKnown?"尚未收到土壤数据":!s.soilCalibrated?"土壤传感器待校准":"传感器暂不可用");
  readingCard({246,138,210,104},"空气温度",s.temperature,"C",fresh);
  readingCard({24,254,210,104},"空气湿度",s.humidity,"%",fresh);
  auto &g=indicator::display();g.fillRoundRect(246,254,210,104,18,theme().card);
  label(262,266,"设备动态",theme().muted);
  label(262,292,!s.pumpKnown?"泵：状态未知":!s.pumpEnabled?"泵：尚未启用":s.pumpOn?"泵：正在运行":"泵：已停止",fresh?theme().ink:theme().muted);
  label(262,324,!s.lightKnown?"灯：状态未知":!s.lightEnabled?"灯：尚未启用":s.lightOn?"灯：已开启":"灯：已关闭",fresh?theme().ink:theme().muted);
  pill(kControls,"打开硬件操作区",theme().accent,contrastInk(theme().accent));
  statusStrip(s,now);
}
void infoRow(int y,const char *name,const char *value,uint16_t color) {
  label(40,y,name,theme().muted);clipped(198,y,value,240,color);
}
void infoPage(const State &s,uint32_t now) {
  title("工位情报局","信息来自花盆，不替读数下结论");
  auto &g=indicator::display();g.fillRoundRect(24,138,432,206,18,theme().card);
  const bool fresh=planter::fresh(s,now);char buffer[96];
  infoRow(155,"连接",connection(s,now),fresh?theme().accent:theme().muted);
  if(s.received && s.ageKnown)snprintf(buffer,sizeof(buffer),"%lus",static_cast<unsigned long>((static_cast<uint64_t>(now-s.receivedMs)+s.sampleAgeMs)/1000));
  else strlcpy(buffer,"未知",sizeof(buffer));infoRow(190,"采样距今",buffer,theme().ink);
  if(s.airRaw.valid)snprintf(buffer,sizeof(buffer),"%.0f raw%s",s.airRaw.value,fresh?"":" · 旧值");else strlcpy(buffer,"暂无有效值",sizeof(buffer));infoRow(225,"空气原始值",buffer,fresh?theme().ink:theme().muted);
  if(s.uvRaw.valid)snprintf(buffer,sizeof(buffer),"%.0f raw%s",s.uvRaw.value,fresh?"":" · 旧值");else strlcpy(buffer,"暂无有效值",sizeof(buffer));infoRow(260,"紫外原始值",buffer,fresh?theme().ink:theme().muted);
  infoRow(295,"设备",s.deviceId[0]?s.deviceId:"等待设备信息",theme().ink);
  label(32,358,"原始数值不等于空气质量或光照强度",theme().muted);
  clipped(32,386,s.networkError[0]?s.networkError:"向右滑回到今日状态",416,theme().muted);
  statusStrip(s,now);
}
void photosPage(const State &s,uint32_t now) {
  label(24,70,"留一点喜欢的风景",theme().ink);
  const size_t count=ui_photos::count();
  if(count) {
    photoIndex%=count;
    if(!ui_photos::draw(indicator::display(),photoIndex,40,100)) {
      indicator::display().fillRoundRect(40,100,400,280,18,theme().card);
      centered(224,"这张照片暂时无法显示",theme().muted);
    }
    char countLabel[20];snprintf(countLabel,sizeof(countLabel),"%u / %u",unsigned(photoIndex+1),unsigned(count));
    pill(kPhotoPrevious,"<",theme().card,theme().ink);pill(kPhotoNext,">",theme().card,theme().ink);
    const char *caption=ui_photos::caption(photoIndex);
    clipped(104,397,caption && caption[0]?caption:"喜欢的瞬间",270,theme().ink);
    centered(418,countLabel,theme().muted);
  } else {
    auto &g=indicator::display();g.fillRoundRect(40,100,400,280,22,theme().card);
    g.drawRoundRect(130,148,220,132,12,theme().muted);g.fillCircle(287,185,16,theme().second);
    g.fillTriangle(140,269,203,201,267,269,theme().face);g.fillTriangle(221,269,274,224,340,269,theme().accent);
    centered(303,"还没有照片",theme().ink);centered(333,"放一张喜欢的风景",theme().muted);
    centered(405,"左滑，留一段时间给自己",theme().muted);
  }
  statusStrip(s,now);
}
void focusPage(const State &s,uint32_t now) {
  auto &timer=model.timer();auto &g=indicator::display();
  title(timer.done()?"这一段结束啦":"留一段时间给自己",timer.done()?"伸个懒腰，喝口水吧":
      timer.running() || timer.phase()==ui::TimerPhase::Paused?"切换时长前请先重置":"专注与休息，都值得认真对待");
  const uint32_t seconds=(timer.remainingMs(now)+999)/1000;char clock[12];
  snprintf(clock,sizeof(clock),"%02lu:%02lu",static_cast<unsigned long>(seconds/60),static_cast<unsigned long>(seconds%60));
  g.fillRoundRect(24,143,432,150,24,theme().card);centered(166,clock,theme().ink,4);
  const char *phase=timer.phase()==ui::TimerPhase::Running?"正在计时":timer.phase()==ui::TimerPhase::Paused?"已暂停":timer.done()?"已完成 · 不自动开始下一段":"准备好了就开始";
  centered(246,phase,theme().muted);
  const int progress=timer.durationMs()?static_cast<uint64_t>(timer.durationMs()-timer.remainingMs(now))*392/timer.durationMs():0;
  g.fillRoundRect(44,277,392,5,2,theme().face);if(progress>0)g.fillRoundRect(44,277,progress,5,2,theme().accent);
  const char *presets[]={"专注 25 分","短歇 5 分","长歇 15 分"};
  const bool canChange=timer.phase()==ui::TimerPhase::Ready || timer.done();
  for(uint8_t i=0;i<3;++i)pill(kTimerPresets[i],presets[i],static_cast<uint8_t>(timer.preset())==i?theme().accent:theme().card,static_cast<uint8_t>(timer.preset())==i?contrastInk(theme().accent):canChange?theme().ink:theme().muted);
  pill(kTimerToggle,timer.running()?"暂停一下":timer.phase()==ui::TimerPhase::Paused?"继续计时":timer.done()?"再来一段":"开始计时",theme().accent,contrastInk(theme().accent));
  pill(kTimerReset,"重置",theme().card,theme().ink,true);
  statusStrip(s,now);
}
bool enabled(planter::Action action,const State &s,uint32_t now) {
  const bool available=planter::fresh(s,now) && !s.commandBlocked && clientReady;
  switch(action) {
    case planter::Action::Water:return available && s.pumpEnabled && !s.pumpOn;
    case planter::Action::StopWater:return clientReady && s.wifi && s.pumpEnabled;
    case planter::Action::Light:return available && s.lightEnabled;
    case planter::Action::Print:return available && s.printerEnabled;
  }return false;
}
const char *commandLabel(const State &s) {
  if(s.uncertain)return "结果未知 · 请先检查花盆";
  if(!strcmp(s.commandStatus,"accepted"))return "设备已接受 · 等待完成";
  if(!strcmp(s.commandStatus,"completed"))return "设备报告软件动作已完成";
  if(!strcmp(s.commandStatus,"rejected"))return "设备拒绝了这次操作";
  if(!strcmp(s.commandStatus,"queued"))return "指令正在排队";
  return s.commandStatus[0]?s.commandStatus:"每次点击只发送一次指令";
}
void controlsPage(const State &s,uint32_t now) {
  label(24,76,"硬件操作",theme().ink,2);pill(kDismiss,"收起",theme().card,theme().ink);
  label(24,117,"这里只响应你明确点击的操作",theme().muted);
  char waterLabel[32];snprintf(waterLabel,sizeof(waterLabel),"浇水 %.1f 秒",PLANTER_WATER_DURATION_MS/1000.0);
  const char *labels[]={waterLabel,"立即停泵",s.lightOn?"关闭灯光":"开启灯光","打印便签"};
  auto &g=indicator::display();
  for(uint8_t i=0;i<4;++i) {
    const bool active=enabled(kActions[i],s,now);const Rect &r=kHardware[i];
    const uint16_t fill=active?(i==1?kDanger:theme().accent):theme().card;
    const uint16_t ink=active?contrastInk(fill):theme().muted;
    g.fillRoundRect(r.x,r.y,r.w,r.h,18,fill);
    label(r.x+16,r.y+17,labels[i],ink);
    const char *hint=active?(i==1?"网络指令，非物理急停":"抬手后发送一次"):"当前不可操作";
    if(i==0 && s.pumpOn)hint="泵正在运行";
    if(i==3 && s.printerKnown && !s.printerEnabled)hint="打印机尚未启用";
    label(r.x+16,r.y+52,hint,ink);
  }
  label(24,352,commandLabel(s),s.uncertain?kWarning:theme().ink);
  if(s.uncertain)pill({24,379,432,44},"已现场检查 · 解除操作锁定",theme().card,kWarning,true);
  else {clipped(24,380,s.commandError[0]?s.commandError:"软件完成不等于已出水或已出纸",432,theme().muted);clipped(24,407,s.requestId,432,theme().muted);}
  statusStrip(s,now);
}
void themePage(uint32_t now) {
  label(24,70,"今天想住进哪种世界？",theme().ink);
  for(uint8_t i=0;i<ui_theme::kCount;++i) {
    const int x=24+(i%2)*222,y=104+(i/2)*88;
    const auto &t=ui_theme::get(i);auto &g=indicator::display();
    g.fillRoundRect(x,y,210,78,16,t.card);
    if(i==model.theme()){g.drawRoundRect(x,y,210,78,16,theme().accent);g.drawRoundRect(x+1,y+1,208,76,15,theme().accent);}
    ui_theme::face(g,i,x+43,y+40,24,false,now);
    label(x+82,y+24,t.name,t.ink);if(i==model.theme())label(x+82,y+46,"正在使用",t.muted);
  }
}
void draw(const State &s,uint32_t now) {
  ui_theme::background(indicator::display(),model.theme());header();
  if(model.themesOpen())themePage(now);
  else if(controlsOpen)controlsPage(s,now);
  else switch(model.page()) {
    case ui::Page::Face:facePage(s,now);break;
    case ui::Page::Today:todayPage(s,now);break;
    case ui::Page::Info:infoPage(s,now);break;
    case ui::Page::Photos:photosPage(s,now);break;
    case ui::Page::Focus:focusPage(s,now);break;
  }
  indicator::present();
}
void handle(ui::Gesture gesture,const State &s,uint32_t now) {
  if(gesture==ui::Gesture::None)return;
  if(gesture!=ui::Gesture::Tap) {
    if(model.themesOpen())return;
    if(controlsOpen){controlsOpen=false;return;}
    model.handleGesture(gesture);return;
  }
  if(tapped(kTheme)) {if(model.themesOpen())model.closeThemes();else {controlsOpen=false;model.openThemes();}return;}
  if(tapped(kBack)) {
    if(model.themesOpen())model.closeThemes();else if(controlsOpen)controlsOpen=false;else model.previousPage();return;
  }
  if(model.themesOpen()) {
    for(uint8_t i=0;i<ui_theme::kCount;++i)if(tapped({int16_t(24+(i%2)*222),int16_t(104+(i/2)*88),210,78})) {
      if(model.selectTheme(i) && prefsReady)preferences.putUChar("theme",i);return;
    }return;
  }
  if(controlsOpen) {
    if(tapped(kDismiss)){controlsOpen=false;return;}
    if(s.uncertain && tapped({24,379,432,44})){planter::acknowledgeUncertain();return;}
    for(uint8_t i=0;i<4;++i)if(tapped(kHardware[i]) && enabled(kActions[i],s,now)) {planter::submit(kActions[i],!s.lightOn);return;}
    return;
  }
  switch(model.page()) {
    case ui::Page::Face:if(tapped({72,138,336,286}))model.tapFace();break;
    case ui::Page::Today:if(tapped(kControls))controlsOpen=true;break;
    case ui::Page::Info:break;
    case ui::Page::Photos:
      if(ui_photos::count()) {
        if(tapped(kPhotoPrevious))photoIndex=(photoIndex+ui_photos::count()-1)%ui_photos::count();
        else if(tapped(kPhotoNext))photoIndex=(photoIndex+1)%ui_photos::count();
      }break;
    case ui::Page::Focus:
      for(uint8_t i=0;i<3;++i)if(tapped(kTimerPresets[i])) {model.timer().selectPreset(static_cast<ui::Preset>(i));return;}
      if(tapped(kTimerToggle))model.timer().toggle(now);else if(tapped(kTimerReset))model.timer().reset();break;
  }
}
}

void setup() {
  Serial.begin(115200); // CH340 UART0 only; leave RP2040 UART19/20 untouched.
  Serial.printf("RoboPlanter Indicator boot: flash=%lu psram=%lu\n",
      static_cast<unsigned long>(ESP.getFlashChipSize()),
      static_cast<unsigned long>(ESP.getPsramSize()));
  prefsReady=preferences.begin("rp-ui",false);
  if(prefsReady)model.selectTheme(preferences.getUChar("theme",0)%ui_theme::kCount);
  displayReady=indicator::begin();
  if(!displayReady){Serial.println("Indicator LCD init failed: check OPI PSRAM / original ST7701 panel.");return;}
  clientReady=planter::begin();
  Serial.printf("Indicator init: display=%s touch=%s client_task=%s preferences=%s\n",
      displayReady?"ready":"failed", indicator::touchReady()?"ready":"not_found",
      clientReady?"ready":"failed", prefsReady?"ready":"unavailable");
  Serial.println("Indicator renderer: offscreen canvas -> complete frame copy.");
  Serial.println("RoboPlanter: release-only UI; decorative expressions; independent local focus timer.");
  draw(planter::snapshot(),millis());
}
void loop() {
  if(!displayReady){delay(100);return;}
  const uint32_t now=millis();model.timer().tick(now);
  int16_t x=0,y=0;
  const indicator::TouchStatus input=indicator::readTouchStatus(x,y);
  const ui::TouchInput mapped=input==indicator::TouchStatus::Down?ui::TouchInput::Down:input==indicator::TouchStatus::Up?ui::TouchInput::Up:ui::TouchInput::Invalid;
  const ui::Gesture gesture=touch.update(mapped,x,y,now);
  static uint32_t lastDraw=0;static bool first=true;
  const State state=planter::snapshot();
  if(gesture!=ui::Gesture::None)handle(gesture,state,now);
  // Only the decorative face needs animation frames. Data screens refresh at
  // the same 1 s cadence as the background HTTP poll; release events repaint now.
  const uint32_t interval=model.page()==ui::Page::Face && !model.themesOpen() && !controlsOpen?120:1000;
  if(first || gesture!=ui::Gesture::None || now-lastDraw>=interval) {
    draw(state,now);lastDraw=now;first=false;
  }
  delay(15);
}
