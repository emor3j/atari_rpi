#include "gpio_control.h"
#include "config.h"
#include "global.h"
#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <stdlib.h>


// Delay between quadrature transitions (in microseconds)
#define QUADRATURE_DELAY 2000
#define MIN_DELAY 500      // Minimum delay (500µs for fast movements)
#define MAX_DELAY 2000     // Maximum delay (2ms for slow movements)
#define SPEED_THRESHOLD 5  // Threshold to consider a movement as "fast"


// Quadrature states for forward (clockwise) motion
// 00 -> 01 -> 11 -> 10 -> 00
static const int quad_states[4][2] = {
	{0, 0},	// State 0
	{0, 1},	// State 1
	{1, 1},	// State 2
	{1, 0}	// State 3
};


// Initializes GPIOs used for quadrature signal generation.
int init_gpio() {
	if (wiringPiSetup() == -1) {
		ERROR_PRINT("Unable to initialize wiringPi\n");
		return -1;
	}

	// Configure pins as outputs (according to the config)
	DEBUG_PRINT("Configuring GPIO ports as OUTPUT\n");
	pinMode(config.pin_xa, OUTPUT);
	pinMode(config.pin_xb, OUTPUT);
	pinMode(config.pin_ya, OUTPUT);
	pinMode(config.pin_yb, OUTPUT);
	pinMode(config.pin_left_button, OUTPUT);
	pinMode(config.pin_right_button, OUTPUT);

	// Initial state
	DEBUG_PRINT("Resetting GPIO ports\n");
	digitalWrite(config.pin_xa, LOW);
	digitalWrite(config.pin_xb, LOW);
	digitalWrite(config.pin_ya, LOW);
	digitalWrite(config.pin_yb, LOW);
	digitalWrite(config.pin_left_button, HIGH);
	digitalWrite(config.pin_right_button, HIGH);

	gpio_initialized = 1;
	return 0;
}

// Cleans up and releases GPIOs.
void cleanup_gpio() {
	if (!gpio_initialized) return;

	DEBUG_PRINT("Cleaning up GPIOs...\n");
	
	// Reset all pins to initial state
	DEBUG_PRINT("Resetting GPIO ports\n");
	digitalWrite(config.pin_xa, LOW);
	digitalWrite(config.pin_xb, LOW);
	digitalWrite(config.pin_ya, LOW);
	digitalWrite(config.pin_yb, LOW);
	digitalWrite(config.pin_left_button, HIGH);  // Boutons relâchés
	digitalWrite(config.pin_right_button, HIGH);
	
	// Optionally: set pins back to INPUT mode
	DEBUG_PRINT("Configuring GPIO ports as INPUT\n");
	pinMode(config.pin_xa, INPUT);
	pinMode(config.pin_xb, INPUT);
	pinMode(config.pin_ya, INPUT);
	pinMode(config.pin_xb, INPUT);
	pinMode(config.pin_left_button, INPUT);
	pinMode(config.pin_right_button, INPUT);
	
	gpio_initialized = 0;
	DEBUG_PRINT("GPIO cleanup complete\n");
}

// Updates the internal X quadrature state and applies it to the GPIOs.
void set_x_quadrature(quadrature_state_t *state, int xa, int xb) {
	state->xa_state = xa;
	state->xb_state = xb;
	digitalWrite(config.pin_xa, xa);
	digitalWrite(config.pin_xb, xb);
}

// Updates the internal Y quadrature state and applies it to the GPIOs.
void set_y_quadrature(quadrature_state_t *state, int ya, int yb) {
	state->ya_state = ya;
	state->yb_state = yb;
	digitalWrite(config.pin_ya, ya);
	digitalWrite(config.pin_yb, yb);
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

		usleep(delay);
	}
}