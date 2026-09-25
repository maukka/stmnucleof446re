#ifndef ILI9342_DRIVER_H
#define ILI9342_DRIVER_H

#include <stdint.h>

uint8_t ili9341_write_command(uint8_t cmd, const uint8_t *params, uint16_t param_count);
uint8_t ili9341_read(uint8_t cmd, uint8_t *out_buf, uint16_t read_count);
uint8_t ili9341_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
uint8_t ili9341_write_color(uint16_t color, uint32_t count);
uint8_t ili9341_write_pixels(const uint16_t *colors, uint32_t count);
uint8_t ili9341_write_pixels_dma(const uint16_t *colors, uint32_t count);
uint8_t ili9341_write_color_dma(uint16_t color, uint32_t count);
void ili9341_init_display();
void ili9341_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color);
void ili9341_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg_color, uint16_t bg_color);

#endif