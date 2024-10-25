@echo off

cd p_launcher 

cmake -G "MinGW Makefiles" -S . -B ./build
cmake --build ./build

cd ..
