#ifndef CONFIG_H
#define CONFIG_H


/**
 * Structure holding the configuration for the program.
 *
 * - pin_xa, pin_xb: GPIO pins used for horizontal (X-axis) signal generation.
 * - pin_ya, pin_yb: GPIO pins used for vertical (Y-axis) signal generation.
 * - pin_left_button: GPIO pin for the left mouse button.
 * - pin_right_button: GPIO pin for the right mouse button.
 * - sensitivity: sensitivity factor applied to mouse movement.
 * - device_path: path to the input device
 */
typedef struct {
	int pin_xa;
	int pin_xb;
	int pin_ya;
	int pin_yb;
	int pin_left_button;
	int pin_right_button;
	int sensitivity;
	char device_path[256];
} config_t;


/**
 * Loads the configuration from a file.
 * 
 * This function initializes the provided config structure with the default configuration,
 * then attempts to read and parse the specified JSON file. If the file is missing,
 * the function returns 0 and leaves the configuration as default.
 * If the file exists and is valid, values in the JSON file override the defaults.
 *
 * @param config_path Path to the configuration file.
 * @param cfg Pointer to a config_t structure where the configuration will be stored.
 * @return 0 on success or if the file is missing (defaults are used),
 *         -1 on error (e.g., file open error, memory allocation failure, or invalid JSON).
 */
int load_config(const char *config_path, config_t *cfg);

/**
 * Prints the current configuration to stdout.
 */
void print_config(void);


/**
 * Default configuration values.
 */
extern const config_t default_config;

/**
 * Global configuration used by the program after initialization.
 */
extern config_t config;


#endif // CONFIG_H