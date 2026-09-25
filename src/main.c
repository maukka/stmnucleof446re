#include <stddef.h>
#include "stm32f446xx.h"
#include "../include/dma_driver.h"
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
	init_dma_driver();
    // Taustavalo PA8 HIGH
    GPIOA->BSRR = (1U << 8);

    ili9341_reset();
	ili9341_init_display();

	uint8_t s_win = ili9341_set_address_window(0, 0, 239, 319);   // koko rivin leveys
	uint8_t s_dma = ili9341_write_color_dma(0xF800, 240UL * 320UL);
	//ili9341_set_address_window(0, 0, 239, 319);   // koko ruutu
	//ili9341_write_color_dma(0x0000, 240UL * 320UL); 
	//ili9341_write_color(0x0000, 240UL * 320UL);   // tyhjennä mustaksi

	ili9341_draw_string(105, 148, "Hello", 0xFFFF, 0x0000);   // valkoinen teksti, musta tausta
	ili9341_draw_string(105, 160, "World", 0xFFFF, 0x0000);

    while (1) {

    }
}