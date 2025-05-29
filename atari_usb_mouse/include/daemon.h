#ifndef DAEMON_H
#define DAEMON_H


/**
 * Creates the PID file and writes the current process PID to it.
 * 
 * @return 0 on success, -1 on failure.
 */
int create_pidfile(void);

/**
 * Deletes the PID file if it exists.
 */
void remove_pidfile(void);

/**
 * Detaches the process from the terminal and runs it as a daemon.
 * 
 * @return 0 on success, -1 on failure.
 */
int daemonize(void);

/**
 * Checks if a daemon is already running by inspecting the PID file.
 * 
 * @return pid if a process is running, 0 if not.
 */
int check_running_daemon(void);


/**
 * Path to the PID file used to identify the running daemon.
 */
extern char *pidfile_path;


#endif // DAEMON_H