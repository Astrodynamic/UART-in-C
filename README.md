# 2UART_ver1.0

This project is a simple program that allows communication between two UART ports on a Linux system using the libserialport library.

## Build

To build the project, follow these steps:

1. Make sure you have the required libraries installed. You can install them by running the following command:

   ```bash
   sudo apt-get update
   sudo apt-get install libserialport-dev
   ```

2. Compile the project using the `gcc` compiler. Run the following command:

   ```bash
   gcc -Wall -Wextra -pedantic -std=c11 main.c -o 2UART_ver1.0
   ```

## Dependencies

This project requires the following libraries:

- libserialport-dev

Please make sure you have these libraries installed before building the project.

## Usage

1. Connect the UART devices to the respective COM ports on your Linux system.

2. Run the program using the following command:

   ```bash
   ./2UART_ver1.0
   ```

3. The program will prompt you to enter the COM port numbers for Thread No. 1 and Thread No. 2. Enter the numbers as per your configuration (1-99).

4. The program will open the specified COM ports, configure them with a baud rate of 115200, and create a system message queue.

5. The program will create two threads, each responsible for handling one UART port.

6. Once the threads are running, the program will start listening for packets on Thread No. 1 (COM port specified by the user).

7. To send a packet, type the packet data and press Enter. The program will validate the packet and send it to Thread No. 2 (COM port specified by the user).

8. The received packet will be displayed on the terminal.

9. You can repeat the process to send and receive packets between the two UART ports.

## Example

Here's an example usage of the program:

1. Enter the COM port number for Thread No. 1: 1
2. Enter the COM port number for Thread No. 2: 2
3. Start sending packets from Thread No. 1 by typing the packet data and pressing Enter.
4. Observe the received packets on Thread No. 2 displayed on the terminal.

## License

This project is licensed under the [MIT License](LICENSE). Feel free to modify and distribute it as per the terms of the license.