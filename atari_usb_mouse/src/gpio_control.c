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

// GPIO chip and request handles
static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *request = NULL;

// Line offsets array for easier management
static unsigned int line_offsets[6];

// Line indices for easier reference
enum {
    LINE_XA = 0,
    LINE_XB = 1,
    LINE_YA = 2,
    LINE_YB = 3,
    LINE_LEFT_BUTTON = 4,
    LINE_RIGHT_BUTTON = 5,
    NUM_LINES = 6
};


// Quadrature states for forward (clockwise) motion
// 00 -> 01 -> 11 -> 10 -> 00
static const int quad_states[4][2] = {
	{0, 0},	// State 0
	{0, 1},	// State 1
	{1, 1},	// State 2
	{1, 0}	// State 3
};


int init_gpio() {
	struct gpiod_line_settings *settings = NULL;
	struct gpiod_line_config *line_config = NULL;
	struct gpiod_request_config *req_config = NULL;
	enum gpiod_line_value initial_values[NUM_LINES];

	// Open GPIO chip
	chip = gpiod_chip_open(GPIO_CHIP_DEVICE);
	if (!chip) {
		ERROR_PRINT("Unable to open GPIO chip %s\n", GPIO_CHIP_DEVICE);
		return -1;
	}

	// Setup line offsets from config
	line_offsets[LINE_XA] = config.pin_xa;
	line_offsets[LINE_XB] = config.pin_xb;
	line_offsets[LINE_YA] = config.pin_ya;
	line_offsets[LINE_YB] = config.pin_yb;
	line_offsets[LINE_LEFT_BUTTON] = config.pin_left_button;
	line_offsets[LINE_RIGHT_BUTTON] = config.pin_right_button;

	// Set initial values (quadrature lines start at 0, buttons released = 1)
	initial_values[LINE_XA] = GPIOD_LINE_VALUE_INACTIVE;
	initial_values[LINE_XB] = GPIOD_LINE_VALUE_INACTIVE;
	initial_values[LINE_YA] = GPIOD_LINE_VALUE_INACTIVE;
	initial_values[LINE_YB] = GPIOD_LINE_VALUE_INACTIVE;
	initial_values[LINE_LEFT_BUTTON] = GPIOD_LINE_VALUE_ACTIVE;   // Button released
	initial_values[LINE_RIGHT_BUTTON] = GPIOD_LINE_VALUE_ACTIVE;  // Button released

	DEBUG_PRINT("Configuring GPIO ports as OUTPUT\n");

	// Create line settings for output
	settings = gpiod_line_settings_new();
	if (!settings) {
		ERROR_PRINT("Failed to create line settings\n");
		return -1;
	}

	if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) != 0) {
		ERROR_PRINT("Failed to set line direction\n");
		return -1;
	}

	// Create line configuration
	line_config = gpiod_line_config_new();
	if (!line_config) {
		ERROR_PRINT("Failed to create line config\n");
		return -1;
	}

	// Apply settings to all lines
	if (gpiod_line_config_add_line_settings(line_config, line_offsets, NUM_LINES, settings) != 0) {
		ERROR_PRINT("Failed to add line settings to config\n");
		return -1;
	}

	// Set initial output values
	if (gpiod_line_config_set_output_values(line_config, initial_values, NUM_LINES) != 0) {
		ERROR_PRINT("Failed to set initial output values\n");
		return -1;
	}

	// Create request configuration
	req_config = gpiod_request_config_new();
	if (!req_config) {
		ERROR_PRINT("Failed to create request config\n");
		return -1;
	}

	// Set consumer name
	gpiod_request_config_set_consumer(req_config, "quadrature_controller");

	// Request the lines
	request = gpiod_chip_request_lines(chip, req_config, line_config);
	if (!request) {
		ERROR_PRINT("Failed to request GPIO lines\n");
		return -1;
	}

	gpio_initialized = 1;

	DEBUG_PRINT("GPIO initialization complete\n");

	// Clean up temporary objects
	if (settings)
		gpiod_line_settings_free(settings);
	if (line_config)
		gpiod_line_config_free(line_config);
	if (req_config)
		gpiod_request_config_free(req_config);

	return 0;
}

// Cleans up and releases GPIOs.
void cleanup_gpio() {
	if (!gpio_initialized) return;

	DEBUG_PRINT("Cleaning up GPIOs...\n");

	// Set all quadrature lines to 0 and buttons to released (1) before cleanup
	if (request) {
		enum gpiod_line_value final_values[NUM_LINES] = {
			GPIOD_LINE_VALUE_INACTIVE,  // XA
			GPIOD_LINE_VALUE_INACTIVE,  // XB
			GPIOD_LINE_VALUE_INACTIVE,  // YA
			GPIOD_LINE_VALUE_INACTIVE,  // YB
			GPIOD_LINE_VALUE_ACTIVE,    // Left button released
			GPIOD_LINE_VALUE_ACTIVE     // Right button released
		};
		
		gpiod_line_request_set_values(request, final_values);
		gpiod_line_request_release(request);
		request = NULL;
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

	if (request) {
		gpiod_line_request_set_value(request, line_offsets[LINE_XA], 
			xa ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
		gpiod_line_request_set_value(request, line_offsets[LINE_XB], 
			xb ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
	}
}

// Updates the internal Y quadrature state and applies it to the GPIOs.
void set_y_quadrature(quadrature_state_t *state, int ya, int yb) {
	state->ya_state = ya;
	state->yb_state = yb;

	if (request) {
		gpiod_line_request_set_value(request, line_offsets[LINE_YA], 
			ya ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
		gpiod_line_request_set_value(request, line_offsets[LINE_YB], 
			yb ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
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

		usleep(delay);
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

		usleep(delay);
	}
}

// Sets the left button state (0 = pressed, 1 = released)
void set_left_button(int pressed) {
	if (request) {
		enum gpiod_line_value value = pressed ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
		int result = gpiod_line_request_set_value(request, line_offsets[LINE_LEFT_BUTTON], value);
		DEBUG_PRINT("Left button: pressed=%d, gpio_value=%d, result=%d\n", pressed, value, result);
	}
}

// Sets the right button state (0 = pressed, 1 = released)
void set_right_button(int pressed) {
	if (request) {
		enum gpiod_line_value value = pressed ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
		int result = gpiod_line_request_set_value(request, line_offsets[LINE_RIGHT_BUTTON], value);
		DEBUG_PRINT("Right button: pressed=%d, gpio_value=%d, result=%d\n", pressed, value, result);
	}
}
