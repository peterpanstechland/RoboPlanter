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
using ui_theme::Icon;
bool displayReady=false, clientReady=false, controlsOpen=false, prefsReady=false;
bool confirmOpen=false;
planter::Action confirmAction=planter::Action::Water;
ui::Model model;
ui::TouchGate touch;
Preferences preferences;
size_t photoIndex=0;
constexpr uint16_t kWarning=0xF60C, kDanger=0xEAE9, kAqua=0xA6FD;
struct Rect { int16_t x,y,w,h; };
constexpr Rect kBack{16,12,44,44}, kTheme{354,12,110,44};
constexpr Rect kControls{24,374,432,52}, kDismiss{368,76,88,40};
constexpr Rect kHardware[]={{24,143,210,86},{246,143,210,86},{24,241,210,86},{246,241,210,86}};
constexpr planter::Action kActions[]={planter::Action::Water,planter::Action::StopWater,planter::Action::Light,planter::Action::Print};
constexpr Rect kConfirmCancel{24,374,142,52}, kConfirmGo{178,374,278,52};
constexpr Rect kTimerPresets[]={{24,314,136,44},{172,314,136,44},{320,314,136,44}};
constexpr Rect kTimerToggle{24,374,278,52}, kTimerReset{316,374,140,52};
constexpr Rect kPhotoPrevious{24,391,64,44}, kPhotoNext{392,391,64,44};
const ui_theme::Theme &theme() { return ui_theme::get(model.theme()); }
uint16_t contrastInk(uint16_t color) {
  auto linear=[](float c){return c<=0.04045f?c/12.92f:powf((c+0.055f)/1.055f,2.4f);};
  const float l=0.2126f*linear(((color>>11)&31)/31.0f)+0.7152f*linear(((color>>5)&63)/63.0f)+0.0722f*linear((color&31)/31.0f);
  return l>0.179f?0x1082:0xFFFF;
}
bool inside(const Rect &r,int32_t x,int32_t y) {return x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h;}
bool tapped(const Rect &r) {return inside(r,touch.startX(),touch.startY()) && inside(r,touch.lastX(),touch.lastY());}
void label(int16_t x,int16_t y,const char *s,uint16_t color,uint8_t scale=1) {ui_text::draw(indicator::display(),x,y,s,color,scale);}
void centered(int16_t y,const char *s,uint16_t color,uint8_t scale=1) {label((480-ui_text::width(s,scale))/2,y,s,color,scale);}
void clipped(int16_t x,int16_t y,const char *s,int16_t maxWidth,uint16_t color) {
  char out[96]={};size_t n=0,i=0;
  while(s[i] && n<sizeof(out)-5) {
    const uint8_t lead=static_cast<uint8_t>(s[i]);
    size_t bytes=lead<0x80?1:(lead>=0xC2 && lead<=0xDF)?2:(lead>=0xE0 && lead<=0xEF)?3:(lead>=0xF0 && lead<=0xF4)?4:0;
    bool valid=bytes!=0;
    for(size_t j=1;valid && j<bytes;++j) {const uint8_t c=static_cast<uint8_t>(s[i+j]);if(c==0 || (c&0xC0)!=0x80)valid=false;}
    if(!valid){bytes=1;out[n]='?';}else memcpy(out+n,s+i,bytes);
    out[n+bytes]=0;
    if(ui_text::width(out)>maxWidth-14) {out[n]=0;strcat(out,"..");break;}
    n+=bytes;i+=bytes;
  }
  label(x,y,out,color);
}
void icon(Icon i,int x,int y,int size,uint16_t color) {ui_theme::icon(indicator::display(),i,x,y,size,color);}
void pill(const Rect &r,const char *s,uint16_t fill,uint16_t ink,bool outline=false) {
  auto &g=indicator::display();
  g.fillRoundRect(r.x,r.y+3,r.w,r.h,10,theme().background);
  g.fillRoundRect(r.x,r.y,r.w,r.h,10,fill);
  if(outline)g.drawRoundRect(r.x,r.y,r.w,r.h,10,theme().muted);
  label(r.x+(r.w-ui_text::width(s))/2,r.y+(r.h-16)/2,s,ink);
}
const char *connection(const State &s,uint32_t now) {
  if(!clientReady)return "连接未就绪";
  if(!s.received)return s.wifi?"等待花盆回应":"等待花盆连接";
  if(!s.wifi)return "离线 · 上次读数";
  if(!planter::fresh(s,now))return "读数已过期";
  return "花盆已连接";
}
void statusStrip(const State &s,uint32_t now) {
  auto &g=indicator::display();
  g.drawFastHLine(24,448,432,theme().surface);
  g.fillCircle(29,463,3,planter::fresh(s,now)?theme().accent:kWarning);
  label(40,455,indicator::touchReady()?connection(s,now):"触摸未就绪",theme().muted);
  const uint8_t page=static_cast<uint8_t>(model.page());
  for(uint8_t i=0;i<5;++i)g.fillRoundRect(367+i*18,460,i==page?12:6,5,2,i==page?theme().accent:theme().surface);
}
void header(const State &s) {
  auto &g=indicator::display();
  if(model.page()!=ui::Page::Face || controlsOpen || model.themesOpen()) {
    g.fillRoundRect(kBack.x,kBack.y,kBack.w,kBack.h,10,theme().surface);
    icon(Icon::Back,28,24,20,theme().ink);
    label(74,25,"WORKER / PLANTER",theme().muted);
  } else {
    icon(Icon::Leaf,24,20,28,theme().accent);
    label(62,15,"WORKER",theme().accent);
    label(62,33,"SMART PLANTER",theme().ink);
  }
  icon(Icon::Wifi,318,24,22,s.wifi?theme().ink:theme().muted);
  pill(kTheme,model.themesOpen()?"收起主题":"换个心情",theme().surface,theme().ink);
}
void title(const char *s,const char *subtitle) {
  label(24,76,s,theme().ink,2);
  if(subtitle)label(25,115,subtitle,theme().muted);
}
planter::Reading soilReading(const State &s) {
  auto reading=s.soilPercent;
  if(!s.soilCalibrationKnown || !s.soilCalibrated)reading.valid=false;
  return reading;
}
void miniReading(int x,Icon kind,const planter::Reading &r,const char *unit,bool fresh) {
  icon(kind,x,417,22,kind==Icon::Temp?theme().second:kAqua);
  char value[24];if(r.valid)snprintf(value,sizeof(value),"%.0f%s%s",r.value,unit,fresh?"":"*");else strlcpy(value,"--",sizeof(value));
  label(x+32,420,value,r.valid&&fresh?theme().ink:theme().muted);
}
void facePage(const State &s,uint32_t now) {
  label(25,80,"工位小绿洲",theme().ink,2);
  label(26,120,"A GREENER WORKDAY.",theme().muted);
  auto &g=indicator::display();
  g.fillRoundRect(348,80,108,28,7,theme().surface);
  label(370,86,"陪伴模式",theme().accent);
  ui_theme::deskScene(g,model.theme(),now);
  const uint32_t phase=now%5300U;
  ui_theme::face(g,model.theme(),240,252,104,phase<160 || (phase>350 && phase<460),now);
  pill({104,360,272,38},"Still growing.",theme().card,theme().cardInk);
  miniReading(38,Icon::Water,soilReading(s),"%",planter::fresh(s,now));
  miniReading(188,Icon::Temp,s.temperature,"C",planter::fresh(s,now));
  miniReading(334,Icon::Humidity,s.humidity,"%",planter::fresh(s,now));
  if(s.received && !planter::fresh(s,now))label(352,391,"* 旧读数",theme().muted);
  statusStrip(s,now);
}
void readingCard(const Rect &r,Icon kind,const char *name,const planter::Reading &reading,const char *unit,bool fresh,const char *invalid=nullptr,bool percent=false) {
  auto &g=indicator::display();
  g.fillRoundRect(r.x,r.y,r.w,r.h,12,theme().card);
  icon(kind,r.x+14,r.y+33,30,kind==Icon::Temp?0xC30C:0x4B94);
  label(r.x+54,r.y+12,name,theme().cardMuted);
  char value[28];if(reading.valid)snprintf(value,sizeof(value),"%.1f%s",reading.value,unit);else strlcpy(value,"--",sizeof(value));
  label(r.x+54,r.y+35,value,reading.valid&&fresh?theme().cardInk:theme().cardMuted,2);
  if(percent) {
    g.fillRoundRect(r.x+54,r.y+73,r.w-70,5,2,0xD6B9);
    if(reading.valid) {
      const float v=fminf(100.0f,fmaxf(0.0f,reading.value));
      const int amount=static_cast<int>((r.w-70)*v/100.0f);
      if(amount>0)g.fillRoundRect(r.x+54,r.y+73,amount,5,2,fresh?theme().accent:theme().cardMuted);
    }
  }
  const char *hint=!reading.valid?(invalid?invalid:"等待传感器"):!fresh?"上次读数 · 已过期":"有效读数";
  label(r.x+14,r.y+r.h-22,hint,theme().cardMuted);
}
void todayPage(const State &s,uint32_t now) {
  title("Plant status","今日状态 · 慢慢照顾，也照顾自己");
  const bool fresh=planter::fresh(s,now);
  readingCard({24,145,210,106},Icon::Water,"土壤湿度",soilReading(s),"%",fresh,!s.soilCalibrationKnown?"等待土壤数据":!s.soilCalibrated?"传感器待校准":"传感器暂不可用",true);
  readingCard({246,145,210,106},Icon::Temp,"空气温度",s.temperature,"C",fresh);
  readingCard({24,261,210,106},Icon::Humidity,"空气湿度",s.humidity,"%",fresh,nullptr,true);
  auto &g=indicator::display();g.fillRoundRect(246,261,210,106,12,theme().card);
  label(260,273,"设备动态",theme().cardMuted);
  icon(Icon::Water,260,301,20,0x4B94);
  label(289,302,!s.pumpKnown?"泵 · 未知":!s.pumpEnabled?"泵 · 未启用":s.pumpOn?"泵 · 运行中":"泵 · 已停止",theme().cardInk);
  icon(Icon::Light,260,333,20,0x9B83);
  label(289,334,!s.lightKnown?"灯 · 未知":!s.lightEnabled?"灯 · 未启用":s.lightOn?"灯 · 已开启":"灯 · 已关闭",theme().cardInk);
  if(s.received&&!fresh)label(374,273,"旧状态",theme().cardMuted);
  pill(kControls,"照顾小盆栽    >",theme().accent,contrastInk(theme().accent));
  statusStrip(s,now);
}
void infoRow(int y,Icon symbol,const char *name,const char *value) {
  icon(symbol,39,y-2,20,theme().cardInk);
  label(72,y,name,theme().cardMuted);clipped(213,y,value,222,theme().cardInk);
}
void infoPage(const State &s,uint32_t now) {
  title("Desk notes","工位情报 · 来自花盆的消息");
  auto &g=indicator::display();g.fillRoundRect(24,145,432,191,12,theme().card);
  const bool fresh=planter::fresh(s,now);char buffer[80];
  infoRow(161,Icon::Wifi,"连接",connection(s,now));
  g.drawFastHLine(40,190,400,0xD6B9);
  if(s.received&&s.ageKnown)snprintf(buffer,sizeof(buffer),"%lus",static_cast<unsigned long>((static_cast<uint64_t>(now-s.receivedMs)+s.sampleAgeMs)/1000));else strlcpy(buffer,"未知",sizeof(buffer));
  infoRow(202,Icon::Clock,"采样距今",buffer);
  if(s.airRaw.valid)snprintf(buffer,sizeof(buffer),"%.0f raw%s",s.airRaw.value,fresh?"":" / 旧值");else strlcpy(buffer,"暂无有效值",sizeof(buffer));
  infoRow(244,Icon::Leaf,"空气原始值",buffer);
  if(s.uvRaw.valid)snprintf(buffer,sizeof(buffer),"%.0f raw%s",s.uvRaw.value,fresh?"":" / 旧值");else strlcpy(buffer,"暂无有效值",sizeof(buffer));
  infoRow(286,Icon::Light,"紫外原始值",buffer);
  g.fillRoundRect(24,348,432,78,12,theme().surface);
  icon(Icon::Yoyo,40,365,34,theme().second);
  label(88,359,"YOYO 在手机上陪你",theme().ink);
  label(88,386,"原始数值不等于空气质量或光照",theme().muted);
  statusStrip(s,now);
}
void photosPage(const State &s,uint32_t now) {
  label(24,76,"Little escapes",theme().ink,2);
  const size_t count=ui_photos::count();auto &g=indicator::display();
  if(count) {
    photoIndex%=count;
    if(!ui_photos::draw(g,photoIndex,40,110)) {g.fillRoundRect(40,110,400,280,12,theme().surface);centered(225,"这张照片暂时无法显示",theme().muted);}
    pill(kPhotoPrevious,"<",theme().card,theme().cardInk);pill(kPhotoNext,">",theme().card,theme().cardInk);
    const char *caption=ui_photos::caption(photoIndex);clipped(104,397,caption&&caption[0]?caption:"喜欢的瞬间",270,theme().ink);
    char countLabel[20];snprintf(countLabel,sizeof(countLabel),"%u / %u",unsigned(photoIndex+1),unsigned(count));centered(420,countLabel,theme().muted);
  } else {
    g.fillRoundRect(40,126,400,260,8,theme().card);
    g.fillRect(56,142,368,180,theme().surface);
    g.fillRect(56,286,368,36,theme().accent);
    for(int i=0;i<12;++i)g.fillRect(56+i*32,265-(i%3)*12,i==11?16:32,57+(i%3)*12,theme().accent);
    g.fillRect(325,171,32,32,theme().second);g.fillRect(317,179,48,16,theme().second);
    ui_theme::face(g,model.theme(),224,250,60,false,now);
    centered(343,"还没有照片 · 放一张喜欢的风景",theme().cardInk);
    centered(408,"左滑，留一段时间给自己",theme().muted);
  }
  statusStrip(s,now);
}
void focusPage(const State &s,uint32_t now) {
  auto &timer=model.timer();auto &g=indicator::display();
  title("Time to focus",timer.done()?"这一段结束啦，伸个懒腰吧":timer.running()?"正在专注 · 换时长请先重置":timer.phase()==ui::TimerPhase::Paused?"暂停一下 · 换时长请先重置":"专注与休息，都值得认真对待");
  g.fillRoundRect(24,145,432,153,12,theme().card);
  icon(Icon::Clock,43,164,22,theme().cardMuted);
  label(76,166,timer.running()?"FOCUS IN PROGRESS":timer.done()?"WELL DONE":timer.phase()==ui::TimerPhase::Paused?"TAKE YOUR TIME":"ONE THING AT A TIME",theme().cardMuted);
  const uint32_t seconds=(timer.remainingMs(now)+999)/1000;char clock[12];
  snprintf(clock,sizeof(clock),"%02lu:%02lu",static_cast<unsigned long>(seconds/60),static_cast<unsigned long>(seconds%60));
  label(46,198,clock,theme().cardInk,4);
  ui_theme::face(g,model.theme(),385,226,43,(now%5300)<160,now);
  const int progress=timer.durationMs()?static_cast<uint64_t>(timer.durationMs()-timer.remainingMs(now))*392/timer.durationMs():0;
  g.fillRoundRect(44,278,392,6,3,0xD6B9);if(progress>0)g.fillRoundRect(44,278,progress,6,3,theme().accent);
  const char *presets[]={"专注 25 分","短歇 5 分","长歇 15 分"};
  const bool canChange=timer.phase()==ui::TimerPhase::Ready || timer.done();
  for(uint8_t i=0;i<3;++i) {
    const bool selected=static_cast<uint8_t>(timer.preset())==i;
    pill(kTimerPresets[i],presets[i],selected?theme().second:theme().surface,selected?contrastInk(theme().second):canChange?theme().ink:theme().muted);
  }
  pill(kTimerToggle,timer.running()?"暂停一下":timer.phase()==ui::TimerPhase::Paused?"继续计时":timer.done()?"再来一段":"开始计时",theme().accent,contrastInk(theme().accent));
  pill(kTimerReset,"重置",theme().card,theme().cardInk);
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
  return s.commandStatus[0]?s.commandStatus:"照顾植物，也给自己一点停顿";
}
void confirmPage(const State &s,uint32_t now) {
  auto &g=indicator::display();const bool water=confirmAction==planter::Action::Water;
  title(water?"Water plant?":"Print a note",water?"浇水前，先看看土壤和接水盘":"打印预览 · 给今天留一张小纸条");
  if(water) {
    ui_theme::face(g,model.theme(),132,229,76,(now%5300)<160,now);
    g.fillRoundRect(244,145,212,156,12,theme().card);
    icon(Icon::Water,260,161,26,0x4B94);label(298,168,"手动浇水",theme().cardMuted);
    char duration[24];snprintf(duration,sizeof(duration),"%.1f s",PLANTER_WATER_DURATION_MS/1000.0f);
    label(264,210,duration,theme().cardInk,3);
    label(260,270,"按时长控制，不估算水量",theme().cardMuted);
    centered(326,"确认后发送一次浇水指令",theme().ink);
  } else {
    g.fillRect(54,148,372,192,theme().card);
    for(int x=54;x<426;x+=12) {g.fillTriangle(x,148,x+6,142,x+11,148,theme().card);g.fillTriangle(x,340,x+6,346,x+11,340,theme().card);}
    icon(Icon::Print,76,168,26,theme().cardInk);label(119,174,"RoboPlanter",theme().cardInk,2);
    for(int x=75;x<406;x+=12)g.drawFastHLine(x,212,6,theme().cardMuted);
    label(76,232,"Manual garden check-in.",theme().cardInk);
    label(76,265,"Observe your plant",theme().cardInk);
    label(76,288,"before watering.",theme().cardInk);
    centered(352,"小纸条，留给忙碌的自己",theme().muted);
  }
  pill(kConfirmCancel,"取消",theme().card,theme().cardInk);
  const bool active=enabled(confirmAction,s,now);
  pill(kConfirmGo,active?(water?"确认浇水":"打印这张便签"):"当前不可操作",active?theme().second:theme().surface,active?contrastInk(theme().second):theme().muted);
  statusStrip(s,now);
}
void controlsPage(const State &s,uint32_t now) {
  if(confirmOpen){confirmPage(s,now);return;}
  label(24,76,"Plant care",theme().ink,2);pill(kDismiss,"收起",theme().surface,theme().ink);
  label(24,115,"一点水，一点光，一点工作间隙",theme().muted);
  char waterLabel[32];snprintf(waterLabel,sizeof(waterLabel),"浇水 %.1f 秒",PLANTER_WATER_DURATION_MS/1000.0);
  const char *labels[]={waterLabel,"立即停泵",s.lightOn?"关闭灯光":"开启灯光","打印便签"};
  const Icon symbols[]={Icon::Water,Icon::Water,Icon::Light,Icon::Print};
  auto &g=indicator::display();
  for(uint8_t i=0;i<4;++i) {
    const bool active=enabled(kActions[i],s,now);const Rect &r=kHardware[i];
    const uint16_t fill=active?(i==1?kDanger:i==3?theme().second:theme().accent):theme().card;
    const uint16_t ink=active?contrastInk(fill):theme().cardMuted;
    g.fillRoundRect(r.x,r.y,r.w,r.h,12,fill);
    icon(symbols[i],r.x+14,r.y+15,24,ink);label(r.x+50,r.y+19,labels[i],ink);
    const char *hint=active?(i==1?"网络停泵，非物理急停":i==0||i==3?"点击查看确认页":"抬手后发送一次"):"当前不可操作";
    if(i==0&&s.pumpOn)hint=planter::fresh(s,now)&&s.pumpKnown?"泵正在运行":"上次状态：泵运行";
    if(i==3&&s.printerKnown&&!s.printerEnabled)hint="打印机尚未启用";
    label(r.x+14,r.y+55,hint,ink);
  }
  g.fillRoundRect(24,340,432,86,12,theme().surface);
  label(38,352,commandLabel(s),s.uncertain?kWarning:theme().ink);
  if(s.uncertain)pill({24,379,432,44},"已现场检查 · 解除操作锁定",theme().card,theme().cardInk,true);
  else {
    clipped(38,383,s.commandError[0]?s.commandError:"软件完成不等于已出水或已出纸",402,theme().muted);
    if(s.pumpKnown&&s.pumpOn&&planter::fresh(s,now)) {
      // Decorative motion only; no guessed amount, delivery or completion rate.
      for(int i=0;i<7;++i)g.fillRect(40+i*56,412,30,4,((now/120+i)%7)<3?kAqua:theme().muted);
    }
  }
  statusStrip(s,now);
}
void themePage(uint32_t now) {
  label(24,74,"为工位换个心情",theme().ink,2);
  auto &g=indicator::display();
  for(uint8_t i=0;i<ui_theme::kCount;++i) {
    const int x=24+(i%2)*222,y=119+(i/2)*82;const auto &t=ui_theme::get(i);
    g.fillRoundRect(x,y,210,74,10,t.surface);
    if(i==model.theme()){g.drawRoundRect(x,y,210,74,10,theme().accent);g.drawRoundRect(x+1,y+1,208,72,9,theme().accent);}
    ui_theme::face(g,i,x+39,y+38,31,false,now);
    label(x+82,y+16,t.name,t.ink);
    g.fillRect(x+82,y+44,16,7,t.accent);g.fillRect(x+103,y+44,16,7,t.second);g.fillRect(x+124,y+44,16,7,t.card);
    if(i==model.theme())label(x+153,y+41,"当前",t.muted);
  }
}
void draw(const State &s,uint32_t now) {
  ui_theme::background(indicator::display(),model.theme());header(s);
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
    if(confirmOpen){confirmOpen=false;return;}
    if(controlsOpen){controlsOpen=false;return;}
    model.handleGesture(gesture);return;
  }
  if(tapped(kTheme)) {confirmOpen=false;if(model.themesOpen())model.closeThemes();else {controlsOpen=false;model.openThemes();}return;}
  if(tapped(kBack)) {
    if(model.themesOpen())model.closeThemes();else if(confirmOpen)confirmOpen=false;else if(controlsOpen)controlsOpen=false;else model.previousPage();return;
  }
  if(model.themesOpen()) {
    for(uint8_t i=0;i<ui_theme::kCount;++i)if(tapped({int16_t(24+(i%2)*222),int16_t(119+(i/2)*82),210,74})) {
      if(model.selectTheme(i)&&prefsReady)preferences.putUChar("theme",i);return;
    }return;
  }
  if(controlsOpen) {
    if(confirmOpen) {
      if(tapped(kConfirmCancel))confirmOpen=false;
      else if(tapped(kConfirmGo)&&enabled(confirmAction,s,now)) {
        if(planter::submit(confirmAction))confirmOpen=false;
      }
      return;
    }
    if(tapped(kDismiss)){controlsOpen=false;return;}
    if(s.uncertain&&tapped({24,379,432,44})){planter::acknowledgeUncertain();return;}
    for(uint8_t i=0;i<4;++i)if(tapped(kHardware[i])&&enabled(kActions[i],s,now)) {
      if(kActions[i]==planter::Action::Water||kActions[i]==planter::Action::Print){confirmAction=kActions[i];confirmOpen=true;}
      else planter::submit(kActions[i],!s.lightOn);
      return;
    }
    return;
  }
  switch(model.page()) {
    case ui::Page::Face:if(tapped({72,138,336,266}))model.tapFace();break;
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
  Serial.begin(115200);
  Serial.printf("RoboPlanter Indicator boot: flash=%lu psram=%lu\n",static_cast<unsigned long>(ESP.getFlashChipSize()),static_cast<unsigned long>(ESP.getPsramSize()));
  prefsReady=preferences.begin("rp-ui",false);
  if(prefsReady)model.selectTheme(preferences.getUChar("theme",0)%ui_theme::kCount);
  displayReady=indicator::begin();
  if(!displayReady){Serial.println("Indicator LCD init failed: check OPI PSRAM / original ST7701 panel.");return;}
  clientReady=planter::begin();
  Serial.printf("Indicator init: display=%s touch=%s client_task=%s preferences=%s\n",displayReady?"ready":"failed",indicator::touchReady()?"ready":"not_found",clientReady?"ready":"failed",prefsReady?"ready":"unavailable");
  Serial.println("Indicator renderer: offscreen canvas -> complete frame copy.");
  Serial.println("Indicator UI: Worker Smart Planter / pixel companion / animated.");
  draw(planter::snapshot(),millis());
}
void loop() {
  if(!displayReady){delay(100);return;}
  const uint32_t now=millis();model.timer().tick(now);
  int16_t x=0,y=0;const auto input=indicator::readTouchStatus(x,y);
  const ui::TouchInput mapped=input==indicator::TouchStatus::Down?ui::TouchInput::Down:input==indicator::TouchStatus::Up?ui::TouchInput::Up:ui::TouchInput::Invalid;
  const ui::Gesture gesture=touch.update(mapped,x,y,now);
  static uint32_t lastDraw=0;static bool first=true;const State state=planter::snapshot();
  if(gesture!=ui::Gesture::None)handle(gesture,state,now);
  const bool animated=!model.themesOpen() &&
      ((!controlsOpen && (model.page()==ui::Page::Face || model.page()==ui::Page::Focus)) ||
       (controlsOpen && (confirmOpen || (state.pumpKnown && state.pumpOn && planter::fresh(state,now)))));
  const uint32_t interval=animated?100:1000;
  if(first||gesture!=ui::Gesture::None||now-lastDraw>=interval){draw(state,now);lastDraw=millis();first=false;}
  delay(15);
}
