#pragma once
#include <ArduinoJson.h>
#include <cstring>
#include "daily_ticket.h"
namespace robo {
constexpr const char* DailyFields[]={"card","reversed","english","species","soil","raw","temperature","humidity"};
constexpr int DailyTicket::* DailyMembers[]={&DailyTicket::card,&DailyTicket::reversed,&DailyTicket::english,&DailyTicket::species,&DailyTicket::soil,&DailyTicket::raw,&DailyTicket::temperature,&DailyTicket::humidity};
inline bool readDaily(JsonVariantConst value,DailyTicket& d) {
  if(!value.is<JsonObjectConst>())return false;
  auto object=value.as<JsonObjectConst>();if(object.size()!=8)return false;
  for(int i=0;i<8;++i){auto field=object[DailyFields[i]];if(field.is<bool>()||!field.is<int>())return false;d.*DailyMembers[i]=field.as<int>();}
  return validDaily(d);
}
inline void writeDaily(JsonObject object,const DailyTicket& d) {
  for(int i=0;i<8;++i)object[DailyFields[i]]=d.*DailyMembers[i];
}
}
