#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#define INTERPOLATE_SCALE 8
#define N_HEATMAP 512
#define ENA_OUTWARD_CAMERA 1

const uint16_t heatmap[][N_HEATMAP] = {
#include "../heatmap.h"
};
struct {int box_size=1,color_scheme=0,range_min=20,range_max=120;} mlx_cnf;
struct Sprite {std::vector<uint16_t> pixels;void* getBuffer(){return pixels.data();}} lcd_sprite;
int constrain(int v,int lo,int hi){return std::max(lo,std::min(v,hi));}
#include "../interpolation.hpp"
int main(){
 float src[32*24];
 for(int scale:{1,2,4,6,8}) for(int box:{1,2,4,8}) {
  if(scale*box>8)continue;
  const int w=32*scale*box,h=24*scale*box;
  mlx_cnf.box_size=box;lcd_sprite.pixels.assign(w*h,0);
  for(int y=0;y<24;++y)for(int x=0;x<32;++x)src[y*32+x]=float(x*4+y*2);
  interpolate_image(src,24,32,24*scale,32*scale);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
   const int ux=(w-1-x)/box,uy=y/box;
   const float fx=float(ux%scale)/scale,fy=float(uy%scale)/scale;
   const int sx=ux/scale,sy=uy/scale;
   const float top=src[sy*32+sx]*(1-fx)+src[sy*32+std::min(sx+1,31)]*fx;
   const float bottom=src[std::min(sy+1,23)*32+sx]*(1-fx)+src[std::min(sy+1,23)*32+std::min(sx+1,31)]*fx;
   const int index=constrain(int((top*(1-fy)+bottom*fy-mlx_cnf.range_min)*float(N_HEATMAP-1)/(mlx_cnf.range_max-mlx_cnf.range_min)),0,N_HEATMAP-1);
   bool match=false;
   for(int k=std::max(0,index-1);k<=std::min(N_HEATMAP-1,index+1);++k){
    const uint16_t c=heatmap[0][k];
    match|=lcd_sprite.pixels[y*w+x]==uint16_t((c<<8)|(c>>8));
   }
   assert(match);
  }
 }
 std::puts("PASS: interpolation bounds, bilinear values, mirror and RGB565 byte order at all supported scales");
}
