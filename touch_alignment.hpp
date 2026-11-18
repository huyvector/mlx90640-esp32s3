#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace camera {
struct TouchAffine {float xx=1,xy=0,x0=0,yx=0,yy=1,y0=0;};
struct TouchSample {float raw_x,raw_y,screen_x,screen_y;};
inline bool valid_touch_affine(const TouchAffine& a){
 const float v[]={a.xx,a.xy,a.x0,a.yx,a.yy,a.y0};for(float x:v)if(!std::isfinite(x))return false;
 return a.xx>=0.5f&&a.xx<=2&&a.yy>=0.5f&&a.yy<=2&&fabsf(a.xy)<0.3f&&fabsf(a.yx)<0.3f&&fabsf(a.x0)<=80&&fabsf(a.y0)<=80;
}
inline bool fit_touch_affine(const TouchSample* p,size_t n,TouchAffine& result){
 if(n<4)return false;
 double rx=0,ry=0,tx=0,ty=0;for(size_t i=0;i<n;++i){rx+=p[i].raw_x;ry+=p[i].raw_y;tx+=p[i].screen_x;ty+=p[i].screen_y;}rx/=n;ry/=n;tx/=n;ty/=n;
 double xx=0,xy=0,yy=0,xt=0,yt=0,xu=0,yu=0;
 for(size_t i=0;i<n;++i){const double x=p[i].raw_x-rx,y=p[i].raw_y-ry,t=p[i].screen_x-tx,u=p[i].screen_y-ty;xx+=x*x;xy+=x*y;yy+=y*y;xt+=x*t;yt+=y*t;xu+=x*u;yu+=y*u;}
 const double det=xx*yy-xy*xy;if(det<1e6)return false;
 TouchAffine a;a.xx=(xt*yy-yt*xy)/det;a.xy=(yt*xx-xt*xy)/det;a.x0=tx-a.xx*rx-a.xy*ry;
 a.yx=(xu*yy-yu*xy)/det;a.yy=(yu*xx-xu*xy)/det;a.y0=ty-a.yx*rx-a.yy*ry;
 if(!valid_touch_affine(a))return false;
 double error=0;for(size_t i=0;i<n;++i){const double dx=a.xx*p[i].raw_x+a.xy*p[i].raw_y+a.x0-p[i].screen_x,dy=a.yx*p[i].raw_x+a.yy*p[i].raw_y+a.y0-p[i].screen_y;if(dx*dx+dy*dy>100)return false;error+=dx*dx+dy*dy;}
 if(error/n>36)return false;result=a;return true;
}
inline void align_touch(const TouchAffine& a,uint16_t& x,uint16_t& y){
 const float rx=x,ry=y;const long xx=lroundf(a.xx*rx+a.xy*ry+a.x0),yy=lroundf(a.yx*rx+a.yy*ry+a.y0);
 x=xx<0?0:(xx>319?319:xx);y=yy<0?0:(yy>239?239:yy);
}
// Six targets measured on this user's panel; used only when no valid NVS fit exists.
inline TouchAffine measured_touch_affine(){
 const TouchSample p[]={{42,64,40,38},{165,63,160,38},{285,62,280,38},{160,103,160,90},{162,153,160,150},{166,201,160,208}};
 TouchAffine a;fit_touch_affine(p,6,a);return a;
}
struct TouchAffineRecord {uint32_t version=1;TouchAffine map;uint32_t checksum=0;};
inline uint32_t touch_affine_checksum(const TouchAffineRecord& r){
 uint32_t hash=2166136261u;const auto* bytes=reinterpret_cast<const uint8_t*>(&r);for(size_t i=0;i<offsetof(TouchAffineRecord,checksum);++i)hash=(hash^bytes[i])*16777619u;return hash;
}
inline bool valid_touch_record(const TouchAffineRecord& r){return r.version==1&&valid_touch_affine(r.map)&&r.checksum==touch_affine_checksum(r);}
}
