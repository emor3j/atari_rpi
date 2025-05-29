#ifndef DEVICE_DETECTION_H
#define DEVICE_DETECTION_H


#define MAX_DEVICES 32


/**
 * Tests whether the device at the given path is a compatible mouse.
 *
 * @param device_path The path to the device (e.g., "/dev/input/eventX").
 * @return 1 if the device is a supported mouse, 0 if not, and -1 on error.
 */
int test_mouse_device(const char *device_path);

/**
 * Attempts to automatically find a compatible mouse device among available input devices.
 *
 * @return A newly allocated string containing the path to the mouse device
 *         (e.g., "/dev/input/eventX"), or NULL if not found.
 */
char* find_mouse_device(void);

/**
 * Waits for a compatible mouse device to appear.
 *
 * @return A newly allocated string containing the path to the detected mouse device,
 *         or NULL if an error occurs.
 */
char* wait_for_mouse_device(void);


#endif // DEVICE_DETECTION_H