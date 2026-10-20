
# Hardware Testing & Validation

**Project:** Smart Digital Photo Album & Weather Display  
**Developer:** Hasan Fairuz  
**Firmware:** v1.5  
**Hardware:** ESP32-S3 SuperMini, ST7789 TFT Display  
**Testing Method:** Manual Functional Verification

---

## 1. Testing Overview

The firmware was compiled, uploaded, and tested on the
assembled ESP32-S3 prototype.

Functional verification was performed by interacting
with the physical buttons and visually inspecting
the display to confirm that the implemented features
operated as intended.

No automated testing framework or specialized
measurement equipment was used.

## 2. Functional Test Results

| Test | Expected Behaviour | Result |
|---|---|---|
| Firmware Upload | Firmware compiles and uploads successfully | PASS |
| System Startup | Device initializes and displays the startup screen | PASS |
| Wi-Fi Connection | Device connects to an available configured network | PASS |
| Clock & Date | Correct time and date are displayed | PASS |
| Weather Display | Weather information is retrieved and displayed | PASS |
| Album Button | Opens the photo album and advances photographs | PASS |
| Photo Rendering | Stored JPEG images display correctly | PASS |
| Automatic Slideshow | Photographs advance at configured intervals | PASS |
| Refresh Button | Refreshes the clock and weather information | PASS |
| Display Transitions | Screens transition as intended | PASS |
| Offline Operation | Album remains accessible without Wi-Fi | NOT TESTED |
| Network Recovery | Device reconnects when Wi-Fi becomes available | NOT TESTED |

**Result Key:**
- **PASS:** Expected functionality was observed.
- **FAIL:** Expected functionality was not achieved.
- **NOT TESTED:** Functionality has not yet been verified.

Results reflect initial manual testing. Any function
not personally verified should be marked NOT TESTED.

## 3. Observations

The firmware operated successfully on the physical
prototype during normal use.

The TFT display, physical buttons, photo album,
clock, and weather functions responded as expected.

No obvious functional failures were observed during
the initial testing session.

## 4. Limitations & Future Testing

The following areas have not been formally evaluated:

- Extended-duration system stability
- Precise button response and display timing
- Electrical current consumption
- Wi-Fi reconnection reliability
- Error handling under abnormal conditions
- Long-term memory and system performance

Future testing may include additional network
failure scenarios, timing measurements, and
extended runtime evaluation.

## 5. Conclusion

Initial manual functional testing confirmed that
the prototype's primary features operated as
intended under normal conditions.

Further testing is required to establish long-term
reliability, performance, and robustness.

---

**Developer:** Hasan Fairuz  
**GitHub:** https://github.com/HasanFairuz  
**Portfolio:** https://hasanfairuz.com
