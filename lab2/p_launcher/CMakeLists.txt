cmake_minimum_required(VERSION 3.20)

project(process_launcher VERSION 1.0 
  DESCRIPTION "run process in background"
  LANGUAGES CXX)

add_library( p_lib STATIC
  p_launcher.cpp)

add_executable(p_t main.cpp)

target_link_libraries(p_t p_lib)

