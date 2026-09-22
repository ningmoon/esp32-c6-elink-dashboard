#pragma once

#include <Arduino.h>

#include "app_config.h"

struct AppState {
  bool wifi_connected;
  bool ap_running;
  bool weather_valid;
  bool time_valid;
  bool wifi_reconnect_required;
  bool screen_update_required;
  bool weather_update_required;
  String local_ip;
  String ap_ip;
  String last_error;
  String last_update_time;
};

struct WeatherData {
  float current_temp;
  float temp_max;
  float temp_min;
  int weather_code;
  int precipitation_probability;
  float wind_speed;
  String weather_text;
};

extern AppConfig app_config;
extern AppState app_state;
extern WeatherData weather_data;

void app_state_reset();
void weather_data_reset();
void app_state_set_error(const char* message);
