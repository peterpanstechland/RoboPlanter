#include <cstdlib>
#include <iostream>
#include <limits>
#include "../../firmware/xiao/include/light_effects.h"

namespace {
int checks = 0;
void expect(bool result, const char* expression, int line) {
  ++checks;
  if (!result) {
    std::cerr << "FAIL line " << line << ": " << expression << '\n';
    std::exit(1);
  }
}
#define CHECK(expression) expect((expression), #expression, __LINE__)
bool equal(const robo::LightRgb& a, const robo::LightRgb& b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}
bool black(const robo::LightRgb& color) { return equal(color, {}); }

void parsingAndLimits() {
  robo::LightEffect effect = robo::LightEffect::Solid;
  for (const char* name : {"solid", "breath", "chase", "rainbow"}) {
    CHECK(robo::parseLightEffect(name, effect));
    CHECK(std::string(robo::lightEffectName(effect)) == name);
  }
  for (const char* name : {"", "Breath", "blink", "solid ", "rainbow|x"})
    CHECK(!robo::parseLightEffect(name, effect));
  for (auto kind : {robo::LightEffect::Solid, robo::LightEffect::Breath,
                    robo::LightEffect::Chase, robo::LightEffect::Rainbow}) {
    CHECK(black(robo::lightColor(kind, 0, 0, 0)));
    CHECK(black(robo::lightColor(kind, 0, 4, 4)));
    for (uint32_t time = 0; time < 25000; time += 137) {
      for (size_t index = 0; index < 120; index += 7) {
        const auto color = robo::lightColor(kind, time, index, 120);
        const unsigned limit = kind == robo::LightEffect::Solid ? 255 : 100;
        CHECK(color.red <= limit && color.green <= limit && color.blue <= limit);
      }
    }
  }
}

void distinctFrames() {
  using robo::LightEffect;
  CHECK(equal(robo::lightColor(LightEffect::Solid, 0, 0, 4), {255,255,255}));
  CHECK(equal(robo::lightColor(LightEffect::Solid, 4000, 3, 4), {255,255,255}));
  const auto dim = robo::lightColor(LightEffect::Breath, 0, 0, 4);
  CHECK(dim.green == 5);
  CHECK(robo::lightColor(LightEffect::Breath, 2000, 0, 4).green == 100);
  CHECK(equal(dim, robo::lightColor(LightEffect::Breath, 4000, 0, 4)));
  for (uint32_t time = 0; time <= 2000; time += 100) {
    CHECK(equal(robo::lightColor(LightEffect::Breath, time, 0, 4),
                robo::lightColor(LightEffect::Breath, 4000 - time, 3, 4)));
  }
  CHECK(robo::lightColor(LightEffect::Chase, 0, 0, 4).green == 100);
  CHECK(robo::lightColor(LightEffect::Chase, 0, 1, 4).green == 0);
  CHECK(robo::lightColor(LightEffect::Chase, 240, 1, 4).green == 100);
  CHECK(equal(robo::lightColor(LightEffect::Chase, 0, 0, 4),
              robo::lightColor(LightEffect::Chase, 960, 0, 4)));
  CHECK(!equal(robo::lightColor(LightEffect::Chase, 0, 0, 1),
               robo::lightColor(LightEffect::Chase, 2000, 0, 1)));
  CHECK(equal(robo::lightColor(LightEffect::Rainbow, 0, 0, 3), {85,0,0}));
  CHECK(equal(robo::lightColor(LightEffect::Rainbow, 0, 1, 3), {0,85,0}));
  CHECK(equal(robo::lightColor(LightEffect::Rainbow, 0, 2, 3), {0,0,85}));
  CHECK(equal(robo::lightColor(LightEffect::Rainbow, 0, 0, 3),
              robo::lightColor(LightEffect::Rainbow, 12000, 0, 3)));
  CHECK(!equal(robo::lightColor(LightEffect::Rainbow, 0, 0, 3),
               robo::lightColor(LightEffect::Rainbow, 4000, 0, 3)));
}

void stateAndRollover() {
  using robo::LightEffect;
  robo::LightAnimation animation;
  CHECK(!animation.on());
  CHECK(black(animation.pixel(1000, 0, 4)));
  CHECK(animation.configure(true, LightEffect::Breath, 100));
  CHECK(!animation.configure(true, LightEffect::Breath, 1100));
  CHECK(equal(animation.pixel(2100, 0, 4), {0,100,20}));
  CHECK(animation.configure(false, LightEffect::Rainbow, 2101));
  CHECK(!animation.on());
  CHECK(animation.effect() == LightEffect::Solid);
  CHECK(black(animation.pixel(2101, 0, 4)));
  CHECK(black(animation.pixel(std::numeric_limits<uint32_t>::max(), 0, 4)));
  CHECK(animation.configure(true, LightEffect::Breath, 3000));
  CHECK(animation.pixel(3000, 0, 4).green == 5);

  for (auto effect : {LightEffect::Solid, LightEffect::Breath, LightEffect::Chase, LightEffect::Rainbow}) {
    robo::LightAnimation normal, wrapped;
    const uint32_t start = std::numeric_limits<uint32_t>::max() - 99;
    normal.configure(true, effect, 0);
    wrapped.configure(true, effect, start);
    for (uint32_t elapsed : {0U, 99U, 100U, 240U, 1999U, 2000U, 4000U, 12000U, 50000U}) {
      const uint32_t now = start + elapsed;
      for (size_t i = 0; i < 5; ++i)
        CHECK(equal(normal.pixel(elapsed, i, 5), wrapped.pixel(now, i, 5)));
    }
    robo::LightAnimation longRunning;
    longRunning.configure(true, effect, 0);
    CHECK(!longRunning.configure(true, effect, std::numeric_limits<uint32_t>::max() - 99));
    CHECK(!longRunning.configure(true, effect, 100));
    for (size_t i = 0; i < 5; ++i)
      CHECK(equal(longRunning.pixel(100, i, 5),
                  robo::lightColor(effect, (uint64_t{1} << 32) + 100, i, 5)));
  }
}
void statusAndFillPriority() {
  using robo::LightStatus;
  using robo::LightEffect;
  CHECK(robo::connectionLightStatus(false,true,true)==LightStatus::Off);
  CHECK(robo::connectionLightStatus(true,false,true)==LightStatus::Waiting);
  CHECK(robo::connectionLightStatus(true,true,false)==LightStatus::Waiting);
  CHECK(robo::connectionLightStatus(true,true,true)==LightStatus::Online);
  robo::LightOutput output;
  CHECK(output.configure(false,LightEffect::Solid,LightStatus::Online,0));
  for(size_t i=0;i<30;++i)CHECK(equal(output.pixel(0,i,30),{0,64,0}));
  CHECK(black(output.pixel(0,30,30)));
  CHECK(!output.configure(false,LightEffect::Solid,LightStatus::Online,40));
  CHECK(!output.animated());
  CHECK(output.configure(true,LightEffect::Solid,LightStatus::Online,80));
  for(size_t i=0;i<30;++i)CHECK(equal(output.pixel(80,i,30),{255,255,255}));
  // Losing connectivity cannot replace requested fill with a status colour.
  CHECK(!output.configure(true,LightEffect::Solid,LightStatus::Waiting,120));
  CHECK(equal(output.pixel(120,29,30),{255,255,255}));
  CHECK(output.configure(false,LightEffect::Solid,LightStatus::Waiting,160));
  for(size_t i=0;i<30;++i)CHECK(equal(output.pixel(160,i,30),{64,24,0}));
  CHECK(output.configure(false,LightEffect::Solid,LightStatus::Off,200));
  CHECK(black(output.pixel(200,29,30)));
  CHECK(output.configure(true,LightEffect::Breath,LightStatus::Online,240));
  CHECK(output.animated());
  CHECK(!output.configure(true,LightEffect::Breath,LightStatus::Waiting,2240));
  CHECK(equal(output.pixel(2240,29,30),{0,100,20}));
  CHECK(std::string(robo::lightDisplayMode(false,LightEffect::Solid,LightStatus::Online))=="status_online");
  CHECK(std::string(robo::lightDisplayMode(true,LightEffect::Solid,LightStatus::Waiting))=="fill");
}
}  // namespace

int main() {
  parsingAndLimits(); distinctFrames(); stateAndRollover(); statusAndFillPriority();
  std::cout << "PASS " << checks << " light effect checks\n";
}
