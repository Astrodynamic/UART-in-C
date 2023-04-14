# Define variables for project name and required libraries
PROJECT_NAME := 2UART_ver1.0
LIB_NAMES := libserialport-dev

# Define compiler flags
CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11

all: $(PROJECT_NAME)

# Update package lists and install required libraries using apt-get
install:
	sudo apt-get update
	sudo apt-get install $(LIB_NAMES)

# Compile the project with libserialport
$(PROJECT_NAME): main.o
	$(CC) $(CFLAGS) main.c -o $(PROJECT_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

# Clean up object files and executable
clean:
	rm -f $(PROJECT_NAME)
	rm -f *.o

# sudo screen /dev/USBtty0 115200
# sudo screen /dev/USBtty1 115200