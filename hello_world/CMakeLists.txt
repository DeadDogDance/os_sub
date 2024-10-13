cmake_minimum_required(VERSION 3.20)

project(hello_world VERSION 1.0
                    DESCRIPTION "It just prints hello world"
                    LANGUAGES CXX)


add_executable(hello main.cpp hello.cpp hello.h)


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

