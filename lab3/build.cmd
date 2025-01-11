@echo off

cd logger

cmake -G "MinGW Makefiles" -S . -B ./build
cmake --build ./build

cd ..
