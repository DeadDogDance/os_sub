cmake_minimum_required(VERSION 3.15)
project(port_read_log_service)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_executable(port_simulator port_simulator.cpp)

add_executable(service service.cpp)

