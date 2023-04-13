#include <errno.h>
#include <libserialport.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int MAX_COM_NUM = 99;
const int BUF_SIZE = 16;

// Define constants for serial port parameters
enum {
  BAUDRATE = 115200,
  DATA_BITS = 8,
  STOP_BITS = 1
};

// Define a struct to hold COM port data
typedef struct {
  int num;
  struct sp_port *port;
} com_port;

// Print an error message and exit the program with a specific error code
void error(const char *msg, com_port *ports, int num_ports, int exit_code) {
  fprintf(stderr, "%s: %s\n", msg, sp_last_error_message());
  for (int i = 0; i < num_ports; i++) {
    if (ports[i].port != NULL) {
      sp_close(ports[i].port);
      sp_free_port(ports[i].port);
    }
  }
  exit(exit_code);
}

// Open a COM port by number and return a pointer to the port struct
struct sp_port *open_port(int num) {
  char port_name[BUF_SIZE];
  sprintf(port_name, "COM%d", num);
  struct sp_port *port = NULL;
  if ((sp_get_port_by_name(port_name, &port)) != SP_OK || sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
    return NULL;
  }
  sp_set_baudrate(port, BAUDRATE);
  sp_set_parity(port, SP_PARITY_NONE);
  sp_set_bits(port, DATA_BITS);
  sp_set_stopbits(port, STOP_BITS);
  sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

  return port;
}

// Read a line of input from the user and parse it as an integer in range [min,
// max]
int get_int(const char *prompt, int min, int max) {
  char buf[BUF_SIZE];
  int num;
  while (1) {
    printf("%s", prompt);
    fgets(buf, BUF_SIZE, stdin);
    if (sscanf(buf, "%d", &num) == 1 && num >= min && num <= max) {
      return num;
    }
    printf("Error: invalid input\n");
  }
}

int main() {
  com_port ports[] = {
      {.num = get_int("Enter COM number for thread 1 (1-99): ", 1, MAX_COM_NUM)},
      {.num = get_int("Enter COM number for thread 2 (1-99): ", 1, MAX_COM_NUM)}};
  int num_ports = sizeof(ports) / sizeof(com_port);

  // Set up signal handler for graceful termination
  signal(SIGINT, exit);

  // Open all COM ports
  for (int i = 0; i < num_ports; i++) {
    ports[i].port = open_port(ports[i].num);
    if (ports[i].port == NULL) {
      error("Error opening port", ports, num_ports, -1);
    }
  }

  // Loop to read/write data through the COM ports
  char tx_data[8], rx_data[3];
  size_t bytes_written = 0, bytes_read = 0;
  while (1) {
    // Prompt user for input
    printf("Insert_TX_Packet:\n");
    fgets(tx_data, sizeof(tx_data), stdin);
    size_t len = strlen(tx_data);
    if (len > 0 && tx_data[len - 1] == '\n') {
      tx_data[len - 1] = 0;
    }

    // Write data to COM port 1
    sp_flush(ports[0].port, SP_BUF_BOTH);
    bytes_written = sp_nonblocking_write(ports[0].port, tx_data, strlen(tx_data));
    if (bytes_written != strlen(tx_data)) {
      error("Error writing data", ports, num_ports, -1);
    }

    // Read data from COM port 2
    sp_flush(ports[1].port, SP_BUF_BOTH);
    bytes_read = sp_nonblocking_read(ports[1].port, rx_data, sizeof(rx_data));
    if (bytes_read != sizeof(rx_data)) {
      error("Error reading data", ports, num_ports, -1);
    }

    // Process received data
    // unsigned char payload[2] = {rx_data[1], rx_data[0]};
    printf("ReceivePacket: [%02X][%02X][%02X]\n", rx_data[0], rx_data[1], rx_data[2]);
  }

  // Close all COM ports
  for (int i = 0; i < num_ports; i++) {
    sp_close(ports[i].port);
    sp_free_port(ports[i].port);
  }

  return 0;
}
