#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "font8x8_basic.h"

static const char *TAG = "oled";

// Board/display defaults for the common ESP32-C3 0.42" OLED boards:
// - SSD1306-compatible controller
// - 72x40 pixels, with x offset 28
// - I2C on GPIO5 (SDA) / GPIO6 (SCL)

static const i2c_port_t OLED_I2C_PORT = (i2c_port_t)CONFIG_OLED_I2C_PORT;
static const uint8_t OLED_I2C_ADDR = (uint8_t)CONFIG_OLED_I2C_ADDR; // 7-bit
static const gpio_num_t OLED_I2C_SDA = (gpio_num_t)CONFIG_OLED_I2C_SDA;
static const gpio_num_t OLED_I2C_SCL = (gpio_num_t)CONFIG_OLED_I2C_SCL;

enum {
    OLED_WIDTH = 72,
    OLED_HEIGHT = 40,
    OLED_X_OFFSET = 28,
    OLED_PAGES = (OLED_HEIGHT + 7) / 8,
};

static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];

static esp_err_t oled_i2c_write(uint8_t control_byte, const uint8_t *data, size_t len)
{
    if (len == 0) {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, control_byte, true);
    i2c_master_write(cmd, (uint8_t *)data, len, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(OLED_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t oled_cmd(uint8_t cmd)
{
    return oled_i2c_write(0x00, &cmd, 1);
}

static esp_err_t oled_cmds(const uint8_t *cmds, size_t len)
{
    return oled_i2c_write(0x00, cmds, len);
}

static esp_err_t oled_data(const uint8_t *data, size_t len)
{
    return oled_i2c_write(0x40, data, len);
}

static esp_err_t oled_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_I2C_SDA,
        .scl_io_num = OLED_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(OLED_I2C_PORT, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(OLED_I2C_PORT, conf.mode, 0, 0, 0);
}

static void oled_i2c_scan(void)
{
    ESP_LOGI(TAG, "I2C scan: port=%d SDA=%d SCL=%d", (int)OLED_I2C_PORT, (int)OLED_I2C_SDA, (int)OLED_I2C_SCL);
    int found = 0;
    for (int addr = 0x03; addr < 0x78; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(OLED_I2C_PORT, cmd, pdMS_TO_TICKS(30));
        i2c_cmd_link_delete(cmd);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at 0x%02X", addr);
            found++;
        }
    }

    if (!found) {
        ESP_LOGW(TAG, "No I2C devices found (check wiring/pins/pullups).");
    }
}

static esp_err_t oled_init(void)
{
    const uint8_t init_seq[] = {
        0xAE, // display off
        0xD5, 0x80, // clock divide ratio / oscillator frequency
        0xA8, 0x27, // multiplex ratio: 1/40 duty
        0xD3, 0x00, // display offset
        0xAD, 0x30, // internal IREF setting (0.42" OLED)
        0x8D, 0x14, // charge pump enable
        0x40, // set display start line
        0xA6, // normal display (not inverted)
        0xA4, // output follows RAM contents
        0x20, 0x00, // horizontal addressing mode
        0xA1, // segment remap
        0xC8, // COM output scan direction
        0xDA, 0x12, // COM pins hardware configuration
        0x81, 0xAF, // contrast
        0xD9, 0x22, // pre-charge period
        0xDB, 0x20, // VCOMH deselect level
        0x2E, // deactivate scroll
        0xAF, // display on
    };

    return oled_cmds(init_seq, sizeof(init_seq));
}

static esp_err_t oled_flush(void)
{
    for (int page = 0; page < OLED_PAGES; page++) {
        const uint8_t set_pos[] = {
            (uint8_t)(0x10 | ((OLED_X_OFFSET >> 4) & 0x0F)), // set high column nibble
            (uint8_t)(0x00 | (OLED_X_OFFSET & 0x0F)), // set low column nibble
            (uint8_t)(0xB0 | (page & 0x0F)), // set page address
        };
        esp_err_t err = oled_cmds(set_pos, sizeof(set_pos));
        if (err != ESP_OK) {
            return err;
        }
        err = oled_data(&oled_buffer[page * OLED_WIDTH], OLED_WIDTH);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

static void oled_draw_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }

    int page = y / 8;
    uint8_t bit = (uint8_t)(1u << (y & 7));
    size_t index = (size_t)page * OLED_WIDTH + (size_t)x;
    if (on) {
        oled_buffer[index] |= bit;
    } else {
        oled_buffer[index] &= (uint8_t)~bit;
    }
}

static void oled_draw_char8x8(int x, int y, char c)
{
    const uint8_t *glyph = font8x8_basic[(uint8_t)c];
    for (int row = 0; row < 8; row++) {
        uint8_t row_bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            bool on = ((row_bits >> col) & 0x01) != 0;
            oled_draw_pixel(x + col, y + row, on);
        }
    }
}

static void oled_draw_string(int x, int y, const char *s)
{
    int cursor_x = x;
    for (const char *p = s; *p != '\0'; p++) {
        if (cursor_x > OLED_WIDTH - 8) {
            break;
        }
        oled_draw_char8x8(cursor_x, y, *p);
        cursor_x += 8;
    }
}

static void oled_draw_string_centered(int y, const char *s)
{
    int text_width = (int)strlen(s) * 8;
    int x = (OLED_WIDTH - text_width) / 2;
    if (x < 0) {
        x = 0;
    }
    oled_draw_string(x, y, s);
}

void app_main(void)
{
    ESP_LOGI(TAG, "SSD1306 72x40 OLED hello world");

    ESP_ERROR_CHECK(oled_i2c_init());
    oled_i2c_scan();

    esp_err_t err = oled_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED init failed at I2C address 0x%02X: %s", OLED_I2C_ADDR, esp_err_to_name(err));
        return;
    }

    memset(oled_buffer, 0, sizeof(oled_buffer));
    oled_draw_string_centered(12, "Hello");
    oled_draw_string_centered(24, "World");

    ESP_ERROR_CHECK(oled_flush());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
