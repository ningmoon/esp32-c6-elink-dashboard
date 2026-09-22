#pragma once

#include <Arduino.h>

#include "app_config.h"
#include "app_state.h"

void web_server_begin(AppConfig& cfg);
void web_server_handle_client();
void web_server_register_routes();
String render_main_page(const AppConfig& cfg, const AppState& state, const WeatherData& weather);
void handle_root();
void handle_save_config();
void handle_save_todos();
void handle_refresh_screen();
void handle_api_status();
void handle_not_found();
