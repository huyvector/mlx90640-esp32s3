/*================================================================================
 * Thermograpy camera
 * Copyright (c) 2024 - 2025 embedded-kiddie
 * Released under the MIT license
 * https://opensource.org/license/mit
 *================================================================================*/
#include <Arduino.h>
#include "board_config.h"
#include "pin_assign.h"
#include "board_hardware.hpp"

/*--------------------------------------------------------------------------------
 * Step 1: Configure serial output for debugging
 *--------------------------------------------------------------------------------*/
#define DEBUG false
#if     DEBUG
#define DBG_EXEC(x) x
#else
#define DBG_EXEC(x)
#endif

/*--------------------------------------------------------------------------------
 * Step 2: Configure operational settings
 *--------------------------------------------------------------------------------*/
#define ENA_INTERPOLATION   true  // Enable interpolation
#define ENA_MULTITASKING    true  // Enable multi-task by FreeRTOS
#define ENA_OUTWARD_CAMERA  BOARD_SENSOR_OUTWARD // Camera orientation (true: Outward, false: Selfie)

/*--------------------------------------------------------------------------------
 * Step 3: Select GFX Library
 *--------------------------------------------------------------------------------*/
#if   1
#define USE_LOVYAN_GFX
/*--------------------------------------------------------------------------------
 * LovyanGFX
 * Note: Currently 'AUTODETECT' only works for 'ESP32 2432S028R'.
 * For other boards you need to configure appropriately to fit your device.
 * See https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx/boards.hpp
 *--------------------------------------------------------------------------------*/
#define USE_AUTODETECT  false

#else
#define USE_TFT_ESPI
/*--------------------------------------------------------------------------------
 * TFT_eSPI
 * CYD requires XPT2046 touch screen library.
 * See SPI bus assignment in 'boards/Setup_CYD_2432S028R.h' and 'pin_assign.h'.
 * https://github.com/Bodmer/TFT_eSPI/discussions/3123
 * https://forum.arduino.cc/t/cyd-sd-issues-probably-due-to-touch-pins/1280758/3
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/discussions/88
 * https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library
 *--------------------------------------------------------------------------------*/
  #if defined (ARDUINO_ESP32_2432S028R)
    #include <XPT2046_Bitbang.h>
  #endif
#endif

/*--------------------------------------------------------------------------------
 * Step 4: Configure flash memory setting to save touch calibration data
 * Preferences requires at least 2 partitions. Check the partition scheme on Tools.
 *--------------------------------------------------------------------------------*/
#define USE_PREFERENCES true

/*--------------------------------------------------------------------------------
 * Step 5: Configure resolution settings
 *--------------------------------------------------------------------------------*/
#if ENA_INTERPOLATION
#define INTERPOLATE_SCALE 8
#define BOX_SIZE          1
#define REFRESH_RATE      MLX90640_16_HZ
#else
#define INTERPOLATE_SCALE 1
#define BOX_SIZE          8
#define REFRESH_RATE      MLX90640_16_HZ
#endif

/*--------------------------------------------------------------------------------
 * Heat map
 *--------------------------------------------------------------------------------*/
#define N_HEATMAP   512 // 256 or 512 --> heatmap.h
constexpr uint16_t heatmap[2][N_HEATMAP] = {
#include "heatmap.h"    // 0; Rainbow, 1: Inferno
};

/*--------------------------------------------------------------------------------
 * Load modules
 *--------------------------------------------------------------------------------*/
#include "mlx.hpp"
#include "gfx.hpp"
#include "task.hpp"
#include "touch.hpp"
#include "sdcard.hpp"
#include "filter.hpp"
#include "interpolation.hpp"
#include "video_recorder.hpp"
#include "settings_store.hpp"
#include "mp4_player.hpp"
#include "perf_monitor.hpp"
#include "widget.hpp"
#include "board_ui.hpp"
#include "modern_ui.hpp"
#include "doom_port.hpp"

/*--------------------------------------------------------------------------------
 * Input process - Get thermal image from MLX90640
 *--------------------------------------------------------------------------------*/
void ProcessInput(uint8_t bank) {
  static unsigned failures=0;
  board_frame_valid[bank] = false;
  if (!board_acquire_enabled.load()) { delay(20); return; }
  if (!board_sensor_ready.load()) {
    static uint32_t previous_attempt = 0;
    if (uint32_t(millis() - previous_attempt) >= 2000) {
      previous_attempt = millis();
      mlx_setup();
    }
    delay(20);
    return;
  }
  static uint32_t next_frame_us=0;static uint8_t previous_fps=0;
  const uint8_t fps=board_requested_fps.load();const uint32_t period=(1000000u+fps/2)/fps;
  if(previous_fps!=fps){next_frame_us=0;previous_fps=fps;}
  while(uint32_t(next_frame_us-micros())<period)delay(1);
  const uint32_t frame_started_us=micros();next_frame_us=frame_started_us+period;
  mlx_refresh(); // Only this task owns the MLX instance.
  const int result = mlx.getFrame(src[bank]);
  board_sensor_error.store(result);
  if (result != 0) {
    Serial.printf("MLX frame error: %d\n", result);
    board_sensor_ready.store(false);
    if(++failures>=3) {
      board_sensor_hz=400000;board_reference_acquisition=true;board_requested_refresh=camera::sensor_refresh_for_fps(board_requested_fps.load());
      Serial.println("I2C fallback: 400kHz / 16 subpages; using standard read recovery.");
    }
    delay(20);
    return;
  }
  // A damaged/disconnected sensor must not create a believable saved image.
  for (size_t i = 0; i < MLX90640_COLS * MLX90640_ROWS; ++i) {
    if (!isfinite(src[bank][i])) {
      board_sensor_error.store(-12);
      return;
    }
  }
  if(board_sensor_settle_frames.load()) {
    board_sensor_settle_frames.fetch_sub(1);
    return;
  }
  if(board_diagnostics_pending.exchange(false)) {
    float chess[2]={0,0};
    Serial.printf("FRAME mode=%u rate=%u reference=%d rows:",mlx.getMode(),mlx.getRefreshRate(),board_reference_acquisition.load());
    for(int y=0;y<24;++y) {
      float sum=0;
      for(int x=0;x<32;++x){const float t=src[bank][y*32+x];sum+=t;chess[(x+y)&1]+=t;}
      Serial.printf(" %.2f",sum/32);
    }
    Serial.printf("; chess=%.2f/%.2f C\n",chess[0]/384,chess[1]/384);
  }
  board_sensor_ambient.store(mlx.getTa(false)); // no extra I2C operation
  failures=0;
  perf_acquired.fetch_add(1);
  perf_read_us.fetch_add(mlx.lastReadUs);
  perf_calc_us.fetch_add(mlx.lastCalculateUs);
  board_frame_valid[bank] = true;
  delay(1);
}

/*--------------------------------------------------------------------------------
 * Output process - Interpolate thermal image and display on LCD.
 *--------------------------------------------------------------------------------*/
void ProcessOutput(uint8_t bank, uint32_t inputStart, uint32_t inputFinish) {
  if(board_doom_pending.load()&&!video_recorder.busy()){board_doom_pending=false;if(!doom_start())modern::ui.home();}
  if(board_doom_active.load()){delay(10);return;}
  if(board_home_pending.exchange(false))modern::ui.home();
  if(board_touch_test_pending.exchange(false))modern::ui.startTouchTest();
  if(board_play_last_pending.exchange(false))modern::ui.playPath(video_recorder.path());
  const float* input=(bank!=NOT_UPDATED&&board_frame_valid[bank])?src[bank]:nullptr;
  const uint32_t render_started=micros();
  const bool new_frame=modern::ui.update(input);
  board_acquire_enabled=modern::ui.wantsLive();
  if(new_frame)perf_render_us.fetch_add(uint32_t(micros()-render_started));
  if(board_save_settings_pending.exchange(false))board_settings_save();
  if(board_restart_pending.exchange(false)) {
    if(video_recorder.busy())video_recorder.stop();
    Serial.println("Restarting; saved settings will be restored.");Serial.flush();delay(100);ESP.restart();
  }
  if(new_frame&&video_recorder.active())video_recorder.submit(screen.getBuffer(),inputFinish);
  const uint32_t lcd_started=micros();gfx_flush();
  if(new_frame){perf_rendered.fetch_add(1);perf_lcd_us.fetch_add(uint32_t(micros()-lcd_started));}
}

void setup() {
  Serial.begin(115200); // Never wait for a USB host: battery boot must work.
  Serial.printf("Boot reset reason: %u\n",unsigned(esp_reset_reason()));
  const bool bus_ok = board_hardware_setup();
  gfx_setup();
  Serial.printf("LCDWIKI ES3C28P / ESP32-S3, PSRAM %u bytes\n", ESP.getPsramSize());
  if (!bus_ok) {
    lcd.setTextSize(1);
    lcd.drawString("I2C initialization failed. Reset device.", 5, 30);
    while (true) delay(100);
  }
  touch_setup();
  board_select_fps(8);
  board_settings_load();
  mlx_cnf.init();
  mlx_setup();
  Serial.println(sdcard_setup()?"MicroSD background check started.":"MicroSD task unavailable; camera still works.");
  Serial.println(video_recorder.setup()?"MP4 recorder ready.":"MP4 buffer allocation failed.");
  Serial.println(modern::ui.begin()?"Modern UI ready.":"Modern UI ready; animations unavailable.");
  gfx_flush();
  task_setup(ProcessInput, ProcessOutput);
}

void loop() {
  if(board_doom_active.load()){delay(10);return;}
  perf_service();
  delay(board_media_focus.load()?50:2);
}
