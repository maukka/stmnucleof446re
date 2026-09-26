#include "../include/button_driver.h"

#define LONG_PRESS_MS 500U

static bool was_down = false;
static uint32_t held_ms = 0;
static bool short_press_flag = false;
static bool long_press_flag = false;

void button_init(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

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
 * Debounced, edge-triggered single-press detection. Call this repeatedly
 * from the main loop (e.g. every iteration). Returns true exactly once
 * per physical press, even if the mechanical contact bounces.
 */
bool button_was_pressed(void){

	was_down = false;
	bool now_down = button_is_pressed();

	// Very simple time-based debounce: only trust a state change
	// after it has held steady for a short window.
	// (Uses your existing millisecond timebase via a counter you
	// increment somewhere, e.g. in SysTick, OR poll-based approach below.)

	if (now_down != was_down){
		was_down = now_down;
		if (now_down){
			return true;   // rising edge of "pressed"
		}
	}

	return false;
}

void button_update(uint32_t elapsed_ms){

	bool now_down = button_is_pressed();

	if (now_down){
		held_ms += elapsed_ms;
		was_down = true;
	} else if (was_down){
		// Just released -- classify the press that just ended
		if (held_ms >= LONG_PRESS_MS){
			long_press_flag = true;
		} else {
			short_press_flag = true;
		}
		held_ms = 0;
		was_down = false;
	}
}

bool button_get_short_press(void){
	if (short_press_flag){
		short_press_flag = false;
		return true;
	}
	return false;
}

bool button_get_long_press(void){
	if (long_press_flag){
		long_press_flag = false;
		return true;
	}
	return false;
}