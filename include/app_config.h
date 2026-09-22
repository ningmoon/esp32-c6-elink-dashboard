#pragma once

#include <Arduino.h>
#include <stdint.h>

static constexpr uint8_t APP_MAX_TODOS = 5;
static constexpr uint8_t APP_TODO_MAX_LEN = 64;
static constexpr uint32_t APP_VERSION = 1;

struct AppConfig {
  char wifi_ssid[64];
  char wifi_password[64];
  char city_name[32];
  float latitude;
  float longitude;
  uint32_t weather_update_interval_min;
  uint32_t screen_update_interval_min;
  bool ap_mode_always_on;
  char todo_items[APP_MAX_TODOS][APP_TODO_MAX_LEN];
  uint8_t todo_count;
};

void app_config_set_defaults(AppConfig& cfg);
void app_config_sanitize(AppConfig& cfg);
