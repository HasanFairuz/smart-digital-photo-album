# Bill of Materials: Smart Digital Photo Album & Weather Display

| Field | Value |
|---|---|
| Project | Smart Digital Photo Album & Weather Display |
| Author | Hasan Fairuz |
| BOM revision | Rev 1.1 (matches schematic Rev 1.1, Sheets 1 and 2) |
| Microcontroller | ESP32-S3 SuperMini |
| Display | 1.54-inch ST7789 SPI TFT, 240 × 240 pixels |
| Inputs | 2 × normally-open momentary push buttons |
| Power | USB 5 V via the ESP32-S3 SuperMini on-board USB-C connector |
| Currency | Canadian dollars (CAD), estimated replacement cost |
| Price research date | 2026-10-19 |
| FX rate used | 1 USD = 1.4095 CAD (mid-market, 2026-10-19; see [S9](#price-sources)) |

---

## 1. Confirmed Components

These parts are part of the completed device and appear on the electrical schematic (Sheet 1) and physical wiring diagram (Sheet 2).

| Item | Component | Specification / Model | Qty | Est. Unit Cost Low (CAD) | Est. Unit Cost High (CAD) | Est. Ext. Cost Low (CAD) | Est. Ext. Cost High (CAD) | Function in System | Representative Supplier / Source | Notes / Assumptions |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|
| 1 | Microcontroller development board (U1) | ESP32-S3 SuperMini; ESP32-S3 dual-core Xtensa LX7, 240 MHz, Wi-Fi 2.4 GHz + Bluetooth LE, native USB-C; 18 edge header pins (5V, GND, 3V3, TX, RX, GPIO1 to GPIO13) | 1 | 4.83 | 9.48 | 4.83 | 9.48 | Main controller: runs firmware, drives the TFT over SPI, reads buttons, connects to Wi-Fi for time and weather data | Low: ESP32s.com [S1]; High: eBay.ca listing [S2] | Low is US$3.43 converted at the FX rate; the source page shows "$" without a currency code, so USD is assumed. Flash/PSRAM variant (for example ESP32-S3FH4R2: 4 MB flash, 2 MB PSRAM) must match the firmware build settings; confirm the variant on your board. |
| 2 | TFT display module (U2) | 1.54-inch IPS TFT, 240 × 240, ST7789 driver, 4-wire SPI; 8-pin header GND, VCC, SCL, SDA, RES, DC, CS, BLK; on-board regulator (VCC fed from 5 V) and on-board backlight driver | 1 | 6.72 | 13.38 | 6.72 | 13.38 | Displays photos, clock, date, and weather information | Low: BuyDisplay [S3]; High: Waveshare [S4] | Low is US$4.77 and high is US$9.49, both converted at the FX rate. Equivalent modules must include an on-board regulator if VCC is wired to 5 V, as in this build. Header pin order varies by vendor; wire by silkscreen label. |
| 3 | Push button, momentary (SW1, SW2) | Normally-open momentary tactile switch, through-hole (6 × 6 mm class shown as a representative equivalent) | 2 | 0.15 | 0.35 | 0.30 | 0.70 | SW1 (GPIO2): Album / Next Photo. SW2 (GPIO3): Refresh / Clock / Weather. Active LOW with internal pull-ups | Low: Universal-Solder Canada [S5]; High: Adafruit [S6] | Multipack per-unit cost. Low: CAD 1.50 per 10-pack ÷ 10. High: US$2.50 per 10-pack ÷ 10, converted. Exact switch body size and actuator height in the build are unconfirmed. |
| 4 | Hookup wire / jumper leads | Pre-crimped 2.54 mm Dupont-style jumper wires, approx. 15 to 20 cm (female-female assumed for header-to-header wiring) | 12 | 0.04 | 0.14 | 0.48 | 1.68 | Connects U1 to U2 (W1 to W8), U1 to the buttons (W9, W10), and the button ground return (W11, 2 conductors) | Low: Universal-Solder Canada [S5b]; High: Adafruit [S7] | Multipack per-unit cost. Low: CAD 4.95 per 120-pack ÷ 120. High: US$3.95 per 40-pack ÷ 40, converted. Quantity follows the Sheet 2 wiring table: 10 point-to-point wires + 2 ground-return conductors. Wire type (jumper vs. stranded/solid hookup wire) and the GND splice method are to be confirmed against the build. |
| | **Subtotal, confirmed components** | | | | | **12.33** | **25.24** | | | |

---

## 2. Conditional Entries (Pending Physical Confirmation)

These entries are **not** confirmed as part of the completed device. They are listed so the cost range is complete if they turn out to be used. Remove or confirm each one after checking the physical build.

| Item | Component | Specification / Model | Qty | Est. Unit Cost Low (CAD) | Est. Unit Cost High (CAD) | Est. Ext. Cost Low (CAD) | Est. Ext. Cost High (CAD) | Function in System | Representative Supplier / Source | Notes / Assumptions |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|
| C1 | USB cable | USB-C, USB 2.0, data-capable, approx. 1 m | 1 | 1.41 | 12.00 | 1.41 | 12.00 | Supplies 5 V VBUS to U1; also used for firmware upload | Low: Universal-Solder Canada [S8] | **Conditional.** Low is researched. **High is a preliminary estimate** for a retail-store cable and is not tied to a specific listing. Often already owned; set qty to 0 if an existing cable is used. |
| C2 | Perfboard / prototyping PCB | Single-sided perfboard, 2.54 mm pitch, 50 × 70 mm (low); double-sided 90 × 150 mm (high) | 1 | 0.46 | 3.39 | 0.46 | 3.39 | Mechanical mounting for the buttons and/or a ground splice point | Universal-Solder Canada [S8b] | **Conditional.** Low: CAD 2.29 per 5-pack ÷ 5. High: one larger double-sided board. Only applies if the build uses a perfboard rather than direct wiring. |
| C3 | Enclosure material | Fabrication method unconfirmed; estimate assumes a 3D-printed PLA enclosure, approx. 40 to 80 g of filament | 1 | 1.00 | 2.80 | 1.00 | 2.80 | Houses and protects U1, U2, and the buttons | None (preliminary) | **Conditional. Preliminary estimate, not researched.** Material cost only, based on approx. CAD 25 to 35 per kg of PLA. Excludes printer time, failed prints, and design time. Replace with actual method and material once confirmed. |
| C4 | Mounting hardware | Small machine screws (for example M2 or M2.5) or equivalent fasteners; size and count unconfirmed | 4 | 0.05 | 0.30 | 0.20 | 1.20 | Fastens the display, board, or enclosure parts | None (preliminary) | **Conditional. Preliminary estimate, not researched.** Per-unit multipack cost. Quantity of 4 is a placeholder. |
| C5 | Fabrication service | Third-party 3D printing, laser cutting, or PCB fabrication | 0 | n/a | n/a | 0.00 | 0.00 | n/a | n/a | **Conditional. Not estimated.** No evidence of an outsourced fabrication service. Request a quote and add it here only if one was used. |
| | **Subtotal, conditional entries** | | | | | **3.07** | **19.39** | | | |

---

## 3. Consumables (Reported Separately)

Consumables are used during assembly but are not counted in the component subtotals.

| Item | Component | Specification / Model | Qty | Est. Unit Cost Low (CAD) | Est. Unit Cost High (CAD) | Est. Ext. Cost Low (CAD) | Est. Ext. Cost High (CAD) | Function in System | Representative Supplier / Source | Notes / Assumptions |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|
| S1 | Solder | Rosin-core electronics solder (lead-free or 63/37), approx. 1 to 2 g used | 1 lot | 0.10 | 0.50 | 0.10 | 0.50 | Joins header pins, wires, and button leads | None (preliminary) | **Conditional. Preliminary estimate, not researched.** Used amount is a small fraction of a spool. Only applies if the build involved soldering. |

---

## 4. Cost Summary

| Category | Est. Low (CAD) | Est. High (CAD) |
|---|---:|---:|
| A. Confirmed components (Items 1 to 4) | 12.33 | 25.24 |
| B. Conditional entries (Items C1 to C5) | 3.07 | 19.39 |
| **Estimated component cost, confirmed only (A)** | **12.33** | **25.24** |
| **Estimated total build cost, confirmed + conditional (A + B)** | **15.40** | **44.63** |
| C. Consumables, reported separately (S1) | 0.10 | 0.50 |
| D. Excluded: shipping, import duties, taxes (GST/PST), tools, equipment, labour, development time | Not included | Not included |

**Estimated low-end component cost:** CAD 12.33
**Estimated high-end component cost:** CAD 25.24
**Estimated total build cost range:** CAD 15.40 to 44.63 (confirmed + conditional, components and materials only)

---

## 5. Assumptions and Uncertainty

1. **Replacement cost, not purchase cost.** All values are estimated costs to buy equivalent parts today. They are not reconstructions of the author's actual purchases.
2. **Multipack pricing.** Parts normally sold in packs (buttons, jumper wires, perfboard, screws) are costed as the pack price divided by the units per pack. No item carries the full pack price.
3. **Low vs. high range.** Low values come from a low-cost online supplier or marketplace. High values come from an established branded supplier or retail-style pricing. The range covers equivalent parts, not identical parts.
4. **Currency conversion.** USD prices were converted at 1 USD = 1.4095 CAD, a mid-market rate on 2026-10-19. Card and bank rates are typically 2 to 3% higher. Converted values are rounded to the nearest cent at unit level, and extended costs are calculated from the rounded unit costs.
5. **ESP32-S3 SuperMini low price.** The source shows a "$" price with no currency code; USD is assumed. If the price is actually in CAD, the low unit cost would be CAD 3.43.
6. **Display power.** The wiring diagrams show TFT VCC connected to 5 V. Compatibility depends on the specific ST7789 breakout module having an appropriate onboard regulator. The physical module should be verified before this connection is recommended for reproduction.
7. **Header pins.** SuperMini boards and TFT modules are commonly sold with loose pin headers included. They are treated as included with Items 1 and 2 and are not costed separately.
8. **Wire quantity.** Twelve conductors follow the Sheet 2 wiring table (W1 to W11, with W11 split to both buttons). If wiring is daisy-chained or soldered with bulk hookup wire, the quantity and cost change slightly.
9. **Preliminary estimates.** Items C3, C4, S1, and the C1 high value are preliminary and not tied to a researched listing. They are labelled as such and should be replaced with sourced prices once the build details are confirmed.
10. **Price volatility.** Marketplace prices change frequently, and some referenced listings were out of stock on the access date. Re-check sources before relying on these figures.
11. **No manufacturer part numbers.** Generic modules are sold under many vendor names without stable manufacturer part numbers. Parts are identified by function and specification only.

---

## 6. Price Sources

All sources accessed 2026-10-19.

| Ref | Item(s) | Source | Price Observed | Notes |
|---|---|---|---|---|
| S1 | 1 (low) | [ESP32s.com: ESP32-S3 SuperMini Development Board](https://esp32s.com/product/esp32-s3-supermini-development-board/) | $3.43 (currency not stated; USD assumed) | Single unit |
| S2 | 1 (high) | [eBay.ca: ESP32-S3 SuperMini development board](https://www.ebay.ca/itm/145738336288) | C$9.48 | Single unit, marketplace listing |
| S3 | 2 (low) | [BuyDisplay: 1.54" TFT IPS LCD Module 240×240 SPI](https://www.buydisplay.com/ips-1-54-inch-tft-lcd-display-module-240x240-spi-for-arduino-raspberry-pi) | US$4.77 | Single unit |
| S4 | 2 (high) | [Waveshare: 1.54inch LCD Module](https://www.waveshare.com/1.54inch-lcd-module.htm) | US$9.49 | ST7789, 240×240, 3.3 V / 5 V |
| S5 | 3 (low) | [Universal-Solder Canada: 10 × Tactile Push Button 6×6 mm](https://universal-solder.ca/product/tactile-push-button-6x6mm-short-thd/) | CAD 1.50 per 10-pack | Through-hole |
| S5b | 4 (low) | [Universal-Solder Canada: 120 pcs DuPont Jumper Wires 20 cm](https://universal-solder.ca/product/120-pcs-dupont-breadboard-jumper-wires-20cm-40xmm-40xmf-40xff/) | CAD 4.95 per 120-pack | 40 MM, 40 MF, 40 FF |
| S6 | 3 (high) | [Adafruit: Tactile Switch Buttons, 10 pack (ID 1490)](https://www.adafruit.com/products/1490) | US$2.50 per 10-pack | Out of stock on access date |
| S7 | 4 (high) | [Adafruit: Premium Female/Female Jumper Wires, 40 × 6" (ID 266)](https://www.adafruit.com/product/266) | US$3.95 per 40-pack | Out of stock on access date |
| S8 | C1 (low) | [Universal-Solder Canada: product search, USB Type-C cable](https://universal-solder.ca/?s=usb+type-c+cable&post_type=product) | $1.41 (CAD store) | USB Type-C 2.0, 90 cm |
| S8b | C2 | [Universal-Solder Canada: product search, prototype PCB](https://universal-solder.ca/?s=prototype+pcb&post_type=product) | C$2.29 per 5-pack (50 × 70 mm); C$3.39 (90 × 150 mm) | |
| S9 | FX | [CurrencyConvert.online: USD to CAD](https://currencyconvert.online/usd/cad) | 1 USD = 1.4095 CAD | Mid-market, 2026-10-19 17:00 UTC |

---

> **Cost Disclaimer:** The prices shown are estimated replacement costs in Canadian dollars and do not represent the author's actual historical expenditure. Components were accumulated through multipack purchases and discounted orders over time. Estimates exclude labour, development time, equipment, shipping, and taxes unless otherwise indicated.
