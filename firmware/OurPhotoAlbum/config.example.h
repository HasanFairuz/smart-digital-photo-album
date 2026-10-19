#pragma once
// LOCAL DEVICE SETTINGS TEMPLATE
// 1. Copy this file in the same folder as the .ino file and rename it config.h.
// 2. Edit config.h only. It is ignored by Git; keep this example public.
// 3. Never include actual passwords, API tokens or private location details here.

#define WIFI_COUNT 3 // Number of networks in both arrays (change both arrays together).
const char* WIFI_SSIDS[WIFI_COUNT] = {
  "YOUR_WIFI_SSID_1", // Exact Wi-Fi name (SSID), found in your router settings or device Wi-Fi menu.
  "YOUR_WIFI_SSID_2", // Second trusted Wi-Fi network; replace or reduce WIFI_COUNT if not used.
  "YOUR_WIFI_SSID_3"  // Third trusted Wi-Fi network; update the corresponding password below.
};
const char* WIFI_PASSWORDS[WIFI_COUNT] = {
  "YOUR_WIFI_PASSWORD_1", // Password for SSID 1; find in router admin panel or your own records.
  "YOUR_WIFI_PASSWORD_2", // Password for SSID 2. NEVER commit your completed config.h.
  "YOUR_WIFI_PASSWORD_3"  // Password for SSID 3. NEVER commit your completed config.h.
};

// Weather forecast position: decimal degrees (latitude -90..90, longitude -180..180).
// Find coordinates for the weather LOCATION YOU WANT (not necessarily your home)
// using OpenStreetMap or a maps app. Avoid precise home coordinates in public commits.
const float LATITUDE = 49.28f;   // Example only: approximate city center of Vancouver.
const float LONGITUDE = -123.10f; // Example only: approximate city center of Vancouver.

// POSIX timezone string used by ESP32 configTzTime(), NOT a plain UTC offset.
// Example shown: Vancouver / Pacific, includes automatic daylight saving time.
// Find the correct POSIX TZ string for your region in the IANA tz database's
// POSIX rules/reference material, or via an ESP32 POSIX timezone reference.
// Do not use a bare "UTC-8" if you need daylight-saving transitions.
const char* TIME_ZONE = "PST8PDT,M3.2.0,M11.1.0";

// Public Network Time Protocol (NTP) servers used to synchronize the clock.
// These addresses are public, not secrets. Usually no changes are necessary.
// Choose a geographically appropriate pool from https://www.ntppool.org/.
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "time.nist.gov";

// No weather API key is required: this project uses the public Open-Meteo API.
// Public endpoint documentation: https://open-meteo.com/en/docs
