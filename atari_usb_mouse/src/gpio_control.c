#define _GNU_SOURCE
#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <stdlib.h>

#include "gpio_control.h"
#include "config.h"
#include "global.h"


// Delay between quadrature transitions (in microseconds)
#define QUADRATURE_DELAY 2000
#define MIN_DELAY 500      // Minimum delay (500µs for fast movements)
#define MAX_DELAY 2000     // Maximum delay (2ms for slow movements)
#define SPEED_THRESHOLD 5  // Threshold to consider a movement as "fast"

// GPIO chip device (usually /dev/gpiochip0 on Raspberry Pi)
#define GPIO_CHIP_DEVICE "/dev/gpiochip0"


// GPIO chip and line handles
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *line_xa = NULL;
static struct gpiod_line *line_xb = NULL;
static struct gpiod_line *line_ya = NULL;
static struct gpiod_line *line_yb = NULL;
static struct gpiod_line *line_left_button = NULL;
static struct gpiod_line *line_right_button = NULL;


// Quadrature states for forward (clockwise) motion
// 00 -> 01 -> 11 -> 10 -> 00
static const int quad_states[4][2] = {
	{0, 0},	// State 0
	{0, 1},	// State 1
	{1, 1},	// State 2
	{1, 0}	// State 3
};


// Helper function to get and configure a GPIO line
static struct gpiod_line* setup_output_line(struct gpiod_chip *chip, unsigned int offset, const char *consumer, int initial_value) {
	struct gpiod_line *line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		ERROR_PRINT("Failed to get GPIO line %u\n", offset);
		return NULL;
	}
	
	if (gpiod_line_request_output(line, consumer, initial_value) < 0) {
		ERROR_PRINT("Failed to request GPIO line %u as output\n", offset);
		return NULL;
	}
	
	return line;
}

// Initializes GPIOs used for quadrature signal generation.
int init_gpio() {
	// Open GPIO chip
	chip = gpiod_chip_open(GPIO_CHIP_DEVICE);
	if (!chip) {
		ERROR_PRINT("Unable to open GPIO chip %s\n", GPIO_CHIP_DEVICE);
		return -1;
	}

	gpio_initialized = 1;

	DEBUG_PRINT("Configuring GPIO ports as OUTPUT\n");
	
	// Configure pins as outputs with initial values
	if ((line_xa = setup_output_line(chip, config.pin_xa, "quadrature_xa", 0)) == NULL)
		return -1;
	if ((line_xb = setup_output_line(chip, config.pin_xb, "quadrature_xb", 0)) == NULL)
		return -1;
	if ((line_ya = setup_output_line(chip, config.pin_ya, "quadrature_ya", 0)) == NULL)
		return -1;
	if ((line_yb = setup_output_line(chip, config.pin_yb, "quadrature_yb", 0)) == NULL)
		return -1;
	if ((line_left_button = setup_output_line(chip, config.pin_left_button, "left_button", 0)) == NULL)
		return -1;
	if ((line_right_button = setup_output_line(chip, config.pin_right_button, "right_button", 0)) == NULL)
		return -1;

	DEBUG_PRINT("GPIO initialization complete\n");
	
	return 0;
}

// Cleans up and releases GPIOs.
void cleanup_gpio() {
	if (!gpio_initialized) return;

	DEBUG_PRINT("Cleaning up GPIOs...\n");
	
	// Release all GPIO lines
	if (line_xa) {
		gpiod_line_set_value(line_xa, 0);
		gpiod_line_release(line_xa);
		line_xa = NULL;
	}
	
	if (line_xb) {
		gpiod_line_set_value(line_xb, 0);
		gpiod_line_release(line_xb);
		line_xb = NULL;
	}
	
	if (line_ya) {
		gpiod_line_set_value(line_ya, 0);
		gpiod_line_release(line_ya);
		line_ya = NULL;
	}
	
	if (line_yb) {
		gpiod_line_set_value(line_yb, 0);
		gpiod_line_release(line_yb);
		line_yb = NULL;
	}
	
	if (line_left_button) {
		gpiod_line_set_value(line_left_button, 1);  // Boutons relâchés
		gpiod_line_release(line_left_button);
		line_left_button = NULL;
	}
	
	if (line_right_button) {
		gpiod_line_set_value(line_right_button, 1);
		gpiod_line_release(line_right_button);
		line_right_button = NULL;
	}
	
	// Close GPIO chip
	if (chip) {
		gpiod_chip_close(chip);
		chip = NULL;
	}
	
	gpio_initialized = 0;
	DEBUG_PRINT("GPIO cleanup complete\n");
}

// Updates the internal X quadrature state and applies it to the GPIOs.
void set_x_quadrature(quadrature_state_t *state, int xa, int xb) {
	state->xa_state = xa;
	state->xb_state = xb;
	
	if (line_xa && line_xb) {
		gpiod_line_set_value(line_xa, xa);
		gpiod_line_set_value(line_xb, xb);
	}
}

// Updates the internal Y quadrature state and applies it to the GPIOs.
void set_y_quadrature(quadrature_state_t *state, int ya, int yb) {
	state->ya_state = ya;
	state->yb_state = yb;
	
	if (line_ya && line_yb) {
		gpiod_line_set_value(line_ya, ya);
		gpiod_line_set_value(line_yb, yb);
	}
}

// Compute delay based on the number of pulses (adaptive speed)
static inline int calculate_delay(int pulses) {
	if (pulses <= SPEED_THRESHOLD) {
		return MAX_DELAY;
	}
	int delay = MAX_DELAY - ((pulses - SPEED_THRESHOLD) * (MAX_DELAY - MIN_DELAY) / 10);
	return (delay < MIN_DELAY) ? MIN_DELAY : delay;
}

// Generates quadrature pulses along the X axis.
void generate_x_pulses(quadrature_state_t *state, int delta) {
	int direction = (delta > 0) ? 1 : -1;
	int pulses = abs(delta);
	
	// Calculate adaptive delay based on movement speed
	int delay = calculate_delay(pulses);

	for (int i = 0; i < pulses; i++) {
		if (direction > 0) {
			state->x_phase = (state->x_phase + 1) % 4;
		} else {
			state->x_phase = (state->x_phase + 3) % 4;
		}

		set_x_quadrature(state,
				quad_states[state->x_phase][0],
				quad_states[state->x_phase][1]);

		DEBUG_PRINT("direction: %d,X phase: %d, pulses: %d, delay: %d\n", direction, state->x_phase, pulses, delay);

		usleep(MAX_DELAY);
	}
}

// Generates quadrature pulses along the Y axis.
void generate_y_pulses(quadrature_state_t *state, int delta) {
	int direction = (delta > 0) ? 1 : -1;
	int pulses = abs(delta);
	
	// Calculate adaptive delay based on movement speed
	int delay = calculate_delay(pulses);

	for (int i = 0; i < pulses; i++) {
		if (direction > 0) {
			state->y_phase = (state->y_phase + 1) % 4;
		} else {
			state->y_phase = (state->y_phase + 3) % 4;
		}

		set_y_quadrature(state,
				quad_states[state->y_phase][0],
				quad_states[state->y_phase][1]);

		DEBUG_PRINT("direction: %d,Y phase: %d, pulses: %d, delay: %d\n", direction, state->y_phase, pulses, delay);

		usleep(MAX_DELAY);
	}
}

// Sets the left button state (0 = pressed, 1 = released)
void set_left_button(int pressed) {
	if (line_left_button) {
		gpiod_line_set_value(line_left_button, pressed ? 0 : 1);
	}
}

// Sets the right button state (0 = pressed, 1 = released)
void set_right_button(int pressed) {
	if (line_right_button) {
		gpiod_line_set_value(line_right_button, pressed ? 0 : 1);
	}
}