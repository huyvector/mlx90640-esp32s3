#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
 public:
  LGFX() {
    auto bus = bus_.config();
    bus.spi_host = SPI2_HOST;
    bus.spi_mode = 0;
    bus.freq_write = SPI_FREQUENCY;
    bus.freq_read = SPI_READ_FREQUENCY;
    bus.spi_3wire = false;
    bus.use_lock = true;
    bus.dma_channel = SPI_DMA_CH_AUTO;
    bus.pin_sclk = TFT_SCLK;
    bus.pin_mosi = TFT_MOSI;
    bus.pin_miso = TFT_MISO;
    bus.pin_dc = TFT_DC;
    bus_.config(bus);
    panel_.setBus(&bus_);

    auto panel = panel_.config();
    panel.pin_cs = TFT_CS;
    panel.pin_rst = -1;  // LCD reset shares ESP32 EN: never drive it as a GPIO.
    panel.pin_busy = -1;
    panel.memory_width = panel.panel_width = 240;
    panel.memory_height = panel.panel_height = 320;
    panel.offset_x = panel.offset_y = panel.offset_rotation = 0;
    panel.dummy_read_pixel = 8;
    panel.dummy_read_bits = 1;
    panel.readable = true;
    panel.invert = BOARD_LCD_INVERT;
    panel.rgb_order = BOARD_LCD_RGB_ORDER;
    panel.dlen_16bit = false;
    panel.bus_shared = false; // SD uses SD_MMC, not this SPI host.
    panel_.config(panel);

    auto light = light_.config();
    light.pin_bl = TFT_BL;
    light.invert = false;
    light.freq = 12000;
    light.pwm_channel = 7;
    light_.config(light);
    panel_.setLight(&light_);
    setPanel(&panel_);
  }
};
