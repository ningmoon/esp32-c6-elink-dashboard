#pragma once

#include <Arduino.h>

#include "app_config.h"

bool wifi_connect_sta(const AppConfig& cfg, uint32_t timeout_ms = 15000);
bool wifi_start_ap();
void wifi_stop_ap();
bool wifi_is_connected();
String wifi_get_local_ip();
String wifi_get_ap_ip();
void wifi_print_status();
