#pragma once
#include "ui_fonts.hpp"
namespace modern_type {
inline lgfx::VLWfont caption,body,number;
inline lgfx::PointerWrapper caption_data(modern_fonts::caption,sizeof(modern_fonts::caption)),body_data(modern_fonts::body,sizeof(modern_fonts::body)),number_data(modern_fonts::number,sizeof(modern_fonts::number));
inline bool ready=false;
inline void begin(){if(!ready)ready=caption.loadFont(&caption_data)&&body.loadFont(&body_data)&&number.loadFont(&number_data);}
}
