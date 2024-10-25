#include "p_launcher.hpp"
#include <iostream>

ProcessLauncher::ProcessLauncher() {
}

ProcessLauncher::~ProcessLauncher() {
#ifdef _WIN32
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
#endif
}

bool ProcessLauncher::run(const char *command) {
#ifdef __WIN32
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  if (!CreateProcessA(NULL, (LPSTR)command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    std::cerr << "Failed to start process: " << GetLastError() << std::endl;
    return false;
  }
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
#ifdef __WIN32
  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  return static_cast<int>(exitCode);
#else
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return -1;
#endif
}

