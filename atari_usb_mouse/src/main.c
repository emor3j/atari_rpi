/**
 * @file main.c
 * @brief Entry point for the atari_usb_mouse program.
 *
 * This program reads input from a modern USB mouse and translates it into
 * signals usable by an Atari 1040 STE mouse port using GPIO pins.
 * 
 * It supports daemon mode, debug output, device auto-detection, and 
 * configurable sensitivity. The program can be run interactively or as a
 * background service (systemd-compatible).
 * 
 * Usage:
 *  - See `--help` for command-line options.
 * 
 * 
 * Copyright (C) 2025 Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * Author: Jérôme SONRIER <jsid@emor3j.fr.eu.org>
 * Date: 2025-05-29
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <syslog.h>

#include "global.h"
#include "config.h"
#include "gpio_control.h"
#include "device_detection.h"
#include "daemon.h"
#include "monitor.h"


#ifndef VERSION
#define VERSION "unknown"
#endif

#define DEFAULT_SENSITIVITY 2  // Default sensitivity (divides movement by 2)


int debug_mode = 0;
int daemon_mode = 0;
int monitor_mode = 0;
volatile sig_atomic_t running = 1;
int gpio_initialized = 0;

// Default configuration values
const config_t default_config = {
	.pin_xa = 27,
	.pin_xb = 24,
	.pin_ya = 28,
	.pin_yb = 25,
	.pin_left_button = 23,
	.pin_right_button = 29,
	.sensitivity = 2,
	.device_path = ""
};
config_t config;


// Display usage help
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Atari ST mouse simulator using GPIO\n\n");
    printf("Options:\n");
    printf("  -c, --config FILE      JSON configuration file\n");
    printf("  -C,                    Display current configuration\n");
    printf("  -d, --debug            Enable debug messages\n");
    printf("  -D, --device DEVICE    Input device path (e.g. /dev/input/event1)\n");
    printf("  -m, --monitor          Show real-time GPIO and event status\n");
    printf("  -s, --sensitivity N    Set sensitivity (1=normal, 2=half, etc.)\n");
    printf("      --pin-xa N         GPIO pin for XA signal (default: %d)\n", default_config.pin_xa);
    printf("      --pin-xb N         GPIO pin for XB signal (default: %d)\n", default_config.pin_xb);
    printf("      --pin-ya N         GPIO pin for YA signal (default: %d)\n", default_config.pin_ya);
    printf("      --pin-yb N         GPIO pin for YB signal (default: %d)\n", default_config.pin_yb);
    printf("      --pin-bleft N      GPIO pin for left button (default: %d)\n", default_config.pin_left_button);
    printf("      --pin-bright N     GPIO pin for right button (default: %d)\n", default_config.pin_right_button);
    printf("  -b, --daemon           Run as a daemon\n");
    printf("  -p, --pidfile FILE     PID file for daemon mode (default: %s)\n", pidfile_path);
    printf("  -k, --kill             Stop running daemon\n");
    printf("  -r, --restart          Restart daemon\n");
    printf("  -t, --status           Show daemon status\n");
    printf("  -v, --version          Print version\n");
    printf("  -h, --help             Show this help message\n\n");
    printf("If no device is specified, it will be detected automatically.\n");
}

// Print version number
void print_version() {
    printf("%s\n", VERSION);
}

// Signal handler to stop the program gracefully
void signal_handler(int signum) {
    INFO_PRINT("\nSignal %d received, stopping...\n", signum);
    running = 0;
}

// Cleanup function executed on exit
void cleanup() {
    cleanup_gpio();
    cleanup_screen();
    if (daemon_mode) {
        remove_pidfile();
        closelog();
    }
}

// Process mouse events and generate corresponding GPIO signals
void process_mouse_event(struct input_event *ie, quadrature_state_t *state, int sensitivity) {
    if (sensitivity == 0) {
        sensitivity = DEFAULT_SENSITIVITY;
    }

    // Mettre à jour le timestamp et le compteur d'événements
    get_current_time(stats.last_event_time, sizeof(stats.last_event_time));

    switch (ie->type) {
        case EV_REL:
            switch (ie->code) {
                case REL_X:
                    if (ie->value != 0) {
                        stats.last_x_delta = ie->value;
                        
                        if (!monitor_mode) {
                            DEBUG_PRINT("X movement: %d\n", ie->value);
                        }
                        
                        int movement = -ie->value / sensitivity;
                        if (movement != 0) {
                            generate_x_pulses(state, movement);
                        }
                        
                        if (monitor_mode) {
                            display_monitor_status(state);
                        }
                    }
                    break;

                case REL_Y:
                    if (ie->value != 0) {
                        stats.last_y_delta = ie->value;
                        
                        if (!monitor_mode) {
                            DEBUG_PRINT("Y movement: %d\n", ie->value);
                        }
                        
                        int movement = ie->value / sensitivity;
                        if (movement != 0) {
                            generate_y_pulses(state, movement);
                        }
                        
                        if (monitor_mode) {
                            display_monitor_status(state);
                        }
                    }
                    break;
            }
            break;

        case EV_KEY:
            switch (ie->code) {
                case BTN_LEFT:
                    stats.left_button_state = ie->value;
                    
                    if (!monitor_mode) {
                        DEBUG_PRINT("Left button: %s\n", ie->value ? "pressed" : "released");
                    }
                    
                    digitalWrite(config.pin_left_button, ie->value ? LOW : HIGH);
                    
                    if (monitor_mode) {
                        display_monitor_status(state);
                    }
                    break;

                case BTN_RIGHT:
                    stats.right_button_state = ie->value;
                    
                    if (!monitor_mode) {
                        DEBUG_PRINT("Right button: %s\n", ie->value ? "pressed" : "released");
                    }
                    
                    digitalWrite(config.pin_right_button, ie->value ? LOW : HIGH);
                    
                    if (monitor_mode) {
                        display_monitor_status(state);
                    }
                    break;
            }
            break;

        case EV_SYN:
            // Synchronization event, no action needed
            break;
    }
}


/**
 * @brief Main program entry point.
 */
int main(int argc, char *argv[]) {
    // Set default configuration
    config = default_config;

    int fd;
    struct input_event ie;
    quadrature_state_t quad_state = {0, 0, 0, 0, 0, 0};
    ssize_t bytes_read;
    
    int opt;
    char *config_file = "/etc/atari_rpi/atari_usb_mouse.json";
    int sensitivity = DEFAULT_SENSITIVITY;
    int pin_xa, pin_xb, pin_ya, pin_yb, pin_bleft, pin_bright;
    pin_xa = pin_xb = pin_ya = pin_yb = pin_bleft = pin_bright = -1;
    char *mouse_device = NULL;
    int view_config = 0;

    // getopt_long options
    static struct option long_options[] = {
        {"config",      required_argument, 0, 'c'},
        {"daemon",      no_argument,       0, 'b'},
        {"debug",       no_argument,       0, 'd'},
        {"device",      required_argument, 0, 'D'},
        {"kill",        no_argument,       0, 'k'},
        {"monitor",     no_argument,       0, 'm'},
        {"pidfile",     required_argument, 0, 'p'},
        {"restart",     no_argument,       0, 'r'},
        {"sensitivity", required_argument, 0, 's'},
        {"status",      no_argument,       0, 't'},
        {"pin-xa",      required_argument, 0, 1001},
        {"pin-xb",      required_argument, 0, 1002},
        {"pin-ya",      required_argument, 0, 1003},
        {"pin-yb",      required_argument, 0, 1004},
        {"pin-left",    required_argument, 0, 1005},
        {"pin-right",   required_argument, 0, 1006},
        {"version",     no_argument      , 0, 'v'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    INFO_PRINT("Atari ST mouse simulator\n");

    // Signal handlers
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // Terminaison
    signal(SIGHUP, signal_handler);   // Hangup

    // Set cleanup function for atexit
    atexit(cleanup);

    // Parse command-line arguments
    while ((opt = getopt_long(argc, argv, "c:CbdD:s:mhkrtp:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'C':
                view_config = 1;
                break;
            case 'd':
                debug_mode = 1;
                DEBUG_PRINT("Debug mode enabled\n");
                break;
            case 'D':
                mouse_device = optarg;
                break;
            case 'm':
                monitor_mode = 1;
                DEBUG_PRINT("Monitor mode enabled\n");
                break;
            case 's':
                sensitivity = atoi(optarg);
                if (sensitivity < 1) {
                    ERROR_PRINT("Sensitivity must be >= 1\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 1001: // --pin-xa
                pin_xa = atoi(optarg);
                if (pin_xa < 0 || pin_xa > 40) {
                    ERROR_PRINT("Pin XA must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("XA pin set to : %d\n", config.pin_xa);
                break;
            case 1002: // --pin-xb
                pin_xb = atoi(optarg);
                if (pin_xb < 0 || pin_xb > 40) {
                    ERROR_PRINT("Pin XB must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("XB pin set to : %d\n", config.pin_xb);
                break;
            case 1003: // --pin-ya
                pin_ya = atoi(optarg);
                if (pin_ya < 0 || pin_ya > 40) {
                    ERROR_PRINT("Pin YA must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("YA pin set to : %d\n", config.pin_ya);
                break;
            case 1004: // --pin-yb
                pin_yb = atoi(optarg);
                if (pin_yb < 0 || pin_yb > 40) {
                    ERROR_PRINT("Pin YB must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("YB pin set to : %d\n", config.pin_yb);
                break;
            case 1005: // --pin-left
                pin_bleft = atoi(optarg);
                if (pin_bleft < 0 || pin_bleft > 40) {
                    ERROR_PRINT("Pin left button must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("Left button pin set to : %d\n", config.pin_left_button);
                break;
            case 1006: // --pin-right
                pin_bright = atoi(optarg);
                if (pin_bright < 0 || pin_bright > 40) {
                    ERROR_PRINT("Pin right button must be between 0 et 40\n");
                    exit(EXIT_FAILURE);
                }
                INFO_PRINT("Right button pin set to : %d\n", config.pin_right_button);
                break;
            case 'b':
                daemon_mode = 1;
                monitor_mode = 0; // Incompatible avec le mode daemon
                DEBUG_PRINT("Daemon mode enabled\n");
                break;
            case 'k':
                {
                    pid_t running_pid = check_running_daemon();
                    if (running_pid > 0) {
                        printf("Stopping daemon (PID: %d)...\n", running_pid);
                        if (kill(running_pid, SIGTERM) == 0) {
                            printf("TERM signal sent\n");
                        } else {
                            ERROR_PRINT("Cannot stop daemon: %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        printf("No running daemon\n");
                    }
                    exit(EXIT_SUCCESS);
                }
                break;
            case 'p':
                pidfile_path = optarg;
                break;
            case 'r':
                {
                    pid_t running_pid = check_running_daemon();
                    if (running_pid > 0) {
                        printf("Restart daemon (PID: %d)...\n", running_pid);
                        kill(running_pid, SIGTERM);
                        sleep(2); // Wait termination 
                    }
                    daemon_mode = 1;
                    monitor_mode = 0;
                    DEBUG_PRINT("Restart daemon mode\n");
                }
                break;
            case 't':
                {
                    pid_t running_pid = check_running_daemon();
                    if (running_pid > 0) {
                        printf("Daemon is running (PID: %d)\n", running_pid);
                    } else {
                        printf("Daemon is not running\n");
                    }
                    exit(EXIT_SUCCESS);
                }
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                ERROR_PRINT("Unknown option. Use -h for help.\n");
                exit(EXIT_FAILURE);
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Load configuration
    DEBUG_PRINT("Loading configuration from %s...\n", config_file);
    if (load_config(config_file, &config) < 0) {
        ERROR_PRINT("Cannot load configuration\n");
        exit(EXIT_FAILURE);
    }

    // Set config with options
    if (pin_xa != -1) {
        config.pin_xa = pin_xa;
        DEBUG_PRINT("Setting pin_xa=%d from command line\n", config.pin_xa);
    }
    if (pin_xb != -1) {
        config.pin_xb = pin_xb;
        DEBUG_PRINT("Setting pin_xb=%d from command line\n", config.pin_xb);
    }
    if (pin_ya != -1) {
        config.pin_ya = pin_ya;
        DEBUG_PRINT("Setting pin_ya=%d from command line\n", config.pin_ya);
    }
    if (pin_yb != -1) {
        config.pin_yb = pin_yb;
        DEBUG_PRINT("Setting pin_yb=%d from command line\n", config.pin_yb);
    }
    if (pin_bleft != -1) {
        config.pin_left_button = pin_bleft;
        DEBUG_PRINT("Setting pin_left_button=%d from command line\n", config.pin_left_button);
    }
    if (pin_bright != -1) {
        config.pin_right_button = pin_bright;
        DEBUG_PRINT("Setting pin_right_button=%d from command line\n", config.pin_right_button);
    }
    if (sensitivity != DEFAULT_SENSITIVITY) {
        config.sensitivity = sensitivity;
        DEBUG_PRINT("Setting sensitivity=%d from command line\n", config.sensitivity);
    }
    if (mouse_device != NULL ) {
        snprintf(config.device_path, sizeof(config.device_path), "%s", mouse_device);
        DEBUG_PRINT("Setting device_path=%s from command line\n", config.device_path);
    }

    // Check daemon mode requirements
    if (daemon_mode) {
        if (monitor_mode) {
            ERROR_PRINT("Daemon mode and monitor mode are not compatible\n");
            exit(EXIT_FAILURE);
        }
        
        // Check if daemon is running
        pid_t running_pid = check_running_daemon();
        if (running_pid > 0) {
            ERROR_PRINT("Daemon already runnong (PID: %d)\n", running_pid);
            exit(EXIT_FAILURE);
        }
        
        // Demonize
        if (daemonize() < 0) {
            exit(EXIT_FAILURE);
        }
        
        // Create PID file
        if (create_pidfile() < 0) {
            exit(EXIT_FAILURE);
        }
        
        INFO_PRINT("Daemon started (PID: %d)\n", getpid());
    }

    // Get mouse device
    if (config.device_path[0] == '\0') {
        INFO_PRINT("Auto detect mouse device...\n");
        char *detected_device = wait_for_mouse_device();
        if (detected_device == NULL) {
            DEBUG_PRINT("Stop while searching device\n");
            exit(EXIT_SUCCESS);
        } else {
            snprintf(config.device_path, sizeof(config.device_path), "%s", detected_device);
        }
    }

    // Show configuration
    if (view_config) {
        printf("Configuration :\n");
        print_config();
        exit(0);
    }

    // Open mouse event file
    if ((fd = open(config.device_path, O_RDONLY | O_NONBLOCK)) == -1) {
        ERROR_PRINT("Cannot open file %s: %s\n", config.device_path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    DEBUG_PRINT("Device %s opened\n", config.device_path);
    
    // Init GPIO
    DEBUG_PRINT("Initialisation of PIO ports...\n");
    if (init_gpio() < 0) {
        exit(EXIT_FAILURE);
    }

    // Init screen if monitor mode is enable
    if (monitor_mode) {
        printf(HIDE_CURSOR);
        printf(CLEAR_SCREEN);
        get_current_time(stats.last_event_time, sizeof(stats.last_event_time));
        display_monitor_status(&quad_state);
    }
        
    // Main loop
    if (!monitor_mode) {
        INFO_PRINT("Waiting mouse events...\n");
    }

    while (running) {
        // open/re-open device file
        if ((fd = open(config.device_path, O_RDONLY | O_NONBLOCK)) == -1) {
            INFO_PRINT("Cannot open device %s : %s\n", config.device_path, strerror(errno));
            INFO_PRINT("Looking for new mouse device...\n");
            
            char *new_device = wait_for_mouse_device();
            if (new_device == NULL) {
                DEBUG_PRINT("Stop while searching device\n");
                break;
            }
            
            snprintf(config.device_path, sizeof(config.device_path), "%s", new_device);
            INFO_PRINT("New device detected : %s\n", config.device_path);
            continue;
        }
        
        // Reading loop for this device
        while (running) {
            fd_set readfds;
            struct timeval timeout;
    
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
    
            // 50ms timeout to check running flag
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
    
            int select_result = select(fd + 1, &readfds, NULL, NULL, &timeout);
    
            if (select_result == -1) {
                if (errno == EINTR) {
                    continue;  // Signal received, check running
                }
                perror("select");
                break;
            }
    
            if (select_result == 0) {
                // Timeout - continue to check running
                continue;
            }
    
            if (FD_ISSET(fd, &readfds)) {
                bytes_read = read(fd, &ie, sizeof(struct input_event));
    
                if (bytes_read == -1) {
                    if (errno == EINTR) {
                        // System interruption - continue
                        continue;
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // No available data (non-bloquant mode)
                        continue;
                    }
                    if (errno == ENODEV || errno == ENOENT) {
                        // Device disconnected
                        INFO_PRINT("Mouse device disconnected\n");
                        close(fd);
                        fd = -1;
                        break; // exit to search for new device
                    }
                    ERROR_PRINT("Reading events: %s\n", strerror(errno));
                    break;
                }
    
                if (bytes_read == 0) {
                    // EOF - device probably disconnected
                    INFO_PRINT("Mouse device disconnected (EOF)\n");
                    close(fd);
                    fd = -1;
                    break;
                }
    
                if (bytes_read != sizeof(struct input_event)) {
                    ERROR_PRINT("Read partial event\n");
                    continue;
                }
    
                // Process event
                process_mouse_event(&ie, &quad_state, config.sensitivity);
            }
        }
        
        // Clode fd if open
        if (fd != -1) {
            close(fd);
            fd = -1;
        }
        
        // If we are here and running is true, device probably disconnected
        if (running) {
            INFO_PRINT("Looking for new mouse device...\n");
        }
    }

    INFO_PRINT("Quit\n");
    
    return 0;
}