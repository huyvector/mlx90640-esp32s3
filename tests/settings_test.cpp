#include "../settings_codec.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using namespace camera;Settings s;s.interpolation=6;s.box=1;s.refresh=5;s.fps=8;s.minimum=-20;s.maximum=140;s.marker=3;s.pick_x=31;s.pick_y=23;s.capture=1;s.color=1;s.unit=2;
 assert(valid_settings(s));const auto bytes=encode_settings(s);Settings out;assert(decode_settings(bytes,out));assert(encode_settings(out)==bytes);
 for(unsigned i=0;i<bytes.size()*8;++i){auto broken=bytes;broken[i/8]^=1u<<(i%8);assert(!decode_settings(broken,out));}
 auto future=bytes;future[2]=4;assert(!decode_settings(future,out));
 s.box=2;assert(!valid_settings(s));s.box=1;s.refresh=8;assert(!valid_settings(s));s.refresh=4;s.maximum=s.minimum;assert(!valid_settings(s));
 auto legacy=bytes;legacy[2]=1;legacy[16]=0;const auto crc=settings_crc(legacy.data(),20);for(int i=0;i<4;++i)legacy[20+i]=crc>>(8*i);
 assert(decode_settings(legacy,out)&&out.fps==8&&out.refresh==5&&out.interpolation==6&&out.capture==1&&out.unit==0);
 for(unsigned fps:{2u,4u,6u,8u}){assert(valid_fps(fps));assert(sensor_clock_for_fps(fps)==400000u);}
 s.fps=9;s.maximum=140;assert(!valid_settings(s));
 for(unsigned u=0;u<3;++u){s=Settings{};s.unit=u;assert(decode_settings(encode_settings(s),out)&&out.unit==u);}
 s.unit=3;assert(!valid_settings(s));
 Settings defaults;assert(valid_settings(defaults));assert(decode_settings(encode_settings(defaults),out));
 std::puts("PASS: settings round-trip, signed temperatures, CRC corruption, version and bounds");
}
