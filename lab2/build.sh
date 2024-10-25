#!/bin/bash

cd p_launcher

cmake -S . -B ./build
cmake --build ./build

cd ..
