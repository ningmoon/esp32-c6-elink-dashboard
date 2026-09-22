#include "web_config_server.h"

#include <WebServer.h>

#include "storage_manager.h"

namespace {
WebServer server(80);
AppConfig* active_config = nullptr;

String html_escape(const String& src) {
  String out;
  out.reserve(src.length() + 8);
  for (size_t i = 0; i < src.length(); ++i) {
    char c = src[i];
    if (c == '&') {
      out += "&amp;";
    } else if (c == '<') {
      out += "&lt;";
    } else if (c == '>') {
      out += "&gt;";
    } else if (c == '"') {
      out += "&quot;";
    } else if (c == '\'') {
      out += "&#39;";
    } else {
      out += c;
    }
  }
  return out;
}

void log_request_args(const char* tag) {
  Serial.printf("[WEB] %s args=%d\n", tag, server.args());
  for (uint8_t i = 0; i < server.args(); ++i) {
    const String name = server.argName(i);
    // Form fields may contain credentials, locations, or private TODO text.
    Serial.printf("[WEB]   %s len=%u\n", name.c_str(), server.arg(i).length());
  }
}

String checked_attr(bool value) {
  return value ? " checked" : "";
}

void copy_arg(const char* name, char* dest, size_t len) {
  String value = server.arg(name);
  value.trim();
  strlcpy(dest, value.c_str(), len);
}

uint32_t arg_to_uint(const char* name, uint32_t fallback) {
  if (!server.hasArg(name)) {
    return fallback;
  }
  uint32_t value = static_cast<uint32_t>(server.arg(name).toInt());
  return value == 0 ? fallback : value;
}
}  // namespace

void web_server_begin(AppConfig& cfg) {
  active_config = &cfg;
  web_server_register_routes();
  server.begin();
  Serial.println("[WEB] Server started on port 80");
}

void web_server_handle_client() {
  server.handleClient();
}

void web_server_register_routes() {
  server.on("/", HTTP_GET, handle_root);
  server.on("/save_config", HTTP_POST, handle_save_config);
  server.on("/save_todos", HTTP_POST, handle_save_todos);
  server.on("/refresh_screen", HTTP_ANY, handle_refresh_screen);
  server.on("/api/status", HTTP_GET, handle_api_status);
  server.onNotFound(handle_not_found);
}

String render_main_page(const AppConfig& cfg, const AppState& state, const WeatherData& weather) {
  String page;
  page.reserve(7200);
  page += F("<!doctype html><html><head><meta charset='utf-8'>");
  page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>EPD Weather Todo Panel</title>");
  page += F("<style>");
  page += F("body{margin:0;font-family:Arial,sans-serif;background:#f4f6f8;color:#17202a}");
  page += F("main{max-width:820px;margin:0 auto;padding:18px}");
  page += F("section{background:#fff;border:1px solid #d7dde3;border-radius:8px;padding:14px;margin:12px 0}");
  page += F("h1{font-size:24px;margin:4px 0 12px}h2{font-size:18px;margin:0 0 10px}");
  page += F("label{display:block;font-weight:700;margin:10px 0 4px}");
  page += F("input{box-sizing:border-box;width:100%;font-size:16px;padding:10px;border:1px solid #b9c2cc;border-radius:6px}");
  page += F("button{font-size:16px;padding:10px 14px;border:0;border-radius:6px;background:#1f6feb;color:white;margin-top:12px}");
  page += F(".grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.muted{color:#59636e}.warn{color:#a64200}");
  page += F("@media(max-width:640px){.grid{grid-template-columns:1fr}}");
  page += F("</style></head><body><main>");
  page += F("<h1>EPD Weather Todo Panel</h1>");

  page += F("<section><h2>Status</h2><div class='grid'>");
  page += "<div>Wi-Fi: " + String(state.wifi_connected ? "connected" : "not connected") + "</div>";
  page += "<div>AP: " + String(state.ap_running ? "running" : "off") + "</div>";
  page += "<div>STA IP: " + html_escape(state.local_ip) + "</div>";
  page += "<div>AP IP: " + html_escape(state.ap_ip) + "</div>";
  page += "<div>Weather: " + String(state.weather_valid ? "valid" : "invalid") + "</div>";
  page += "<div>Updated: " + html_escape(state.last_update_time) + "</div>";
  page += F("</div>");
  if (state.last_error.length() > 0) {
    page += "<p class='warn'>Last error: " + html_escape(state.last_error) + "</p>";
  }
  page += F("</section>");

  page += F("<section><h2>Weather</h2>");
  page += "<p class='muted'>Date: " + html_escape(state.last_update_time.length() > 0 ? state.last_update_time : "NO TIME") +
          "</p>";
  page += "<p>" + html_escape(cfg.city_name) + ": " + html_escape(weather.weather_text) + ", ";
  page += String(weather.current_temp, 1) + " C, H " + String(weather.temp_max, 1) + " / L ";
  page += String(weather.temp_min, 1) + " C, Rain " + String(weather.precipitation_probability);
  page += "%, Wind " + String(weather.wind_speed, 1) + "</p>";
  page += F("<p class='muted'>Weather data by <a href='https://open-meteo.com/' target='_blank' rel='noopener noreferrer'>Open-Meteo.com</a> (CC BY 4.0)</p></section>");

  page += F("<section><h2>Configuration</h2><form method='post' action='/save_config'>");
  page += F("<label for='wifi_ssid'>Wi-Fi SSID</label>");
  page += "<input id='wifi_ssid' name='wifi_ssid' maxlength='63' value='" + html_escape(cfg.wifi_ssid) + "'>";
  page += F("<label for='wifi_password'>Wi-Fi Password</label>");
  page += F("<input id='wifi_password' name='wifi_password' maxlength='63' type='password' placeholder='Leave blank to keep current password'>");
  page += F("<label for='city_name'>City</label>");
  page += "<input id='city_name' name='city_name' maxlength='31' value='" + html_escape(cfg.city_name) + "'>";
  page += F("<div class='grid'><div><label for='latitude'>Latitude</label>");
  page += "<input id='latitude' name='latitude' value='" + String(cfg.latitude, 4) + "'></div>";
  page += F("<div><label for='longitude'>Longitude</label>");
  page += "<input id='longitude' name='longitude' value='" + String(cfg.longitude, 4) + "'></div></div>";
  page += F("<div class='grid'><div><label for='weather_update_interval_min'>Weather update min</label>");
  page += "<input id='weather_update_interval_min' name='weather_update_interval_min' value='" +
          String(cfg.weather_update_interval_min) + "'></div>";
  page += F("<div><label for='screen_update_interval_min'>Screen update min</label>");
  page += "<input id='screen_update_interval_min' name='screen_update_interval_min' value='" +
          String(cfg.screen_update_interval_min) + "'></div></div>";
  page += F("<label><input style='width:auto' type='checkbox' name='ap_mode_always_on' value='1'");
  page += checked_attr(cfg.ap_mode_always_on);
  page += F("> AP mode always on</label><button type='submit'>Save configuration</button></form></section>");

  page += F("<section><h2>Todo Items</h2><form method='post' action='/save_todos'>");
  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    page += "<label for='todo" + String(i) + "'>Todo " + String(i + 1) + "</label>";
    page += "<input id='todo" + String(i) + "' name='todo" + String(i) + "' maxlength='63' value='" +
            html_escape(cfg.todo_items[i]) + "'>";
  }
  page += F("<button type='submit'>Save todos</button></form>");
  page += F("<form method='post' action='/refresh_screen'><button type='submit'>Refresh screen</button></form>");
  page += F("</section></main></body></html>");
  return page;
}

void handle_root() {
  if (active_config == nullptr) {
    server.send(500, "text/plain", "Config not ready");
    return;
  }
  server.send(200, "text/html", render_main_page(*active_config, app_state, weather_data));
}

void handle_save_config() {
  if (active_config == nullptr) {
    server.send(500, "text/plain", "Config not ready");
    return;
  }

  Serial.println("[WEB] Save config");
  log_request_args("save_config");
  copy_arg("wifi_ssid", active_config->wifi_ssid, sizeof(active_config->wifi_ssid));
  String new_password = server.arg("wifi_password");
  if (new_password.length() > 0) {
    strlcpy(active_config->wifi_password, new_password.c_str(), sizeof(active_config->wifi_password));
  }
  copy_arg("city_name", active_config->city_name, sizeof(active_config->city_name));
  active_config->latitude = server.arg("latitude").toFloat();
  active_config->longitude = server.arg("longitude").toFloat();
  active_config->weather_update_interval_min =
      arg_to_uint("weather_update_interval_min", active_config->weather_update_interval_min);
  active_config->screen_update_interval_min =
      arg_to_uint("screen_update_interval_min", active_config->screen_update_interval_min);
  active_config->ap_mode_always_on = server.hasArg("ap_mode_always_on");
  app_config_sanitize(*active_config);
  save_app_config(*active_config);
  app_state.wifi_reconnect_required = true;
  app_state.weather_update_required = true;
  app_state.screen_update_required = true;
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "Saved");
}

void handle_save_todos() {
  if (active_config == nullptr) {
    server.send(500, "text/plain", "Config not ready");
    return;
  }

  Serial.println("[WEB] Save todos");
  log_request_args("save_todos");
  uint8_t count = 0;
  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "todo%u", i);
    copy_arg(key, active_config->todo_items[i], APP_TODO_MAX_LEN);
    if (strlen(active_config->todo_items[i]) > 0) {
      count = i + 1;
    }
    Serial.printf("[WEB] Todo %u saved len=%u\n", i, strlen(active_config->todo_items[i]));
  }
  active_config->todo_count = count;
  save_todo_items(*active_config);
  app_state.screen_update_required = true;
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "Saved");
}

void handle_refresh_screen() {
  Serial.println("[WEB] Manual screen refresh requested");
  app_state.screen_update_required = true;
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "Refresh requested");
}

void handle_api_status() {
  String json;
  json.reserve(360);
  json += "{";
  json += "\"wifi_connected\":" + String(app_state.wifi_connected ? "true" : "false") + ",";
  json += "\"ap_running\":" + String(app_state.ap_running ? "true" : "false") + ",";
  json += "\"weather_valid\":" + String(app_state.weather_valid ? "true" : "false") + ",";
  json += "\"time_valid\":" + String(app_state.time_valid ? "true" : "false") + ",";
  json += "\"local_ip\":\"" + app_state.local_ip + "\",";
  json += "\"ap_ip\":\"" + app_state.ap_ip + "\",";
  json += "\"last_update_time\":\"" + app_state.last_update_time + "\",";
  json += "\"last_error\":\"" + app_state.last_error + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handle_not_found() {
  server.send(404, "text/plain", "Not found");
}
