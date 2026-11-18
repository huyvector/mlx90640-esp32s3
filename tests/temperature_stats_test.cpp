#include "../temperature_units.hpp"
#include "../thermal_stats.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using namespace camera;assert(fabsf(display_temperature(0,1)-32)<0.001f);assert(fabsf(display_temperature(100,1)-212)<0.001f);assert(fabsf(display_temperature(-40,1)+40)<0.001f);assert(fabsf(display_temperature(-40,2)-233.15f)<0.001f);assert(fabsf(display_temperature(300,2)-573.15f)<0.001f);
 StableThermalStats s;float frame[768];for(auto& v:frame)v=25;assert(s.update(frame,.125f));auto lo=s.minimum().index,hi=s.maximum().index;
 for(int n=0;n<100;++n){for(unsigned i=0;i<768;++i)frame[i]=25+((i*37+n*11)%17-8.0f)*.01f;assert(s.update(frame,.125f));assert(s.minimum().index==lo&&s.maximum().index==hi);assert(fabsf(s.minimum().value-25)<.1f&&fabsf(s.maximum().value-25)<.1f);}
 frame[10]=10;frame[700]=80;assert(s.update(frame,.125f));assert(s.minimum().index==10&&s.maximum().index==700);for(int n=0;n<5;++n)s.update(frame,.125f);assert(s.minimum().value<11&&s.maximum().value>79);
 s.reset();for(auto& v:frame)v=-40;assert(s.update(frame,.125f));assert(s.minimum().value==-40&&s.maximum().value==-40);
 for(auto& v:frame)v=NAN;assert(!s.update(frame,.125f));puts("PASS: C/F/K conversion, stable MIN/MAX markers, real changes, reset and invalid samples");
}
