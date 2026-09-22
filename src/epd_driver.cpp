#include "epd_driver.h"

#include <Adafruit_GFX.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "board_config.h"

namespace {
SPIClass& epd_spi = SPI;
SPISettings epd_spi_settings(EPD_SPI_HZ, MSBFIRST, SPI_MODE0);

constexpr uint32_t EPD_DEFAULT_BUSY_TIMEOUT_MS = 15000;
constexpr size_t EPD_BUFFER_SIZE = static_cast<size_t>(EPD_WIDTH) * EPD_HEIGHT / 8;

uint8_t bw_buffer[EPD_BUFFER_SIZE];
uint8_t color_buffer[EPD_BUFFER_SIZE];
bool epd_sleeping = false;

void set_plane_pixel(uint8_t* buffer, uint16_t x, uint16_t y, bool active, bool active_is_zero);

class EpdGfxCanvas : public Adafruit_GFX {
 public:
  EpdGfxCanvas(uint8_t* buffer, bool active_is_zero)
      : Adafruit_GFX(EPD_WIDTH, EPD_HEIGHT), buffer_(buffer), active_is_zero_(active_is_zero) {}

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) {
      return;
    }
    set_plane_pixel(buffer_, static_cast<uint16_t>(x), static_cast<uint16_t>(y), color != 0, active_is_zero_);
  }

 private:
  uint8_t* buffer_;
  bool active_is_zero_;
};

size_t epd_pixel_index(uint16_t x, uint16_t y) {
  return static_cast<size_t>(x / 8) + static_cast<size_t>(y) * (EPD_WIDTH / 8);
}

void set_plane_pixel(uint8_t* buffer, uint16_t x, uint16_t y, bool active, bool active_is_zero) {
  const size_t idx = epd_pixel_index(x, y);
  const uint8_t mask = static_cast<uint8_t>(0x80 >> (x % 8));
  if (active_is_zero) {
    if (active) {
      buffer[idx] &= static_cast<uint8_t>(~mask);
    } else {
      buffer[idx] |= mask;
    }
  } else {
    if (active) {
      buffer[idx] |= mask;
    } else {
      buffer[idx] &= static_cast<uint8_t>(~mask);
    }
  }
}

void fill_plane(uint8_t* buffer, bool active_is_zero, bool active) {
  const uint8_t value = active ? (active_is_zero ? 0x00 : 0xFF) : (active_is_zero ? 0xFF : 0x00);
  memset(buffer, value, EPD_BUFFER_SIZE);
}

void draw_filled_rect(uint8_t* buffer, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool active,
                      bool active_is_zero) {
  const uint16_t x_end = min<uint16_t>(EPD_WIDTH, x + w);
  const uint16_t y_end = min<uint16_t>(EPD_HEIGHT, y + h);
  for (uint16_t py = y; py < y_end; ++py) {
    for (uint16_t px = x; px < x_end; ++px) {
      set_plane_pixel(buffer, px, py, active, active_is_zero);
    }
  }
}

void draw_text_ascii(uint8_t* buffer, uint16_t x, uint16_t y, const char* text, uint8_t scale,
                     bool active_is_zero) {
  if (text == nullptr || scale == 0) {
    return;
  }

  // Use Adafruit GFX's BSD-licensed built-in font; do not embed a separate glyph table.
  EpdGfxCanvas canvas(buffer, active_is_zero);
  uint16_t cursor_x = x;
  while (*text != '\0') {
    char c = *text;
    if (c >= 'a' && c <= 'z') {
      c = static_cast<char>(c - 'a' + 'A');
    }
    if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.' ||
          c == ':' || c == '/' || c == '%' || c == ' ')) {
      c = ' ';
    }
    canvas.drawChar(cursor_x, y, static_cast<uint8_t>(c), 1, 1, scale);
    cursor_x += 6 * scale;
    ++text;
  }
}

void draw_text_utf8_u8g2(uint8_t* buffer, uint16_t x, uint16_t y, const char* text, bool active_is_zero) {
  if (text == nullptr) {
    return;
  }

  EpdGfxCanvas canvas(buffer, active_is_zero);
  U8G2_FOR_ADAFRUIT_GFX u8g2;
  u8g2.begin(canvas);
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  u8g2.setForegroundColor(1);
  u8g2.setBackgroundColor(0);
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.drawUTF8(x, y + 16, text);
}

void epd_cs_low() {
  digitalWrite(EPD_CS1, LOW);
}

void epd_cs_high() {
  digitalWrite(EPD_CS1, HIGH);
}

void epd_send_command_data(uint8_t cmd, const uint8_t* data, size_t len) {
  epd_send_command(cmd);
  if (data != nullptr && len > 0) {
    epd_send_data_buffer(data, len);
  }
}

bool epd_trigger_refresh() {
  // NOTE: The refresh command sequence may vary between SSD1619 variants/vendors.
  // Replace this block with the panel-specific sequence from datasheet/application note.
  epd_send_command(EPD_REFRESH_SETUP_COMMAND);
  epd_send_data(EPD_REFRESH_SETUP_DATA);
  epd_send_command(EPD_REFRESH_COMMAND);
  return epd_wait_busy(EPD_DEFAULT_BUSY_TIMEOUT_MS);
}

void epd_log_pin_snapshot(const char* tag) {
  Serial.printf("[EPD] %s BUSY=%d DC=%d RST=%d CS=%d\n", tag, digitalRead(EPD_BUSY), digitalRead(EPD_DC),
                digitalRead(EPD_RST), digitalRead(EPD_CS1));
}

void epd_write_frame() {
  epd_send_command(EPD_RAM_BW_COMMAND);
  epd_send_data_buffer(bw_buffer, sizeof(bw_buffer));

  epd_send_command(EPD_RAM_COLOR_COMMAND);
  epd_send_data_buffer(color_buffer, sizeof(color_buffer));
}
}  // namespace

bool epd_begin() {
  Serial.println("[EPD] epd_begin()");

  pinMode(EPD_CS1, OUTPUT);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_BUSY, INPUT);

  epd_cs_high();
  digitalWrite(EPD_DC, HIGH);
  digitalWrite(EPD_RST, HIGH);

  Serial.printf("[EPD] SPI.begin(SCK=%d, MISO=%d, MOSI=%d, CS=%d, HZ=%lu)\n", EPD_SPI_SCK, EPD_SPI_MISO,
                EPD_SPI_MOSI, EPD_CS1, static_cast<unsigned long>(EPD_SPI_HZ));
  epd_spi.begin(EPD_SPI_SCK, EPD_SPI_MISO, EPD_SPI_MOSI, EPD_CS1);

  fill_plane(bw_buffer, EPD_BW_ACTIVE_IS_ZERO, false);
  fill_plane(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO, false);

  Serial.printf("[EPD] Buffers ready, each=%u bytes\n", static_cast<unsigned>(EPD_BUFFER_SIZE));
  Serial.printf("[EPD] Polarity: BW active_is_zero=%u, COLOR active_is_zero=%u\n", EPD_BW_ACTIVE_IS_ZERO,
                EPD_COLOR_ACTIVE_IS_ZERO);
  epd_log_pin_snapshot("After epd_begin");
  return true;
}

void epd_reset() {
  Serial.println("[EPD] Hardware reset start");
  // Reset timing template:
  // 1) RST high (stable)
  // 2) RST low 10ms
  // 3) RST high and wait 10ms
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);
  delay(10);
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  Serial.println("[EPD] Hardware reset done");
}

void epd_send_command(uint8_t cmd) {
  digitalWrite(EPD_DC, LOW);  // Command mode
  epd_spi.beginTransaction(epd_spi_settings);
  epd_cs_low();
  epd_spi.transfer(cmd);
  epd_cs_high();
  epd_spi.endTransaction();
}

void epd_send_data(uint8_t data) {
  digitalWrite(EPD_DC, HIGH);  // Data mode
  epd_spi.beginTransaction(epd_spi_settings);
  epd_cs_low();
  epd_spi.transfer(data);
  epd_cs_high();
  epd_spi.endTransaction();
}

void epd_send_data_buffer(const uint8_t* data, size_t len) {
  if (data == nullptr || len == 0) {
    return;
  }

  digitalWrite(EPD_DC, HIGH);  // Data mode
  epd_spi.beginTransaction(epd_spi_settings);
  epd_cs_low();
  epd_spi.transfer(const_cast<uint8_t*>(data), len);
  epd_cs_high();
  epd_spi.endTransaction();
}

bool epd_wait_busy(uint32_t timeout_ms) {
  const uint32_t start_ms = millis();
  Serial.printf("[EPD] Wait BUSY (active level=%d, timeout=%lu ms)\n", EPD_BUSY_ACTIVE_LEVEL,
                static_cast<unsigned long>(timeout_ms));

  while (digitalRead(EPD_BUSY) == EPD_BUSY_ACTIVE_LEVEL) {
    if (millis() - start_ms > timeout_ms) {
      Serial.printf("[EPD] ERROR: BUSY timeout after %lu ms\n", static_cast<unsigned long>(timeout_ms));
      return false;
    }
    delay(10);
  }

  const uint32_t elapsed_ms = millis() - start_ms;
  Serial.printf("[EPD] BUSY released after %lu ms\n", static_cast<unsigned long>(elapsed_ms));
  return true;
}

bool epd_init_ssd1619_4in2_tricolor() {
  Serial.println("[EPD] Init SSD1619 4.2in tri-color template start");
  Serial.println("[EPD] NOTE: Verify init table/LUT with your exact panel vendor.");

  epd_reset();

  // Some panels require a software reset before full init.
  epd_send_command(0x12);  // SWRESET (template command for many SSD16xx-compatible ICs)
  if (!epd_wait_busy(EPD_DEFAULT_BUSY_TIMEOUT_MS)) {
    return false;
  }

  // Driver output control: (height - 1), LSB first then MSB, scan direction.
  // This value MUST match the panel gate count and orientation.
  const uint16_t gate_lines = EPD_HEIGHT - 1;
  uint8_t doc_data[3] = {
      static_cast<uint8_t>(gate_lines & 0xFF),
      static_cast<uint8_t>((gate_lines >> 8) & 0xFF),
      0x00,
  };
  epd_send_command_data(0x01, doc_data, sizeof(doc_data));

  // Data entry mode: X increment, Y increment (common template value).
  epd_send_command(0x11);
  epd_send_data(0x03);

  // RAM X start/end (400 / 8 = 50 bytes -> 0x00..0x31).
  epd_send_command(0x44);
  epd_send_data(0x00);
  epd_send_data((EPD_WIDTH / 8) - 1);

  // RAM Y start/end (0..299).
  epd_send_command(0x45);
  epd_send_data(0x00);
  epd_send_data(0x00);
  epd_send_data((EPD_HEIGHT - 1) & 0xFF);
  epd_send_data(((EPD_HEIGHT - 1) >> 8) & 0xFF);

  // Border waveform and temperature/voltage settings:
  // These are panel-dependent and may need vendor replacement.
  epd_send_command(0x3C);
  epd_send_data(0x05);

  // Set RAM pointer to origin.
  epd_send_command(0x4E);
  epd_send_data(0x00);
  epd_send_command(0x4F);
  epd_send_data(0x00);
  epd_send_data(0x00);

  if (!epd_wait_busy(EPD_DEFAULT_BUSY_TIMEOUT_MS)) {
    return false;
  }

  Serial.println("[EPD] Init template done");
  epd_sleeping = false;
  return true;
}

bool epd_clear() {
  Serial.println("[EPD] Clear screen");

  if (epd_sleeping && !epd_init_ssd1619_4in2_tricolor()) {
    return false;
  }

  fill_plane(bw_buffer, EPD_BW_ACTIVE_IS_ZERO, false);
  fill_plane(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO, false);

  epd_write_frame();

  return epd_trigger_refresh();
}

void epd_frame_clear() {
  Serial.println("[EPD] Clear frame buffers");
  fill_plane(bw_buffer, EPD_BW_ACTIVE_IS_ZERO, false);
  fill_plane(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO, false);
}

void epd_draw_black_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  draw_filled_rect(bw_buffer, x, y, w, h, true, EPD_BW_ACTIVE_IS_ZERO != 0);
}

void epd_draw_color_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  draw_filled_rect(color_buffer, x, y, w, h, true, EPD_COLOR_ACTIVE_IS_ZERO != 0);
}

void epd_draw_black_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  EpdGfxCanvas canvas(bw_buffer, EPD_BW_ACTIVE_IS_ZERO != 0);
  canvas.drawLine(x0, y0, x1, y1, 1);
}

void epd_draw_color_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  EpdGfxCanvas canvas(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO != 0);
  canvas.drawLine(x0, y0, x1, y1, 1);
}

void epd_draw_black_outline_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  EpdGfxCanvas canvas(bw_buffer, EPD_BW_ACTIVE_IS_ZERO != 0);
  canvas.drawRect(x, y, w, h, 1);
}

void epd_draw_color_outline_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  EpdGfxCanvas canvas(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO != 0);
  canvas.drawRect(x, y, w, h, 1);
}

void epd_draw_black_circle(uint16_t x, uint16_t y, uint16_t r) {
  EpdGfxCanvas canvas(bw_buffer, EPD_BW_ACTIVE_IS_ZERO != 0);
  canvas.drawCircle(x, y, r, 1);
}

void epd_draw_black_bitmap(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h) {
  if (bitmap == nullptr || w == 0 || h == 0) {
    return;
  }
  EpdGfxCanvas canvas(bw_buffer, EPD_BW_ACTIVE_IS_ZERO != 0);
  canvas.drawBitmap(x, y, bitmap, w, h, 1);
}

void epd_draw_color_bitmap(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h) {
  if (bitmap == nullptr || w == 0 || h == 0) {
    return;
  }
  EpdGfxCanvas canvas(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO != 0);
  canvas.drawBitmap(x, y, bitmap, w, h, 1);
}

void epd_draw_black_text(uint16_t x, uint16_t y, const char* text, uint8_t scale) {
  draw_text_ascii(bw_buffer, x, y, text, scale, EPD_BW_ACTIVE_IS_ZERO != 0);
}

void epd_draw_color_text(uint16_t x, uint16_t y, const char* text, uint8_t scale) {
  draw_text_ascii(color_buffer, x, y, text, scale, EPD_COLOR_ACTIVE_IS_ZERO != 0);
}

void epd_draw_black_text_utf8(uint16_t x, uint16_t y, const char* text) {
  draw_text_utf8_u8g2(bw_buffer, x, y, text, EPD_BW_ACTIVE_IS_ZERO != 0);
}

void epd_draw_color_text_utf8(uint16_t x, uint16_t y, const char* text) {
  draw_text_utf8_u8g2(color_buffer, x, y, text, EPD_COLOR_ACTIVE_IS_ZERO != 0);
}

bool epd_display_frame() {
  Serial.println("[EPD] Display prepared frame");
  if (epd_sleeping) {
    Serial.println("[EPD] Panel is sleeping, reinitializing before refresh");
    if (!epd_init_ssd1619_4in2_tricolor()) {
      return false;
    }
  }
  epd_write_frame();
  return epd_trigger_refresh();
}

bool epd_display_test_pattern() {
  Serial.println("[EPD] Draw test pattern: border + black blocks + HelloWorld + color block");

  if (epd_sleeping && !epd_init_ssd1619_4in2_tricolor()) {
    return false;
  }

  fill_plane(bw_buffer, EPD_BW_ACTIVE_IS_ZERO, false);
  fill_plane(color_buffer, EPD_COLOR_ACTIVE_IS_ZERO, false);

  for (uint16_t y = 0; y < EPD_HEIGHT; ++y) {
    for (uint16_t x = 0; x < EPD_WIDTH; ++x) {
      const bool on_border = (x < 3 || x >= EPD_WIDTH - 3 || y < 3 || y >= EPD_HEIGHT - 3);
      const bool in_black_block =
          (x > 40 && x < 140 && y > 40 && y < 120) || (x > 170 && x < 270 && y > 140 && y < 220);
      if (on_border || in_black_block) {
        set_plane_pixel(bw_buffer, x, y, true, EPD_BW_ACTIVE_IS_ZERO != 0);
      }
    }
  }

  // The color block occupies the right side so it is easy to identify
  // whether the second plane is being interpreted correctly.
  for (uint16_t y = 70; y < 230; ++y) {
    for (uint16_t x = 280; x < 380; ++x) {
      set_plane_pixel(color_buffer, x, y, true, EPD_COLOR_ACTIVE_IS_ZERO != 0);
    }
  }

  // Text stays on the BW plane so the result is easy to distinguish from the color block.
  draw_text_ascii(bw_buffer, 72, 248, "HelloWorld", 4, EPD_BW_ACTIVE_IS_ZERO != 0);

  epd_write_frame();

  return epd_trigger_refresh();
}

void epd_sleep() {
  Serial.println("[EPD] Enter deep sleep");
  epd_send_command(0x10);
  epd_send_data(0x01);
  epd_sleeping = true;
  delay(50);
}
