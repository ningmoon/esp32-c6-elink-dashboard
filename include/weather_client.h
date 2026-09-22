#pragma once

#include "app_config.h"
#include "app_state.h"

bool weather_fetch(const AppConfig& cfg, WeatherData& out);
const char* weather_code_to_text(int code);
