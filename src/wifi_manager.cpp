#include "wifi_manager.h"

#include <WiFi.h>

namespace {
constexpr const char* AP_SSID = "EPD-Config";
constexpr const char* AP_PASSWORD = "12345678";
}

bool wifi_connect_sta(const AppConfig& cfg, uint32_t timeout_ms) {
  if (strlen(cfg.wifi_ssid) == 0) {
    Serial.println("[WIFI] Empty SSID, skip STA connect");
    return false;
  }

  Serial.printf("[WIFI] Connecting STA timeout=%lu ms\n", static_cast<unsigned long>(timeout_ms));
  const wifi_mode_t current_mode = WiFi.getMode();
  WiFi.mode((current_mode == WIFI_AP || current_mode == WIFI_AP_STA) ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(cfg.wifi_ssid, cfg.wifi_password);

  const uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start_ms >= timeout_ms) {
      Serial.println("[WIFI] STA connect timeout");
      return false;
    }
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("[WIFI] STA connected, IP=%s\n", WiFi.localIP().toString().c_str());
  return true;
}

bool wifi_start_ap() {
  Serial.printf("[WIFI] Starting AP SSID=%s\n", AP_SSID);
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("[WIFI] ERROR: softAP failed");
    return false;
  }
  Serial.printf("[WIFI] AP started, IP=%s\n", WiFi.softAPIP().toString().c_str());
  return true;
}

void wifi_stop_ap() {
  Serial.println("[WIFI] Stop AP");
  WiFi.softAPdisconnect(true);
}

bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifi_get_local_ip() {
  return wifi_is_connected() ? WiFi.localIP().toString() : "";
}

String wifi_get_ap_ip() {
  IPAddress ip = WiFi.softAPIP();
  if (ip == IPAddress(0, 0, 0, 0)) {
    return "";
  }
  return ip.toString();
}

void wifi_print_status() {
  Serial.printf("[WIFI] status=%d sta=%u local_ip=%s ap_ip=%s\n", WiFi.status(), wifi_is_connected(),
                wifi_get_local_ip().c_str(), wifi_get_ap_ip().c_str());
}
