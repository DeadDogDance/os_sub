#include "p_launcher.hpp"
#include <iostream>

ProcessLauncher::ProcessLauncher() {
}

ProcessLauncher::~ProcessLauncher() {
}

bool ProcessLauncher::run(const char *command) {
#ifdef _WIN32
#else
  pid = fork();
  switch(pid) {
  case -1:
    perror("fork");
    return false;
  case 0:
    execlp("/bin/sh", "sh", "-c", command, NULL);
    exit(EXIT_SUCCESS);
  }
#endif
  return true;
}

int ProcessLauncher::waitForExit() {
#ifdef _WIN32
#else
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return -1;
#endif
}

