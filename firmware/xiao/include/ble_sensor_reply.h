#pragma once
#include "controller.h"
#include "sensor_calibration.h"
#include <cmath>
#include <cstdio>
#include <string>
namespace robo {
inline std::string soilBleReply(int raw,bool valid,const SensorCalibration& saved) {
  if(!valid || raw<0 || raw>4095)return "Soil:unavailable";
  if(!saved.soilValid())return "Soil:uncalibrated";
  return "Soil:"+std::to_string(int(std::lround(soilPercent(raw,saved.dry,saved.wet))))+"% calibrated";
}
inline std::string deskBleReply(int raw,bool soilValid,const SensorCalibration& saved,
                                float temperature,float humidity,bool envValid) {
  const auto soil=soilBleReply(raw,soilValid,saved);
  std::string value="S--";
  if(soil=="Soil:uncalibrated")value="Suncal";
  else if(soilValid && saved.soilValid() && raw>=0 && raw<=4095)
    value="S"+std::to_string(int(std::lround(soilPercent(raw,saved.dry,saved.wet))))+"%";
  char env[24];
  if(envValid && std::isfinite(temperature) && std::isfinite(humidity) && temperature>=-40 && temperature<=85 && humidity>=0 && humidity<=100)
    snprintf(env,sizeof(env)," T%.1fC H%.0f%%",double(temperature),double(humidity));
  else snprintf(env,sizeof(env)," T-- H--");
  value+=env;
  // Invalid dimensions must never be converted to an oversized ATT reply.
  return value.size()<=20?value:"E:SIZE";
}
}
