#pragma once
#include <stdint.h>
#include <stddef.h>
#include <array>
#include "app_logic.hpp"
#include "temperature_units.hpp"
namespace camera {
struct Settings {
 uint8_t interpolation=8,box=1,refresh=5,color=0,marker=0,automatic=0,capture=0;
 int16_t minimum=20,maximum=35;
 uint8_t pick_x=0,pick_y=0,fps=8,unit=0;
};
using SettingsBytes=std::array<uint8_t,24>;
inline bool valid_settings(const Settings& s) {
 const bool scale=s.interpolation==1||s.interpolation==2||s.interpolation==4||s.interpolation==6||s.interpolation==8;
 const bool box=s.box==1||s.box==2||s.box==4||s.box==8;
 return scale&&box&&s.interpolation*s.box<=8&&s.refresh>=3&&s.refresh<=5&&s.color<2&&s.marker<4&&
   s.automatic<2&&s.capture<2&&valid_temperature_unit(s.unit)&&valid_fps(s.fps)&&s.minimum>=-40&&s.maximum<=300&&s.maximum>s.minimum&&s.pick_x<32&&s.pick_y<24;
}
inline uint32_t settings_crc(const uint8_t* p,size_t n) {
 uint32_t crc=~0u;
 while(n--){crc^=*p++;for(int i=0;i<8;++i)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}
 return ~crc;
}
inline SettingsBytes encode_settings(const Settings& s) {
 SettingsBytes b{};b[0]='T';b[1]='C';b[2]=3;
 b[3]=s.interpolation;b[4]=s.box;b[5]=s.refresh;b[6]=s.color;b[7]=s.marker;b[8]=s.automatic;b[9]=s.capture;
 b[10]=uint16_t(s.minimum);b[11]=uint16_t(s.minimum)>>8;b[12]=uint16_t(s.maximum);b[13]=uint16_t(s.maximum)>>8;
 b[14]=s.pick_x;b[15]=s.pick_y;b[16]=s.fps;b[17]=s.unit;
 const auto crc=settings_crc(b.data(),20);for(int i=0;i<4;++i)b[20+i]=crc>>(8*i);
 return b;
}
inline bool decode_settings(const SettingsBytes& b,Settings& out) {
 if(b[0]!='T'||b[1]!='C'||(b[2]!=1&&b[2]!=2&&b[2]!=3))return false;
 uint32_t crc=0;for(int i=0;i<4;++i)crc|=uint32_t(b[20+i])<<(8*i);
 if(crc!=settings_crc(b.data(),20))return false;
 if(b[5]<3||b[5]>7)return false;
 Settings s;s.interpolation=b[3];s.box=b[4];s.refresh=b[5];s.color=b[6];s.marker=b[7];s.automatic=b[8];s.capture=b[9];
 s.minimum=int16_t(uint16_t(b[10])|(uint16_t(b[11])<<8));s.maximum=int16_t(uint16_t(b[12])|(uint16_t(b[13])<<8));
 s.pick_x=b[14];s.pick_y=b[15];s.unit=b[2]>=3?b[17]:0;s.fps=b[2]==1?8:b[16];if(s.fps==12||s.fps==14||s.fps==16||s.fps==20||s.fps==24)s.fps=8;s.refresh=sensor_refresh_for_fps(s.fps);if(!valid_settings(s))return false;out=s;return true;
}
}
