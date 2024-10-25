cmake_minimum_required(VERSION 3.20)

project(process_launcher VERSION 1.0
                        DESCRIPTION "run process in background"
                        LANGUAGES CXX)


add_executable(p_t main.cpp p_launcher.cpp p_launcher.hpp)


if(CMAKE_SYSTEM_NAME STREQUAL "WINDOWS")
  add_custom_target(clean_all
    COMMAND cmd /c del /q "${CMAKE_BINARY_DIR}"
  )
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_custom_target(clean_all
    COMMAND rm -rf "${CMAKE_BINARY_DIR}"
  )
endif()

