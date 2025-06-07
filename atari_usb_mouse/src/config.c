#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <json-c/json.h>

#include "config.h"
#include "global.h"


// Loads the configuration from a file.
int load_config(const char *config_path, config_t *cfg) {
	// Start with default configuration values
	*cfg = default_config;

	struct stat st;
	if (stat(config_path, &st) != 0) {
		// Config file does not exist; silently keep defaults
		DEBUG_PRINT("No configuration file found\n");
		return 0;
	}

	FILE *fp = fopen(config_path, "r");
	if (fp == NULL) {
		ERROR_PRINT("Cannot open configuration file %s\n", config_path);
		return -1;
	}

	// Determine file size to allocate exact buffer
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *json_string = malloc(file_size + 1);
	if (json_string == NULL) {
		ERROR_PRINT("Memory allocation error while reading config file %s\n", config_path);
		fclose(fp);
		return -1;
	}

	// Read full file into buffer
	size_t bytes_read = fread(json_string, 1, file_size, fp);
	if (bytes_read != (size_t)file_size) {
		ERROR_PRINT("Error reading config file: expected %ld bytes, read %zu bytes\n", file_size, bytes_read);
		free(json_string);
		fclose(fp);
		return -1;
	}

	json_string[file_size] = '\0';
	fclose(fp);

	// Parse the JSON content
	json_object *root = json_tokener_parse(json_string);
	free(json_string);

	// Invalid JSON format
	if (root == NULL) {
		ERROR_PRINT("Invalid JSON in config file\n");
		return -1;
	}

	// Parse GPIO pin configuration
	json_object *pins_obj;
	if (json_object_object_get_ex(root, "pins_gpio", &pins_obj)) {
		json_object *pin_obj;
		if (json_object_object_get_ex(pins_obj, "xa", &pin_obj)) {
			cfg->pin_xa = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_xa=%d from config file\n", cfg->pin_xa);
		}
		if (json_object_object_get_ex(pins_obj, "xb", &pin_obj)) {
			cfg->pin_xb = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_xb=%d from config file\n", cfg->pin_xb);
		}
		if (json_object_object_get_ex(pins_obj, "ya", &pin_obj)) {
			cfg->pin_ya = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_ya=%d from config file\n", cfg->pin_ya);
		}
		if (json_object_object_get_ex(pins_obj, "yb", &pin_obj)) {
			cfg->pin_yb = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_yb=%d from config file\n", cfg->pin_yb);
		}
		if (json_object_object_get_ex(pins_obj, "left_button", &pin_obj)) {
			cfg->pin_left_button = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_left_button=%d from config file\n", cfg->pin_left_button);
		}
		if (json_object_object_get_ex(pins_obj, "right_button", &pin_obj)) {
			cfg->pin_right_button = json_object_get_int(pin_obj);
			DEBUG_PRINT("Setting pin_right_button=%d from config file\n", cfg->pin_right_button);
		}
	}

	// Parse general parameters
	json_object *param_obj;
	if (json_object_object_get_ex(root, "sensitivity", &param_obj)) {
		cfg->sensitivity = json_object_get_int(param_obj);
		DEBUG_PRINT("Setting sensitivity=%d from config file\n", cfg->sensitivity);
	}
	if (json_object_object_get_ex(root, "device_path", &param_obj)) {
		const char *path = json_object_get_string(param_obj);
		if (path != NULL) {
			// Copy with bounds checking
			strncpy(cfg->device_path, path, sizeof(cfg->device_path) - 1);
			cfg->device_path[sizeof(cfg->device_path) - 1] = '\0';
			DEBUG_PRINT("Setting device_path=%s from config file\n", cfg->device_path);
		}
	}

	// Release JSON object memory
	json_object_put(root);

	return 0;
}

// Prints the current configuration to stdout.
void print_config() {
	printf("pin_xa=%d\n", config.pin_xa);
	printf("pin_xb=%d\n", config.pin_xb);
	printf("pin_ya=%d\n", config.pin_ya);
	printf("pin_yb=%d\n", config.pin_yb);
	printf("pin_left_button=%d\n", config.pin_left_button);
	printf("pin_right_button=%d\n", config.pin_right_button);
	printf("sensitivity=%d\n", config.sensitivity);
	printf("device_path=%s\n", config.device_path);
}