/*
 * OurPhotoAlbum_v1.5
 * ─────────────────────────────────────────────────────────────────────────────
 * Hardware: ESP32-S3 SuperMini + 1.54" ST7789 TFT (240x240)
 *
 * Modes:
 *   Button 1 / Album   (GPIO2) - start album / advance / restart album timer
 *   Button 2 / Refresh (GPIO3) - refresh weather/time, then show info for 10 s
 *   Button 3 / Reset   (RST/EN) - hardware reset and Wi-Fi reconnect
 *
 * Photos:
 *   LittleFS baseline JPEG .jpg/.jpeg files, prepared at 240x240.
 *   JPEGDEC decodes directly to RGB565 and draws at native TFT resolution.
 *
 * Notes:
 *   - GPIO3 is an ESP32-S3 strapping pin. Do not hold Button 2 during reset/boot.
 *   - Copy config.example.h to config.h and fill in your own settings.
 *   - This published sketch contains no personal Wi-Fi credentials or locations.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <JPEGDEC.h>
#include <time.h>
#include <esp_arduino_version.h>

// Network credentials, weather location, and timezone are intentionally kept in
// an untracked local header. Copy config.example.h to config.h and edit it.
// Do not commit config.h, passwords, tokens, or personal photo assets.
#include "config.h"

// ── Display Pins ──────────────────────────────────────────────────────────────
#define TFT_BL    7
#define TFT_RST   8
#define TFT_DC    9
#define TFT_CS   10
#define TFT_MOSI 11
#define TFT_SCLK 12

// ── Buttons ───────────────────────────────────────────────────────────────────
#define BUTTON_ALBUM  2   // Button 1 - album / advance
#define BUTTON_INFO   3   // Button 2 - refresh / clock / weather

// ── Display / Backlight ───────────────────────────────────────────────────────
#define TFT_WIDTH   240
#define TFT_HEIGHT  240
#define TFT_SPI_FREQUENCY 40000000UL   // 40 MHz for faster photo redraws

#define BACKLIGHT_FREQUENCY   5000
#define BACKLIGHT_RESOLUTION     8
#define BACKLIGHT_DUTY          217   // Adjustable: ~85% of 255

#if ESP_ARDUINO_VERSION_MAJOR < 3
#define BACKLIGHT_CHANNEL         0
#endif

// ── Photo Settings ────────────────────────────────────────────────────────────
#define PHOTO_WIDTH       240
#define PHOTO_HEIGHT      240
#define PHOTO_MAX_IMAGES   25

// ── Timing ────────────────────────────────────────────────────────────────────
#define INFO_DISPLAY_TIME       10000UL
#define REFRESH_SCREEN_TIME       3000UL   // Refresh screen is visible for max 3 s
#define PHOTO_DISPLAY_TIME        5000UL   // Each photo remains for max 5 s
#define ALBUM_SESSION_TIME       10000UL   // Two automatic 5-second photo periods
#define HEART_ANIMATION_TIME      3000UL
#define TITLE_SCREEN_TIME         3000UL   // Photo Moments: 3 s, skippable
#define ANIMATION_FRAME_TIME       35UL
#define DEBOUNCE_TIME              35UL
#define ALBUM_IRQ_DEBOUNCE        160UL
#define WIFI_ATTEMPT_TIME        8000UL
#define INFO_REFRESH_INTERVAL    1000UL

// ── Time / Weather configuration ─────────────────────────────────────────────
// TIME_ZONE, NTP_SERVER_1/2, LATITUDE and LONGITUDE are in local config.h.
// Open-Meteo is a public, keyless API; its endpoint is defined in fetchWeather().
#define JSON_MEMORY_BUFFER (1024 * 4)

// ── Colors ────────────────────────────────────────────────────────────────────
#define COLOR_SKYBLUE  0x867D
#define COLOR_DARKGREY 0x2104
#define COLOR_PINK     0xFB56
#define COLOR_UK_BLUE  0x0011
#define COLOR_UK_RED   0xC800

// ── Wi-Fi Bitmap ──────────────────────────────────────────────────────────────
const uint8_t wifiBitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x07, 0xc0, 0x00,
  0x00, 0xff, 0xfe, 0x00,
  0x03, 0xff, 0xff, 0x80,
  0x0f, 0xf0, 0x0f, 0xe0,
  0x1f, 0x00, 0x01, 0xf0,
  0x3c, 0x3f, 0xf0, 0x78,
  0x78, 0xff, 0xfe, 0x3c,
  0x23, 0xff, 0xff, 0x88,
  0x07, 0xc0, 0x0f, 0xc0,
  0x0f, 0x07, 0xc1, 0xe0,
  0x06, 0x3f, 0xf8, 0xc0,
  0x00, 0xff, 0xfe, 0x00,
  0x00, 0xf8, 0x3e, 0x00,
  0x00, 0xe0, 0x0e, 0x00,
  0x00, 0x0f, 0xe0, 0x00,
  0x00, 0x1f, 0xf0, 0x00,
  0x00, 0x1f, 0xf0, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x03, 0x80, 0x00,
  0x00, 0x03, 0x80, 0x00,
  0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// ── Weather Icon Classification ───────────────────────────────────────────────
#define WINDY_THRESHOLD_KPH 30

enum WeatherIcon {
  ICON_SUNNY,
  ICON_PARTLY_CLOUDY,
  ICON_CLOUDY,
  ICON_RAIN,
  ICON_SNOW,
  ICON_THUNDERSTORM,
  ICON_WINDY,
  ICON_FOGGY,
  ICON_NIGHT
};

// ── Screen State ──────────────────────────────────────────────────────────────
enum ScreenMode {
  MODE_OFF,
  MODE_INFO,
  MODE_ALBUM
};

enum AlbumStage {
  ALBUM_IDLE,
  ALBUM_ANIMATION,
  ALBUM_TITLE,
  ALBUM_PHOTOS
};

// ── Display Object ────────────────────────────────────────────────────────────
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
JPEGDEC jpeg;

// ── Button Debounce ───────────────────────────────────────────────────────────
struct DebouncedButton {
  uint8_t pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChange;
};

DebouncedButton infoButton;
DebouncedButton albumButton;

// Album button is also interrupt-latched so a quick press is not lost while
// JPEGDEC is busy rendering a frame.
volatile bool albumPressIRQ = false;
unsigned long lastAlbumPressHandled = 0;

// ── Clock / Weather State ─────────────────────────────────────────────────────
String hourText       = "--";
String minuteText     = "--";
String alternative    = "--";
String weekDay        = "---";
String monthText      = "---";

int dayNumber         = 0;
int yearNumber        = 0;
int temperatureC      = 0;
int weatherCode       = 1000;

float windSpeedKph    = 0.0f;

bool isDay            = true;
bool timeValid        = false;
bool weatherValid     = false;

WeatherIcon lastDrawnIcon = (WeatherIcon)-1;

// ── Photo State ───────────────────────────────────────────────────────────────
String photoFiles[PHOTO_MAX_IMAGES];
int photoCount        = 0;
int currentPhoto      = 0;

AlbumStage albumStage = ALBUM_IDLE;

unsigned long albumStageStart   = 0;
unsigned long animationLastDraw = 0;
unsigned long nextPhotoTime     = 0;
unsigned long albumDeadline     = 0;

// ── General State ─────────────────────────────────────────────────────────────
ScreenMode screenMode = MODE_OFF;

bool backlightOn = false;

unsigned long screenDeadline = 0;
unsigned long lastInfoDraw   = 0;

// Non-blocking refresh state. Network work runs on the second ESP32-S3 core
// while the main loop remains responsive to both buttons.
volatile bool refreshTaskRunning = false;
volatile bool refreshTaskDone    = false;
bool refreshScreenActive         = false;
unsigned long refreshScreenStart = 0;

// ── Function Prototypes ───────────────────────────────────────────────────────
void setupBacklight();
void writeBacklight(uint8_t duty);
void setBacklight(bool on);
void setScreenTimeout(unsigned long duration);

void initButton(DebouncedButton &button, uint8_t pin);
bool buttonPressed(DebouncedButton &button);
void IRAM_ATTR albumButtonISR();
bool consumeAlbumPress();

bool connectWiFi(bool showStatus);
bool tryWiFiNetwork(int index, bool showStatus);
void syncTime();
bool updateTimeStrings();
bool fetchWeather();

uint16_t getTempColor(int temperature);
WeatherIcon classifyWeather(int code, float windKph, bool dayTime);
void drawWeatherIcon(WeatherIcon icon, int x, int y, int size);
void drawBitmapScaled(int x, int y, const uint8_t* bitmap,
                      int width, int height, float scale, uint16_t color);
void drawInfoScreen(bool fullRefresh);
void enterInfoMode(bool refreshData);
void startInfoRefresh();
void updateInfoRefresh();
void refreshTask(void* parameter);

void buildPhotoIndex();
bool drawPhoto(int index);
void drawPhotoError(const String &message);

void* jpegOpenFile(const char* filename, int32_t* size);
void jpegCloseFile(void* handle);
int32_t jpegReadFile(JPEGFILE* handle, uint8_t* buffer, int32_t length);
int32_t jpegSeekFile(JPEGFILE* handle, int32_t position);
int jpegDrawBlock(JPEGDRAW* draw);
void startAlbumSequence();
void advanceAlbumStage();
void updateAlbum();
void showAlbumAnimationStart();
void updateAlbumAnimation();
void showAlbumTitle();
void showFirstPhoto();
void advancePhoto(bool restartSession);

void drawCenteredText(const String &text, int centerY,
                      uint8_t size, uint16_t color);
void drawThickLine(int x0, int y0, int x1, int y1,
                   uint16_t color, uint8_t thickness);
void drawCanadaFlag(int x, int y, int width, int height);
void drawUKFlag(int x, int y, int width, int height);
void drawHeart(int centerX, int centerY, int size, uint16_t color);

// ── Backlight Control ─────────────────────────────────────────────────────────
void setupBacklight() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(TFT_BL, BACKLIGHT_FREQUENCY, BACKLIGHT_RESOLUTION);
#else
  ledcSetup(BACKLIGHT_CHANNEL, BACKLIGHT_FREQUENCY, BACKLIGHT_RESOLUTION);
  ledcAttachPin(TFT_BL, BACKLIGHT_CHANNEL);
#endif

  writeBacklight(0);
}

void writeBacklight(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(TFT_BL, duty);
#else
  ledcWrite(BACKLIGHT_CHANNEL, duty);
#endif
}

void setBacklight(bool on) {
  writeBacklight(on ? BACKLIGHT_DUTY : 0);
  backlightOn = on;
}

void setScreenTimeout(unsigned long duration) {
  setBacklight(true);
  screenDeadline = millis() + duration;
}

// ── Button Debouncing ─────────────────────────────────────────────────────────
void initButton(DebouncedButton &button, uint8_t pin) {
  button.pin = pin;
  pinMode(pin, INPUT_PULLUP);

  button.lastReading = digitalRead(pin);
  button.stableState = button.lastReading;
  button.lastChange  = millis();
}

bool buttonPressed(DebouncedButton &button) {
  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {
    button.lastReading = reading;
    button.lastChange = millis();
  }

  if ((millis() - button.lastChange) >= DEBOUNCE_TIME &&
      reading != button.stableState) {
    button.stableState = reading;
    return button.stableState == LOW;
  }

  return false;
}


// ── Album Button Interrupt Latch ──────────────────────────────────────────────
// FALLING edge = active-LOW button press. The ISR only sets a flag; all display
// work remains in loop().
void IRAM_ATTR albumButtonISR() {
  albumPressIRQ = true;
}

bool consumeAlbumPress() {
  bool pending;

  noInterrupts();
  pending = albumPressIRQ;
  albumPressIRQ = false;
  interrupts();

  if (!pending) {
    return false;
  }

  unsigned long now = millis();

  // Ignore switch bounce / duplicate falling edges from one physical press.
  if ((now - lastAlbumPressHandled) < ALBUM_IRQ_DEBOUNCE) {
    return false;
  }

  lastAlbumPressHandled = now;
  return true;
}

// ── Wi-Fi Connection ──────────────────────────────────────────────────────────
bool tryWiFiNetwork(int index, bool showStatus) {
  if (showStatus) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(40, 80);
    tft.println("Connecting to:");

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(40, 108);
    tft.println(WIFI_SSIDS[index]);
  }

  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(WIFI_SSIDS[index], WIFI_PASSWORDS[index]);

  unsigned long start = millis();
  int dotCount = 0;

  while (WiFi.status() != WL_CONNECTED &&
         (millis() - start) < WIFI_ATTEMPT_TIME) {
    if (showStatus) {
      dotCount = (dotCount % 3) + 1;
      tft.fillRect(40, 132, 60, 14, ST77XX_BLACK);
      tft.setCursor(40, 132);
      for (int i = 0; i < dotCount; i++) tft.print(".");
    }

    delay(300);
  }

  return WiFi.status() == WL_CONNECTED;
}

bool connectWiFi(bool showStatus) {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  if (showStatus) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(50, 110);
    tft.println("Scanning...");
  }

  int found = WiFi.scanNetworks();
  bool detected[WIFI_COUNT] = {false, false, false};

  for (int saved = 0; saved < WIFI_COUNT; saved++) {
    for (int visible = 0; visible < found; visible++) {
      if (WiFi.SSID(visible) == WIFI_SSIDS[saved]) {
        detected[saved] = true;
        break;
      }
    }
  }

  WiFi.scanDelete();

  // First try saved networks actually seen in the scan, in priority order.
  for (int i = 0; i < WIFI_COUNT; i++) {
    if (detected[i] && tryWiFiNetwork(i, showStatus)) {
      if (showStatus) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
        tft.setCursor(50, 100);
        tft.println("Connected!");

        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(50, 125);
        tft.println(WiFi.SSID());
        delay(1200);
      }
      return true;
    }
  }

  // Fall back to any saved networks not detected by the scan.
  for (int i = 0; i < WIFI_COUNT; i++) {
    if (!detected[i] && tryWiFiNetwork(i, showStatus)) {
      if (showStatus) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
        tft.setCursor(50, 100);
        tft.println("Connected!");

        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(50, 125);
        tft.println(WiFi.SSID());
        delay(1200);
      }
      return true;
    }
  }

  if (showStatus) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(55, 100);
    tft.println("Offline");

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(35, 130);
    tft.println("Button 1 retries Wi-Fi");
    delay(1200);
  }

  return false;
}

// ── Time Synchronization / Formatting ─────────────────────────────────────────
void syncTime() {
  if (WiFi.status() != WL_CONNECTED) return;

  configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);

  if (backlightOn) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(45, 110);
    tft.println("Syncing time...");
  }

  struct tm timeInfo;
  unsigned long start = millis();

  while (!getLocalTime(&timeInfo, 250) &&
         (millis() - start) < 10000UL) {
    delay(100);
  }

  updateTimeStrings();
}

bool updateTimeStrings() {
  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 20)) {
    timeValid = false;
    hourText = "--";
    minuteText = "--";
    alternative = "--";
    weekDay = "---";
    monthText = "---";
    dayNumber = 0;
    yearNumber = 0;
    return false;
  }

  int hour24 = timeInfo.tm_hour;
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;

  char buffer[8];

  snprintf(buffer, sizeof(buffer), "%02d", hour12);
  hourText = buffer;

  snprintf(buffer, sizeof(buffer), "%02d", timeInfo.tm_min);
  minuteText = buffer;

  alternative = (hour24 >= 12) ? "PM" : "AM";

  const char* weekDays[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };

  const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  weekDay = weekDays[timeInfo.tm_wday];
  monthText = months[timeInfo.tm_mon];
  dayNumber = timeInfo.tm_mday;
  yearNumber = timeInfo.tm_year + 1900;

  timeValid = true;
  return true;
}

// ── Weather Fetch ─────────────────────────────────────────────────────────────
bool fetchWeather() {
  weatherValid = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Weather] Wi-Fi is not connected.");
    return false;
  }

  String weatherURL =
    "https://api.open-meteo.com/v1/forecast?latitude=" +
    String(LATITUDE, 4) +
    "&longitude=" +
    String(LONGITUDE, 4) +
    "&current=temperature_2m,weather_code,wind_speed_10m,is_day"
    "&temperature_unit=celsius&wind_speed_unit=kmh";

  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();
  client.setTimeout(10000);
  https.setTimeout(10000);
  https.useHTTP10(true);

  Serial.print("[Weather] Request: ");
  Serial.println(weatherURL);

  if (!https.begin(client, weatherURL.c_str())) {
    Serial.println("[Weather] HTTPS begin() failed.");
    return false;
  }

  https.addHeader("Accept", "application/json");

  int httpCode = https.GET();

  Serial.print("[Weather] HTTP status: ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK) {
    String response = https.getString();

    if (response.length() > 300) {
      response = response.substring(0, 300);
    }

    Serial.print("[Weather] Response: ");
    Serial.println(response);

    https.end();
    return false;
  }

  DynamicJsonDocument doc(JSON_MEMORY_BUFFER);
  DeserializationError error =
    deserializeJson(doc, https.getStream());

  if (error) {
    Serial.print("[Weather] JSON error: ");
    Serial.println(error.c_str());
    https.end();
    return false;
  }

  if (doc["current"].isNull()) {
    Serial.println("[Weather] Missing 'current' object.");
    https.end();
    return false;
  }

  temperatureC = round(doc["current"]["temperature_2m"].as<float>());
  weatherCode  = doc["current"]["weather_code"].as<int>();
  windSpeedKph = doc["current"]["wind_speed_10m"].as<float>();
  isDay        = doc["current"]["is_day"].as<int>() == 1;

  weatherValid = true;

  Serial.print("[Weather] Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" C");

  Serial.print("[Weather] WMO code: ");
  Serial.println(weatherCode);

  Serial.print("[Weather] Wind: ");
  Serial.print(windSpeedKph);
  Serial.println(" kph");

  https.end();
  return true;
}

// ── Temperature Color Scale ───────────────────────────────────────────────────
uint16_t getTempColor(int temperature) {
  if (temperature < 18)  return COLOR_SKYBLUE;
  if (temperature <= 22) return ST77XX_YELLOW;
  return ST77XX_ORANGE;
}

// ── Weather Icon Selection ────────────────────────────────────────────────────
WeatherIcon classifyWeather(int code, float windKph, bool dayTime) {
  // Open-Meteo uses WMO weather interpretation codes.
  if (code == 95 || code == 96 || code == 99) {
    return ICON_THUNDERSTORM;
  }

  if ((code >= 71 && code <= 77) ||
      code == 85 || code == 86) {
    return ICON_SNOW;
  }

  if ((code >= 51 && code <= 67) ||
      (code >= 80 && code <= 82)) {
    return ICON_RAIN;
  }

  if (code == 45 || code == 48) {
    return ICON_FOGGY;
  }

  if (windKph > WINDY_THRESHOLD_KPH) {
    return ICON_WINDY;
  }

  if (code == 3) {
    return ICON_CLOUDY;
  }

  if (code == 1 || code == 2) {
    return ICON_PARTLY_CLOUDY;
  }

  if (code == 0 && !dayTime) {
    return ICON_NIGHT;
  }

  return ICON_SUNNY;
}

// ── Weather Icon Rendering ────────────────────────────────────────────────────
void drawWeatherIcon(WeatherIcon icon, int x, int y, int size) {
  float scale = size / 40.0f;
  auto S = [scale](float value) {
    return (int)(value * scale);
  };

  tft.fillRect(x, y, size, size, ST77XX_BLACK);

  switch (icon) {
    case ICON_SUNNY:
      tft.fillCircle(x + S(20), y + S(20), S(10), ST77XX_YELLOW);
      for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4.0f;
        tft.drawLine(
          x + S(20) + S(12) * cos(angle),
          y + S(20) + S(12) * sin(angle),
          x + S(20) + S(18) * cos(angle),
          y + S(20) + S(18) * sin(angle),
          ST77XX_YELLOW
        );
      }
      break;

    case ICON_PARTLY_CLOUDY:
      tft.fillCircle(x + S(14), y + S(14), S(8), ST77XX_YELLOW);
      tft.fillCircle(x + S(16), y + S(26), S(8), 0x8410);
      tft.fillCircle(x + S(26), y + S(22), S(10), ST77XX_WHITE);
      tft.fillRect(x + S(16), y + S(26), S(18), S(8), ST77XX_WHITE);
      break;

    case ICON_CLOUDY:
      tft.fillCircle(x + S(12), y + S(24), S(8), 0x8410);
      tft.fillCircle(x + S(20), y + S(18), S(10), ST77XX_WHITE);
      tft.fillCircle(x + S(30), y + S(24), S(7), ST77XX_WHITE);
      tft.fillRect(x + S(12), y + S(24), S(20), S(8), ST77XX_WHITE);
      break;

    case ICON_RAIN:
      tft.fillCircle(x + S(12), y + S(16), S(7), 0x8410);
      tft.fillCircle(x + S(20), y + S(10), S(9), 0x8410);
      tft.fillCircle(x + S(28), y + S(16), S(6), 0x8410);
      tft.fillRect(x + S(12), y + S(16), S(18), S(6), 0x8410);
      tft.drawLine(x + S(14), y + S(26), x + S(11), y + S(34), ST77XX_CYAN);
      tft.drawLine(x + S(20), y + S(26), x + S(17), y + S(34), ST77XX_CYAN);
      tft.drawLine(x + S(26), y + S(26), x + S(23), y + S(34), ST77XX_CYAN);
      break;

    case ICON_SNOW:
      tft.fillCircle(x + S(12), y + S(16), S(7), 0x8410);
      tft.fillCircle(x + S(20), y + S(10), S(9), 0x8410);
      tft.fillCircle(x + S(28), y + S(16), S(6), 0x8410);
      tft.fillRect(x + S(12), y + S(16), S(18), S(6), 0x8410);
      tft.drawLine(x + S(14), y + S(26), x + S(14), y + S(34), ST77XX_WHITE);
      tft.drawLine(x + S(11), y + S(30), x + S(17), y + S(30), ST77XX_WHITE);
      tft.drawLine(x + S(24), y + S(26), x + S(24), y + S(34), ST77XX_WHITE);
      tft.drawLine(x + S(21), y + S(30), x + S(27), y + S(30), ST77XX_WHITE);
      break;

    case ICON_THUNDERSTORM:
      tft.fillCircle(x + S(12), y + S(14), S(7), 0x4208);
      tft.fillCircle(x + S(20), y + S(8),  S(9), 0x4208);
      tft.fillCircle(x + S(28), y + S(14), S(6), 0x4208);
      tft.fillRect(x + S(12), y + S(14), S(18), S(6), 0x4208);
      tft.fillTriangle(
        x + S(22), y + S(22),
        x + S(14), y + S(32),
        x + S(20), y + S(30),
        ST77XX_YELLOW
      );
      tft.fillTriangle(
        x + S(20), y + S(30),
        x + S(26), y + S(22),
        x + S(22), y + S(22),
        ST77XX_YELLOW
      );
      tft.fillTriangle(
        x + S(18), y + S(30),
        x + S(12), y + S(38),
        x + S(20), y + S(33),
        ST77XX_YELLOW
      );
      break;

    case ICON_WINDY:
      tft.drawLine(x + S(6), y + S(14), x + S(28), y + S(14), ST77XX_WHITE);
      tft.drawLine(x + S(28), y + S(14), x + S(28), y + S(10), ST77XX_WHITE);
      tft.drawLine(x + S(6), y + S(22), x + S(32), y + S(22), ST77XX_WHITE);
      tft.drawLine(x + S(32), y + S(22), x + S(32), y + S(26), ST77XX_WHITE);
      tft.drawLine(x + S(6), y + S(30), x + S(24), y + S(30), ST77XX_WHITE);
      tft.drawLine(x + S(24), y + S(30), x + S(24), y + S(34), ST77XX_WHITE);
      break;

    case ICON_FOGGY:
      for (int i = 0; i < 4; i++) {
        tft.drawFastHLine(x + S(4), y + S(10 + i * 8), S(32), 0x8410);
        tft.drawFastHLine(x + S(4), y + S(11 + i * 8), S(32), 0x8410);
      }
      break;

    case ICON_NIGHT:
      tft.fillCircle(x + S(20), y + S(20), S(12), ST77XX_WHITE);
      tft.fillCircle(x + S(26), y + S(16), S(11), ST77XX_BLACK);
      tft.fillCircle(x + S(8), y + S(10), S(2), ST77XX_CYAN);
      tft.fillCircle(x + S(32), y + S(30), S(2), ST77XX_CYAN);
      tft.fillCircle(x + S(6), y + S(32), S(1), ST77XX_CYAN);
      break;
  }
}

// ── Bitmap Scaling ────────────────────────────────────────────────────────────
void drawBitmapScaled(int x, int y, const uint8_t* bitmap,
                      int width, int height, float scale, uint16_t color) {
  int bytesPerRow = (width + 7) / 8;
  int outputWidth = (int)(width * scale);
  int outputHeight = (int)(height * scale);

  for (int outputY = 0; outputY < outputHeight; outputY++) {
    int sourceY = (int)(outputY / scale);
    if (sourceY >= height) sourceY = height - 1;

    for (int outputX = 0; outputX < outputWidth; outputX++) {
      int sourceX = (int)(outputX / scale);
      if (sourceX >= width) sourceX = width - 1;

      uint8_t byteValue =
        pgm_read_byte(&bitmap[sourceY * bytesPerRow + (sourceX / 8)]);

      bool set =
        (byteValue >> (7 - (sourceX % 8))) & 0x01;

      if (set) {
        tft.drawPixel(x + outputX, y + outputY, color);
      }
    }
  }
}

// ── Info Screen ───────────────────────────────────────────────────────────────
void drawInfoScreen(bool fullRefresh) {
  const int fontW = 6;
  const int fontH = 8;
  const int timeSize = 6;
  const int colonSize = 4;
  const int altSize = 3;
  const int degreeSize = 2;
  const int daySize = 3;
  const int padding = 8;
  const int timeY = 40;
  const float wifiScale = 0.65f;

  updateTimeStrings();

  if (fullRefresh) {
    tft.fillScreen(ST77XX_BLACK);
    lastDrawnIcon = (WeatherIcon)-1;
  }

  int blockWidth =
    (fontW * timeSize * 4) + (fontW * colonSize);

  int timeX =
    (TFT_WIDTH / 2) - (blockWidth / 2) - (fontW * altSize);

  // Time
  tft.setTextSize(timeSize);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(timeX, timeY);
  tft.print(hourText);

  int colonX = tft.getCursorX();

  tft.setTextSize(colonSize);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(
    colonX,
    timeY + (fontH * (timeSize - colonSize)) / 2
  );
  tft.print(":");

  int minuteX = tft.getCursorX();

  tft.setTextSize(timeSize);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(minuteX, timeY);
  tft.print(minuteText);

  int rightColumnX = tft.getCursorX();

  // AM / PM
  tft.setTextSize(altSize);
  uint16_t altColor =
    alternative == "AM" ? ST77XX_CYAN : ST77XX_YELLOW;

  tft.setTextColor(altColor, ST77XX_BLACK);
  tft.setCursor(rightColumnX, timeY);
  tft.print(alternative);

  // Wi-Fi icon
  int wifiX = rightColumnX;
  int wifiY = timeY + (fontH * altSize) + 4;

  tft.fillRect(wifiX, wifiY, 24, 18, ST77XX_BLACK);

  drawBitmapScaled(
    wifiX,
    wifiY,
    wifiBitmap,
    31,
    24,
    wifiScale,
    WiFi.status() == WL_CONNECTED ? ST77XX_WHITE : ST77XX_RED
  );

  // Weekday
  int weekdayY =
    timeY + (fontH * timeSize) + padding + 10;

  tft.setTextSize(daySize);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setCursor(20, weekdayY);
  tft.print(weekDay);

  // Day / month
  int dateY =
    timeY +
    (fontH * timeSize) +
    (fontH * daySize) +
    padding * 2 + 10;

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(20, dateY);

  if (timeValid) {
    tft.printf("%02d", dayNumber);
  } else {
    tft.print("--");
  }

  tft.setCursor(
    20 + (fontW * daySize) * 2 + padding,
    dateY
  );
  tft.print(monthText);

  // Year
  int yearY =
    timeY +
    (fontH * timeSize) +
    (fontH * daySize) * 2 +
    padding * 3 + 10;

  tft.fillRect(16, yearY - 2, 110, 36, ST77XX_BLACK);
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(20, yearY);

  if (timeValid) {
    tft.print(yearNumber);
  } else {
    tft.print("----");
  }

  // Weather icon
  int weatherIconSize = 64;
  int weatherX =
    TFT_WIDTH - (fontW * 4) * 2 - padding - (fontW * altSize);

  int weatherY = weekdayY;

  if (weatherValid) {
    WeatherIcon icon =
      classifyWeather(weatherCode, windSpeedKph, isDay);

    if (icon != lastDrawnIcon) {
      drawWeatherIcon(icon, weatherX, weatherY, weatherIconSize);
      lastDrawnIcon = icon;
    }
  } else {
    tft.fillRect(
      weatherX,
      weatherY,
      weatherIconSize,
      weatherIconSize,
      ST77XX_BLACK
    );

    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(weatherX + 8, weatherY + 20);
    tft.print("WX");
    tft.setCursor(weatherX + 8, weatherY + 38);
    tft.print("ERR");
    lastDrawnIcon = (WeatherIcon)-1;
  }

  // Temperature
  int tempX = 130;
  int tempY = yearY;

  tft.fillRect(tempX - 2, tempY - 2, 110, 36, ST77XX_BLACK);

  if (weatherValid) {
    uint16_t tempColor = getTempColor(temperatureC);

    tft.setTextSize(4);
    tft.setTextColor(tempColor, ST77XX_BLACK);
    tft.setCursor(tempX, tempY);
    tft.print(temperatureC);

    tft.setTextSize(degreeSize);
    tft.print("o");

    tft.setTextSize(4);
    tft.print("C");
  } else {
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.setCursor(tempX, tempY + 4);
    tft.print("--");
    tft.setTextSize(2);
    tft.print("o");
    tft.setTextSize(3);
    tft.print("C");
  }
}

void enterInfoMode(bool refreshData) {
  if (refreshData) {
    startInfoRefresh();
    return;
  }

  screenMode = MODE_INFO;
  refreshScreenActive = false;

  drawInfoScreen(true);
  setScreenTimeout(INFO_DISPLAY_TIME);
  lastInfoDraw = millis();
}

// Network refresh runs separately so "Refreshing..." never blocks the UI.
void refreshTask(void* parameter) {
  refreshTaskRunning = true;
  refreshTaskDone = false;

  // Time continues locally once SNTP has synchronized. Re-sync configuration
  // is harmless and allows recovery after a network change.
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();

    unsigned long reconnectStart = millis();

    while (WiFi.status() != WL_CONNECTED &&
           (millis() - reconnectStart) < 2200UL) {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
    updateTimeStrings();
    fetchWeather();
  } else {
    Serial.println("[Refresh] Wi-Fi unavailable; keeping existing weather data.");
    updateTimeStrings();
  }

  refreshTaskDone = true;
  refreshTaskRunning = false;

  vTaskDelete(nullptr);
}

void startInfoRefresh() {
  // Button 2 always exits the album immediately.
  albumStage = ALBUM_IDLE;
  screenMode = MODE_INFO;

  setBacklight(true);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(52, 105);
  tft.println("Refreshing...");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(73, 135);
  tft.println("Please wait");

  refreshScreenActive = true;
  refreshScreenStart = millis();

  // Do not launch a duplicate network task if one is already finishing.
  if (!refreshTaskRunning) {
    refreshTaskDone = false;

    BaseType_t result = xTaskCreatePinnedToCore(
      refreshTask,
      "WeatherRefresh",
      8192,
      nullptr,
      1,
      nullptr,
      0
    );

    if (result != pdPASS) {
      Serial.println("[Refresh] Failed to create refresh task.");
      refreshTaskRunning = false;
      refreshTaskDone = true;
    }
  }
}

void updateInfoRefresh() {
  if (!refreshScreenActive) {
    return;
  }

  // Always leave the refresh screen after exactly 3 seconds. If the weather
  // request is still finishing in the background, the existing data appears
  // first and the new data is picked up by the normal once-per-second redraw.
  if ((millis() - refreshScreenStart) >= REFRESH_SCREEN_TIME) {
    refreshScreenActive = false;

    updateTimeStrings();
    drawInfoScreen(true);

    setScreenTimeout(INFO_DISPLAY_TIME);
    lastInfoDraw = millis();
  }
}

// ── Photo Index / JPEG Rendering ──────────────────────────────────────────────
void buildPhotoIndex() {
  photoCount = 0;

  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("[Photos] LittleFS root open failed.");
    return;
  }

  File file = root.openNextFile();

  while (file && photoCount < PHOTO_MAX_IMAGES) {
    if (!file.isDirectory()) {
      String name = file.name();
      String lowerName = name;
      lowerName.toLowerCase();

      if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) {
        if (!name.startsWith("/")) name = "/" + name;
        photoFiles[photoCount++] = name;
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();

  // Alphabetical filename order controls slideshow order.
  for (int i = 0; i < photoCount - 1; i++) {
    for (int j = i + 1; j < photoCount; j++) {
      if (photoFiles[j] < photoFiles[i]) {
        String temporary = photoFiles[i];
        photoFiles[i] = photoFiles[j];
        photoFiles[j] = temporary;
      }
    }
  }

  Serial.print("[Photos] JPG files indexed: ");
  Serial.println(photoCount);

  for (int i = 0; i < photoCount; i++) {
    Serial.print("  ");
    Serial.println(photoFiles[i]);
  }
}

// JPEGDEC LittleFS callbacks.
void* jpegOpenFile(const char* filename, int32_t* size) {
  static File jpegFile;

  jpegFile = LittleFS.open(filename, FILE_READ);

  if (!jpegFile) {
    return nullptr;
  }

  *size = jpegFile.size();
  return &jpegFile;
}

void jpegCloseFile(void* handle) {
  File* file = (File*)handle;

  if (file) {
    file->close();
  }
}

int32_t jpegReadFile(JPEGFILE* handle, uint8_t* buffer, int32_t length) {
  File* file = (File*)handle->fHandle;

  if (!file) {
    return 0;
  }

  return file->read(buffer, length);
}

int32_t jpegSeekFile(JPEGFILE* handle, int32_t position) {
  File* file = (File*)handle->fHandle;

  if (!file) {
    return 0;
  }

  return file->seek(position) ? position : 0;
}

// JPEGDEC supplies RGB565 blocks. BIG_ENDIAN lets Adafruit_SPITFT forward the
// pixels efficiently without another byte swap.
int jpegDrawBlock(JPEGDRAW* draw) {
  if ((draw->x + draw->iWidth) > TFT_WIDTH ||
      (draw->y + draw->iHeight) > TFT_HEIGHT) {
    return 0;
  }

  // drawPhoto() keeps one SPI transaction open for the full JPEG.
  tft.setAddrWindow(draw->x, draw->y, draw->iWidth, draw->iHeight);
  tft.writePixels(
    draw->pPixels,
    draw->iWidth * draw->iHeight,
    true,
    true
  );

  return 1;
}

void drawPhotoError(const String &message) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setCursor(20, 100);
  tft.println(message);
}

bool drawPhoto(int index) {
  if (index < 0 || index >= photoCount) {
    drawPhotoError("No photos found");
    return false;
  }

  String filename = photoFiles[index];

  Serial.print("[Photos] Opening ");
  Serial.println(filename);

  if (!jpeg.open(
        (char*)filename.c_str(),
        jpegOpenFile,
        jpegCloseFile,
        jpegReadFile,
        jpegSeekFile,
        jpegDrawBlock)) {
    Serial.print("[Photos] JPEG open failed, error ");
    Serial.println(jpeg.getLastError());
    drawPhotoError("JPEG open error");
    return false;
  }

  int width  = jpeg.getWidth();
  int height = jpeg.getHeight();

  Serial.print("[Photos] JPEG size: ");
  Serial.print(width);
  Serial.print(" x ");
  Serial.println(height);

  if (width != PHOTO_WIDTH || height != PHOTO_HEIGHT) {
    jpeg.close();

    Serial.println("[Photos] Expected exactly 240x240.");
    drawPhotoError("Use 240x240 JPG");
    return false;
  }

  tft.fillScreen(ST77XX_BLACK);

  jpeg.setPixelType(RGB565_BIG_ENDIAN);

  unsigned long renderStart = millis();

  // Keep the SPI transaction open across all JPEGDEC output blocks.
  tft.startWrite();
  bool decoded = jpeg.decode(0, 0, 0);
  tft.endWrite();

  unsigned long renderTime = millis() - renderStart;
  int jpegError = jpeg.getLastError();

  jpeg.close();

  Serial.print("[Photos] Render time: ");
  Serial.print(renderTime);
  Serial.println(" ms");

  if (!decoded) {
    Serial.print("[Photos] JPEG decode failed, error ");
    Serial.println(jpegError);
    drawPhotoError("JPEG decode error");
    return false;
  }

  return true;
}

// ── Album Intro Graphics ──────────────────────────────────────────────────────
void drawCenteredText(const String &text, int centerY,
                      uint8_t size, uint16_t color) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  tft.setTextSize(size);
  tft.setTextWrap(false);

  tft.getTextBounds(
    text,
    0,
    0,
    &x1,
    &y1,
    &width,
    &height
  );

  int x =
    (TFT_WIDTH - (int)width) / 2;

  int y =
    centerY - (int)height / 2 - y1;

  tft.setTextColor(color, ST77XX_BLACK);
  tft.setCursor(x, y);
  tft.print(text);
}

void drawThickLine(int x0, int y0, int x1, int y1,
                   uint16_t color, uint8_t thickness) {
  int half = thickness / 2;

  for (int offset = -half; offset <= half; offset++) {
    tft.drawLine(
      x0,
      y0 + offset,
      x1,
      y1 + offset,
      color
    );
  }
}

void drawCanadaFlag(int x, int y, int width, int height) {
  int sideWidth = width / 4;
  int centerX = x + width / 2;

  tft.fillRect(x, y, width, height, ST77XX_WHITE);
  tft.fillRect(x, y, sideWidth, height, ST77XX_RED);
  tft.fillRect(
    x + width - sideWidth,
    y,
    sideWidth,
    height,
    ST77XX_RED
  );

  tft.fillTriangle(
    centerX,
    y + 5,
    centerX - 5,
    y + 18,
    centerX + 5,
    y + 18,
    ST77XX_RED
  );

  tft.fillTriangle(
    centerX - 3,
    y + 14,
    centerX - 16,
    y + 11,
    centerX - 10,
    y + 25,
    ST77XX_RED
  );

  tft.fillTriangle(
    centerX + 3,
    y + 14,
    centerX + 16,
    y + 11,
    centerX + 10,
    y + 25,
    ST77XX_RED
  );

  tft.fillTriangle(
    centerX,
    y + 13,
    centerX - 11,
    y + 32,
    centerX + 11,
    y + 32,
    ST77XX_RED
  );

  tft.fillRect(
    centerX - 2,
    y + 30,
    4,
    10,
    ST77XX_RED
  );
}

void drawUKFlag(int x, int y, int width, int height) {
  tft.fillRect(x, y, width, height, COLOR_UK_BLUE);

  drawThickLine(
    x, y,
    x + width - 1, y + height - 1,
    ST77XX_WHITE, 9
  );

  drawThickLine(
    x, y + height - 1,
    x + width - 1, y,
    ST77XX_WHITE, 9
  );

  drawThickLine(
    x, y,
    x + width - 1, y + height - 1,
    COLOR_UK_RED, 3
  );

  drawThickLine(
    x, y + height - 1,
    x + width - 1, y,
    COLOR_UK_RED, 3
  );

  tft.fillRect(
    x + width / 2 - 7,
    y,
    14,
    height,
    ST77XX_WHITE
  );

  tft.fillRect(
    x,
    y + height / 2 - 7,
    width,
    14,
    ST77XX_WHITE
  );

  tft.fillRect(
    x + width / 2 - 3,
    y,
    6,
    height,
    COLOR_UK_RED
  );

  tft.fillRect(
    x,
    y + height / 2 - 3,
    width,
    6,
    COLOR_UK_RED
  );
}

void drawHeart(int centerX, int centerY,
               int size, uint16_t color) {
  int radius = max(2, size / 2);
  int lobeOffset = size / 2;
  int lobeY = centerY - size / 4;

  tft.fillCircle(
    centerX - lobeOffset,
    lobeY,
    radius,
    color
  );

  tft.fillCircle(
    centerX + lobeOffset,
    lobeY,
    radius,
    color
  );

  tft.fillTriangle(
    centerX - size,
    lobeY,
    centerX + size,
    lobeY,
    centerX,
    centerY + size,
    color
  );
}

// ── Non-Blocking Album State Machine ─────────────────────────────────────────
// Button 1 acts like a "next" control at every stage:
//
//   OFF/INFO -> animation
//   animation -> Photo Moments
//   title -> first photo
//   photo -> next photo
//
// Automatic progression:
//   animation       3 s maximum
//   Photo Moments  3 s maximum
//   photo 1         5 s maximum
//   photo 2         5 s maximum
//   display off
//
// During photo mode, every manual Button 1 press immediately shows the next
// photo and restarts the 10-second viewing window.

void showAlbumAnimationStart() {
  const int flagWidth  = 72;
  const int flagHeight = 48;
  const int flagY      = 96;

  tft.fillScreen(ST77XX_BLACK);

  drawCanadaFlag(
    8,
    flagY,
    flagWidth,
    flagHeight
  );

  drawUKFlag(
    TFT_WIDTH - flagWidth - 8,
    flagY,
    flagWidth,
    flagHeight
  );

  albumStage = ALBUM_ANIMATION;
  albumStageStart = millis();

  Serial.println("[Album] Animation");
  animationLastDraw = 0;

  setBacklight(true);
}

void updateAlbumAnimation() {
  if (albumStage != ALBUM_ANIMATION) {
    return;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - albumStageStart;

  if (elapsed >= HEART_ANIMATION_TIME) {
    showAlbumTitle();
    return;
  }

  if ((now - animationLastDraw) < ANIMATION_FRAME_TIME) {
    return;
  }

  animationLastDraw = now;

  const int minimumSize = 4;
  const int maximumSize = 26;

  int heartSize =
    minimumSize +
    (int)(
      elapsed *
      (maximumSize - minimumSize) /
      HEART_ANIMATION_TIME
    );

  tft.fillRect(86, 82, 68, 78, ST77XX_BLACK);

  drawHeart(
    TFT_WIDTH / 2,
    TFT_HEIGHT / 2,
    heartSize,
    COLOR_PINK
  );
}

void showAlbumTitle() {
  tft.fillScreen(ST77XX_BLACK);

  drawCenteredText("MOMENTS", 98, 3, ST77XX_WHITE);
  drawCenteredText("Moments", 142, 3, COLOR_PINK);

  albumStage = ALBUM_TITLE;
  albumStageStart = millis();

  Serial.println("[Album] Photo Moments");

  setBacklight(true);
}

void showFirstPhoto() {
  if (photoCount == 0) {
    albumStage = ALBUM_PHOTOS;
    drawPhotoError("No photos found");

    unsigned long now = millis();
    nextPhotoTime = now + PHOTO_DISPLAY_TIME;
    albumDeadline = now + ALBUM_SESSION_TIME;
    return;
  }

  currentPhoto = 0;

  Serial.print("[Album] Photo ");
  Serial.println(currentPhoto + 1);

  drawPhoto(currentPhoto);

  albumStage = ALBUM_PHOTOS;

  unsigned long now = millis();

  nextPhotoTime = now + PHOTO_DISPLAY_TIME;
  albumDeadline = now + ALBUM_SESSION_TIME;

  setBacklight(true);
}

void startAlbumSequence() {
  screenMode = MODE_ALBUM;
  showAlbumAnimationStart();
}

void advanceAlbumStage() {
  if (screenMode != MODE_ALBUM || !backlightOn) {
    startAlbumSequence();
    return;
  }

  switch (albumStage) {
    case ALBUM_ANIMATION:
      // Skip immediately to the title screen.
      showAlbumTitle();
      break;

    case ALBUM_TITLE:
      // Skip immediately to photo 1.
      showFirstPhoto();
      break;

    case ALBUM_PHOTOS:
      // Immediate next photo + fresh 10-second session.
      advancePhoto(true);
      break;

    case ALBUM_IDLE:
    default:
      startAlbumSequence();
      break;
  }
}

void advancePhoto(bool restartSession) {
  if (screenMode != MODE_ALBUM ||
      albumStage != ALBUM_PHOTOS ||
      photoCount == 0) {
    return;
  }

  currentPhoto =
    (currentPhoto + 1) % photoCount;

  Serial.print(restartSession ? "[Album] Manual -> photo " : "[Album] Auto -> photo ");
  Serial.println(currentPhoto + 1);

  drawPhoto(currentPhoto);

  unsigned long now = millis();

  nextPhotoTime = now + PHOTO_DISPLAY_TIME;

  if (restartSession) {
    albumDeadline = now + ALBUM_SESSION_TIME;
  }
}

void updateAlbum() {
  if (screenMode != MODE_ALBUM || !backlightOn) {
    return;
  }

  unsigned long now = millis();

  switch (albumStage) {
    case ALBUM_ANIMATION:
      updateAlbumAnimation();
      break;

    case ALBUM_TITLE:
      if ((now - albumStageStart) >= TITLE_SCREEN_TIME) {
        showFirstPhoto();
      }
      break;

    case ALBUM_PHOTOS:
      // Deadline check comes first so a third photo is not automatically drawn
      // at the exact end of the 10-second two-photo session.
      if ((long)(now - albumDeadline) >= 0) {
        Serial.println("[Album] 10-second window complete -> display off");
        setBacklight(false);
        screenMode = MODE_OFF;
        albumStage = ALBUM_IDLE;
      }
      else if (photoCount > 0 &&
               (long)(now - nextPhotoTime) >= 0) {
        advancePhoto(false);
      }
      break;

    case ALBUM_IDLE:
    default:
      break;
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Custom SPI pins for the ESP32-S3.
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(TFT_SPI_FREQUENCY);
  tft.setRotation(3);
  tft.setTextWrap(false);

  setupBacklight();

  initButton(infoButton, BUTTON_INFO);
  initButton(albumButton, BUTTON_ALBUM);

  // Button 1 / GPIO2 is the active-LOW album button. Latch every falling edge
  // so quick presses are not lost while a JPEG is being decoded.
  attachInterrupt(
    digitalPinToInterrupt(BUTTON_ALBUM),
    albumButtonISR,
    FALLING
  );

  setBacklight(true);
  tft.fillScreen(ST77XX_BLACK);

  // Photo filesystem.
  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed");
  } else {
    buildPhotoIndex();

    Serial.print("[LittleFS] Used: ");
    Serial.print(LittleFS.usedBytes());
    Serial.print(" / ");
    Serial.println(LittleFS.totalBytes());
  }

  // Configure SNTP timezone/server before Wi-Fi connection.
  configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);

  connectWiFi(true);

  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
    fetchWeather();
  } else {
    updateTimeStrings();
  }

  // Show clock/weather after startup, then turn the backlight off after 10 seconds.
  screenMode = MODE_INFO;
  drawInfoScreen(true);
  setScreenTimeout(INFO_DISPLAY_TIME);
  lastInfoDraw = millis();
}

// ── Main Loop ─────────────────────────────────────────────────────────────────
void loop() {
  bool albumPressed = consumeAlbumPress();
  bool infoPressed  = buttonPressed(infoButton);

  // Button 1 / GPIO2 = ALBUM / NEXT.
  //
  // Every press advances immediately:
  //   OFF/INFO -> animation
  //   animation -> Photo Moments
  //   title -> photo 1
  //   photo -> next photo
  if (albumPressed) {
    refreshScreenActive = false;
    advanceAlbumStage();
  }

  // Button 2 / GPIO3 = REFRESH.
  //
  // Exit album mode immediately, show "Refreshing..." for exactly 3 seconds,
  // then return to the clock/date/weather screen for 10 seconds.
  if (infoPressed && !albumPressed) {
    startInfoRefresh();
  }

  // Album timing stays fully non-blocking, so Button 1 remains responsive.
  updateAlbum();

  // Enforce the 3-second refresh-screen duration.
  updateInfoRefresh();

  // Keep the visible clock current once per second.
  // Do not overwrite the temporary Refreshing screen.
  if (screenMode == MODE_INFO &&
      backlightOn &&
      !refreshScreenActive &&
      (millis() - lastInfoDraw) >= INFO_REFRESH_INTERVAL) {
    drawInfoScreen(false);
    lastInfoDraw = millis();
  }

  // Clock/info screen timeout starts only after the refresh screen has ended.
  if (screenMode == MODE_INFO &&
      backlightOn &&
      !refreshScreenActive &&
      (long)(millis() - screenDeadline) >= 0) {
    setBacklight(false);
    screenMode = MODE_OFF;
  }

  delay(1);
}
