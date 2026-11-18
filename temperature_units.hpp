#pragma once
#include <stdint.h>
namespace camera {
enum class TemperatureUnit:uint8_t {Celsius,Fahrenheit,Kelvin};
inline bool valid_temperature_unit(uint8_t unit){return unit<=2;}
inline float display_temperature(float c,uint8_t unit){return unit==1?c*1.8f+32.0f:(unit==2?c+273.15f:c);}
inline const char* temperature_suffix(uint8_t unit){return unit==1?"F":(unit==2?"K":"C");}
constexpr int kTemperatureMin=-40,kTemperatureMax=300;
}
