#pragma once

#include <Arduino.h>

// =========================
// Board selection
// =========================
#define BOARD_NANO_ESP32_C6 1

#ifndef EPD_TARGET_BOARD
#define EPD_TARGET_BOARD BOARD_NANO_ESP32_C6
#endif

// =========================
// Bring-up profile
// =========================
// 0 = baseline template
// 1 = invert color-plane polarity only
// 2 = swap BW/color RAM commands
// 3 = swap BW/color RAM commands + invert color-plane polarity
//
// The current verified panel works with profile 1.
#define EPD_DRIVER_PROFILE 1

// =========================
// Board-specific GPIO map
// =========================
#if EPD_TARGET_BOARD == BOARD_NANO_ESP32_C6
static constexpr const char* BOARD_NAME = "MuseLab nanoESP32-C6 (EPD validation)";
static constexpr int8_t EPD_SPI_SCK = 4;
static constexpr int8_t EPD_SPI_MOSI = 6;
static constexpr int8_t EPD_SPI_MISO = -1;
static constexpr int8_t EPD_CS1 = 7;
static constexpr int8_t EPD_CS2 = -1;
static constexpr int8_t EPD_DC = 5;
static constexpr int8_t EPD_RST = 10;
static constexpr int8_t EPD_BUSY = 3;
static constexpr uint32_t EPD_SPI_HZ = 4000000;
static constexpr uint8_t EPD_BUSY_ACTIVE_LEVEL = LOW;
static constexpr int8_t BOARD_RGB_LED_PIN = 8;
static constexpr uint8_t BOARD_RGB_LED_ENABLED = 1;
// Set to 1 only when debugging LED startup stages.
static constexpr uint8_t BOARD_RGB_LED_BOOT_PROBE_ENABLED = 0;
// Temporary diagnostic mode: when enabled, firmware only scans candidate RGB LED GPIOs.
#define BOARD_RGB_LED_SCAN_MODE 0
#else
#error Unsupported EPD_TARGET_BOARD
#endif

// =========================
// Panel properties
// =========================
static constexpr uint16_t EPD_WIDTH = 400;
static constexpr uint16_t EPD_HEIGHT = 300;
static constexpr uint8_t EPD_IS_TRICOLOR = 1;

// =========================
// Driver template tuning
// =========================
static constexpr uint8_t EPD_REFRESH_SETUP_COMMAND = 0x22;
static constexpr uint8_t EPD_REFRESH_COMMAND = 0x20;
static constexpr uint8_t EPD_REFRESH_SETUP_DATA = 0xF7;

#if EPD_DRIVER_PROFILE == 0
static constexpr uint8_t EPD_RAM_BW_COMMAND = 0x24;
static constexpr uint8_t EPD_RAM_COLOR_COMMAND = 0x26;
static constexpr uint8_t EPD_BW_ACTIVE_IS_ZERO = 1;
static constexpr uint8_t EPD_COLOR_ACTIVE_IS_ZERO = 1;
#elif EPD_DRIVER_PROFILE == 1
static constexpr uint8_t EPD_RAM_BW_COMMAND = 0x24;
static constexpr uint8_t EPD_RAM_COLOR_COMMAND = 0x26;
static constexpr uint8_t EPD_BW_ACTIVE_IS_ZERO = 1;
static constexpr uint8_t EPD_COLOR_ACTIVE_IS_ZERO = 0;
#elif EPD_DRIVER_PROFILE == 2
static constexpr uint8_t EPD_RAM_BW_COMMAND = 0x26;
static constexpr uint8_t EPD_RAM_COLOR_COMMAND = 0x24;
static constexpr uint8_t EPD_BW_ACTIVE_IS_ZERO = 1;
static constexpr uint8_t EPD_COLOR_ACTIVE_IS_ZERO = 1;
#elif EPD_DRIVER_PROFILE == 3
static constexpr uint8_t EPD_RAM_BW_COMMAND = 0x26;
static constexpr uint8_t EPD_RAM_COLOR_COMMAND = 0x24;
static constexpr uint8_t EPD_BW_ACTIVE_IS_ZERO = 1;
static constexpr uint8_t EPD_COLOR_ACTIVE_IS_ZERO = 0;
#else
#error Unsupported EPD_DRIVER_PROFILE
#endif

// Diagnostics:
// - Keep step delays visible enough that you can distinguish refresh phases.
static constexpr uint32_t EPD_DIAGNOSTIC_STEP_DELAY_MS = 1500;
