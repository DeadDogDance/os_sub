#pragma once
#ifdef __WIN32
#include <windows.h>
#else // UNIX:
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

class ProcessLauncher {
public:
  ProcessLauncher();
  ~ProcessLauncher();

  bool run(const char *command);

  int waitForExit();

private:
#ifdef __WIN32
  PROCESS_INFORMATION pi;
#else
  pid_t pid;
#endif
};
