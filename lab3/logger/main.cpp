#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <semaphore.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

const char *SHARED_MEM_NAME = "/shared_counter";
const char *SEMAPHORE_NAME = "/semaphore";
const char *PROCESS_COUNT_NAME = "/process_count";

class Logger *logger = nullptr;

class Logger {
public:
  Logger(const std::string &filename = "log.txt")
      : filename(filename), pid(getpid()), isMaster(false), running(true) {}
  ~Logger() { cleanup(); }

  int start() {
    int status = this->setup();
    if (status) {
      return status;
    }

    int fd;
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        std::cerr << "Error on file open/creation: " << strerror(errno)
                  << std::endl;
        break;
      }

      if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        isMaster = true;
      }

      if (isMaster) {
        status = write_in_file(fd, *counter);
      }
    }

    if (isMaster) {
      flock(fd, LOCK_UN);
    }
    close(fd);

    return 0;
  }

  void cleanup() {
    if (this->running) {
      this->running = false;
      if (counterThread.joinable()) {
        counterThread.join();
      }
      (*process_count)--;

      if (*process_count == 0) {
        munmap(this->counter, sizeof(int));
        shm_unlink(SHARED_MEM_NAME);
        shm_unlink(SEMAPHORE_NAME);
        shm_unlink(PROCESS_COUNT_NAME);
      }
      munmap(this->counter, sizeof(int));
      munmap(this->process_count, sizeof(int));
      close(this->shm_fd);
      close(this->process_count_fd);
      sem_close(this->semaphore);
    }
  }

private:
  pid_t pid;
  std::string filename;
  int shm_fd;
  int *counter;
  std::thread counterThread;
  bool running;
  bool isMaster;
  sem_t *semaphore;
  int process_count_fd;
  int *process_count;

private:
  int setup() {
    this->shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);

    if (this->shm_fd == -1) {
      std::cerr << "Error on creaing shared memmory for counter : "
                << strerror(errno) << std::endl;
      return 1;
    }

    ftruncate(this->shm_fd, sizeof(int));

    this->counter =
        static_cast<int *>(mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE,
                                MAP_SHARED, this->shm_fd, 0));

    if (this->counter == MAP_FAILED) {
      std::cerr << "Error on mapping shared memmory for counter: "
                << strerror(errno) << std::endl;
      return 1;
    }

    this->semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
    if (this->semaphore == SEM_FAILED) {
      std::cerr << "Error on creating semaphore: " << strerror(errno)
                << std::endl;
      return 1;
    }

    this->process_count_fd =
        shm_open(PROCESS_COUNT_NAME, O_CREAT | O_RDWR, 0666);
    if (this->process_count_fd == -1) {
      std::cerr << "Error on creating shared memmory for process counter: "
                << strerror(errno) << std::endl;
      return 1;
    }

    ftruncate(this->process_count_fd, sizeof(int));

    this->process_count =
        static_cast<int *>(mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE,
                                MAP_SHARED, this->process_count_fd, 0));
    if (this->process_count == MAP_FAILED) {
      std::cerr << "Error on mapping shared memmory for process counter: "
                << strerror(errno) << std::endl;
      return 1;
    }

    if (this->process_count == 0) {
      *this->counter = 0;
      *this->process_count = 0;
    }

    (*this->process_count)++;

    counterThread = std::thread(&Logger::incrementCounter, this);
    return 0;
  }

  std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

    std::ostringstream timeStream;
    timeStream << std::put_time(std::localtime(&now_time_t),
                                "%Y-%m-%d %H:%M:%S")
               << ":" << std::setfill('0') << std::setw(3) << now_ms.count();

    return timeStream.str();
  }

  int write_in_file(const int &fd, const int &currentCount) {
    std::string currentTime = getCurrentTime();

    std::string output = "PID: " + std::to_string(pid) +
                         ", Start Time: " + currentTime +
                         ", Count: " + std::to_string(currentCount) + "\n";

    if (write(fd, output.c_str(), output.size()) == -1) {
      std::cerr << "Error on writing in file: " << strerror(errno) << std::endl;
      flock(fd, LOCK_UN);
      close(fd);
      return 1;
    }
    return 0;
  }

  void incrementCounter() {
    while (this->running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      sem_wait(this->semaphore);
      (*this->counter)++;
      sem_post(this->semaphore);
    }
  }
};

class UserInterface {
public:
  UserInterface() {}

private:
};

void signal_handler(int signum) {
  if (logger) {
    logger->cleanup();
    delete logger;
    logger = nullptr;
  }
  exit(0);
}

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  logger = new Logger();
  logger->start();
  return 0;
}
