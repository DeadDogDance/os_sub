#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

template <typename T> class SharedMemory;
class NamedSemaphore;

#define LOG_DIR "logs/"
#define LOG_FILE LOG_DIR "log.txt"
const char *SHM_COUNTER = "/counter";
const char *SHM_PROCESSES_COUNTER = "/processes_counter";
const char *COUNTER_SEMAPHORE = "/counter_semaphore";
const char *PROCESSES_COUNTER_SEMAPHORE = "/processes_counter_semaphore";
const char *SHM_MAIN_PID = "/main_pid";
const char *PID_SEMAPHORE = "/pid_semaphore";
const char *LOCALTIME_SEMAPHORE = "/localtime_semaphore";

std::atomic<bool> isRunning(true);

std::mutex file_mutex;

#ifdef _WIN32
using PidType = DWORD;
#else
using PidType = pid_t;
#endif

#ifdef _WIN32
DWORD getPid() {
  return GetCurrentProcessId();
#else
pid_t getPid() {
  return getpid();
#endif // DEBUG
}

PidType pid = getPid();
bool isMainProcess = false;
bool isMainThreadsStarted = false;

std::thread loggerThread;
std::thread spawnChildrenThread;
std::thread userInputThread;
std::thread counterThread;
std::thread checkMainPidThread;

SharedMemory<int> *shmCounter;
SharedMemory<int> *shmProcesses;
SharedMemory<PidType> *shmMainPid;

NamedSemaphore *countSemaphore;
NamedSemaphore *pidSemaphore;
NamedSemaphore *processesCounterSemaphore;
NamedSemaphore *localtimeSemaphore;

template <typename T> class SharedMemory {
public:
  SharedMemory(const std::string &name)
      : name_(name), size_(sizeof(T)), is_new_(false) {
#ifdef _WIN32
    handle_ = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name_.c_str());
    if (!handle_) {
      handle_ = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                  0, static_cast<DWORD>(size_), name_.c_str());
      if (!handle_) {
        throw std::runtime_error("Failed to create shared memory");
      }
      is_new_ = true;
    }
    memory_ = static_cast<T *>(
        MapViewOfFile(handle_, FILE_MAP_ALL_ACCESS, 0, 0, size_));
    if (!memory_) {
      CloseHandle(handle_);
      throw std::runtime_error("Failed to map shared memory");
    }
#else
    fd_ = shm_open(name_.c_str(), O_RDWR, 0666);
    if (fd_ == -1) {
      fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
      if (fd_ == -1) {
        throw std::runtime_error("Failed to create shared memory");
      }
      is_new_ = true;
    }
    if (is_new_ && ftruncate(fd_, size_) == -1) {
      close(fd_);
      shm_unlink(name.c_str());
      throw std::runtime_error("Failed to set shared memory size");
    }
    memory_ = static_cast<T *>(
        mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (memory_ == MAP_FAILED) {
      close(fd_);
      shm_unlink(name.c_str());
      throw std::runtime_error("Failed to map shared memory");
    }
#endif
  }

  ~SharedMemory() {
#ifdef _WIN32
    if (memory_) {
      UnmapViewOfFile(memory_);
    }
    if (handle_) {
      CloseHandle(handle_);
    }
#else
    if (memory_) {
      munmap(memory_, size_);
    }
    if (fd_ != -1) {
      close(fd_);
    }
    if (canDelete) {
      shm_unlink(name_.c_str());
      std::cout << "Unlink " << name_.c_str() << std::endl;
    }
    std::cout << "Desrcutor of " << name_.c_str() << std::endl;
#endif
  }

  T *data() { return memory_; }

  const T *data() const { return memory_; }

  std::size_t size() const { return size_; }

  bool isNew() const { return is_new_; }

  void setDelete(bool canDelete) { this->canDelete = canDelete; }

private:
  std::string name_;
  std::size_t size_;
  T *memory_ = nullptr;
  bool is_new_ = false;
  bool canDelete = false;

#ifdef _WIN32
  HANDLE handle_ = nullptr;
#else
  int fd_ = -1;
#endif
};

class NamedSemaphore {
public:
  NamedSemaphore(const std::string &name, unsigned int initialCount = 1)
      : name_(name), is_new_(false) {
#ifdef _WIN32

    // Try to open an existing semaphore
    semaphore_ = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, name.c_str());
    if (!semaphore_) {
      // If it doesn't exist, create a new semaphore
      semaphore_ =
          CreateSemaphore(nullptr, initialCount, LONG_MAX, name.c_str());
      if (semaphore_ && GetLastError() != ERROR_ALREADY_EXISTS) {
        is_new_ = true;
      }
    }
    if (!semaphore_) {
      throw std::runtime_error("Failed to create or open semaphore: " + name);
    }
#else
    // Try to open an existing semaphore
    semaphore_ = sem_open(name.c_str(), 0);
    if (semaphore_ == SEM_FAILED && errno == ENOENT) {
      // If it doesn't exist, create a new semaphore
      semaphore_ = sem_open(name.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR,
                            initialCount);
      if (semaphore_ != SEM_FAILED) {
        is_new_ = true;
      }
    }
    if (semaphore_ == SEM_FAILED) {
      throw std::runtime_error("Failed to create or open semaphore: " + name);
    }
#endif
  }

  ~NamedSemaphore() {
#ifdef _WIN32
    CloseHandle(semaphore_);
#else
    sem_close(semaphore_);
    if (canDelete) {
      sem_unlink(name_.c_str());
      std::cout << "Unlink " << name_.c_str() << std::endl;
    }
#endif
    std::cout << "Desrcutor of " << name_.c_str() << std::endl;
  }

  void wait() {
#ifdef _WIN32
    WaitForSingleObject(semaphore_, INFINITE);
#else
    sem_wait(semaphore_);
#endif
  }

  void signal() {
#ifdef _WIN32
    if (!ReleaseSemaphore(semaphore_, 1, nullptr)) {
      throw std::runtime_error("Failed to signal semaphore");
    }
#else
    sem_post(semaphore_);
#endif
  }

  void setDelete(bool canDelete) { this->canDelete = canDelete; }

private:
  std::string name_;
  bool is_new_;
  bool canDelete = false;
#ifdef _WIN32
  HANDLE semaphore_;
#else
  sem_t *semaphore_;
#endif
};

void create_log_directory() {
#ifdef _WIN32
  mkdir(LOG_DIR);
#else
  mkdir(LOG_DIR, 0777);
#endif
}

std::string getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) %
                1000;

  std::ostringstream timeStream;
  localtimeSemaphore->wait();
  timeStream << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
             << ":" << std::setfill('0') << std::setw(3) << now_ms.count();
  localtimeSemaphore->signal();
  return timeStream.str();
}

void checkMainPid() {
  while (!isMainProcess && isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    pidSemaphore->wait();
    if (*shmMainPid->data() == 0) {
      *shmMainPid->data() = pid;
      isMainProcess = true;
    }
    pidSemaphore->signal();
  }
}

void increaseCounter() {
  while (isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    countSemaphore->wait();
    (*shmCounter->data())++;
    countSemaphore->signal();
  }
}

void logMessage(const std::string &message) {
  std::lock_guard<std::mutex> lock(file_mutex);
  std::ofstream logFile;
  logFile.open(LOG_FILE, std::ios::app);
  logFile << message;
}

void logCount() {
  int count = 0;
  while (isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    countSemaphore->wait();
    count = *(shmCounter->data());
    countSemaphore->signal();
    std::string message = "PID: " + std::to_string(pid) +
                          ", Time: " + getCurrentTime() +
                          ", Count: " + std::to_string(count) + "\n";
    logMessage(message);
  }
}

void child1_task() {
  std::string message = "Start Child 1 - PID: " + std::to_string(getPid()) +
                        ", Time: " + getCurrentTime() + "\n";
  logMessage(message);

  countSemaphore->wait();
  *shmCounter->data() += 10;
  countSemaphore->signal();

  message = "End Child - PID: " + std::to_string(getPid()) +
            ", Time: " + getCurrentTime() + "\n";
  logMessage(message);
}

void child2_task() {
  std::string message = "Start Child 2 - PID: " + std::to_string(getPid()) +
                        ", Time: " + getCurrentTime() + "\n";
  logMessage(message);

  countSemaphore->wait();
  *shmCounter->data() *= 2;
  countSemaphore->signal();

  std::this_thread::sleep_for(std::chrono::seconds(2));

  countSemaphore->wait();
  *shmCounter->data() /= 2;
  countSemaphore->signal();

  message = "End Child - PID: " + std::to_string(getPid()) +
            ", Time: " + getCurrentTime() + "\n";
  logMessage(message);
}

void spawnChildren() {
  std::string message = "Child process still working\n";
  while (isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::thread child1(child1_task);
    std::thread child2(child2_task);

    if (child1.joinable() && child2.joinable()) {
      logMessage(message);
      child1.join();
      child2.join();
    }
  }
}

void userInput() {
  while (isRunning.load()) {

    std::cout << "input, pls" << std::endl;
    std::string input;
    std::getline(std::cin, input);

    int newValue = std::stoi(input);
    countSemaphore->wait();
    *shmCounter->data() = newValue;
    countSemaphore->signal();
  }
}

void cleanup() {
  if (checkMainPidThread.joinable()) {
    checkMainPidThread.join();
  }
  if (loggerThread.joinable()) {
    loggerThread.join();
  }
  if (spawnChildrenThread.joinable()) {
    spawnChildrenThread.join();
  }
  if (userInputThread.joinable()) {
    userInputThread.join();
  }
  if (counterThread.joinable()) {
    counterThread.join();
  }

  processesCounterSemaphore->wait();
  --(*shmProcesses->data());
  processesCounterSemaphore->signal();

  if (isMainProcess) {
    pidSemaphore->wait();
    *shmMainPid->data() = 0;
    pidSemaphore->signal();

    processesCounterSemaphore->wait();
    if (*shmProcesses->data() == 0) {
      shmProcesses->setDelete(true);
      delete shmProcesses;
      shmProcesses = nullptr;
    }
    processesCounterSemaphore->signal();

    if (!shmProcesses) {
      shmMainPid->setDelete(true);
      shmCounter->setDelete(true);
      delete shmMainPid;
      delete shmCounter;

      shmMainPid = nullptr;
      shmCounter = nullptr;

      localtimeSemaphore->setDelete(true);
      processesCounterSemaphore->setDelete(true);
      pidSemaphore->setDelete(true);
      countSemaphore->setDelete(true);

      delete localtimeSemaphore;
      delete processesCounterSemaphore;
      delete pidSemaphore;
      delete countSemaphore;

      localtimeSemaphore = nullptr;
      processesCounterSemaphore = nullptr;
      pidSemaphore = nullptr;
      countSemaphore = nullptr;
    }
  }
}

void signalHandler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "Signal received: " << signal << ", stopping program..."
              << std::endl;
    isRunning = false;
  }
}

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
    std::cout << "Signal received: " << signal << ", stopping program..."
              << std::endl;
    isRunning = false;
    return TRUE;
  }
  return FALSE;
}
#endif

int main() {
  create_log_directory();
#ifdef _WIN32
  // Register the signal handler for Windows
  SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
  // Register the signal handler for UNIX-like systems
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
#endif

  std::cout << "new shared memory. Initializing data...\n";
  shmCounter = new SharedMemory<int>(std::string(SHM_COUNTER));
  std::cout << "new shared memory. Initializing data...\n";
  shmProcesses = new SharedMemory<int>(std::string(SHM_PROCESSES_COUNTER));
  std::cout << "new shared memory. Initializing data...\n";
  shmMainPid = new SharedMemory<PidType>(std::string(SHM_MAIN_PID));

  if (shmCounter->isNew()) {
    std::cout << "Created new shared memory. Initializing data...\n";
    std::memset(shmCounter->data(), 0, shmCounter->size());
  }
  if (shmProcesses->isNew()) {
    std::cout << "Created new shared memory. Initializing data...\n";
    std::memset(shmProcesses->data(), 0, shmProcesses->size());
  }

  if (shmMainPid->isNew()) {
    std::cout << "Created new shared memory. Initializing data...\n";
    *shmMainPid->data() = pid;
    isMainProcess = true;
  }

  std::cout << "new shared Semaphore. Initializing data...\n";
  countSemaphore = new NamedSemaphore(std::string(COUNTER_SEMAPHORE));
  std::cout << "new shared Semaphore. Initializing data...\n";
  pidSemaphore = new NamedSemaphore(std::string(PID_SEMAPHORE));
  std::cout << "new shared Semaphore. Initializing data...\n";
  processesCounterSemaphore =
      new NamedSemaphore(std::string(PROCESSES_COUNTER_SEMAPHORE));
  std::cout << "new shared Semaphore. Initializing data...\n";
  localtimeSemaphore = new NamedSemaphore(std::string(LOCALTIME_SEMAPHORE));

  std::cout << "Increase process counter\n";
  processesCounterSemaphore->wait();
  ++(*shmProcesses->data());
  processesCounterSemaphore->signal();
  std::cout << "Increase process counter\n";

  std::string message =
      "PID: " + std::to_string(pid) + ", Time: " + getCurrentTime() + "\n";

  logMessage(message);

  counterThread = std::thread(increaseCounter);

  if (!isMainProcess) {
    checkMainPidThread = std::thread(checkMainPid);
  }

  std::cout << "Is main process: " << isMainProcess << "\n";
  while (isRunning.load()) {
    if (isMainProcess && !isMainThreadsStarted) {
      loggerThread = std::thread(logCount);
      spawnChildrenThread = std::thread(spawnChildren);
      userInputThread = std::thread(userInput);
      isMainThreadsStarted = true;
    }
  }

  cleanup();

  return 0;
}
