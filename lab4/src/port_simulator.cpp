#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

void simulate_device(const char *port_name, int interval) {
#ifdef _WIN32
  HANDLE hSerial =
      CreateFile(port_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hSerial == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Error: Unable to open serial port %s\n", port_name);
    exit(EXIT_FAILURE);
  }
#else
  int fd = open(port_name, O_WRONLY | O_NOCTTY);
  if (fd == -1) {
    perror("Error opening serial port");
    exit(EXIT_FAILURE);
  }
  struct termios options;
  tcgetattr(fd, &options);
  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);
  options.c_cflag |= (CLOCAL | CREAD);
  tcsetattr(fd, TCSANOW, &options);
#endif

  srand((unsigned int)time(NULL)); // Инициализация генератора случайных чисел

  while (1) {
    double temperature = (rand() % 3000) / 100.0 -
                         10.0; // Генерация температуры от -10.00 до +20.00
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.2f\n", temperature);

#ifdef _WIN32
    DWORD bytesWritten;
    if (!WriteFile(hSerial, buffer, strlen(buffer), &bytesWritten, NULL)) {
      fprintf(stderr, "Error writing to serial port\n");
      break;
    }
#else
    if (write(fd, buffer, strlen(buffer)) == -1) {
      perror("Error writing to serial port");
      break;
    }
#endif

    printf("Simulated Temperature: %s",
           buffer); // Для отладки выводим в консоль
#ifdef _WIN32
    Sleep(interval * 1000); // Интервал в миллисекундах
#else
    sleep(interval); // Интервал в секундах
#endif
  }

#ifdef _WIN32
  CloseHandle(hSerial);
#else
  close(fd);
#endif
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <serial_port> [interval_seconds]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  const char *port_name = argv[1];
  int interval =
      argc > 2 ? atoi(argv[2]) : 1; // Интервал по умолчанию: 1 секунда

  simulate_device(port_name, interval);

  return 0;
}
