#ifndef ILI9342_DRIVER_H
#define ILI9342_DRIVER_H

#include <stdint.h>

// Display rotation values
#define MADCTL_MY  0x80  // Row Address Order (Y-akselin peilaus)
#define MADCTL_MX  0x40  // Column Address Order (X-akselin peilaus)
#define MADCTL_MV  0x20  // Row / Column Exchange (X ja Y vaihto, eli vaakatila)
#define MADCTL_ML  0x10  // Vertical Refresh Order
#define MADCTL_BGR 0x08  // BGR Filter Panel (Käytetään useimmissa näytöissä RGB:n sijaan)
#define MADCTL_MH  0x04  // Horizontal Refresh Order

// Common colors (RGB565)
#define COLOR_BLACK       0x0000  //   0,   0,   0
#define COLOR_NAVY        0x000F  //   0,   0, 128
#define COLOR_DARKGREEN   0x03E0  //   0, 128,   0
#define COLOR_DARKCYAN    0x03EF  //   0, 128, 128
#define COLOR_MAROON      0x7800  // 128,   0,   0
#define COLOR_PURPLE      0x780F  // 128,   0, 128
#define COLOR_OLIVE       0x7BE0  // 128, 128,   0
#define COLOR_LIGHTGREY   0xC618  // 192, 192, 192
#define COLOR_DARKGREY    0x7BEF  // 128, 128, 128
#define COLOR_BLUE        0x001F  //   0,   0, 255
#define COLOR_GREEN       0x07E0  //   0, 255,   0
#define COLOR_CYAN        0x07FF  //   0, 255, 255
#define COLOR_RED         0xF800  // 255,   0,   0
#define COLOR_MAGENTA     0xF81F  // 255,   0, 255
#define COLOR_YELLOW      0xFFE0  // 255, 255,   0
#define COLOR_WHITE       0xFFFF  // 255, 255, 255
#define COLOR_ORANGE      #FDA0  // 255, 165,   0
#define COLOR_GREENYELLOW 0xAFE5  // 173, 255,  47
#define COLOR_PINK        0xFC18  // 255, 192, 203

#define RGB565(r, g, b) (((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | ((b) >> 3))

uint8_t ili9341_write_command(uint8_t cmd, const uint8_t *params, uint16_t param_count);
uint8_t ili9341_read(uint8_t cmd, uint8_t *out_buf, uint16_t read_count);
uint8_t ili9341_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
uint8_t ili9341_write_color(uint16_t color, uint32_t count);
uint8_t ili9341_write_pixels(const uint16_t *colors, uint32_t count);
uint8_t ili9341_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
uint8_t ili9341_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
uint8_t ili9341_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
uint8_t ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
uint8_t ili9341_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
uint8_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
uint8_t ili9341_fill_screen(uint16_t color);
uint8_t ili9341_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
uint8_t ili9341_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
uint8_t ili9341_draw_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
uint8_t ili9341_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
uint8_t ili9341_write_pixels_dma(const uint16_t *colors, uint32_t count);
uint8_t ili9341_write_color_dma(uint16_t color, uint32_t count);
uint16_t color565(uint8_t r, uint8_t g, uint8_t b); 
void ili9341_init_display();
void ili9341_set_rotation(uint8_t rotation);
void ili9341_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color);
void ili9341_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg_color, uint16_t bg_color);

#endif