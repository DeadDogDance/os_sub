@echo off

cd hello_world

cmake -G "MinGW Makefiles" -S . -B ./build
cmake --build ./build

cd ..
