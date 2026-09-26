#include "../include/button_driver.h"
#include "../include/button_logic.h"

static button_logic_t button_logic;

void button_init(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	button_logic_init(&button_logic);

	// PC13 input mode (00)
	GPIOC->MODER &= ~(3U << (13 * 2));
	GPIOC->PUPDR &= ~(3U << (13 * 2));
}

/**
 * Raw, instantaneous button state. Active-low: returns true when pressed.
 * No debouncing -- may bounce/glitch right at the moment of press/release.
 */
static bool button_is_pressed(void){

	return !(GPIOC->IDR & (1U << 13));
}

/**
 * Edge-triggered press detection. Call this repeatedly from the main loop.
 * This reports one event per sampled press edge; it does not debounce.
 */
bool button_was_pressed(void){

	return button_logic_was_pressed(&button_logic, button_is_pressed());
}

void button_update(uint32_t elapsed_ms){
	button_logic_update(&button_logic, button_is_pressed(), elapsed_ms);
}

bool button_get_short_press(void){
	return button_logic_take_short_press(&button_logic);
}

bool button_get_long_press(void){
	return button_logic_take_long_press(&button_logic);
}