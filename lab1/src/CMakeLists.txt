cmake_minimum_required(VERSION 3.20)

project(hello_world VERSION 1.0
                    DESCRIPTION "It just prints hello world"
                    LANGUAGES CXX)


add_executable(hello main.cpp hello.cpp hello.h)


