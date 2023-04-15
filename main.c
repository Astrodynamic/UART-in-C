#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define PACKET_SIZE 256      // Размер пакета
const char SYNCBYTE = 0x25;  // Значение синхробайта

struct msg_buffer {
  long mtype;
  char mtext[PACKET_SIZE];
};

int fd_com_1, fd_com_2;  // Дескрипторы файлов для com-портов
int sys_msg_id;  // Идентификатор системной очереди

void *thread1_handler(void *arg);
void *thread2_handler(void *arg);

int main() {
  int com_1, com_2;
  pthread_t thread_1, thread_2;

  // Получаем номер порта для потока No1 от пользователя
  printf("Введите номер COM для ПОТОК No1 число от 1-99: ");
  scanf("%d", &com_1);
  char device_1[20];
  snprintf(device_1, sizeof(device_1), "/dev/USBtty%d", com_1);

  // Получаем номер порта для потока No2 от пользователя
  printf("Введите номер COM для ПОТОК No2 число от 1-99: ");
  scanf("%d", &com_2);
  char device_2[20];
  snprintf(device_2, sizeof(device_2), "/dev/USBtty%d", com_2);

  // Открываем файлы для com-портов
  fd_com_1 = open(device_1, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_com_1 < 0) {
    perror("Error opening serial port");
    exit(1);
  }

  struct termios serial_1;
  tcgetattr(fd_com_1, &serial_1);
  cfsetispeed(&serial_1, B115200);
  cfsetospeed(&serial_1, B115200);
  serial_1.c_cflag |= (CLOCAL | CREAD);
  serial_1.c_cflag &= ~PARENB;
  serial_1.c_cflag &= ~CSTOPB;
  serial_1.c_cflag &= ~CSIZE;
  serial_1.c_cflag |= CS8;
  tcsetattr(fd_com_1, TCSANOW, &serial_1);

  fd_com_2 = open(device_2, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_com_2 < 0) {
    perror("Error opening serial port");
    exit(1);
  }

  struct termios serial_2;
  tcgetattr(fd_com_2, &serial_2);
  cfsetispeed(&serial_2, B115200);
  cfsetospeed(&serial_2, B115200);
  serial_2.c_cflag |= (CLOCAL | CREAD);
  serial_2.c_cflag &= ~PARENB;
  serial_2.c_cflag &= ~CSTOPB;
  serial_2.c_cflag &= ~CSIZE;
  tcsetattr(fd_com_2, TCSANOW, &serial_2);

  // Создаем системную очередь
  sys_msg_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (sys_msg_id < 0) {
    perror("Error creating system message queue");
    exit(1);
  }

  // Запускаем потоки
  pthread_create(&thread_1, NULL, thread1_handler, NULL);
  pthread_create(&thread_2, NULL, thread2_handler, NULL);

  printf("START\n");

  // Ждем завершения потоков
  pthread_join(thread_1, NULL);
  pthread_join(thread_2, NULL);

  return 0;
}

void *thread1_handler(void *arg __attribute__((unused))) {
  int read_bytes;
  char buffer[PACKET_SIZE] = {0};
  struct msg_buffer msg = {0};

  while (1) {
    // Получаем пакет от пользователя через ком-порт 1
    printf("Insert_TX_Packet:\n");
    if ((read_bytes = read(fd_com_1, buffer, sizeof(buffer))) != buffer[4] + 5) {
      printf("Error_TX_Packet\n");
      continue;
    }

    // Проверяем синхробайты
    if (buffer[0] != SYNCBYTE || buffer[1] != SYNCBYTE ||
        buffer[2] != SYNCBYTE || buffer[3] != SYNCBYTE) {
      printf("Error_TX_Packet\n");
      continue;
    }

    // Получаем длину пакета и полезную информацию
    int packet_len = (int)buffer[4];
    if (packet_len != PACKET_SIZE - 5) {
      printf("Error_TX_Packet\n");
      continue;
    }
    memcpy(msg.mtext, buffer, sizeof(buffer));

    // Отправляем сообщение в системную очередь
    msg.mtype = 1;
    if (msgsnd(sys_msg_id, &msg, sizeof(msg), 0) < 0) {
      perror("Error sending message to system queue");
      exit(1);
    }
  }
}

void *thread2_handler(void *arg __attribute__((unused))) {
  int write_bytes, rcvmsg_flags = 0;
  char buffer[PACKET_SIZE] = {0};
  struct msg_buffer msg = {0};

  while (1) {
    // Получаем сообщение из системной очереди
    if (msgrcv(sys_msg_id, &msg, sizeof(msg.mtext), 1, rcvmsg_flags) < 0) {
      perror("Error receiving message from system queue");
      exit(1);
    }

    buffer[0] = msg.mtext[4];
    // Инвертируем полезную информацию
    for (int i = 0; i < buffer[0]; ++i) {
      buffer[i + 1] = msg.mtext[PACKET_SIZE - i - 1];
    }

    // Отправляем пакет через ком-порт 2
    if ((write_bytes = write(fd_com_2, buffer, buffer[0] + 1)) != buffer[0] + 1) {
      perror("Error writing packet to com2 port");
      exit(1);
    }

    // Выводим принятый пакет в терминал
    printf("ReceivePacket: ");
    for (int i = 0; i < buffer[0] + 1; ++i) {
      printf("[%02X]", buffer[i]);
    }
    printf("\n");
  }
}
