#pragma once

#include "app_config.h"

bool storage_begin();
bool load_app_config(AppConfig& cfg);
bool save_app_config(const AppConfig& cfg);
bool load_todo_items(AppConfig& cfg);
bool save_todo_items(const AppConfig& cfg);
void reset_config_to_default(AppConfig& cfg);
