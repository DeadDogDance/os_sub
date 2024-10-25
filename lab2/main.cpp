#include "p_launcher.hpp"
#include <iostream>
#include <sstream>

int main(int argc, char *argv[]) {
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

    int exitCode = launcher.waitForExit();
    std::cout << "Process exited with code: " << exitCode << std::endl;
    return 0;
}

