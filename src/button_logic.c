#include "../include/button_logic.h"

#define LONG_PRESS_MS 500U

void button_logic_init(button_logic_t *logic)
{
	logic->held_ms = 0;
	logic->was_pressed = false;
	logic->edge_was_pressed = false;
	logic->short_press_pending = false;
	logic->long_press_pending = false;
}

bool button_logic_was_pressed(button_logic_t *logic, bool pressed)
{
	bool newly_pressed = pressed && !logic->edge_was_pressed;
	logic->edge_was_pressed = pressed;
	return newly_pressed;
}

void button_logic_update(button_logic_t *logic, bool pressed, uint32_t elapsed_ms)
{
	if (pressed) {
		if (logic->held_ms < LONG_PRESS_MS) {
			uint32_t remaining_ms = LONG_PRESS_MS - logic->held_ms;
			logic->held_ms += elapsed_ms < remaining_ms ? elapsed_ms : remaining_ms;
		}
		logic->was_pressed = true;
	} else if (logic->was_pressed) {
		if (logic->held_ms >= LONG_PRESS_MS) {
			logic->long_press_pending = true;
		} else {
			logic->short_press_pending = true;
		}
		logic->held_ms = 0;
		logic->was_pressed = false;
	}
}

bool button_logic_take_short_press(button_logic_t *logic)
{
	bool pending = logic->short_press_pending;
	logic->short_press_pending = false;
	return pending;
}

bool button_logic_take_long_press(button_logic_t *logic)
{
	bool pending = logic->long_press_pending;
	logic->long_press_pending = false;
	return pending;
}
