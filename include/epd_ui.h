#pragma once

#include "app_config.h"
#include "app_state.h"

bool epd_ui_render_dashboard(const AppConfig& cfg, const AppState& state, const WeatherData& weather);
bool epd_ui_render_boot_screen(const char* message);
bool epd_ui_render_error_screen(const char* message);
