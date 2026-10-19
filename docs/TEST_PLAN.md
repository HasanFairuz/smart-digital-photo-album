# Hardware verification checklist

Record date, board/core/library versions, partition configuration, and tester before publishing measured results.

- [ ] Build with a newly copied `config.h` and no missing libraries.
- [ ] Flash to the ESP32-S3 SuperMini and open Serial Monitor at 115200 baud.
- [ ] Verify boot with all buttons released (especially GPIO3).
- [ ] With an available configured network, check Wi-Fi connection and SNTP time synchronization.
- [ ] Confirm current Open-Meteo weather values and a reasonable location.
- [ ] Verify time zone and DST behavior around known local transitions.
- [ ] With Wi-Fi unavailable at cold boot, confirm the album button still works; confirm unsynchronized time placeholders.
- [ ] Confirm physical refresh button interrupts album playback and displays refresh UI.
- [ ] Check refresh UI stays visible about 3 s and resulting info screen about 10 s.
- [ ] Confirm JPEG files (240 × 240 baseline, .jpg) display in alphabetical order from LittleFS.
- [ ] Confirm oversized/invalid JPEG images are rejected gracefully.
- [ ] Confirm fast album-button presses register without duplicate advances.
- [ ] Confirm display backlight times out and repeated interactions resume as expected.
- [ ] Record power/current draw and photo render latency when measurement equipment is available.
- [ ] Review serial logs, screenshots, photo assets and repository commit before public release.

**Status:** Not yet executed on redistributed starter; no measured outcomes should be claimed.
