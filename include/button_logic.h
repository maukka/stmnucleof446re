#ifndef BUTTON_LOGIC_H
#define BUTTON_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t held_ms;
	bool was_pressed;
	bool edge_was_pressed;
	bool short_press_pending;
	bool long_press_pending;
} button_logic_t;

void button_logic_init(button_logic_t *logic);
bool button_logic_was_pressed(button_logic_t *logic, bool pressed);
void button_logic_update(button_logic_t *logic, bool pressed, uint32_t elapsed_ms);
bool button_logic_take_short_press(button_logic_t *logic);
bool button_logic_take_long_press(button_logic_t *logic);

#endif
