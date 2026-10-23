#include "ui_theme.h"
#include <math.h>

namespace ui_theme {
namespace {
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
const Theme themes[kCount] = {
    {"森林手绘", rgb(244,241,222), rgb(255,251,237), rgb(49,74,51), rgb(85,101,73), rgb(94,137,83), rgb(222,175,106), rgb(187,211,138), rgb(59,92,59)},
    {"漫画日常", rgb(255,239,91), rgb(255,255,244), rgb(31,32,41), rgb(99,94,96), rgb(247,99,112), rgb(79,158,211), rgb(255,248,237), rgb(31,32,41)},
    {"清新薄荷", rgb(226,247,243), rgb(249,255,251), rgb(49,103,99), rgb(62,108,104), rgb(82,164,143), rgb(246,172,169), rgb(195,228,212), rgb(75,139,119)},
    {"海底假日", rgb(213,240,241), rgb(227,249,237), rgb(16,74,100), rgb(37,94,112), rgb(249,179,67), rgb(255,114,101), rgb(250,197,83), rgb(116,76,39)},
    {"怪诞宇宙", rgb(34,27,57), rgb(59,44,80), rgb(245,230,220), rgb(169,151,184), rgb(184,237,121), rgb(244,132,170), rgb(153,114,190), rgb(229,209,255)},
    {"剪纸小镇", rgb(247,222,181), rgb(255,244,216), rgb(93,63,63), rgb(121,80,67), rgb(195,87,75), rgb(71,139,141), rgb(224,125,85), rgb(117,69,65)},
    {"自然山野", rgb(229,235,211), rgb(249,251,234), rgb(45,83,77), rgb(75,104,83), rgb(78,129,109), rgb(205,151,76), rgb(178,203,154), rgb(61,109,88)},
    {"赛博机甲", rgb(10,19,32), rgb(21,38,54), rgb(209,250,243), rgb(96,151,159), rgb(79,232,215), rgb(245,141,191), rgb(36,68,87), rgb(106,243,226)},
};
void stroke(Arduino_GFX &g, int x0, int y0, int x1, int y1, uint16_t c, int n) {
  for (int i=0; i<n; ++i) g.drawLine(x0, y0+i, x1, y1+i, c);
}
void leaf(Arduino_GFX &g, int x, int y, int size, uint16_t c) {
  g.fillTriangle(x-size, y, x+size, y-size, x+size/2, y+size, c);
  g.drawLine(x-size, y+size, x+size, y-size, c);
}
void star(Arduino_GFX &g, int x, int y, int s, uint16_t c) {
  g.fillTriangle(x,y-s,x-s/3,y+s/3,x+s/3,y+s/3,c);
  g.fillTriangle(x-s,y,x+s,y,x,y+s,c);
}
}
const Theme &get(uint8_t id) { return themes[id % kCount]; }
void background(Arduino_GFX &g, uint8_t id) {
  const Theme &t=get(id);
  g.fillScreen(t.background);
  switch(id % kCount) {
    case 0:
      for(int i=0;i<7;++i) { leaf(g, 14+i*77, 462-(i%2)*12, 14, t.second); leaf(g, 8, 160+i*39, 9, t.accent); }
      break;
    case 1:
      for(int y=110;y<460;y+=22) for(int x=7;x<480;x+=22) if(x<22 || x>458)g.fillCircle(x,y,2,t.ink);
      stroke(g,0,100,39,80,t.ink,3); stroke(g,443,448,480,419,t.ink,3);
      break;
    case 2:
      g.drawCircle(461,146,46,t.second); g.drawCircle(461,146,38,t.second);
      g.fillCircle(14,415,27,t.face); g.drawCircle(33,436,23,t.accent); break;
    case 3:
      for(int i=0;i<9;++i) { int x=12+(i*73)%460,y=113+(i*53)%332;g.drawCircle(x,y,5+i%4,t.card); }
      for(int x=0;x<480;x+=60)g.drawCircle(x,482,35,t.accent); break;
    case 4:
      for(int i=0;i<17;++i)star(g,8+(i*79)%466,103+(i*53)%359,3+i%3,t.second);
      g.drawCircle(457,434,28,t.accent); g.drawEllipse(457,434,42,11,t.accent); break;
    case 5:
      for(int i=0;i<9;++i) { int x=i*61-12;g.fillRect(x,448,45,32,t.second);g.fillTriangle(x-4,448,x+22,429,x+49,448,t.accent); }
      break;
    case 6:
      g.fillTriangle(0,480,112,374,244,480,t.face);g.fillTriangle(152,480,344,369,480,480,t.second);
      g.fillTriangle(290,420,344,369,389,414,t.card); break;
    case 7:
      for(int x=0;x<480;x+=32) {g.drawFastVLine(x,108,372,t.card);g.drawFastHLine(0,x+112,480,t.card);}
      stroke(g,0,447,37,447,t.accent,2);stroke(g,37,447,51,433,t.accent,2);stroke(g,429,108,444,123,t.second,2);break;
  }
}
void face(Arduino_GFX &g,uint8_t id,int16_t cx,int16_t cy,int16_t r,bool blink,uint32_t now) {
  const Theme &t=get(id); const int line=r>50?3:1;
  const int y=cy+static_cast<int>(sinf(now*0.0015f)* (r>50?3:0));
  switch(id % kCount) {
    case 0: // Asymmetric hand-drawn forest sprite with leafy hair.
      g.fillCircle(cx-r/2,y-r/2,r/2,t.face);g.fillCircle(cx+r/2,y-r/2,r/2,t.face);
      g.fillRoundRect(cx-r,y-r/2,r*2,r*3/2,r/2,t.face);
      g.drawRoundRect(cx-r,y-r/2,r*2,r*3/2,r/2,t.outline);
      leaf(g,cx-r/8,y-r+r/10,r/3,t.accent);leaf(g,cx+r/4,y-r+r/7,r/4,t.second);
      stroke(g,cx-r/2,y+r*3/4,cx-r/4,y+r*4/5,t.outline,line);break;
    case 1: // Bold comic cloud; heavy contour and motion rays.
      for(int i=0;i<6;++i) {float a=i*1.0472f;int x=cx+cosf(a)*r/2,yy=y+sinf(a)*r/2;g.fillCircle(x,yy,r*3/5+line,t.outline);}
      for(int i=0;i<6;++i) {float a=i*1.0472f;g.fillCircle(cx+cosf(a)*r/2,y+sinf(a)*r/2,r*3/5,t.face);}
      g.fillCircle(cx,y,r*3/4,t.face);
      stroke(g,cx-r-16,y-r/2,cx-r-28,y-r*3/4,t.outline,line);
      stroke(g,cx+r+15,y-r/2,cx+r+27,y-r*3/4,t.outline,line);break;
    case 2: // Soft seed-shaped friend with a little flower.
      g.fillEllipse(cx,y,r*4/5,r,t.face);g.drawEllipse(cx,y,r*4/5,r,t.outline);
      for(int i=0;i<5;++i){float a=i*1.2566f;g.fillCircle(cx+r/2+cosf(a)*r/7,y-r*4/5+sinf(a)*r/7,r/8,t.second);}
      g.fillCircle(cx+r/2,y-r*4/5,r/10,t.card);break;
    case 3: // Original round pufferfish, not a licensed character.
      g.fillTriangle(cx-r/2,y-r/3,cx-r*5/4,y-r/2,cx-r,y+r/3,t.second);
      g.fillTriangle(cx+r/2,y-r/3,cx+r*5/4,y-r/2,cx+r,y+r/3,t.second);
      g.fillCircle(cx,y,r,t.face);g.drawCircle(cx,y,r,t.outline);
      for(int i=0;i<8;++i){float a=i*0.7854f;g.fillCircle(cx+cosf(a)*r*4/5,y+sinf(a)*r*4/5,r/15,t.second);}
      g.fillTriangle(cx-r/4,y-r+4,cx,y-r*6/5,cx+r/4,y-r+4,t.accent);break;
    case 4: // Odd space creature with antenna, asymmetric ears and third eye.
      g.fillTriangle(cx-r,y-r/3,cx-r*6/5,y-r,cx-r/3,y-r*3/4,t.face);
      g.fillTriangle(cx+r,y-r/3,cx+r*5/4,y-r/2,cx+r/3,y-r*3/4,t.face);
      g.fillEllipse(cx,y,r,r*4/5,t.face);g.drawEllipse(cx,y,r,r*4/5,t.outline);
      stroke(g,cx,y-r*3/4,cx+r/5,y-r*6/5,t.outline,line);g.fillCircle(cx+r/5,y-r*6/5,r/9,t.accent);
      g.fillCircle(cx,y-r/2,r/7,t.card);g.fillCircle(cx,y-r/2,r/14,t.outline);break;
    case 5: // Cut-paper fox with folded geometric panels.
      g.fillTriangle(cx-r,y-r,cx-r/5,y-r/2,cx-r*4/5,y+r/3,t.outline);
      g.fillTriangle(cx+r,y-r,cx+r/5,y-r/2,cx+r*4/5,y+r/3,t.outline);
      g.fillTriangle(cx-r,y-r/2,cx+r,y-r/2,cx,y+r,t.face);
      g.fillTriangle(cx-r,y-r/2,cx,y+r,cx-r/10,y+r/5,t.card);
      g.fillTriangle(cx+r,y-r/2,cx,y+r,cx+r/10,y+r/5,t.card);
      g.fillTriangle(cx-r/9,y+r/3,cx+r/9,y+r/3,cx,y+r/2,t.outline);break;
    case 6: // Mountain moss spirit with a snowy silhouette.
      g.fillTriangle(cx-r,y+r*4/5,cx-r/4,y-r,cx+r,y+r*4/5,t.face);
      g.fillTriangle(cx-r/3,y-r*3/4,cx-r/4,y-r,cx,y-r/2,t.card);
      g.fillRoundRect(cx-r*3/4,y-r/5,r*3/2,r, r/3,t.face);
      leaf(g,cx+r*3/4,y-r/5,r/3,t.accent);break;
    case 7: // Mecha visor, angular shell, vents and antenna.
      g.fillRoundRect(cx-r,y-r*3/4,r*2,r*3/2,r/5,t.outline);
      g.fillRoundRect(cx-r+line,y-r*3/4+line,r*2-line*2,r*3/2-line*2,r/5,t.face);
      g.fillRect(cx-r*4/5,y-r/3,r*8/5,r*2/3,t.background);
      g.fillRect(cx-r*6/5,y-r/3,r/5,r*2/3,t.accent);g.fillRect(cx+r,y-r/3,r/5,r*2/3,t.second);
      stroke(g,cx-r/2,y-r*3/4,cx-r/2,y-r,t.outline,line);g.fillCircle(cx-r/2,y-r,r/12,t.second);
      for(int i=-1;i<=1;++i)g.fillRect(cx+i*r/5-r/16,y+r/2,r/8,line+1,t.accent);break;
  }
  const int ex=r*2/5, ey=y-r/12, ew=r/10>1?r/10:2, eh=r/5>2?r/5:3;
  const uint16_t eye=id==7?t.accent:t.outline;
  if(blink) {stroke(g,cx-ex-ew,ey,cx-ex+ew,ey,eye,line+1);stroke(g,cx+ex-ew,ey,cx+ex+ew,ey,eye,line+1);}
  else {
    g.fillRoundRect(cx-ex-ew,ey-eh/2,ew*2,eh,ew,eye);g.fillRoundRect(cx+ex-ew,ey-eh/2,ew*2,eh,ew,eye);
    if(r>50 && id!=7){g.fillCircle(cx-ex+ew/4,ey-eh/5,ew/3,t.card);g.fillCircle(cx+ex+ew/4,ey-eh/5,ew/3,t.card);}
  }
  if(id!=5 && id!=7) {stroke(g,cx-r/7,y+r/3,cx,y+r*2/5,eye,line);stroke(g,cx,y+r*2/5,cx+r/7,y+r/3,eye,line);}
  if(id!=7 && id!=5) {g.fillEllipse(cx-r*3/5,y+r/5,r/8,r/18,t.second);g.fillEllipse(cx+r*3/5,y+r/5,r/8,r/18,t.second);}
}
}
