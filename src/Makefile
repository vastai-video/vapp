# Makefile for compiling a libvappi.so

INSTALL_DIR := ../lib/
# Default target
.DEFAULT_GOAL := release


# Compiler and flags
CC := $(CC)
CFLAGS := -Wall -fPIC -I../include -Wno-missing-braces

# Source files
SRC := $(wildcard *.c)

# Object files
OBJ := $(SRC:.c=.o)

# Target library
TARGET := libvappi.so

# Build rule for debug version
# Debug flags
DEBUG_FLAGS := -g -DDEBUG -O0
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Build rule for release version
# Release flags
RELEASE_FLAGS := -O2
release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

LDFLAGS = -shared 
# Build rule
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(ANDROID) -c -o $@ $<


# Install rule
install: $(TARGET)
	cp $(TARGET) $(INSTALL_DIR)
	rm -f $(OBJ)  
# Clean rule
clean:
	rm -f $(OBJ) $(TARGET)
