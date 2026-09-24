#include <stddef.h>
#include "stm32f446xx.h"
#include "../include/spi_driver.h"
#include "../include/ili9341_driver.h"

// Yksinkertainen viivefunktio (ohittaa kääntäjän optimoinnin volatile-avainsanalla)
void delay(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        // asm volatile estää kääntäjää poistamasta tyhjää silmukkaa
        asm volatile ("nop");
    }
}

int main(void) {
    systick_delay_init();
    init_spi();

    // Taustavalo PA8 HIGH
    GPIOA->BSRR = (1U << 8);

    ili9341_reset();
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

	ili9341_set_address_window(0, 0, 239, 319);   // koko ruutu
	ili9341_write_color(0x0000, 240UL * 320UL);   // tyhjennä mustaksi

	ili9341_draw_string(105, 148, "Hello", 0xFFFF, 0x0000);   // valkoinen teksti, musta tausta
	ili9341_draw_string(105, 160, "World", 0xFFFF, 0x0000);

    while (1) {

    }
}