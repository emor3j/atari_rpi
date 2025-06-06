# Makefile for the atari_usb_mouse application

# Variables
VERSION := 1.0.0
CC = gcc
CFLAGS = -DVERSION=\"$(VERSION)\" -Iinclude -Wall -Wextra -O2 -std=c99
LDFLAGS = -lgpiod -ljson-c
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = atari_usb_mouse
CONFIG_FILE = atari_usb_mouse.json
CONFIG_DEST = /etc/atari_rpi/$(CONFIG_FILE)

# Default rule
all: $(TARGET)

# Compilation of the main program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Installation (requires root privileges)
install: $(TARGET)
	sudo cp $(TARGET) /usr/bin/
	sudo chmod 755 /usr/bin/$(TARGET)
	@if [ -f $(CONFIG_FILE) ]; then \
		sudo cp $(CONFIG_FILE) $(CONFIG_DEST); \
		sudo chmod 644 $(CONFIG_DEST); \
		echo "Configuration installed in $(CONFIG_DEST)"; \
	else \
		echo "No configuration file to install"; \
	fi

# Uninstallation
uninstall:
	sudo rm -f /usr/bin/$(TARGET)
	sudo rm -f $(CONFIG_DEST)
	echo "Uninstallation complete"

# Cleaning
clean:
	rm -f $(OBJS) $(TARGET)

# Full cleaning
distclean: clean
	rm -f *~ *.bak

# Test compilation
test: $(TARGET)
	./$(TARGET) --help

# Debug compilation
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Create a sample configuration file
config:
	@echo "Creating a sample configuration file..."
	@echo '{' > $(CONFIG_FILE)
	@echo '  "pins_gpio": {' >> $(CONFIG_FILE)
	@echo '    "xa": 16,' >> $(CONFIG_FILE)
	@echo '    "xb": 19,' >> $(CONFIG_FILE)
	@echo '    "ya": 20,' >> $(CONFIG_FILE)
	@echo '    "yb": 26,' >> $(CONFIG_FILE)
	@echo '    "left_button": 13,' >> $(CONFIG_FILE)
	@echo '    "right_button": 21' >> $(CONFIG_FILE)
	@echo '  },' >> $(CONFIG_FILE)
	@echo '  "sensitivity": 2,' >> $(CONFIG_FILE)
	@echo '  "device_path": ""' >> $(CONFIG_FILE)
	@echo '}' >> $(CONFIG_FILE)
	@echo "File $(CONFIG_FILE) created"

# Display help
help:
	@echo "Makefile for $(TARGET)"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Compile the program (default)"
	@echo "  install   - Install the program in /usr/bin/ (requires sudo)"
	@echo "  uninstall - Uninstall the program"
	@echo "  clean     - Remove compiled files"
	@echo "  distclean - Full cleaning"
	@echo "  debug     - Compile with debug symbols"
	@echo "  test      - Compile and test help display"
	@echo "  config    - Create a sample configuration file"
	@echo "  run       - Run the atari_usb_mouse application"
	@echo "  help      - Display this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Compile the program"
	@echo "  make install      # Compile and install"
	@echo "  make config       # Create a sample config file"
	@echo "  make clean        # Clean compiled files"
	@echo "  make run          # Run the atari_usb_mouse application"

# Run the atari_usb_mouse application
run: $(TARGET)
	./$(TARGET)

# Declaration of targets that do not correspond to files
.PHONY: all install uninstall clean distclean test debug config run help
