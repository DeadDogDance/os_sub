@echo off

cmake -G "MinGW Makefiles" -S ./src -B ./build
cmake --build ./build
