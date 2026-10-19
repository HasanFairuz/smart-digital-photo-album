# Configuration and privacy guide

## Do this before building

`firmware/OurPhotoAlbum/config.example.h` is a documented, safe template. **Copy it to `config.h`** in the same directory. Edit the copy, never the template. `.gitignore` excludes `config.h` from ordinary Git staging.

| Setting | Private? | What to enter / where to find it |
| --- | --- | --- |
| `WIFI_SSIDS` | Often personal | Exact network names (SSID) from your router admin page or Wi-Fi list. Keep names private if they identify you, your employer, or your home. |
| `WIFI_PASSWORDS` | **Secret** | The corresponding Wi-Fi passwords you control; use your router admin page or own password manager. Never share or commit. |
| `WIFI_COUNT` | No | Number of entries in **both** arrays. Keep those sizes matched. |
| `LATITUDE`, `LONGITUDE` | Potentially private | Decimal degrees for the **weather location**, available from a maps app or OpenStreetMap. Prefer city-level coordinates for a public example; keep precise home coordinates private. |
| `TIME_ZONE` | Usually not secret | POSIX TZ string for your region and DST rules. The template gives Pacific time with DST. This is **not** a plain IANA label like `America/Vancouver`; look up an ESP32/Arduino POSIX timezone reference. |
| `NTP_SERVER_1/2` | No | Public NTP pool hosts (e.g. `pool.ntp.org`) or region-specific pool servers listed at `ntppool.org`. |
| Open-Meteo endpoint | No | The URL in `fetchWeather()` is a public API address, not a token. No API key is used. See https://open-meteo.com/en/docs. |
| Timing macros | No | `INFO_DISPLAY_TIME`, `REFRESH_SCREEN_TIME`, `PHOTO_DISPLAY_TIME`, etc. are durations in **milliseconds** near the start of the sketch; do not remove for privacy. |
| Display / GPIO constants | No | Actual electrical connections; update only to match a different hardware design. |

## Review other information

- Original animation text included a personal-looking `TF2122` identifier. The published starter changes the rendered title to `MOMENTS`; ensure all media, titles and artwork are suitable for public viewing.
- The source draws Canadian and UK flags; these are ordinary graphics but reflect the original personalized presentation.
- `Serial` logs connected Wi-Fi SSIDs and a weather URL containing configured coordinates. Avoid posting unredacted serial logs.
- Never commit your private photo JPEGs; use synthetic/sample images if you later want a public example.
- Before a public release, review screenshots/videos and verify they reveal no private identifiers or home information.
- `client.setInsecure()` disables HTTPS certificate checking. Track this as a security limitation until corrected and tested.

## Verify Git tracking before committing

From the local repository, `git status` should **not** list `config.h` or photo files. From GitHub Desktop, the Changes list should contain only intended public files. `.gitignore` does not erase a secret that was previously committed. If secrets were ever committed, replace/rotate them and handle repository-history removal separately.
