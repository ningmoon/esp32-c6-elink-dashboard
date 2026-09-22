#pragma once

#include <Arduino.h>

void status_led_begin();
void status_led_update(uint32_t now_ms);
void status_led_off();
void status_led_probe(const char* stage, uint8_t red, uint8_t green, uint8_t blue);
void status_led_scan_forever();
