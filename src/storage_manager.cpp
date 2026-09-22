#include "storage_manager.h"

#include <Preferences.h>

namespace {
Preferences prefs;
constexpr const char* STORAGE_NS = "epd_panel";

void save_string_key(const char* key, const char* value) {
  prefs.putString(key, value == nullptr ? "" : value);
}

void load_string_key(const char* key, char* dest, size_t len, const char* fallback = "") {
  String value = prefs.getString(key, fallback);
  strlcpy(dest, value.c_str(), len);
}
}  // namespace

bool storage_begin() {
  Serial.println("[STORAGE] Begin Preferences");
  if (!prefs.begin(STORAGE_NS, false)) {
    Serial.println("[STORAGE] ERROR: Preferences begin failed");
    return false;
  }
  return true;
}

void reset_config_to_default(AppConfig& cfg) {
  Serial.println("[STORAGE] Reset config to defaults");
  app_config_set_defaults(cfg);
}

bool load_app_config(AppConfig& cfg) {
  const bool initialized = prefs.getBool("initialized", false);
  if (!initialized) {
    Serial.println("[STORAGE] First boot, writing default config");
    reset_config_to_default(cfg);
    if (!save_app_config(cfg) || !save_todo_items(cfg)) {
      return false;
    }
    return true;
  }

  Serial.println("[STORAGE] Loading app config");
  app_config_set_defaults(cfg);
  load_string_key("wifi_ssid", cfg.wifi_ssid, sizeof(cfg.wifi_ssid));
  load_string_key("wifi_password", cfg.wifi_password, sizeof(cfg.wifi_password));
  load_string_key("city_name", cfg.city_name, sizeof(cfg.city_name), "Example City");
  cfg.latitude = prefs.getFloat("latitude", 0.0f);
  cfg.longitude = prefs.getFloat("longitude", 0.0f);
  cfg.weather_update_interval_min = prefs.getUInt("weather_min", 60);
  cfg.screen_update_interval_min = prefs.getUInt("screen_min", 60);
  cfg.ap_mode_always_on = prefs.getBool("ap_always", false);
  app_config_sanitize(cfg);
  Serial.printf("[STORAGE] Config loaded: ssid_set=%u weather_min=%lu screen_min=%lu ap_always=%u\n",
                strlen(cfg.wifi_ssid) > 0,
                static_cast<unsigned long>(cfg.weather_update_interval_min),
                static_cast<unsigned long>(cfg.screen_update_interval_min), cfg.ap_mode_always_on);
  return true;
}

bool save_app_config(const AppConfig& cfg) {
  Serial.printf("[STORAGE] Saving app config: ssid_set=%u\n", strlen(cfg.wifi_ssid) > 0);
  prefs.putBool("initialized", true);
  save_string_key("wifi_ssid", cfg.wifi_ssid);
  save_string_key("wifi_password", cfg.wifi_password);
  save_string_key("city_name", cfg.city_name);
  prefs.putFloat("latitude", cfg.latitude);
  prefs.putFloat("longitude", cfg.longitude);
  prefs.putUInt("weather_min", cfg.weather_update_interval_min);
  prefs.putUInt("screen_min", cfg.screen_update_interval_min);
  prefs.putBool("ap_always", cfg.ap_mode_always_on);
  return true;
}

bool load_todo_items(AppConfig& cfg) {
  Serial.println("[STORAGE] Loading todo items");
  cfg.todo_count = prefs.getUChar("todo_count", 0);
  if (cfg.todo_count > APP_MAX_TODOS) {
    cfg.todo_count = APP_MAX_TODOS;
  }

  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    char key[12];
    snprintf(key, sizeof(key), "todo%u", i);
    load_string_key(key, cfg.todo_items[i], APP_TODO_MAX_LEN);
  }
  Serial.printf("[STORAGE] Loaded %u todo items\n", cfg.todo_count);
  return true;
}

bool save_todo_items(const AppConfig& cfg) {
  Serial.printf("[STORAGE] Saving %u todo items\n", cfg.todo_count);
  prefs.putUChar("todo_count", cfg.todo_count);
  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    char key[12];
    snprintf(key, sizeof(key), "todo%u", i);
    save_string_key(key, cfg.todo_items[i]);
  }
  return true;
}
