# Makefile for DTMF PIN Detection Test Program

CC = gcc
CFLAGS = -Wall -Wextra -O2 -DHAVE_CONFIG_H $(shell pkg-config --cflags gstreamer-1.0)
LDFLAGS = $(shell pkg-config --libs gstreamer-1.0) -lglib-2.0

# Target executable
TARGET = test_dtmfpinsrc

# Source file
SOURCE = test_dtmfpinsrc.c

# Object file
OBJECT = $(TARGET).o

# Default target
all: $(TARGET)

# Build the test program
$(TARGET): $(OBJECT)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECT) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source
$(OBJECT): $(SOURCE)
	@echo "Compiling $(SOURCE)..."
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(OBJECT)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -f $(OBJECT) $(TARGET)
	@echo "Clean complete"

# Run test with default files
test: $(TARGET)
	@echo "Running test with dtmf_test_complete.wav..."
	./$(TARGET) ../dtmf_test_complete.wav ../codes.pin

# Install test program (optional)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Removing $(TARGET) from /usr/local/bin..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete"

.PHONY: all clean test install uninstall
