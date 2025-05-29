/**
 * @file monitor.c
 * @brief Provides monitoring and display for GPIO and mouse events.
 */


#include "monitor.h"
#include "global.h"
#include <stdio.h>
#include <time.h>


monitor_stats_t stats = {0};


// Displays the current quadrature and mouse status.
void display_monitor_status(quadrature_state_t *state) {
	if (!monitor_mode) return;
	
	printf(SAVE_CURSOR);
	printf(CURSOR_HOME);
	
	printf("\033[36m═══════════════════════════════════════════════════════════════════════════════\n\033[0m");
	printf("\033[36m                           ATARI ST MOUSE SIMULATOR                            \n\033[0m");
	printf("\033[36m═══════════════════════════════════════════════════════════════════════════════\n\033[0m");
	
	printf("\n┌─ GPIO STATES ────────────────────────────────────────────────────────────────┐\n");
	printf("│ X Axis:  XA=\033[32m%d\033[0m  XB=\033[32m%d\033[0m  (Phase: %d)    ", 
		   state->xa_state, state->xb_state, state->x_phase);
	printf("Y Axis:  YA=\033[32m%d\033[0m  YB=\033[32m%d\033[0m  (Phase: %d)           │\n", 
		   state->ya_state, state->yb_state, state->y_phase);
	
	printf("│ Buttons: Left=%s%s\033[0m  Right=%s%s\033[0m                                       │\n",
		   (stats.left_button_state ? "\033[31m" : "\033[32m"), 
		   (stats.left_button_state ? "PRESSED " : "RELEASED"),
		   (stats.right_button_state ? "\033[31m" : "\033[32m"), 
		   (stats.right_button_state ? "PRESSED " : "RELEASED"));
	printf("└──────────────────────────────────────────────────────────────────────────────┘\n");
	
	printf("\n┌─ LAST MOVEMENTS ─────────────────────────────────────────────────────────────┐\n");
	printf("│ Last X movement: \033[33m%+4d\033[0m           Last Y movement: \033[33m%+4d\033[0m                        │\n", 
		   stats.last_x_delta, stats.last_y_delta);
	printf("│ Last activity: \033[35m%s\033[0m                                                      │\n", 
		   stats.last_event_time);
	printf("└──────────────────────────────────────────────────────────────────────────────┘\n");
	
	printf("\n\033[33mPress Ctrl+C to quit\033[0m\n");
	
	printf(RESTORE_CURSOR);
	fflush(stdout);
}

// Restores the screen or terminal to its original state.
void cleanup_screen() {
	if (monitor_mode) {
		printf(SHOW_CURSOR);
		printf("\n");
	}
}

// Fills a buffer with the current time as a formatted string.
void get_current_time(char *buffer, size_t size) {
	time_t now;
	struct tm *tm_info;
	
	time(&now);
	tm_info = localtime(&now);
	strftime(buffer, size, "%H:%M:%S", tm_info);
}