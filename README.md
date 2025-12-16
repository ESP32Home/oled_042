# ESP32-C3 0.42" OLED (SSD1306) Hello World (ESP-IDF)

Minimal ESP-IDF project that prints `Hello` / `World` on the built-in 0.42" OLED.

## Hardware defaults

This project targets the common ESP32-C3 "0.42 OLED" boards (often sold as 01Space/diymore ESP32‑C3 SuperMini OLED):

- OLED controller: SSD1306 compatible
- Resolution: 72x40
- I2C pins (common default): SDA = GPIO5, SCL = GPIO6
- I2C address (common default): `0x3C`

If your board uses different I2C pins/address, change them in `idf.py menuconfig` under `OLED`.

## Build / flash

From the project folder:

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

If nothing shows up on the display, check the monitor log: the app performs an I2C scan and logs detected device addresses.
