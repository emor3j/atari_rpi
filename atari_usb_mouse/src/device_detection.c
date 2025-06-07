#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>

#include "device_detection.h"
#include "global.h"


// Tests whether the device at the given path is a compatible mouse.
int test_mouse_device(const char *device_path) {
	int fd;
	unsigned long evbit = 0;
	
	fd = open(device_path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return 0;
	
	// Check if the device supports relative movement events (EV_REL)
	DEBUG_PRINT("Testing %s capabilities\n", device_path);
	if (ioctl(fd, EVIOCGBIT(0, EV_MAX), &evbit) >= 0) {
		if (evbit & (1 << EV_REL)) {	// Valid relative motion device
			close(fd);
			DEBUG_PRINT("%s support EV_REL events\n", device_path);
			return 1;
		} else {
			DEBUG_PRINT("%s does not support EV_REL events\n", device_path);
		}
	} else {
		DEBUG_PRINT("ioctl EVIOCGBIT failed\n");
	}
	
	close(fd);
	return 0;
}

// Attempts to automatically find a compatible mouse device among available input devices.
char* find_mouse_device() {
	FILE *fp;
	char line[256];
	static char device_path[64];
	int event_nums[MAX_DEVICES];
	char device_names[MAX_DEVICES][256];
	int device_count = 0;

	// Automatically find a mouse device by parsing /proc/bus/input/devices
	DEBUG_PRINT("Reading input device list\n");
	fp = fopen("/proc/bus/input/devices", "r");
	if (fp == NULL) {
		ERROR_PRINT("Filed to open /proc/bus/input/devices\n");
		return NULL;
	}

	int in_mouse_section = 0;
	int current_event = -1;
	char current_name[256] = "";

	while (fgets(line, sizeof(line), fp)) {
		// Start of a new device section
		if (line[0] == 'I' && strstr(line, "Bus=")) {
			in_mouse_section = 0;  // Reset for new device
			current_event = -1;
			current_name[0] = '\0';
		}

		// Look for a device name containing keywords typical of mice
		if (line[0] == 'N' && strstr(line, "Name=")) {
			if (strstr(line, "mouse") || strstr(line, "Mouse") ||
				strstr(line, "USB") || strstr(line, "Optical")) {
				in_mouse_section = 1;
				sscanf(line, "N: Name=\"%255[^\"]", current_name);
				DEBUG_PRINT("Potential device found: %s\n", current_name);
			}
		}

		// If we're in a potential mouse section, try to extract the "eventX" handler
		if (in_mouse_section && line[0] == 'H' && strstr(line, "Handlers=")) {
			char *event_ptr = strstr(line, "event");
			if (event_ptr) {
				sscanf(event_ptr, "event%d", &current_event);
				if (current_event >= 0 && device_count < MAX_DEVICES) {
					event_nums[device_count] = current_event;
					snprintf(device_names[device_count], sizeof(device_names[device_count]), "%s", current_name);
					device_count++;
				}
			}
		}
	}

	fclose(fp);

	// Test each event device found to see if it's a valid mouse
	for (int i = 0; i < device_count; i++) {
		snprintf(device_path, sizeof(device_path), "/dev/input/event%d", event_nums[i]);
		DEBUG_PRINT("Testing device: %s (%s)...\n", device_path, device_names[i]);
		
		if (test_mouse_device(device_path)) {
			INFO_PRINT("Mouse device detected: %s (%s)\n", device_path, device_names[i]);
			return device_path;
		} else {
			DEBUG_PRINT("Not a valid mouse device\n");
		}
	}

	INFO_PRINT("No valid mouse device found\n");
	return NULL;
}

// Waits for a compatible mouse device to appear.
char* wait_for_mouse_device() {
	char *device_path;
	
	INFO_PRINT("Searching for a mouse device...\n");
	
	while (running) {
		device_path = find_mouse_device();
		if (device_path != NULL) {
			return device_path;
		}
		
		if (!running) break;
		
		DEBUG_PRINT("No mouse device found, retrying in 3 seconds...\n");
		
		// Wait for 3 seconds in 100ms intervals, checking if still running
		for (int i = 0; i < 30 && running; i++) {
			usleep(100000); // 100ms x 30 = 3 secondes
		}
	}
	
	return NULL;
}
