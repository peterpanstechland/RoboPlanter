#include "ui_theme.h"

namespace ui_theme {
namespace {
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
constexpr uint16_t kIvory = rgb(246,244,235);
constexpr uint16_t kCharcoal = rgb(30,31,34);
constexpr uint16_t kSage = rgb(124,169,130);
constexpr uint16_t kPurple = rgb(183,167,243);
constexpr uint16_t kAqua = rgb(167,221,234);
constexpr uint16_t kCardInk = rgb(35,39,38);
constexpr uint16_t kCardMuted = rgb(95,105,99);
constexpr uint16_t kOutline = rgb(15,19,19);
constexpr uint16_t kScreen = rgb(18,24,22);
constexpr uint16_t kEyes = rgb(237,244,208);
constexpr uint16_t kPink = rgb(228,173,180);
constexpr uint16_t kLeafLight = rgb(164,201,136);
constexpr uint16_t kLeafDark = rgb(73,111,72);

// One family: quiet dark desks, warm paper cards and pastel CRT housings.
const Theme themes[kCount] = {
    {"工位花园", kCharcoal, kIvory, kIvory, rgb(166,180,170), kSage, kPurple,
     rgb(214,226,182), kOutline, kCardInk, kCardMuted, rgb(44,48,47)},
    {"紫色午后", rgb(34,32,44), rgb(248,244,240), rgb(246,240,252), rgb(181,171,194), rgb(150,188,151), kPurple,
     rgb(213,201,237), kOutline, kCardInk, rgb(105,98,112), rgb(48,44,62)},
    {"薄荷微风", rgb(25,38,38), rgb(241,249,239), rgb(232,248,239), rgb(157,184,177), rgb(128,186,156), kAqua,
     rgb(196,230,210), kOutline, kCardInk, rgb(87,108,101), rgb(39,55,53)},
    {"暖光书桌", rgb(39,34,29), rgb(250,242,224), rgb(247,237,219), rgb(190,174,150), rgb(151,175,124), rgb(233,193,129),
     rgb(234,217,173), kOutline, kCardInk, rgb(112,100,80), rgb(55,48,39)},
    {"夜航工位", rgb(24,35,43), rgb(235,244,241), rgb(225,242,242), rgb(152,177,187), rgb(135,189,171), rgb(165,193,234),
     rgb(175,209,210), kOutline, kCardInk, rgb(87,106,111), rgb(36,51,61)},
    {"莓果便签", rgb(42,31,37), rgb(251,240,232), rgb(250,232,235), rgb(190,164,177), rgb(153,181,138), rgb(221,168,194),
     rgb(233,199,196), kOutline, kCardInk, rgb(112,96,104), rgb(59,43,52)},
    {"苔绿书房", rgb(31,38,29), rgb(244,243,222), rgb(237,240,217), rgb(173,184,151), rgb(150,177,119), rgb(205,193,153),
     rgb(216,224,180), kOutline, kCardInk, rgb(102,109,86), rgb(45,55,40)},
    {"像素街机", rgb(29,30,45), rgb(243,241,249), rgb(239,236,253), rgb(169,169,196), rgb(134,190,162), rgb(184,171,240),
     rgb(199,196,230), kOutline, kCardInk, rgb(100,101,119), rgb(43,44,63)},
};
uint16_t mix(uint16_t a, uint16_t b, uint8_t weight) {
  const unsigned inv=255-weight;
  const unsigned ar=((a>>11)&31)*255/31, ag=((a>>5)&63)*255/63, ab=(a&31)*255/31;
  const unsigned br=((b>>11)&31)*255/31, bg=((b>>5)&63)*255/63, bb=(b&31)*255/31;
  return rgb((ar*inv+br*weight)/255,(ag*inv+bg*weight)/255,(ab*inv+bb*weight)/255);
}
// Integer edges keep the art crisp at arbitrary radii; neighbouring cells meet.
struct Pixels {
  Arduino_GFX &g;
  int16_t x,y,extent,cells;
  void rect(int16_t u,int16_t v,int16_t w,int16_t h,uint16_t color) {
    if(w<=0 || h<=0 || extent<=0)return;
    const int16_t left=x+static_cast<int32_t>(u)*extent/cells;
    const int16_t top=y+static_cast<int32_t>(v)*extent/cells;
    const int16_t right=x+static_cast<int32_t>(u+w)*extent/cells;
    const int16_t bottom=y+static_cast<int32_t>(v+h)*extent/cells;
    if(right>left && bottom>top)g.fillRect(left,top,right-left,bottom-top,color);
  }
  void plus(int16_t u,int16_t v,uint16_t color) {
    rect(u,v-1,1,3,color);rect(u-1,v,3,1,color);
  }
};
// Hand-authored monochrome pixel glyphs; each row is 16 bits, MSB at left.
const uint16_t icons[][16] = {
    {0x0000,0x0180,0x0180,0x0240,0x0240,0x0420,0x0420,0x0810,
     0x0810,0x1008,0x1008,0x1228,0x1018,0x0810,0x07E0,0x0000}, // Water
    {0x0000,0x03C0,0x0420,0x0420,0x0520,0x0520,0x0520,0x0520,
     0x0520,0x0910,0x1008,0x13C8,0x13C8,0x1008,0x0FF0,0x0000}, // Temp
    {0x0000,0x0000,0x0000,0x03C0,0x0420,0x0810,0x1818,0x2424,
     0x4002,0x4002,0x4002,0x3FFC,0x0000,0x0440,0x0880,0x0000}, // Humidity
    {0x0180,0x0180,0x2184,0x1008,0x0420,0x0FF0,0x1818,0xD81B,
     0xD81B,0x1818,0x0FF0,0x0420,0x1008,0x2184,0x0180,0x0180}, // Light
    {0x0000,0x0FF0,0x0810,0x0810,0x3FFC,0x4002,0x4022,0x4002,
     0x4002,0x4FF2,0x4812,0x3FFC,0x0810,0x0810,0x0FF0,0x0000}, // Print
    {0x0000,0x0FF0,0x1008,0x17E8,0x1428,0x1428,0x1668,0x1428,
     0x15A8,0x17E8,0x1008,0x1188,0x1188,0x1008,0x0FF0,0x0000}, // Yoyo
    {0x0180,0x03C0,0x1FF8,0x3FFC,0x1818,0x300C,0x33CC,0x73CE,
     0x73CE,0x33CC,0x300C,0x1818,0x3FFC,0x1FF8,0x03C0,0x0180}, // Settings
    {0x0000,0x0000,0x700E,0x781E,0x3C3C,0x1E78,0x0FF0,0x07E0,
     0x03C0,0x0180,0x0180,0x0180,0x0180,0x0180,0x0000,0x0000}, // Leaf
    {0x0000,0x07E0,0x1818,0x2004,0x4182,0x4182,0x4182,0x4182,
     0x41E2,0x4002,0x4002,0x2004,0x1818,0x07E0,0x0000,0x0000}, // Clock
    {0x0000,0x0000,0x07E0,0x1FF8,0x380C,0x6006,0x07E0,0x1FF8,
     0x1808,0x0000,0x03C0,0x0660,0x0000,0x0180,0x0180,0x0000}, // Wifi
    {0x0000,0x0000,0x0100,0x0300,0x0600,0x0C00,0x1800,0x3FFC,
     0x3FFC,0x1800,0x0C00,0x0600,0x0300,0x0100,0x0000,0x0000}, // Back
};
}
const Theme &get(uint8_t id) { return themes[id%kCount]; }
void background(Arduino_GFX &g,uint8_t id) { g.fillScreen(get(id).background); }
void icon(Arduino_GFX &g,Icon kind,int16_t x,int16_t y,int16_t size,uint16_t color) {
  const uint8_t index=static_cast<uint8_t>(kind);
  if(index>=sizeof(icons)/sizeof(icons[0]) || size<=0)return;
  Pixels p{g,x,y,size,16};
  for(int16_t row=0;row<16;++row) {
    for(int16_t col=0;col<16;) {
      if(!(icons[index][row]&(0x8000U>>col))) {++col;continue;}
      const int16_t start=col++;
      while(col<16 && (icons[index][row]&(0x8000U>>col)))++col;
      p.rect(start,row,col-start,1,color);
    }
  }
}
void face(Arduino_GFX &g,uint8_t id,int16_t cx,int16_t cy,
          int16_t radius,bool blink,uint32_t now) {
  if(radius<=0)return;
  const Theme &t=get(id);
  const uint16_t shellLight=mix(t.face,kIvory,150);
  const uint16_t shellShade=mix(t.face,kLeafDark,95);
  const uint16_t shellSide=mix(t.face,t.outline,115);
  const uint16_t screenRim=mix(t.face,t.outline,155);
  const int16_t breathe=radius>=64 && ((now/1000U)%4U==1U) ? -1 : 0;
  Pixels p{g,static_cast<int16_t>(cx-radius),
           static_cast<int16_t>(cy-radius+breathe),static_cast<int16_t>(radius*2),48};

  // Ground shadow, stepped plinth and short monitor neck.
  p.rect(8,45,34,2,mix(t.background,t.outline,130));
  p.rect(18,39,13,5,t.outline);
  p.rect(20,40,9,4,shellShade);
  p.rect(12,44,25,2,t.outline);
  p.rect(14,42,21,3,t.face);
  p.rect(15,42,19,1,shellLight);
  p.rect(12,44,25,1,shellShade);

  // CRT silhouette: stepped corners, deep right side and a warm front bevel.
  p.rect(8,16,33,27,t.outline);
  p.rect(6,18,37,23,t.outline);
  p.rect(5,20,39,19,t.outline);
  p.rect(40,19,3,21,shellSide);
  p.rect(8,41,33,1,shellSide);
  p.rect(8,17,31,24,t.face);
  p.rect(7,19,33,20,t.face);
  p.rect(6,21,1,16,shellShade);
  p.rect(8,17,31,2,shellLight);
  p.rect(7,19,2,20,shellLight);
  p.rect(39,20,1,20,shellShade);
  p.rect(9,40,30,1,shellShade);

  // Recessed glass with a subdued upper-left reflection.
  p.rect(11,20,25,18,screenRim);
  p.rect(9,22,29,14,screenRim);
  p.rect(10,21,27,16,screenRim);
  p.rect(12,21,23,16,kScreen);
  p.rect(10,23,27,12,kScreen);
  p.rect(11,22,25,14,kScreen);
  p.rect(12,22,10,1,rgb(36,45,37));
  p.rect(12,23,3,1,rgb(31,39,32));

  // Deterministic double blink, independent of rendering frequency.
  const uint32_t eyePhase=now%5700U;
  const bool closed=blink || (eyePhase>=4600U && eyePhase<4740U) ||
                    (eyePhase>=4900U && eyePhase<5010U);
  if(closed) {
    p.rect(15,28,5,1,kEyes);p.rect(28,28,5,1,kEyes);
  } else {
    p.rect(16,26,4,3,kEyes);p.rect(28,26,4,3,kEyes);
    p.rect(16,26,1,1,kIvory);p.rect(28,26,1,1,kIvory);
    p.rect(15,29,4,1,mix(kEyes,kScreen,145));
    p.rect(29,29,4,1,mix(kEyes,kScreen,145));
  }
  p.rect(14,31,3,1,kPink);p.rect(32,31,3,1,kPink);
  p.rect(21,31,1,1,kEyes);p.rect(26,31,1,1,kEyes);p.rect(22,32,4,1,kEyes);

  // Decorative power light and speaker slots; not actual device indicators.
  p.rect(23,39,1,1,t.outline);p.rect(25,39,1,1,t.accent);
  p.rect(32,39,2,1,shellSide);p.rect(35,39,2,1,shellSide);

  // Stepped leaves, highlighted vein and a gently swaying tip.
  const uint32_t leafPhase=(now/1200U)%4U;
  const int16_t sway=leafPhase==1U ? 1 : leafPhase==3U ? -1 : 0;
  p.rect(23,10,2,7,kLeafDark);p.rect(24,10,1,6,t.accent);
  p.rect(23+sway,7,2,5,kLeafDark);
  p.rect(13+sway,3,6,2,kLeafDark);p.rect(14+sway,5,7,2,kLeafDark);
  p.rect(16+sway,7,6,2,kLeafDark);p.rect(19+sway,9,4,2,kLeafDark);
  p.rect(13+sway,3,5,1,kLeafLight);p.rect(14+sway,4,5,2,t.accent);
  p.rect(16+sway,6,5,2,t.accent);p.rect(19+sway,8,3,1,t.accent);
  p.rect(28+sway,3,6,2,kLeafDark);p.rect(26+sway,5,7,2,kLeafDark);
  p.rect(25+sway,7,6,2,kLeafDark);p.rect(24+sway,9,4,2,kLeafDark);
  p.rect(29+sway,3,5,1,kLeafLight);p.rect(27+sway,4,6,2,t.accent);
  p.rect(26+sway,6,5,2,t.accent);p.rect(25+sway,8,3,1,t.accent);
  p.rect(16+sway,4,1,1,kLeafLight);p.rect(18+sway,6,1,1,kLeafLight);
  p.rect(29+sway,5,1,1,kLeafLight);

  // Small palette-specific badges keep every theme the same character.
  if(id%kCount==1 || id%kCount==5)p.rect(8,38,2,2,t.second);
  else if(id%kCount==4 || id%kCount==7) {
    p.rect(42,24,2,4,t.second);p.rect(42,30,2,4,t.accent);
  }
  if(radius>=48) {
    const uint32_t sparkle=(now/700U)%4U;
    p.plus(3,13,sparkle==0U ? kIvory : mix(t.muted,t.background,95));
    p.plus(44,9,sparkle==1U ? t.second : mix(t.second,t.background,100));
    p.plus(45,32,sparkle==2U ? kAqua : mix(kAqua,t.background,115));
  }
}
void deskScene(Arduino_GFX &g,uint8_t id,uint32_t now) {
  const Theme &t=get(id);
  const uint16_t edge=mix(t.surface,t.muted,45);
  g.fillRect(28,355,424,2,edge);
  g.fillRect(39,352,72,3,mix(t.background,t.outline,100));
  g.fillRect(356,352,87,3,mix(t.background,t.outline,100));

  // Lilac mug, stepped handle, coffee surface and a moving steam wisp.
  const uint16_t mug=t.second, mugShade=mix(mug,t.outline,65);
  g.fillRect(82,322,16,22,t.outline);
  g.fillRect(84,324,12,18,mugShade);
  g.fillRect(87,327,6,12,t.background);
  g.fillRect(47,316,38,35,t.outline);
  g.fillRect(49,318,34,31,mug);
  g.fillRect(78,319,5,30,mugShade);
  g.fillRect(52,320,4,25,mix(mug,kIvory,110));
  g.fillRect(48,315,36,4,kIvory);
  g.fillRect(53,316,25,2,rgb(114,92,72));
  g.fillRect(51,348,30,2,mugShade);
  const int16_t steam=(now/650U)%2U ? 2 : 0;
  g.fillRect(61+steam,302,2,7,mix(t.muted,t.background,45));
  g.fillRect(59+steam,296,2,6,mix(t.muted,t.background,80));
  g.fillRect(61+steam,291,2,5,mix(t.muted,t.background,115));

  // Three books, pale page blocks and tiny cover/spine details.
  g.fillRect(360,342,78,11,t.outline);
  g.fillRect(362,343,74,2,mix(t.face,kIvory,65));
  g.fillRect(365,345,68,5,t.card);
  g.fillRect(366,347,63,1,mix(t.card,t.cardMuted,50));
  g.fillRect(361,350,76,2,mix(t.face,t.outline,75));
  g.fillRect(371,329,60,13,t.outline);
  g.fillRect(373,330,56,11,t.second);
  g.fillRect(379,333,45,5,t.card);
  g.fillRect(380,335,42,1,mix(t.card,t.cardMuted,45));
  g.fillRect(373,330,3,11,mix(t.second,t.outline,65));
  g.fillRect(360,317,62,12,t.outline);
  g.fillRect(362,318,58,2,t.face);
  g.fillRect(367,320,50,6,t.card);
  g.fillRect(368,323,46,1,mix(t.card,t.cardMuted,45));
  g.fillRect(362,326,58,2,t.face);
  g.fillRect(413,318,2,10,t.accent);
}
}
