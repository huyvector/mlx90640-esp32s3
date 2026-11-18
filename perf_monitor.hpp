#include "doom_control.hpp"
#pragma once
inline std::atomic<uint32_t> perf_acquired{0},perf_rendered{0};
inline std::atomic<uint32_t> perf_read_us{0},perf_calc_us{0},perf_lcd_us{0};
inline std::atomic<uint32_t> perf_render_us{0};
inline std::atomic<float> perf_display_fps{0};

inline void perf_service() {
  while(Serial.available()) {
    const char c=Serial.read();
    if(c>='1'&&c<='4'){const uint8_t rates[]={2,4,6,8};board_select_fps(rates[c-'1']);Serial.printf("FPS target=%u, sensor clock=%uHz\n",board_requested_fps.load(),board_sensor_hz.load());}
    else if(c=='v')board_video_toggle_pending=true;
    else if(c=='p')board_capture_pending=true;
    else if(c=='l')board_play_last_pending=true;
    else if(c=='h')board_home_pending=true;
    else if(c=='g')board_doom_pending=true;
    else if(c=='d')board_touch_test_pending=true;
    else if(c=='t'){board_touch_trace=!board_touch_trace;Serial.printf("Touch trace=%d\n",board_touch_trace.load());}
    else if(c=='s')board_save_settings_pending=true;
    else if(c=='R')board_restart_pending=true;
    else if(c=='i') {
      Serial.printf("INFO PSRAM=%u touch=%d sensor=%d last_error=%d sensor_clock=%u target_fps=%u jpeg-last=%.1fms sd-last=%.1fms\n",
          ESP.getPsramSize(),board_touch_ready,board_sensor_ready.load(),
          board_sensor_error.load(),board_sensor_hz.load(),board_requested_fps.load(),video_recorder.last_jpeg_us.load()/1000.0f,video_recorder.last_write_us.load()/1000.0f);
    }
  }
  static uint32_t previous=millis(),prev_in=0,prev_out=0;
  static uint32_t prev_read=0,prev_calc=0,prev_lcd=0,prev_vid=0,prev_render=0;
  const uint32_t now=millis();static bool paused=false;
  if(board_media_focus.load()){paused=true;return;}
  if(paused){previous=now;prev_in=perf_acquired.load();prev_out=perf_rendered.load();prev_read=perf_read_us.load();prev_calc=perf_calc_us.load();prev_lcd=perf_lcd_us.load();prev_vid=video_recorder.encoded.load();prev_render=perf_render_us.load();paused=false;return;}
  const uint32_t dt=now-previous;
  if(dt<5000)return;
  const auto in=perf_acquired.load(),out=perf_rendered.load();
  const auto rd=perf_read_us.load(),calc=perf_calc_us.load(),lcd=perf_lcd_us.load();
  const auto vid=video_recorder.encoded.load();
  const auto render=perf_render_us.load();
  const unsigned n=in-prev_in,m=out-prev_out;
  perf_display_fps=1000.0f*m/dt;
  Serial.printf("PERF source=%.2ffps display=%.2ffps read=%.2fms calc=%.2fms render=%.2fms lcd=%.2fms video=%.2ffps dropped=%u I2C_errors=%u clock=%u jpeg-last=%.1fms sd-last=%.1fms\n",
      1000.0f*n/dt,perf_display_fps.load(),n?(rd-prev_read)/1000.0f/n:0,
      n?(calc-prev_calc)/1000.0f/n:0,m?(render-prev_render)/1000.0f/m:0,m?(lcd-prev_lcd)/1000.0f/m:0,
      video_recorder.active()?1000.0f*(vid>=prev_vid?vid-prev_vid:vid)/dt:0,
      video_recorder.dropped.load(),board_i2c_errors.load(),board_sensor_hz.load(),video_recorder.last_jpeg_us.load()/1000.0f,video_recorder.last_write_us.load()/1000.0f);
  previous=now;prev_in=in;prev_out=out;prev_read=rd;prev_calc=calc;prev_lcd=lcd;prev_vid=vid;prev_render=render;
}
