#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#endif

#define LOG_DIR "logs/"
#define LOG_ALL LOG_DIR "all_measurements.log"
#define LOG_HOURLY LOG_DIR "hourly_avg.log"
#define LOG_DAILY LOG_DIR "daily_avg.log"

std::mutex all_mutex;
std::mutex hour_mutex;
std::mutex day_mutex;
std::mutex time_mutex;

struct Measurement {
  double temperature;
  std::time_t timestamp;
};

void create_log_directory() {
#ifdef _WIN32
  _mkdir(LOG_DIR);
#else
  mkdir(LOG_DIR, 0777);
#endif
}

#ifdef _WIN32
HANDLE open_serial_port(const std::string &port_name) {
  HANDLE hSerial = CreateFile(port_name.c_str(), GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, 0, NULL);
  if (hSerial == INVALID_HANDLE_VALUE) {
    std::cerr << "Error opening serial port" << std::endl;
    exit(EXIT_FAILURE);
  }
  return hSerial;
}

void close_serial_port(HANDLE hSerial) { CloseHandle(hSerial); }

bool read_temperature(HANDLE hSerial, double &temperature) {
  char buffer[16];
  DWORD bytesRead;
  if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
    buffer[bytesRead] = '\0';
    temperature = atof(buffer);
    return true;
  }
  return false;
}
#else
int open_serial_port(const std::string &port_name) {
  int fd = open(port_name.c_str(), O_RDONLY | O_NOCTTY);
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
  return fd;
}

void close_serial_port(int fd) { close(fd); }

bool read_temperature(int fd, double &temperature) {
  char buffer[16];
  int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  if (bytesRead > 0) {
    buffer[bytesRead] = '\0';
    temperature = atof(buffer);
    return true;
  }
  return false;
}
#endif

void log_measurement(const std::string &filename,
                     const Measurement &measurement) {

  std::lock_guard<std::mutex> guard(all_mutex);
  std::ofstream file(filename, std::ios::app);
  if (!file.is_open()) {
    std::cerr << "Error opening log file: " << filename << std::endl;
    exit(EXIT_FAILURE);
  }
  file << measurement.timestamp << " " << std::fixed << std::setprecision(2)
       << measurement.temperature << std::endl;
}

void lock_mutex(const std::string &filename) {
  if (filename == LOG_ALL) {
    all_mutex.lock();
  } else if (filename == LOG_HOURLY) {
    hour_mutex.lock();
  } else {
    day_mutex.lock();
  }
}

void unlock_mutex(const std::string &filename) {
  if (filename == LOG_ALL) {
    all_mutex.unlock();
  } else if (filename == LOG_HOURLY) {
    hour_mutex.unlock();
  } else {
    day_mutex.unlock();
  }
}

void calculate_and_log_average(const std::string &input_file,
                               const std::string &output_file,
                               std::time_t current_time, std::time_t interval) {
  double sum = 0.0;
  int count = 0;

  lock_mutex(input_file);
  std::ifstream input(input_file, std::ios::app);
  if (!input.is_open()) {
    std::cerr << "Error opening input file: " << input_file << std::endl;
    unlock_mutex(input_file);
    return;
  }

  std::time_t start_time = current_time - interval;
  std::time_t timestamp;
  double temperature;

  while (input >> timestamp >> temperature) {
    if (timestamp >= start_time && timestamp <= current_time) {
      sum += temperature;
      count++;
    }
  }

  input.close();
  unlock_mutex(input_file);

  lock_mutex(output_file);

  std::ofstream output(output_file, std::ios::app);
  if (!output.is_open()) {
    std::cerr << "Error opening output file: " << output_file << "\n"
              << "Details: file is open" << std::endl;
    unlock_mutex(output_file);
    return;
  }

  if (count > 0) {
    double average = sum / count;
    output << current_time << " " << std::fixed << std::setprecision(2)
           << average << std::endl;
  }

  output.close();
  unlock_mutex(output_file);
}

void cleanup_logs(const std::string &filename, std::time_t current_time,
                  int max_age) {

  lock_mutex(filename);
  std::ifstream input(filename);
  if (!input.is_open()) {
    unlock_mutex(filename);
    return;
  }

  std::string temp_filename = filename + ".tmp";
  std::ofstream temp(temp_filename, std::ios::app);
  if (!temp.is_open()) {
    unlock_mutex(filename);
    return;
  }

  std::time_t cutoff_time = current_time - max_age;
  std::time_t timestamp;
  double temperature;
  while (input >> timestamp >> temperature) {
    if (timestamp >= cutoff_time) {
      temp << timestamp << " " << std::fixed << std::setprecision(2)
           << temperature << std::endl;
    }
  }

  input.close();
  temp.close();

  remove(filename.c_str());
  rename(temp_filename.c_str(), filename.c_str());
  unlock_mutex(filename);
}

void read_data_from_port(const std::string &serial_port) {

  Measurement measurement;
#ifdef _WIN32
  HANDLE serial_fd = open_serial_port(serial_port);
#else
  int serial_fd = open_serial_port(serial_port);
#endif

  while (true) {
    double temperature;
    if (read_temperature(serial_fd, temperature)) {
      measurement.temperature = temperature;
      measurement.timestamp = time(nullptr);
      log_measurement(LOG_ALL, measurement);
    }
    std::time_t current_time = time(nullptr);
    time_mutex.lock();
    std::tm *tm_info = std::localtime(&current_time);
    time_mutex.unlock();

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  close_serial_port(serial_fd);
}

enum round_type { HOUR, DAY };
// Ну это кринге
time_t round_time(time_t t, round_type r_type) {
  time_mutex.lock();
  struct std::tm tm_info = *std::localtime(&t);
  time_mutex.unlock();

  switch (r_type) {
  case DAY:
    tm_info.tm_hour = 0;
    break;
  default:;
  }

  tm_info.tm_min = 0;
  tm_info.tm_sec = 0;
  return std::mktime(&tm_info);
}

void hourly_task() {
  std::this_thread::sleep_for(std::chrono::seconds(10));
  std::time_t current_time = time(nullptr);
  std::time_t last_time = time(nullptr);

  while (true) {
    current_time = time(nullptr);

    if (round_time(current_time, HOUR) - round_time(last_time, HOUR) >=
        60 * 60) {
      calculate_and_log_average(LOG_ALL, LOG_HOURLY,
                                round_time(current_time, HOUR), 60 * 60);
      last_time = current_time;
    }
  }
}

void daily_task() {
  std::this_thread::sleep_for(std::chrono::seconds(10));
  std::time_t current_time = time(nullptr);
  std::time_t last_time = time(nullptr);

  while (true) {
    current_time = time(nullptr);

    if (round_time(current_time, DAY) - round_time(last_time, DAY) >=
        60 * 60 * 24) {
      calculate_and_log_average(LOG_HOURLY, LOG_DAILY,
                                round_time(current_time, DAY), 60 * 60 * 24);
      last_time = current_time;
    }
  }
}

void clean_logs() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(20));
    std::time_t current_time = time(nullptr);
    cleanup_logs(LOG_ALL, current_time, 60 * 60 * 24);
    cleanup_logs(LOG_HOURLY, round_time(current_time, HOUR), 60 * 60 * 24 * 30);
    cleanup_logs(LOG_DAILY, round_time(current_time, DAY),
                 60 * 60 * 24 * 30 * 12);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <serial_port>" << std::endl;
    return 1;
  }

  std::string serial_port = argv[1];
  create_log_directory();

  std::thread reader_thread(read_data_from_port, serial_port);
  std::thread hourly_thread(hourly_task);
  std::thread daily_thread(daily_task);
  std::thread clean_thread(clean_logs);

  reader_thread.join();
  hourly_thread.join();
  daily_thread.join();
  clean_thread.join();

  return 0;
}
