# Define variables for project name and required libraries
PROJECT_NAME := 2UART_ver1.0
LIB_NAMES := libserialport-dev

# Define compiler flags
CFLAGS := -Wall -Wextra -pedantic -std=c11

all: $(PROJECT_NAME)

# Update package lists and install required libraries using apt-get
install:
	sudo apt-get update
	sudo apt-get install $(LIB_NAMES)

# Compile the project with libserialport
$(PROJECT_NAME): main.c
	gcc $(CFLAGS) main.c -lserialport -o $(PROJECT_NAME)

# Clean up object files and executable
clean:
	rm -f $(PROJECT_NAME)
