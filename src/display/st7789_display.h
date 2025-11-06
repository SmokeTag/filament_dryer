#ifndef ST7789_DISPLAY_H
#define ST7789_DISPLAY_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Display dimensions
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

// Pin definitions (adjust according to your wiring)
#define PIN_MISO 16  // Not used for display
#define PIN_CS   17  // Chip Select
#define PIN_SCK  18  // Clock
#define PIN_MOSI 19  // Data (SDA)
#define PIN_DC   20  // Data/Command
#define PIN_RST  21  // Reset

// SPI instance
#define SPI_PORT spi0

// Colors (RGB565 format)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ST7789 Commands
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_INVON   0x21
#define ST7789_INVOFF  0x20
#define ST7789_DISPON  0x29

// Function prototypes
void st7789_init(void);
void st7789_write_cmd(uint8_t cmd);
void st7789_write_data(uint8_t data);
void st7789_write_data16(uint16_t data);
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789_fill_color(uint16_t color);
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
void st7789_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg_color);
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

#endif // ST7789_DISPLAY_H