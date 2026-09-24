#include <stddef.h>
#include "../include/ili9341_driver.h"
#include "../include/spi_driver.h"
#include "../include/font5x7.h"

#define FONT_WIDTH  5
#define FONT_HEIGHT 7

/**
 * This function defines in which rectagular position we are wrtiting the data in GRAM that is
 * 240x320 pixel array in ili9341 dusplay. This is done by sending two commands:
 * 1. 0x2A which is column adderss (start x and end x)
 * 2. 0x2B which is page address (start y and end y)
 * Each command takes 4 parameters two 16 bits value which are split to high and low bytes (8 bits) because
 * SPI bus sends only 8 bits at the time.
 * @param uint16_t start of x column position
 * @param uint16_t start of y page position
 * @param uint16_t end of x column position
 * @param uint16_t end of y page position 
 */
uint8_t ili9341_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){

	uint8_t status = 0;
	// Example x0 = 0x00F0: This masks value to 0x00 to  high_start_x and 
	// value F0 for low_start_x
	uint8_t high_start_x = (x0 >> 8) & 0xFF;
	uint8_t low_start_x = x0 & 0xFF;

	// Same for end values
	uint8_t high_end_x = (x1 >> 8) & 0xFF;
	uint8_t low_end_x = x1 & 0xFF;

	// All four values are passed to SPI
	uint8_t columns[] = {high_start_x, low_start_x, high_end_x, low_end_x};
	
	// Write the colum command to display with given column values
	status |= ili9341_write_command(0x2A, columns, 4);


	//Same for pages 
	uint8_t high_start_y = (y0 >> 8) & 0xFF;
	uint8_t low_start_y = y0 & 0xFF;
	uint8_t high_end_y = (y1 >> 8) & 0xFF;
	uint8_t low_end_y = y1 & 0xFF;

	uint8_t pages[] = {high_start_y, low_start_y, high_end_y, low_end_y};

	// Write pages
	status |= ili9341_write_command(0x2B, pages, 4);

	return status;
}

/**
 * Writes to display. First writes the command, then the actual data.
 * @param uint8_t the command i.e. 0x2B pages.
 * @param const uint8_t* pointer to actual data we want to write
 * @param uint16_t the size of the data packet.
 */
uint8_t ili9341_write_command(uint8_t cmd, const uint8_t *params, uint16_t param_count){

	uint8_t status = 0;
	cs_low();
	dc_command();
	status |= spi_tx_raw(&cmd, 1);
	if( param_count > 0){
		dc_data();
		status |= spi_tx_raw(params, param_count);
	}

	cs_high();

	return status;
}

/**
 * 
 */
uint8_t ili9341_read(uint8_t cmd, uint8_t *out_buf, uint16_t read_count){

	uint8_t status = 0;
	cs_low();
	dc_command();
	status |= spi_tx_raw(&cmd, 1);
	dc_data();
	status |= spi_rx_raw(out_buf, read_count);
	cs_high();

	return status;
}

/**
 * Starts a Memory Write operation (0x2C) and streams an array of RGB565
 * pixel colors to the display. Keeps CS low for the entire command+data
 * transaction. The address window must be set beforehand with
 * ili9341_set_address_window() so the display knows where this data lands.
 *
 * @param colors pointer to array of 16-bit RGB565 color values
 * @param count number of pixels to write
 */
uint8_t ili9341_write_pixels(const uint16_t *colors, uint32_t count){

	if (colors == NULL && count > 0){
		return -1;
	}

	uint8_t status = 0;
	uint8_t cmd = 0x2C;   // Memory Write

	cs_low();

	dc_command();
	status |= spi_tx_raw(&cmd, 1);

	dc_data();

	for (uint32_t i = 0; i < count; i++){
		uint8_t pixel_bytes[2] = {
			// First index contains high bytes of 16 bit data second index the low bytes
			//So this splits the 16 bit data to 8 bit chuncks for SPI
			(uint8_t)((colors[i] >> 8) & 0xFF),
			(uint8_t)(colors[i] & 0xFF)
		};
		status |= spi_tx_raw(pixel_bytes, 2);
	}

	cs_high();

	return status;
}

/**
 * Starts a Memory Write operation (0x2C) and writes the SAME color
 * repeatedly, "count" times. Useful for filling a rectangular area with
 * a solid color without needing to build a large array in RAM first.
 * The address window must be set beforehand with
 * ili9341_set_address_window().
 *
 * @param color single RGB565 color value to repeat
 * @param count how many pixels to fill with this color
 */
uint8_t ili9341_write_color(uint16_t color, uint32_t count){

	uint8_t status = 0;
	uint8_t cmd = 0x2C;   // Memory Write

	uint8_t pixel_bytes[2] = {
		(uint8_t)((color >> 8) & 0xFF),
		(uint8_t)(color & 0xFF)
	};

	cs_low();

	dc_command();
	status |= spi_tx_raw(&cmd, 1);

	dc_data();

	for (uint32_t i = 0; i < count; i++){
		status |= spi_tx_raw(pixel_bytes, 2);
	}

	cs_high();

	return status;
}

/**
 * Draws a single character at pixel position (x,y) using the 5x7 font.
 * Builds a small pixel buffer for the character's bounding box and writes
 * it in one Memory Write transaction via ili9341_write_pixels.
 */
void ili9341_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg_color, uint16_t bg_color){

	if (c < 0x20 || c > 0x7E){
		c = '?';
	}

	uint16_t pixel_buf[FONT_WIDTH * FONT_HEIGHT];
	uint8_t font_index = c - 0x20;

	for (uint8_t col = 0; col < FONT_WIDTH; col++){
		// 1D-indeksointi: merkin alkukohta + sarake
		uint8_t column_bits = font5x7[6 + font_index * FONT_WIDTH + col];

		for (uint8_t row = 0; row < FONT_HEIGHT; row++){
			uint16_t color = (column_bits & (1U << row)) ? fg_color : bg_color;
			pixel_buf[row * FONT_WIDTH + col] = color;
		}
	}

	ili9341_set_address_window(x, y, x + FONT_WIDTH - 1, y + FONT_HEIGHT - 1);
	ili9341_write_pixels(pixel_buf, FONT_WIDTH * FONT_HEIGHT);
}

/**
 * Draws a null-terminated string starting at (x,y), advancing x by
 * FONT_WIDTH+1 pixels (5 pixels glyph + 1 pixel spacing) per character.
 */
void ili9341_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg_color, uint16_t bg_color){

	if (str == NULL){
		return;
	}

	uint16_t cursor_x = x;

	while (*str != '\0'){
		ili9341_draw_char(cursor_x, y, *str, fg_color, bg_color);
		cursor_x += (FONT_WIDTH + 1);
		str++;
	}
}