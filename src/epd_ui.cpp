#include "epd_ui.h"

#include <time.h>

#include "board_config.h"
#include "epd_driver.h"

#ifndef UI_DEBUG_BOX
#define UI_DEBUG_BOX 0
#endif

namespace {
static constexpr int SCREEN_W = EPD_WIDTH;
static constexpr int SCREEN_H = EPD_HEIGHT;

static constexpr int OUTER_FRAME_X = 2;
static constexpr int OUTER_FRAME_Y = 2;
static constexpr int OUTER_FRAME_W = 396;
static constexpr int OUTER_FRAME_H = 296;

static constexpr int TOP_BAR_X = 8;
static constexpr int TOP_BAR_Y = 4;  // 日期栏整体上边距，想让日期栏更靠上/下就改这里
static constexpr int TOP_BAR_W = 380;
static constexpr int TOP_BAR_H = 26;

static constexpr int WEATHER_X = 12;
static constexpr int WEATHER_Y = 44;
static constexpr int WEATHER_W = 240;
static constexpr int WEATHER_H = 92;

static constexpr int TODO_X = 270;
static constexpr int TODO_Y = 50;
static constexpr int TODO_W = 116;
static constexpr int TODO_H = 126;

static constexpr int DECOR_X = 10;
static constexpr int DECOR_Y = 186;
static constexpr int DECOR_W = 380;
static constexpr int DECOR_H = 92;

static constexpr int STATUS_X = 8;
static constexpr int STATUS_Y = 282;
static constexpr int STATUS_W = 384;
static constexpr int STATUS_H = 12;

String ip_or_dash(const String& ip) {
  return ip.length() == 0 ? "-" : ip;
}

bool get_time_info(struct tm& info) {
  return getLocalTime(&info, 20);
}

String make_date_string() {
  struct tm info;
  if (!get_time_info(info)) {
    // Fixed fallback keeps the date area testable even before NTP sync.
    return "2000-01-01";
  }
  char buf[16];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &info);
  return String(buf);
}

const char* weekday_to_english(int weekday) {
  static const char* weekdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (weekday < 0 || weekday > 6) {
    return "SAT";
  }
  return weekdays[weekday];
}

String make_weekday_string() {
  struct tm info;
  if (!get_time_info(info)) {
    return "TUE";
  }
  return weekday_to_english(info.tm_wday);
}

String truncate_utf8_chars(const char* text, uint8_t max_chars, bool add_dots = false) {
  String out;
  if (text == nullptr || max_chars == 0) {
    return out;
  }

  const uint8_t* p = reinterpret_cast<const uint8_t*>(text);
  uint8_t count = 0;
  bool truncated = false;
  while (*p != 0) {
    if (count >= max_chars) {
      truncated = true;
      break;
    }

    uint8_t len = 1;
    if ((*p & 0x80) == 0) {
      len = 1;
    } else if ((*p & 0xE0) == 0xC0) {
      len = 2;
    } else if ((*p & 0xF0) == 0xE0) {
      len = 3;
    } else if ((*p & 0xF8) == 0xF0) {
      len = 4;
    }

    for (uint8_t i = 0; i < len && p[i] != 0; ++i) {
      out += static_cast<char>(p[i]);
    }
    p += len;
    ++count;
  }

  if (truncated && add_dots) {
    out += "...";
  }
  return out;
}

String truncate_todo_text(const char* text, int max_chars) {
  return truncate_utf8_chars(text, static_cast<uint8_t>(max_chars), false);
}

String weather_code_to_label(int code) {
  switch (code) {
    case 0:
    case 1:
      return "CLEAR";
    case 2:
      return "PARTLY";
    case 3:
      return "CLOUDY";
    case 45:
    case 48:
      return "FOG";
    case 51:
    case 53:
    case 55:
      return "DRIZZLE";
    case 61:
    case 63:
    case 65:
      return "RAIN";
    case 71:
    case 73:
    case 75:
      return "SNOW";
    case 80:
    case 81:
    case 82:
      return "SHOWER";
    case 95:
      return "STORM";
    default:
      return "WAIT";
  }
}

void draw_debug_boxes() {
#if UI_DEBUG_BOX
  epd_draw_color_outline_rect(TOP_BAR_X, TOP_BAR_Y, TOP_BAR_W, TOP_BAR_H);
  epd_draw_black_outline_rect(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);
  epd_draw_color_outline_rect(TODO_X, TODO_Y, TODO_W, TODO_H);
  epd_draw_color_outline_rect(STATUS_X, STATUS_Y, STATUS_W, STATUS_H);
#endif
}

void draw_single_outer_frame() {
  epd_draw_black_outline_rect(OUTER_FRAME_X, OUTER_FRAME_Y, OUTER_FRAME_W, OUTER_FRAME_H);
}

void draw_thick_black_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  epd_draw_black_line(x0, y0, x1, y1);
  epd_draw_black_line(x0, y0 + 1, x1, y1 + 1);
}

void draw_filled_black_circle(int16_t cx, int16_t cy, int16_t r) {
  for (int16_t y = -r; y <= r; ++y) {
    for (int16_t x = -r; x <= r; ++x) {
      if (x * x + y * y <= r * r) {
        epd_draw_black_rect(cx + x, cy + y, 1, 1);
      }
    }
  }
}

void draw_weather_cloud_silhouette(uint16_t x, uint16_t y, uint16_t size) {
  const uint16_t base_y = y + size * 2 / 3;
  draw_filled_black_circle(x + size * 35 / 100, base_y - size * 18 / 100, size * 18 / 100);
  draw_filled_black_circle(x + size * 52 / 100, base_y - size * 27 / 100, size * 24 / 100);
  draw_filled_black_circle(x + size * 70 / 100, base_y - size * 17 / 100, size * 18 / 100);
  epd_draw_black_rect(x + size * 22 / 100, base_y - size * 18 / 100, size * 58 / 100, size * 21 / 100);
  draw_thick_black_line(x + size * 20 / 100, base_y + 3, x + size * 82 / 100, base_y + 3);
}

void draw_simple_cloud_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_weather_cloud_silhouette(x, y, size);
}

void draw_sun_icon(uint16_t x, uint16_t y, uint16_t size) {
  const uint16_t cx = x + size / 2;
  const uint16_t cy = y + size / 2;
  const uint16_t r = size / 6;
  draw_filled_black_circle(cx, cy, r);
  draw_thick_black_line(cx, y + 2, cx, y + size / 4);
  draw_thick_black_line(cx, y + size * 3 / 4, cx, y + size - 2);
  draw_thick_black_line(x + 2, cy, x + size / 4, cy);
  draw_thick_black_line(x + size * 3 / 4, cy, x + size - 2, cy);
  draw_thick_black_line(x + 8, y + 8, x + size / 3, y + size / 3);
  draw_thick_black_line(x + size * 2 / 3, y + size * 2 / 3, x + size - 8, y + size - 8);
  draw_thick_black_line(x + size - 8, y + 8, x + size * 2 / 3, y + size / 3);
  draw_thick_black_line(x + size / 3, y + size * 2 / 3, x + 8, y + size - 8);
}

void draw_partly_cloudy_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_sun_icon(x, y, size * 2 / 3);
  draw_weather_cloud_silhouette(x + size / 5, y + size / 4, size * 4 / 5);
}

void draw_rain_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_weather_cloud_silhouette(x, y, size);
  for (uint8_t i = 0; i < 4; ++i) {
    const uint16_t px = x + size / 4 + i * (size / 7);
    draw_thick_black_line(px, y + size - 10, px - 4, y + size - 1);
  }
}

void draw_snow_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_weather_cloud_silhouette(x, y, size);
  for (uint8_t i = 0; i < 3; ++i) {
    const uint16_t px = x + size / 3 + i * (size / 6);
    const uint16_t py = y + size - 5;
    epd_draw_black_line(px - 4, py, px + 4, py);
    epd_draw_black_line(px, py - 4, px, py + 4);
    epd_draw_black_line(px - 3, py - 3, px + 3, py + 3);
    epd_draw_black_line(px + 3, py - 3, px - 3, py + 3);
  }
}

void draw_fog_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_weather_cloud_silhouette(x, y, size);
  for (uint8_t i = 0; i < 4; ++i) {
    const uint16_t yy = y + size - 12 + i * 5;
    const uint16_t x0 = x + 8 + (i % 2) * 8;
    epd_draw_black_line(x0, yy, x + size - 8, yy);
  }
}

void draw_thunder_icon(uint16_t x, uint16_t y, uint16_t size) {
  draw_weather_cloud_silhouette(x, y, size);
  const uint16_t bx = x + size / 2;
  const uint16_t by = y + size - 18;
  draw_thick_black_line(bx + 4, by, bx - 6, by + 15);
  draw_thick_black_line(bx - 6, by + 15, bx + 4, by + 13);
  draw_thick_black_line(bx + 4, by + 13, bx - 2, by + 28);
}

void drawWeatherIcon(int weather_code, int x, int y, int size) {
  switch (weather_code) {
    case 0:
    case 1:
      draw_sun_icon(x, y, size);
      break;
    case 2:
      draw_partly_cloudy_icon(x, y, size);
      break;
    case 3:
      draw_simple_cloud_icon(x, y, size);
      break;
    case 45:
    case 48:
      draw_fog_icon(x, y, size);
      break;
    case 51:
    case 53:
    case 55:
    case 61:
    case 63:
    case 65:
    case 80:
    case 81:
    case 82:
      draw_rain_icon(x, y, size);
      break;
    case 71:
    case 73:
    case 75:
      draw_snow_icon(x, y, size);
      break;
    case 95:
      draw_thunder_icon(x, y, size);
      break;
    default:
      draw_partly_cloudy_icon(x, y, size);
      break;
  }
}

void draw_calendar_icon(uint16_t x, uint16_t y) {
  epd_draw_color_outline_rect(x, y, 18, 20);
  epd_draw_color_line(x, y + 6, x + 17, y + 6);
  epd_draw_black_rect(x + 4, y - 2, 3, 6);
  epd_draw_black_rect(x + 12, y - 2, 3, 6);
  for (uint8_t row = 0; row < 2; ++row) {
    for (uint8_t col = 0; col < 3; ++col) {
      epd_draw_black_rect(x + 4 + col * 5, y + 10 + row * 6, 2, 2);
    }
  }
}

void draw_checklist_icon(uint16_t x, uint16_t y) {
  epd_draw_color_outline_rect(x, y, 15, 18);
  epd_draw_black_rect(x + 5, y - 2, 5, 4);
  epd_draw_black_line(x + 4, y + 10, x + 7, y + 13);
  epd_draw_black_line(x + 7, y + 13, x + 12, y + 6);
}

void draw_top_date_bar(const AppConfig& cfg, const AppState& state) {
  (void)cfg;
  (void)state;
  epd_draw_color_line(TOP_BAR_X, TOP_BAR_Y + TOP_BAR_H + 1, TOP_BAR_X + TOP_BAR_W,
                      TOP_BAR_Y + TOP_BAR_H + 1);
  draw_calendar_icon(16, 7);  // 日期 icon 坐标，若 icon 与文字不齐，优先微调这里的 y

  epd_draw_black_text(TOP_BAR_X + 45, TOP_BAR_Y + 6, make_date_string().c_str(), 2);  // 日期文字坐标
  epd_draw_black_text(190, TOP_BAR_Y + 8, make_weekday_string().c_str(), 2);          // 英文星期文字坐标
}

void draw_temperature(float temp, int x, int y) {
  char temp_value[16];
  snprintf(temp_value, sizeof(temp_value), "%.1f", temp);
  epd_draw_black_text(x, y, temp_value, 4);
  epd_draw_black_circle(x + 102, y + 3, 3);
  epd_draw_black_text(x + 110, y + 7, "C", 2);
}

void draw_weather_main_panel(const WeatherData& weather) {
  const int icon_x = WEATHER_X + 24;
  const int icon_y = WEATHER_Y + 8;
  drawWeatherIcon(weather.weather_code, icon_x, icon_y, 56);
  epd_draw_black_text(WEATHER_X + 24, WEATHER_Y + 74, weather_code_to_label(weather.weather_code).c_str(), 1);

  if (weather.weather_code >= 0) {
    draw_temperature(weather.current_temp, WEATHER_X + 122, WEATHER_Y + 22);  // 大温度坐标，增大 y 会更靠近红线
  } else {
    epd_draw_black_text(WEATHER_X + 122, WEATHER_Y + 34, "NO DATA", 2);
  }

  epd_draw_color_line(WEATHER_X + 122, WEATHER_Y + 68, WEATHER_X + WEATHER_W - 12, WEATHER_Y + 68);  // 温度下方红色基线

  char hi_lo[28];
  snprintf(hi_lo, sizeof(hi_lo), "H%.0f  L%.0f", weather.temp_max, weather.temp_min);
  epd_draw_black_text(WEATHER_X + 126, WEATHER_Y + 78, hi_lo, 2);  // 最高/最低温度坐标
}

void draw_todo_card(const AppConfig& cfg) {
  static constexpr int header_h = 24;
  static constexpr int row_h = 18;
  static constexpr int list_x = TODO_X + 8;
  static constexpr int list_y = TODO_Y + 32;

  epd_draw_color_outline_rect(TODO_X, TODO_Y, TODO_W, TODO_H);
  epd_draw_color_line(TODO_X + 1, TODO_Y + header_h, TODO_X + TODO_W - 2, TODO_Y + header_h);
  draw_checklist_icon(TODO_X + 8, TODO_Y + 4);
  epd_draw_black_text(TODO_X + 34, TODO_Y + 7, "TODO", 2);

  for (uint8_t i = 0; i < APP_MAX_TODOS; ++i) {
    const int row_y = list_y + i * row_h;
    String body = truncate_todo_text(cfg.todo_items[i], 5);
    String line = String(i + 1) + ". " + body;
    epd_draw_black_text_utf8(list_x, row_y, line.c_str());
  }
}

void draw_decorative_scene() {
  // Copyright-safe procedural placeholder used by the public repository. It
  // has no external image asset or source-file dependency.
  //
  // To use artwork you have permission to distribute, convert it to a
  // 380x92, 1-bit, MSB-first bitmap and replace this body with one or both of:
  // epd_draw_black_bitmap(DECOR_X, DECOR_Y, black_bitmap, DECOR_W, DECOR_H);
  // epd_draw_color_bitmap(DECOR_X, DECOR_Y, color_bitmap, DECOR_W, DECOR_H);
  Serial.printf("[EPD_UI] Draw procedural decor at x=%d y=%d w=%d h=%d\n", DECOR_X, DECOR_Y, DECOR_W,
                DECOR_H);
  const int horizon_y = DECOR_Y + 57;
  epd_draw_black_line(DECOR_X, horizon_y, DECOR_X + DECOR_W - 1, horizon_y);
  epd_draw_black_circle(DECOR_X + 320, DECOR_Y + 22, 12);
  epd_draw_black_line(DECOR_X, horizon_y, DECOR_X + 65, DECOR_Y + 24);
  epd_draw_black_line(DECOR_X + 65, DECOR_Y + 24, DECOR_X + 132, horizon_y);
  epd_draw_black_line(DECOR_X + 78, horizon_y, DECOR_X + 165, DECOR_Y + 34);
  epd_draw_black_line(DECOR_X + 165, DECOR_Y + 34, DECOR_X + 235, horizon_y);
  epd_draw_color_outline_rect(DECOR_X + 254, DECOR_Y + 39, 34, 19);
  epd_draw_color_line(DECOR_X + 250, DECOR_Y + 39, DECOR_X + 271, DECOR_Y + 25);
  epd_draw_color_line(DECOR_X + 271, DECOR_Y + 25, DECOR_X + 292, DECOR_Y + 39);
  epd_draw_black_line(DECOR_X, DECOR_Y + 72, DECOR_X + DECOR_W - 1, DECOR_Y + 72);
  epd_draw_black_line(DECOR_X, DECOR_Y + 82, DECOR_X + DECOR_W - 1, DECOR_Y + 82);
}

void draw_bottom_status_bar(const AppState& state) {
  epd_draw_black_line(STATUS_X, STATUS_Y, STATUS_X + STATUS_W, STATUS_Y);
  String wifi;
  if (state.wifi_connected) {
    wifi = "WIFI:OK " + ip_or_dash(state.local_ip);
  } else if (state.ap_running) {
    wifi = "WIFI:AP " + ip_or_dash(state.ap_ip);
  } else {
    wifi = "WIFI:OFF";
  }
  epd_draw_black_text(STATUS_X + 12, STATUS_Y + 5, wifi.c_str(), 1);
  // Open-Meteo's CC BY 4.0 terms require attribution where its data is shown.
  epd_draw_black_text(STATUS_X + 200, STATUS_Y + 5, "OPEN-METEO.COM", 1);
  epd_draw_black_text(STATUS_X + STATUS_W - 46, STATUS_Y + 5, "FW:2", 1);
}
}  // namespace

bool epd_ui_render_boot_screen(const char* message) {
  Serial.printf("[EPD_UI] Render boot screen: %s\n", message == nullptr ? "" : message);
  epd_frame_clear();
  epd_draw_color_outline_rect(0, 0, EPD_WIDTH, 28);
  epd_draw_black_text(12, 8, "EPD PANEL", 2);
  epd_draw_black_text(36, 120, message == nullptr ? "BOOTING" : message, 3);
  bool ok = epd_display_frame();
  epd_sleep();
  return ok;
}

bool epd_ui_render_error_screen(const char* message) {
  Serial.printf("[EPD_UI] Render error screen: %s\n", message == nullptr ? "" : message);
  epd_frame_clear();
  epd_draw_color_outline_rect(0, 0, EPD_WIDTH, 34);
  epd_draw_black_text(12, 10, "ERROR", 2);
  epd_draw_black_text(12, 80, message == nullptr ? "UNKNOWN" : message, 2);
  bool ok = epd_display_frame();
  epd_sleep();
  return ok;
}

bool epd_ui_render_dashboard(const AppConfig& cfg, const AppState& state, const WeatherData& weather) {
  Serial.println("[EPD_UI] Render stable dashboard v4");
  epd_frame_clear();

  draw_single_outer_frame();
  draw_top_date_bar(cfg, state);
  draw_weather_main_panel(weather);
  draw_todo_card(cfg);
  draw_decorative_scene();
  draw_bottom_status_bar(state);
  draw_debug_boxes();

  bool ok = epd_display_frame();
  epd_sleep();
  return ok;
}
