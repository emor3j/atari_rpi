#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H


/**
 * Structure to track the current state of the X and Y quadrature signals
 */
typedef struct {
	int xa_state;	// Current state of XA GPIO line
	int xb_state;	// Current state of XB GPIO line
	int ya_state;	// Current state of YA GPIO line
	int yb_state;	// Current state of YB GPIO line
	int x_phase;	// Current phase of X quadrature signal
	int y_phase;	// Current phase of Y quadrature signal
} quadrature_state_t;


/**
 * Initializes GPIOs used for quadrature signal generation.
 * Must be called before any other GPIO-related operation.
 *
 * @return 0 on success, -1 on failure.
 */
int init_gpio(void);

/**
 * Cleans up and releases GPIOs.
 * Should be called before program exit.
 */
void cleanup_gpio(void);

/**
 * Updates the internal X quadrature state and applies it to the GPIOs.
 *
 * @param state Pointer to the current quadrature state.
 * @param xa    New state for XA signal (0 or 1).
 * @param xb    New state for XB signal (0 or 1).
 */
void set_x_quadrature(quadrature_state_t *state, int xa, int xb);

/**
 * Updates the internal Y quadrature state and applies it to the GPIOs.
 *
 * @param state Pointer to the current quadrature state.
 * @param ya    New state for YA signal (0 or 1).
 * @param yb    New state for YB signal (0 or 1).
 */
void set_y_quadrature(quadrature_state_t *state, int ya, int yb);

/**
 * Generates quadrature pulses along the X axis.
 *
 * @param state Pointer to the current quadrature state.
 * @param delta Number of movement steps to simulate (positive or negative).
 */
void generate_x_pulses(quadrature_state_t *state, int delta);

/**
 * Generates quadrature pulses along the Y axis.
 *
 * @param state Pointer to the current quadrature state.
 * @param delta Number of movement steps to simulate (positive or negative).
 */
void generate_y_pulses(quadrature_state_t *state, int delta);


// Flag indicating whether GPIO has been successfully initialized
extern int gpio_initialized;


#endif // GPIO_CONTROL_H