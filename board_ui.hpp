#pragma once

// Called only from the UI task: all LCD and SD operations have one owner.
inline float board_battery_mv = 0;
inline uint8_t board_battery_level = 0;
inline bool board_battery_valid = false;

inline bool board_battery_update() {
  static uint32_t previous = 0;
  static bool first = true;
  static bool sampled = false;
  const uint32_t now = millis();
  if (sampled && uint32_t(now - previous) < BOARD_BATTERY_PERIOD_MS) return false;
  previous = now;
  sampled = true;
  uint32_t sum = 0;
  for (int i = 0; i < 16; ++i) sum += analogReadMilliVolts(BOARD_BATTERY_PIN);
  const float measured = (sum / 16.0f) * 2.0f * BOARD_BATTERY_GAIN
                       + BOARD_BATTERY_OFFSET_MV;
  board_battery_valid = measured >= 2500 && measured <= 4500;
  if (board_battery_valid) {
    board_battery_mv = first ? measured : (0.8f * board_battery_mv + 0.2f * measured);
    const uint8_t percent = camera::battery_percent(uint16_t(board_battery_mv));
    if (first || abs(int(percent) - int(board_battery_level)) >= 2 || percent == 0 || percent == 100)
      board_battery_level = percent;
    first = false;
  }
  Serial.printf("BAT: %.0f mV, %u%% estimated%s\n", measured,
      board_battery_level, board_battery_valid ? "" : " (invalid ADC voltage)");
  return true;
}

inline void board_draw_battery() {
  // Replaces the upstream Output[ms] field; leaves camera/settings icons intact.
  screen.fillRect(258, 80, 62, 26, TFT_BLACK);
  screen.setTextDatum(TL_DATUM);
  screen.setTextSize(1);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  screen.drawString("PIN", 260, 80);
  const uint16_t color = board_battery_level <= 20 ? TFT_RED : TFT_GREEN;
  screen.drawRect(260, 94, 20, 10, TFT_WHITE);
  screen.fillRect(280, 97, 2, 4, TFT_WHITE);
  if (board_battery_valid) screen.fillRect(262, 96, 16 * board_battery_level / 100, 6, color);
  char label[8];
  if (board_battery_valid) snprintf(label, sizeof(label), "%u%%", board_battery_level);
  else snprintf(label, sizeof(label), "--%%");
  screen.drawString(label, 286, 95);
  screen.setTextSize(2);
}

inline void board_ui_update(State_t current, bool new_frame) {
  static State_t previous = STATE_ON;
  static bool displayed_frame = false;
  static bool error_drawn = false;
  static const char* toast = nullptr;
  static uint32_t toast_until = 0;
  static bool toast_drawn = false;
  const bool changed = current != previous;
  const bool battery_changed = board_battery_update();
  if (changed) { displayed_frame = false; error_drawn = false; toast = nullptr; }
  if (new_frame) { displayed_frame = true; error_drawn = false; toast_drawn = false; }

  if (current == STATE_MAIN) {
    if (changed || battery_changed) { board_draw_battery(); screen_dirty=true; }
    if (board_sensor_error.load() != 0 && !error_drawn) {
      screen.fillRect(0, 0, 256, 192, TFT_BLACK);
      screen.setTextSize(1);
      screen.setTextColor(TFT_RED, TFT_BLACK);
      screen.drawString("MLX90640: no valid frame", 12, 65);
      screen.setTextColor(TFT_WHITE, TFT_BLACK);
      screen.drawString("Check 3V3/GND, SDA16/SCL15", 12, 85);
      screen.drawString("Retrying automatically...", 12, 105);
      screen.setTextSize(2);
      screen_dirty=true;
      error_drawn = true;
      displayed_frame = false;
    }
  }

  if (board_capture_pending.exchange(false)) {
    if (current != STATE_MAIN) {
      Serial.println("Shutter ignored: return to camera screen first.");
    } else {
      const bool valid = displayed_frame && board_sensor_error.load() == 0;
      const bool saved = valid && sdcard_save();
      toast = !valid ? "No valid thermal frame" : (saved ? "Saved to MicroSD" : "SD missing / write failed");
      Serial.println(toast);
      toast_until = millis() + 1500;
      toast_drawn = false;
    }
  }
  if (current == STATE_MAIN && toast && int32_t(toast_until - millis()) > 0 && !toast_drawn) {
    screen.fillRect(4, 173, 248, 15, TFT_BLACK);
    screen.setTextSize(1);
    screen.setTextColor(TFT_WHITE, TFT_BLACK);
    screen.drawString(toast, 8, 176);
    screen.setTextSize(2);
    screen_dirty=true;
    toast_drawn = true;
  }
  if(board_video_toggle_pending.exchange(false)) {
    if(video_recorder.active()) {
      screen.fillRect(4,173,248,15,TFT_BLACK);screen.setTextSize(1);screen.drawString("Saving MP4...",8,176);screen.setTextSize(2);screen_dirty=true;gfx_flush();
      mlx_cap.recording=false;
      toast=video_recorder.stop()?"MP4 saved":"MP4 save failed";
    } else if(current==STATE_MAIN && displayed_frame) {
      screen.fillRect(4,173,248,15,TFT_BLACK);screen.setTextSize(1);screen.drawString("Starting MP4...",8,176);screen.setTextSize(2);screen_dirty=true;gfx_flush();
      mlx_cap.capture_mode=1;
      mlx_cap.recording=video_recorder.begin();
      toast=mlx_cap.recording?"Recording full-screen MP4":"MP4 start failed (check SD)";
    } else toast="Return to a valid camera frame";
    if(current==STATE_MAIN)onMainCapture(widget_main+4,doInit);
    toast_until=millis()+1500; toast_drawn=false; screen_dirty=true;
  }
  if(video_recorder.active() && (current!=STATE_MAIN || !mlx_cap.recording || video_recorder.failed())) {
    mlx_cap.recording=false;
    toast=video_recorder.stop()?"MP4 saved":"MP4 save failed";
    toast_until=millis()+1500;toast_drawn=false;
    if(current==STATE_MAIN)onMainCapture(widget_main+4,doInit);
    screen_dirty=true;
  }
  previous = current;
}
