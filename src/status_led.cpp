#include "status_led.h"

#include "board_config.h"
#include "esp32-hal-rmt.h"

namespace {
constexpr uint32_t BREATH_PERIOD_MS = 2400;
constexpr uint8_t BREATH_MAX_BLUE = 96;
constexpr uint8_t BREATH_MIN_BLUE = 8;
constexpr int8_t LED_SCAN_PINS[] = {0, 1, 2, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
constexpr uint32_t LED_RMT_HZ = 10000000;
constexpr uint8_t BOOT_WHITE = 96;
constexpr uint8_t PROBE_DELAY_MS = 180;
bool led_rmt_ready = false;

bool init_status_led_rmt(bool force_reinit) {
  if (led_rmt_ready && !force_reinit) {
    return true;
  }

  if (force_reinit || led_rmt_ready) {
    rmtDeinit(BOARD_RGB_LED_PIN);
    delay(2);
  }

  led_rmt_ready = rmtInit(BOARD_RGB_LED_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, LED_RMT_HZ);
  if (led_rmt_ready) {
    rmtSetEOT(BOARD_RGB_LED_PIN, LOW);
  }

  Serial.printf("[LED] RMT init GPIO%d hz=%lu ok=%u\n", BOARD_RGB_LED_PIN, static_cast<unsigned long>(LED_RMT_HZ),
                led_rmt_ready);
  return led_rmt_ready;
}

bool write_status_led(uint8_t red, uint8_t green, uint8_t blue) {
  if (!init_status_led_rmt(false)) {
    return false;
  }

  rmt_data_t led_data[24];
  const uint8_t color_grb[3] = {green, red, blue};
  size_t i = 0;
  for (uint8_t col = 0; col < 3; ++col) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const bool one = (color_grb[col] & (1U << (7 - bit))) != 0;
      led_data[i].level0 = 1;
      led_data[i].duration0 = one ? 8 : 4;
      led_data[i].level1 = 0;
      led_data[i].duration1 = one ? 4 : 8;
      ++i;
    }
  }

  bool ok = rmtWrite(BOARD_RGB_LED_PIN, led_data, RMT_SYMBOLS_OF(led_data), 20);
  if (!ok) {
    Serial.printf("[LED] RMT write timeout/fail on GPIO%d, retrying with reinit\n", BOARD_RGB_LED_PIN);
    if (init_status_led_rmt(true)) {
      ok = rmtWrite(BOARD_RGB_LED_PIN, led_data, RMT_SYMBOLS_OF(led_data), 20);
    }
  }
  return ok;
}

uint8_t triangle_wave(uint32_t phase_ms) {
  const uint32_t half_period = BREATH_PERIOD_MS / 2;
  if (phase_ms < half_period) {
    return static_cast<uint8_t>((phase_ms * 255UL) / half_period);
  }
  return static_cast<uint8_t>(((BREATH_PERIOD_MS - phase_ms) * 255UL) / half_period);
}
}  // namespace

void status_led_begin() {
  if constexpr (BOARD_RGB_LED_ENABLED) {
    pinMode(BOARD_RGB_LED_PIN, OUTPUT);
    const bool ok = init_status_led_rmt(true) && write_status_led(BOOT_WHITE, BOOT_WHITE, BOOT_WHITE);
    Serial.printf("[LED] RGB status LED enabled on GPIO%d boot_white=%u write_ok=%u\n", BOARD_RGB_LED_PIN, BOOT_WHITE,
                  ok);
  } else {
    Serial.println("[LED] RGB breathing disabled");
  }
}

void status_led_update(uint32_t now_ms) {
  if constexpr (BOARD_RGB_LED_ENABLED) {
    static uint32_t last_update_ms = 0;
    static uint32_t last_log_ms = 0;
    if (now_ms - last_update_ms < 60) {
      return;
    }
    last_update_ms = now_ms;

    const uint8_t wave = triangle_wave(now_ms % BREATH_PERIOD_MS);
    const uint8_t blue = static_cast<uint8_t>(
        BREATH_MIN_BLUE + ((BREATH_MAX_BLUE - BREATH_MIN_BLUE) * static_cast<uint16_t>(wave)) / 255U);
    const bool ok = write_status_led(0, 0, blue);

    if (now_ms - last_log_ms >= 5000) {
      last_log_ms = now_ms;
      Serial.printf("[LED] breathing GPIO%d blue=%u write_ok=%u\n", BOARD_RGB_LED_PIN, blue, ok);
    }
  } else {
    (void)now_ms;
  }
}

void status_led_off() {
  if constexpr (BOARD_RGB_LED_ENABLED) {
    write_status_led(0, 0, 0);
  }
}

void status_led_probe(const char* stage, uint8_t red, uint8_t green, uint8_t blue) {
  if constexpr (BOARD_RGB_LED_ENABLED && BOARD_RGB_LED_BOOT_PROBE_ENABLED) {
    const bool ok = write_status_led(red, green, blue);
    Serial.printf("[LED_PROBE] %-18s GPIO%d rgb=(%u,%u,%u) ok=%u\n", stage, BOARD_RGB_LED_PIN, red, green, blue, ok);
    delay(PROBE_DELAY_MS);
  } else {
    (void)stage;
    (void)red;
    (void)green;
    (void)blue;
  }
}

void status_led_scan_forever() {
  Serial.println();
  Serial.println("[LED_SCAN] RGB LED GPIO scan mode");
  Serial.println("[LED_SCAN] Watch the board LED and note the GPIO printed when it lights.");
  Serial.println("[LED_SCAN] EPD/WiFi/app startup is intentionally skipped in this mode.");

  while (true) {
    for (size_t i = 0; i < sizeof(LED_SCAN_PINS) / sizeof(LED_SCAN_PINS[0]); ++i) {
      const int8_t pin = LED_SCAN_PINS[i];
      pinMode(pin, OUTPUT);

      Serial.printf("[LED_SCAN] testing GPIO%d: RED\n", pin);
      rgbLedWrite(pin, 96, 0, 0);
      delay(900);

      Serial.printf("[LED_SCAN] testing GPIO%d: GREEN\n", pin);
      rgbLedWrite(pin, 0, 96, 0);
      delay(900);

      Serial.printf("[LED_SCAN] testing GPIO%d: BLUE\n", pin);
      rgbLedWrite(pin, 0, 0, 96);
      delay(900);

      rgbLedWrite(pin, 0, 0, 0);
      delay(300);
      rmtDeinit(pin);
    }

    Serial.println("[LED_SCAN] scan round complete, restarting in 2 seconds");
    delay(2000);
  }
}
