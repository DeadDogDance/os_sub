#include "p_launcher.hpp"
#include <iostream>
#include <sstream>


int main(int argc, char *argv[]) {

  int pause_time = 5;

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <command>" << std::endl;
    return 1;
  }

  std::ostringstream command;
  for (int i = 1; i < argc; ++i) {
    command << argv[i];
    if (i < argc - 1) {
      command << " ";
    }
  }

  ProcessLauncher launcher;
  if (!launcher.run(command.str().c_str())) {
    std::cerr << "Failed to start command." << std::endl;
    return 1;
  }

  std::cout << "Process start, waiting " << pause_time << " seconds" << std::endl;

  #ifdef _WIN32
    Sleep(pause_time * 1000);
  #else
    sleep(pause_time);
  #endif

  std::cout << "Woke up" << std::endl;

  int exitCode = launcher.waitForExit();
  std::cout << "Process exited with code: " << exitCode << std::endl;
  return 0;
}

