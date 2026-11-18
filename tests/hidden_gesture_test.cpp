#include "../hidden_gesture.hpp"
#include <cassert>
#include <cstdio>
int main(){camera::HiddenInfoGesture g;for(unsigned i=0;i<4;++i)assert(!g.tap(100+i*100,false));assert(g.tap(500,false));assert(!g.tap(600,false));g.reset();for(unsigned i=0;i<4;++i)assert(!g.tap(1000+i*100,false));assert(!g.tap(1400,true));assert(!g.tap(1500,false));assert(!g.tap(4000,false));for(unsigned i=1;i<4;++i)assert(!g.tap(4000+i*100,false));assert(g.tap(4400,false));puts("PASS: exactly five taps, excluded controls reset sequence, timeout and reset");}
