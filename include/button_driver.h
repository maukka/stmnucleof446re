#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include "stm32f446xx.h"
#include <stdbool.h>

void button_init(void);
// debounced, edge-triggered "single press" -recognition
bool button_was_pressed(void);

void button_update(uint32_t elapsed_ms);
// true exactly once, on release of a short press
bool button_get_short_press(void);
bool button_get_long_press(void);

#endif