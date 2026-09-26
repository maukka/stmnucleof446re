#ifndef MENU_H
#define MENU_H

#include "stm32f446xx.h"
#include <stdbool.h>

typedef struct {
	const char *label;
	void (*action)(void);
} menu_item_t;

void menu_init(void);
void menu_update(void);   // call once per main-loop iteration, after button_update()

#endif