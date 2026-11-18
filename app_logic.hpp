#pragma once
#include <stdint.h>

namespace camera {

inline bool valid_fps(uint8_t fps){return fps==2||fps==4||fps==6||fps==8;}
inline uint32_t sensor_clock_for_fps(uint8_t){return 400000u;}
inline uint8_t sensor_refresh_for_fps(uint8_t fps){return fps<=2?3:(fps<=4?4:5);}

// A finger that changed screens must be lifted before the next screen reacts.
class TouchReleaseGate {
  bool waiting_ = false;
 public:
  void suppress() { waiting_ = true; }
  bool accept(bool pressed) {
    if(!waiting_)return true;
    if(!pressed)waiting_=false;
    return false;
  }
};

inline bool touch_inside(uint16_t x,uint16_t y,uint16_t left,uint16_t top,uint16_t width,uint16_t height) {
 return x>=left&&x<uint32_t(left)+width&&y>=top&&y<uint32_t(top)+height;
}

// Keep taps immediate; smooth coordinates and require two release samples so a
// single missing FT report does not cancel an otherwise steady press.
class TouchSmoother {
  uint16_t xs_[3]{},ys_[3]{},last_x_=0,last_y_=0;
  unsigned count_=0,index_=0,release_=0;bool down_=false;
  static uint16_t middle(uint16_t a,uint16_t b,uint16_t c){
    return a>b?(b>c?b:(a>c?c:a)):(a>c?a:(b>c?c:b));
  }
 public:
  bool update(bool pressed,uint16_t& x,uint16_t& y){
    if(!pressed){
      x=last_x_;y=last_y_;
      if(down_&&++release_<2)return true;
      down_=false;count_=index_=0;return false;
    }
    release_=0;down_=true;xs_[index_]=x;ys_[index_]=y;index_=(index_+1)%3;
    if(count_<3)++count_;
    if(count_==2){x=(unsigned(xs_[0])+xs_[1])/2;y=(unsigned(ys_[0])+ys_[1])/2;}
    else if(count_==3){x=middle(xs_[0],xs_[1],xs_[2]);y=middle(ys_[0],ys_[1],ys_[2]);}
    last_x_=x;last_y_=y;return true;
  }
};

// Approximation for one standard 4.2V Li-ion cell under load, not a fuel gauge.
inline uint8_t battery_percent(uint16_t mv) {
  static const uint16_t volts[] = {3300,3500,3600,3700,3750,3800,3850,3900,4000,4100,4200};
  static const uint8_t levels[] = {0,5,10,25,40,50,60,70,80,90,100};
  if (mv <= volts[0]) return 0;
  for (unsigned i = 1; i < sizeof(volts)/sizeof(volts[0]); ++i) {
    if (mv <= volts[i]) {
      return levels[i-1] + uint32_t(mv-volts[i-1]) *
          (levels[i]-levels[i-1]) / (volts[i]-volts[i-1]);
    }
  }
  return 100;
}

inline bool touch_position(uint16_t raw_x, uint16_t raw_y, uint8_t rotation,
                           uint16_t& x, uint16_t& y) {
  if (raw_x >= 240 || raw_y >= 320) return false;
  switch (rotation & 3) {
    case 0: x = raw_x;       y = raw_y;       break;
    case 1: x = raw_y;       y = 239 - raw_x; break;
    case 2: x = 239 - raw_x; y = 319 - raw_y; break;
    default:x = 319 - raw_y; y = raw_x;       break;
  }
  return true;
}
inline bool touch_position_with_border(uint16_t raw_x,uint16_t raw_y,uint8_t rotation,uint16_t& x,uint16_t& y) {
  if(raw_x>=256||raw_y>=336)return false;
  return touch_position(raw_x>239?239:raw_x,raw_y>319?319:raw_y,rotation,x,y);
}
} // namespace camera
