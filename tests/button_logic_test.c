#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/button_logic.h"

static button_logic_t logic;

static void expect(bool condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void expect_short_press(uint32_t duration_ms)
{
	button_logic_init(&logic);
	button_logic_update(&logic, true, duration_ms);
	button_logic_update(&logic, false, 0);

	expect(button_logic_take_short_press(&logic), "release should report a short press");
	expect(!button_logic_take_short_press(&logic), "short press event should be consumed once");
	expect(!button_logic_take_long_press(&logic), "short press should not report a long press");
}

static void test_short_press(void)
{
	expect_short_press(100);
}

static void test_long_press(void)
{
	button_logic_init(&logic);
	for (unsigned int i = 0; i < 25; ++i) {
		button_logic_update(&logic, true, 20);
	}
	button_logic_update(&logic, false, 0);

	expect(button_logic_take_long_press(&logic), "500 ms press should report a long press");
	expect(!button_logic_take_long_press(&logic), "long press event should be consumed once");
	expect(!button_logic_take_short_press(&logic), "long press should not report a short press");
}

static void test_threshold_boundary(void)
{
	expect_short_press(499);

	button_logic_init(&logic);
	button_logic_update(&logic, true, 500);
	button_logic_update(&logic, false, 0);
	expect(button_logic_take_long_press(&logic), "exactly 500 ms should count as a long press");
	expect(!button_logic_take_short_press(&logic), "exactly 500 ms should not count as short");
}

static void test_idle_and_held_button(void)
{
	button_logic_init(&logic);
	button_logic_update(&logic, false, 20);
	button_logic_update(&logic, true, 250);
	button_logic_update(&logic, true, 250);

	expect(!button_logic_take_short_press(&logic), "held button should not emit before release");
	expect(!button_logic_take_long_press(&logic), "held button should not emit before release");

	button_logic_update(&logic, false, 0);
	expect(button_logic_take_long_press(&logic), "release should emit the held button event");
}

static void test_pressed_edge_once(void)
{
	button_logic_init(&logic);

	expect(button_logic_was_pressed(&logic, true), "first pressed sample should report an edge");
	expect(!button_logic_was_pressed(&logic, true), "continued press should not report another edge");
	expect(!button_logic_was_pressed(&logic, false), "release should not report a press edge");
	expect(button_logic_was_pressed(&logic, true), "a new press should report a new edge");
}

static void test_events_are_independent(void)
{
	button_logic_init(&logic);
	button_logic_update(&logic, true, 80);
	button_logic_update(&logic, false, 0);

	expect(button_logic_take_short_press(&logic), "short event should be pending");

	button_logic_update(&logic, true, 700);
	button_logic_update(&logic, false, 0);

	expect(button_logic_take_long_press(&logic), "long event should be pending independently");
	expect(!button_logic_take_short_press(&logic), "consumed short event should stay cleared");
}

struct test_case {
	const char *name;
	void (*run)(void);
};

static const struct test_case test_cases[] = {
	{"short-press", test_short_press},
	{"long-press", test_long_press},
	{"threshold-boundary", test_threshold_boundary},
	{"idle-and-held-button", test_idle_and_held_button},
	{"pressed-edge-once", test_pressed_edge_once},
	{"independent-events", test_events_are_independent},
};

int main(int argc, char **argv)
{
	bool ran_test = false;

	for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
		if (argc == 1 || (argc == 2 && strcmp(argv[1], test_cases[i].name) == 0)) {
			test_cases[i].run();
			printf("PASS: %s\n", test_cases[i].name);
			ran_test = true;
		}
	}

	if (!ran_test) {
		fprintf(stderr, "Unknown test case: %s\n", argc > 1 ? argv[1] : "");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
