@echo off

cd hello_world

cmake -S . -B ./build
cmake --build ./build

cd ..
