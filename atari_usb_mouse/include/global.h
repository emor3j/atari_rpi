#ifndef GLOBAL_H
#define GLOBAL_H


#include <signal.h>
#include <syslog.h>


// ANSI codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define SAVE_CURSOR "\033[s"
#define RESTORE_CURSOR "\033[u"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

// Macro for debug messages
#define DEBUG_PRINT(fmt, ...) \
    do { \
        if (debug_mode) { \
            if (daemon_mode) { \
                syslog(LOG_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__); \
            } else { \
                printf("[DEBUG] " fmt, ##__VA_ARGS__); \
            } \
        } \
    } while(0)

// Macro for error messages
#define ERROR_PRINT(fmt, ...) \
    do { \
        if (daemon_mode) { \
            syslog(LOG_ERR, "[ERROR] " fmt, ##__VA_ARGS__); \
        } else { \
            fprintf(stderr, RED fmt RESET, ##__VA_ARGS__); \
        } \
    } while(0)

// Macro for info messages
#define INFO_PRINT(fmt, ...) \
    do { \
        if (daemon_mode) { \
            syslog(LOG_INFO, "[INFO] " fmt, ##__VA_ARGS__); \
        } else { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

// Global variables
extern int debug_mode;
extern int daemon_mode;
// Global variable for signal handler
extern volatile sig_atomic_t running;


#endif // GLOBAL_H