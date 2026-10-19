# Smart Digital Photo Album & Weather Display

**ESP32-S3 SuperMini · ST7789 240 × 240 TFT · Arduino C++ · LittleFS · JPEGDEC**

A microcontroller-based digital photo album that displays preloaded images and provides an on-demand clock, date, and current-weather display. Physical buttons control photo navigation and weather refresh. The system is designed to keep the offline photo-album function usable even without Wi-Fi.

> **Project status:** Firmware preserved from working personal prototype v1.5, with local settings externalized for safer publication. The redistributed configuration arrangement has **not yet been compiled or validated on hardware**. A public demonstration, PCB/wiring schematic, enclosure CAD and measured benchmarks can be added after verification.

## Features

- Local 240 × 240 JPEG photos decoded with JPEGDEC from ESP32 LittleFS.
- Three-stage album presentation with short animation/title introduction, timed progression, and button-advance support.
- Clock and date synchronized using SNTP/NTP with POSIX timezone and daylight-saving rules.
- Current conditions via the [Open-Meteo API](https://open-meteo.com/en/docs): temperature, WMO weather code, wind speed and day/night.
- Three configurable priority-ordered Wi-Fi networks, with scanning and connection fallback.
- Button-controlled refresh using a FreeRTOS task; temporary refresh UI has a bounded 3-second display duration.
- Display backlight timeout and operation without network access; missing unsynchronized values are represented by dashes.

## Hardware

| Component | Role |
| --- | --- |
| ESP32-S3 SuperMini | Application controller, Wi-Fi, file storage |
| ST7789 1.54-inch TFT, 240 × 240 | Color graphics and photo presentation |
| Two push buttons | Album/next and weather/info refresh |
| Reset/EN button | Hardware reset |
| Appropriate power supply and wiring | Power and electrical interconnect |

### Pin assignments (as defined in firmware)

| Signal | ESP32-S3 GPIO |
| --- | --- |
| TFT backlight (BL) | 7 |
| TFT reset (RST) | 8 |
| TFT data/command (DC) | 9 |
| TFT chip select (CS) | 10 |
| TFT MOSI | 11 |
| TFT SCLK | 12 |
| Album button (active LOW) | 2 |
| Refresh button (active LOW) | 3 |

**Boot caution:** GPIO3 is an ESP32-S3 strapping pin. Do not hold the refresh button during reset or power-up. Verify display power requirements and logic voltage against your actual module before wiring.

## Software and dependencies

- [Arduino IDE](https://www.arduino.cc/en/software) with the **ESP32 by Espressif Systems** board package, using a board definition appropriate for your ESP32-S3 SuperMini.
- Built-in ESP32 libraries: `WiFi`, `WiFiClientSecure`, `HTTPClient`, `SPI`, `LittleFS`, `time`, `esp_arduino_version`.
- External Arduino libraries: **Adafruit GFX Library**, **Adafruit ST7735 and ST7789 Library**, **ArduinoJson**, **JPEGDEC** (Larry Bank).
- ESP32 core includes an explicit compatibility branch for LEDC backlight APIs before versus after Arduino-ESP32 3.x. The sketch also uses `DynamicJsonDocument`; ArduinoJson 6 is the safest starting version until version-7 compatibility is verified.

Library versions, board-menu options and exact filesystem-upload method should be recorded after a successful clean rebuild.

## Repository layout

```text
firmware/OurPhotoAlbum/
  OurPhotoAlbum.ino       Main Arduino sketch
  config.example.h       Safe local-settings template; tracked
  config.h               Private local settings; create it yourself, untracked
docs/
  CONFIGURATION.md       Configuration and publishing guidance
  TEST_PLAN.md           Repeatable smoke-test checklist
.gitignore               Excludes local secrets, photos and builds
README.md                This document
```

## Set up and run

1. Install Arduino IDE, the ESP32 board package, and the four external libraries listed above.
2. Open `firmware/OurPhotoAlbum/OurPhotoAlbum.ino` in Arduino IDE.
3. Copy `config.example.h` to `config.h` **in the same directory** and edit only your own local copy. See [configuration guidance](docs/CONFIGURATION.md).
4. Set three known Wi-Fi SSIDs/passwords (or adjust `WIFI_COUNT` and both arrays consistently); select a public weather coordinate and correct POSIX timezone.
5. Prepare baseline JPEGs named `.jpg` or `.jpeg`, exactly **240 × 240 pixels**, and upload them to the device's LittleFS filesystem using a filesystem uploader compatible with your Arduino IDE and ESP32 core. Up to **25** images are indexed. Keep personal photos out of the public repository.
6. Choose the correct board, USB/port, partition scheme with adequate LittleFS space, and flash the sketch.
7. Open Serial Monitor at **115200 baud** for status and diagnostics; follow the [test plan](docs/TEST_PLAN.md).

**Important:** Sketch uploads and LittleFS filesystem-image uploads are distinct operations. Flashing the sketch alone will not populate the photo storage. The exact filesystem-uploader procedure depends on your Arduino IDE/ESP32 core combination.

## User controls

| Control | Behavior |
| --- | --- |
| Album button (GPIO2) | Enters album mode, advances intro/title/photos, and restarts the timed photo session as appropriate |
| Refresh button (GPIO3) | Exits album mode, displays refresh screen for approximately 3 seconds, then displays clock/weather for approximately 10 seconds |
| Reset/EN | Reboots the device and triggers startup initialization and Wi-Fi selection |

The initial information screen also times out after approximately 10 seconds. The album sequence uses approximately 3-second animation and title intervals and 5-second photo intervals. These are configured near the beginning of the sketch.

## Architecture overview

```text
Physical buttons ─────────► Screen-state / album-state logic ──► ST7789 TFT
                                 ▲              ▲
LittleFS JPEG files ─► JPEGDEC ┘              │
                                                │
Wi-Fi ──► NTP / POSIX timezone ────────────────┤
      └─► Open-Meteo JSON weather endpoint ─────┘
```

- `setup()` initializes SPI, display/backlight, button handling, LittleFS photo index and Wi-Fi/time/weather startup state.
- `loop()` handles button events, advances the slideshow state machine, redraws the clock and implements screen timeouts.
- An interrupt latches quick album-button presses. A FreeRTOS task handles weather refresh to keep its loading screen responsive.
- `fetchWeather()` downloads JSON current conditions and maps WMO codes to custom drawn weather icons.
- JPEGDEC callbacks stream image data from LittleFS into the TFT using RGB565 pixel blocks.

## Engineering decisions and limitations

- **Photo constraints:** Exactly 240 × 240 baseline JPEGs; images in other sizes are rejected. File count capped at 25.
- **Online services:** Weather needs Wi-Fi. Time needs an initial successful synchronization; a clock that has synced may continue running while offline.
- **Network retry:** Firmware attempts visible configured networks first, then unseen configured networks; the exact total connection duration depends on scanning and each attempt timeout, not a global 10-second deadline.
- **TLS security:** The current weather client calls `client.setInsecure()`, **disabling TLS certificate verification**. This is an outstanding security improvement, not a recommended secure production configuration.
- **Concurrency:** Shared refresh flags and weather state are accessed across tasks; additional synchronization and race-condition tests are recommended for future revisions.
- **Reproducibility:** Hardware wiring, exact board-menu settings and performance benchmarks must be confirmed on the actual build.
- **Personalization:** The original firmware includes a Canada/UK themed animation. Review title and visual assets before wider release.

## Validation

See [docs/TEST_PLAN.md](docs/TEST_PLAN.md) for startup, offline behavior, buttons, photo indexing, refresh and time-zone checks. This package has been **statically reviewed but not compiled or hardware-tested** in this environment.

## Credits and data services

Weather data: [Open-Meteo](https://open-meteo.com/) (check current attribution and usage terms before distributing an application). Time synchronization uses public NTP services. External software libraries retain their respective licenses.

## Author and portfolio

**Hasan Fairuz** — Mechatronic Systems Engineering, Simon Fraser University. [GitHub profile](https://github.com/HasanFairuz) · [Engineering portfolio](https://icarus909.com)

*Project source is provided for engineering demonstration and learning. No repository license has been chosen; permission to reuse the author's code is not automatically granted.*
