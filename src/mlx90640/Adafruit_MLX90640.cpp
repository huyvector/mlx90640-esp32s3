#include "Adafruit_MLX90640.h"
#include "../../board_bus.hpp"

/*!
 *    @brief  Instantiates a new MLX90640 class
 */
Adafruit_MLX90640::Adafruit_MLX90640(void) {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_addr
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
boolean Adafruit_MLX90640::begin(uint8_t i2c_addr, TwoWire *wire) {
  wire_ = wire; address_ = i2c_addr;
  delete i2c_dev; // begin() may be retried after a disconnected sensor.
  i2c_dev = new Adafruit_I2CDevice(i2c_addr, wire);

  if (!i2c_dev->begin()) {
    return false;
  }
  // wire->setClock(400000); // Speed it up, lots to read :)
  MLX90640_I2CRead(0, MLX90640_DEVICEID1, 3, serialNumber);

  uint16_t eeMLX90640[832];
  if (MLX90640_DumpEE(0, eeMLX90640) != 0) {
    return false;
  }
#ifdef MLX90640_DEBUG
  for (int i = 0; i < 832; i++) {
    Serial.printf("0x%x, ", eeMLX90640[i]);
  }
  Serial.println();
#endif

  if (MLX90640_ExtractParameters(eeMLX90640, &_params) != 0) return false;
  // whew!
  return true;
}

/*!
 *    @brief  Read nMemAddressRead words from I2C startAddress into data
 *    @param  slaveAddr Not used - kept to maintain backcompatible API
 *    @param  startAddress I2C memory address to start reading
 *    @param  nMemAddressRead 16-bit words to read
 *    @param  data Location to place data read
 *    @return 0 on success
 */
int Adafruit_MLX90640::MLX90640_I2CRead(uint8_t slaveAddr,
                                        uint16_t startAddress,
                                        uint16_t nMemAddressRead,
                                        uint16_t *data) {
  BoardI2CLock lock;
  if (!lock) return -10;
  if (nMemAddressRead > BOARD_WIRE_BUFFER_SIZE/2) return -10;
  wire_->beginTransmission(address_);
  wire_->write(uint8_t(startAddress >> 8));
  wire_->write(uint8_t(startAddress));
  if (wire_->endTransmission(false) != 0 ||
      wire_->requestFrom(address_, size_t(nMemAddressRead)*2) != size_t(nMemAddressRead)*2) {
    while (wire_->available()) wire_->read();
    board_i2c_errors.fetch_add(1);
    return -1;
  }
  for (uint16_t i = 0; i < nMemAddressRead; ++i) {
    const uint16_t hi = wire_->read();
    data[i] = (hi << 8) | wire_->read();
  }
  return 0;
}

int Adafruit_MLX90640::MLX90640_I2CWrite(uint8_t slaveAddr,
                                         uint16_t writeAddress, uint16_t data) {
  BoardI2CLock lock;
  if (!lock) return -10;
  uint8_t cmd[4];
  uint16_t dataCheck = 0;

  cmd[0] = writeAddress >> 8;
  cmd[1] = writeAddress & 0x00FF;
  cmd[2] = data >> 8;
  cmd[3] = data & 0x00FF;

  if (!i2c_dev->write(cmd, 4, true)) {
    return -1;
  }
  // Status bits are volatile/read-only; do not compare them with the written value.
  if (writeAddress == 0x8000) return 0;
  if (writeAddress >= 0x2400 && writeAddress < 0x2740) delay(5);

  if (MLX90640_I2CRead(slaveAddr, writeAddress, 1, &dataCheck) != 0) {
    return -1;
  }

  // check echo
  if (dataCheck != data) {
    return -2;
  }
  // OK!
  return 0;
}

/*!
 *    @brief Get the frame-read mode
 *    @return Chess or interleaved mode
 */
mlx90640_mode_t Adafruit_MLX90640::getMode(void) {
  return (mlx90640_mode_t)MLX90640_GetCurMode(0);
}

/*!
 *    @brief Set the frame-read mode
 *    @param mode Chess or interleaved mode
 */
void Adafruit_MLX90640::setMode(mlx90640_mode_t mode) {
  if (mode == MLX90640_CHESS) {
    MLX90640_SetChessMode(0);
  } else {
    MLX90640_SetInterleavedMode(0);
  }
}

/*!
 *    @brief  Get resolution for temperature precision
 *    @returns The desired resolution (bits)
 */
mlx90640_resolution_t Adafruit_MLX90640::getResolution(void) {
  return (mlx90640_resolution_t)MLX90640_GetCurResolution(0);
}

/*!
 *    @brief  Set resolution for temperature precision
 *    @param res The desired resolution (bits)
 */
void Adafruit_MLX90640::setResolution(mlx90640_resolution_t res) {
  MLX90640_SetResolution(0, (int)res);
}

/*!
 *    @brief  Get max refresh rate
 *    @returns How many pages per second to read (2 pages per frame)
 */
mlx90640_refreshrate_t Adafruit_MLX90640::getRefreshRate(void) {
  return (mlx90640_refreshrate_t)MLX90640_GetRefreshRate(0);
}

/*!
 *    @brief  Set max refresh rate - too fast and we can't read the
 *    the pages in time, start low and then increment while speeding
 *    up I2C!
 *    @param rate How many pages per second to read (2 pages per frame)
 */
void Adafruit_MLX90640::setRefreshRate(mlx90640_refreshrate_t rate) {
  MLX90640_SetRefreshRate(0, (int)rate);
}

/*!
 *    @brief  Read 2 pages, calculate temperatures and place into framebuf
 *    @param  framebuf 24*32 floating point memory buffer
 *    @return 0 on success
 */
int Adafruit_MLX90640::getFrame(float *framebuf) {
  float emissivity = 0.95;
  float tr = 23.15;
  uint16_t mlx90640Frame[834];
  int status;

  lastReadUs = lastCalculateUs = 0;
  uint8_t pages_seen = 0;
  const uint32_t started = millis();
  while (pages_seen != 3) {
    if (uint32_t(millis() - started) > 2000) return -11;
    uint32_t phaseStart = micros();
    status = MLX90640_GetFrameData(0, mlx90640Frame);
    lastReadUs += uint32_t(micros() - phaseStart);

#ifdef MLX90640_DEBUG
    Serial.printf("Subpage%d = [", status);
    for (int i = 0; i < 834; i++) {
      Serial.printf("0x%x, ", mlx90640Frame[i]);
    }
    Serial.println("]");
#endif

    if (status < 0) {
      return status;
    }

    pages_seen |= 1u << (mlx90640Frame[833] & 1);
    ta = MLX90640_GetTa(mlx90640Frame, &_params); // Store ambient temp locally
    tr = ta - OPENAIR_TA_SHIFT; // For a MLX90640 in the open air the shift is
                                // -8 degC.
#ifdef MLX90640_DEBUG
    Serial.print("Tr = ");
    Serial.println(tr, 8);
#endif
    phaseStart = micros();
    MLX90640_CalculateTo(mlx90640Frame, &_params, emissivity, tr, framebuf);
    lastCalculateUs += uint32_t(micros() - phaseStart);
  }
  return 0;
}

/*!
 *    @brief  Return ambient temperature of the TO39 package.
 *    @param  newFrame If true, will also capture a new data frame. If false,
 * return the value from the last data frame read.
 *    @return Ambient temperature as a float in degrees Celsius.
 */
float Adafruit_MLX90640::getTa(bool newFrame) {
  if (!newFrame) {
    return ta;
  }
  uint16_t mlx90640Frame[834];
  MLX90640_GetFrameData(0, mlx90640Frame);
  return MLX90640_GetTa(mlx90640Frame, &_params);
}

bool Adafruit_MLX90640::enableDataHold(bool enabled) {
  uint16_t control;
  if (MLX90640_I2CRead(0, 0x800D, 1, &control) != 0) return false;
  // Control register bit2: only update RAM when status.en_overwrite is set.
  if(MLX90640_I2CWrite(0, 0x800D, enabled ? (control | 0x0004) : (control & ~0x0004))!=0)return false;
  // The MCU can reset while a previous firmware held RAM frozen. The sensor
  // stays powered, so explicitly re-arm RAM before waiting for data-ready.
  return MLX90640_I2CWrite(0,0x8000,0x0030)==0;
}
