#include <stddef.h>
#include "stm32f446xx.h"
#include "../include/dma_driver.h"
#include "../include/spi_driver.h"
#include "../include/ili9341_driver.h"
#include "../include/main_menu.h"
#include "../include/button_driver.h"

int main(void) {
    systick_delay_init();
    init_spi();
	init_dma_driver();
	button_init();
	menu_init();
    // Taustavalo PA8 HIGH
    GPIOA->BSRR = (1U << 8);

    ili9341_reset();
	ili9341_init_display();

    while (1) {
		button_update(20);
   	 	menu_update();
    	delay_ms(20);
    }
}