#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "daemon.h"
#include "global.h"


char *pidfile_path = "/var/run/atari_usb_mouse.pid";


// Creates the PID file and writes the current process PID to it.
int create_pidfile() {
	FILE *fp = fopen(pidfile_path, "w");
	if (fp == NULL) {
		ERROR_PRINT("Unable to create PID file %s: %s\n", 
			pidfile_path, strerror(errno));
		return -1;
	}
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
	DEBUG_PRINT("PID file created: %s (PID: %d)\n", pidfile_path, getpid());
	return 0;
}

// Deletes the PID file if it exists.
void remove_pidfile() {
	if (unlink(pidfile_path) == -1 && errno != ENOENT) {
		ERROR_PRINT("Error removing PID file: %s\n", strerror(errno));
	} else {
		DEBUG_PRINT("PID file removed\n");
	}
}

// Detaches the process from the terminal and runs it as a daemon.
int daemonize() {
	pid_t pid;

	// First fork to create a background process
	pid = fork();
	if (pid < 0) {
		ERROR_PRINT("Background fork failed: %s\n", strerror(errno));
		return -1;
	}
	if (pid > 0) {	// Parent process exits
		exit(EXIT_SUCCESS);
	}

	// Create a new session and become session leader
	if (setsid() < 0) {
		ERROR_PRINT("setsid failed: %s\n", strerror(errno));
		return -1;
	}

	// Ignore SIGHUP signal
	signal(SIGHUP, SIG_IGN);

	// Second fork to ensure the daemon cannot reacquire a terminal
	pid = fork();
	if (pid < 0) {
		ERROR_PRINT("Second fork failed: %s\n", strerror(errno));
		return -1;
	}
	if (pid > 0) {	// Parent process exits
		exit(EXIT_SUCCESS);
	}

	// Change the working directory to root
	if (chdir("/") < 0) {
		ERROR_PRINT("chdir failed: %s\n", strerror(errno));
		return -1;
	}

	// Reset the file mode creation mask
	umask(0);

	// Close standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Redirect stdin, stdout, stderr to /dev/null
	open("/dev/null", O_RDONLY); // stdin
	open("/dev/null", O_WRONLY); // stdout
	open("/dev/null", O_WRONLY); // stderr

	// Open system log for the daemon
	openlog("atari_usb_mouse", LOG_PID | LOG_CONS, LOG_DAEMON);

	return 0;
}

// Checks if a daemon is already running by inspecting the PID file.
int check_running_daemon() {
	FILE *fp = fopen(pidfile_path, "r");

	// PID file does not exist, daemon is not running
	if (fp == NULL) {
		return 0;
	}

	pid_t pid;
	if (fscanf(fp, "%d", &pid) != 1) {
		fclose(fp);
		remove_pidfile(); // Corrupted PID file, remove it
		return 0;
	}
	fclose(fp);

	// Check if the process with the read PID exists
	if (kill(pid, 0) == 0) {	// Daemon is running
		return pid;
	} else {			// Stale PID file, remove i
		remove_pidfile();
		return 0;
	}
}