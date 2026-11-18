/*================================================================================
 * Pixel interpolation
 *================================================================================*/
inline float get_point(float *p, const int rows, const int cols, int x, int y) __attribute__((always_inline));
inline float get_point(float *p, const int rows, const int cols, int x, int y) {
//if (x < 0) { x = 0; } else
  if (x >= cols) { x = cols - 1; }
//if (y < 0) { y = 0; } else
  if (y >= rows) { y = rows - 1; }
  return p[y * cols + x];
}

/*--------------------------------------------------------------------------------------------
 * Bilinear interpolation
 * https://algorithm.joho.info/image-processing/bi-linear-interpolation/
 *--------------------------------------------------------------------------------------------*/
static float table_ratio[INTERPOLATE_SCALE][2];
void interpolate_setup(const int scale) {
  for(int i=0;i<scale;++i) {
    table_ratio[i][0]=float(i)/scale;
    table_ratio[i][1]=1.0f-table_ratio[i][0];
  }
}

void interpolate_image(float* src,const int rows,const int cols,const int dst_rows,const int dst_cols) {
  const int scale=dst_rows/rows;
  const int shift=scale==8?3:(scale==4?2:(scale==2?1:0));
  const bool binary_scale=scale!=6;
  auto divide=[=](int32_t n) { return binary_scale ? (n>>shift) : n/scale; };
  const int box=mlx_cnf.box_size;
  const int stride=dst_cols*box;
  uint16_t rowbuffer[32*INTERPOLATE_SCALE]; // Internal task RAM, copied to PSRAM in one burst.
  auto* pixels=static_cast<uint16_t*>(lcd_sprite.getBuffer());
  const uint16_t* palette=heatmap[mlx_cnf.color_scheme];
  const float lo=mlx_cnf.range_min;
  const float gain=float(N_HEATMAP-1)/std::max(1,int(mlx_cnf.range_max-mlx_cnf.range_min));
  // Quantize only the displayed palette coordinate, never the temperature data.
  // Q12 removes floating point work from the large output-pixel loop.
  static int32_t levels[32*24];
  for(int i=0;i<rows*cols;++i) {
    const float level=std::max(-4096.0f,std::min(4096.0f,(src[i]-lo)*gain));
    levels[i]=int32_t(level*4096.0f);
  }
  for(int y=0;y<rows;++y) {
    const int32_t* top=levels+y*cols;
    const int32_t* bottom=levels+std::min(y+1,rows-1)*cols;
    for(int j=0;j<scale;++j) {
      uint16_t* out=rowbuffer;
      for(int x=0;x<cols;++x) {
        const int next=std::min(x+1,cols-1);
        const int32_t a=top[x]+divide(j*(bottom[x]-top[x]));
        const int32_t b=top[next]+divide(j*(bottom[next]-top[next]));
        const int32_t step=divide(b-a);
        int32_t v=a;
        for(int i=0;i<scale;++i,v+=step) {
          const int index=constrain(int(v>>12),0,N_HEATMAP-1);
          const uint16_t rgb=palette[index];
          const uint16_t wire=uint16_t((rgb<<8)|(rgb>>8));
          int px=x*scale+i;
#if ENA_OUTWARD_CAMERA
          px=dst_cols-1-px;
#endif
          for(int n=0;n<box;++n)out[px*box+n]=wire;
        }
      }
      for(int n=0;n<box;++n)memcpy(pixels+((y*scale+j)*box+n)*stride,out,stride*sizeof(uint16_t));
    }
  }
}
