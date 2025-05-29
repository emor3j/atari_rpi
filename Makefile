# Main Makefile for the atari_rpi project

# List of all projects/applications in the repository
PROJECTS = atari_usb_mouse

# Rule to build all applications
all: $(PROJECTS)

# Rule to build a specific application
$(PROJECTS):
	$(MAKE) -C $@

# Rule to clean all applications
clean:
	$(MAKE) -C atari_usb_mouse clean

run:
	$(MAKE) -C atari_usb_mouse run

# Default rule
.PHONY: all clean run $(PROJECTS)