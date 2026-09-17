#include "../include/on_board_led.h"

void init_led(){

	// 1. Switch clock on for GPIOA-port (RCC AHB1 peripheral clock enable register)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 2. Set PA5-pin for initial state (General purpose output mode: '01')
    GPIOA->MODER &= ~(3U << (5 * 2)); // Clear bits 10 and 11
    GPIOA->MODER |=  (1U << (5 * 2)); // Set bit 10 to value 1
}
void toggle_led(){
	// toggle led on and off
	GPIOA->ODR ^= GPIO_ODR_OD5;
}