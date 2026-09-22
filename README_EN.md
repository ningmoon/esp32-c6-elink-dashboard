# ESP32-C6 E-Paper Dashboard

[简体中文](README.md) | English

Arduino/PlatformIO firmware for a 400×300 tri-color e-paper dashboard on an ESP32-C6. It displays time, weather, up to five TODO items, network status, and a decorative scene drawn in code. Configuration is handled through a web page served by the device.

A finished unit can be installed in a self-designed or third-party 3D-printed enclosure. The following photo only demonstrates a development-stage physical build. Its AI-assisted on-screen artwork is not provided as a reusable bitmap and does not represent the default output of the current public firmware.

![Finished e-paper dashboard with its 3D-printed enclosure](./docs/dashboard4.jpg)

*Prototype build with a 3D-printed enclosure, shown only as an appearance and use-case example. The public firmware uses a procedural placeholder with no external asset dependency.*

> The project is currently configured for the board, adapter, and panel combination below. E-paper controller variants, BUSY polarity, and refresh waveforms may differ between batches. A successful build does not by itself establish panel compatibility.

## Quick start

1. Connect the board, adapter, and panel according to the [wiring table](#wiring), using a `3.3V` supply.
2. Install Python 3 and PlatformIO Core 6.x, then connect the board's CH343 serial/programming port with a USB data cable.
3. Build, upload, and open the serial monitor:

   ```bash
   pio run -e esp32c6
   pio run -e esp32c6 -t upload
   pio device monitor -b 115200
   ```

4. On first boot, join the `EPD-Config` Wi-Fi network with password `12345678`, then open <http://192.168.4.1>.
5. Enter the Wi-Fi, location, and refresh settings. After saving, the device attempts to join Wi-Fi and updates the weather and display.
6. Find the device's LAN IP in the serial log. Use that IP to reopen the configuration page later.

The first build downloads the platform and dependencies declared in `platformio.ini`. A full e-paper refresh takes time; do not remove power or change wiring during a refresh.

## Hardware

Configured hardware combination:

- MuseLab nanoESP32-C6;
- 400×300 tri-color panel using an SSD1619-compatible command set;
- SPI e-paper adapter, using `CS1` only;
- USB data cable, jumper wires, and a stable 3.3V supply.

The photos below identify the adapter and development-board connectors used by this project. They do not represent the current firmware's screen output.

| SPI e-paper adapter | MuseLab nanoESP32-C6 |
|---|---|
| ![E-paper adapter and connector labels](./docs/dashboard2.jpg) | ![MuseLab nanoESP32-C6 board and connectors](./docs/dashboard3.jpg) |

### Wiring

| Adapter pin | ESP32-C6 | Notes |
|---|---:|---|
| GND | GND | A common ground is required |
| 3V3 | 3V3 | Do not connect to 5V |
| SCK | GPIO4 | SPI clock |
| SDA | GPIO6 | The adapter labels its SPI MOSI pin as `SDA` |
| RST | GPIO10 | Panel reset |
| DC | GPIO5 | Data/command select |
| CS1 | GPIO7 | First chip select |
| BUSY | GPIO3 | Panel busy signal |
| CS2 | Not connected | Not used by the current firmware |

The onboard RGB status LED uses GPIO8 and requires no external wiring. Pin assignments and panel parameters are centralized in `include/board_config.h`. The default driver setting is `EPD_DRIVER_PROFILE 1`.

Before applying power or refreshing, recheck the panel-cable orientation, supply voltage, and every connection. Do not infer panel compatibility from connector appearance alone.

### Custom decorative bitmap

The dashboard reserves a **380×92 pixel** decorative area at `(10, 186)`. The public version draws a line-art placeholder in `draw_decorative_scene()` in `src/epd_ui.cpp` and contains no external bitmap asset.

The driver exposes two 1-bit bitmap helpers:

```cpp
epd_draw_black_bitmap(10, 186, black_bitmap, 380, 92);
epd_draw_color_bitmap(10, 186, color_bitmap, 380, 92);
```

- Bitmap rows are MSB-first; each set bit draws a pixel on the corresponding color plane.
- At 380 pixels wide, each row occupies `(380 + 7) / 8 = 48` bytes. One complete color plane occupies `48 × 92 = 4416` bytes.
- Black and accent color use separate 1-bit arrays; unset pixels remain white.
- An array can be declared as `static const uint8_t ...[] PROGMEM` and passed to the helpers from `draw_decorative_scene()`.
- Use only original, public-domain, or compatibly licensed artwork. Converting, resizing, or dithering an image into a bitmap does not grant rights to the source image.

## First-time configuration

The device starts a setup access point when it has no saved Wi-Fi network or cannot connect to the saved network:

- Network: `EPD-Config`
- Password: `12345678`
- Setup URL: <http://192.168.4.1>

The configuration page provides:

- **Wi-Fi SSID / Password**: the network the device should join; leaving the password field empty preserves the existing password.
- **City**: the location name shown on the display. Weather lookup uses the coordinates below, not this label.
- **Latitude / Longitude**: the weather location. Replace the default `0, 0` with real coordinates.
- **Weather update min**: weather update interval in minutes.
- **Screen update min**: display refresh interval in minutes.
- **AP mode always on**: keeps the setup network active. This is normally best left off on a trusted home network.
- **Todo Items**: stores up to five items. Because of display space, only the first five characters of each item are currently shown.
- **Refresh screen**: requests a manual refresh. The main loop performs the refresh, so the display may continue working after the page returns.

After the main configuration is saved, the device automatically attempts to reconnect to Wi-Fi; an immediate manual reset is not required. If the setup AP was already started during first boot, clearing **AP mode always on** does not stop that existing AP immediately. Once station Wi-Fi works, reboot the device; the AP will then remain off on successful subsequent connections.

Display time is currently fixed to **UTC+8** by `configTime()` in `src/main.cpp`. Users outside China Standard Time must change the UTC offset and rebuild. The weather API selects a timezone from the coordinates, but that does not change the dashboard clock's fixed UTC+8 setting.

## Normal use

- The serial baud rate is `115200`; startup logs show Wi-Fi status, the LAN IP, and errors.
- From the same LAN, open the device IP to change configuration or TODO items and request a manual refresh.
- `http://<device-ip>/api/status` returns connection, weather, time, and refresh status.
- Configuration and TODO items are stored in ESP32 Preferences/NVS and survive a reboot.

### Weather data and permitted use

Weather data are provided by [Open-Meteo.com](https://open-meteo.com/) under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). The firmware prints `OPEN-METEO.COM` in the display footer, and the configuration page provides a clickable attribution link.

The project source remains MIT-licensed and may itself be used commercially. The Open-Meteo API is a separate online service; its service terms do not change the source-code license. Open-Meteo's current Free/Open-Access API is limited to non-commercial use and is subject to request limits. The default project configuration is intended for personal, home-automation, and other non-commercial use. Commercial deployments must follow the latest terms by subscribing to an appropriate commercial API plan or modifying the code to use another suitably licensed weather provider.

## Troubleshooting

### No serial port or upload failure

Use a USB data cable rather than a power-only cable and check the CH343 driver. Run `pio device list` to identify the port. If automatic detection fails, provide the port explicitly:

```bash
pio run -e esp32c6 -t upload --upload-port COM3
pio device monitor --port COM3 -b 115200
```

Replace `COM3` with the actual port.

### The setup page does not open

Make sure the computer or phone is still connected to `EPD-Config`, then open <http://192.168.4.1> directly. A warning that this network has no internet access is expected. If the device has already joined the home network, use the LAN IP printed in the serial log instead.

### Weather location is wrong

`City` is only a display label. Check the latitude and longitude; southern latitudes and western longitudes must be negative.

### Blank display or incorrect colors

Remove power and recheck 3.3V, GND, SCK, SDA/MOSI, CS1, DC, RST, and BUSY. Panels with the same resolution may still require different driver settings, BUSY polarity, or waveforms. Do not repeatedly refresh a panel whose compatibility has not been established.

## Features and code structure

- Connects to station Wi-Fi and starts a local setup AP when needed.
- Stores Wi-Fi settings, location, refresh intervals, and TODO items in ESP32 Preferences/NVS.
- Fetches current weather from Open-Meteo without an API key and serves a local configuration page plus `/api/status`.
- Draws a two-plane black/color framebuffer and performs slow e-paper refreshes only from the main loop.
- Drives the onboard addressable status LED without blocking the main loop.

`src/main.cpp` coordinates Wi-Fi, NTP, weather, the web server, and screen refresh. Configuration and runtime state are in `include/app_config.h` and `include/app_state.h`; persistence, network, display, and LED implementations are in their correspondingly named `src/` files.

## Verification and contributing

```bash
pio run -e esp32c6 -t clean
pio run -e esp32c6
```

There are no host-side unit tests. Hardware smoke tests should cover boot, the setup AP, NVS reload, station reconnect, NTP, weather, web routes, LED operation, and at least one full display refresh. Keep GPIO definitions in `include/board_config.h`, defer display refreshes to the main loop, and document the exact board, adapter, and panel used for driver changes.

Do not submit credentials, logs, private locations, vendor documents, or external assets/source without documented provenance and compatible licensing.

## Security and privacy

This is a trusted-LAN prototype, not an internet-facing device. The setup password is public and fixed in `src/wifi_manager.cpp`; change it for a real deployment. The configuration server uses unauthenticated HTTP without CSRF protection. Do not forward port 80, expose the device through a public tunnel, or use it on an untrusted network.

Wi-Fi credentials are stored in NVS rather than hardware-backed secure storage. Weather requests use HTTP, so responses are unauthenticated and coordinates are visible to the network; use validated TLS for security-sensitive deployments. Serial output may contain IP addresses and operational metadata. Do not post credentials or device identifiers in public issues.

The release tree uses synthetic defaults, and `.pio/` build artifacts are ignored. Review local files and Git staging before every public release. Automated scans cannot establish the legal provenance of code or assets and cannot cover files added later.

## Dependencies and license

PlatformIO downloads dependencies; third-party library source is not vendored: pioarduino `espressif32`/Arduino ESP32 (platform release `55.03.39`), ArduinoJson 7.4.3 (MIT), [Adafruit GFX 1.12.6](https://github.com/adafruit/Adafruit-GFX-Library/blob/master/license.txt) (BSD), [U8g2 for Adafruit GFX 1.8.0](https://github.com/olikraus/u8g2_for_Adafruit_GFX/blob/master/LICENSE) (library code BSD-2-Clause), and transitive Adafruit BusIO (MIT). Third-party dependencies retain their upstream licenses.

Chinese TODO text uses `u8g2_font_wqy16_t_gb2312`. U8g2's [font provenance page](https://github.com/olikraus/u8g2/wiki/fntgrpwqy) attributes it to the WenQuanYi Project (2004–2010, WenQuanYi Project Board of Trustees and Qianqian Fang) under **GPL v2 with a font embedding exception**, not the BSD license for the library code; see [NOTICE](NOTICE). The repository does not directly vendor the font source, but compiled firmware includes the font data. Before distributing a binary, separately verify whether the embedding exception covers the intended distribution and which license or source materials must accompany it. The project's MIT license does not relicense the font.

Original project code and documentation are released under the [MIT License](LICENSE), Copyright 2026 ningmoon; see [NOTICE](NOTICE). For photos in `docs/` taken by ningmoon, only elements created by and actually owned by ningmoon are provided as project documentation under the MIT License. AI-assisted decorative screen content visible in prototype photos is illustrative only; it is not separately distributed as an asset or claimed as exclusively copyrighted material. The repository does not include vendor documents, downloaded SDKs, private development history, finished decorative bitmap assets, or generated binaries. The current decorative scene is drawn procedurally in `src/epd_ui.cpp`.
