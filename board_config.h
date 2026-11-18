#pragma once

// LCDWIKI ES3C28P (ESP32-S3 N16R8, ILI9341V, FT6336G).
// Change this only when intentionally building an original upstream target.
#define LCDWIKI_ES3C28P 1
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "This firmware targets the ESP32-S3 ES3C28P, not ESP32-S2 or classic CYD."
#endif

#define BOARD_I2C_HZ        400000
#define BOARD_TOUCH_ADDR    0x38
#define BOARD_TOUCH_RST     18
#define BOARD_TOUCH_INT     17
#define BOARD_BATTERY_PIN   9
#define BOARD_AUDIO_EN      1
#define BOARD_ROTATION      3
// This board mounts the FT6336G axes 180 degrees from the LCD axes.
#define BOARD_TOUCH_ROTATION ((BOARD_ROTATION + 2) & 3)
#define BOARD_LCD_INVERT    true
#define BOARD_LCD_RGB_ORDER false
#define BOARD_BRIGHTNESS    160
#define BOARD_SENSOR_OUTWARD true

// 200k/200k onboard divider. Adjust GAIN/OFFSET after checking a multimeter.
#define BOARD_BATTERY_GAIN      1.0f
#define BOARD_BATTERY_OFFSET_MV 0.0f
#define BOARD_BATTERY_PERIOD_MS 2000


// Experimental sensor-only clock. Touch/control transactions always return to 400k.
#define BOARD_SENSOR_FAST_HZ 400000
#define BOARD_SENSOR_SAFE_HZ 400000
#define BOARD_WIRE_BUFFER_SIZE 2048
#define BOARD_VIDEO_QUALITY 70
#define BOARD_VIDEO_MAX_FRAMES 7200
