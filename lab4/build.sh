#!/bin/bash

cmake -DCMAKE_CXX_COMPILER=g++ -S ./src -B ./build
cmake --build ./build
