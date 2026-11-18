from pathlib import Path
import subprocess, tempfile
repo=Path(__file__).resolve().parents[1]
s=(repo/'modern_ui.hpp').read_text();a=s.index(' void heat(');j=s.index('{',a);depth=1;k=j+1
while depth:
 if s[k]=='{':depth+=1
 if s[k]=='}':depth-=1
 k+=1
fn=s[a:k]
code='''#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cmath>
#define BOARD_SENSOR_OUTWARD true
#define N_HEATMAP 512
const uint16_t heatmap[2][N_HEATMAP]={
#include "heatmap.h"
};
int constrain(int n,int a,int b){return std::max(a,std::min(n,b));}
struct Rect {int x,y,w,h;};
struct Mock {std::vector<uint16_t> pixels=std::vector<uint16_t>(320*240,0xA55A);void* getBuffer(){return pixels.data();}} screen;
struct Renderer{int32_t levels_[768];
'''+fn+'''
};
int main(){Renderer renderer;float frame[768];for(int y=0;y<24;++y)for(int x=0;x<32;++x)frame[y*32+x]=-10+x*2+y*.75f;
 for(int interp:{1,2,4,6,8})for(Rect r: {Rect{4,24,240,180},Rect{0,0,320,240},Rect{96,72,128,96},Rect{120,95,32,24}}){
  std::fill(screen.pixels.begin(),screen.pixels.end(),0xA55A);renderer.heat(frame,r,interp,0,20,35);
  for(int y=0;y<240;++y)for(int x=0;x<320;++x){
   const bool inside=x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;
   const auto actual=screen.pixels[y*320+x];if(!inside){assert(actual==0xA55A);continue;}
   const int ux=r.w-1-(x-r.x),uy=y-r.y;
   const float sx=float(ux*(32*interp)/r.w)/interp,sy=float(uy*(24*interp)/r.h)/interp;
   const int ix=int(sx),iy=int(sy),nx=std::min(31,ix+1),ny=std::min(23,iy+1);const float fx=sx-ix,fy=sy-iy;
   const float a=frame[iy*32+ix]*(1-fx)+frame[iy*32+nx]*fx,b=frame[ny*32+ix]*(1-fx)+frame[ny*32+nx]*fx;
   const int index=constrain(int(((a*(1-fy)+b*fy)-20)*511/15),0,511);bool match=false;
   for(int i=std::max(0,index-1);i<=std::min(511,index+1);++i){const auto c=heatmap[0][i];match|=actual==uint16_t((c<<8)|(c>>8));}assert(match);
  }
 }
 std::puts("PASS: production modern heat renderer, all interpolation modes, ROI bounds, mirror, palette and RGB565");
}
'''
with tempfile.TemporaryDirectory(prefix='thermal-ui-test-') as directory:
 cpp=Path(directory)/'test.cpp';exe=Path(directory)/'test'
 cpp.write_text(code)
 subprocess.run(['clang++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(repo),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
