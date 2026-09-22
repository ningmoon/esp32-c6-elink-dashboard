#include "app_state.h"

AppConfig app_config;
AppState app_state;
WeatherData weather_data;

void app_config_set_defaults(AppConfig& cfg) {
  memset(&cfg, 0, sizeof(cfg));
  strlcpy(cfg.city_name, "Example City", sizeof(cfg.city_name));
  cfg.latitude = 0.0f;
  cfg.longitude = 0.0f;
  cfg.weather_update_interval_min = 60;
  cfg.screen_update_interval_min = 60;
  cfg.ap_mode_always_on = false;
  cfg.todo_count = 0;
}

void app_config_sanitize(AppConfig& cfg) {
  cfg.wifi_ssid[sizeof(cfg.wifi_ssid) - 1] = '\0';
  cfg.wifi_password[sizeof(cfg.wifi_password) - 1] = '\0';
  cfg.city_name[sizeof(cfg.city_name) - 1] = '\0';

  if (cfg.weather_update_interval_min == 0) {
    cfg.weather_update_interval_min = 60;
  }
  if (cfg.screen_update_interval_min == 0) {
    cfg.screen_update_interval_min = 60;
  }
  if (cfg.todo_count > APP_MAX_TODOS) {
    cfg.todo_count = APP_MAX_TODOS;
  }
  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    cfg.todo_items[i][APP_TODO_MAX_LEN - 1] = '\0';
  }
}

void app_state_reset() {
  app_state.wifi_connected = false;
  app_state.ap_running = false;
  app_state.weather_valid = false;
  app_state.time_valid = false;
  app_state.wifi_reconnect_required = false;
  app_state.screen_update_required = false;
  app_state.weather_update_required = false;
  app_state.local_ip = "";
  app_state.ap_ip = "";
  app_state.last_error = "";
  app_state.last_update_time = "";
}

void weather_data_reset() {
  weather_data.current_temp = 0.0f;
  weather_data.temp_max = 0.0f;
  weather_data.temp_min = 0.0f;
  weather_data.weather_code = -1;
  weather_data.precipitation_probability = 0;
  weather_data.wind_speed = 0.0f;
  weather_data.weather_text = "Unknown";
}

void app_state_set_error(const char* message) {
  app_state.last_error = message == nullptr ? "" : message;
  if (app_state.last_error.length() > 96) {
    app_state.last_error = app_state.last_error.substring(0, 96);
  }
  Serial.printf("[STATE] Error: %s\n", app_state.last_error.c_str());
}
