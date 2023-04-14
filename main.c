#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 256

int fd1, fd2;                        // Дескрипторы файлов для COM портов
pthread_mutex_t mutex;               // Мьютекс для синхронизации доступа к COM портам
pthread_cond_t cond1, cond2;         // Условные переменные для ожидания появления данных в COM портах
pthread_t thread1, thread2;          // Идентификаторы потоков
int port1, port2;                    // Номера COM портов
char buf1[BUF_SIZE], buf2[BUF_SIZE]; // Буферы для чтения/записи данных из/в COM порты

/* Функция-обработчик первого потока */
void* thread_func1(void* arg __attribute__((unused))) {
    while (1) {
        // Ждем сигнала от основного потока о готовности к работе
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond1, &mutex);
        pthread_mutex_unlock(&mutex);

        // Считываем данные из COM порта 1
        int nread = read(fd1, buf1, BUF_SIZE - 1);
        if (nread > 0) {
            buf1[nread] = '\0';
            printf("Received from port %d: %s\n", port1, buf1);

            // Инвертируем байты данных
            for (int i = 0; i < nread; i += 2) {
                char tmp = buf1[i];
                buf1[i] = buf1[i + 1];
                buf1[i + 1] = tmp;
            }

            // Отправляем данные в COM порт 2
            write(fd2, buf1, nread);
        } else if (nread == -1) {
            perror("read");
        }
    }
}

/* Функция-обработчик второго потока */
void* thread_func2(void* arg __attribute__((unused))) {
    while (1) {
        // Ждем сигнала от основного потока о готовности к работе
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond2, &mutex);
        pthread_mutex_unlock(&mutex);

        // Считываем данные из COM порта 2
        int nread = read(fd2, buf2, BUF_SIZE - 1);
        if (nread > 0) {
            buf2[nread] = '\0';
            printf("Received from port %d: %s\n", port2, buf2);
        } else if (nread == -1) {
            perror("read");
        }
    }
}

/* Функция для настройки параметров COM порта */
void set_serial_port(int fd, int speed) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // Устанавливаем параметры порта
    tty.c_cflag &= ~PARENB;         // Без проверки четности
    tty.c_cflag &= ~CSTOPB;         // Один стоп-бит
    tty.c_cflag &= ~CSIZE;          // Маска битов данных
    tty.c_cflag |= CS8;             // 8 бит данных
    tty.c_cc[VMIN]  = 1;            // Минимальное количество символов для чтения
    tty.c_cc[VTIME] = 5;            // Таймаут ожидания ввода/вывода

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return;
    }
}

/* Функция для инициализации COM порта */
int init_serial_port(const char* port_name) {
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    set_serial_port(fd, B115200);

    return fd;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);

    // Получаем номера COM портов от пользователя
    printf("Enter port number for stream 1 (1-99): ");
    scanf("%d", &port1);

    printf("Enter port number for stream 2 (1-99): ");
    scanf("%d", &port2);

    // Инициализируем COM порты
    char port_name1[16], port_name2[16];
    sprintf(port_name1, "/dev/ttyUSB%d", port1);
    sprintf(port_name2, "/dev/ttyUSB%d", port2);

    fd1 = init_serial_port(port_name1);
    if (fd1 == -1) {
        exit(EXIT_FAILURE);
    }

    fd2 = init_serial_port(port_name2);
    if (fd2 == -1) {
        close(fd1);
        exit(EXIT_FAILURE);
    }

    // Создаем потоки
    int rc = pthread_create(&thread1, NULL, thread_func1, NULL);
    if (rc != 0) {
        perror("pthread_create");
        close(fd1);
        close(fd2);
        exit(EXIT_FAILURE);
    }

    rc = pthread_create(&thread2, NULL, thread_func2, NULL);
    if (rc != 0) {
        perror("pthread_create");
        close(fd1);
        close(fd2);
        exit(EXIT_FAILURE);
    }

    // Основной цикл программы
    while (1) {
        // Выводим приглашение к вводу сообщения для передачи
        printf("Insert TX packet: ");
        fflush(stdout);

        // Считываем строку из консоли
        char input_buf[BUF_SIZE];
        fgets(input_buf, BUF_SIZE, stdin);

        // Отправляем данные в COM порт 1
        pthread_mutex_lock(&mutex);
        strcpy(buf1, input_buf);
        write(fd1, buf1, strlen(input_buf));
        pthread_cond_signal(&cond1);
        pthread_cond_signal(&cond2);
        pthread_mutex_unlock(&mutex);
    }

    // Не дойдем до сюда, программа завершится по сигналу
    close(fd1);
    close(fd2);
    exit(EXIT_SUCCESS);
}
