#!/bin/bash

cd logger

cmake -S . -B ./build
cmake --build ./build

cd ..
