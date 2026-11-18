#include "../touch_alignment.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using namespace camera;
 const auto a=measured_touch_affine();assert(valid_touch_affine(a));
 const TouchSample measured[]={{42,64,40,38},{165,63,160,38},{285,62,280,38},{160,103,160,90},{162,153,160,150},{166,201,160,208}};
 for(auto p:measured){uint16_t x=p.raw_x,y=p.raw_y;align_touch(a,x,y);assert(fabsf(float(x)-p.screen_x)<5&&fabsf(float(y)-p.screen_y)<4);}
 uint16_t x=0,y=0;align_touch(a,x,y);assert(x<=319&&y==0);x=319;y=239;align_touch(a,x,y);assert(x<=319&&y==239);
 TouchAffineRecord r;r.map=a;r.checksum=touch_affine_checksum(r);assert(valid_touch_record(r));r.map.y0+=1;assert(!valid_touch_record(r));
 TouchSample bad[6];for(int i=0;i<6;++i)bad[i]=measured[i];bad[2].screen_y+=40;TouchAffine unchanged;assert(!fit_touch_affine(bad,6,unchanged));
 for(auto& p:bad)p={100,100,100,100};assert(!fit_touch_affine(bad,6,unchanged));
 assert(!fit_touch_affine(measured,2,unchanged));
 puts("PASS: measured full-panel affine fit, bounds, corrupt record and inconsistent/degenerate samples");
}
