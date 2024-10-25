#pragma once
#ifdef __WIN32
#include<windows.h>
#else // UNIX:
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif

class ProcessLauncher {
public:
  ProcessLauncher();
  ~ProcessLauncher();

  bool run(const char* command);

  int waitForExit();

  private:
#ifdef __WIN32
  PROCESS_INFORMATION pi;
#else
  pid_t pid;
#endif
};
