#include <Arduino.h>
#include <U8g2lib.h>

#define OLED_RESET U8X8_PIN_NONE
#define OLED_SDA 5
#define OLED_SCL 6

static constexpr int kPanelWidth = 72;
static constexpr int kPanelHeight = 40;
static constexpr int kPanelXOffset = 30;
static constexpr int kPanelYOffset = 12;

// Tall, narrow font so strings like "12.1V" fill the display without clipping.
static const uint8_t *kFont = u8g2_font_logisoso24_tf;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RESET, OLED_SCL, OLED_SDA);

static char line_buffer[11];
static size_t line_len = 0;

static void draw_text(const char *text)
{
  u8g2.clearBuffer();
  u8g2.setFont(kFont);

  int16_t text_width = u8g2.getStrWidth(text);
  if (text_width < 0) {
    text_width = 0;
  }
  int x = kPanelXOffset + (kPanelWidth - text_width) / 2;
  if (x < 0) {
    x = 0;
  }
  int y = kPanelYOffset + u8g2.getAscent();  // keep baseline inside 40 px window

  u8g2.drawStr(x, y, text);
  u8g2.sendBuffer();
}

static void handle_line()
{
  if (line_len == 0) {
    return;
  }
  line_buffer[line_len] = '\0';
  draw_text(line_buffer);
  Serial.println(line_buffer);  // Echo exactly what was displayed.

  line_len = 0;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.setBusClock(400000);

  draw_text("READY");
  Serial.println("READY");
}

void loop()
{
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      handle_line();
      continue;
    }
    if (line_len < sizeof(line_buffer) - 1) {
      line_buffer[line_len++] = c;
    }
  }
}
