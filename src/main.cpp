#include <Arduino.h>
#include <time.h>

#include "app_state.h"
#include "board_config.h"
#include "epd_driver.h"
#include "epd_ui.h"
#include "status_led.h"
#include "storage_manager.h"
#include "weather_client.h"
#include "web_config_server.h"
#include "wifi_manager.h"

namespace {
constexpr const char* FIRMWARE_NAME = "EPD Weather Todo Panel";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t NTP_TIMEOUT_MS = 10000;
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 5000;
constexpr uint32_t TIME_SYNC_RETRY_INTERVAL_MS = 5UL * 60UL * 1000UL;

uint32_t last_status_log_ms = 0;
uint32_t last_weather_update_ms = 0;
uint32_t last_screen_update_ms = 0;
uint32_t last_wifi_check_ms = 0;
uint32_t last_time_sync_attempt_ms = 0;

String current_time_string() {
  struct tm info;
  if (!getLocalTime(&info, 20)) {
    return String("NO TIME");
  }
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &info);
  return String(buf);
}

bool sync_ntp_time(uint32_t timeout_ms) {
  if (!wifi_is_connected()) {
    app_state_set_error("NTP skipped: Wi-Fi not connected");
    return false;
  }

  last_time_sync_attempt_ms = millis();
  Serial.println("[TIME] configTime UTC+8");
  configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com", "time.windows.com");

  const uint32_t start_ms = millis();
  struct tm info;
  while (!getLocalTime(&info, 100)) {
    if (millis() - start_ms >= timeout_ms) {
      app_state_set_error("NTP sync timeout");
      return false;
    }
    web_server_handle_client();
    delay(20);
  }

  app_state.time_valid = true;
  app_state.last_update_time = current_time_string();
  Serial.printf("[TIME] Synced: %s\n", app_state.last_update_time.c_str());
  return true;
}

void refresh_network_state() {
  app_state.wifi_connected = wifi_is_connected();
  app_state.local_ip = wifi_get_local_ip();
  app_state.ap_ip = wifi_get_ap_ip();
}

void update_weather_if_possible() {
  if (!wifi_is_connected()) {
    return;
  }

  Serial.println("[MAIN] Weather update requested");
  if (weather_fetch(app_config, weather_data)) {
    app_state.weather_valid = true;
    app_state.weather_update_required = false;
    app_state.screen_update_required = true;
    app_state.last_update_time = current_time_string();
    last_weather_update_ms = millis();
  } else {
    app_state.weather_valid = false;
    app_state.weather_update_required = false;
    last_weather_update_ms = millis();
  }
}

void reconnect_wifi_if_requested() {
  if (!app_state.wifi_reconnect_required) {
    return;
  }

  Serial.println("[MAIN] Wi-Fi reconnect requested by config change");
  app_state.wifi_reconnect_required = false;
  app_state.time_valid = false;
  app_state.weather_valid = false;

  const bool sta_ok = wifi_connect_sta(app_config, WIFI_CONNECT_TIMEOUT_MS);
  refresh_network_state();
  wifi_print_status();

  if (!sta_ok) {
    app_state_set_error("Wi-Fi reconnect failed");
    app_state.screen_update_required = true;
    return;
  }

  app_state.last_error = "";
  sync_ntp_time(NTP_TIMEOUT_MS);
  update_weather_if_possible();
  app_state.screen_update_required = true;
}

void sync_time_if_needed(uint32_t now_ms) {
  if (!wifi_is_connected() || app_state.time_valid) {
    return;
  }
  if (last_time_sync_attempt_ms != 0 && now_ms - last_time_sync_attempt_ms < TIME_SYNC_RETRY_INTERVAL_MS) {
    return;
  }

  Serial.println("[MAIN] Time sync retry requested");
  if (sync_ntp_time(NTP_TIMEOUT_MS)) {
    app_state.screen_update_required = true;
  }
}

void print_startup_info() {
  Serial.println();
  Serial.println("========================================");
  Serial.printf("%s\n", FIRMWARE_NAME);
  Serial.printf("Board: %s\n", BOARD_NAME);
  Serial.printf("EPD size: %ux%u profile=%d\n", EPD_WIDTH, EPD_HEIGHT, EPD_DRIVER_PROFILE);
  Serial.println("========================================");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
#if BOARD_RGB_LED_SCAN_MODE
  status_led_scan_forever();
#endif
  status_led_begin();
  status_led_probe("serial", 96, 0, 0);
  print_startup_info();

  app_config_set_defaults(app_config);
  app_state_reset();
  weather_data_reset();
  status_led_probe("state-reset", 0, 96, 0);

  if (!storage_begin()) {
    app_state_set_error("Storage begin failed");
  }
  load_app_config(app_config);
  load_todo_items(app_config);
  status_led_probe("storage", 0, 0, 96);

  if (!epd_begin() || !epd_init_ssd1619_4in2_tricolor()) {
    app_state_set_error("EPD init failed");
  } else {
    epd_ui_render_boot_screen("BOOTING");
  }
  status_led_probe("epd", 96, 96, 0);

  bool sta_ok = wifi_connect_sta(app_config, WIFI_CONNECT_TIMEOUT_MS);
  app_state.wifi_connected = sta_ok;
  app_state.local_ip = wifi_get_local_ip();
  status_led_probe("wifi-sta", 0, 96, 96);

  if (!sta_ok || app_config.ap_mode_always_on) {
    app_state.ap_running = wifi_start_ap();
    app_state.ap_ip = wifi_get_ap_ip();
  }
  status_led_probe("wifi-ap", 96, 0, 96);

  wifi_print_status();
  web_server_begin(app_config);
  status_led_probe("web", 96, 48, 0);

  if (sta_ok) {
    sync_ntp_time(NTP_TIMEOUT_MS);
    status_led_probe("ntp", 48, 96, 0);
    update_weather_if_possible();
    status_led_probe("weather", 0, 48, 96);
  } else {
    app_state_set_error("Use AP EPD-Config at 192.168.4.1");
  }

  app_state.screen_update_required = true;
}

void loop() {
  const uint32_t now_ms = millis();
  status_led_update(now_ms);
  web_server_handle_client();

  reconnect_wifi_if_requested();

  if (now_ms - last_wifi_check_ms >= 10000) {
    last_wifi_check_ms = now_ms;
    refresh_network_state();
  }

  sync_time_if_needed(now_ms);

  const uint32_t weather_interval_ms = app_config.weather_update_interval_min * 60UL * 1000UL;
  if (weather_interval_ms > 0 && now_ms - last_weather_update_ms >= weather_interval_ms) {
    app_state.weather_update_required = true;
  }

  if (app_state.weather_update_required) {
    update_weather_if_possible();
  }

  const uint32_t screen_interval_ms = app_config.screen_update_interval_min * 60UL * 1000UL;
  if (screen_interval_ms > 0 && now_ms - last_screen_update_ms >= screen_interval_ms) {
    app_state.screen_update_required = true;
  }

  if (app_state.screen_update_required) {
    Serial.println("[MAIN] Screen update requested");
    if (epd_ui_render_dashboard(app_config, app_state, weather_data)) {
      app_state.screen_update_required = false;
      app_state.last_update_time = current_time_string();
      last_screen_update_ms = millis();
    } else {
      app_state_set_error("Screen render failed");
    }
  }

  if (now_ms - last_status_log_ms >= STATUS_LOG_INTERVAL_MS) {
    last_status_log_ms = now_ms;
    Serial.printf("[MAIN] running wifi=%u ap=%u weather=%u time=%u local=%s ap=%s\n", app_state.wifi_connected,
                  app_state.ap_running, app_state.weather_valid, app_state.time_valid, app_state.local_ip.c_str(),
                  app_state.ap_ip.c_str());
  }
}
