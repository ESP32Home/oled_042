# ESP32-C3 Serial → OLED bridge

PlatformIO + Arduino example that listens on the USB serial console and prints each received line (up to the first 10 characters) on the built-in 0.42" SSD1306 OLED.

## Behavior

- Send text over serial (115200 baud). Once a newline is received, the first 10 characters of that line (or fewer, if shorter) are shown on the OLED using the largest readable font.
- The sketch echoes back *exactly* the text that was displayed so the host PC can confirm what appeared.
- The text stays on screen until the next line arrives.

## Build / flash

From this folder:

```bash
pio run
pio run -t upload
pio device monitor
```

Adjust the board type or pins in `platformio.ini` / `src/main.cpp` if needed for your hardware variant.

