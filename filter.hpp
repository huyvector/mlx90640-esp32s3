#include "thermal_stats.hpp"
#include "temperature_units.hpp"
inline camera::StableThermalStats stable_thermal_stats;
inline bool thermal_range_ready=false;
/*================================================================================
 * Multitasking helper functions
 *================================================================================*/
#define TIME_CONSTANT   3.0f // [sec]

/*--------------------------------------------------------------------------------
 * Low pass filter (x: input, T: sampling time [sec])
 *--------------------------------------------------------------------------------*/
typedef struct {
  float y;
  float filter(float x, const float T) {
    return (y += (T / (T + TIME_CONSTANT) * (x - y)));
  }
} LowPass_t;

static LowPass_t lp_min, lp_max, lp_pick;

/*--------------------------------------------------------------------------------
 * Set the initial value for the low-pass filter
 *--------------------------------------------------------------------------------*/
void filter_reset(void) {
  stable_thermal_stats.reset();thermal_range_ready=false;
  lp_min.y  = MINTEMP;
  lp_max.y  = MAXTEMP;
  lp_pick.y = (MINTEMP + MAXTEMP) / 2;
}

/*--------------------------------------------------------------------------------
 * Measure temperature and update it's range & min/max points
 *--------------------------------------------------------------------------------*/
#if false
#define CHECK_VALUE
#include <math.h>
#endif

typedef struct {
  uint16_t  x, y;
  float     v;
} Temperature_t;

static Temperature_t te_min, te_max, _te_min, _te_max, te_pick;

void filter_update(void) {
  _te_min = te_min;
  _te_max = te_max;
}

void filter_temperature(float *src) {
  // Measure the temperature at the picked up point
  if (mlx_cnf.marker_mode & 2) {
    te_pick.v = src[te_pick.x + (te_pick.y * MLX90640_COLS)];
    te_pick.v = lp_pick.filter(te_pick.v, mlx_cnf.sampling_period);
  }

  if(!stable_thermal_stats.update(src,mlx_cnf.sampling_period))return;
  const auto& low=stable_thermal_stats.minimum();const auto& high=stable_thermal_stats.maximum();
  te_min={uint16_t(low.index%32),uint16_t(low.index/32),low.value};
  te_max={uint16_t(high.index%32),uint16_t(high.index/32),high.value};
  if(mlx_cnf.range_auto){
    const float low=stable_thermal_stats.rangeLow(),high=stable_thermal_stats.rangeHigh();
    const int wanted_low=std::max(camera::kTemperatureMin,std::min(int(floorf(low)),camera::kTemperatureMax-1));
    const int wanted_high=std::max(wanted_low+1,std::min(int(ceilf(high)),camera::kTemperatureMax));
    // Include a 0.25 C dead band before contracting the color scale.
    if(!thermal_range_ready||low<mlx_cnf.range_min||low>mlx_cnf.range_min+1.25f)mlx_cnf.range_min=wanted_low;
    if(!thermal_range_ready||high>mlx_cnf.range_max||high<mlx_cnf.range_max-1.25f)mlx_cnf.range_max=wanted_high;
    mlx_cnf.range_max=std::max(int(mlx_cnf.range_min)+1,int(mlx_cnf.range_max));thermal_range_ready=true;
  }
}
