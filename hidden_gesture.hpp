#pragma once
#include <stdint.h>
namespace camera {
class HiddenInfoGesture{
 unsigned taps_=0;uint32_t last_=0;
 public:void reset(){taps_=0;}bool tap(uint32_t now,bool excluded){if(excluded){reset();return false;}if(uint32_t(now-last_)>1500)taps_=0;last_=now;if(++taps_==5){reset();return true;}return false;}
};
}
