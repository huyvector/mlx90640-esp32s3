#pragma once
#include <Arduino.h>
#include <atomic>
#include "board_config.h"
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Both FT6336G and the vendored MLX driver use this ONE Wire bus/lock.
// No LovyanGFX I2C driver is attached to this panel.
inline SemaphoreHandle_t board_i2c_mutex = nullptr;
inline TaskHandle_t board_touch_task_handle=nullptr;
inline void board_yield_to_touch(){if(board_touch_task_handle)xTaskNotifyGive(board_touch_task_handle);delay(1);}
inline std::atomic<uint32_t> board_sensor_hz{BOARD_SENSOR_SAFE_HZ};
inline std::atomic<uint32_t> board_i2c_errors{0};
inline std::atomic<bool> board_reference_acquisition{true};
inline std::atomic<bool> board_diagnostics_pending{true};

class BoardI2CLock {
 public:
  explicit BoardI2CLock(TickType_t timeout = portMAX_DELAY)
      : held_(board_i2c_mutex &&
              xSemaphoreTakeRecursive(board_i2c_mutex, timeout) == pdTRUE) {}
  ~BoardI2CLock() {
    if (held_) xSemaphoreGiveRecursive(board_i2c_mutex);
  }
  explicit operator bool() const { return held_; }
  BoardI2CLock(const BoardI2CLock&) = delete;
  BoardI2CLock& operator=(const BoardI2CLock&) = delete;
 private:
  bool held_;
};

inline bool board_bus_setup(int sda, int scl, uint32_t frequency) {
  board_i2c_mutex = xSemaphoreCreateRecursiveMutex();
  if (!board_i2c_mutex) return false;
  if (Wire.setBufferSize(BOARD_WIRE_BUFFER_SIZE) < BOARD_WIRE_BUFFER_SIZE) return false;
  Wire.setTimeOut(50);
  return Wire.begin(sda, scl, frequency);
}

inline bool board_read_register(uint8_t address, uint8_t reg,
                                uint8_t* data, size_t length) {
  BoardI2CLock lock(0); // UI skips this poll rather than stalling behind a sensor burst.
  if (!lock) return false;
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(address, length) != length) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < length; ++i) data[i] = Wire.read();
  return true;
}

inline bool board_write_register(uint8_t address, uint8_t reg, uint8_t value) {
  BoardI2CLock lock;
  if (!lock) return false;
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Holds one sensor snapshot, restores the normal clock before touch can use Wire.
class BoardSensorBurst {
  BoardI2CLock lock_;
 public:
  BoardSensorBurst() { if (lock_) Wire.setClock(board_sensor_hz.load()); }
  ~BoardSensorBurst() { if (lock_) Wire.setClock(BOARD_I2C_HZ); }
  explicit operator bool() const { return bool(lock_); }
};
