#pragma once
#include <atomic>
#include <algorithm>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "board_bus.hpp"
#include "app_logic.hpp"

inline std::atomic<uint8_t> board_requested_fps{8};
inline std::atomic<bool> board_media_focus{false};
inline std::atomic<uint8_t> board_requested_refresh{5}; // 16 subpages/s, up to 8 complete frames/s at 400kHz
inline std::atomic<bool> board_acquire_enabled{true};
inline std::atomic<bool> board_sensor_ready{false};
inline std::atomic<unsigned> board_sensor_settle_frames{3};
inline std::atomic<int> board_sensor_error{0};
inline std::atomic<float> board_sensor_ambient{0};
inline bool board_frame_valid[2] = {false, false}; // handed over by FreeRTOS queue
inline std::atomic<bool> board_capture_pending{false};
inline std::atomic<bool> board_video_toggle_pending{false};
inline bool board_touch_ready = false;
inline std::atomic<bool> board_touch_trace{false};
inline void board_select_fps(uint8_t fps) {
  if(!camera::valid_fps(fps))fps=8;
  board_reference_acquisition=false;
  board_sensor_hz=camera::sensor_clock_for_fps(fps);
  board_diagnostics_pending=true;
  board_requested_refresh=camera::sensor_refresh_for_fps(fps);
  board_requested_fps=fps;
}
inline void board_select_refresh(uint8_t){board_select_fps(board_requested_fps.load());}

inline bool board_hardware_setup() {
  pinMode(BOARD_AUDIO_EN, OUTPUT);
  digitalWrite(BOARD_AUDIO_EN, HIGH); // Mute the onboard amplifier.
  analogReadResolution(12);
  analogSetPinAttenuation(BOARD_BATTERY_PIN, ADC_11db);
  pinMode(BOARD_TOUCH_INT, INPUT_PULLUP);
  pinMode(BOARD_TOUCH_RST, OUTPUT);
  digitalWrite(BOARD_TOUCH_RST, LOW);
  delay(10);
  digitalWrite(BOARD_TOUCH_RST, HIGH);
  delay(200);
  return board_bus_setup(MLX_I2C_SDA, MLX_I2C_SCL, BOARD_I2C_HZ);
}

struct BoardTouchEdge { uint16_t x,y;bool pressed; };
inline QueueHandle_t board_touch_queue=nullptr;
inline std::atomic<uint32_t> board_touch_xy{0};
inline std::atomic<uint32_t> board_touch_unfiltered_xy{0};
inline std::atomic<bool> board_touch_test_pending{false};

inline int board_touch_read_hardware(uint16_t& x,uint16_t& y);
inline void board_touch_task(void*) {
  uint16_t x=0,y=0;bool previous=false;camera::TouchSmoother smoother;
  while(true) {
    const int state=board_touch_read_hardware(x,y);
    if(state>=0) {
      board_touch_unfiltered_xy.store(uint32_t(x)|(uint32_t(y)<<16)|(state>0?0x80000000u:0u));
      const bool pressed=smoother.update(state>0,x,y);
      board_touch_xy.store(uint32_t(x)|(uint32_t(y)<<16)|(pressed?0x80000000u:0u));
      if(pressed!=previous) {
        BoardTouchEdge edge{x,y,pressed};
        if(xQueueSend(board_touch_queue,&edge,0)!=pdTRUE){BoardTouchEdge old;xQueueReceive(board_touch_queue,&old,0);xQueueSend(board_touch_queue,&edge,0);}
        previous=pressed;
      }
    }
    ulTaskNotifyTake(pdTRUE,std::max(TickType_t(1),pdMS_TO_TICKS(board_media_focus.load()?10:5)));
  }
}
inline void board_touch_discard_events(){if(board_touch_queue)xQueueReset(board_touch_queue);}

inline bool board_touch_setup() {
  board_touch_ready = board_write_register(BOARD_TOUCH_ADDR, 0x00, 0x00)
      && board_write_register(BOARD_TOUCH_ADDR, 0xA4, 0x00); // polling mode
  uint8_t id = 0;
  if (board_touch_ready) {
    board_read_register(BOARD_TOUCH_ADDR, 0xA8, &id, 1);
    Serial.printf("FT6336G: address 0x38, vendor 0x%02X\n", id);
  } else {
    Serial.println("FT6336G not found; use Serial commands p/v for capture/record.");
  }
  if(board_touch_ready){
    uint8_t threshold=0,control=0;
    board_read_register(BOARD_TOUCH_ADDR,0x80,&threshold,1);board_read_register(BOARD_TOUCH_ADDR,0x86,&control,1);
    const bool active=board_write_register(BOARD_TOUCH_ADDR,0x86,0x00);
    Serial.printf("FT touch threshold=%u, previous control=%u, active mode=%d\n",threshold,control,active);
  }
  if(board_touch_ready) {
    board_touch_queue=xQueueCreate(16,sizeof(BoardTouchEdge));
    if(!board_touch_queue||xTaskCreatePinnedToCore(board_touch_task,"Touch sampling",3072,nullptr,2,&board_touch_task_handle,0)!=pdPASS)board_touch_ready=false;
  }
  return board_touch_ready;
}

// -1 = bus busy/read error (preserve last state), 0 = released, 1 = touched.
inline int board_touch_read_hardware(uint16_t& x, uint16_t& y) {
  if (!board_touch_ready) return 0;
  uint8_t data[5]; // TD_STATUS then first point XH/XL/YH/YL
  if (!board_read_register(BOARD_TOUCH_ADDR, 0x02, data, sizeof(data))) return -1;
  if ((data[0] & 0x0F) == 0) return 0;
  if ((data[0] & 0x0F) > 2) return -1;
  if ((data[1] >> 6) == 1) return 0;
  const uint16_t raw_x = ((data[1] & 0x0F) << 8) | data[2];
  const uint16_t raw_y = ((data[3] & 0x0F) << 8) | data[4];
  if(raw_x>=256||raw_y>=336){if(board_touch_trace)Serial.printf("TOUCH rejected raw=%u,%u\n",raw_x,raw_y);return -1;}
  return camera::touch_position_with_border(raw_x,raw_y,BOARD_TOUCH_ROTATION,x,y)?1:0;
}

inline int board_touch_read(uint16_t& x,uint16_t& y) {
 if(!board_touch_ready)return 0;
 BoardTouchEdge edge;
 if(board_touch_queue&&xQueueReceive(board_touch_queue,&edge,0)==pdTRUE){x=edge.x;y=edge.y;return edge.pressed?1:0;}
 const uint32_t xy=board_touch_xy.load();x=xy&65535;y=(xy>>16)&32767;return (xy&0x80000000u)?1:0;
}
