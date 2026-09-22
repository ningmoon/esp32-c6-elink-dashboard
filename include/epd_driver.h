#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

bool epd_begin();
void epd_reset();
void epd_send_command(uint8_t cmd);
void epd_send_data(uint8_t data);
void epd_send_data_buffer(const uint8_t* data, size_t len);
bool epd_wait_busy(uint32_t timeout_ms);
bool epd_init_ssd1619_4in2_tricolor();
bool epd_clear();
bool epd_display_test_pattern();
void epd_frame_clear();
void epd_draw_black_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epd_draw_color_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epd_draw_black_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void epd_draw_color_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void epd_draw_black_outline_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epd_draw_color_outline_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epd_draw_black_circle(uint16_t x, uint16_t y, uint16_t r);
void epd_draw_black_bitmap(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h);
void epd_draw_color_bitmap(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h);
void epd_draw_black_text(uint16_t x, uint16_t y, const char* text, uint8_t scale);
void epd_draw_color_text(uint16_t x, uint16_t y, const char* text, uint8_t scale);
void epd_draw_black_text_utf8(uint16_t x, uint16_t y, const char* text);
void epd_draw_color_text_utf8(uint16_t x, uint16_t y, const char* text);
bool epd_display_frame();
void epd_sleep();
