#include <stddef.h>
#include "../include/ili9341_driver.h"
#include "../include/spi_driver.h"
#include "../include/font5x7.h"

#define FONT_WIDTH  5
#define FONT_HEIGHT 7
// Buffer size for DMA
#define DMA_CHUNK_PIXELS 2048

/**
 * Writes the needed configuration for display.
 */
void ili9341_init_display(){
	uint8_t p;

	p = 0x23; 
	ili9341_write_command(0xC0, &p, 1);           // Power Control 1

	p = 0x10; 
	ili9341_write_command(0xC1, &p, 1);           // Power Control 2

	uint8_t vcom[2] = {0x3E, 0x28}; 
	ili9341_write_command(0xC5, vcom, 2);  // VCOM Control 1

	p = 0x86; 
	ili9341_write_command(0xC7, &p, 1);            // VCOM Control 2

	p = 0x48; 
	ili9341_write_command(0x36, &p, 1);            // MADCTL

	p = 0x55; 
	ili9341_write_command(0x3A, &p, 1);            // Pixel Format RGB565

	uint8_t frmctr[2] = {0x00, 0x18}; 
	ili9341_write_command(0xB1, frmctr, 2); // Frame Rate

	uint8_t dfunc[3] = {0x08, 0x82, 0x27}; 
	ili9341_write_command(0xB6, dfunc, 3); // Display Function Control

	p = 0x11; 
	ili9341_write_command(0xF2, &p, 1);            // 3Gamma disable

	p = 0x01; 
	ili9341_write_command(0x26, &p, 1);            // Gamma curve

	ili9341_write_command(0x11, NULL, 0);   // Sleep Out
	delay_ms(120);

	ili9341_write_command(0x29, NULL, 0);   // Display On
	delay_ms(20);
}
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

/**
 * Starts a Memory Write operation (0x2C) and streams an array of RGB565
 * pixel colors to the display. Keeps CS low for the entire command+data
 * transaction. The address window must be set beforehand with
 * ili9341_set_address_window() so the display knows where this data lands.
 *
 * @param colors pointer to array of 16-bit RGB565 color values
 * @param count number of pixels to write
 */
uint8_t ili9341_write_pixels_dma(const uint16_t *colors, uint32_t count){

	if (colors == NULL && count > 0){
		return -1;
	}

	uint8_t status = 0;
	uint8_t cmd = 0x2C;
	static uint8_t chunk_buf[DMA_CHUNK_PIXELS * 2];   // static: pois pinosta, säästää stack-tilaa

	cs_low();

	dc_command();
	status |= spi_tx_raw(&cmd, 1);

	dc_data();

	uint32_t remaining = count;
	uint32_t offset = 0;

	while (remaining > 0 && status == 0){

		uint32_t this_chunk = (remaining > DMA_CHUNK_PIXELS) ? DMA_CHUNK_PIXELS : remaining;

		// CPU tekee byte-swapin tähän pieneen puskuriin
		for (uint32_t i = 0; i < this_chunk; i++){
			uint16_t c = colors[offset + i];
			chunk_buf[i * 2]     = (uint8_t)((c >> 8) & 0xFF);
			chunk_buf[i * 2 + 1] = (uint8_t)(c & 0xFF);
		}

		// DMA lähettää tämän palan
		status |= spi_tx_dma(chunk_buf, (uint16_t)(this_chunk * 2));

		offset += this_chunk;
		remaining -= this_chunk;
	}

	cs_high();

	return status;
}

uint8_t ili9341_write_color_dma(uint16_t color, uint32_t count){

	uint8_t status = 0;
	uint8_t cmd = 0x2C;
	static uint8_t chunk_buf[DMA_CHUNK_PIXELS * 2];

	// Täytä puskuri kerran -- väri ei muutu palojen välissä
	uint8_t high = (uint8_t)((color >> 8) & 0xFF);
	uint8_t low  = (uint8_t)(color & 0xFF);
	for (uint32_t i = 0; i < DMA_CHUNK_PIXELS; i++){
		chunk_buf[i * 2]     = high;
		chunk_buf[i * 2 + 1] = low;
	}

	cs_low();
	dc_command();
	status |= spi_tx_raw(&cmd, 1);
	dc_data();

	uint32_t remaining = count;
	while (remaining > 0 && status == 0){
		uint32_t this_chunk = (remaining > DMA_CHUNK_PIXELS) ? DMA_CHUNK_PIXELS : remaining;
		status |= spi_tx_dma(chunk_buf, (uint16_t)(this_chunk * 2));
		remaining -= this_chunk;
	}

	cs_high();
	return status;
}

/**
 * Sets display rotation.
 * @param uint8_t can have next values:
 * - 0 portrait
 * - 1 landscape
 * - 2 Inverted portrait
 * - 3 Inverted landscape
 */
void ili9341_set_rotation(uint8_t rotation) {
    uint8_t madctl = 0;

    switch (rotation & 0x03) {
        case 0: // Portrait
            madctl = MADCTL_MX | MADCTL_BGR;
            break;
        case 1: // Landscape
            madctl = MADCTL_MV | MADCTL_BGR;
            break;
        case 2: // Inverted Portrait
            madctl = MADCTL_MY | MADCTL_BGR;
            break;
        case 3: // Inverted Landscape
            madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            break;
    }

    ili9341_write_command(0x36, &madctl, 1);
}

/**
 * Helper function to draw one pixel at the time with given color on the display.
 * @param uint16_t x coordinate
 * @param uint16_t y coordinate
 * @param uint16_t color
 */
uint8_t ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color){

	uint8_t status = 0;

	status |= ili9341_set_address_window(x, y, x, y);
	status |= ili9341_write_color(color, 1);

	return status;
}

/**
 * Draws a horizontal line, optimized as a single filled rectangle
 * of height 1 instead of looping pixel by pixel.
 */
uint8_t ili9341_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color){

	uint8_t status = 0;

	status |= ili9341_set_address_window(x, y, x + w - 1, y);
	status |= ili9341_write_color_dma(color, w);

	return status;
}

/**
 * Draws a vertical line, optimized as a single filled rectangle
 * of width 1 instead of looping pixel by pixel.
 */
uint8_t ili9341_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color){

	uint8_t status = 0;

	status |= ili9341_set_address_window(x, y, x, y + h - 1);
	status |= ili9341_write_color_dma(color, h);

	return status;
}

/**
 * Draws a rectangle outline (border only, no fill) using four line calls.
 */
uint8_t ili9341_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){

	uint8_t status = 0;

	status |= ili9341_draw_hline(x, y, w, color);                  // top
	status |= ili9341_draw_hline(x, y + h - 1, w, color);          // bottom
	status |= ili9341_draw_vline(x, y, h, color);                  // left
	status |= ili9341_draw_vline(x + w - 1, y, h, color);          // right

	return status;
}

/**
 * Draws an arbitrary diagonal line using Bresenham's line algorithm.
 * Falls back to the optimized hline/vline for perfectly horizontal
 * or vertical lines, since those are far faster than pixel-by-pixel.
 */
uint8_t ili9341_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color){

	uint8_t status = 0;

	// Fast paths for perfectly horizontal/vertical lines
	if (y0 == y1){
		uint16_t x_start = (x0 < x1) ? x0 : x1;
		uint16_t w = (x0 < x1) ? (x1 - x0 + 1) : (x0 - x1 + 1);
		return ili9341_draw_hline(x_start, y0, w, color);
	}
	if (x0 == x1){
		uint16_t y_start = (y0 < y1) ? y0 : y1;
		uint16_t h = (y0 < y1) ? (y1 - y0 + 1) : (y0 - y1 + 1);
		return ili9341_draw_vline(x0, y_start, h, color);
	}

	// Bresenham's algorithm for diagonal lines
	int16_t dx = (int16_t)((x1 > x0) ? (x1 - x0) : (x0 - x1));
	int16_t dy = (int16_t)((y1 > y0) ? (y1 - y0) : (y0 - y1));
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t err = dx - dy;

	int16_t cx = (int16_t)x0;
	int16_t cy = (int16_t)y0;

	while (1){
		status |= ili9341_draw_pixel((uint16_t)cx, (uint16_t)cy, color);

		if (cx == (int16_t)x1 && cy == (int16_t)y1){
			break;
		}

		int16_t e2 = 2 * err;
		if (e2 > -dy){
			err -= dy;
			cx += sx;
		}
		if (e2 < dx){
			err += dx;
			cy += sy;
		}
	}

	return status;
}

/**
 * Fills a rectangular area with a solid color. Uses DMA since this can
 * cover a large number of pixels (e.g. the whole screen).
 */
uint8_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){

	uint8_t status = 0;

	status |= ili9341_set_address_window(x, y, x + w - 1, y + h - 1);
	status |= ili9341_write_color_dma(color, (uint32_t)w * (uint32_t)h);

	return status;
}

/**
 * Fills the entire 240x320 screen with a solid color.
 * Thin wrapper around fill_rect for convenience.
 */
uint8_t ili9341_fill_screen(uint16_t color){

	return ili9341_fill_rect(0, 0, 240, 320, color);
}

/**
 * Draws a circle outline using the midpoint (Bresenham) circle algorithm.
 * Plots 8 symmetric points per step instead of computing each one directly,
 * which is what makes the algorithm efficient.
 */
uint8_t ili9341_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color){

	uint8_t status = 0;

	int16_t x = (int16_t)r;
	int16_t y = 0;
	int16_t err = 0;

	while (x >= y){
		status |= ili9341_draw_pixel(x0 + x, y0 + y, color);
		status |= ili9341_draw_pixel(x0 + y, y0 + x, color);
		status |= ili9341_draw_pixel(x0 - y, y0 + x, color);
		status |= ili9341_draw_pixel(x0 - x, y0 + y, color);
		status |= ili9341_draw_pixel(x0 - x, y0 - y, color);
		status |= ili9341_draw_pixel(x0 - y, y0 - x, color);
		status |= ili9341_draw_pixel(x0 + y, y0 - x, color);
		status |= ili9341_draw_pixel(x0 + x, y0 - y, color);

		if (err <= 0){
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0){
			x -= 1;
			err -= 2 * x + 1;
		}
	}

	return status;
}

/**
 * Draws a filled circle. Instead of plotting individual pixels for the
 * interior, draws horizontal lines (chords) between symmetric edge points
 * on each step -- same idea as draw_hline being faster than pixel-by-pixel,
 * since each chord is one set_address_window + write_color_dma call.
 */
uint8_t ili9341_fill_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color){

	uint8_t status = 0;

	int16_t x = (int16_t)r;
	int16_t y = 0;
	int16_t err = 0;

	while (x >= y){
		// Horizontal chord at y0+y, spanning from x0-x to x0+x
		status |= ili9341_draw_hline((uint16_t)(x0 - x), (uint16_t)(y0 + y), (uint16_t)(2 * x + 1), color);
		// Horizontal chord at y0-y (mirrored, skip if y==0 to avoid drawing it twice)
		if (y != 0){
			status |= ili9341_draw_hline((uint16_t)(x0 - x), (uint16_t)(y0 - y), (uint16_t)(2 * x + 1), color);
		}
		// Horizontal chord at y0+x, spanning from x0-y to x0+y
		status |= ili9341_draw_hline((uint16_t)(x0 - y), (uint16_t)(y0 + x), (uint16_t)(2 * y + 1), color);
		// Horizontal chord at y0-x (mirrored, skip if x==0)
		if (x != 0){
			status |= ili9341_draw_hline((uint16_t)(x0 - y), (uint16_t)(y0 - x), (uint16_t)(2 * y + 1), color);
		}

		if (err <= 0){
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0){
			x -= 1;
			err -= 2 * x + 1;
		}
	}

	return status;
}

/**
 * Draws a rectangle outline with rounded corners. Uses the straight-edge
 * lines for the sides and quarter-circle arcs (via draw_circle_helper)
 * for the four corners.
 */
static uint8_t ili9341_draw_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corner_mask, uint16_t color){

	uint8_t status = 0;

	int16_t x = (int16_t)r;
	int16_t y = 0;
	int16_t err = 0;

	while (x >= y){
		if (corner_mask & 0x1){   // top-right
			status |= ili9341_draw_pixel(x0 + x, y0 - y, color);
			status |= ili9341_draw_pixel(x0 + y, y0 - x, color);
		}
		if (corner_mask & 0x2){   // top-left
			status |= ili9341_draw_pixel(x0 - x, y0 - y, color);
			status |= ili9341_draw_pixel(x0 - y, y0 - x, color);
		}
		if (corner_mask & 0x4){   // bottom-left
			status |= ili9341_draw_pixel(x0 - x, y0 + y, color);
			status |= ili9341_draw_pixel(x0 - y, y0 + x, color);
		}
		if (corner_mask & 0x8){   // bottom-right
			status |= ili9341_draw_pixel(x0 + x, y0 + y, color);
			status |= ili9341_draw_pixel(x0 + y, y0 + x, color);
		}

		if (err <= 0){
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0){
			x -= 1;
			err -= 2 * x + 1;
		}
	}

	return status;
}

uint8_t ili9341_draw_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color){

	uint8_t status = 0;

	// Straight edges, shortened by the corner radius on each end
	status |= ili9341_draw_hline(x + r, y, w - 2 * r, color);                   // top
	status |= ili9341_draw_hline(x + r, y + h - 1, w - 2 * r, color);           // bottom
	status |= ili9341_draw_vline(x, y + r, h - 2 * r, color);                   // left
	status |= ili9341_draw_vline(x + w - 1, y + r, h - 2 * r, color);           // right

	// Four corner arcs
	status |= ili9341_draw_circle_helper(x + r,         y + r,         r, 0x2, color);  // top-left
	status |= ili9341_draw_circle_helper(x + w - 1 - r,  y + r,         r, 0x1, color);  // top-right
	status |= ili9341_draw_circle_helper(x + r,         y + h - 1 - r, r, 0x4, color);  // bottom-left
	status |= ili9341_draw_circle_helper(x + w - 1 - r,  y + h - 1 - r, r, 0x8, color);  // bottom-right

	return status;
}

/**
 * Filled rounded rectangle: a solid fill_rect for the middle section plus
 * two solid corner-fill helpers for the left/right rounded edges.
 */
static uint8_t ili9341_fill_circle_helper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t corner_mask, int16_t delta, uint16_t color){

	uint8_t status = 0;

	int16_t x = (int16_t)r;
	int16_t y = 0;
	int16_t err = 0;

	while (x >= y){
		if (corner_mask & 0x1){   // right half
			status |= ili9341_draw_vline((uint16_t)(x0 + x), (uint16_t)(y0 - y), (uint16_t)(2 * y + 1 + delta), color);
			status |= ili9341_draw_vline((uint16_t)(x0 + y), (uint16_t)(y0 - x), (uint16_t)(2 * x + 1 + delta), color);
		}
		if (corner_mask & 0x2){   // left half
			status |= ili9341_draw_vline((uint16_t)(x0 - x), (uint16_t)(y0 - y), (uint16_t)(2 * y + 1 + delta), color);
			status |= ili9341_draw_vline((uint16_t)(x0 - y), (uint16_t)(y0 - x), (uint16_t)(2 * x + 1 + delta), color);
		}

		if (err <= 0){
			y += 1;
			err += 2 * y + 1;
		}
		if (err > 0){
			x -= 1;
			err -= 2 * x + 1;
		}
	}

	return status;
}

uint8_t ili9341_fill_round_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color){

	uint8_t status = 0;

	// Center block, full width, height minus the two rounded caps
	status |= ili9341_fill_rect(x + r, y, w - 2 * r, h, color);

	// Left and right rounded caps
	status |= ili9341_fill_circle_helper((uint16_t)(x + w - 1 - r), (uint16_t)(y + r), r, 0x1, (int16_t)(h - 2 * r - 1), color);
	status |= ili9341_fill_circle_helper((uint16_t)(x + r),         (uint16_t)(y + r), r, 0x2, (int16_t)(h - 2 * r - 1), color);

	return status;
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}