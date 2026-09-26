#include <stddef.h>
#include "../include/main_menu.h"
#include "../include/button_driver.h"
#include "../include/ili9341_driver.h"

#define MENU_ITEM_COUNT 3
#define MENU_ROW_HEIGHT 20
#define MENU_START_Y    100
#define MENU_START_X    60

static uint8_t current_index = 0;
static bool needs_redraw = true;

// -------- Actions --------

static void action_red(void){
	ili9341_fill_screen(0xF800);
}

static void action_green(void){
	ili9341_fill_screen(0x07E0);
}

static void action_blue(void){
	ili9341_fill_screen(0x001F);
}

static const menu_item_t menu_items[MENU_ITEM_COUNT] = {
	{"Red",   action_red},
	{"Green", action_green},
	{"Blue",  action_blue}
};

// -------- Drawing --------

static void menu_draw(void){

	ili9341_fill_screen(0x0000);   // clear to black

	for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++){

		uint16_t y = MENU_START_Y + (i * MENU_ROW_HEIGHT);

		if (i == current_index){
			// Highlight the selected row: filled background behind the text
			ili9341_fill_rect(MENU_START_X - 5, y - 2, 100, 12, 0xFFFF);
			ili9341_draw_string(MENU_START_X, y, menu_items[i].label, 0x0000, 0xFFFF);
		} else {
			ili9341_draw_string(MENU_START_X, y, menu_items[i].label, 0xFFFF, 0x0000);
		}
	}
}

// -------- Public interface --------

void menu_init(void){
	current_index = 0;
	needs_redraw = true;
}

void menu_update(void){

	if (button_get_short_press()){
		current_index = (current_index + 1) % MENU_ITEM_COUNT;
		needs_redraw = true;
	}

	if (button_get_long_press()){
		if (menu_items[current_index].action != NULL){
			menu_items[current_index].action();
		}
		needs_redraw = true;   // palaa valikkoon toiminnon jälkeen
	}

	if (needs_redraw){
		menu_draw();
		needs_redraw = false;
	}
}