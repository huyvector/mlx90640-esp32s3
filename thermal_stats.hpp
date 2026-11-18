#pragma once
#include <cmath>
#include <algorithm>
namespace camera {
struct ThermalPoint {unsigned index=0;float value=0;};
// Display statistics only. Keep the acquired temperature image unchanged.
class StableThermalStats {
 float pixels_[768]{};bool ready_=false;float range_low_=0,range_high_=0;
 ThermalPoint low_,high_;
 static float follow(float from,float to,float dt){const float delta=to-from;return from+delta*(fabsf(delta)>=2.0f?0.8f:dt/(0.30f+dt));}
 public:
 void reset(){ready_=false;}
 bool update(const float* values,float dt){
  dt=std::max(0.01f,std::min(dt,1.0f));unsigned lo=0,hi=0;float mn=INFINITY,mx=-INFINITY;
  for(unsigned i=0;i<768;++i){if(!std::isfinite(values[i]))continue;pixels_[i]=ready_?follow(pixels_[i],values[i],dt):values[i];if(pixels_[i]<mn){mn=pixels_[i];lo=i;}if(pixels_[i]>mx){mx=pixels_[i];hi=i;}}
  if(!std::isfinite(mn)||!std::isfinite(mx))return false;
  if(!ready_){low_={lo,mn};high_={hi,mx};range_low_=mn;range_high_=mx;ready_=true;}
  else{
   // Retain a marker while its pixel is within 0.4 C of the filtered extremum.
   // A distinctly hotter/colder region moves the marker immediately.
   if(pixels_[low_.index]-mn>0.4f)low_.index=lo;
   if(mx-pixels_[high_.index]>0.4f)high_.index=hi;
   low_.value=follow(low_.value,pixels_[low_.index],dt);high_.value=follow(high_.value,pixels_[high_.index],dt);
   range_low_=follow(range_low_,mn,dt);range_high_=follow(range_high_,mx,dt);
  }
  return true;
 }
 const ThermalPoint& minimum()const{return low_;}const ThermalPoint& maximum()const{return high_;}
 float rangeLow()const{return range_low_;}float rangeHigh()const{return range_high_;}
};
}
