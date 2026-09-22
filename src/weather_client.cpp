#include "weather_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char* weather_code_to_text(int code) {
  switch (code) {
    case 0:
      return "Clear";
    case 1:
      return "Mainly clear";
    case 2:
      return "Partly cloudy";
    case 3:
      return "Overcast";
    case 45:
      return "Fog";
    case 48:
      return "Rime fog";
    case 51:
    case 53:
    case 55:
      return "Drizzle";
    case 61:
    case 63:
    case 65:
      return "Rain";
    case 71:
    case 73:
    case 75:
      return "Snow";
    case 80:
    case 81:
    case 82:
      return "Rain showers";
    case 95:
      return "Thunderstorm";
    default:
      return "Unknown";
  }
}

bool weather_fetch(const AppConfig& cfg, WeatherData& out) {
  if (WiFi.status() != WL_CONNECTED) {
    app_state_set_error("Weather fetch skipped: Wi-Fi not connected");
    return false;
  }

  char url[320];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max&timezone=auto&forecast_days=1",
           cfg.latitude, cfg.longitude);

  Serial.println("[WEATHER] Requesting forecast");
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(url)) {
    app_state_set_error("HTTP begin failed");
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    char error[64];
    snprintf(error, sizeof(error), "Weather HTTP status %d", status);
    app_state_set_error(error);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError json_error = deserializeJson(doc, payload);
  if (json_error) {
    char error[96];
    snprintf(error, sizeof(error), "Weather JSON parse failed: %s", json_error.c_str());
    app_state_set_error(error);
    return false;
  }

  out.current_temp = doc["current"]["temperature_2m"].as<float>();
  out.weather_code = doc["current"]["weather_code"].as<int>();
  out.wind_speed = doc["current"]["wind_speed_10m"].as<float>();
  out.temp_max = doc["daily"]["temperature_2m_max"][0].as<float>();
  out.temp_min = doc["daily"]["temperature_2m_min"][0].as<float>();
  out.precipitation_probability = doc["daily"]["precipitation_probability_max"][0].as<int>();
  out.weather_text = weather_code_to_text(out.weather_code);
  out.weather_text.reserve(24);

  Serial.printf("[WEATHER] OK temp=%.1f max=%.1f min=%.1f code=%d text=%s rain=%d wind=%.1f\n",
                out.current_temp, out.temp_max, out.temp_min, out.weather_code, out.weather_text.c_str(),
                out.precipitation_probability, out.wind_speed);
  return true;
}
