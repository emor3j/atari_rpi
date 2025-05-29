#ifndef MONITOR_H
#define MONITOR_H


#include "gpio_control.h"
#include <stddef.h>


/**
 * @struct monitor_stats_t
 * @brief Holds statistics about the latest mouse events.
 *
 * This structure stores the most recent deltas on X and Y axes,
 * the states of the mouse buttons, and the timestamp of the last event.
 */
typedef struct {
	int last_x_delta;		/**< Last movement delta on the X axis */
	int last_y_delta;		/**< Last movement delta on the Y axis */
	int left_button_state;		/**< State of the left mouse button (1 = pressed, 0 = released) */
	int right_button_state;		/**< State of the right mouse button (1 = pressed, 0 = released) */
	char last_event_time[32];	/**< Timestamp of the last detected event */
} monitor_stats_t;


/**
 * @brief Displays the current quadrature and mouse status.
 * 
 * @param state Pointer to the current quadrature state.
 */
void display_monitor_status(quadrature_state_t *state);

/**
 * @brief Restores the screen or terminal to its original state.
 */
void cleanup_screen(void);

/**
 * @brief Fills a buffer with the current time as a formatted string.
 *
 * @param buffer Pointer to the character buffer to fill.
 * @param size Size of the buffer.
 */
void get_current_time(char *buffer, size_t size);


/**
 * @brief Enables or disables monitor mode.
 */
extern int monitor_mode;

/**
 * @brief Global structure holding the last known mouse event stats.
 */
extern monitor_stats_t stats;


#endif // MONITOR_H