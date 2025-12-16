# ESP32-C3 0.42" OLED (SSD1306) Hello World (PlatformIO)

PlatformIO + Arduino example for the common ESP32-C3 "0.42 OLED" boards (often sold as 01Space/diymore ESP32‑C3 SuperMini OLED).

## Hardware defaults

- OLED controller: SSD1306 compatible
- Resolution: 72x40
- I2C pins (common default): SDA = GPIO5, SCL = GPIO6
- I2C address (common default): `0x3C`

## Build / flash

Open this folder (`02_pio_hello_oled`) in PlatformIO and use **Build** / **Upload**.

If your board is different, update `board = ...` in `platformio.ini`.

